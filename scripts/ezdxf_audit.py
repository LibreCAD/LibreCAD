#!/usr/bin/env python3
"""ezdxf_audit.py — external DXF read/audit validator for the DXF-completeness work.

Given a directory of DXF files, ezdxf.readfile() + audit() each one and report:
  - read failures (exception while parsing)
  - non-unique-handle audit errors (and any other audit ERROR-level entries)
  - audit fixes (count, by class)

Optionally, given a paired INPUT directory (--input), compute a per-record-type
in -> out diff (count of each "0/<TYPE>" record in the ENTITIES + OBJECTS sections)
for files that exist in both directories (matched by basename), so a round-trip
through RS_FilterDXFRW can be checked for dropped/added types.

Usage:
  ezdxf_audit.py OUTDIR [--input INDIR] [--quiet]

Exit status:
  0  every file read OK and no audit ERROR entries (fixes are allowed/reported)
  1  at least one read failure or audit ERROR
  2  setup error (ezdxf missing, bad dir)

This intentionally reuses the audit logic proven during the DXF-completeness
session: a.errors must be empty; a.fixes are tolerated and counted.
"""

import argparse
import collections
import os
import sys

try:
    import ezdxf
    from ezdxf.audit import AuditError
except Exception as exc:  # pragma: no cover - environment guard
    sys.stderr.write(f"error: ezdxf import failed: {exc}\n")
    sys.exit(2)


def code_name(code):
    """Map an ezdxf AuditError integer code to its symbolic name (best effort)."""
    for name in dir(AuditError):
        if not name.startswith("_") and getattr(AuditError, name) == code:
            return name
    return str(code)


def list_dxf(directory):
    out = []
    for name in sorted(os.listdir(directory)):
        if name.lower().endswith(".dxf"):
            out.append(os.path.join(directory, name))
    return out


def count_record_types(path):
    """Count '0/<TYPE>' record markers across the whole DXF (group code 0)."""
    counts = collections.Counter()
    try:
        with open(path, "r", errors="replace") as fh:
            prev = None
            for raw in fh:
                line = raw.rstrip("\r\n").strip()
                if prev == "0":
                    counts[line] += 1
                prev = line
    except OSError:
        pass
    return counts


def audit_file(path):
    """Return (ok, n_errors, error_lines, n_fixes, fix_classes)."""
    try:
        doc = ezdxf.readfile(path)
    except Exception as exc:
        return (False, 0, [f"READ FAILURE: {exc}"], 0, collections.Counter())

    try:
        auditor = doc.audit()
    except Exception as exc:
        return (False, 0, [f"AUDIT EXCEPTION: {exc}"], 0, collections.Counter())

    error_lines = []
    for e in auditor.errors:
        # AuditError entries expose .code and .message
        msg = getattr(e, "message", str(e))
        code = getattr(e, "code", "?")
        error_lines.append(f"[{code}] {msg}")

    fix_classes = collections.Counter()
    for f in auditor.fixes:
        code = getattr(f, "code", "?")
        fix_classes[code] += 1

    ok = len(auditor.errors) == 0
    return (ok, len(auditor.errors), error_lines, len(auditor.fixes), fix_classes)


def main(argv):
    ap = argparse.ArgumentParser(description="ezdxf read/audit a directory of DXFs.")
    ap.add_argument("outdir", help="directory of DXF files to audit")
    ap.add_argument("--input", help="paired input directory for in->out type diff")
    ap.add_argument("--quiet", action="store_true", help="suppress per-file OK lines")
    args = ap.parse_args(argv)

    if not os.path.isdir(args.outdir):
        sys.stderr.write(f"error: not a directory: {args.outdir}\n")
        return 2

    files = list_dxf(args.outdir)
    if not files:
        sys.stderr.write(f"error: no .dxf files in {args.outdir}\n")
        return 2

    total = len(files)
    read_ok = 0
    error_files = 0
    total_errors = 0
    total_fixes = 0
    fix_totals = collections.Counter()

    print(f"== ezdxf {ezdxf.__version__} auditing {total} DXF file(s) in {args.outdir}")
    for path in files:
        base = os.path.basename(path)
        ok, n_err, err_lines, n_fix, fix_classes = audit_file(path)
        if not ok and any(l.startswith("READ FAILURE") or l.startswith("AUDIT EXCEPTION")
                          for l in err_lines):
            error_files += 1
            print(f"FAIL  {base}")
            for l in err_lines:
                print(f"        {l}")
            continue

        read_ok += 1
        total_errors += n_err
        total_fixes += n_fix
        fix_totals.update(fix_classes)
        if n_err:
            error_files += 1
            print(f"ERROR {base}  ({n_err} audit error(s), {n_fix} fix(es))")
            for l in err_lines:
                print(f"        {l}")
        elif not args.quiet:
            extra = f" ({n_fix} fix(es))" if n_fix else ""
            print(f"OK    {base}{extra}")

        if args.input:
            inpath = os.path.join(args.input, base)
            if os.path.isfile(inpath):
                in_counts = count_record_types(inpath)
                out_counts = count_record_types(path)
                keys = set(in_counts) | set(out_counts)
                diffs = []
                for k in sorted(keys):
                    a, b = in_counts.get(k, 0), out_counts.get(k, 0)
                    if a != b:
                        diffs.append(f"{k}: {a}->{b}")
                if diffs:
                    print(f"        DIFF {base}: " + ", ".join(diffs))

    print("== summary")
    print(f"   read OK:        {read_ok}/{total}")
    print(f"   files w/ error: {error_files}")
    print(f"   audit errors:   {total_errors}")
    print(f"   audit fixes:    {total_fixes}")
    if fix_totals:
        for code, n in fix_totals.most_common():
            print(f"      fix [{code_name(code)}]: {n}")

    return 0 if (error_files == 0 and total_errors == 0) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
