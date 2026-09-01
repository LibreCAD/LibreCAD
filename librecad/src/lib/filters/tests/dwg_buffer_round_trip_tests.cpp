/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

/**
 * Round-trip unit tests for the Phase 1 DWG writer primitives.
 *
 * For each primitive in dwgBufferW, encode a value, then decode it
 * with the existing dwgBuffer reader and assert equality.  Covers
 * the bit-packing boundary cases (byte-aligned, mid-byte, and
 * spanning byte boundaries) that are the most common source of
 * off-by-one bugs in the bit-level encoder.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "drw_base.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgutil.h"
#include "intern/dwgwriter.h"

namespace {

/// Construct a reader over the bytes accumulated by the writer.
/// dwgBuffer requires a non-const pointer + length; we widen to a
/// scratch copy so the writer's accumulator stays untouched.
std::vector<std::uint8_t> snapshot(const dwgBufferW& w) {
    return w.data();
}

} // namespace

TEST_CASE("dwgBufferW: putBit round-trips", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Pack 8 bits: 1,0,1,1,0,0,1,0 — checks MSB-first ordering against
    // the reader (which reads MSB-first).
    const std::uint8_t expected[8] = {1, 0, 1, 1, 0, 0, 1, 0};
    for (std::uint8_t b : expected) w.putBit(b);
    REQUIRE(w.size() == 1);
    REQUIRE(w.bitPos() == 0); // exactly one byte, fully filled
    REQUIRE(w.data()[0] == 0b10110010);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (std::uint8_t b : expected) {
        REQUIRE(r.getBit() == b);
    }
}

TEST_CASE("dwgBuffer rejects truncated bit and default-double reads",
          "[dwg-read][buffer][safety]") {
    std::uint8_t byte = 0;
    dwgBuffer empty(&byte, 0);
    CHECK(empty.getBit() == 0);
    CHECK_FALSE(empty.isGood());

    dwgBuffer emptyPair(&byte, 0);
    CHECK(emptyPair.get2Bits() == 0);
    CHECK_FALSE(emptyPair.isGood());

    // 01 selects the four-byte default-double payload, which is intentionally
    // truncated here. The reader must report failure without using bytes it
    // did not receive.
    std::uint8_t truncatedDefault[] = {0x40};
    dwgBuffer defaultReader(truncatedDefault, sizeof(truncatedDefault));
    CHECK(defaultReader.getDefaultDouble(42.0) == 0.0);
    CHECK_FALSE(defaultReader.isGood());

    dwgBuffer nullReader(nullptr, 1);
    CHECK(nullReader.getRawChar8() == 0);
    CHECK_FALSE(nullReader.isGood());

    dwgBuffer nullCrc(nullptr, 1);
    CHECK(nullCrc.crc8(0, 0, 1) == 0);
    CHECK_FALSE(nullCrc.isGood());
}

TEST_CASE("file-backed dwgBuffer copies keep independent cursors",
          "[dwg-read][buffer][safety]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw-file-stream-clone.dwg";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output.good());
        const std::uint8_t bytes[] = {0x11, 0x22, 0x33};
        output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
        REQUIRE(output.good());
    }

    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.good());
    dwgBuffer source(&input);
    REQUIRE(source.getRawChar8() == 0x11u);

    dwgBuffer copy = source;
    CHECK(copy.getRawChar8() == 0x22u);
    CHECK(source.getRawChar8() == 0x22u);
    CHECK(copy.getRawChar8() == 0x33u);
    CHECK(source.getRawChar8() == 0x33u);

    dwgBuffer failed = source.forkIndependent();
    CHECK_FALSE(failed.setPosition(4));
    CHECK_FALSE(failed.isGood());
    CHECK(source.isGood());

    std::error_code error;
    std::filesystem::remove(path, error);
    CHECK_FALSE(error);
}

TEST_CASE("dwgBufferW: putBit crosses byte boundary", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Write 17 alternating bits — forces transition across 2 byte
    // boundaries and leaves an in-progress third byte (bitPos == 1).
    for (int i = 0; i < 17; ++i) w.putBit(static_cast<std::uint8_t>(i & 1));
    REQUIRE(w.size() == 3);
    REQUIRE(w.bitPos() == 1);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (int i = 0; i < 17; ++i)
        REQUIRE(r.getBit() == static_cast<std::uint8_t>(i & 1));
}

TEST_CASE("dwgBufferW: put2Bits and put3Bits", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Cover all 4 / 8 values plus boundary crossings.
    for (std::uint8_t v : {0u, 1u, 2u, 3u, 0u, 3u, 1u, 2u}) w.put2Bits(static_cast<std::uint8_t>(v));
    // 3-bit fields — exercise within-byte writes plus the 6->1 and 7->2
    // boundary crossings.
    const std::vector<std::uint8_t> threeBitVals = {0, 7, 5, 2, 6, 3};
    for (std::uint8_t v : threeBitVals) w.put3Bits(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (std::uint8_t v : {0u, 1u, 2u, 3u, 0u, 3u, 1u, 2u}) REQUIRE(r.get2Bits() == v);
    for (std::uint8_t v : threeBitVals) REQUIRE(r.get3Bits() == v);
}

TEST_CASE("dwgBufferW: putBitShort all four code paths", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Code 10 (zero), code 11 (256), code 01 (1-byte), code 00 (2-byte).
    const std::vector<std::uint16_t> values = {0, 256, 1, 42, 255, 257, 32767, 65535};
    for (auto v : values) w.putBitShort(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitShort() == v);
}

TEST_CASE("dwgBufferW: putBitLong all three code paths", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::int32_t> values = {0, 1, 127, 255, 256, 65535, 1 << 20, -1, -65536};
    for (auto v : values) w.putBitLong(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitLong() == v);
}

TEST_CASE("dwgBufferW: putBitLongLong uses compact little-endian payloads",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::uint64_t> values = {
        0x0ULL, 0x12ULL, 0x1234ULL, 0x123456ULL, 0x12345678ULL,
        0x01020304050607ULL
    };
    for (auto v : values)
        w.putBitLongLong(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values)
        REQUIRE(r.getBitLongLong() == v);
}

TEST_CASE("dwgBufferW: bounded integer and handle fields reject overflow",
          "[dwg-write][primitives][safety]") {
    dwgBufferW bll;
    bll.putBitLongLong(std::uint64_t{1} << 56);
    CHECK_FALSE(bll.isGood());
    CHECK(bll.data().empty());

    dwgBufferW fixedHandle;
    fixedHandle.putFixedHandle(4, 9, 1);
    CHECK_FALSE(fixedHandle.isGood());
    CHECK(fixedHandle.data().empty());

    dwgBufferW handle;
    dwgHandle invalid;
    invalid.size = 9;
    handle.putHandle(invalid);
    CHECK_FALSE(handle.isGood());
    CHECK(handle.data().empty());
}

TEST_CASE("dwgBufferW: putBitDouble special and arbitrary values", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<double> values = {0.0, 1.0, -1.0, 3.14159265358979, 1e-300, 1e300};
    for (auto v : values) w.putBitDouble(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitDouble() == v);
}

TEST_CASE("dwgBufferW: raw doubles followed by zero bit-doubles", "[dwg-write][primitives]") {
    dwgBufferW w;
    for (int i = 0; i < 4; ++i)
        w.putBit(0);
    w.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
    w.put3BitDouble(DRW_Coord{1.0e10, 1.0e10, 0.0});
    w.put3BitDouble(DRW_Coord{-1.0e10, -1.0e10, 0.0});
    w.put2RawDouble(DRW_Coord{210.0, 297.0, 0.0});
    w.put2RawDouble(DRW_Coord{0.0, 0.0, 0.0});
    w.putBitDouble(0.0);
    w.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (int i = 0; i < 4; ++i)
        CHECK(r.getBit() == 0);
    const DRW_Coord insBase = r.get3BitDouble();
    CHECK(insBase.x == 0.0);
    CHECK(insBase.y == 0.0);
    CHECK(insBase.z == 0.0);
    const DRW_Coord extMin = r.get3BitDouble();
    CHECK(extMin.x == 1.0e10);
    CHECK(extMin.y == 1.0e10);
    CHECK(extMin.z == 0.0);
    const DRW_Coord extMax = r.get3BitDouble();
    CHECK(extMax.x == -1.0e10);
    CHECK(extMax.y == -1.0e10);
    CHECK(extMax.z == 0.0);
    const DRW_Coord limits = r.get2RawDouble();
    CHECK(limits.x == 210.0);
    CHECK(limits.y == 297.0);
    const DRW_Coord zerosBefore = r.get2RawDouble();
    CHECK(zerosBefore.x == 0.0);
    CHECK(zerosBefore.y == 0.0);
    CHECK(r.getBitDouble() == 0.0);
    const DRW_Coord zeros = r.get3BitDouble();
    CHECK(zeros.x == 0.0);
    CHECK(zeros.y == 0.0);
    CHECK(zeros.z == 0.0);
}

TEST_CASE("dwgBufferW: putRawChar8 byte-aligned and bit-shifted", "[dwg-write][primitives]") {
    // Aligned: every byte trivially round-trips.
    {
        dwgBufferW w;
        const std::vector<std::uint8_t> values = {0, 1, 0x7F, 0x80, 0xFF, 0xAA, 0x55};
        for (auto v : values) w.putRawChar8(v);
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        for (auto v : values) REQUIRE(r.getRawChar8() == v);
    }
    // Shifted: a leading bit forces every RC to span a byte boundary.
    {
        dwgBufferW w;
        w.putBit(1);
        const std::vector<std::uint8_t> values = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF};
        for (auto v : values) w.putRawChar8(v);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        REQUIRE(r.getBit() == 1);
        for (auto v : values) REQUIRE(r.getRawChar8() == v);
    }
}

TEST_CASE("dwgBufferW: putRawShort16 little-endian", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::uint16_t> values = {0x0000, 0x0001, 0x00FF, 0x0100, 0xABCD, 0xFFFF};
    for (auto v : values) w.putRawShort16(v);
    // Spot-check little-endian byte order on the first value.
    auto bytes = snapshot(w);
    REQUIRE(bytes[0] == 0x00);
    REQUIRE(bytes[1] == 0x00);
    REQUIRE(bytes[2] == 0x01); // low byte of 0x0001
    REQUIRE(bytes[3] == 0x00); // high byte of 0x0001

    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getRawShort16() == v);
}

TEST_CASE("dwgBufferW: putBERawShort16 big-endian", "[dwg-write][primitives]") {
    dwgBufferW w;
    w.putBERawShort16(0xABCD);
    auto bytes = snapshot(w);
    REQUIRE(bytes[0] == 0xAB); // high byte first
    REQUIRE(bytes[1] == 0xCD);

    dwgBuffer r(bytes.data(), bytes.size());
    REQUIRE(r.getBERawShort16() == 0xABCD);

    // 1.6 UB fix: a high byte with the sign bit set (0xFF) must not invoke
    // signed-char left-shift UB and must still decode big-endian.
    dwgBufferW w2;
    w2.putBERawShort16(0xFF80);
    auto b2 = snapshot(w2);
    REQUIRE(b2[0] == 0xFF);
    REQUIRE(b2[1] == 0x80);
    dwgBuffer r2(b2.data(), b2.size());
    REQUIRE(r2.getBERawShort16() == 0xFF80);
}

TEST_CASE("dwgBufferW: putRawLong32 little-endian", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::uint32_t> values = {0u, 1u, 0xFFu, 0x100u, 0xDEADBEEFu, 0xFFFFFFFFu};
    for (auto v : values) w.putRawLong32(v);

    auto bytes = snapshot(w);
    // Spot-check: 0xDEADBEEF stored LE
    size_t off = 4 + 4 + 4 + 4; // skip 0, 1, 0xFF, 0x100
    REQUIRE(bytes[off + 0] == 0xEF);
    REQUIRE(bytes[off + 1] == 0xBE);
    REQUIRE(bytes[off + 2] == 0xAD);
    REQUIRE(bytes[off + 3] == 0xDE);

    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getRawLong32() == v);
}

TEST_CASE("dwgBufferW: putRawDouble matches IEEE 754", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<double> values = {0.0, 1.0, -1.0, 1e-9, 1.234e200};
    for (auto v : values) w.putRawDouble(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) {
        double got = r.getRawDouble();
        REQUIRE(std::memcmp(&got, &v, 8) == 0);
    }
}

TEST_CASE("dwgBufferW: putUModularChar boundary widths", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Boundary values include the five-byte DWG form used by large deltas.
    const std::vector<std::uint64_t> values = {
        0, 1, 0x7F, 0x80, 0x3FFF, 0x4000, 0x1FFFFF, 0x200000,
        0x0FFFFFFFu, 0x10000000u, 0x7FFFFFFFFULL
    };
    for (auto v : values) w.putUModularChar(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getUModularChar() == v);
}

TEST_CASE("dwgBufferW: putModularChar signed boundaries", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::int64_t> values = {
        0, 1, -1, 63, -63, 64, -64, 8191, -8191, 8192, -8192,
        1048575, -1048575, 1048576, -1048576,
        0x20000000LL, -0x20000000LL, 0x3FFFFFFFFLL, -0x3FFFFFFFFLL
    };
    for (auto v : values) w.putModularChar(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) {
        std::int64_t got = r.getModularChar();
        REQUIRE(got == v);
    }
}

TEST_CASE("dwgBufferW: modular chars reject values outside the wire range",
          "[dwg-write][primitives][safety]") {
    dwgBufferW unsignedBuffer;
    CHECK_FALSE(unsignedBuffer.putUModularChar(std::uint64_t{1} << 35));
    CHECK_FALSE(unsignedBuffer.isGood());
    CHECK(unsignedBuffer.data().empty());

    dwgBufferW signedBuffer;
    const std::int64_t tooLarge = static_cast<std::int64_t>(std::uint64_t{1} << 34);
    CHECK_FALSE(signedBuffer.putModularChar(tooLarge));
    CHECK_FALSE(signedBuffer.isGood());
    CHECK_FALSE(signedBuffer.putModularChar(-tooLarge));
    CHECK(signedBuffer.data().empty());
}

TEST_CASE("dwgBufferW: modular shorts reject nonrepresentable lengths",
          "[dwg-write][primitives][safety]") {
    dwgBufferW tooLarge;
    tooLarge.putModularShort(1 << 30);
    CHECK_FALSE(tooLarge.isGood());
    CHECK(tooLarge.data().empty());

    dwgBufferW negative;
    negative.putModularShort(-1);
    CHECK_FALSE(negative.isGood());
    CHECK(negative.data().empty());

    dwgBufferW maximum;
    maximum.putModularShort((1 << 30) - 1);
    CHECK(maximum.isGood());
    auto bytes = snapshot(maximum);
    dwgBuffer decoded(bytes.data(), bytes.size());
    CHECK(decoded.getModularShort() == (1 << 30) - 1);
}

TEST_CASE("dwgBufferW: text primitives reject length overflow",
          "[dwg-write][primitives][safety]") {
    const std::string tooLong(0xFFFFu, 'x');

    dwgBufferW cp8;
    cp8.putCP8Text(tooLong);
    CHECK_FALSE(cp8.isGood());
    CHECK(cp8.data().empty());

    dwgBufferW enc;
    enc.putENCText(tooLong);
    CHECK_FALSE(enc.isGood());
    CHECK(enc.data().empty());

    dwgBufferW ucs;
    ucs.putUCSText(tooLong);
    CHECK_FALSE(ucs.isGood());
    CHECK(ucs.data().empty());
}

TEST_CASE("dwgBuffer: modular chars reject missing five-byte terminators",
          "[dwg][safety][primitives]") {
    std::uint8_t continuation[] = {0x81, 0x82, 0x83, 0x84, 0x85};
    dwgBuffer unsignedBuffer(continuation, sizeof(continuation));
    (void)unsignedBuffer.getUModularChar();
    CHECK_FALSE(unsignedBuffer.isGood());

    dwgBuffer signedBuffer(continuation, sizeof(continuation));
    (void)signedBuffer.getModularChar();
    CHECK_FALSE(signedBuffer.isGood());
}

TEST_CASE("dwgBufferW: putModularShort", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<std::int32_t> values = {0, 1, 0x7FFF, 0x8000, 0x10000, 0x3FFFFFFF};
    for (auto v : values) w.putModularShort(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getModularShort() == v);
}

TEST_CASE("dwgBuffer: modular shorts reject overlong continuation",
          "[dwg][safety][primitives]") {
    const std::uint8_t overlong[] = {
        0x00, 0x80, // first 15-bit word continues
        0x00, 0x80, // second word incorrectly continues
        0x00, 0x00  // bytes that must remain unread
    };
    dwgBuffer buffer(const_cast<std::uint8_t*>(overlong), sizeof(overlong));
    CHECK(buffer.getModularShort() == 0);
    CHECK_FALSE(buffer.isGood());
    CHECK(buffer.getPosition() == 4);
}

TEST_CASE("dwgBufferW: putHandle round-trip", "[dwg-write][primitives]") {
    dwgBufferW w;
    struct Sample { std::uint8_t code; std::uint32_t ref; };
    const std::vector<Sample> samples = {
        {0, 0},       // null handle, size 0
        {1, 0x12},    // 1 byte
        {2, 0x1234},  // 2 bytes
        {3, 0x123456}, // 3 bytes
        {4, 0x12345678}, // 4 bytes
        {5, 0x000000FF}, // 1 byte (top byte zero)
    };
    for (auto& s : samples) {
        dwgHandle h;
        h.code = s.code;
        h.ref  = s.ref;
        w.putHandle(h);
    }

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto& s : samples) {
        dwgHandle h = r.getHandle();
        REQUIRE(h.code == s.code);
        REQUIRE(h.ref  == s.ref);
    }
}

TEST_CASE("dwgBufferW: putExtrusion shortcut and explicit", "[dwg-write][primitives]") {
    dwgBufferW w;
    DRW_Coord shortcut(0.0, 0.0, 1.0);
    DRW_Coord explicit_(0.1, 0.2, 0.97); // not the shortcut
    w.putExtrusion(shortcut, true);
    w.putExtrusion(explicit_, true);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Coord got1 = r.getExtrusion(true);
    REQUIRE(got1.x == 0.0);
    REQUIRE(got1.y == 0.0);
    REQUIRE(got1.z == 1.0);
    DRW_Coord got2 = r.getExtrusion(true);
    REQUIRE(got2.x == 0.1);
    REQUIRE(got2.y == 0.2);
    REQUIRE(got2.z == 0.97);
}

TEST_CASE("dwgBufferW: putThickness shortcut and explicit", "[dwg-write][primitives]") {
    dwgBufferW w;
    w.putThickness(0.0, true);
    w.putThickness(2.5, true);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    REQUIRE(r.getThickness(true) == 0.0);
    REQUIRE(r.getThickness(true) == 2.5);
}

TEST_CASE("dwgBufferW: putVariableText ASCII round-trip (no codec)", "[dwg-write][primitives]") {
    // No decoder: bytes are written and read verbatim.
    dwgBufferW w;
    w.putVariableText(DRW::AC1015, "");
    w.putVariableText(DRW::AC1015, "Hello");
    w.putVariableText(DRW::AC1015, "LAYER0");

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    // R2000 path: getVariableText routes to getCP8Text → get8bitStr.
    // With no decoder, bytes pass through unchanged.
    REQUIRE(r.getVariableText(DRW::AC1015, false) == "");
    REQUIRE(r.getVariableText(DRW::AC1015, false) == "Hello");
    REQUIRE(r.getVariableText(DRW::AC1015, false) == "LAYER0");
}

TEST_CASE("dwgBufferW: putVariableText AC1021 UTF-16 code-unit round-trip",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    w.putVariableText(DRW::AC1021, "");
    w.putVariableText(DRW::AC1021, "Hello");
    w.putVariableText(DRW::AC1021, "Table");

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1021, false);
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size(), &codec);
    REQUIRE(r.getVariableText(DRW::AC1021, false) == "");
    REQUIRE(r.getVariableText(DRW::AC1021, false) == "Hello");
    REQUIRE(r.getVariableText(DRW::AC1021, false) == "Table");
}

TEST_CASE("dwgBufferW: ENC orders RGB before alpha and converts CP8 names",
          "[dwg-write][primitives][enc]") {
    {
        dwgBufferW w;
        w.putEnColor(DRW::AC1027, 7, 0x112233, {}, {}, 0x03000080, false);
        auto bytes = snapshot(w);
        dwgBuffer flags(bytes.data(), bytes.size());
        CHECK((flags.getBitShort() & 0xFF00u) == 0xA000u);
        dwgBuffer r(bytes.data(), bytes.size());
        CHECK(r.getEnColor(DRW::AC1027) == 7);
        CHECK(r.lastEnColorRgb == 0x112233);
        CHECK(r.lastEnColorAlphaRaw == 0x03000080);
    }
    {
        dwgBufferW w;
        w.putEnColor(DRW::AC1027, 256, -1, "Entry", "Book", 0, true);
        auto bytes = snapshot(w);
        dwgBuffer flags(bytes.data(), bytes.size());
        // ACadSharp emits both the DBCOLOR (0x40) and complex-color (0x80)
        // markers for a book-color reference.  A semantic self-read would
        // accept the incomplete 0x40 form, so assert the raw wire flags.
        CHECK((flags.getBitShort() & 0xFF00u) == 0xC300u);
        dwgBuffer r(bytes.data(), bytes.size());
        CHECK(r.getEnColor(DRW::AC1027) == 256);
        CHECK(r.lastEnColorHadDbColorRef);
        CHECK(r.lastEnColorName == "Entry");
        CHECK(r.lastEnColorBookName == "Book");
    }
    {
        dwgBufferW w;
        w.putEnColor(DRW::AC1027, 256, -1, {}, {}, 0, true);
        auto bytes = snapshot(w);
        dwgBuffer flags(bytes.data(), bytes.size());
        // A nameless DBCOLOR reference cannot carry ACI 256 in the shared
        // low flag bit; ACadSharp emits the reference form with index zero.
        CHECK((flags.getBitShort() & 0xFF00u) == 0xC000u);
        dwgBuffer r(bytes.data(), bytes.size());
        CHECK(r.getEnColor(DRW::AC1027) == 0);
        CHECK(r.lastEnColorHadDbColorRef);
    }
}

TEST_CASE("dwgBufferW: ENC names use a secondary codepage on AC1027",
          "[dwg-write][primitives][enc][i18n]") {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    codec.setByteCodePage("ANSI_932");

    dwgBufferW w(&codec);
    // Name flags share the low index byte's bit 8; the canonical named
    // DBCOLOR form therefore carries the ByLayer index representation.
    w.putEnColor(DRW::AC1027, 256, -1, "Ａ", "Ａ", 0, true);
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size(), &codec);
    REQUIRE(r.getEnColor(DRW::AC1027) == 256);
    REQUIRE(r.lastEnColorName == "Ａ");
    REQUIRE(r.lastEnColorBookName == "Ａ");
}

TEST_CASE("dwgBufferW: putBytes large block, aligned and shifted", "[dwg-write][primitives]") {
    std::vector<std::uint8_t> payload(257);
    for (size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<std::uint8_t>(i & 0xFF);

    // Aligned case.
    {
        dwgBufferW w;
        w.putBytes(payload.data(), payload.size());
        REQUIRE(w.size() == payload.size());
        REQUIRE(std::memcmp(w.data().data(), payload.data(), payload.size()) == 0);
    }
    // Shifted case — one bit ahead.
    {
        dwgBufferW w;
        w.putBit(1);
        w.putBytes(payload.data(), payload.size());
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        REQUIRE(r.getBit() == 1);
        std::vector<std::uint8_t> recovered(payload.size());
        r.getBytes(recovered.data(), recovered.size());
        REQUIRE(std::memcmp(recovered.data(), payload.data(), payload.size()) == 0);
    }
}

TEST_CASE("dwgBufferW: crc16 matches reader's CRC over a known buffer",
          "[dwg-write][primitives]") {
    // Build a 32-byte run via the writer, then compute crc16(0xC0C1) using
    // both dwgBuffer and dwgBufferW.  They must agree byte-for-byte.
    dwgBufferW w;
    for (std::uint8_t i = 0; i < 32; ++i) w.putRawChar8(i);

    std::uint16_t writerCrc = w.crc16(0xC0C1, 0, w.size());

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    std::uint16_t readerCrc = r.crc8(0xC0C1, 0, static_cast<std::int32_t>(bytes.size()));

    REQUIRE(writerCrc == readerCrc);
}

// 1.1 (gap classes-crc-not-validated): crc8/crc32 computed `new[end-start]`
// with no guard, so a corrupt section size with end<=start allocated a
// negative (huge) size — heap overflow / bad_alloc.  Now they return the
// seed identity on an empty/negative range without allocating.
TEST_CASE("dwgBuffer::crc8/crc32 guard the empty/negative byte range",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    for (std::uint8_t i = 0; i < 8; ++i) w.putRawChar8(i);
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());

    // end < start (negative range): no allocation/abort; seed returned.
    REQUIRE(r.crc8(0xC0C1, 4, 0) == 0xC0C1);
    // end == start (empty range): seed returned.
    REQUIRE(r.crc8(0xC0C1, 4, 4) == 0xC0C1);
    // A negative endpoint is also an invalid range and must not reach the
    // stream seek or signed subtraction.
    REQUIRE(r.crc8(0xC0C1, -1, 4) == 0xC0C1);
    REQUIRE(r.crc32(0xFFFFFFFFu, 8, 0) == 0xFFFFFFFFu);
    REQUIRE(r.crc32(0x12345678u, 4, 4) == 0x12345678u);
    REQUIRE(r.crc32(0x12345678u, -1, 4) == 0x12345678u);

    // Valid range still produces a real CRC (happy path unaffected).
    dwgBufferW w2;
    for (std::uint8_t i = 0; i < 4; ++i) w2.putRawChar8(i);
    auto b2 = snapshot(w2);
    dwgBuffer r2(b2.data(), b2.size());
    REQUIRE(r2.crc8(0xC0C1, 0, 4) == w2.crc16(0xC0C1, 0, 4));
}

// 1.2 (gap classes-crc-not-validated): decompress18 peeked
// compressedBuffer[compressedSize-2] before any length check, so a <2-byte
// compressed page read out of bounds (compressedSize is std::uint32_t; 0-2
// underflows to a huge index).  Now it fails fast.
TEST_CASE("dwgCompressor::decompress18 rejects compressedSize<2 without OOB",
          "[dwg-write][primitives]") {
    std::uint8_t cbuf[4] = {0, 0, 0, 0};
    std::uint8_t dbuf[16] = {0};
    dwgCompressor comp;

    // csize == 1 and 0 must fail without dereferencing cbuf[csize-2].
    REQUIRE_FALSE(comp.decompress18(cbuf, dbuf, /*csize=*/1, /*dsize=*/16));
    REQUIRE_FALSE(comp.decompress18(cbuf, dbuf, /*csize=*/0, /*dsize=*/16));
    // null compressed buffer also rejected.
    REQUIRE_FALSE(comp.decompress18(nullptr, dbuf, /*csize=*/8, /*dsize=*/16));
}

TEST_CASE("dwgBufferW: patchRawShort16 / patchRawLong32 overwrite cleanly",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    w.putRawShort16(0);   // placeholder offset 0..1
    w.putRawLong32(0);    // placeholder offset 2..5
    w.putRawChar8(0xAA);  // trailing byte to ensure patch doesn't bleed

    w.patchRawShort16(0, 0xABCD);
    w.patchRawLong32(2, 0xDEADBEEFu);

    auto bytes = snapshot(w);
    REQUIRE(bytes[0] == 0xCD);
    REQUIRE(bytes[1] == 0xAB);
    REQUIRE(bytes[2] == 0xEF);
    REQUIRE(bytes[3] == 0xBE);
    REQUIRE(bytes[4] == 0xAD);
    REQUIRE(bytes[5] == 0xDE);
    REQUIRE(bytes[6] == 0xAA);
}

TEST_CASE("dwgUtil: section sentinels and version strings", "[dwg-write][primitives]") {
    // Spot-check that pairs are byte-inverses of each other — guards
    // against accidental edits to the byte arrays.
    for (int i = 0; i < 16; ++i) {
        REQUIRE((dwgSentinels::HEADER_BEGIN[i] ^ dwgSentinels::HEADER_END[i]) == 0xFF);
        REQUIRE((dwgSentinels::CLASSES_BEGIN[i] ^ dwgSentinels::CLASSES_END[i]) == 0xFF);
        REQUIRE((dwgSentinels::PREVIEW_BEGIN[i] ^ dwgSentinels::PREVIEW_END[i]) == 0xFF);
        REQUIRE((dwgSentinels::SECOND_HEADER_BEGIN[i] ^ dwgSentinels::SECOND_HEADER_END[i]) == 0xFF);
    }
    // Version strings are exactly 6 bytes, the expected literal text.
    REQUIRE(std::memcmp(dwgVersionString::R2000, "AC1015", 6) == 0);
    REQUIRE(std::memcmp(dwgVersionString::R2018, "AC1032", 6) == 0);
}

TEST_CASE("HandleAllocator: reserved seeding + sequential allocation",
          "[dwg-write][primitives]") {
    HandleAllocator alloc;
    alloc.seedReserved();

    // First user allocation skips every reserved handle in 0x01–0x18
    // and lands on 0x30 (the convention) — all reserved handles are
    // below 0x30, so the while-loop in next() short-circuits.
    REQUIRE(alloc.next() == 0x30);
    REQUIRE(alloc.next() == 0x31);
    REQUIRE(alloc.next() == 0x32);

    // None of the canonical reserved handles can be reallocated.  Reserving
    // a high imported handle advances the high-water mark as well, so HANDSEED
    // remains above replayed source handles.
    alloc.reserve(0x35);
    REQUIRE(alloc.next() == 0x36);  // 0x35 is skipped
    REQUIRE(alloc.next() == 0x37);

    // current() reports the high-water mark (the next *candidate*,
    // which may be reserved — not the last-allocated value).
    REQUIRE(alloc.current() >= 0x38);
}

TEST_CASE("HandleAllocator: 0x04 gap is honored (not in reserved set)",
          "[dwg-write][primitives]") {
    // 0x04 is the documented gap between STYLE_CONTROL (0x03) and
    // LTYPE_CONTROL (0x05).  It's NOT in the reserved set — meaning
    // technically next() could land there if the cursor reached it,
    // but in practice the cursor starts at 0x30 so 0x04 stays free.
    // The test below documents this: if some caller manually
    // reserved 0x04, allocator would skip it; without the reserve,
    // 0x04 remains available.
    HandleAllocator alloc;
    alloc.seedReserved();

    // 0x04 isn't pre-reserved; verify by attempting reserve from a
    // fresh allocator and checking the first user allocation still
    // starts at 0x30 (the cursor convention dominates over gaps).
    REQUIRE(alloc.next() == 0x30);

    // Now reserve 0x04 explicitly and verify reserve() is idempotent.
    alloc.reserve(0x04);
    alloc.reserve(0x04);
    // Allocation continues from 0x31 regardless of the late reserve
    // because m_next has moved past it.
    REQUIRE(alloc.next() == 0x31);
}

TEST_CASE("HandleAllocator: exhaustion reservation is transactional",
          "[dwg-write][primitives][safety]") {
    HandleAllocator alloc;
    const auto maximum = std::numeric_limits<std::uint32_t>::max();

    REQUIRE_THROWS_AS(alloc.reserve(maximum), std::overflow_error);
    REQUIRE(alloc.current() == 0x30u);
    REQUIRE(alloc.next() == 0x30u);

    HandleAllocator nearMaximum;
    nearMaximum.reserve(maximum - 1u);
    REQUIRE_THROWS_AS(nearMaximum.next(), std::overflow_error);
    REQUIRE(nearMaximum.current() == maximum);
}
