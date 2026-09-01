#!/usr/bin/env python3
# File: run_fixture_smoke.py

# /*
#  * ********************************************************************************
#  * This file is part of the LibreCAD project, a 2D CAD program
#  *
#  * Copyright (C) 2025 LibreCAD.org
#  * Copyright (C) 2025 Dongxu Li (github.com/dxli)
#  *
#  * This program is free software; you can redistribute it and/or
#  * modify it under the terms of the GNU General Public License
#  * as published by the Free Software Foundation; either version 2
#  * of the License, or (at your option) any later version.
#  *
#  * This program is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  * GNU General Public License for more details.
#  *
#  * You should have received a copy of the GNU General Public License
#  * along with this program; if not, write to the Free Software
#  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
#  * USA.
#  * ********************************************************************************
#  */

"""Run smoke audits for fixtures listed in the libdxfrw fixture manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

from dwg_inventory_common import repo_root_from_script


DEFAULT_MANIFEST = Path("tests/fixtures/fixture_manifest.json")


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"error: cannot read {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"error: invalid JSON in {path}: {exc}") from exc
    if not isinstance(data, dict) or not isinstance(data.get("fixtures"), list):
        raise SystemExit("error: manifest root must contain a fixtures list")
    return data


def selected_fixtures(repo: Path, manifest: dict[str, Any], default_only: bool) -> list[dict[str, Any]]:
    fixtures: list[dict[str, Any]] = []
    for index, fixture in enumerate(manifest.get("fixtures", [])):
        if not isinstance(fixture, dict):
            raise SystemExit(f"error: fixtures[{index}] is not an object")
        if default_only and fixture.get("defaultEnabled") is not True:
            continue
        raw_path = fixture.get("path")
        if not isinstance(raw_path, str) or not raw_path:
            raise SystemExit(f"error: {fixture.get('id', index)}: missing path")
        path = Path(raw_path)
        if path.is_absolute() or raw_path.startswith("${"):
            if fixture.get("defaultEnabled") is True:
                raise SystemExit(f"error: {fixture.get('id', index)}: default fixture path must be source-relative")
            continue
        full_path = repo / path
        if not full_path.is_file():
            if fixture.get("defaultEnabled") is True:
                raise SystemExit(f"error: {fixture.get('id', index)}: missing default fixture: {raw_path}")
            continue
        copy = dict(fixture)
        copy["_fullPath"] = str(full_path)
        fixtures.append(copy)
    return fixtures


def audit_binary(explicit: str | None, env_name: str) -> str:
    if explicit:
        return explicit
    return os.environ.get(env_name, "")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def dxf_header_version(path: Path) -> str | None:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    pairs = [(lines[index].strip(), lines[index + 1].strip())
             for index in range(0, len(lines) - 1, 2)]
    for index, (code, value) in enumerate(pairs[:-1]):
        if code == "9" and value == "$ACADVER" and pairs[index + 1][0] == "1":
            return pairs[index + 1][1]
    return None


def run_audit(tool: str, files: list[Path], allow_missing_tool: bool) -> tuple[dict[str, Any], int]:
    if not files:
        return {"status": "skipped", "diagnostics": ["no files for this format"]}, 0
    if not tool:
        status = "skipped" if allow_missing_tool else "error"
        code = 0 if allow_missing_tool else 2
        return {"status": status, "diagnostics": ["audit helper not configured"]}, code
    command = [tool, "--json", *(str(path) for path in files)]
    try:
        proc = subprocess.run(command, capture_output=True, text=True, timeout=300)
    except OSError as exc:
        return {"status": "error", "command": command, "diagnostics": [str(exc)]}, 2
    try:
        payload = json.loads(proc.stdout) if proc.stdout.strip() else {}
    except json.JSONDecodeError as exc:
        return {
            "status": "failed",
            "command": command,
            "exitCode": proc.returncode,
            "diagnostics": [f"invalid audit JSON: {exc}"],
            "stderr": proc.stderr[-4000:],
        }, 1
    failures = [
        str(item.get("fixture"))
        for item in payload.get("files", [])
        if isinstance(item, dict) and item.get("readOk") is False
    ]
    status = "passed" if proc.returncode == 0 and not failures else "failed"
    result = {
        "status": status,
        "command": command,
        "exitCode": proc.returncode,
        "files": len(files),
        "failedFixtures": failures,
        "stderr": proc.stderr[-4000:],
    }
    return result, 0 if status == "passed" else 1


def fixture_result(fixture: dict[str, Any], tool: str,
                   allow_missing_tool: bool) -> tuple[dict[str, Any], int]:
    path = Path(str(fixture["_fullPath"]))
    result: dict[str, Any] = {
        "id": fixture.get("id"),
        "format": fixture.get("format"),
        "version": fixture.get("version"),
        "operation": "read",
        "status": "failed",
        "diagnostics": [],
    }
    try:
        actual_hash = sha256_file(path)
    except OSError as exc:
        result["diagnostics"].append(str(exc))
        return result, 1
    result["sha256"] = actual_hash
    if actual_hash != fixture.get("sha256"):
        result["diagnostics"].append("fixture SHA-256 does not match manifest")
        return result, 1
    if fixture.get("format") == "DWG":
        version = fixture.get("version")
        try:
            magic = path.read_bytes()[:6]
        except OSError as exc:
            result["diagnostics"].append(str(exc))
            return result, 1
        if not isinstance(version, str) or magic != version.encode("ascii"):
            result["diagnostics"].append("DWG magic does not match manifest version")
            return result, 1
    elif fixture.get("format") == "DXF":
        version = fixture.get("version")
        if not isinstance(version, str) or dxf_header_version(path) != version:
            result["diagnostics"].append("DXF $ACADVER does not match manifest version")
            return result, 1

    audit, status = run_audit(tool, [path], allow_missing_tool)
    result["status"] = audit["status"]
    result["diagnostics"].extend(audit.get("diagnostics", []))
    result["audit"] = audit
    return result, status


def summarize_format(results: list[dict[str, Any]]) -> dict[str, Any]:
    statuses = [str(result.get("status", "failed")) for result in results]
    if not statuses:
        status = "skipped"
    elif any(status in {"failed", "error", "missing-required"} for status in statuses):
        status = "failed"
    elif all(status == "skipped" for status in statuses):
        status = "skipped"
    else:
        status = "passed"
    return {
        "status": status,
        "files": len(results),
        "passed": sum(status == "passed" for status in statuses),
        "skipped": sum(status == "skipped" for status in statuses),
        "failed": sum(status in {"failed", "error", "missing-required"}
                      for status in statuses),
    }


def print_text(results: dict[str, Any]) -> None:
    print(f"fixture smoke: {results['status']} ({results['selectedFixtures']} selected)")
    for name, result in results["audits"].items():
        print(f"{name}: {result['status']}")
        for diagnostic in result.get("diagnostics", []):
            print(f"  {diagnostic}")
        for fixture in result.get("failedFixtures", []):
            print(f"  failed: {fixture}")
    for fixture in results["fixtures"]:
        print(f"{fixture['id']}: {fixture['status']}")
        for diagnostic in fixture.get("diagnostics", []):
            print(f"  {diagnostic}")


def main(argv: list[str]) -> int:
    repo = repo_root_from_script(__file__)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=repo)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--default-only", action="store_true", help="smoke only default-enabled fixtures")
    parser.add_argument("--dwg-audit", help="path to built dwg_audit helper")
    parser.add_argument("--dxf-audit", help="path to built dxf_audit helper")
    parser.add_argument(
        "--allow-missing-audit-tools",
        action="store_true",
        help="skip selected fixtures instead of failing when audit helpers are absent",
    )
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    manifest_path = args.manifest if args.manifest.is_absolute() else repo / args.manifest
    fixtures = selected_fixtures(repo, load_manifest(manifest_path), args.default_only)
    fixture_results: list[dict[str, Any]] = []
    exit_codes: list[int] = []
    for fixture in fixtures:
        if fixture.get("format") == "DWG":
            tool = audit_binary(args.dwg_audit, "DWG_AUDIT")
        elif fixture.get("format") == "DXF":
            tool = audit_binary(args.dxf_audit, "DXF_AUDIT")
        else:
            fixture_results.append({
                "id": fixture.get("id"),
                "format": fixture.get("format"),
                "version": fixture.get("version"),
                "operation": "read",
                "status": "skipped",
                "diagnostics": ["no audit is defined for this format"],
            })
            continue
        result, status = fixture_result(fixture, tool,
                                        args.allow_missing_audit_tools)
        fixture_results.append(result)
        exit_codes.append(status)

    dwg_result = summarize_format(
        [result for result in fixture_results if result["format"] == "DWG"])
    dxf_result = summarize_format(
        [result for result in fixture_results if result["format"] == "DXF"])
    exit_code = 1 if 1 in exit_codes else 2 if 2 in exit_codes else 0
    status = "passed" if exit_code == 0 else "failed"
    payload = {
        "schema": 1,
        "tool": "run_fixture_smoke",
        "status": status,
        "selectedFixtures": len(fixtures),
        "audits": {
            "DWG": dwg_result,
            "DXF": dxf_result,
        },
        "fixtures": fixture_results,
    }
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print_text(payload)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
