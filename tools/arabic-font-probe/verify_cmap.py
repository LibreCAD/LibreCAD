#!/usr/bin/env python3
"""Verify pinned Amiri cmap coverage without FreeType or the C++ probe adapter.

The cmap reader is independent; shaping still uses HarfBuzz's hb-shape CLI.
This is not an independent shaping-engine comparison.
"""
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

import argparse
import hashlib
import itertools
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys


FONT_SHA256 = "cd2550c0f4c05eb341bf97958211aaa39382bca96577ba3a67d4a3b4912c43c0"
ALPHABET = (
    0x627, 0x628, 0x62A, 0x62B, 0x62C, 0x62D, 0x62E, 0x62F,
    0x630, 0x631, 0x632, 0x633, 0x634, 0x635, 0x636, 0x637,
    0x638, 0x639, 0x63A, 0x641, 0x642, 0x643, 0x644, 0x645,
    0x646, 0x647, 0x648, 0x64A,
)
SAMPLES = (
    (0x628, 0x628, 0x628), (0x644, 0x627), (0x644, 0x64E, 0x627),
    (0x633, 0x644, 0x627, 0x645), (0x645, 0x62D, 0x645, 0x62F),
    (0x628, 0x64E, 0x651), (0x628, 0x651, 0x64E), (0x628, 0x650, 0x651),
    (0x644, 0x644, 0x647), (0x639, 0x631), (0x642, 0x62D),
    (0x628, 0x200D, 0x628), (0x628, 0x200C, 0x628), (0x66E, 0x66E),
    (0x750, 0x750),
)
EXPECTED = {
    "cmapGlyphs": 1622, "coreTriples": 21952, "usesUncodedGlyph": 20944,
    "usesOffset": 671, "cursChanges": 418, "rligChanges": 8393,
    "distinctUsed": 415, "distinctUncoded": 387,
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def unicode_cmaps(data):
    """Read Unicode format 4/12 maps directly from the pinned SFNT bytes."""
    def u16(offset):
        return struct.unpack_from(">H", data, offset)[0]

    def u32(offset):
        return struct.unpack_from(">I", data, offset)[0]

    tables = {}
    for index in range(u16(4)):
        record = 12 + 16 * index
        offset, length = u32(record + 8), u32(record + 12)
        require(offset + length <= len(data), "SFNT table exceeds font length")
        tables[data[record:record + 4]] = offset
    require(b"cmap" in tables and b"maxp" in tables, "Required SFNT tables missing")
    glyph_count = u16(tables[b"maxp"] + 4)
    require(glyph_count == 6710, "Pinned glyph count differs")
    cmap = tables[b"cmap"]
    maps = {}
    for index in range(u16(cmap + 2)):
        record = cmap + 4 + 8 * index
        platform, encoding = u16(record), u16(record + 2)
        if not (platform == 0 or platform == 3 and encoding in (1, 10)):
            continue
        subtable = cmap + u32(record + 4)
        format_id = u16(subtable)
        glyphs = set()
        if format_id == 12:
            for group in range(u32(subtable + 12)):
                offset = subtable + 16 + 12 * group
                start, end, first = u32(offset), u32(offset + 4), u32(offset + 8)
                require(start <= end <= 0x10FFFF, "Invalid Unicode cmap group")
                glyphs.update(range(first, first + end - start + 1))
        elif format_id == 4:
            count = u16(subtable + 6) // 2
            ends = subtable + 14
            starts = ends + 2 * count + 2
            deltas = starts + 2 * count
            ranges = deltas + 2 * count
            for segment in range(count):
                start, end = u16(starts + 2 * segment), u16(ends + 2 * segment)
                require(start <= end, "Invalid BMP cmap segment")
                for scalar in range(start, end + 1):
                    delta, distance = u16(deltas + 2 * segment), u16(ranges + 2 * segment)
                    if distance:
                        gid = u16(ranges + 2 * segment + distance + 2 * (scalar - start))
                        if gid:
                            gid = (gid + delta) & 0xFFFF
                    else:
                        gid = (scalar + delta) & 0xFFFF
                    if gid:
                        glyphs.add(gid)
        else:
            raise ValueError(f"Unsupported Unicode cmap format {format_id}")
        glyphs.discard(0)
        require(glyphs and max(glyphs) < glyph_count, "Empty or invalid cmap glyph coverage")
        maps[(platform, encoding, format_id)] = frozenset(glyphs)
        print(f"cmap platform={platform} encoding={encoding} format={format_id} distinctGIDs={len(glyphs)}")
    require((3, 10, 12) in maps, "Pinned Windows full-Unicode cmap missing")
    encoded = maps[(3, 10, 12)]
    require(maps.get((0, 4, 12)) == encoded, "Unicode format 12 cmap disagreement")
    return encoded, glyph_count


def run_command(arguments, **kwargs):
    return subprocess.run(arguments, text=True, capture_output=True, check=True, timeout=60, **kwargs)


def shape(executable, font, texts, glyph_count, feature=None):
    arguments = [
        executable, "--font-funcs=ot", "--font-size=upem", "--shapers=ot",
        "--direction=rtl", "--script=arab", "--language=ar", "--cluster-level=0",
        "--output-format=json", "--no-glyph-names", str(font),
    ]
    if feature:
        arguments.append("--features=" + feature)
    result = run_command(arguments, input="\n".join(texts) + "\n")
    lines = [json.loads(line) for line in result.stdout.splitlines()]
    require(len(lines) == len(texts), "Shaper returned the wrong number of lines")
    for line in lines:
        require(isinstance(line, list) and line, "Empty or malformed shaping output")
        for glyph in line:
            require(isinstance(glyph, dict) and all(
                type(glyph.get(key)) is int for key in ("g", "cl", "ax", "ay", "dx", "dy")
            ), "Invalid glyph record from hb-shape")
            require(0 < glyph["g"] < glyph_count, "Missing or invalid shaped GID")
    return lines


def verify(font, executable):
    require(font.stat().st_size <= 32 * 1024 * 1024, "Font exceeds audit size limit")
    data = font.read_bytes()
    checksum = hashlib.sha256(data).hexdigest()
    print(f"Python={sys.version.split()[0]} fontSHA256={checksum}")
    require(checksum == FONT_SHA256, "Font differs from pinned Amiri 1.003 bytes")
    version = run_command([executable, "--version"]).stdout.strip()
    require(version, "hb-shape did not report its version")
    print(version)
    print("Independent cmap parser; HarfBuzz CLI shaping, not a second shaping-engine oracle")
    encoded, glyph_count = unicode_cmaps(data)
    words = ["".join(map(chr, values)) for values in itertools.product(ALPHABET, repeat=3)]
    normal = shape(executable, font, words, glyph_count)
    without_curs = shape(executable, font, words, glyph_count, "curs=0")
    without_rlig = shape(executable, font, words, glyph_count, "rlig=0")
    samples = shape(executable, font, ["".join(map(chr, text)) for text in SAMPLES], glyph_count)
    used = {glyph["g"] for line in normal for glyph in line}
    actual = {
        "cmapGlyphs": len(encoded), "coreTriples": len(normal),
        "usesUncodedGlyph": sum(any(g["g"] not in encoded for g in line) for line in normal),
        "usesOffset": sum(any(g["dx"] or g["dy"] for g in line) for line in normal),
        "cursChanges": sum(a != b for a, b in zip(normal, without_curs)),
        "rligChanges": sum(a != b for a, b in zip(normal, without_rlig)),
        "distinctUsed": len(used), "distinctUncoded": len(used - encoded),
    }
    print("INDEPENDENT " + " ".join(f"{key}={value}" for key, value in actual.items()))
    inserted = {glyph["g"] for line in samples for glyph in line} - encoded
    mutated = encoded | inserted
    old_triples = sum(any(g["g"] not in mutated for g in line) for line in normal)
    old_distinct = len(used - mutated)
    print(f"OLD_PRINT_BUG addedUncodedSampleKeys={len(inserted)} "
          f"usesUncodedGlyph={old_triples} distinctUncoded={old_distinct} "
          f"sampleInsertedGIDs={sorted(inserted)}")
    require(actual == EXPECTED, f"Pinned counts differ: expected {EXPECTED}")
    require((len(inserted), old_triples, old_distinct) == (25, 20819, 369),
            "Original print-mutation reproduction differs")
    print("Independent cmap audit: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("font", type=Path, help="Pinned Amiri 1.003 Amiri-Regular.ttf")
    parser.add_argument("--hb-shape", default=shutil.which("hb-shape"), help="hb-shape executable (default: PATH)")
    arguments = parser.parse_args()
    try:
        require(arguments.hb_shape, "hb-shape not found; install it or pass --hb-shape")
        verify(arguments.font.resolve(), arguments.hb_shape)
    except (OSError, ValueError, struct.error, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
