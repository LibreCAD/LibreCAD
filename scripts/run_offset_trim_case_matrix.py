#!/usr/bin/env python3
"""Run each named Offset Trim test case in an isolated, auditable process."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time


SELECTOR = "[lc-offsetcurve][trim-action]"
RNG_SEED = "1"
SUMMARY = re.compile(r"\((\d+) assertions? in (\d+) test cases?\)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path, help="path to librecad_tests")
    parser.add_argument("--case-index", type=int, action="append",
                        help="zero-based case index to run (repeatable; defaults to all)")
    parser.add_argument("--skip-case-name", action="append", default=[],
                        help="exact test-case name covered by a dedicated profile matrix")
    parser.add_argument("--start-index", type=int, default=0,
                        help="first discovered case index, inclusive")
    parser.add_argument("--end-index", type=int,
                        help="last discovered case index, exclusive")
    parser.add_argument("--timeout-seconds", type=float, default=300.0,
                        help="per-case process timeout (default: 300 seconds)")
    parser.add_argument("--worker-timeout-ms", type=int,
                        help="test-only worker wait bound inherited by each case")
    parser.add_argument("--log-dir", type=Path,
                        help="optional directory for one combined output log per case")
    parser.add_argument("--summary-json", type=Path,
                        help="optional JSON manifest with case identities and outcomes")
    return parser.parse_args()


def run(command: list[str], env: dict[str, str], timeout: float) -> subprocess.CompletedProcess[str]:
    process = subprocess.Popen(
        command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, start_new_session=os.name != "nt",
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0,
    )
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired as error:
        if os.name != "nt":
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        else:
            try:
                killed = subprocess.run(
                    ["taskkill", "/T", "/F", "/PID", str(process.pid)],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                    check=False,
                )
                if killed.returncode != 0:
                    process.kill()
            except OSError:
                process.kill()
        output, _ = process.communicate()
        raise subprocess.TimeoutExpired(command, timeout, output=output) from error
    return subprocess.CompletedProcess(command, process.returncode, output)


def main() -> int:
    args = parse_args()
    binary = args.binary.resolve()
    if not binary.is_file():
        print(f"test binary does not exist: {binary}", file=sys.stderr)
        return 2
    if args.timeout_seconds <= 0 or args.start_index < 0:
        print("timeout must be positive and start index non-negative", file=sys.stderr)
        return 2
    if args.end_index is not None and args.end_index < args.start_index:
        print("end index must be greater than or equal to start index", file=sys.stderr)
        return 2
    if args.worker_timeout_ms is not None and args.worker_timeout_ms <= 0:
        print("--worker-timeout-ms must be positive", file=sys.stderr)
        return 2

    base_env = os.environ.copy()
    try:
        listed = run([str(binary), SELECTOR, "--list-tests", "--verbosity", "quiet",
                      "--rng-seed", RNG_SEED], base_env, args.timeout_seconds)
    except subprocess.TimeoutExpired:
        print("timed out discovering Trim test cases", file=sys.stderr)
        return 2
    if listed.returncode != 0:
        sys.stdout.write(listed.stdout)
        return 2
    cases = [line.strip() for line in listed.stdout.splitlines() if line.strip()]
    if not cases or len(cases) != len(set(cases)):
        print(f"invalid Catch2 test-case inventory ({len(cases)} entries)", file=sys.stderr)
        return 2
    missing_skips = [name for name in args.skip_case_name if name not in cases]
    if missing_skips or len(args.skip_case_name) != len(set(args.skip_case_name)):
        print(f"unknown or duplicate skipped case name(s): {missing_skips}", file=sys.stderr)
        return 2

    if args.case_index is not None:
        if args.start_index != 0 or args.end_index is not None:
            print("use either --case-index or an index range, not both", file=sys.stderr)
            return 2
        indices = args.case_index
        if not indices or len(indices) != len(set(indices)):
            print("case indices must be non-empty and unique", file=sys.stderr)
            return 2
    else:
        stop = len(cases) if args.end_index is None else args.end_index
        indices = list(range(args.start_index, stop))
    skipped_indices = {cases.index(name) for name in args.skip_case_name}
    indices = [index for index in indices if index not in skipped_indices]
    if not indices:
        print("selected case range is empty after dedicated-matrix exclusions", file=sys.stderr)
        return 2
    invalid = [index for index in indices if index < 0 or index >= len(cases)]
    if invalid:
        print(f"case index out of range (0..{len(cases) - 1}): {invalid}", file=sys.stderr)
        return 2

    if args.log_dir:
        args.log_dir.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    results: list[dict[str, object]] = []
    for ordinal, index in enumerate(indices, start=1):
        name = cases[index]
        env = base_env.copy()
        if args.worker_timeout_ms is not None:
            env["LIBRECAD_TEST_OFFSET_WORKER_TIMEOUT_MS"] = str(args.worker_timeout_ms)
        case_started = time.monotonic()
        try:
            completed = run([str(binary), name, "--reporter", "compact",
                             "--rng-seed", RNG_SEED], env, args.timeout_seconds)
            output = completed.stdout
            match = SUMMARY.search(output)
            assertions = int(match.group(1)) if match else 0
            count = int(match.group(2)) if match else 0
            passed = completed.returncode == 0 and count == 1 and assertions > 0
            detail = "passed" if passed else (
                f"exit={completed.returncode}, cases={count}, assertions={assertions}"
            )
        except subprocess.TimeoutExpired as error:
            output = error.stdout or ""
            if isinstance(output, bytes):
                output = output.decode(errors="replace")
            assertions = count = 0
            passed = False
            detail = f"timeout after {args.timeout_seconds:g}s"

        elapsed = time.monotonic() - case_started
        results.append({
            "index": index, "name": name, "passed": passed,
            "assertions": assertions, "test_cases": count,
            "elapsed_seconds": round(elapsed, 3), "detail": detail,
        })
        if args.log_dir:
            (args.log_dir / f"trim-{index:03d}.log").write_text(output)
        print(f"[{ordinal}/{len(indices)}] case {index}: {detail} "
              f"({assertions} assertions, {elapsed:.2f}s) - {name}", flush=True)

    passed_indices = [r["index"] for r in results if r["passed"]]
    failed_indices = [r["index"] for r in results if not r["passed"]]
    manifest = {
        "binary": str(binary), "selector": SELECTOR,
        "discovered_cases": len(cases), "requested_indices": indices,
        "skipped_cases": args.skip_case_name,
        "passed_indices": passed_indices, "failed_indices": failed_indices,
        "elapsed_seconds": round(time.monotonic() - started, 3),
        "results": results,
    }
    if args.summary_json:
        args.summary_json.parent.mkdir(parents=True, exist_ok=True)
        args.summary_json.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Trim cases: {len(passed_indices)}/{len(indices)} passed; "
          f"failed indices={failed_indices}", flush=True)
    return 0 if not failed_indices else 1


if __name__ == "__main__":
    raise SystemExit(main())
