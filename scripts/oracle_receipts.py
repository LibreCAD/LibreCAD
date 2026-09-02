#!/usr/bin/env python3
# File: oracle_receipts.py

# /*
#  * ********************************************************************************
#  * This file is part of the LibreCAD project, a 2D CAD program
#  *
#  * Copyright (C) 2026 LibreCAD.org
#  * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

"""Validated, atomic external-oracle receipt helpers."""

from __future__ import annotations

import hashlib
import json
import os
import re
import tempfile
from pathlib import Path
from typing import Any


SAFE_COMPONENT = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*\Z")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
STATUSES = {"passed", "failed", "skipped", "missing-required", "error"}
FAILURE_CLASSES = {
    "none",
    "fixture-id",
    "fixture-integrity",
    "tool-unavailable",
    "tool-probe",
    "timeout",
    "oracle-rejected",
    "output-missing",
    "internal",
}


class ReceiptError(ValueError):
    """Raised when fixture selection or receipt structure is invalid."""


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ReceiptError(f"cannot read fixture manifest {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ReceiptError(f"invalid JSON in fixture manifest {path}: {exc}") from exc
    if not isinstance(data, dict) or not isinstance(data.get("fixtures"), list):
        raise ReceiptError(f"fixture manifest must contain a fixtures list: {path}")
    return data


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_component(value: object, label: str) -> str:
    if not isinstance(value, str) or not SAFE_COMPONENT.fullmatch(value):
        raise ReceiptError(f"{label} must contain only letters, digits, '.', '_' or '-'")
    return value


def fixture_identity(fixture: dict[str, Any]) -> dict[str, str]:
    fixture_id = safe_component(fixture.get("id"), "fixture id")
    path = fixture.get("path")
    version = fixture.get("version")
    format_name = fixture.get("format")
    digest = fixture.get("sha256")
    if not all(isinstance(value, str) and value for value in (path, version, format_name)):
        raise ReceiptError(f"fixture {fixture_id} is missing path, format, or version")
    if not isinstance(digest, str) or not SHA256.fullmatch(digest):
        raise ReceiptError(f"fixture {fixture_id} has an invalid SHA-256")
    return {
        "id": fixture_id,
        "path": path,
        "format": format_name,
        "version": version,
        "sha256": digest,
    }


def select_fixtures(manifest: dict[str, Any], fixture_ids: list[str],
                    default_only: bool) -> list[dict[str, Any]]:
    if default_only and fixture_ids:
        raise ReceiptError("--default-fixtures cannot be combined with --fixture")
    duplicates = {fixture_id for fixture_id in fixture_ids if fixture_ids.count(fixture_id) > 1}
    if duplicates:
        raise ReceiptError(f"duplicate fixture selector: {sorted(duplicates)[0]}")

    fixtures = manifest["fixtures"]
    by_id: dict[str, dict[str, Any]] = {}
    for entry in fixtures:
        if not isinstance(entry, dict):
            continue
        fixture_id = entry.get("id")
        if isinstance(fixture_id, str):
            if fixture_id in by_id:
                raise ReceiptError(f"fixture manifest has duplicate id: {fixture_id}")
            by_id[fixture_id] = entry

    if default_only:
        selected = [entry for entry in by_id.values() if entry.get("defaultEnabled") is True]
    else:
        unknown = [fixture_id for fixture_id in fixture_ids if fixture_id not in by_id]
        if unknown:
            raise ReceiptError(f"unknown fixture id: {unknown[0]}")
        selected = [by_id[fixture_id] for fixture_id in fixture_ids]
    if not selected:
        raise ReceiptError("no manifest fixtures selected")
    return sorted(selected, key=lambda fixture: str(fixture.get("id", "")))


def verify_repo_fixture(repo: Path, fixture: dict[str, Any]) -> Path:
    identity = fixture_identity(fixture)
    relative = Path(identity["path"])
    if relative.is_absolute() or "${" in identity["path"]:
        raise ReceiptError(f"fixture {identity['id']} is not repository-relative")
    root = repo.resolve()
    candidate = root / relative
    try:
        resolved = candidate.resolve(strict=True)
    except OSError as exc:
        raise ReceiptError(f"fixture {identity['id']} is missing: {identity['path']}") from exc
    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise ReceiptError(f"fixture {identity['id']} resolves outside the repository") from exc
    if not resolved.is_file():
        raise ReceiptError(f"fixture {identity['id']} is not a regular file")
    try:
        actual_hash = sha256_file(resolved)
    except OSError as exc:
        raise ReceiptError(f"cannot hash fixture {identity['id']}: {exc}") from exc
    if actual_hash != identity["sha256"]:
        raise ReceiptError(f"fixture {identity['id']} SHA-256 does not match the manifest")
    if identity["format"] == "DWG":
        try:
            magic = resolved.read_bytes()[:6]
            expected_magic = identity["version"].encode("ascii")
        except (OSError, UnicodeEncodeError) as exc:
            raise ReceiptError(f"cannot read DWG magic for fixture {identity['id']}") from exc
        if len(expected_magic) != 6 or magic != expected_magic:
            raise ReceiptError(f"fixture {identity['id']} DWG magic does not match the manifest")
    return resolved


def receipt_key(fixture: dict[str, Any], oracle_name: str, operation: str) -> str:
    return "--".join((fixture_identity(fixture)["id"],
                       safe_component(oracle_name, "oracle name"),
                       safe_component(operation, "operation")))


def validate_receipt(receipt: dict[str, Any]) -> None:
    if not isinstance(receipt, dict) or receipt.get("schema") != 1:
        raise ReceiptError("receipt schema must be 1")
    safe_component(receipt.get("receiptKey"), "receipt key")
    fixture = receipt.get("fixture")
    if not isinstance(fixture, dict):
        raise ReceiptError("receipt fixture must be an object")
    fixture_identity(fixture)

    operation = receipt.get("operation")
    if not isinstance(operation, dict):
        raise ReceiptError("receipt operation must be an object")
    safe_component(operation.get("name"), "operation name")
    if not isinstance(operation.get("sourceFormat"), str) or not operation["sourceFormat"]:
        raise ReceiptError("receipt operation requires sourceFormat")
    target_format = operation.get("targetFormat")
    target_version = operation.get("targetVersion")
    if (target_format is None) != (target_version is None):
        raise ReceiptError("receipt targetFormat and targetVersion must appear together")
    if target_format is not None and (not isinstance(target_format, str)
                                      or not isinstance(target_version, str)
                                      or not target_format or not target_version):
        raise ReceiptError("receipt target format/version must be non-empty strings")

    oracle = receipt.get("oracle")
    if not isinstance(oracle, dict):
        raise ReceiptError("receipt oracle must be an object")
    safe_component(oracle.get("name"), "oracle name")
    for field in ("path", "version", "mode"):
        if not isinstance(oracle.get(field), str) or not oracle[field]:
            raise ReceiptError(f"receipt oracle requires {field}")

    template = receipt.get("commandTemplate")
    if (not isinstance(template, list) or not template
            or any(not isinstance(value, str) or not value for value in template)):
        raise ReceiptError("receipt commandTemplate must be a non-empty string list")

    status = receipt.get("status")
    failure_class = receipt.get("failureClass")
    if status not in STATUSES or failure_class not in FAILURE_CLASSES:
        raise ReceiptError("receipt status or failureClass is invalid")
    if status == "passed" and failure_class != "none":
        raise ReceiptError("a passed receipt must have failureClass none")
    if status != "passed" and failure_class == "none":
        raise ReceiptError("a non-passed receipt requires a failureClass")

    diagnostics = receipt.get("diagnostics")
    if (not isinstance(diagnostics, list) or len(diagnostics) > 32
            or any(not isinstance(value, str) or len(value) > 1000 for value in diagnostics)):
        raise ReceiptError("receipt diagnostics must be at most 32 short strings")
    if "exitCode" in receipt and (not isinstance(receipt["exitCode"], int)
                                  or isinstance(receipt["exitCode"], bool)):
        raise ReceiptError("receipt exitCode must be an integer")

    output = receipt.get("output")
    if output is None:
        if target_format is not None:
            raise ReceiptError("a producing operation requires an output receipt")
    else:
        if target_format is None or not isinstance(output, dict):
            raise ReceiptError("receipt output requires a producing operation")
        if output.get("format") != target_format:
            raise ReceiptError("receipt output format must match operation targetFormat")
        digest = output.get("sha256")
        if not isinstance(digest, str) or not SHA256.fullmatch(digest):
            raise ReceiptError("receipt output requires a SHA-256")


def write_receipt_atomic(receipt_dir: Path, receipt: dict[str, Any]) -> Path:
    validate_receipt(receipt)
    receipt_dir.mkdir(parents=True, exist_ok=True)
    destination = receipt_dir / f"{receipt['receiptKey']}.json"
    descriptor = -1
    temporary = Path()
    try:
        descriptor, temporary_name = tempfile.mkstemp(
            prefix=f".{receipt['receiptKey']}.", suffix=".tmp", dir=receipt_dir)
        temporary = Path(temporary_name)
        with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
            descriptor = -1
            json.dump(receipt, stream, indent=2, sort_keys=True)
            stream.write("\n")
        os.replace(temporary, destination)
    except OSError as exc:
        raise ReceiptError(f"cannot write receipt {destination}: {exc}") from exc
    finally:
        if descriptor != -1:
            os.close(descriptor)
        if temporary and temporary.exists():
            temporary.unlink(missing_ok=True)
    return destination
