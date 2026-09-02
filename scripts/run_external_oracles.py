#!/usr/bin/env python3
# File: run_external_oracles.py

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

"""Run optional libdxfrw DWG/DXF external-oracle checks from a JSON config."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

from dwg_inventory_common import repo_root_from_script
from oracle_receipts import (
    ReceiptError,
    fixture_identity,
    load_manifest,
    receipt_key,
    select_fixtures,
    verify_repo_fixture,
    write_receipt_atomic,
)


DEFAULT_CONFIG = Path("tests/fixtures/oracles.json")
DEFAULT_MANIFEST = Path("tests/fixtures/fixture_manifest.json")
DEFAULT_RECEIPT_DIR = Path("tmp/external-oracles")
FATAL_DWGREAD = re.compile(r"Failed to decode|^ERROR 0x|Assertion failed", re.MULTILINE)

# Per-fixture receipts are intentionally narrower than the legacy recursive
# corpus runners below. Each adapter has one source format and one operation,
# allowing a default manifest batch to select a useful oracle for every format.
MANIFEST_ORACLE_ADAPTERS = {
    "libredwg": ("DWG", "open"),
    "ezdxf": ("DXF", "audit"),
}
DEFAULT_MANIFEST_ORACLES = {
    "DWG": ("libredwg",),
    "DXF": ("ezdxf",),
}


def load_json(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"error: cannot read {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"error: invalid JSON in {path}: {exc}") from exc
    if not isinstance(data, dict):
        raise SystemExit(f"error: oracle config root must be an object: {path}")
    return data


def mode(config: dict[str, Any], name: str) -> str:
    entry = config.get(name, {})
    if not isinstance(entry, dict):
        return "optional"
    return str(entry.get("mode", "optional"))


def path_from_env(entry: dict[str, Any], fallback_env: str | None = None) -> str:
    env_name = str(entry.get("env", "") or "")
    if env_name and os.environ.get(env_name):
        return os.environ[env_name]
    if fallback_env and os.environ.get(fallback_env):
        return os.environ[fallback_env]
    configured = str(entry.get("path", "") or "")
    if configured:
        return configured
    return ""


def finish_missing(result: dict[str, Any], required: bool, reason: str) -> int:
    result["status"] = "missing-required" if required else "skipped"
    result["diagnostics"].append(reason)
    return 2 if required else 0


def run_command(result: dict[str, Any], command: list[str], cwd: Path | None = None) -> int:
    result["command"] = command
    try:
        proc = subprocess.run(command, cwd=cwd, capture_output=True, text=True, timeout=300)
    except OSError as exc:
        result["status"] = "error"
        result["diagnostics"].append(str(exc))
        return 2
    except subprocess.TimeoutExpired as exc:
        result["status"] = "failed"
        result["diagnostics"].append(f"timeout after {exc.timeout}s")
        return 1
    result["exitCode"] = proc.returncode
    result["stdout"] = proc.stdout[-4000:]
    result["stderr"] = proc.stderr[-4000:]
    result["status"] = "passed" if proc.returncode == 0 else "failed"
    return 0 if proc.returncode == 0 else 1


def list_files(corpus: Path, suffix: str) -> list[Path]:
    if not corpus.is_dir():
        return []
    return sorted(path for path in corpus.rglob(f"*{suffix}") if path.is_file())


def run_dwgts(repo: Path, config: dict[str, Any], corpus: Path, quiet: bool) -> tuple[dict[str, Any], int]:
    entry = config.get("dwgts", {})
    entry = entry if isinstance(entry, dict) else {}
    required = mode(config, "dwgts") == "required"
    result = {"name": "dwgts", "mode": mode(config, "dwgts"), "status": "not-run", "diagnostics": []}
    dwgts_path = path_from_env(entry, "DWGTS_CLI")
    if not dwgts_path:
        return result, finish_missing(result, required, "DWGTS_CLI is not configured")
    candidate = Path(dwgts_path).expanduser()
    if candidate.name == "cad-to-json.cjs":
        candidate = candidate.parents[2]
    if not (candidate / "dist/cli/cad-to-json.cjs").is_file():
        return result, finish_missing(result, required, f"dwgTs CLI not built under {candidate}")
    dwg_files = list_files(corpus, ".dwg")
    if not dwg_files:
        result["status"] = "skipped"
        result["diagnostics"].append(f"no .dwg files under {corpus}")
        return result, 0
    command = [sys.executable, str(repo / "scripts/dwgts_oracle.py"), str(corpus), "--dwgts", str(candidate)]
    if quiet:
        command.append("--quiet")
    return result, run_command(result, command, cwd=repo)


def run_ezdxf(repo: Path, config: dict[str, Any], corpus: Path, quiet: bool) -> tuple[dict[str, Any], int]:
    entry = config.get("ezdxf", {})
    entry = entry if isinstance(entry, dict) else {}
    required = mode(config, "ezdxf") == "required"
    result = {"name": "ezdxf", "mode": mode(config, "ezdxf"), "status": "not-run", "diagnostics": []}
    if not list_files(corpus, ".dxf"):
        result["status"] = "skipped"
        result["diagnostics"].append(f"no .dxf files under {corpus}")
        return result, 0
    python = str(entry.get("python", "") or sys.executable)
    if not shutil.which(python) and not Path(python).exists():
        return result, finish_missing(result, required, f"python executable not found: {python}")
    command = [python, str(repo / "scripts/ezdxf_audit.py"), str(corpus)]
    if quiet:
        command.append("--quiet")
    return result, run_command(result, command, cwd=repo)


def run_libredwg(config: dict[str, Any], corpus: Path) -> tuple[dict[str, Any], int]:
    entry = config.get("libredwg", {})
    entry = entry if isinstance(entry, dict) else {}
    required = mode(config, "libredwg") == "required"
    result = {"name": "libredwg", "mode": mode(config, "libredwg"), "status": "not-run", "diagnostics": []}
    dwgread = path_from_env(entry, "DWGREAD")
    if not dwgread:
        return result, finish_missing(result, required, "LIBREDWG_DWGREAD/DWGREAD is not configured")
    dwgread_path = Path(dwgread).expanduser()
    if not dwgread_path.is_file() or not os.access(dwgread_path, os.X_OK):
        return result, finish_missing(result, required, f"dwgread is not executable: {dwgread_path}")
    dwg_files = list_files(corpus, ".dwg")
    if not dwg_files:
        result["status"] = "skipped"
        result["diagnostics"].append(f"no .dwg files under {corpus}")
        return result, 0

    failures: list[str] = []
    result["command"] = [str(dwgread_path), "<fixture>"]
    result["checkedFiles"] = len(dwg_files)
    for path in dwg_files:
        proc = subprocess.run([str(dwgread_path), str(path)], capture_output=True, text=True, timeout=120)
        output = proc.stdout + proc.stderr
        if proc.returncode != 0 or FATAL_DWGREAD.search(output):
            failures.append(f"{path}: exit={proc.returncode}")
    if failures:
        result["status"] = "failed"
        result["diagnostics"].extend(failures[:20])
        return result, 1
    result["status"] = "passed"
    return result, 0


def run_oda(repo: Path, config: dict[str, Any], corpus: Path) -> tuple[dict[str, Any], int]:
    entry = config.get("oda", {})
    entry = entry if isinstance(entry, dict) else {}
    required = mode(config, "oda") == "required"
    result = {"name": "oda", "mode": mode(config, "oda"), "status": "not-run", "diagnostics": []}
    oda = path_from_env(entry, "ODAFC")
    if not oda:
        oda = path_from_env(entry, "ODA_FILE_CONVERTER")
    if not oda:
        return result, finish_missing(result, required, "ODA_FILE_CONVERTER/ODAFC is not configured")
    oda_path = Path(oda).expanduser()
    if not oda_path.is_file() or not os.access(oda_path, os.X_OK):
        return result, finish_missing(result, required, f"ODA File Converter is not executable: {oda_path}")
    if not list_files(corpus, ".dwg") and not list_files(corpus, ".dxf"):
        result["status"] = "skipped"
        result["diagnostics"].append(f"no DWG/DXF files under {corpus}")
        return result, 0
    output = repo / "tmp/oracle-oda"
    command = [str(repo / "scripts/oda-validate.sh"), str(corpus), str(output)]
    env = os.environ.copy()
    env["ODAFC"] = str(oda_path)
    result["command"] = command
    try:
        proc = subprocess.run(command, cwd=repo, env=env, capture_output=True, text=True, timeout=300)
    except OSError as exc:
        result["status"] = "error"
        result["diagnostics"].append(str(exc))
        return result, 2
    result["exitCode"] = proc.returncode
    result["stdout"] = proc.stdout[-4000:]
    result["stderr"] = proc.stderr[-4000:]
    result["status"] = "passed" if proc.returncode == 0 else "failed"
    return result, 0 if proc.returncode == 0 else 1


def print_text(results: list[dict[str, Any]]) -> None:
    for result in results:
        line = f"{result['name']}: {result['status']} ({result['mode']})"
        if result.get("checkedFiles") is not None:
            line += f" files={result['checkedFiles']}"
        print(line)
        for diagnostic in result.get("diagnostics", []):
            print(f"  {diagnostic}")


def receipt_template(fixture: dict[str, Any], oracle_path: str,
                     oracle_version: str, oracle_mode: str,
                     status: str, failure_class: str,
                     diagnostics: list[str], *, oracle_name: str = "libredwg",
                     operation: str = "open", source_format: str = "DWG",
                     command_template: list[str] | None = None) -> dict[str, Any]:
    """Build a non-producing per-fixture external-oracle receipt."""
    if command_template is None:
        command_template = ["<dwgread>", "<fixture>"]
    return {
        "schema": 1,
        "receiptKey": receipt_key(fixture, oracle_name, operation),
        "fixture": fixture_identity(fixture),
        "operation": {
            "name": operation,
            "sourceFormat": source_format,
        },
        "oracle": {
            "name": oracle_name,
            "path": oracle_path,
            "version": oracle_version,
            "mode": oracle_mode,
        },
        "commandTemplate": command_template,
        "status": status,
        "failureClass": failure_class,
        "diagnostics": diagnostics,
    }


def libredwg_tool(config: dict[str, Any]) -> tuple[str, Path | None]:
    entry = config.get("libredwg", {})
    entry = entry if isinstance(entry, dict) else {}
    configured = path_from_env(entry, "DWGREAD")
    if not configured:
        return "not-configured", None
    candidate = Path(configured).expanduser()
    if not candidate.is_file() or not os.access(candidate, os.X_OK):
        return str(candidate), None
    return str(candidate.resolve()), candidate.resolve()


def probe_command_version(command: list[str]) -> tuple[str, str | None]:
    """Return a compact version string without making a later open fail."""
    try:
        proc = subprocess.run(command, capture_output=True, text=True, timeout=15)
    except (OSError, subprocess.TimeoutExpired) as exc:
        return "unknown", f"version probe unavailable: {type(exc).__name__}"
    version = (proc.stdout or proc.stderr).strip().splitlines()
    if proc.returncode != 0 or not version:
        return "unknown", "version probe did not return a version"
    return version[0].strip()[:512] or "unknown", None


def probe_version(executable: Path) -> tuple[str, str | None]:
    return probe_command_version([str(executable), "--version"])


def probe_ezdxf_version(python: str, audit_script: Path) -> tuple[str, str | None, bool]:
    """Probe the optional ezdxf dependency without hiding audit failures."""
    try:
        proc = subprocess.run([python, str(audit_script), "--version"],
                              capture_output=True, text=True, timeout=15)
    except (OSError, subprocess.TimeoutExpired) as exc:
        return "unknown", f"version probe unavailable: {type(exc).__name__}", False
    output = (proc.stdout or proc.stderr).strip()
    if proc.returncode == 0 and output:
        return output.splitlines()[0].strip()[:512] or "unknown", None, False
    if "ezdxf import failed" in output:
        return "unavailable", "ezdxf Python dependency is unavailable", True
    return "unknown", "version probe did not return a version", False


def run_libredwg_open_receipt(fixture: dict[str, Any], fixture_path: Path,
                              config: dict[str, Any], require_oracle: bool) -> tuple[dict[str, Any], int]:
    configured_path, executable = libredwg_tool(config)
    oracle_mode = mode(config, "libredwg")
    required = require_oracle or oracle_mode == "required"
    if executable is None:
        status = "missing-required" if required else "skipped"
        return receipt_template(
            fixture, configured_path, "unavailable", oracle_mode, status,
            "tool-unavailable", ["dwgread is not configured or executable"]), (2 if required else 0)

    version, probe_diagnostic = probe_version(executable)
    diagnostics = [probe_diagnostic] if probe_diagnostic else []
    receipt = receipt_template(fixture, str(executable), version, oracle_mode,
                               "failed", "oracle-rejected", diagnostics)
    try:
        proc = subprocess.run([str(executable), str(fixture_path)], capture_output=True,
                              text=True, timeout=120)
    except subprocess.TimeoutExpired:
        receipt["status"] = "failed"
        receipt["failureClass"] = "timeout"
        receipt["diagnostics"].append("dwgread timed out after 120s")
        return receipt, 1
    except OSError as exc:
        receipt["status"] = "error"
        receipt["failureClass"] = "internal"
        receipt["diagnostics"].append(f"dwgread could not be started: {exc}")
        return receipt, 2

    receipt["exitCode"] = proc.returncode
    output = proc.stdout + proc.stderr
    if proc.returncode != 0:
        receipt["diagnostics"].append(f"dwgread exited with {proc.returncode}")
        return receipt, 1
    if FATAL_DWGREAD.search(output):
        receipt["diagnostics"].append("dwgread reported a fatal decode diagnostic")
        return receipt, 1
    receipt["status"] = "passed"
    receipt["failureClass"] = "none"
    return receipt, 0


def ezdxf_tool(config: dict[str, Any]) -> tuple[str, str | None]:
    entry = config.get("ezdxf", {})
    entry = entry if isinstance(entry, dict) else {}
    configured = str(entry.get("python", "") or sys.executable)
    candidate = Path(configured).expanduser()
    if candidate.is_file() and os.access(candidate, os.X_OK):
        return str(candidate.resolve()), str(candidate.resolve())
    resolved = shutil.which(configured)
    if resolved:
        return str(Path(resolved).resolve()), str(Path(resolved).resolve())
    return configured, None


def run_ezdxf_audit_receipt(repo: Path, fixture: dict[str, Any],
                            fixture_path: Path, config: dict[str, Any],
                            require_oracle: bool) -> tuple[dict[str, Any], int]:
    configured_path, python = ezdxf_tool(config)
    oracle_mode = mode(config, "ezdxf")
    required = require_oracle or oracle_mode == "required"
    template = ["<python>", "scripts/ezdxf_audit.py", "<fixture>", "--quiet"]
    if python is None:
        status = "missing-required" if required else "skipped"
        return receipt_template(
            fixture, configured_path, "unavailable", oracle_mode, status,
            "tool-unavailable", ["python for ezdxf is not configured or executable"],
            oracle_name="ezdxf", operation="audit", source_format="DXF",
            command_template=template), (2 if required else 0)

    audit_script = repo / "scripts/ezdxf_audit.py"
    version, probe_diagnostic, unavailable = probe_ezdxf_version(python, audit_script)
    if unavailable:
        status = "missing-required" if required else "skipped"
        return receipt_template(
            fixture, python, version, oracle_mode, status, "tool-unavailable",
            [probe_diagnostic] if probe_diagnostic else [], oracle_name="ezdxf",
            operation="audit", source_format="DXF", command_template=template), (
                2 if required else 0)
    diagnostics = [probe_diagnostic] if probe_diagnostic else []
    receipt = receipt_template(
        fixture, python, version, oracle_mode, "failed", "oracle-rejected",
        diagnostics, oracle_name="ezdxf", operation="audit", source_format="DXF",
        command_template=template)
    try:
        proc = subprocess.run([python, str(audit_script), str(fixture_path), "--quiet"],
                              capture_output=True, text=True, timeout=120)
    except subprocess.TimeoutExpired:
        receipt["status"] = "failed"
        receipt["failureClass"] = "timeout"
        receipt["diagnostics"].append("ezdxf audit timed out after 120s")
        return receipt, 1
    except OSError as exc:
        receipt["status"] = "error"
        receipt["failureClass"] = "internal"
        receipt["diagnostics"].append(f"ezdxf audit could not be started: {exc}")
        return receipt, 2

    receipt["exitCode"] = proc.returncode
    if proc.returncode != 0:
        receipt["diagnostics"].append(f"ezdxf audit exited with {proc.returncode}")
        return receipt, 1
    receipt["status"] = "passed"
    receipt["failureClass"] = "none"
    return receipt, 0


def manifest_receipt_jobs(fixtures: list[dict[str, Any]],
                          selected_oracles: set[str],
                          explicit_fixtures: bool) -> list[tuple[dict[str, Any], str]]:
    """Choose only adapters that can inspect each selected fixture."""
    jobs: list[tuple[dict[str, Any], str]] = []
    for fixture in fixtures:
        identity = fixture_identity(fixture)
        fixture_format = identity["format"]
        declared = fixture.get("requiredOracles", [])
        if not isinstance(declared, list) or any(
                not isinstance(value, str) for value in declared):
            raise ReceiptError(
                f"fixture {identity['id']} has an invalid requiredOracles list")

        names: list[str] = []
        if declared:
            for requirement in declared:
                name, separator, operation = requirement.partition(":")
                adapter = MANIFEST_ORACLE_ADAPTERS.get(name)
                if (not separator or adapter is None or adapter[1] != operation
                        or adapter[0] != fixture_format):
                    raise ReceiptError(
                        f"fixture {identity['id']} requires incompatible oracle {requirement}")
                if name not in names:
                    names.append(name)
        else:
            names.extend(DEFAULT_MANIFEST_ORACLES.get(fixture_format, ()))
        if not names:
            raise ReceiptError(
                f"fixture {identity['id']} has no receipt adapter for {fixture_format}")

        compatible = [name for name in names if name in selected_oracles]
        if not compatible and explicit_fixtures:
            requested = ", ".join(sorted(selected_oracles))
            raise ReceiptError(
                f"fixture {identity['id']} has no adapter selected by --only {requested}")
        jobs.extend((fixture, name) for name in compatible)

    if not jobs:
        raise ReceiptError("no compatible manifest oracle receipts selected")
    return jobs


def run_manifest_adapter_receipt(repo: Path, fixture: dict[str, Any],
                                 fixture_path: Path, oracle_name: str,
                                 config: dict[str, Any],
                                 require_oracle: bool) -> tuple[dict[str, Any], int]:
    if oracle_name == "libredwg":
        return run_libredwg_open_receipt(fixture, fixture_path, config,
                                         require_oracle)
    if oracle_name == "ezdxf":
        return run_ezdxf_audit_receipt(repo, fixture, fixture_path, config,
                                       require_oracle)
    raise ReceiptError(f"unsupported manifest oracle adapter: {oracle_name}")


def fixture_integrity_receipt(fixture: dict[str, Any], oracle_name: str,
                              config: dict[str, Any], diagnostic: str) -> dict[str, Any]:
    source_format, operation = MANIFEST_ORACLE_ADAPTERS[oracle_name]
    if oracle_name == "libredwg":
        configured_path, _ = libredwg_tool(config)
        template = ["<dwgread>", "<fixture>"]
    else:
        configured_path, _ = ezdxf_tool(config)
        template = ["<python>", "scripts/ezdxf_audit.py", "<fixture>", "--quiet"]
    return receipt_template(
        fixture, configured_path, "not-probed", mode(config, oracle_name),
        "failed", "fixture-integrity", [diagnostic], oracle_name=oracle_name,
        operation=operation, source_format=source_format,
        command_template=template)


def print_receipts(results: list[dict[str, Any]]) -> None:
    for result in results:
        receipt = result["receipt"]
        fixture = receipt["fixture"]
        print(f"{fixture['id']}: {receipt['operation']['name']} "
              f"{receipt['status']} ({receipt['failureClass']})")
        print(f"  receipt: {result['path']}")
        for diagnostic in receipt["diagnostics"]:
            print(f"  {diagnostic}")


def receipt_exit_code(statuses: list[int]) -> int:
    if 2 in statuses:
        return 2
    if 1 in statuses:
        return 1
    return 0


def run_manifest_receipts(repo: Path, args: argparse.Namespace,
                          config: dict[str, Any]) -> int:
    manifest_path = args.manifest if args.manifest.is_absolute() else repo / args.manifest
    receipt_dir = args.receipt_dir if args.receipt_dir.is_absolute() else repo / args.receipt_dir
    try:
        fixtures = select_fixtures(load_manifest(manifest_path), args.fixture,
                                   args.default_fixtures)
        requested = set(getattr(args, "only", None) or MANIFEST_ORACLE_ADAPTERS)
        unsupported = requested - set(MANIFEST_ORACLE_ADAPTERS)
        if unsupported:
            raise ReceiptError(
                "manifest receipts do not support " + sorted(unsupported)[0])
        jobs = manifest_receipt_jobs(fixtures, requested, bool(args.fixture))
    except ReceiptError as exc:
        sys.stderr.write(f"error: {exc}\n")
        return 2

    verified: dict[str, Path] = {}
    invalid: dict[str, str] = {}
    for fixture in fixtures:
        try:
            verified[fixture_identity(fixture)["id"]] = verify_repo_fixture(repo, fixture)
        except ReceiptError as exc:
            invalid[fixture_identity(fixture)["id"]] = str(exc)

    results: list[dict[str, Any]] = []
    statuses: list[int] = []
    # Validate the entire selected batch before letting any external process
    # observe a fixture. A bad input is still recorded for every selected
    # adapter, but no valid sibling is sent to an oracle in this invocation.
    for fixture, oracle_name in jobs:
        fixture_id = fixture_identity(fixture)["id"]
        if invalid:
            receipt = fixture_integrity_receipt(
                fixture, oracle_name, config,
                invalid.get(fixture_id, "another selected fixture failed integrity validation"))
            status = 1
        else:
            receipt, status = run_manifest_adapter_receipt(
                repo, fixture, verified[fixture_id], oracle_name, config,
                args.require_oracle)
        try:
            destination = write_receipt_atomic(receipt_dir, receipt)
        except ReceiptError as exc:
            sys.stderr.write(f"error: {exc}\n")
            return 2
        results.append({"receipt": receipt, "path": str(destination)})
        statuses.append(status)

    payload = {
        "schema": 1,
        "tool": "run_external_oracles",
        "mode": "manifest-receipts",
        "results": results,
    }
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print_receipts(results)
    return receipt_exit_code(statuses)


def main(argv: list[str]) -> int:
    repo = repo_root_from_script(__file__)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--corpus", type=Path,
                        help="legacy recursive corpus directory (default: tests/fixtures/corpus)")
    selector = parser.add_mutually_exclusive_group()
    selector.add_argument("--fixture", action="append", default=[],
                          help="manifest fixture id for a receipt (repeatable)")
    selector.add_argument("--default-fixtures", action="store_true",
                          help="write receipts for all default-enabled manifest fixtures")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--receipt-dir", type=Path, default=DEFAULT_RECEIPT_DIR)
    parser.add_argument("--require-oracle", action="store_true",
                        help="make an unavailable selected manifest oracle fail")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--only", choices=["dwgts", "libredwg", "ezdxf", "oda"], action="append")
    args = parser.parse_args(argv)

    config_path = args.config if args.config.is_absolute() else repo / args.config
    config = load_json(config_path)
    manifest_mode = bool(args.fixture) or args.default_fixtures
    if manifest_mode:
        if args.corpus is not None:
            parser.error("--corpus cannot be combined with manifest receipt selectors")
        selected = set(args.only or MANIFEST_ORACLE_ADAPTERS)
        unsupported = selected - set(MANIFEST_ORACLE_ADAPTERS)
        if unsupported:
            parser.error("manifest receipts support only --only libredwg or ezdxf")
        return run_manifest_receipts(repo, args, config)
    if args.require_oracle:
        parser.error("--require-oracle requires --fixture or --default-fixtures")
    if args.receipt_dir != DEFAULT_RECEIPT_DIR:
        parser.error("--receipt-dir requires --fixture or --default-fixtures")

    corpus_arg = args.corpus or Path("tests/fixtures/corpus")
    corpus = corpus_arg if corpus_arg.is_absolute() else repo / corpus_arg
    selected = set(args.only or ["dwgts", "libredwg", "ezdxf", "oda"])

    results: list[dict[str, Any]] = []
    exit_code = 0
    runners = {
        "dwgts": lambda: run_dwgts(repo, config, corpus, args.quiet),
        "libredwg": lambda: run_libredwg(config, corpus),
        "ezdxf": lambda: run_ezdxf(repo, config, corpus, args.quiet),
        "oda": lambda: run_oda(repo, config, corpus),
    }
    for name in ("dwgts", "libredwg", "ezdxf", "oda"):
        if name not in selected:
            continue
        result, status = runners[name]()
        results.append(result)
        if status == 1:
            exit_code = 1
        elif status == 2 and exit_code == 0:
            exit_code = 2

    payload = {"schema": 1, "tool": "run_external_oracles", "corpus": str(corpus), "results": results}
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print_text(results)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
