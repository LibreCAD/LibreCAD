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

#include <array>
#include <cstdint>
#include <vector>

#include "intern/rscodec.h"
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

TEST_CASE("RScodec: invalid configuration fails closed", "[dwg][rscodec][safety]") {
    RScodec invalidField(0x96, 0, 8);
    CHECK_FALSE(invalidField.isOkey());
    CHECK_FALSE(invalidField.encode(nullptr, nullptr));
    CHECK(invalidField.decode(nullptr) == -1);

    RScodec invalidCorrection(0x96, 8, 128);
    CHECK_FALSE(invalidCorrection.isOkey());
    CHECK_FALSE(invalidCorrection.encode(nullptr, nullptr));
    CHECK(invalidCorrection.decode(nullptr) == -1);

    // Symbols are unsigned char: GF(2^4) would index its 16-entry log table
    // with byte values, and GF(2^16) with tt == 8 would truncate symbols (and
    // with large tt overflow int in the syndrome exponents).
    for (const int mm : {4, 7, 9, 16, 30}) {
        CAPTURE(mm);
        RScodec unsupportedField(0x96, mm, 8);
        CHECK_FALSE(unsupportedField.isOkey());
        CHECK(unsupportedField.decode(nullptr) == -1);
    }

    // A non-primitive polynomial leaves symbols without a log. pp == 0 gives
    // alpha^8 == 0; 0xFF gives x^8 + x^7 + ... + 1, which divides x^9 + 1,
    // so alpha has order 9.
    for (const unsigned int pp : {0x00u, 0xFFu}) {
        CAPTURE(pp);
        RScodec nonPrimitive(pp, 8, 8);
        CHECK_FALSE(nonPrimitive.isOkey());
        CHECK(nonPrimitive.decode(nullptr) == -1);
    }

    CHECK(RScodec(0x96, 8, 8).isOkey());
    CHECK(RScodec(0xB8, 8, 2).isOkey());
}

namespace {

// Coefficients of prod_{i=1..k} (x + alpha^i) over the GF(256) that
// RScodec(pp, 8, tt) builds. RScodec reads pp bit-reversed, so its
// alpha^8 is the reverse of pp's bits; alpha is 2. Read as a received
// block, the polynomial makes syndromes 1..k zero and syndrome k+1
// non-zero.
std::array<unsigned char, 255> rsSyndromeVanishingBlock(unsigned int pp, int k) {
    int alpha8 = 0;
    for (int bit = 0; bit < 8; ++bit)
        if ((pp >> (7 - bit)) & 1u)
            alpha8 |= 1 << bit;
    std::array<int, 255> powerOf{};
    std::array<int, 256> logOf{};
    int value = 1;
    for (int exponent = 0; exponent < 255; ++exponent) {
        powerOf[exponent] = value;
        logOf[value] = exponent;
        value = (value & 0x80) ? (((value << 1) & 0xFF) ^ alpha8) : (value << 1);
    }
    auto times = [&](int a, int b) {
        return (a == 0 || b == 0) ? 0 : powerOf[(logOf[a] + logOf[b]) % 255];
    };

    std::array<int, 256> poly{};
    poly[0] = 1;
    for (int i = 1; i <= k; ++i) {
        for (int j = i; j > 0; --j)
            poly[j] = poly[j - 1] ^ times(poly[j], powerOf[i]);
        poly[0] = times(poly[0], powerOf[i]);
    }
    std::array<unsigned char, 255> block{};
    for (int j = 0; j <= k; ++j)
        block[j] = static_cast<unsigned char>(poly[j]);
    return block;
}

}  // namespace

// Berlekamp-Massey raises the error-locator degree to k+1 at step k+1 when
// syndromes 1..k vanish. For k == 2*tt-1 that locator has degree 2*tt, one
// more term than an elp row holds, and decode() wrote past the row (heap
// overflow, seen under AddressSanitizer). Any degree above tt must be
// rejected instead; degrees up to tt are ordinary correctable error patterns.
TEST_CASE("RScodec: error locators above tt are rejected, not built",
          "[dwg][rscodec][safety]") {
    struct Code { unsigned int pp; int tt; };
    // (255,239) system pages and (255,251) R2007 data pages.
    for (const Code code : {Code{0x96, 8}, Code{0xB8, 2}}) {
        RScodec codec(code.pp, 8, code.tt);
        REQUIRE(codec.isOkey());
        for (int k = 0; k < 2 * code.tt; ++k) {
            CAPTURE(code.pp, code.tt, k);
            const auto received = rsSyndromeVanishingBlock(code.pp, k);
            auto block = received;
            const int corrected = codec.decode(block.data());
            if (k < code.tt) {
                // k+1 symbols away from the all-zero codeword.
                CHECK(corrected == k + 1);
                CHECK(block == std::array<unsigned char, 255>{});
            } else {
                CHECK(corrected == -1);
                CHECK(block == received);
            }
        }
    }
}
