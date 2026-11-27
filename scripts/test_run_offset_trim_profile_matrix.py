#!/usr/bin/env python3
"""Focused tests for isolated Trim profile receipts and timeout cleanup."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

from run_offset_trim_case_matrix import run as run_isolated


class ProfileRunnerTests(unittest.TestCase):
    @unittest.skipIf(os.name == "nt", "POSIX executable fixture")
    def test_manifest_records_binary_and_mode(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary = root / "fake-tests"
            binary.write_text("#!/bin/sh\necho 'All tests passed (1 assertion in 1 test case)'\n")
            binary.chmod(0o755)
            manifest = root / "manifest.json"
            env = os.environ.copy()
            env["LC_OFFSET_HYPERBOLA_HIGH_ORIGIN"] = "1"
            env.pop("LC_OFFSET_HYPERBOLA_VERY_HIGH_ORIGIN", None)
            completed = subprocess.run(
                [sys.executable, str(Path(__file__).with_name("run_offset_trim_profile_matrix.py")),
                 str(binary), "hyperbola", "--profile", "0", "--worker-timeout-ms", "30000",
                 "--summary-json", str(manifest)],
                env=env, capture_output=True, text=True, check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            result = json.loads(manifest.read_text())
            self.assertEqual(result["binary_sha256"], hashlib.sha256(binary.read_bytes()).hexdigest())
            self.assertTrue(result["binary_unchanged"])
            self.assertEqual(result["mode_environment"], {
                "LC_OFFSET_HYPERBOLA_HIGH_ORIGIN": "1",
                "LC_OFFSET_HYPERBOLA_VERY_HIGH_ORIGIN": None,
            })
            self.assertEqual(result["worker_timeout_ms"], 30000)
            self.assertEqual(result["passed_profiles"], [0])

    @unittest.skipIf(os.name == "nt", "POSIX executable fixture")
    def test_changed_binary_invalidates_receipt(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binary = root / "changing-tests"
            binary.write_text(
                "#!/bin/sh\necho '# changed' >> \"$0\"\n"
                "echo 'All tests passed (1 assertion in 1 test case)'\n"
            )
            binary.chmod(0o755)
            manifest = root / "manifest.json"
            completed = subprocess.run(
                [sys.executable, str(Path(__file__).with_name("run_offset_trim_profile_matrix.py")),
                 str(binary), "arc", "--profile", "0", "--summary-json", str(manifest)],
                capture_output=True, text=True, check=False,
            )
            self.assertEqual(completed.returncode, 1, completed.stdout + completed.stderr)
            self.assertFalse(json.loads(manifest.read_text())["binary_unchanged"])

    @unittest.skipIf(os.name == "nt", "POSIX process-group cleanup test")
    def test_timeout_kills_descendant(self):
        with tempfile.TemporaryDirectory() as directory:
            ready = Path(directory) / "ready"
            marker = Path(directory) / "survived"
            child = (
                "import pathlib,sys,time; pathlib.Path(sys.argv[1]).touch(); "
                "time.sleep(2); pathlib.Path(sys.argv[2]).touch()"
            )
            parent = (
                "import subprocess,sys,time; "
                "subprocess.Popen([sys.executable, '-c', sys.argv[1], sys.argv[2], sys.argv[3]]); "
                "time.sleep(10)"
            )
            with self.assertRaises(subprocess.TimeoutExpired):
                run_isolated([sys.executable, "-c", parent, child, str(ready), str(marker)],
                             os.environ.copy(), 1.0)
            time.sleep(1.2)
            self.assertTrue(ready.exists(), "descendant did not start before timeout")
            self.assertFalse(marker.exists(), "timed-out descendant survived")


if __name__ == "__main__":
    unittest.main()
