#!/usr/bin/env python3
"""Run pinned generated Offset Trim profiles with auditable case accounting."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time


FAMILIES = {
    "arc": {
        "environment": "LC_OFFSET_TRIM_TYPED_ARC_PROFILE",
        "profiles": range(19),
        "test_spec": "[offset-trim-profile-arc]",
    },
    "ellipse": {
        "environment": "LC_OFFSET_TRIM_TYPED_ELLIPSE_ROW",
        "profiles": range(76),
        "test_spec": "[offset-trim-profile-ellipse]",
    },
    "parabola": {
        "environment": "LC_OFFSET_TRIM_TYPED_PARABOLA_PROFILE",
        "profiles": range(152),
        "test_spec": "[offset-trim-profile-parabola]",
    },
    "spline": {
        "environment": "LC_OFFSET_TRIM_TYPED_SPLINE_PROFILE",
        "profiles": range(43),
        "test_spec": "[offset-trim-profile-spline]",
    },
    "polyline": {
        "environment": "LC_OFFSET_TRIM_POLYLINE_PROFILE_KIND",
        "profiles": range(4),
        "test_spec": "[offset-trim-profile-polyline]",
    },
}

SUMMARY = re.compile(r"\((\d+) assertions? in (\d+) test cases?\)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path, help="path to librecad_tests")
    parser.add_argument("family", choices=FAMILIES)
    parser.add_argument(
        "--profile",
        type=int,
        action="append",
        help="profile to run (repeatable; defaults to every profile in the family)",
    )
    parser.add_argument(
        "--timeout-seconds", type=float, default=120.0,
        help="per-profile timeout (default: 120 seconds)",
    )
    parser.add_argument(
        "--log-dir", type=Path,
        help="optional directory for one combined stdout/stderr log per profile",
    )
    parser.add_argument(
        "--summary-json", type=Path,
        help="optional JSON result manifest, including passed and failed profile IDs",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    binary = args.binary.resolve()
    if not binary.is_file():
        print(f"test binary does not exist: {binary}", file=sys.stderr)
        return 2
    if args.timeout_seconds <= 0:
        print("--timeout-seconds must be positive", file=sys.stderr)
        return 2

    family = FAMILIES[args.family]
    profiles = args.profile if args.profile is not None else list(family["profiles"])
    if not profiles or len(set(profiles)) != len(profiles):
        print("profile list must be non-empty and contain no duplicates", file=sys.stderr)
        return 2
    allowed = set(family["profiles"])
    invalid = [profile for profile in profiles if profile not in allowed]
    if invalid:
        print(f"invalid {args.family} profile(s): {invalid}", file=sys.stderr)
        return 2

    if args.log_dir:
        args.log_dir.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    results: list[dict[str, object]] = []
    for index, profile in enumerate(profiles, start=1):
        env = os.environ.copy()
        env[family["environment"]] = str(profile)
        command = [str(binary), family["test_spec"], "--reporter", "compact"]
        case_started = time.monotonic()
        try:
            completed = subprocess.run(
                command,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=args.timeout_seconds,
                check=False,
            )
            output = completed.stdout
            match = SUMMARY.search(output)
            assertions = int(match.group(1)) if match else 0
            cases = int(match.group(2)) if match else 0
            passed = completed.returncode == 0 and cases == 1 and assertions > 0
            detail = "passed" if passed else (
                f"exit={completed.returncode}, cases={cases}, assertions={assertions}"
            )
        except subprocess.TimeoutExpired as error:
            output = error.stdout or ""
            if isinstance(output, bytes):
                output = output.decode(errors="replace")
            assertions = 0
            cases = 0
            passed = False
            detail = f"timeout after {args.timeout_seconds:g}s"

        elapsed = time.monotonic() - case_started
        result = {
            "family": args.family,
            "profile": profile,
            "passed": passed,
            "assertions": assertions,
            "test_cases": cases,
            "elapsed_seconds": round(elapsed, 3),
            "detail": detail,
        }
        results.append(result)
        if args.log_dir:
            (args.log_dir / f"{args.family}-{profile}.log").write_text(output)
        print(
            f"[{index}/{len(profiles)}] {args.family} profile {profile}: {detail} "
            f"({assertions} assertions, {elapsed:.2f}s)",
            flush=True,
        )

    passed_profiles = [r["profile"] for r in results if r["passed"]]
    failed_profiles = [r["profile"] for r in results if not r["passed"]]
    manifest = {
        "binary": str(binary),
        "family": args.family,
        "requested_profiles": profiles,
        "passed_profiles": passed_profiles,
        "failed_profiles": failed_profiles,
        "elapsed_seconds": round(time.monotonic() - started, 3),
        "results": results,
    }
    if args.summary_json:
        args.summary_json.parent.mkdir(parents=True, exist_ok=True)
        args.summary_json.write_text(json.dumps(manifest, indent=2) + "\n")

    print(
        f"{args.family}: {len(passed_profiles)}/{len(profiles)} profiles passed; "
        f"failed={failed_profiles}",
        flush=True,
    )
    return 0 if not failed_profiles else 1


if __name__ == "__main__":
    raise SystemExit(main())
