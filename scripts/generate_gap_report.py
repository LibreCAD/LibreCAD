#!/usr/bin/env python3
# File: generate_gap_report.py

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

"""Generate the fixture- and oracle-backed DWG/DXF gap backlog.

The source inventories deliberately remain broad and machine-oriented.  This
report joins their unresolved reference rows to the writer and version matrices
so implementation work has one stable, reviewable entry point.  It is not a
claim that a static inventory proves format support.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass
import json
from pathlib import Path
import sys

from dwg_inventory_common import (
    dwg_writer_dispatch,
    markdown_cell,
    parse_versions,
    read_text,
    repo_root_from_script,
    write_or_check,
)


DEFAULT_OUTPUT = Path("docs/gap-report.md")
REFERENCE_INPUT = Path("libraries/libdxfrw/DWG_REFERENCE_COVERAGE_STATUS.md")
WRITER_INPUT = Path("libraries/libdxfrw/DWG_DXF_WRITER_SUPPORT_STATUS.md")
VERSION_INPUT = Path("libraries/libdxfrw/DWG_VERSION_SUPPORT_STATUS.md")
FIXTURE_MANIFEST = Path("tests/fixtures/fixture_manifest.json")
PRIORITY_ORDER = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}


@dataclass(frozen=True)
class ReferenceGap:
    source: str
    area: str
    family: str
    name: str
    status: str
    priority: str
    implementation_slice: str
    evidence: str


@dataclass(frozen=True)
class WriterState:
    version: str
    mode: str
    oracle: str
    fixture: str
    blockers: str


def default_fixture_rows(repo: Path) -> list[dict[str, str]]:
    path = repo / FIXTURE_MANIFEST
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SystemExit(f"error: cannot read fixture manifest {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"error: invalid fixture manifest {path}: {exc}") from exc
    fixtures = data.get("fixtures") if isinstance(data, dict) else None
    if not isinstance(fixtures, list):
        raise SystemExit("error: fixture manifest must contain a fixtures list")
    rows: list[dict[str, str]] = []
    for fixture in fixtures:
        if not isinstance(fixture, dict) or fixture.get("defaultEnabled") is not True:
            continue
        rows.append({
            "id": str(fixture.get("id", "")),
            "format": str(fixture.get("format", "")),
            "version": str(fixture.get("version", "")),
            "role": str(fixture.get("evidenceRole", "")),
            "features": ", ".join(str(value) for value in fixture.get("featureFamilies", [])),
            "diagnostics": ", ".join(str(value) for value in fixture.get("expectedDiagnostics", [])),
            "oracles": ", ".join(str(value) for value in fixture.get("requiredOracles", [])),
        })
    return sorted(rows, key=lambda row: (row["format"], row["version"], row["id"]))


def split_markdown_row(line: str) -> list[str]:
    """Split a pipe-delimited Markdown row while retaining escaped pipes."""
    stripped = line.strip()
    if not stripped.startswith("|") or not stripped.endswith("|"):
        return []
    cells: list[str] = []
    cell: list[str] = []
    escaped = False
    for char in stripped[1:-1]:
        if escaped:
            cell.append(char)
            escaped = False
        elif char == "\\":
            escaped = True
        elif char == "|":
            cells.append("".join(cell).strip())
            cell = []
        else:
            cell.append(char)
    if escaped:
        cell.append("\\")
    cells.append("".join(cell).strip())
    return cells


def table_rows(text: str, heading: str) -> list[dict[str, str]]:
    lines = text.splitlines()
    try:
        start = lines.index(heading)
    except ValueError as exc:
        raise SystemExit(f"error: missing table heading {heading!r}") from exc

    header: list[str] | None = None
    rows: list[dict[str, str]] = []
    for line in lines[start + 1 :]:
        if line.startswith("## "):
            break
        if not line.startswith("|"):
            continue
        cells = split_markdown_row(line)
        if not cells:
            continue
        if header is None:
            header = cells
            continue
        if all(cell and set(cell) <= {"-", ":"} for cell in cells):
            continue
        if len(cells) != len(header):
            raise SystemExit(f"error: malformed row beneath {heading!r}: {line}")
        rows.append(dict(zip(header, cells)))
    if header is None:
        raise SystemExit(f"error: missing table beneath {heading!r}")
    return rows


def reference_gaps(path: Path) -> list[ReferenceGap]:
    required = {
        "Source", "Area", "Family", "Name", "libdxfrw status", "Priority",
        "Implementation slice", "Evidence",
    }
    rows = table_rows(read_text(path), "## P0/P1/P2 Gaps")
    if rows and not required <= rows[0].keys():
        raise SystemExit("error: reference-gap table columns changed")
    return [
        ReferenceGap(
            source=row["Source"],
            area=row["Area"],
            family=row["Family"],
            name=row["Name"].strip("`"),
            status=row["libdxfrw status"],
            priority=row["Priority"],
            implementation_slice=row["Implementation slice"],
            evidence=row["Evidence"],
        )
        for row in rows
    ]


def writer_states(
    path: Path, writer_versions: tuple[str, ...]
) -> dict[tuple[str, str], list[WriterState]]:
    rows = table_rows(read_text(path), "## Matrix")
    required = {
        "format", "targetVersion", "featureName", "family", "writerMode",
        "validationOracle", "fixtureIds", "blockers",
    }
    if rows and not required <= rows[0].keys():
        raise SystemExit("error: writer-support table columns changed")
    states: dict[tuple[str, str], list[WriterState]] = defaultdict(list)
    for row in rows:
        if row["format"] != "DWG" or row["targetVersion"] not in writer_versions:
            continue
        states[(row["family"], row["featureName"])].append(
            WriterState(
                version=row["targetVersion"],
                mode=row["writerMode"],
                oracle=row["validationOracle"],
                fixture=row["fixtureIds"],
                blockers=row["blockers"],
            )
        )
    return states


def version_rows(path: Path) -> list[dict[str, str]]:
    rows = table_rows(read_text(path), "## Matrix")
    required = {
        "versionCode", "releaseFamily", "readerPath", "writerPath", "targetLevel",
        "currentLevel", "blockers", "promotionGate",
    }
    if rows and not required <= rows[0].keys():
        raise SystemExit("error: version-support table columns changed")
    return rows


def compact(values: list[str], fallback: str = "not-recorded") -> str:
    unique = sorted({value for value in values if value})
    return "; ".join(unique) if unique else fallback


def writer_summary(
    states: list[WriterState], writer_versions: tuple[str, ...]
) -> tuple[str, str, str, str, str]:
    by_version = {state.version: state for state in states}
    modes = [f"{version}:{by_version[version].mode}" for version in writer_versions if version in by_version]
    return (
        "; ".join(modes) if modes else "no DWG writer row",
        compact([state.fixture for state in states], "fixture-blocked:family/name"),
        compact([state.oracle for state in states], "libdxfrw reread + configured external oracle"),
        compact([state.blockers for state in states], "code-path and fixture audit required"),
        ", ".join(version for version in writer_versions if version in by_version) or "not-recorded",
    )


def build_report(repo: Path) -> str:
    reference_path = repo / REFERENCE_INPUT
    writer_path = repo / WRITER_INPUT
    version_path = repo / VERSION_INPUT
    gaps = reference_gaps(reference_path)
    writer_versions = tuple(
        version.code
        for version in parse_versions(repo)
        if version.code in dwg_writer_dispatch(repo)
    )
    writers = writer_states(writer_path, writer_versions)
    versions = version_rows(version_path)
    fixtures = default_fixture_rows(repo)

    grouped: dict[tuple[str, str], list[ReferenceGap]] = defaultdict(list)
    for gap in gaps:
        grouped[(gap.family, gap.name)].append(gap)

    lines = [
        "# DWG/DXF Gap Report",
        "",
        "Generated by `scripts/generate_gap_report.py`; do not edit by hand.",
        "",
        "This backlog joins static reference coverage with the current DWG writer",
        "matrix and version gates. An entry identifies required evidence; it does not",
        "by itself prove a reader, writer, or conversion is complete.",
        "",
        "## Inputs",
        "",
        f"- `{REFERENCE_INPUT.as_posix()}`",
        f"- `{WRITER_INPUT.as_posix()}`",
        f"- `{VERSION_INPUT.as_posix()}`",
        f"- `{FIXTURE_MANIFEST.as_posix()}` and configured external oracles",
        "",
        "## Version Gates",
        "",
        "| version | reader | writer | current | target | blockers | acceptance |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in versions:
        if row["currentLevel"] == row["targetLevel"] and row["writerPath"] != "unsupported":
            continue
        lines.append(
            "| " + " | ".join(markdown_cell(row[key]) for key in (
                "versionCode", "readerPath", "writerPath", "currentLevel", "targetLevel",
                "blockers", "promotionGate")) + " |"
        )

    lines.extend([
        "",
        "## Default Fixture Evidence",
        "",
        "| fixture id | format | version | evidence role | feature families | expected diagnostics | oracle policy |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ])
    if fixtures:
        for fixture in fixtures:
            lines.append("| " + " | ".join(markdown_cell(fixture[key]) for key in (
                "id", "format", "version", "role", "features", "diagnostics", "oracles")) + " |")
    else:
        lines.append("| _none_ |  |  |  |  |  |  |")

    lines.extend([
        "",
        "## Feature Gaps",
        "",
        "| id | family / feature | source evidence | current status | priority | affected DWG output versions | writer mode | required fixture | oracle | implementation scope | acceptance |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ])
    ordered = sorted(
        grouped.items(),
        key=lambda item: (
            min(PRIORITY_ORDER.get(gap.priority, 99) for gap in item[1]),
            item[0][0], item[0][1],
        ),
    )
    for index, ((family, name), sources) in enumerate(ordered, start=1):
        priority = min(sources, key=lambda gap: PRIORITY_ORDER.get(gap.priority, 99)).priority
        modes, fixtures, oracle, blockers, affected_versions = writer_summary(
            writers.get((family, name), []), writer_versions
        )
        source_evidence = compact([f"{gap.source}: {gap.evidence}" for gap in sources])
        status = compact([gap.status for gap in sources])
        scope = compact([gap.implementation_slice for gap in sources])
        acceptance = f"{fixtures}; {blockers}; reread and {oracle}"
        gap_id = f"GAP-{priority}-{index:03d}"
        cells = (
            gap_id,
            f"{family} / {name}",
            source_evidence,
            status,
            priority,
            affected_versions,
            modes,
            fixtures,
            oracle,
            scope,
            acceptance,
        )
        lines.append("| " + " | ".join(markdown_cell(cell) for cell in cells) + " |")

    lines.extend([
        "",
        "## Completion Rule",
        "",
        "A gap may be closed only when its fixture is manifest-backed, the relevant",
        "reader/writer path has a focused regression, same-version preservation is",
        "verified where raw data is used, and the recorded external oracle is run or",
        "explicitly unavailable under the project oracle policy.",
        "",
    ])
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    repo = repo_root_from_script(__file__)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=repo)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail when the generated report is stale")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    output = args.output if args.output.is_absolute() else repo / args.output
    return write_or_check(output, build_report(repo), args.check, "DWG/DXF gap report")


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
