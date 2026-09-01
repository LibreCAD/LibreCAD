#!/usr/bin/env python3
# File: dwg_version_inventory.py

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

"""Generate the libdxfrw DWG version-support matrix."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

from dwg_inventory_common import (
    extract_function_exact,
    markdown_cell,
    parse_reader_dispatch,
    parse_versions,
    parse_writer_dispatch,
    read_text,
    repo_root_from_script,
    write_or_check,
)


DEFAULT_OUTPUT = Path("libraries/libdxfrw/DWG_VERSION_SUPPORT_STATUS.md")


@dataclass(frozen=True)
class VersionRow:
    version_code: str
    release_family: str
    reader_path: str
    writer_path: str
    target_level: str
    current_level: str
    dispatch_diagnostic: str
    positive_fixture_ids: str
    negative_fixture_ids: str
    blockers: str
    promotion_gate: str


def target_level(version_code: str, has_writer: bool) -> str:
    if version_code in {"MC0.0", "AC1.2", "AC1.4", "AC1.50", "AC2.10", "AC1002", "AC1003", "AC1004"}:
        return "V0-clean-diagnostic"
    if version_code in {"AC1006", "AC1009", "AC1012", "AC1014"}:
        return "V2-core-read"
    if has_writer:
        return "V4-writer-gated"
    return "V3-structural-preservation"


def current_level(version_code: str, has_reader: bool, has_writer: bool) -> str:
    if not has_reader:
        return "V0-recognized-unsupported"
    if version_code in {"AC1006", "AC1009"}:
        return "V1-static-container-reader"
    if version_code in {"AC1012", "AC1014"}:
        return "V2-static-reader"
    if has_writer:
        return "V2/V4-static-entrypoint"
    return "V2-static-reader"


def infer_fixture_ids(version_code: str) -> tuple[str, str]:
    # PR0 deliberately records fixture blockers instead of pretending current
    # source-level dispatch proves byte-level coverage.
    positive = f"fixture-blocked:{version_code.lower()}-positive-needed"
    negative = f"fixture-blocked:{version_code.lower()}-negative-needed"
    return positive, negative


def dispatch_by_code(repo: Path) -> tuple[dict[str, str], dict[str, str], bool]:
    versions = parse_versions(repo)
    code_by_enum = {version.enum_name: version.code for version in versions}
    libdwgr = read_text(repo / "libraries/libdxfrw/src/libdwgr.cpp")
    reader_by_code = {
        code_by_enum[enum_name]: reader
        for enum_name, reader in parse_reader_dispatch(libdwgr).items()
        if enum_name in code_by_enum and reader != "unsupported"
    }
    writer_by_code = {
        code_by_enum[enum_name]: writer
        for enum_name, writer in parse_writer_dispatch(libdwgr).items()
        if enum_name in code_by_enum
    }
    return reader_by_code, writer_by_code, "BAD_VERSION" in libdwgr


def build_rows(repo: Path) -> list[VersionRow]:
    readers, writers, has_bad_version = dispatch_by_code(repo)
    rows: list[VersionRow] = []
    for version in parse_versions(repo):
        reader = readers.get(version.code, "unsupported")
        writer = writers.get(version.code, "unsupported")
        positive, negative = infer_fixture_ids(version.code)
        if reader == "unsupported":
            diagnostic = "recognized-version-no-reader"
            blockers = "fixture plus precise BAD_VERSION diagnostic"
        elif writer == "unsupported":
            diagnostic = "read-only-static-dispatch"
            blockers = "writer decision and fixture-backed output diagnostic"
        else:
            diagnostic = "reader-writer-static-dispatch"
            blockers = "positive/negative fixtures and external writer smoke"

        if not has_bad_version and reader == "unsupported":
            blockers += "; BAD_VERSION path not found"

        rows.append(
            VersionRow(
                version_code=version.code,
                release_family=version.release,
                reader_path=reader,
                writer_path=writer,
                target_level=target_level(version.code, writer != "unsupported"),
                current_level=current_level(version.code, reader != "unsupported", writer != "unsupported"),
                dispatch_diagnostic=diagnostic,
                positive_fixture_ids=positive,
                negative_fixture_ids=negative,
                blockers=blockers,
                promotion_gate="writer-matrix+fixture+oracle-smoke" if writer != "unsupported" else "version-diagnostic-fixture",
            )
        )
    return rows


def render(rows: list[VersionRow], repo: Path) -> str:
    counts: dict[str, int] = {}
    for row in rows:
        counts[row.current_level] = counts.get(row.current_level, 0) + 1
    summary = "\n".join(f"| {markdown_cell(k)} | {v} |" for k, v in sorted(counts.items()))
    table = [
        "| versionCode | releaseFamily | readerPath | writerPath | targetLevel | currentLevel | dispatchDiagnostic | positiveFixtureIds | negativeFixtureIds | blockers | promotionGate |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        table.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    row.version_code,
                    row.release_family,
                    row.reader_path,
                    row.writer_path,
                    row.target_level,
                    row.current_level,
                    row.dispatch_diagnostic,
                    row.positive_fixture_ids,
                    row.negative_fixture_ids,
                    row.blockers,
                    row.promotion_gate,
                )
            )
            + " |"
        )
    return "\n".join(
        [
            "# DWG Version Support Status",
            "",
            "Generated by `scripts/dwg_version_inventory.py`.",
            "",
            "This is a static implementation-readiness matrix. It records recognized",
            "version dispatch and writer entrypoints, but fixture rows remain blocked",
            "until the Phase 0 corpus proves positive and negative behavior.",
            "",
            "## Inputs",
            "",
            "- LibreCAD root: repository checkout",
            "- Version source: `libraries/libdxfrw/src/drw_base.h`",
            "- Dispatch source: `libraries/libdxfrw/src/libdwgr.cpp`",
            "",
            "## Current-Level Summary",
            "",
            "| currentLevel | Count |",
            "| --- | ---: |",
            summary,
            "",
            "## Matrix",
            "",
            "\n".join(table),
            "",
        ]
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=repo_root_from_script(__file__))
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    output = args.output if args.output.is_absolute() else repo / args.output
    return write_or_check(output, render(build_rows(repo), repo), args.check, "DWG version inventory")


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
