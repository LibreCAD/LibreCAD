#!/usr/bin/env python3
# File: dwgts_feature_parity_inventory.py

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

"""Generate the libdxfrw-vs-dwgTs feature parity matrix."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

from dwg_inventory_common import (
    candidate_names,
    family_for,
    local_reader_surfaces,
    markdown_cell,
    normalize_name,
    read_text,
    repo_root_from_script,
    write_or_check,
)


DEFAULT_OUTPUT = Path("libraries/libdxfrw/DWGTS_FEATURE_PARITY_STATUS.md")


@dataclass(frozen=True)
class DwgTsRow:
    dwgts_source_bucket: str
    feature_name: str
    family: str
    dwgts_level: str
    libdxfrw_read_level: str
    libdxfrw_preserve_level: str
    libdxfrw_callback_or_audit_level: str
    writer_reference: str
    delta: str
    fixture_ids: str
    decision: str
    blockers: str


def split_table_row(line: str) -> list[str]:
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def parse_feature_coverage(path: Path) -> list[tuple[str, str, str, str]]:
    text = read_text(path)
    rows: list[tuple[str, str, str, str]] = []
    section = ""
    header: list[str] | None = None
    for raw_line in text.splitlines():
        if raw_line.startswith("## "):
            section = raw_line[3:].strip()
            header = None
            continue
        if raw_line.startswith("### "):
            section = raw_line[4:].strip()
            header = None
            continue
        if not raw_line.startswith("|"):
            continue
        cells = split_table_row(raw_line)
        if all(set(cell) <= {"-"} for cell in cells):
            continue
        if header is None:
            header = cells
            continue
        if section == "ACadSharp Fixed Object Types" and len(cells) >= 3:
            rows.append(("DWG fixed-type", cells[1], cells[2], cells[0]))
        elif section == "ACadSharp Custom/Unlisted Cases" and len(cells) >= 3:
            rows.append(("DWG custom/unlisted", cells[0], cells[2], cells[1] or "-"))
        elif section == "DXF parity gap, classified" and len(cells) >= 2:
            rows.append(("DXF parity-gap", cells[0], cells[1], "-"))
    return rows


def libdxfrw_levels(
    name: str,
    source_bucket: str,
    dwgts_level: str,
    extra_key: str,
    surfaces: dict[str, set[str] | set[int] | bool],
) -> tuple[str, str, str, str, str]:
    normalized = normalize_name(name)
    candidates = candidate_names(normalized)
    dwg_entity_cases = surfaces["dwg_entity_cases"]
    dwg_object_cases = surfaces["dwg_object_cases"]
    dwg_typed_names = surfaces["dwg_typed_names"]
    dwg_fixed_shell_names = surfaces["dwg_fixed_shell_names"]
    dxf_names = surfaces["dxf_entities"] | surfaces["dxf_objects"]  # type: ignore[operator]
    dwg_names = surfaces["dwg_names"]
    class_names = surfaces["class_names"]

    read_level = "not-routed"
    preserve_level = "none"
    callback_level = "none"
    decision = "implement"
    blockers = "fixture and code-path audit required"

    type_id: int | None = None
    if extra_key.isdigit():
        type_id = int(extra_key)

    if type_id is not None and (
        type_id in surfaces["dwg_fixed_entity_shell_types"]
        or type_id in surfaces["dwg_fixed_object_shell_types"]
    ):
        read_level = "fixed-raw-shell"
        preserve_level = "raw-shell"
        callback_level = "callback-or-audit-static"
        decision = "preserve-shell"
        blockers = "raw replay fixture required"
    elif candidates & dwg_fixed_shell_names:
        read_level = "fixed-raw-shell"
        preserve_level = "raw-shell"
        callback_level = "callback-or-audit-static"
        decision = "preserve-shell"
        blockers = "raw replay fixture required"
    elif type_id is not None and (type_id in dwg_entity_cases or type_id in dwg_object_cases):  # type: ignore[operator]
        read_level = "typed-fixed-dispatch"
        preserve_level = "typed"
        callback_level = "callback-or-audit-static"
        decision = "preserve-shell" if dwgts_level.startswith("raw") else "accepted-exception"
        blockers = "fixture confirmation required"
    elif candidates & dwg_typed_names:
        read_level = "typed-named-dwg-dispatch"
        preserve_level = "typed"
        callback_level = "callback-or-audit-static"
        decision = "preserve-shell" if dwgts_level.startswith("raw") else "accepted-exception"
        blockers = "fixture confirmation required"
    elif candidates & dxf_names:
        read_level = "typed-dxf-dispatch"
        preserve_level = "typed"
        callback_level = "callback-or-audit-static"
        decision = "accepted-exception"
        blockers = "fixture confirmation required"
    elif candidates & dwg_names:
        read_level = "named-dwg-route"
        preserve_level = "classed-shell"
        callback_level = "audit-static"
        decision = "preserve-shell"
        blockers = "raw replay fixture required"
    elif candidates & class_names:
        read_level = "classed-raw-route"
        preserve_level = "raw-shell-static"
        callback_level = "audit-static"
        decision = "preserve-shell"
        blockers = "raw shell fixture required"
    elif source_bucket == "DXF parity-gap" and dwgts_level == "naming-artifact":
        read_level = "naming-artifact-needs-map"
        preserve_level = "represented-elsewhere-unproven"
        callback_level = "audit-needed"
        decision = "classify-alias"
        blockers = "mapping note and fixture required"
    elif source_bucket == "DXF parity-gap" and dwgts_level == "raw-shell" and (
        surfaces["has_dxf_raw_entity"] or surfaces["has_dxf_raw_object"]
    ):
        read_level = "dxf-raw-fallback-static"
        preserve_level = "raw-shell-static"
        callback_level = "audit-static"
        decision = "preserve-shell"
        blockers = "raw shell fixture required"
    elif source_bucket == "DXF parity-gap" and dwgts_level == "real-gap":
        decision = "implement"
        blockers = "dwgTs real-gap row needs explicit libdxfrw route or exception"

    if dwgts_level == "structured":
        delta = "equal-or-needs-fixture" if read_level.startswith("typed") else "below"
    elif dwgts_level in {"raw-preserved", "raw-shell"}:
        delta = "equal-or-needs-fixture" if "raw" in preserve_level or preserve_level == "typed" else "below"
    elif dwgts_level == "naming-artifact":
        delta = "needs-review"
    elif dwgts_level == "real-gap":
        delta = "below" if read_level == "not-routed" else "exceeds"
    else:
        delta = "needs-review"

    return read_level, preserve_level, callback_level, delta, decision + "|" + blockers


def build_rows(repo: Path, feature_coverage: Path) -> list[DwgTsRow]:
    surfaces = local_reader_surfaces(repo)
    parsed = parse_feature_coverage(feature_coverage)
    rows: list[DwgTsRow] = []
    seen: set[tuple[str, str, str]] = set()
    for source_bucket, raw_name, dwgts_level, extra_key in parsed:
        name = normalize_name(raw_name)
        key = (source_bucket, name, dwgts_level)
        if key in seen:
            continue
        seen.add(key)
        family = family_for(name)
        read_level, preserve_level, callback_level, delta, decision_blockers = libdxfrw_levels(
            name, source_bucket, dwgts_level, extra_key, surfaces
        )
        decision, blockers = decision_blockers.split("|", 1)
        rows.append(
            DwgTsRow(
                dwgts_source_bucket=source_bucket,
                feature_name=name,
                family=family,
                dwgts_level=dwgts_level,
                libdxfrw_read_level=read_level,
                libdxfrw_preserve_level=preserve_level,
                libdxfrw_callback_or_audit_level=callback_level,
                writer_reference=f"DWG_DXF_WRITER_SUPPORT_STATUS.md::{family}/{name}",
                delta=delta,
                fixture_ids=f"fixture-blocked:{family}/{name}",
                decision=decision,
                blockers=blockers,
            )
        )
    return sorted(rows, key=lambda row: (row.family, row.dwgts_source_bucket, row.feature_name, row.dwgts_level))


def render(rows: list[DwgTsRow], repo: Path, feature_coverage: Path) -> str:
    summary: dict[tuple[str, str], int] = {}
    for row in rows:
        key = (row.dwgts_source_bucket, row.delta)
        summary[key] = summary.get(key, 0) + 1

    summary_lines = [
        "| dwgTsSourceBucket | delta | Count |",
        "| --- | --- | ---: |",
    ]
    for (bucket, delta), count in sorted(summary.items()):
        summary_lines.append(f"| {markdown_cell(bucket)} | {markdown_cell(delta)} | {count} |")

    table = [
        "| dwgTsSourceBucket | featureName | family | dwgTsLevel | libdxfrwReadLevel | libdxfrwPreserveLevel | libdxfrwCallbackOrAuditLevel | writerReference | delta | fixtureIds | decision | blockers |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        table.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    row.dwgts_source_bucket,
                    row.feature_name,
                    row.family,
                    row.dwgts_level,
                    row.libdxfrw_read_level,
                    row.libdxfrw_preserve_level,
                    row.libdxfrw_callback_or_audit_level,
                    row.writer_reference,
                    row.delta,
                    row.fixture_ids,
                    row.decision,
                    row.blockers,
                )
            )
            + " |"
        )

    return "\n".join(
        [
            "# DWGTS Feature Parity Status",
            "",
            "Generated by `scripts/dwgts_feature_parity_inventory.py`.",
            "",
            "This static matrix imports the generated dwgTs feature coverage and",
            "compares it with libdxfrw source-level reader, raw-shell, callback, and",
            "audit surfaces. Rows marked `equal-or-needs-fixture` still require",
            "fixture proof before implementation work can close them.",
            "",
            "## Inputs",
            "",
            "- LibreCAD root: repository checkout",
            "- dwgTs feature coverage: external `doc/FEATURE_COVERAGE.md` snapshot",
            "",
            "## Summary",
            "",
            "\n".join(summary_lines),
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
    parser.add_argument("--dwgts-coverage", type=Path, default=None)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    parser.add_argument("--require-input", action="store_true", help="fail instead of skipping if dwgTs coverage is absent")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    coverage = (args.dwgts_coverage or (repo.parent / "dwgTs/doc/FEATURE_COVERAGE.md")).resolve()
    output = args.output if args.output.is_absolute() else repo / args.output
    if not coverage.is_file():
        message = f"dwgTs feature coverage not found: {coverage}"
        if args.check and output.exists() and not args.require_input:
            print(f"skip: {message}")
            return 0
        raise SystemExit(f"error: {message}")

    return write_or_check(
        output,
        render(build_rows(repo, coverage), repo, coverage),
        args.check,
        "dwgTs feature parity inventory",
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
