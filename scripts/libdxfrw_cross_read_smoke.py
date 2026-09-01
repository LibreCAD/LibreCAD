#!/usr/bin/env python3
#
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD (librecad.org)
# Copyright (C) 2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Run the local libdxfrw side of the DWG/DXF cross-read smoke contract.

The fixture list is deliberately small and source-relative so this check is
reproducible in CI and in developer build directories.  It validates the
headless JSON dump contract before a sibling dwg-parser receipt is compared:
the file opens, the version is identified, typed entity counts are stable, and
post-R13 entity handles are present and unique.  RAW object totals are
observed in receipts but are not asserted, because promoting an object from
RAW to a typed callback is an intended parity improvement.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any


FIXTURES: tuple[dict[str, Any], ...] = (
    {
        "id": "pre_r13_r11_tables",
        "path": "librecad/src/lib/filters/tests/testdata/pre_r13_r11_tables.dwg",
        "version": "AC1009",
        "entityCounts": {
            "ARC": 124,
            "CIRCLE": 50,
            "DIMENSION": 15,
            "INSERT": 19,
            "LINE": 1149,
            "POINT": 45,
            "POLYLINE": 35,
            "SOLID": 37,
            "TEXT": 118,
            "VIEWPORT": 1,
        },
        "uniqueNonzeroHandles": False,
    },
    {
        "id": "ocd_leader_r2000",
        "path": "librecad/src/lib/filters/tests/testdata/ocd_leader_r2000.dwg",
        "version": "AC1015",
        "entityCounts": {
            "IMAGE": 5,
            "LEADER": 1,
            "LINE": 4,
            "LWPOLYLINE": 1,
            "MLEADER": 1,
            "MTEXT": 1,
        },
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "assoc_surface_r2004",
        "path": "librecad/src/lib/filters/tests/testdata/assoc_surface_r2004.dwg",
        "version": "AC1018",
        "entityCounts": {
            "ELLIPSE": 2,
            "LINE": 4,
            "LWPOLYLINE": 4,
            "MESH": 2,
            "POINT": 1,
        },
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "acsh_r2007",
        "path": "librecad/src/lib/filters/tests/testdata/acsh_r2007.dwg",
        "version": "AC1021",
        "entityCounts": {"MTEXT": 1, "POLYLINE": 1},
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "assoc_constraints_r2013",
        "path": "librecad/src/lib/filters/tests/testdata/assoc_constraints_r2013.dwg",
        "version": "AC1027",
        "entityCounts": {"CIRCLE": 1, "LINE": 1},
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "visualstyle_r2013",
        "path": "librecad/src/lib/filters/tests/testdata/visualstyle_r2013.dwg",
        "version": "AC1027",
        "entityCounts": {"POINT": 1},
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "dynblock_point",
        "path": "librecad/src/lib/filters/tests/testdata/dynblock_point.dwg",
        "version": "AC1032",
        "entityCounts": {"CIRCLE": 3, "INSERT": 3, "LINE": 6},
        "uniqueNonzeroHandles": True,
    },
    {
        "id": "blockvisibility",
        "path": "librecad/src/lib/filters/tests/testdata/blockvisibility.dwg",
        "version": "AC1032",
        "entityCounts": {"CIRCLE": 13, "INSERT": 4, "LINE": 26, "LWPOLYLINE": 22},
        "uniqueNonzeroHandles": True,
    },
)


def digest_handles(handles: list[str]) -> str:
    payload = "\n".join(sorted(handles)).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def run_fixture(dump: Path, repo: Path, fixture: dict[str, Any]) -> dict[str, Any]:
    rel_path = Path(str(fixture["path"]))
    path = repo / rel_path
    result: dict[str, Any] = {
        "id": fixture["id"],
        "path": rel_path.as_posix(),
        "expectedVersion": fixture["version"],
        "status": "failed",
        "errors": [],
    }
    if not path.is_file():
        result["errors"].append(f"fixture is missing: {rel_path}")
        return result

    try:
        process = subprocess.run(
            [str(dump), str(path)],
            capture_output=True,
            text=True,
            timeout=180,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        result["errors"].append(f"dump failed to run: {exc}")
        return result

    if process.returncode != 0:
        result["errors"].append(f"dump exited with {process.returncode}")
    try:
        document = json.loads(process.stdout)
    except json.JSONDecodeError as exc:
        result["errors"].append(f"dump did not emit JSON: {exc}")
        if process.stderr.strip():
            result["stderr"] = process.stderr[-1000:]
        return result

    if not isinstance(document, dict):
        result["errors"].append("dump JSON root is not an object")
        return result
    if "error" in document:
        result["errors"].append(f"dump reported error: {document['error']}")
    if document.get("sourceFormat") != "dwg":
        result["errors"].append(f"sourceFormat is {document.get('sourceFormat')!r}, expected 'dwg'")
    if document.get("version") != fixture["version"]:
        result["errors"].append(
            f"version is {document.get('version')!r}, expected {fixture['version']!r}"
        )

    entities = document.get("entities")
    if not isinstance(entities, list):
        result["errors"].append("entities is not an array")
        entities = []
    elif any(not isinstance(item, dict) for item in entities):
        result["errors"].append("entities contains a non-object record")
    typed_entities = [item for item in entities if isinstance(item, dict)]
    if any(not str(item.get("type", "")) for item in typed_entities):
        result["errors"].append("entities contains a record without a type")
    actual_counts = dict(sorted(Counter(str(item.get("type", "")) for item in typed_entities).items()))
    expected_counts = dict(sorted(fixture["entityCounts"].items()))
    result["entityCounts"] = actual_counts
    if actual_counts != expected_counts:
        result["errors"].append(
            f"entity counts differ: actual={actual_counts}, expected={expected_counts}"
        )

    handles = [str(item.get("handle", "")).upper() for item in typed_entities]
    result["entityHandleCount"] = len(handles)
    result["uniqueEntityHandles"] = len(set(handles)) == len(handles)
    result["nonzeroEntityHandles"] = all(handle not in {"", "0"} for handle in handles)
    result["entityHandleSha256"] = digest_handles(handles) if handles else None
    if fixture["uniqueNonzeroHandles"]:
        if not result["uniqueEntityHandles"]:
            result["errors"].append("entity handles are not unique")
        if not result["nonzeroEntityHandles"]:
            result["errors"].append("entity handles contain zero or empty values")

    diagnostics = document.get("diagnostics")
    if not isinstance(diagnostics, dict):
        result["errors"].append("diagnostics is missing or not an object")
    else:
        result["diagnostics"] = diagnostics
        for key in ("entityParseFailures", "objectParseFailures"):
            if diagnostics.get(key, 0) != 0:
                result["errors"].append(f"{key}={diagnostics.get(key)!r}")

    objects = document.get("objects")
    if not isinstance(objects, list):
        result["errors"].append("objects is missing or not an array")
        result["objectCount"] = None
    else:
        result["objectCount"] = len(objects)
    if process.stderr.strip():
        result["stderr"] = process.stderr[-1000:]
    if not result["errors"]:
        result["status"] = "passed"
    return result


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--dump", type=Path, default=None, help="path to libdxfrw_json_dump")
    parser.add_argument("--output", type=Path, help="write a deterministic JSON receipt")
    parser.add_argument("--json", action="store_true", help="print the JSON receipt")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    dump_arg = args.dump or (Path(os.environ["LIBDXFRW_JSON_DUMP"]) if os.environ.get("LIBDXFRW_JSON_DUMP") else None)
    if dump_arg is None:
        print("error: pass --dump or set LIBDXFRW_JSON_DUMP", file=sys.stderr)
        return 2
    dump = dump_arg.resolve()
    if not dump.is_file() or not os.access(dump, os.X_OK):
        print(f"error: dump binary is not executable: {dump}", file=sys.stderr)
        return 2

    receipts = [run_fixture(dump, repo, fixture) for fixture in FIXTURES]
    failures = [receipt for receipt in receipts if receipt["status"] != "passed"]
    payload = {
        "schema": 1,
        "tool": "libdxfrw_cross_read_smoke",
        "status": "passed" if not failures else "failed",
        "fixtureCount": len(receipts),
        "failureCount": len(failures),
        "fixtures": receipts,
    }
    encoded = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.output:
        output = args.output if args.output.is_absolute() else repo / args.output
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded, encoding="utf-8")
    if args.json:
        print(encoded, end="")
    else:
        print(f"libdxfrw cross-read smoke: {payload['status']} ({len(receipts)} fixtures)")
        for receipt in receipts:
            print(f"  {receipt['status'].upper():7} {receipt['id']}")
            for error in receipt["errors"]:
                print(f"           {error}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
