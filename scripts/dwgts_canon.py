#!/usr/bin/env python3
"""dwgTs -> canonical-schema adapter for the P0 read-level diff.

Runs `node <dwgTs>/dist/cli/cad-to-json.cjs <src> --no-raw-blobs` and maps the
dwgTs document onto the SAME neutral field names the libdxfrw C++ dumper emits,
so scripts/dwgts_json_diff.py is a plain deep-compare with no per-library alias.

Pinned to dwgTs schemaVersion 3.0.0 (asserted). Emits:
  {file, sourceFormat, version, entities:{key->canon}, objects:{key->canon}}
Keying: handle when handles are distinct+nonzero; else "TYPE#ordinal"
(pre-R13 files emit handle "0" for every entity - see P0 DRIFT-2).

Field-name map (CANON):
  MUST-FIX #2: the TEXT/MTEXT/INSERT/DIMENSION groups follow dwgTs 3.0.0
  actual member names verified live -- `insertionPoint`, `angle`,
  `definitionPoint`, `textPoint`, `dimStyleHandle`, `blockHandle` --
  NOT the older insertionPt/rotation/defPoint/... spellings from round-1.
"""
from __future__ import annotations
import json
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

SCHEMA_EXPECTED = "3.0.0"

# ODA fixed-type code -> canonical type (mirrors dwgts_oracle.py:38-58, all dims
# fold to DIMENSION; vertex/polyline variants fold to POLYLINE).
TYPE_TO_CANON = {
    1: "TEXT", 7: "INSERT", 8: "INSERT", 17: "ARC", 18: "CIRCLE", 19: "LINE",
    20: "DIMENSION", 21: "DIMENSION", 22: "DIMENSION", 23: "DIMENSION",
    24: "DIMENSION", 25: "DIMENSION", 26: "DIMENSION", 27: "POINT",
    28: "3DFACE", 15: "POLYLINE", 16: "POLYLINE", 29: "POLYLINE", 30: "POLYLINE",
    10: "VERTEX", 11: "VERTEX", 12: "VERTEX", 13: "VERTEX", 14: "VERTEX",
    31: "SOLID", 32: "TRACE", 33: "SHAPE", 34: "VIEWPORT", 35: "ELLIPSE",
    36: "SPLINE", 40: "RAY", 41: "XLINE", 44: "MTEXT", 45: "LEADER",
    47: "MLINE", 74: "OLE2FRAME", 77: "LWPOLYLINE", 78: "HATCH", 101: "IMAGE",
    1101: "MLEADER", 1104: "MESH", 1105: "ACAD_TABLE", 1109: "WIPEOUT",
    1167: "LIGHT", 1168: "MPOLYGON", 1169: "HELIX",
}

# canonical field name -> dwgTs member name (from src/model/CadEntities.ts).
# The LINE/POINT/CIRCLE/ARC/ELLIPSE/SPLINE/HATCH/VIEWPORT/LWPOLYLINE/POLYLINE
# /IMAGE groups are verified correct as authored in round-1; the TEXT/MTEXT
# /INSERT/DIMENSION groups are corrected per MUST-FIX #2 to dwgTs 3.0.0 names.
CANON = {
    "LINE":       {"start": "start", "end": "end", "thickness": "thickness", "extrusion": "extrusion"},
    "POINT":      {"position": "position", "thickness": "thickness", "extrusion": "extrusion"},
    "CIRCLE":     {"center": "center", "radius": "radius", "extrusion": "extrusion"},
    "ARC":        {"center": "center", "radius": "radius", "startAngle": "startAngle",
                   "endAngle": "endAngle", "extrusion": "extrusion"},
    "ELLIPSE":    {"center": "center", "majorAxis": "majorAxis", "ratio": "radiusRatio",
                   "startParam": "startParam", "endParam": "endParam"},
    # MUST-FIX #2: `insertionPoint` (not insertionPt), `angle` (not rotation);
    # keep `styleHandle` as the dwgTs style ref (no `styleName` scalar).
    "TEXT":       {"insertionPt": "insertionPoint", "height": "height", "text": "text",
                   "rotation": "angle", "styleHandle": "styleHandle",
                   "widthScale": "widthFactor", "oblique": "oblique", "textgen": "textgen"},
    "MTEXT":      {"insertionPt": "insertionPoint", "height": "height", "text": "text",
                   "rotation": "angle", "width": "rectWidth", "styleHandle": "styleHandle"},
    # MUST-FIX #2: `blockHandle` (not `blockName` -- dwgTs has no block name on
    # the entity), `insertionPoint`, `angle`.
    "INSERT":     {"blockHandle": "blockHandle", "insertionPt": "insertionPoint",
                   "xScale": "xScale", "yScale": "yScale", "zScale": "zScale",
                   "rotation": "angle", "colCount": "columnCount", "rowCount": "rowCount"},
    "LWPOLYLINE": {"flags": "flags", "constWidth": "constantWidth", "elevation": "elevation",
                   "thickness": "thickness"},
    "SPLINE":     {"degree": "degree", "nControl": "controlPointCount", "nFit": "fitPointCount",
                   "nKnot": "knotCount"},
    "HATCH":      {"name": "patternName", "solid": "solidFill", "associative": "associative",
                   "angle": "patternAngle", "scale": "patternScale"},
    # MUST-FIX #2: `definitionPoint` (not defPoint), `textPoint` (not textMidPoint),
    # `dimStyleHandle` (not styleName).
    "DIMENSION":  {"defPoint": "definitionPoint", "textMidPt": "textPoint",
                   "dimStyleHandle": "dimStyleHandle", "dimText": "text"},
    "VIEWPORT":   {"center": "center", "width": "width", "height": "height",
                   "viewTarget": "viewTarget", "viewHeight": "viewHeight",
                   "status": "status", "id": "id"},
    "IMAGE":      {"insertionPt": "insertionPoint", "uVector": "uVector", "vVector": "vVector",
                   "clip": "clipping"},
}


def _flat(v):
    """dwgTs coords arrive as {'$xyz':[x,y,z]} / {'$xy':[x,y]} -> [x,y,z]."""
    if isinstance(v, dict):
        if "$xyz" in v:
            a = v["$xyz"]
            return [a[0], a[1], a[2] if len(a) > 2 else 0.0]
        if "$xy" in v:
            a = v["$xy"]
            return [a[0], a[1], 0.0]
    return v


def _canon_entity(e: dict) -> dict:
    name = e.get("dxfName") or TYPE_TO_CANON.get(e.get("type"), f"TYPE_{e.get('type')}")
    out = {"type": name, "handle": str(e.get("handle", "0")).upper()}
    for co, tsk in CANON.get(name, {}).items():
        if tsk in e:
            out[co] = _flat(e[tsk])
    return out


def run_dwgts(dwgts: Path, src: Path) -> dict:
    cli = dwgts / "dist" / "cli" / "cad-to-json.cjs"
    with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as tf:
        out = Path(tf.name)
    try:
        r = subprocess.run(["node", str(cli), str(src), "--no-raw-blobs", "-o", str(out)],
                           capture_output=True, text=True, timeout=180)
        if r.returncode != 0:
            raise RuntimeError(f"dwgTs failed: {r.stderr.strip()[:200]}")
        return json.loads(out.read_text())
    finally:
        out.unlink(missing_ok=True)


def canon(dwgts: Path, src: Path) -> dict:
    doc = run_dwgts(dwgts, src)
    sv = str(doc.get("schemaVersion", ""))
    if sv and sv != SCHEMA_EXPECTED:
        print(f"[warn] dwgTs schemaVersion {sv} != pinned {SCHEMA_EXPECTED} "
              f"(field names may have drifted)", file=sys.stderr)

    ents = list(doc.get("entities") or [])
    # Note: dwgTs 3.0.0 has no top-level `blocks` field (see P0 review defect
    # #7); the walk below is a defensive no-op preserved for pre-3.0 schemas.
    blocks = doc.get("blocks")
    if isinstance(blocks, list):
        for b in blocks:
            ents += list((b or {}).get("entities") or [])
    elif isinstance(blocks, dict):
        for b in blocks.values():
            if isinstance(b, dict):
                ents += list(b.get("entities") or [])

    ce = [_canon_entity(e) for e in ents]
    # key by handle if distinct+nonzero, else TYPE#ordinal (pre-R13 fallback).
    handles = [c["handle"] for c in ce]
    keyed: dict = {}
    if len(set(handles)) == len(handles) and all(h not in ("0", "") for h in handles):
        for c in ce:
            keyed[c["handle"]] = c
    else:
        seen: dict = defaultdict(int)
        for c in ce:
            k = f'{c["type"]}#{seen[c["type"]]}'
            seen[c["type"]] += 1
            c["_key"] = k
            keyed[k] = c

    objs: dict = {}
    for o in (doc.get("objects") or []):
        if not isinstance(o, dict):
            continue
        h = str(o.get("handle", "0")).upper()
        objs[h] = {"type": o.get("dxfName") or f'OBJ_{o.get("type")}', "handle": h}

    return {"file": src.name, "sourceFormat": doc.get("sourceFormat"),
            "version": doc.get("cadVersion"), "entities": keyed, "objects": objs}


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--dwgts", default=str(Path.home() / "dev" / "dwgTs"))
    a = ap.parse_args()
    print(json.dumps(canon(Path(a.dwgts).expanduser(), Path(a.src).expanduser()), indent=2))
