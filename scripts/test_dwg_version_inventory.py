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

"""Regression tests for the source-derived DWG version inventory."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS))

from dwg_inventory_common import (
    dwg_writer_dispatch,
    extract_function_exact,
    parse_reader_dispatch,
    parse_versions,
    parse_writer_dispatch,
)
from generate_gap_report import build_report
from writer_support_inventory import build_rows


class DwgVersionInventoryTests(unittest.TestCase):
    REPO = Path(__file__).resolve().parents[1]

    def test_exact_extraction_rejects_function_name_prefixes(self) -> None:
        source = """
bool dwgRW::writeObjectTransaction() { return false; }
bool dwgRW::write(DRW_Interface *, DRW::Version ver, bool) {
    if (ver == DRW::AC1032)
        writer = std::make_unique<dwgWriter32>();
    else
        writer = std::make_unique<dwgWriter15>();
}
"""
        block = extract_function_exact(source, "bool dwgRW::write")
        self.assertIn("dwgWriter32", block)
        self.assertNotIn("writeObjectTransaction", block)

    def test_live_writer_dispatch_maps_all_supported_modern_versions(self) -> None:
        source = (
            Path(__file__).resolve().parents[1]
            / "libraries/libdxfrw/src/libdwgr.cpp"
        ).read_text(encoding="utf-8")
        self.assertEqual(
            parse_writer_dispatch(source),
            {
                "AC1015": "dwgWriter15",
                "AC1018": "dwgWriter18",
                "AC1021": "dwgWriter21",
                "AC1024": "dwgWriter24",
                "AC1027": "dwgWriter27",
                "AC1032": "dwgWriter32",
            },
        )

    def test_live_reader_dispatch_is_not_affected_by_exact_extraction(self) -> None:
        source = (
            self.REPO / "libraries/libdxfrw/src/libdwgr.cpp"
        ).read_text(encoding="utf-8")
        dispatch = parse_reader_dispatch(source)
        self.assertEqual(dispatch["AC1015"], "dwgReader15")
        self.assertEqual(dispatch["AC1032"], "dwgReader32")

    def test_generated_reports_use_all_live_writer_versions(self) -> None:
        expected = {
            "AC1015": "dwgWriter15",
            "AC1018": "dwgWriter18",
            "AC1021": "dwgWriter21",
            "AC1024": "dwgWriter24",
            "AC1027": "dwgWriter27",
            "AC1032": "dwgWriter32",
        }
        rows = build_rows(self.REPO)
        core_rows = [
            row
            for row in rows
            if row.format == "DWG"
            and row.feature_name == "file-container"
            and row.target_version in expected
        ]
        self.assertEqual(len(core_rows), len(expected))
        core = {row.target_version: row.output_mode for row in core_rows}
        self.assertEqual(core, expected)
        report = build_report(self.REPO)
        self.assertIn("| AC1021 | dwgReader21 | dwgWriter21 |", report)
        feature_table = report.split("## Feature Gaps", 1)[1]
        for version in expected:
            self.assertIn(version, feature_table)

    def test_optional_sibling_inputs_do_not_change_source_version_set(self) -> None:
        versions_before = tuple(version.code for version in parse_versions(self.REPO))
        writers_before = dwg_writer_dispatch(self.REPO)
        missing_siblings = self.REPO / "tmp" / "missing-dwg-inventory-siblings"
        self.assertFalse(missing_siblings.exists())
        self.assertEqual(
            tuple(version.code for version in parse_versions(self.REPO)),
            versions_before,
        )
        self.assertEqual(dwg_writer_dispatch(self.REPO), writers_before)


if __name__ == "__main__":
    unittest.main()
