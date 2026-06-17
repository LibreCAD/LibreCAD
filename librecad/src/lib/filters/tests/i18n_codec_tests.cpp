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

// Phase 1 i18n / UTF-8 codec regression tests.
// Targets:
//  - DRW_ConvUTF16::toUtf8 surrogate-pair combining (Phase 1.3)
//  - DRW_Converter::encodeMifText / \M+ pattern decode (Phase 1.4)
//  - DRW_ConvTable::toUtf8 \U+ detection across high-bit context (Phase 1.5)
//  - decodeEedString helper used by both EED sites (Phase 1.1, 1.2)

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include "intern/drw_textcodec.h"
#include "intern/dwgreader.h"
#include "drw_base.h"

namespace {

// Build a UTF-16LE byte stream from a sequence of 16-bit code units.
std::string utf16le(std::initializer_list<std::uint16_t> units) {
    std::string s;
    s.reserve(units.size() * 2);
    for (std::uint16_t u : units) {
        s.push_back(static_cast<char>(u & 0xFF));
        s.push_back(static_cast<char>((u >> 8) & 0xFF));
    }
    return s;
}

} // namespace

TEST_CASE("DRW_ConvUTF16::toUtf8 decodes BMP characters",
          "[i18n][utf16]") {
    DRW_ConvUTF16 conv;
    // U+0041 'A' + U+4E2D (中) + U+0042 'B'
    std::string in = utf16le({0x0041, 0x4E2D, 0x0042});
    std::string out = conv.toUtf8(in);
    // 'A' (0x41) + U+4E2D as 3-byte UTF-8 (E4 B8 AD) + 'B' (0x42)
    REQUIRE(out == std::string("\x41\xE4\xB8\xAD\x42", 5));
}

TEST_CASE("DRW_ConvUTF16::toUtf8 combines surrogate pairs",
          "[i18n][utf16][surrogate]") {
    DRW_ConvUTF16 conv;
    // U+1F600 😀 = surrogate pair (0xD83D, 0xDE00)
    std::string in = utf16le({0xD83D, 0xDE00});
    std::string out = conv.toUtf8(in);
    // U+1F600 as 4-byte UTF-8: F0 9F 98 80
    REQUIRE(out == std::string("\xF0\x9F\x98\x80", 4));
}

TEST_CASE("DRW_ConvUTF16::toUtf8 emits U+FFFD for unpaired high surrogate",
          "[i18n][utf16][surrogate]") {
    DRW_ConvUTF16 conv;
    // High surrogate without a low: should emit U+FFFD then continue.
    std::string in = utf16le({0xD83D, 0x0041});
    std::string out = conv.toUtf8(in);
    // U+FFFD (EF BF BD) + 'A' (0x41)
    REQUIRE(out == std::string("\xEF\xBF\xBD\x41", 4));
}

TEST_CASE("DRW_ConvUTF16::toUtf8 emits U+FFFD for stray low surrogate",
          "[i18n][utf16][surrogate]") {
    DRW_ConvUTF16 conv;
    std::string in = utf16le({0xDC00, 0x0041});
    std::string out = conv.toUtf8(in);
    REQUIRE(out == std::string("\xEF\xBF\xBD\x41", 4));
}

TEST_CASE("DRW_Converter::encodeMifText rejects malformed tokens",
          "[i18n][mif]") {
    DRW_Converter conv(nullptr, 0);
    REQUIRE(conv.encodeMifText("").empty());
    REQUIRE(conv.encodeMifText("\\U+1234").empty()); // wrong escape
    REQUIRE(conv.encodeMifText("\\M+").empty());       // truncated
    REQUIRE(conv.encodeMifText("\\M+0ABCD").empty()); // invalid selector 0
    REQUIRE(conv.encodeMifText("\\M+6ABCD").empty()); // invalid selector 6
    REQUIRE(conv.encodeMifText("\\M+1XYZW").empty()); // non-hex digits
}

TEST_CASE("DRW_Converter::encodeMifText: selector 1 (SJIS) decodes 0x8260 -> A",
          "[i18n][mif]") {
    DRW_Converter conv(nullptr, 0);
    // SJIS 0x8260 = 'Ａ' (full-width A, U+FF21)
    std::string out = conv.encodeMifText("\\M+18260");
    // U+FF21 as UTF-8: EF BC A1
    REQUIRE(out == std::string("\xEF\xBC\xA1", 3));
}

TEST_CASE("DRW_Converter::toUtf8 detects \\M+ alongside \\U+ in ASCII text",
          "[i18n][mif][unicode_escape]") {
    DRW_Converter conv(nullptr, 0);
    // Mix \U+ and \M+ escapes with surrounding ASCII.
    std::string in = "A\\U+00E9B\\M+18260C";
    std::string out = conv.toUtf8(in);
    // Expected: 'A' + 'é' (U+00E9 = C3 A9) + 'B' + 'Ａ' (U+FF21 = EF BC A1) + 'C'
    REQUIRE(out == std::string("A\xC3\xA9" "B\xEF\xBC\xA1" "C", 8));
}

TEST_CASE("DRW_ConvTable::toUtf8 detects \\U+ after high-bit table byte",
          "[i18n][unicode_escape]") {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    codec.setCodePage("ANSI_1252", /*dxfFormat=*/false);
    // Windows-1252 0xC9 = 'É' (U+00C9). Followed immediately by \U+00E9 = 'é'.
    std::string in = "\xC9\\U+00E9";
    std::string out = codec.toUtf8(in);
    // U+00C9 'É' (C3 89) + U+00E9 'é' (C3 A9)
    REQUIRE(out == std::string("\xC3\x89\xC3\xA9", 4));
}

TEST_CASE("decodeEedString: known codepage decodes through temporary codec",
          "[i18n][eed]") {
    // cp=38 (ANSI_932 / Shift-JIS). Raw bytes 0x82 0x60 = SJIS for 'Ａ' (U+FF21).
    std::string raw(2, '\0');
    raw[0] = '\x82';
    raw[1] = '\x60';
    std::string out = decodeEedString(38, raw, nullptr);
    REQUIRE(out == std::string("\xEF\xBC\xA1", 3));
}

TEST_CASE("decodeEedString: unknown codepage falls back to fallback decoder",
          "[i18n][eed]") {
    // cp=999 is unknown; should fall through to the fallback decoder.
    DRW_TextCodec fallback;
    fallback.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    fallback.setCodePage("ANSI_1252", /*dxfFormat=*/false);
    // CP-1252 0xC9 = 'É'.
    std::string raw(1, '\xC9');
    std::string out = decodeEedString(999, raw, &fallback);
    REQUIRE(out == std::string("\xC3\x89", 2));
}

TEST_CASE("decodeEedString: empty input returns empty",
          "[i18n][eed]") {
    REQUIRE(decodeEedString(38, std::string{}, nullptr).empty());
    DRW_TextCodec fallback;
    REQUIRE(decodeEedString(38, std::string{}, &fallback).empty());
}

TEST_CASE("decodeEedString: unknown codepage + null fallback returns raw",
          "[i18n][eed]") {
    std::string raw = "hello";
    REQUIRE(decodeEedString(999, raw, nullptr) == raw);
}

TEST_CASE("dwgCodePageId: round-trip with dwgCodePageName",
          "[i18n][codepage]") {
    // Every name produced by dwgCodePageName() must round-trip back to a
    // valid integer (which dwgCodePageName() then maps to the same name).
    for (std::uint16_t cp : {28, 29, 30, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 44}) {
        const char* name = dwgCodePageName(cp);
        REQUIRE(name != nullptr);
        REQUIRE(dwgCodePageId(name) == cp);
    }
    // null + unknown name both fall back to ANSI_1252 (id 30).
    REQUIRE(dwgCodePageId(nullptr) == 30);
    REQUIRE(dwgCodePageId("nonsense") == 30);
}

TEST_CASE("DRW_ConvUTF16::toUtf8 ignores odd trailing byte",
          "[i18n][utf16]") {
    DRW_ConvUTF16 conv;
    // Odd-length input: 'A' (2 bytes) + one stray byte.
    std::string in = utf16le({0x0041});
    in.push_back('\xAB');
    std::string out = conv.toUtf8(in);
    REQUIRE(out == std::string("A", 1));
}
