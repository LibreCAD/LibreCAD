#!/usr/bin/env python3
"""P0 field-level differ: libdxfrw JSON dump (system-under-test) vs dwgTs (oracle).

Runs the `libdxfrw_json_dump` binary + scripts/dwgts_canon.py over a corpus and
diffs canonical-vs-canonical, keyed by handle (or TYPE#ordinal for pre-R13).
Reports 4 gap kinds: missing_entity, type_mismatch, missing_field, value_mismatch.

  scripts/dwgts_json_diff.py [SRC ...] --lc-dump PATH [--depth {0,1,2}]
      [--atol 1e-9] [--dwgts ~/dev/dwgTs] [--baseline F] [--check] [--write-baseline]
      [--verbose]

Exit 0 clean / 1 NEW gap vs baseline (with --check) / 2 setup error.
Scaffolding evolves scripts/dwgts_oracle.py (argparse + corpus-walk + exit codes).
"""
from __future__ import annotations
import argparse
import json
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dwgts_canon  # noqa: E402

L0 = {"handle", "type", "ownerHandle", "layer", "space", "linetype",
      "color", "color24", "ltypeScale", "visible", "block"}


def lc_dump(binp: Path, src: Path) -> dict:
    with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as tf:
        out = Path(tf.name)
    try:
        r = subprocess.run([str(binp), str(src), "-o", str(out)],
                           capture_output=True, text=True, timeout=180)
        if r.returncode != 0:
            raise RuntimeError(f"lc-dump failed ({r.returncode}): {r.stderr.strip()[:200]}")
        doc = json.loads(out.read_text())
    finally:
        out.unlink(missing_ok=True)

    # Key by handle if all handles are distinct+nonzero (mirrors the ts side);
    # otherwise fall back to TYPE#ordinal so pre-R13 files still diff.
    handles = [str(e.get("handle", "0")).upper() for e in doc.get("entities", [])]
    keyed: dict = {}
    if len(set(handles)) == len(handles) and all(h not in ("0", "") for h in handles):
        for e in doc.get("entities", []):
            keyed[str(e["handle"]).upper()] = e
    else:
        seen: dict = defaultdict(int)
        for e in doc.get("entities", []):
            k = f'{e["type"]}#{seen[e["type"]]}'
            seen[e["type"]] += 1
            keyed[k] = e
    objs = {str(o.get("handle", "0")).upper(): o for o in doc.get("objects", [])}
    return {"entities": keyed, "objects": objs, "version": doc.get("version")}


def _num_eq(a, b, atol):
    try:
        return abs(float(a) - float(b)) <= atol
    except (TypeError, ValueError):
        return a == b


def _val_eq(a, b, atol):
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(_val_eq(x, y, atol) for x, y in zip(a, b))
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return _num_eq(a, b, atol)
    return a == b


def diff_file(binp, dwgts, src, depth, atol, agg, verbose):
    try:
        lc = lc_dump(binp, src)
        ts = dwgts_canon.canon(dwgts, src)
    except Exception as ex:  # noqa: BLE001
        agg[("<file>", "<error>", "file_error")] += 1
        if verbose:
            print(f"  ERR  {src.name}: {ex}")
        return

    lce, tse = lc["entities"], ts["entities"]
    for k in set(tse) - set(lce):
        agg[(tse[k]["type"], "*", "missing_entity")] += 1
        if verbose:
            print(f"  MISS {src.name}: {k} {tse[k]['type']} (libdxfrw missed record)")
    for k in set(lce) & set(tse):
        a, b = lce[k], tse[k]
        if a.get("type") != b.get("type"):
            agg[(b.get("type"), "*", "type_mismatch")] += 1
            continue
        t = b.get("type")
        keys = set(b) - {"_key"}
        if depth < 1:
            keys &= L0
        for f in keys:
            if f in ("_key",):
                continue
            if f not in a:
                agg[(t, f, "missing_field")] += 1
            elif not _val_eq(a[f], b[f], atol):
                agg[(t, f, "value_mismatch")] += 1
                if verbose:
                    print(f"  DIFF {src.name}: {k} {t}.{f}  lc={a[f]}  ts={b[f]}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("sources", nargs="*")
    ap.add_argument("--lc-dump", required=True, help="path to built libdxfrw_json_dump")
    ap.add_argument("--dwgts", default=str(Path.home() / "dev" / "dwgTs"))
    ap.add_argument("--depth", type=int, choices=[0, 1, 2], default=1)
    ap.add_argument("--atol", type=float, default=1e-9)
    ap.add_argument("--baseline", default=None)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--write-baseline", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()

    dwgts = Path(a.dwgts).expanduser()
    binp = Path(a.lc_dump).expanduser()
    if not (dwgts / "dist" / "cli" / "cad-to-json.cjs").exists():
        print(f"[setup] dwgTs CLI not built at {dwgts}/dist", file=sys.stderr)
        return 2
    if not binp.exists():
        print(f"[setup] libdxfrw_json_dump not built at {binp}", file=sys.stderr)
        return 2

    srcs = [Path(s).expanduser() for s in a.sources] or [
        Path.home() / "doc" / "dwg", Path.home() / "doc" / "dwg2"]
    files = sorted(f for d in srcs if d.is_dir()
                   for f in list(d.glob("*.dwg")) + list(d.glob("*.dxf")))
    # Allow individual file arguments too.
    for s in a.sources:
        p = Path(s).expanduser()
        if p.is_file() and p.suffix.lower() in (".dwg", ".dxf") and p not in files:
            files.append(p)
    if not files:
        print(f"[setup] no .dwg/.dxf under {', '.join(map(str, srcs))}", file=sys.stderr)
        return 2

    agg: Counter = Counter()
    for f in files:
        diff_file(binp, dwgts, f, a.depth, a.atol, agg, a.verbose)

    print(f"\n== dwgts-json-diff: {len(files)} files, depth={a.depth} ==")
    for (t, fld, kind), n in agg.most_common():
        print(f"  {kind:<15} {t:<12} {fld:<18} {n:>6}")

    current = {f"{t}|{fld}|{kind}": n for (t, fld, kind), n in agg.items()}
    if a.write_baseline and a.baseline:
        Path(a.baseline).write_text(json.dumps(current, indent=2, sort_keys=True))
        print(f"[baseline] wrote {a.baseline}")
        return 0
    if a.check and a.baseline:
        base = json.loads(Path(a.baseline).read_text()) if Path(a.baseline).exists() else {}
        new = {k: v for k, v in current.items() if v > base.get(k, 0)}
        if new:
            print("\n== NEW/WORSENED gaps vs baseline (FAIL) ==")
            for k, v in sorted(new.items()):
                print(f"  {k}: {v} (was {base.get(k, 0)})")
            return 1
        print("\n== no new gaps vs baseline (OK) ==")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
