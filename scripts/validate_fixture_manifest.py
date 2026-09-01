#!/usr/bin/env python3
# File: validate_fixture_manifest.py

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

"""Validate the default libdxfrw fixture manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

from dwg_inventory_common import repo_root_from_script


DEFAULT_MANIFEST = Path("tests/fixtures/fixture_manifest.json")
DEFAULT_ORACLES = Path("tests/fixtures/oracles.json")
VALID_ORACLE_MODES = {"optional", "required", "local-only"}
DEFAULT_DWG_VERSIONS = {
    "AC1015",
    "AC1018",
    "AC1021",
    "AC1024",
    "AC1027",
    "AC1032",
}
VALID_EVIDENCE_ROLES = {"reader", "writer-readback", "both"}

REQUIRED_FIELDS = {
    "id",
    "path",
    "format",
    "version",
    "sha256",
    "license",
    "redistribution",
    "defaultEnabled",
    "featureFamilies",
    "versionRows",
    "dwgTsRows",
    "writerRows",
    "expectedDiagnostics",
    "requiredOracles",
}


def load_json(path: Path) -> object:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"error: cannot read {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"error: invalid JSON in {path}: {exc}") from exc


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


def validate_fixture(repo: Path, fixture: object, index: int) -> list[str]:
    errors: list[str] = []
    if not isinstance(fixture, dict):
        return [f"fixtures[{index}] must be an object"]
    missing = sorted(REQUIRED_FIELDS - set(fixture))
    if missing:
        errors.append(f"{fixture.get('id', f'fixtures[{index}]')}: missing fields: {', '.join(missing)}")

    fixture_id = str(fixture.get("id", f"fixtures[{index}]"))
    rel_path = fixture.get("path")
    file_path: Path | None = None
    if not isinstance(rel_path, str) or not rel_path:
        errors.append(f"{fixture_id}: path must be a non-empty string")
    elif Path(rel_path).is_absolute() or rel_path.startswith("${"):
        if fixture.get("defaultEnabled") is True:
            errors.append(f"{fixture_id}: default fixture path must be source-relative")
    else:
        file_path = repo / rel_path
        if fixture.get("defaultEnabled") is True and not file_path.is_file():
            errors.append(f"{fixture_id}: default fixture file is missing: {rel_path}")
        expected_hash = fixture.get("sha256")
        if file_path.is_file() and isinstance(expected_hash, str) and expected_hash:
            actual = hashlib.sha256(file_path.read_bytes()).hexdigest()
            if actual != expected_hash:
                errors.append(f"{fixture_id}: sha256 mismatch for {rel_path}")

    if fixture.get("defaultEnabled") is True:
        sha = fixture.get("sha256")
        if not isinstance(sha, str) or len(sha) != 64 or any(ch not in "0123456789abcdef" for ch in sha):
            errors.append(f"{fixture_id}: default fixture requires a lowercase 64-character sha256")
        if fixture.get("license") in {"", None, "unknown"}:
            errors.append(f"{fixture_id}: default fixture requires a clear license")
        if fixture.get("redistribution") not in {"allowed", "repo", "public"}:
            errors.append(f"{fixture_id}: default fixture redistribution must allow CI use")
        role = fixture.get("evidenceRole")
        if role not in VALID_EVIDENCE_ROLES:
            errors.append(f"{fixture_id}: default fixture requires a valid evidenceRole")
        provenance = fixture.get("provenance")
        has_source_and_license = (isinstance(provenance, dict)
                                  and all(isinstance(provenance.get(field), str)
                                          and provenance[field]
                                          for field in ("source", "licenseBasis")))
        has_origin = (isinstance(provenance, dict)
                      and any(isinstance(provenance.get(field), str)
                              and provenance[field]
                              for field in ("importCommit", "createdBy")))
        if not has_source_and_license or not has_origin:
            errors.append(f"{fixture_id}: default fixture requires source, licenseBasis, and importCommit or createdBy provenance")
        expected_outcomes = fixture.get("expectedOutcomes")
        if expected_outcomes is not None and not isinstance(expected_outcomes, dict):
            errors.append(f"{fixture_id}: expectedOutcomes must be an object when present")
        if fixture.get("format") == "DWG":
            version = fixture.get("version")
            if not isinstance(version, str) or len(version) != 6:
                errors.append(f"{fixture_id}: default DWG version must be a six-byte magic")
            elif file_path is not None and file_path.is_file():
                magic = file_path.read_bytes()[:6]
                if magic != version.encode("ascii"):
                    errors.append(f"{fixture_id}: DWG magic {magic!r} does not match {version}")
        elif fixture.get("format") == "DXF":
            version = fixture.get("version")
            if not isinstance(version, str) or not version:
                errors.append(f"{fixture_id}: default DXF requires a declared version")
            elif file_path is not None and file_path.is_file():
                declared_version = dxf_header_version(file_path)
                if declared_version != version:
                    errors.append(f"{fixture_id}: DXF $ACADVER {declared_version!r} does not match {version}")

    for field in ("featureFamilies", "versionRows", "dwgTsRows", "writerRows", "expectedDiagnostics", "requiredOracles"):
        if field in fixture and not isinstance(fixture[field], list):
            errors.append(f"{fixture_id}: {field} must be a list")

    if fixture.get("format") not in {None, "DWG", "DXF", "synthetic"}:
        errors.append(f"{fixture_id}: format must be DWG, DXF, or synthetic")
    return errors


def validate_manifest(repo: Path, manifest_path: Path) -> list[str]:
    data = load_json(manifest_path)
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["manifest root must be an object"]
    if data.get("schema") != 1:
        errors.append("manifest schema must be 1")
    fixtures = data.get("fixtures")
    if not isinstance(fixtures, list):
        errors.append("fixtures must be a list")
        return errors

    seen: set[str] = set()
    default_reader_versions: set[str] = set()
    for index, fixture in enumerate(fixtures):
        if isinstance(fixture, dict):
            fixture_id = fixture.get("id")
            if isinstance(fixture_id, str):
                if fixture_id in seen:
                    errors.append(f"{fixture_id}: duplicate fixture id")
                seen.add(fixture_id)
            if (fixture.get("defaultEnabled") is True
                    and fixture.get("format") == "DWG"
                    and fixture.get("evidenceRole") in {"reader", "both"}
                    and isinstance(fixture.get("version"), str)):
                default_reader_versions.add(fixture["version"])
        errors.extend(validate_fixture(repo, fixture, index))
    missing_default_versions = DEFAULT_DWG_VERSIONS - default_reader_versions
    if missing_default_versions:
        errors.append("default DWG reader fixtures missing versions: "
                      + ", ".join(sorted(missing_default_versions)))
    return errors


def validate_oracles(oracles_path: Path) -> list[str]:
    if not oracles_path.exists():
        return [f"oracle config is missing: {oracles_path}"]
    data = load_json(oracles_path)
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["oracle config root must be an object"]
    for name in ("dwgts", "libredwg", "ezdxf", "oda"):
        entry = data.get(name)
        if not isinstance(entry, dict):
            errors.append(f"oracle {name}: entry must be an object")
            continue
        mode = entry.get("mode")
        if mode not in VALID_ORACLE_MODES:
            errors.append(f"oracle {name}: mode must be one of {', '.join(sorted(VALID_ORACLE_MODES))}")
        env = entry.get("env")
        if env is not None and (not isinstance(env, str) or not env):
            errors.append(f"oracle {name}: env must be a non-empty string when present")
        path = entry.get("path")
        if path is not None and (not isinstance(path, str) or not path):
            errors.append(f"oracle {name}: path must be a non-empty string when present")
        python = entry.get("python")
        if python is not None and (not isinstance(python, str) or not python):
            errors.append(f"oracle {name}: python must be a non-empty string when present")
        if name in {"dwgts", "libredwg", "oda"} and not entry.get("env") and not entry.get("path"):
            errors.append(f"oracle {name}: env or path must be configured")
        if name == "ezdxf" and not entry.get("python"):
            errors.append("oracle ezdxf: python must be configured")
    return errors


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=repo_root_from_script(__file__))
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--oracles", type=Path, default=DEFAULT_ORACLES)
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    manifest = args.manifest if args.manifest.is_absolute() else repo / args.manifest
    oracles = args.oracles if args.oracles.is_absolute() else repo / args.oracles
    errors = validate_manifest(repo, manifest)
    errors.extend(validate_oracles(oracles))
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"validated {manifest}")
    print(f"validated {oracles}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
