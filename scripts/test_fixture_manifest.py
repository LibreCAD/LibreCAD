#!/usr/bin/env python3
# File: test_fixture_manifest.py

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

"""Regression tests for default fixture-manifest admission rules."""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path

from dwg_inventory_common import repo_root_from_script
from validate_fixture_manifest import validate_manifest


class FixtureManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.repo = repo_root_from_script(__file__)
        self.manifest = self.repo / "tests/fixtures/fixture_manifest.json"
        self.data = json.loads(self.manifest.read_text(encoding="utf-8"))

    def validate(self, data: dict[str, object]) -> list[str]:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture_manifest.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return validate_manifest(self.repo, path)

    def test_current_default_fixtures_are_valid(self) -> None:
        self.assertEqual([], self.validate(self.data))

    def test_default_dwg_magic_must_match_declared_version(self) -> None:
        data = copy.deepcopy(self.data)
        default_dwg = next(
            (
                fixture
                for fixture in data["fixtures"]
                if fixture.get("defaultEnabled") is True
                and fixture.get("format") == "DWG"
            ),
            None,
        )
        if default_dwg is None:
            self.skipTest("repository has no bundled default DWG fixture")
        default_dwg["version"] = "AC1022"
        errors = self.validate(data)
        self.assertTrue(any("DWG magic" in error for error in errors), errors)

    def test_empty_default_dwg_corpus_is_allowed(self) -> None:
        data = copy.deepcopy(self.data)
        data["fixtures"] = [
            fixture
            for fixture in data["fixtures"]
            if fixture.get("format") != "DWG"
            or fixture.get("defaultEnabled") is not True
        ]
        errors = self.validate(data)
        self.assertEqual([], errors)

    def test_default_dxf_acadver_must_match_declared_version(self) -> None:
        data = copy.deepcopy(self.data)
        for fixture in data["fixtures"]:
            if fixture.get("id") == "dxf-ac1021-block-record-preview":
                fixture["version"] = "AC1024"
                break
        errors = self.validate(data)
        self.assertTrue(any("DXF $ACADVER" in error for error in errors), errors)


if __name__ == "__main__":
    unittest.main()
