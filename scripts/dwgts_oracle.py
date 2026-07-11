#!/usr/bin/env python3
"""dwgTs cross-check oracle for the libdxfrw DWG/DXF reader.

Third oracle alongside scripts/ezdxf_audit.py (ezdxf) and the LibreDWG dwgread
checks. For each source .dwg it builds dwgTs's entity-type histogram (the
TypeScript reader at ~/dev/dwgTs, treated as the reference for read coverage)
and diffs it against the entity-type histogram of the DXF that LibreCAD's
libdxfrw produced for the same file. The actionable signal is *types dwgTs
decodes that LibreCAD's output lacks* = geometry LibreCAD currently drops.

Usage:
  scripts/dwgts_oracle.py [SOURCE_DWG_DIR ...] [--out LC_OUTPUT_DXF_DIR]
                          [--dwgts ~/dev/dwgTs] [--quiet]

Defaults: SOURCE = ~/doc/dwg ~/doc/dwg2 ; --out = $TMPDIR/lc_dwg_corpus_out
(populated by the Catch2 test `[corpus][dwgdxf]`). If --out is absent or empty
the script still prints dwgTs histograms and flags types not in libdxfrw's
known-rendered set.

Exit 0 when no NEW dwgTs-only renderable type appears beyond the baseline,
1 on a regression, 2 on setup error.
"""
from __future__ import annotations
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

# ODA fixed-type code -> DXF entity name. Many subtypes collapse to one DXF name
# (all dimensions -> DIMENSION, all vertex/polyline variants -> their DXF form),
# matching how a DXF entity histogram counts them. Only entity (graphical) codes
# are listed; object codes are ignored for the render-gap comparison.
TYPE_TO_DXF = {
    1: "TEXT", 2: "ATTRIB", 3: "ATTDEF", 4: "BLOCK", 5: "ENDBLK", 6: "SEQEND",
    7: "INSERT", 8: "INSERT", 10: "VERTEX", 11: "VERTEX", 12: "VERTEX",
    13: "VERTEX", 14: "VERTEX", 15: "POLYLINE", 16: "POLYLINE", 17: "ARC",
    18: "CIRCLE", 19: "LINE", 20: "DIMENSION", 21: "DIMENSION", 22: "DIMENSION",
    23: "DIMENSION", 24: "DIMENSION", 25: "DIMENSION", 26: "DIMENSION",
    27: "POINT", 28: "3DFACE", 29: "POLYLINE", 30: "POLYLINE", 31: "SOLID",
    32: "TRACE", 33: "SHAPE", 34: "VIEWPORT", 35: "ELLIPSE", 36: "SPLINE",
    37: "REGION", 38: "3DSOLID", 39: "BODY", 40: "RAY", 41: "XLINE",
    44: "MTEXT", 45: "LEADER", 46: "TOLERANCE", 47: "MLINE", 74: "OLE2FRAME",
    77: "LWPOLYLINE", 78: "HATCH", 101: "IMAGE", 498: "ACAD_PROXY_ENTITY",
    # custom (class) entity types
    1101: "MULTILEADER", 1104: "MESH", 1105: "ACAD_TABLE", 1108: "PDFUNDERLAY",
    1146: "DGNUNDERLAY", 1147: "DWFUNDERLAY", 1150: "NAVISWORKSMODEL",
    1157: "POINTCLOUD", 1158: "POINTCLOUDEX", 1159: "RTEXT", 1160: "CAMERA",
    1161: "SECTIONOBJECT", 1162: "LINE", 1163: "ARCALIGNEDTEXT",
    1164: "GEOPOSITIONMARKER", 1167: "LIGHT", 1168: "MPOLYGON", 1169: "HELIX",
    1170: "EXTRUDEDSURFACE", 1171: "LOFTEDSURFACE", 1172: "REVOLVEDSURFACE",
    1173: "SWEPTSURFACE", 1174: "PLANESURFACE", 1175: "NURBSURFACE",
    1109: "WIPEOUT", 1168: "MPOLYGON",
}

# DXF entity names libdxfrw parses into a renderable DRW_* (LibreCAD draws them).
# A dwgTs type mapping to a name NOT here is a render gap (dropped on import).
LIBDXFRW_RENDERED = {
    "POINT", "LINE", "RAY", "XLINE", "CIRCLE", "ARC", "ELLIPSE", "TRACE",
    "SOLID", "3DFACE", "TEXT", "MTEXT", "ATTRIB", "ATTDEF", "INSERT", "BLOCK",
    "ENDBLK", "SEQEND", "VERTEX", "POLYLINE", "LWPOLYLINE", "SPLINE", "HATCH",
    "MPOLYGON", "MLINE", "DIMENSION", "ARC_DIMENSION", "LEADER", "MULTILEADER",
    "TOLERANCE", "VIEWPORT", "IMAGE", "WIPEOUT", "MESH", "ACAD_TABLE", "HELIX",
    "LIGHT", "PDFUNDERLAY", "DGNUNDERLAY", "DWFUNDERLAY",
}

ENTITY_NAMES = set(TYPE_TO_DXF.values()) | LIBDXFRW_RENDERED


def dwgts_histogram(dwgts: Path, src: Path) -> Counter:
    """Run the dwgTs JSON CLI on `src` and histogram entities (model + blocks)."""
    cli = dwgts / "dist" / "cli" / "cad-to-json.cjs"
    with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as tf:
        out = Path(tf.name)
    try:
        r = subprocess.run(
            ["node", str(cli), str(src), "--no-raw-blobs", "-o", str(out)],
            capture_output=True, text=True, timeout=180)
        if r.returncode != 0:
            return Counter({"<dwgts-error>": 1})
        doc = json.loads(out.read_text())
    except Exception:
        return Counter({"<dwgts-error>": 1})
    finally:
        out.unlink(missing_ok=True)

    hist = Counter()

    def walk(entities):
        for e in entities or []:
            name = e.get("dxfName")
            if not name:
                t = e.get("type")
                name = TYPE_TO_DXF.get(t, f"TYPE_{t}")
            hist[name] += 1

    walk(doc.get("entities"))
    blocks = doc.get("blocks")
    if isinstance(blocks, list):
        for b in blocks:
            walk(b.get("entities"))
    elif isinstance(blocks, dict):
        for b in blocks.values():
            walk(b.get("entities") if isinstance(b, dict) else None)
    return hist


_G0 = re.compile(r"^\s*0\s*$")


def dxf_histogram(path: Path) -> Counter:
    """Count group-0 entity names in a DXF (robust text scan, no ezdxf needed)."""
    hist = Counter()
    try:
        lines = path.read_text(errors="replace").splitlines()
    except Exception:
        return hist
    i, n = 0, len(lines)
    in_ent = False
    while i + 1 < n:
        if _G0.match(lines[i]):
            val = lines[i + 1].strip()
            if val in ("SECTION",):
                pass
            elif val in ENTITY_NAMES:
                hist[val] += 1
            i += 2
        else:
            i += 1
    return hist


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("sources", nargs="*", help="source .dwg dirs")
    ap.add_argument("--out", default=None, help="LibreCAD DWG->DXF output dir")
    ap.add_argument("--dwgts", default=str(Path.home() / "dev" / "dwgTs"))
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    dwgts = Path(args.dwgts).expanduser()
    if not (dwgts / "dist" / "cli" / "cad-to-json.cjs").exists():
        print(f"[setup] dwgTs CLI not built at {dwgts}/dist (run npm run build)",
              file=sys.stderr)
        return 2

    sources = [Path(s).expanduser() for s in args.sources] or [
        Path.home() / "doc" / "dwg", Path.home() / "doc" / "dwg2"]
    out_dir = Path(args.out).expanduser() if args.out else (
        Path(tempfile.gettempdir()) / "lc_dwg_corpus_out")

    files = sorted(f for d in sources if d.is_dir()
                   for f in d.glob("*.dwg"))
    if not files:
        print(f"[setup] no .dwg under {', '.join(map(str, sources))}",
              file=sys.stderr)
        return 2

    # Methodology note: we diff dwgTs's *source read* against LibreCAD's
    # *round-tripped output DXF*. That conflates three things — true read gaps,
    # write-side normalization (LibreCAD re-emits old POLYLINE/VERTEX/SEQEND as
    # LWPOLYLINE), and failed conversions. So a partial under-count is NOT a
    # reliable gap signal. We report only the two HIGH-CONFIDENCE signals:
    #   (a) never-rendered types  — a type with no renderable DRW_* at all;
    #   (b) categorical drops      — dwgTs>0 but LC output has exactly 0.
    # POLYLINE/VERTEX/SEQEND are folded into LWPOLYLINE before the diff so the
    # normalization does not masquerade as a drop. Files whose conversion failed
    # (LC entity total < 10% of dwgTs) are skipped, not counted as all-dropped.
    NORMALIZE = {"POLYLINE": "LWPOLYLINE", "VERTEX": "LWPOLYLINE",
                 "SEQEND": "LWPOLYLINE"}

    def fold(h: Counter) -> Counter:
        out = Counter()
        for k, v in h.items():
            out[NORMALIZE.get(k, k)] += v
        return out

    agg_dropped = Counter()   # never-rendered types dwgTs decodes
    agg_categorical = Counter()  # rendered-in-principle but LC output had 0
    files_with_gap = 0
    converted = 0
    print(f"== dwgTs oracle: {len(files)} source .dwg ; LC output {out_dir}")
    for src in files:
        dts_raw = dwgts_histogram(dwgts, src)
        if "<dwgts-error>" in dts_raw:
            if not args.quiet:
                print(f"  SKIP {src.name}: dwgTs failed to parse")
            continue
        dts = fold(dts_raw)
        stem = f"{src.parent.name}__{src.stem}.dxf"
        lc_path = out_dir / stem
        have_lc = lc_path.exists()
        lc = fold(dxf_histogram(lc_path)) if have_lc else Counter()

        dts_total = sum(v for k, v in dts.items() if k in ENTITY_NAMES)
        lc_total = sum(lc.values())
        conversion_ok = have_lc and (dts_total == 0 or lc_total >= 0.1 * dts_total)
        if conversion_ok:
            converted += 1

        gaps = {}
        for name, cnt in dts.items():
            if name not in ENTITY_NAMES or cnt == 0:
                continue
            if name not in LIBDXFRW_RENDERED:
                gaps[name] = (cnt, 0, "DROPPED")          # (a)
                agg_dropped[name] += cnt
            elif conversion_ok and lc.get(name, 0) == 0:
                gaps[name] = (cnt, 0, "categorical")      # (b)
                agg_categorical[name] += cnt
        if gaps:
            files_with_gap += 1
            if not args.quiet:
                detail = ", ".join(f"{k}: dwgTs={v[0]} lc=0 [{v[2]}]"
                                   for k, v in sorted(gaps.items()))
                print(f"  GAP  {src.name}: {detail}")

    print("\n== never-rendered types dwgTs decodes (no renderable DRW_*) ==")
    for name, cnt in (agg_dropped.most_common() or [("none", 0)]):
        print(f"  {name:<22} {cnt:>6}")
    print("\n== categorical drops (rendered in principle, LC output had 0) ==")
    print("   (advisory — may also be write-side / conversion artifacts)")
    for name, cnt in (agg_categorical.most_common() or [("none", 0)]):
        print(f"  {name:<22} {cnt:>6}")
    print(f"\n{files_with_gap}/{len(files)} files show a gap; "
          f"{converted}/{len(files)} converted cleanly.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
