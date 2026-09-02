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

"""Small deterministic tests for the cross-read parity inventory helpers."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("dwg_cross_read_parity_inventory.py")
SPEC = importlib.util.spec_from_file_location("dwg_cross_read_parity_inventory", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class CrossReadParityInventoryTests(unittest.TestCase):
    def test_canonical_aliases(self) -> None:
        self.assertEqual(MODULE.normalize("DIMENSION_ANG_2_LN"), "DIMENSION_ANG2LN")
        self.assertEqual(MODULE.normalize("POINTCLOUDDEFINITIONEX"), "POINTCLOUDDEFEX")
        self.assertEqual(MODULE.normalize("NURBSSURFACE"), "NURBSURFACE")
        self.assertEqual(MODULE.normalize("PDFREFERENCE"), "PDFUNDERLAY")

    def test_callback_mapping_uses_exact_and_camel_case_routes(self) -> None:
        callbacks = {"addPointCloud", "addLine", "addDimLinear"}
        self.assertEqual(MODULE.callback_for("POINTCLOUD", callbacks), "addPointCloud")
        self.assertEqual(MODULE.callback_for("LINE", callbacks), "addLine")
        self.assertEqual(MODULE.callback_for("DIMENSION_LINEAR", callbacks), "addDimLinear")
        self.assertIsNone(MODULE.callback_for("UNKNOWN", callbacks))

    def test_inline_override_is_counted(self) -> None:
        header = "void addAppId(const DRW_AppId& /*data*/) override{}\n"
        self.assertIn("addAppId", MODULE.filter_overrides("", header))

    def test_reference_json_keys_are_scoped_to_object_families(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "src/api/cadToJson.ts"
            path.parent.mkdir(parents=True)
            path.write_text(
                "const OBJECT_FAMILIES = [\n"
                "  ['dictionaries', 'dictionaries'],\n"
                "  ['materials', 'materials'],\n"
                "] as const;\n",
                encoding="utf-8",
            )
            self.assertEqual(
                MODULE.reference_json_keys(root), {"dictionaries", "materials"}
            )

    def test_reference_fixed_routes_and_object_parser_ids(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "src/dwg/sections/DwgObjectsReader.ts"
            path.parent.mkdir(parents=True)
            path.write_text(
                "const FIXED_DXF_NAMES = new Map<number, string>([\n"
                "  [19, 'LINE'],\n"
                "  [42, 'DICTIONARY'],\n"
                "]);\n"
                "const ENTITY_PARSERS = new Array();\n"
                "ENTITY_PARSERS[19] = parseLine;\n"
                "const NON_ENTITY_OBJECT_PARSERS = new Map<number, NonEntityObjectParser>([\n"
                "  [42, (buf) => ({ objectTypeName: 'dictionary' })],\n"
                "]);\n",
                encoding="utf-8",
            )
            fixed, registered, object_names = MODULE.reference_dwg_routes(root)
            self.assertEqual(fixed, {19: "LINE", 42: "DICTIONARY"})
            self.assertEqual(registered, {19, 42})
            self.assertEqual(object_names, {"DICTIONARY"})

    def test_local_fixed_shell_routes_are_source_derived(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            util = root / "libraries/libdxfrw/src/intern/dwgutil.h"
            reader = root / "libraries/libdxfrw/src/intern/dwgreader.cpp"
            objects = root / "libraries/libdxfrw/src/drw_objects.h"
            for path in (util, reader, objects):
                path.parent.mkdir(parents=True, exist_ok=True)
            util.write_text(
                "enum Entity { LINE = 19 };\n"
                "enum Object { DICTIONARY = 42 };\n",
                encoding="utf-8",
            )
            reader.write_text(
                "bool dwgReader::readDwgEntity() { case dwgType::LINE: break; }\n"
                "bool dwgReader::readDwgObject() { case dwgObjType::DICTIONARY: break; }\n",
                encoding="utf-8",
            )
            objects.write_text(
                "static constexpr int kFixedAecEntityShellType = 1127;\n"
                "static const char* fixedEntityShellName(int type) {\n"
                "case kFixedAecEntityShellType: return \"AEC_WALL\";\n"
                "}\n"
                "static const char* fixedObjectShellName(int type) {\n"
                "case 1128: return \"AEC_WALL_STYLE\";\n"
                "}\n",
                encoding="utf-8",
            )
            names, entity_ids, object_ids = MODULE.local_dwg_routes(root)
            self.assertEqual(names[1127], "AEC_WALL")
            self.assertEqual(names[1128], "AEC_WALL_STYLE")
            self.assertIn(1127, entity_ids)
            self.assertIn(1128, object_ids)

    def test_missing_reference_still_generates_a_report(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "libraries/libdxfrw/src/intern").mkdir(parents=True)
            (root / "libraries/libdxfrw/src").mkdir(exist_ok=True)
            (root / "librecad/src/lib/filters").mkdir(parents=True)
            (root / "libraries/libdxfrw/src/drw_objects.h").write_text(
                "", encoding="utf-8"
            )
            (root / "libraries/libdxfrw/src/drw_interface.h").write_text(
                "virtual void addLine(const X&);\n", encoding="utf-8"
            )
            (root / "librecad/src/lib/filters/rs_filterdxfrw.cpp").write_text(
                "void RS_FilterDXFRW::addLine(const X&) {}\n", encoding="utf-8"
            )
            (root / "librecad/src/lib/filters/rs_filterdxfrw.h").write_text(
                "", encoding="utf-8"
            )
            (root / "libraries/libdxfrw/src/libdxfrw.cpp").write_text(
                "bool dxfRW::processEntities() { if (nextentity == \"LINE\") {} }\n"
                "bool dxfRW::processObjects() { }\n",
                encoding="utf-8",
            )
            (root / "libraries/libdxfrw/src/intern/dwgutil.h").write_text(
                "enum Entity { LINE = 19 };\nenum Object { DICTIONARY = 42 };\n",
                encoding="utf-8",
            )
            (root / "libraries/libdxfrw/src/intern/dwgreader.cpp").write_text(
                "bool dwgReader::readDwgEntity() { case dwgType::LINE: break; }\n"
                "bool dwgReader::readDwgObject() { }\n",
                encoding="utf-8",
            )
            rows = MODULE.build_rows(root, None, {})
            self.assertTrue(any(row.name == "LINE" and row.lib_dispatched for row in rows))
            self.assertTrue(all(row.dwg_registered is False for row in rows))


if __name__ == "__main__":
    unittest.main()
