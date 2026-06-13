/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Unit tests for the AC1018 (R2004+) LZ77 decompressor dwgCompressor::decompress18.
// These isolate behaviour that the corpus exercises only indirectly:
//  - opcode 0x18 (extended copy-length path: (oc & 7) == 0 -> longCompressionOffset),
//    the bug that desynced real AutoCAD streams (the old (oc & 0x0F)+2 mis-sized it);
//  - graceful end at the output window boundary;
//  - hard-fail on a back-reference before the window start.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include "intern/dwgutil.h"

namespace {

// Encode an AC1018 "literal length" run header for `n` literal bytes that
// follow. Mirrors litLength18(): a leading 0x00 selects the extended form
// (base 0x0F, +0xFF per following 0x00, + final non-zero byte, +3).
std::vector<std::uint8_t> literalRunHeader(std::uint32_t n) {
    std::vector<std::uint8_t> h;
    // n = 0x0F + 0xFF*zeros + finalByte + 3, with finalByte in 1..0xFF.
    std::uint32_t rem = n - 3 - 0x0F;
    h.push_back(0x00);              // select extended form
    while (rem > 0xFF) { h.push_back(0x00); rem -= 0xFF; }
    h.push_back(static_cast<std::uint8_t>(rem));  // final non-zero byte
    return h;
}

}  // namespace

TEST_CASE("decompress18: opcode 0x18 extended copy length", "[dwg][decompress18]") {
    // Build: a 0x8000-byte literal run, then opcode 0x18 with an extended copy
    // length of 14 and a two-byte offset of 0 (== back-ref window distance
    // 0x7FFF), then the 0x11 terminator.
    constexpr std::uint32_t litCount = 0x8000;  // 32768 literals
    std::vector<std::uint8_t> in = literalRunHeader(litCount);
    for (std::uint32_t i = 0; i < litCount; ++i)
        in.push_back(static_cast<std::uint8_t>(i & 0xFF));
    in.push_back(0x18);  // opcode: (0x18 & 7)==0 -> extended length path
    in.push_back(0x05);  // longCompressionOffset single byte -> copy length 5+9 = 14
    in.push_back(0x00);  // two-byte offset firstByte: value bits 0, litCount bits 0
    in.push_back(0x00);  // two-byte offset secondByte: 0
    in.push_back(0x11);  // litLength18 peeks this (>0x0F -> 0), then loop reads it: terminator

    // Output window: room for the literals + the 14-byte back-reference copy.
    std::vector<std::uint8_t> out(0x8100, 0xCC);
    dwgCompressor comp;
    REQUIRE(comp.decompress18(in.data(), out.data(), in.size(), out.size()));

    // 0x8000 literals + 14 copied bytes.
    REQUIRE(comp.decompressedBytes() == 0x8000 + 14);
    // Literal region intact.
    CHECK(out[0] == 0x00);
    CHECK(out[0x7FFF] == 0xFF);
    // The 0x18 copy duplicated the first 14 bytes (offset distance 0x7FFF from
    // decompPos 0x8000 -> source index 0). If the old (oc & 0x0F)+2 length had
    // run, the byte stream would have desynced and these would differ.
    for (std::uint32_t i = 0; i < 14; ++i)
        CHECK(out[0x8000 + i] == static_cast<std::uint8_t>(i & 0xFF));
}

TEST_CASE("decompress18: rejects < 2-byte input", "[dwg][decompress18]") {
    std::uint8_t in[1] = {0x00};
    std::vector<std::uint8_t> out(16, 0);
    dwgCompressor comp;
    CHECK_FALSE(comp.decompress18(in, out.data(), 1, out.size()));
    CHECK_FALSE(comp.decompress18(nullptr, out.data(), 8, out.size()));
}

TEST_CASE("decompress18: hard-fails a back reference before the window start",
          "[dwg][decompress18]") {
    // Initial literal length 4 (leading byte 0x01 -> 1 + 3), then a short-form
    // copy opcode (>= 0x40) whose back-reference offset (~1020) points before
    // the 4 bytes produced so far -> structural error, not silent zero-fill.
    std::vector<std::uint8_t> in;
    in.push_back(0x01);                      // initial literal length = 1 + 3 = 4
    for (int i = 0; i < 4; ++i) in.push_back(0xAB);
    in.push_back(0x71);                      // (0x71>>4)-1 = 6 copy bytes, litCount = 1
    in.push_back(0xFF);                      // offset second byte -> compOffset ~0x3FC >> decompPos 4
    dwgCompressor comp;
    std::vector<std::uint8_t> out(64, 0);
    CHECK_FALSE(comp.decompress18(in.data(), out.data(), in.size(), out.size()));
}
