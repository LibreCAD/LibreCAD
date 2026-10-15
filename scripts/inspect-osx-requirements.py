#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD contributors
"""Report the actual arm64 macOS load-command floor of every bundled Mach-O."""

import argparse
import json
from pathlib import Path
import plistlib
import re
import subprocess


def version(value):
    parts = tuple(int(part) for part in value.split("."))
    return parts + (0,) * (3 - len(parts))


def inspect(app):
    app = app.resolve()
    with (app / "Contents/Info.plist").open("rb") as stream:
        plist = plistlib.load(stream)
    binaries, issues, seen = [], [], set()
    magic = {bytes.fromhex(value) for value in ("feedface", "cefaedfe", "feedfacf", "cffaedfe",
                                               "cafebabe", "bebafeca", "cafebabf", "bfbafeca")}
    for path in sorted(app.rglob("*")):
        if not path.is_file():
            continue
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        with path.open("rb") as stream:
            if stream.read(4) not in magic:
                continue
        name = str(path.relative_to(app))
        archs = subprocess.check_output(["/usr/bin/lipo", "-archs", str(path)], text=True).split()
        record = {"path": name, "architectures": archs}
        if "arm64" not in archs:
            issues.append(f"{name}: missing arm64")
            binaries.append(record)
            continue
        commands = subprocess.check_output(["/usr/bin/otool", "-arch", "arm64", "-l", str(path)], text=True)
        command, minimum, sdk = None, None, None
        for line in commands.splitlines():
            match = re.match(r"\s*cmd (\S+)$", line)
            if match:
                command = match[1]
            elif command in ("LC_BUILD_VERSION", "LC_VERSION_MIN_MACOSX"):
                field = re.match(r"\s*(minos|version|sdk) (\S+)$", line)
                if field:
                    if field[1] == "sdk":
                        sdk = field[2]
                    elif (command == "LC_BUILD_VERSION" and field[1] == "minos") or (
                            command == "LC_VERSION_MIN_MACOSX" and field[1] == "version"):
                        minimum = field[2]
        if minimum is None:
            issues.append(f"{name}: no macOS deployment load command")
        record.update(minimum_macos=minimum, sdk=sdk)
        binaries.append(record)
    floors = [item["minimum_macos"] for item in binaries if item.get("minimum_macos")]
    maximum = max(floors, key=version) if floors else None
    declared = plist.get("LSMinimumSystemVersionByArchitecture", {}).get("arm64", plist.get("LSMinimumSystemVersion"))
    if not binaries:
        issues.append("No Mach-O binaries found")
    if declared is None:
        issues.append("Info.plist does not declare LSMinimumSystemVersion for arm64")
    elif maximum and version(declared) < version(maximum):
        issues.append(f"Declared macOS {declared} is below bundled arm64 requirement {maximum}")
    return {"app": str(app), "slice": "arm64", "declared_minimum_macos": declared,
            "actual_minimum_macos": maximum, "mach_o_count": len(binaries), "binaries": binaries, "issues": issues}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = inspect(args.app)
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{report['mach_o_count']} Mach-O files; arm64 minimum macOS {report['actual_minimum_macos']}; "
          f"Info.plist declares {report['declared_minimum_macos']}")
    for issue in report["issues"]:
        print(f"ERROR: {issue}")
    return bool(report["issues"])


if __name__ == "__main__":
    raise SystemExit(main())
