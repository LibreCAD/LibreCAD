#!/usr/bin/env python3
# File: writer_support_inventory.py

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

"""Generate the libdxfrw DWG/DXF writer-support matrix."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

from dwg_inventory_common import (
    dwg_writer_dispatch,
    family_for,
    markdown_cell,
    normalize_name,
    parse_versions,
    read_text,
    repo_root_from_script,
    write_or_check,
)


DEFAULT_OUTPUT = Path("libraries/libdxfrw/DWG_DXF_WRITER_SUPPORT_STATUS.md")


@dataclass(frozen=True)
class WriterRow:
    format: str
    target_version: str
    output_mode: str
    feature_name: str
    family: str
    writer_mode: str
    downgrade_policy: str
    raw_replay_eligibility: str
    required_classes_handles_owners: str
    validation_oracle: str
    fixture_ids: str
    blockers: str
    advertised: str


DXF_WRITE_VERSIONS = {"AC1009", "AC1014", "AC1015", "AC1018", "AC1021", "AC1024", "AC1027", "AC1032"}

DXF_TYPED_WRITERS = {
    "3DFACE": "write3dface",
    "ARC": "writeArc",
    "ARC_DIMENSION": "writeArcDimension",
    "ATTRIB": "writeAttrib",
    "CIRCLE": "writeCircle",
    "DIMENSION": "writeDimension",
    "ELLIPSE": "writeEllipse",
    "HATCH": "writeHatch",
    "IMAGE": "writeImage",
    "INSERT": "writeInsert",
    "LEADER": "writeLeader",
    "LIGHT": "writeLight",
    "LINE": "writeLine",
    "LWPOLYLINE": "writeLWPolyline",
    "MLINE": "writeMLine",
    "MPOLYGON": "writeHatch",
    "MTEXT": "writeMText",
    "MULTILEADER": "writeMultiLeader",
    "OLE2FRAME": "writeOle2Frame",
    "POINT": "writePoint",
    "POLYLINE": "writePolyline",
    "RAY": "writeRay",
    "SPLINE": "writeSpline",
    "TEXT": "writeText",
    "TOLERANCE": "writeTolerance",
    "TRACE": "writeTrace",
    "VIEWPORT": "writeViewport",
    "WIPEOUT": "writeWipeout",
    "XLINE": "writeXline",
}


def code_order(repo: Path, version_code: str) -> int:
    for version in parse_versions(repo):
        if version.code == version_code:
            return version.order
    return 999


def parse_reference_features(path: Path) -> list[tuple[str, str]]:
    if not path.is_file():
        return []
    text = read_text(path)
    rows: list[tuple[str, str]] = []
    in_gap_table = False
    for line in text.splitlines():
        if line == "## P0/P1/P2 Gaps":
            in_gap_table = True
            continue
        if in_gap_table and line.startswith("## "):
            break
        if not in_gap_table or not line.startswith("|") or line.startswith("| ---") or line.startswith("| Source"):
            continue
        cells = [cell.strip().strip("`") for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 5:
            family = cells[2]
            name = normalize_name(cells[4])
            rows.append((family, name))
    return sorted(set(rows), key=lambda item: (item[0], item[1]))


def feature_writer_mode(
    format_name: str,
    version_code: str,
    feature: str,
    libdxfrw_text: str,
    dwg_writers: dict[str, str],
) -> tuple[str, str, str, str]:
    normalized = normalize_name(feature)
    if format_name == "DXF":
        func = DXF_TYPED_WRITERS.get(normalized)
        if func and func in libdxfrw_text:
            return ("native-typed", "version-gated typed DXF writer", "not-needed", "libdxfrw reread + ezdxf audit")
        if "writeRawDxfObject" in libdxfrw_text:
            return ("dxf-raw-shell", "preserve shell or block unsupported downgrade", "same-format raw groups", "libdxfrw reread + ezdxf audit")
        return ("blocked", "fail with diagnostic", "none", "unit test diagnostic")

    if version_code in dwg_writers and "writeRawDwgObject" in libdxfrw_text:
        return ("raw-replay", "same-version only; cross-version blocked", "same-version raw object", "libdxfrw reread + optional dwgread/ODA")
    return ("blocked", "fail with diagnostic", "none", "unit test diagnostic")


def add_core_rows(
    repo: Path, text: str, rows: list[WriterRow], dwg_writers: dict[str, str]
) -> None:
    for version in parse_versions(repo):
        if version.code in dwg_writers:
            rows.append(
                WriterRow(
                    format="DWG",
                    target_version=version.code,
                    output_mode=dwg_writers[version.code],
                    feature_name="file-container",
                    family="container/version",
                    writer_mode="native-typed",
                    downgrade_policy="no implicit adjacent-version routing",
                    raw_replay_eligibility="not-needed",
                    required_classes_handles_owners="HEADER/HANDSEED/object-map/owner graph",
                    validation_oracle="libdxfrw reread + optional dwgread/ODA",
                    fixture_ids=f"fixture-blocked:{version.code.lower()}-writer-smoke",
                    blockers="external smoke and positive fixture required before promotion",
                    advertised="yes",
                )
            )
            if "writeRawDwgObject" in text:
                rows.append(
                    WriterRow(
                        format="DWG",
                        target_version=version.code,
                        output_mode=dwg_writers[version.code],
                        feature_name="raw-dwg-object-replay",
                        family="raw/preservation",
                        writer_mode="raw-replay",
                        downgrade_policy="same-version only",
                        raw_replay_eligibility="same-version class/type/owner/handle match",
                        required_classes_handles_owners="class map, object owner, object handle",
                        validation_oracle="libdxfrw reread + optional dwgread/ODA",
                        fixture_ids=f"fixture-blocked:{version.code.lower()}-raw-replay",
                        blockers="fixture proving byte-bounded replay required",
                        advertised="no",
                    )
                )
        else:
            rows.append(
                WriterRow(
                    format="DWG",
                    target_version=version.code,
                    output_mode="unsupported",
                    feature_name="file-container",
                    family="container/version",
                    writer_mode="blocked",
                    downgrade_policy="fail BAD_VERSION",
                    raw_replay_eligibility="none",
                    required_classes_handles_owners="not-emitted",
                    validation_oracle="unit test diagnostic",
                    fixture_ids=f"fixture-blocked:{version.code.lower()}-writer-diagnostic",
                    blockers="explicit unsupported-output fixture required",
                    advertised="no",
                )
            )

    for version in parse_versions(repo):
        if version.code not in DXF_WRITE_VERSIONS:
            continue
        for mode in ("DXF-ASCII", "DXF-Binary"):
            rows.append(
                WriterRow(
                    format="DXF",
                    target_version=version.code,
                    output_mode=mode,
                    feature_name="section-spine",
                    family="container/version",
                    writer_mode="native-typed",
                    downgrade_policy="R12 blocks OBJECTS/CLASSES with diagnostics",
                    raw_replay_eligibility="DXF raw groups where version can represent them",
                    required_classes_handles_owners="HEADER/CLASSES/TABLES/BLOCKS/ENTITIES/OBJECTS/HANDSEED",
                    validation_oracle="libdxfrw reread + ezdxf audit",
                    fixture_ids=f"fixture-blocked:{version.code.lower()}-{mode.lower()}",
                    blockers="round-trip fixture and ezdxf audit required",
                    advertised="yes",
                )
            )


def build_rows(repo: Path) -> list[WriterRow]:
    libdxfrw = read_text(repo / "libraries/libdxfrw/src/libdxfrw.cpp")
    libdwgr = read_text(repo / "libraries/libdxfrw/src/libdwgr.cpp")
    text = libdxfrw + "\n" + libdwgr
    dwg_writers = dwg_writer_dispatch(repo)
    rows: list[WriterRow] = []
    add_core_rows(repo, text, rows, dwg_writers)

    features = parse_reference_features(repo / "libraries/libdxfrw/DWG_REFERENCE_COVERAGE_STATUS.md")
    writer_versions = [
        version.code
        for version in parse_versions(repo)
        if version.code in dwg_writers
    ]
    dxf_versions = [
        version.code
        for version in parse_versions(repo)
        if version.code in DXF_WRITE_VERSIONS
    ]
    for family, feature in features:
        for version in writer_versions:
            mode, downgrade, raw, oracle = feature_writer_mode(
                "DWG", version, feature, text, dwg_writers
            )
            rows.append(
                WriterRow(
                    format="DWG",
                    target_version=version,
                    output_mode=dwg_writers[version],
                    feature_name=feature,
                    family=family,
                    writer_mode=mode,
                    downgrade_policy=downgrade,
                    raw_replay_eligibility=raw,
                    required_classes_handles_owners="class metadata, object handle, owner handle",
                    validation_oracle=oracle,
                    fixture_ids=f"fixture-blocked:{family}/{feature}/{version}",
                    blockers="feature writer fixture required",
                    advertised="no",
                )
            )
        for version in dxf_versions:
            for output_mode in ("DXF-ASCII", "DXF-Binary"):
                mode, downgrade, raw, oracle = feature_writer_mode(
                    "DXF", version, feature, text, dwg_writers
                )
                rows.append(
                    WriterRow(
                        format="DXF",
                        target_version=version,
                        output_mode=output_mode,
                        feature_name=feature,
                        family=family,
                        writer_mode=mode,
                        downgrade_policy=downgrade,
                        raw_replay_eligibility=raw,
                        required_classes_handles_owners="CLASSES, handle, owner, reactors/xdicts where applicable",
                        validation_oracle=oracle,
                        fixture_ids=f"fixture-blocked:{family}/{feature}/{version}/{output_mode.lower()}",
                        blockers="feature writer fixture required",
                        advertised="no",
                    )
                )
    return sorted(rows, key=lambda row: (row.format, code_order(repo, row.target_version), row.output_mode, row.family, row.feature_name))


def render(rows: list[WriterRow], repo: Path) -> str:
    summary: dict[tuple[str, str, str], int] = {}
    for row in rows:
        key = (row.format, row.output_mode, row.writer_mode)
        summary[key] = summary.get(key, 0) + 1
    summary_lines = [
        "| format | outputMode | writerMode | Count |",
        "| --- | --- | --- | ---: |",
    ]
    for (format_name, output_mode, writer_mode), count in sorted(summary.items()):
        summary_lines.append(f"| {format_name} | {output_mode} | {writer_mode} | {count} |")

    table = [
        "| format | targetVersion | outputMode | featureName | family | writerMode | downgradePolicy | rawReplayEligibility | requiredClassesHandlesOwners | validationOracle | fixtureIds | blockers | advertised |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        table.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    row.format,
                    row.target_version,
                    row.output_mode,
                    row.feature_name,
                    row.family,
                    row.writer_mode,
                    row.downgrade_policy,
                    row.raw_replay_eligibility,
                    row.required_classes_handles_owners,
                    row.validation_oracle,
                    row.fixture_ids,
                    row.blockers,
                    row.advertised,
                )
            )
            + " |"
        )

    return "\n".join(
        [
            "# DWG/DXF Writer Support Status",
            "",
            "Generated by `scripts/writer_support_inventory.py`.",
            "",
            "This is a static writer-readiness matrix. It records writer entrypoints,",
            "raw replay routes, and blocker diagnostics. Rows are not considered",
            "promoted until fixture and oracle gates named in the row are green.",
            "",
            "## Inputs",
            "",
            "- LibreCAD root: repository checkout",
            "- Writer source: `libraries/libdxfrw/src/libdxfrw.cpp`",
            "- DWG writer source: `libraries/libdxfrw/src/libdwgr.cpp`",
            "- Feature source: `libraries/libdxfrw/DWG_REFERENCE_COVERAGE_STATUS.md`",
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
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    output = args.output if args.output.is_absolute() else repo / args.output
    return write_or_check(output, render(build_rows(repo), repo), args.check, "DWG/DXF writer support inventory")


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
