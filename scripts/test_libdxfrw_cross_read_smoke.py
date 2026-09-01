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

"""Unit tests for the local libdxfrw cross-read smoke runner."""

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("libdxfrw_cross_read_smoke.py")
SPEC = importlib.util.spec_from_file_location("libdxfrw_cross_read_smoke", SCRIPT)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class CrossReadSmokeTests(unittest.TestCase):
    def test_fixture_manifest_covers_version_span(self) -> None:
        versions = {fixture["version"] for fixture in MODULE.FIXTURES}
        self.assertEqual(
            versions,
            {"AC1009", "AC1015", "AC1018", "AC1021", "AC1027", "AC1032"},
        )
        self.assertEqual(len(MODULE.FIXTURES), 8)
        self.assertEqual(
            len({fixture["id"] for fixture in MODULE.FIXTURES}),
            len(MODULE.FIXTURES),
        )

    def test_run_fixture_accepts_valid_dump(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fixture.dwg"
            source.write_bytes(b"fixture")
            dump = root / "dump.py"
            payload = {
                "sourceFormat": "dwg",
                "version": "AC1027",
                "diagnostics": {"entityParseFailures": 0, "objectParseFailures": 0},
                "entities": [
                    {"type": "LINE", "handle": "A"},
                    {"type": "LINE", "handle": "B"},
                ],
                "objects": [],
            }
            dump.write_text(
                "#!/usr/bin/env python3\n"
                "import json\n"
                f"print({json.dumps(payload)!r})\n",
                encoding="utf-8",
            )
            dump.chmod(0o755)
            result = MODULE.run_fixture(
                dump,
                root,
                {
                    "id": "fixture",
                    "path": "fixture.dwg",
                    "version": "AC1027",
                    "entityCounts": {"LINE": 2},
                    "uniqueNonzeroHandles": True,
                },
            )
            self.assertEqual(result["status"], "passed")
            self.assertEqual(result["entityHandleCount"], 2)
            self.assertTrue(result["uniqueEntityHandles"])
            self.assertTrue(result["nonzeroEntityHandles"])

    def test_run_fixture_rejects_parse_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "fixture.dwg"
            source.write_bytes(b"fixture")
            dump = root / "dump.py"
            payload = {
                "sourceFormat": "dwg",
                "version": "AC1027",
                "diagnostics": {"entityParseFailures": 1, "objectParseFailures": 0},
                "entities": [],
                "objects": [],
            }
            dump.write_text(
                "#!/usr/bin/env python3\n"
                "import json\n"
                f"print({json.dumps(payload)!r})\n",
                encoding="utf-8",
            )
            dump.chmod(0o755)
            result = MODULE.run_fixture(
                dump,
                root,
                {
                    "id": "fixture",
                    "path": "fixture.dwg",
                    "version": "AC1027",
                    "entityCounts": {},
                    "uniqueNonzeroHandles": False,
                },
            )
            self.assertEqual(result["status"], "failed")
            self.assertTrue(any("entityParseFailures" in error for error in result["errors"]))

    def test_handle_digest_is_order_independent(self) -> None:
        self.assertEqual(
            MODULE.digest_handles(["B", "A"]),
            MODULE.digest_handles(["A", "B"]),
        )


if __name__ == "__main__":
    unittest.main()
