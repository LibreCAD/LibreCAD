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
std::vector<duint8> snapshot(const dwgBufferW& w) {
    return w.data();
}

} // namespace

TEST_CASE("dwgBufferW: putBit round-trips", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Pack 8 bits: 1,0,1,1,0,0,1,0 — checks MSB-first ordering against
    // the reader (which reads MSB-first).
    const duint8 expected[8] = {1, 0, 1, 1, 0, 0, 1, 0};
    for (duint8 b : expected) w.putBit(b);
    REQUIRE(w.size() == 1);
    REQUIRE(w.bitPos() == 0); // exactly one byte, fully filled
    REQUIRE(w.data()[0] == 0b10110010);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (duint8 b : expected) {
        REQUIRE(r.getBit() == b);
    }
}

TEST_CASE("dwgBufferW: putBit crosses byte boundary", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Write 17 alternating bits — forces transition across 2 byte
    // boundaries and leaves an in-progress third byte (bitPos == 1).
    for (int i = 0; i < 17; ++i) w.putBit(static_cast<duint8>(i & 1));
    REQUIRE(w.size() == 3);
    REQUIRE(w.bitPos() == 1);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (int i = 0; i < 17; ++i)
        REQUIRE(r.getBit() == static_cast<duint8>(i & 1));
}

TEST_CASE("dwgBufferW: put2Bits and put3Bits", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Cover all 4 / 8 values plus boundary crossings.
    for (duint8 v : {0u, 1u, 2u, 3u, 0u, 3u, 1u, 2u}) w.put2Bits(static_cast<duint8>(v));
    // 3-bit fields — exercise within-byte writes plus the 6->1 and 7->2
    // boundary crossings.
    const std::vector<duint8> threeBitVals = {0, 7, 5, 2, 6, 3};
    for (duint8 v : threeBitVals) w.put3Bits(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (duint8 v : {0u, 1u, 2u, 3u, 0u, 3u, 1u, 2u}) REQUIRE(r.get2Bits() == v);
    for (duint8 v : threeBitVals) REQUIRE(r.get3Bits() == v);
}

TEST_CASE("dwgBufferW: putBitShort all four code paths", "[dwg-write][primitives]") {
    dwgBufferW w;
    // Code 10 (zero), code 11 (256), code 01 (1-byte), code 00 (2-byte).
    const std::vector<duint16> values = {0, 256, 1, 42, 255, 257, 32767, 65535};
    for (auto v : values) w.putBitShort(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitShort() == v);
}

TEST_CASE("dwgBufferW: putBitLong all three code paths", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<dint32> values = {0, 1, 127, 255, 256, 65535, 1 << 20, -1, -65536};
    for (auto v : values) w.putBitLong(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitLong() == v);
}

TEST_CASE("dwgBufferW: putBitLongLong uses compact little-endian payloads",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<duint64> values = {
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

TEST_CASE("dwgBufferW: putBitDouble special and arbitrary values", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<double> values = {0.0, 1.0, -1.0, 3.14159265358979, 1e-300, 1e300};
    for (auto v : values) w.putBitDouble(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getBitDouble() == v);
}

TEST_CASE("dwgBufferW: putRawChar8 byte-aligned and bit-shifted", "[dwg-write][primitives]") {
    // Aligned: every byte trivially round-trips.
    {
        dwgBufferW w;
        const std::vector<duint8> values = {0, 1, 0x7F, 0x80, 0xFF, 0xAA, 0x55};
        for (auto v : values) w.putRawChar8(v);
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        for (auto v : values) REQUIRE(r.getRawChar8() == v);
    }
    // Shifted: a leading bit forces every RC to span a byte boundary.
    {
        dwgBufferW w;
        w.putBit(1);
        const std::vector<duint8> values = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF};
        for (auto v : values) w.putRawChar8(v);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        REQUIRE(r.getBit() == 1);
        for (auto v : values) REQUIRE(r.getRawChar8() == v);
    }
}

TEST_CASE("dwgBufferW: putRawShort16 little-endian", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<duint16> values = {0x0000, 0x0001, 0x00FF, 0x0100, 0xABCD, 0xFFFF};
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
}

TEST_CASE("dwgBufferW: putRawLong32 little-endian", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<duint32> values = {0u, 1u, 0xFFu, 0x100u, 0xDEADBEEFu, 0xFFFFFFFFu};
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
    // Boundary values: chunk size changes at 2^7, 2^14, 2^21, 2^28.
    const std::vector<duint32> values = {
        0, 1, 0x7F, 0x80, 0x3FFF, 0x4000, 0x1FFFFF, 0x200000, 0x0FFFFFFFu
    };
    for (auto v : values) w.putUModularChar(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getUModularChar() == v);
}

TEST_CASE("dwgBufferW: putModularChar signed boundaries", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<dint32> values = {
        0, 1, -1, 63, -63, 64, -64, 8191, -8191, 8192, -8192,
        1048575, -1048575, 1048576, -1048576
    };
    for (auto v : values) w.putModularChar(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) {
        dint32 got = r.getModularChar();
        REQUIRE(got == v);
    }
}

TEST_CASE("dwgBufferW: putModularShort", "[dwg-write][primitives]") {
    dwgBufferW w;
    const std::vector<dint32> values = {0, 1, 0x7FFF, 0x8000, 0x10000, 0x3FFFFFFF};
    for (auto v : values) w.putModularShort(v);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    for (auto v : values) REQUIRE(r.getModularShort() == v);
}

TEST_CASE("dwgBufferW: putHandle round-trip", "[dwg-write][primitives]") {
    dwgBufferW w;
    struct Sample { duint8 code; duint32 ref; };
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

TEST_CASE("dwgBufferW: putBytes large block, aligned and shifted", "[dwg-write][primitives]") {
    std::vector<duint8> payload(257);
    for (size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<duint8>(i & 0xFF);

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
        std::vector<duint8> recovered(payload.size());
        r.getBytes(recovered.data(), recovered.size());
        REQUIRE(std::memcmp(recovered.data(), payload.data(), payload.size()) == 0);
    }
}

TEST_CASE("dwgBufferW: crc16 matches reader's CRC over a known buffer",
          "[dwg-write][primitives]") {
    // Build a 32-byte run via the writer, then compute crc16(0xC0C1) using
    // both dwgBuffer and dwgBufferW.  They must agree byte-for-byte.
    dwgBufferW w;
    for (duint8 i = 0; i < 32; ++i) w.putRawChar8(i);

    duint16 writerCrc = w.crc16(0xC0C1, 0, w.size());

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    duint16 readerCrc = r.crc8(0xC0C1, 0, static_cast<dint32>(bytes.size()));

    REQUIRE(writerCrc == readerCrc);
}

// 1.1 (gap classes-crc-not-validated): crc8/crc32 computed `new[end-start]`
// with no guard, so a corrupt section size with end<=start allocated a
// negative (huge) size — heap overflow / bad_alloc.  Now they return the
// seed identity on an empty/negative range without allocating.
TEST_CASE("dwgBuffer::crc8/crc32 guard the empty/negative byte range",
          "[dwg-write][primitives]") {
    dwgBufferW w;
    for (duint8 i = 0; i < 8; ++i) w.putRawChar8(i);
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());

    // end < start (negative range): no allocation/abort; seed returned.
    REQUIRE(r.crc8(0xC0C1, 4, 0) == 0xC0C1);
    // end == start (empty range): seed returned.
    REQUIRE(r.crc8(0xC0C1, 4, 4) == 0xC0C1);
    REQUIRE(r.crc32(0xFFFFFFFFu, 8, 0) == 0xFFFFFFFFu);
    REQUIRE(r.crc32(0x12345678u, 4, 4) == 0x12345678u);

    // Valid range still produces a real CRC (happy path unaffected).
    dwgBufferW w2;
    for (duint8 i = 0; i < 4; ++i) w2.putRawChar8(i);
    auto b2 = snapshot(w2);
    dwgBuffer r2(b2.data(), b2.size());
    REQUIRE(r2.crc8(0xC0C1, 0, 4) == w2.crc16(0xC0C1, 0, 4));
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
