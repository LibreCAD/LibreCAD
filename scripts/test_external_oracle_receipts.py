#!/usr/bin/env python3
# File: test_external_oracle_receipts.py

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

"""Regression tests for per-fixture external-oracle receipts."""

from __future__ import annotations

import hashlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from argparse import Namespace
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

from oracle_receipts import ReceiptError, validate_receipt, write_receipt_atomic
from run_external_oracles import receipt_template, run_manifest_receipts


class ExternalOracleReceiptTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.fixture = self.root / "fixture.dwg"
        self.fixture.write_bytes(b"AC1015fixture payload")
        self.digest = hashlib.sha256(self.fixture.read_bytes()).hexdigest()
        self.manifest = self.root / "fixture_manifest.json"
        self.receipt_dir = self.root / "receipts"
        self.write_manifest()

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write_manifest(self, **changes: object) -> None:
        fixture = self.dwg_entry(**changes)
        self.manifest.write_text(json.dumps({"schema": 1, "fixtures": [fixture]}),
                                 encoding="utf-8")

    def dwg_entry(self, **changes: object) -> dict[str, object]:
        fixture: dict[str, object] = {
            "id": "fixture-ac1015",
            "path": "fixture.dwg",
            "format": "DWG",
            "version": "AC1015",
            "sha256": self.digest,
            "defaultEnabled": True,
        }
        fixture.update(changes)
        return fixture

    def dxf_entry(self) -> dict[str, object]:
        fixture = self.root / "fixture.dxf"
        fixture.write_text(
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
            "0\nENDSEC\n0\nEOF\n", encoding="ascii")
        digest = hashlib.sha256(fixture.read_bytes()).hexdigest()
        return {
            "id": "fixture-ac1021-dxf",
            "path": "fixture.dxf",
            "format": "DXF",
            "version": "AC1021",
            "sha256": digest,
            "defaultEnabled": True,
            "requiredOracles": ["ezdxf:audit"],
        }

    def write_dxf_manifest(self) -> None:
        self.manifest.write_text(json.dumps({"schema": 1,
                                             "fixtures": [self.dxf_entry()]}),
                                 encoding="utf-8")

    def write_mixed_manifest(self) -> None:
        self.manifest.write_text(json.dumps({"schema": 1, "fixtures": [
            self.dwg_entry(), self.dxf_entry()]}), encoding="utf-8")

    def args(self, *, fixture_ids: list[str] | None = None,
             default_fixtures: bool = False, require_oracle: bool = False,
             only: list[str] | None = None) -> Namespace:
        return Namespace(
            manifest=self.manifest,
            receipt_dir=self.receipt_dir,
            fixture=fixture_ids or [],
            default_fixtures=default_fixtures,
            require_oracle=require_oracle,
            json=True,
            only=only,
        )

    @staticmethod
    def config(path: str | None = None,
               ezdxf_python: str | None = None) -> dict[str, object]:
        entry: dict[str, object] = {"mode": "optional"}
        if path is not None:
            entry["path"] = path
        config: dict[str, object] = {"libredwg": entry}
        if ezdxf_python is not None:
            config["ezdxf"] = {"mode": "optional", "python": ezdxf_python}
        return config

    def receipt(self) -> dict[str, object]:
        files = self.receipt_files()
        self.assertEqual(1, len(files))
        return json.loads(files[0].read_text(encoding="utf-8"))

    def receipt_files(self) -> list[Path]:
        return sorted(self.receipt_dir.glob("*.json"))

    def run_receipts(self, args: Namespace, config: dict[str, object]) -> int:
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return run_manifest_receipts(self.root, args, config)

    def executable(self) -> Path:
        executable = self.root / "dwgread"
        executable.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        executable.chmod(0o755)
        return executable

    def test_default_fixture_without_tool_writes_skipped_receipt(self) -> None:
        self.assertEqual(0, self.run_receipts(
            self.args(default_fixtures=True), self.config()))
        receipt = self.receipt()
        self.assertEqual("skipped", receipt["status"])
        self.assertEqual("tool-unavailable", receipt["failureClass"])
        self.assertNotIn("output", receipt)
        validate_receipt(receipt)

    def test_require_oracle_escalates_missing_tool(self) -> None:
        self.assertEqual(2, self.run_receipts(
            self.args(default_fixtures=True, require_oracle=True), self.config()))
        self.assertEqual("missing-required", self.receipt()["status"])

    def test_stale_hash_creates_integrity_receipt_without_running_tool(self) -> None:
        self.write_manifest(sha256="0" * 64)
        with mock.patch("run_external_oracles.subprocess.run") as run:
            self.assertEqual(1, self.run_receipts(
                self.args(default_fixtures=True), self.config()))
        run.assert_not_called()
        receipt = self.receipt()
        self.assertEqual("failed", receipt["status"])
        self.assertEqual("fixture-integrity", receipt["failureClass"])

    def test_magic_mismatch_creates_integrity_receipt_without_running_tool(self) -> None:
        self.write_manifest(version="AC1018")
        with mock.patch("run_external_oracles.subprocess.run") as run:
            self.assertEqual(1, self.run_receipts(
                self.args(default_fixtures=True), self.config()))
        run.assert_not_called()
        self.assertIn("magic", self.receipt()["diagnostics"][0])

    def test_unknown_and_duplicate_fixture_selectors_fail_before_receipts(self) -> None:
        self.assertEqual(2, self.run_receipts(
            self.args(fixture_ids=["unknown"]), self.config()))
        self.assertFalse(self.receipt_dir.exists())
        self.assertEqual(2, self.run_receipts(
            self.args(fixture_ids=["fixture-ac1015", "fixture-ac1015"]), self.config()))
        self.assertFalse(self.receipt_dir.exists())

    def test_path_escape_is_rejected_before_tool_execution(self) -> None:
        outside = self.root.with_name(self.root.name + "-outside.dwg")
        outside.write_bytes(self.fixture.read_bytes())
        self.write_manifest(path="../outside.dwg")
        try:
            with mock.patch("run_external_oracles.subprocess.run") as run:
                self.assertEqual(1, self.run_receipts(
                    self.args(default_fixtures=True), self.config()))
            run.assert_not_called()
            self.assertEqual("fixture-integrity", self.receipt()["failureClass"])
        finally:
            outside.unlink(missing_ok=True)

    def test_successful_open_keeps_failed_version_probe_as_diagnostic(self) -> None:
        executable = self.executable()
        version_probe = subprocess.CompletedProcess([], 1, "", "")
        open_result = subprocess.CompletedProcess([], 0, "opened", "")
        with mock.patch("run_external_oracles.subprocess.run",
                        side_effect=[version_probe, open_result]) as run:
            self.assertEqual(0, self.run_receipts(
                self.args(default_fixtures=True), self.config(str(executable))))
        self.assertEqual(2, run.call_count)
        receipt = self.receipt()
        self.assertEqual("passed", receipt["status"])
        self.assertEqual("none", receipt["failureClass"])
        self.assertEqual("unknown", receipt["oracle"]["version"])

    def test_nonzero_fatal_and_timeout_opens_have_distinct_results(self) -> None:
        executable = self.executable()
        cases = [
            ([subprocess.CompletedProcess([], 0, "dwgread 1\n", ""),
              subprocess.CompletedProcess([], 3, "", "")], "oracle-rejected", 1),
            ([subprocess.CompletedProcess([], 0, "dwgread 1\n", ""),
              subprocess.CompletedProcess([], 0, "ERROR 0x1 bad\n", "")], "oracle-rejected", 1),
            ([subprocess.CompletedProcess([], 0, "dwgread 1\n", ""),
              subprocess.TimeoutExpired([], 120)], "timeout", 1),
        ]
        for side_effect, failure_class, exit_code in cases:
            with self.subTest(failure_class=failure_class), \
                 mock.patch("run_external_oracles.subprocess.run", side_effect=side_effect):
                self.assertEqual(exit_code, self.run_receipts(
                    self.args(default_fixtures=True), self.config(str(executable))))
                self.assertEqual(failure_class, self.receipt()["failureClass"])

    def test_dxf_fixture_uses_its_ezdxf_audit_adapter(self) -> None:
        self.write_dxf_manifest()
        version_probe = subprocess.CompletedProcess([], 0, "ezdxf 1.4\n", "")
        audit_result = subprocess.CompletedProcess([], 0, "", "")
        with mock.patch("run_external_oracles.subprocess.run",
                        side_effect=[version_probe, audit_result]) as run:
            self.assertEqual(0, self.run_receipts(
                self.args(default_fixtures=True),
                self.config(ezdxf_python=sys.executable)))
        self.assertEqual(2, run.call_count)
        receipt = self.receipt()
        self.assertEqual("ezdxf", receipt["oracle"]["name"])
        self.assertEqual("audit", receipt["operation"]["name"])
        self.assertEqual("DXF", receipt["operation"]["sourceFormat"])
        self.assertEqual("passed", receipt["status"])

    def test_missing_ezdxf_dependency_is_optional_tool_unavailable(self) -> None:
        self.write_dxf_manifest()
        missing_dependency = subprocess.CompletedProcess(
            [], 2, "", "error: ezdxf import failed: No module named 'ezdxf'\n")
        with mock.patch("run_external_oracles.subprocess.run",
                        return_value=missing_dependency) as run:
            self.assertEqual(0, self.run_receipts(
                self.args(default_fixtures=True),
                self.config(ezdxf_python=sys.executable)))
        self.assertEqual(1, run.call_count)
        receipt = self.receipt()
        self.assertEqual("skipped", receipt["status"])
        self.assertEqual("tool-unavailable", receipt["failureClass"])

    def test_ezdxf_audit_rejection_and_timeout_have_distinct_receipts(self) -> None:
        self.write_dxf_manifest()
        version_probe = subprocess.CompletedProcess([], 0, "ezdxf 1.4\n", "")
        cases = [
            ([version_probe, subprocess.CompletedProcess([], 1, "", "")],
             "oracle-rejected"),
            ([version_probe, subprocess.TimeoutExpired([], 120)], "timeout"),
        ]
        for side_effect, failure_class in cases:
            with self.subTest(failure_class=failure_class), \
                 mock.patch("run_external_oracles.subprocess.run",
                            side_effect=side_effect):
                self.assertEqual(1, self.run_receipts(
                    self.args(default_fixtures=True),
                    self.config(ezdxf_python=sys.executable)))
                self.assertEqual(failure_class, self.receipt()["failureClass"])

    def test_default_batch_selects_both_compatible_adapters(self) -> None:
        self.write_mixed_manifest()
        executable = self.executable()
        with mock.patch("run_external_oracles.subprocess.run", side_effect=[
                subprocess.CompletedProcess([], 0, "dwgread 1\n", ""),
                subprocess.CompletedProcess([], 0, "", ""),
                subprocess.CompletedProcess([], 0, "ezdxf 1.4\n", ""),
                subprocess.CompletedProcess([], 0, "", "")]) as run:
            self.assertEqual(0, self.run_receipts(
                self.args(default_fixtures=True),
                self.config(str(executable), sys.executable)))
        self.assertEqual(4, run.call_count)
        receipts = [json.loads(path.read_text(encoding="utf-8"))
                    for path in self.receipt_files()]
        self.assertEqual({"libredwg", "ezdxf"},
                         {receipt["oracle"]["name"] for receipt in receipts})

    def test_default_batch_filters_an_explicit_adapter_by_format(self) -> None:
        self.write_mixed_manifest()
        executable = self.executable()
        with mock.patch("run_external_oracles.subprocess.run", side_effect=[
                subprocess.CompletedProcess([], 0, "dwgread 1\n", ""),
                subprocess.CompletedProcess([], 0, "", "")]) as run:
            self.assertEqual(0, self.run_receipts(
                self.args(default_fixtures=True, only=["libredwg"]),
                self.config(str(executable))))
        self.assertEqual(2, run.call_count)
        receipt = self.receipt()
        self.assertEqual("libredwg", receipt["oracle"]["name"])
        self.assertEqual("fixture-ac1015", receipt["fixture"]["id"])

    def test_explicit_fixture_rejects_an_incompatible_adapter(self) -> None:
        self.write_dxf_manifest()
        self.assertEqual(2, self.run_receipts(
            self.args(fixture_ids=["fixture-ac1021-dxf"], only=["libredwg"]),
            self.config()))
        self.assertFalse(self.receipt_dir.exists())

    def test_atomic_write_replaces_only_its_receipt(self) -> None:
        fixture = {
            "id": "fixture-ac1015",
            "path": "fixture.dwg",
            "format": "DWG",
            "version": "AC1015",
            "sha256": self.digest,
        }
        receipt = receipt_template(fixture, "not-configured", "unavailable",
                                   "optional", "skipped", "tool-unavailable", ["first"])
        destination = write_receipt_atomic(self.receipt_dir, receipt)
        receipt["diagnostics"] = ["second"]
        self.assertEqual(destination, write_receipt_atomic(self.receipt_dir, receipt))
        self.assertEqual([destination], list(self.receipt_dir.glob("*.json")))
        self.assertFalse(list(self.receipt_dir.glob("*.tmp")))
        self.assertEqual(["second"], json.loads(destination.read_text())["diagnostics"])

    def test_producing_receipts_require_an_output_hash(self) -> None:
        fixture = {
            "id": "fixture-ac1015",
            "path": "fixture.dwg",
            "format": "DWG",
            "version": "AC1015",
            "sha256": self.digest,
        }
        receipt = receipt_template(fixture, "tool", "1.0", "optional",
                                   "passed", "none", [])
        receipt["operation"] = {
            "name": "convert",
            "sourceFormat": "DWG",
            "targetFormat": "DXF",
            "targetVersion": "AC1015",
        }
        with self.assertRaises(ReceiptError):
            validate_receipt(receipt)
        receipt["output"] = {"format": "DXF", "sha256": self.digest}
        validate_receipt(receipt)


if __name__ == "__main__":
    unittest.main()
