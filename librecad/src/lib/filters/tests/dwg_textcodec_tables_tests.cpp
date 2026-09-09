/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Codepage-table behaviour that is invisible until a file is written or a
// specific character is read, so each case here pins bytes rather than shapes.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "drw_textcodec.h"

namespace {

std::string decodeBytes(const char* codePage, std::initializer_list<int> bytes) {
    DRW_TextCodec codec;
    codec.setCodePage(codePage, false);
    std::string raw;
    for (int b : bytes) {
        raw += static_cast<char>(static_cast<unsigned char>(b));
    }
    return codec.toUtf8(raw);
}

std::string encodeUtf8(const char* codePage, const std::string& utf8) {
    DRW_TextCodec codec;
    codec.setCodePage(codePage, false);
    return codec.fromUtf8(utf8);
}

} // namespace

TEST_CASE("an unmappable code point becomes a bare escape",
          "[dwg][dxf][codec]") {
    // A code point the target codepage cannot hold is written as AutoCAD's
    // \U+XXXX escape. On macOS this used to be built by assigning a whole
    // 16-char buffer, so the escape carried 12 trailing NUL bytes: a DXF
    // writer then refused the string outright and a DWG <= R2004 writer put
    // the NULs into the file.
    const std::string escaped = encodeUtf8("ANSI_1252", "\xE4\xB8\x80"); // U+4E00
    CHECK(escaped == "\\U+4E00");
    CHECK(escaped.size() == 7);
    CHECK(escaped.find('\0') == std::string::npos);
}
TEST_CASE("windows-1255 maps the hole at 0xCA", "[dwg][dxf][codec]") {
    // The Hebrew points run 0xC0+n -> U+05B0+n. 0xCA was the only gap,
    // because U+05BA did not exist when the codepage was defined.
    CHECK(decodeBytes("ANSI_1255", {0xC9}) == "\xD6\xB9");  // U+05B9
    CHECK(decodeBytes("ANSI_1255", {0xCA}) == "\xD6\xBA");  // U+05BA
    CHECK(decodeBytes("ANSI_1255", {0xCB}) == "\xD6\xBB");  // U+05BB

    SECTION("and encodes back to the same byte") {
        CHECK(encodeUtf8("ANSI_1255", "\xD6\xBA") == "\xCA");
    }
}
TEST_CASE("every double-byte entry is reachable through its lead byte",
          "[dwg][dxf][codec]") {
    // The lead table indexes ranges of the double table, so an off-by-one
    // there silently drops mappings that are present in the data: the
    // decoder just answers '?'. These are entries that were unreachable.
    SECTION("cp936 entries that sat in the wrong bucket") {
        CHECK(decodeBytes("ANSI_936", {0xA0, 0x40}) == "\xE7\x87\x96"); // U+71D6
        CHECK(decodeBytes("ANSI_936", {0xA0, 0x41}) == "\xE7\x87\x97"); // U+71D7
    }
    SECTION("cp949 entries that sat in the wrong bucket, and its last entry") {
        CHECK(decodeBytes("ANSI_949", {0x84, 0x41}) == "\xEA\xBB\xA6"); // U+AEE6
        CHECK(decodeBytes("ANSI_949", {0xFD, 0xFE}) == "\xE8\xA9\xB0"); // U+8A70
    }
    SECTION("cp950 entries that sat in the wrong bucket, and its last entry") {
        CHECK(decodeBytes("ANSI_950", {0xC4, 0x40}) == "\xE9\xA1\x98"); // U+9858
        CHECK(decodeBytes("ANSI_950", {0xC4, 0x49}) == "\xE9\xAF\xA7"); // U+9BE7
        CHECK(decodeBytes("ANSI_950", {0xF9, 0xFE}) == "\xE2\x96\x93"); // U+2593
    }
}
TEST_CASE("big5 carries the hkscs extension", "[dwg][dxf][codec]") {
    // Sequences the plain cp950 table did not hold.
    CHECK(decodeBytes("ANSI_950", {0x87, 0x40}) == "\xE4\x8F\xB0"); // U+43F0

    SECTION("the cp950 reading of 0xF9FE is kept, not the hkscs one") {
        // cp950 says U+2593 (DARK SHADE), big5-hkscs says U+FFED. The two
        // are a genuine vendor fork rather than a gap, so the existing
        // reading stands and only absent sequences were added.
        CHECK(decodeBytes("ANSI_950", {0xF9, 0xFE}) == "\xE2\x96\x93");
    }
}
TEST_CASE("double-byte encoding round-trips through the reverse index",
          "[dwg][dxf][codec]") {
    // fromUtf8 answers from a prebuilt map now instead of scanning the whole
    // double table per character; it must still pick the same mapping.
    struct { const char* cp; int lead; int trail; } cases[] = {
        {"ANSI_936", 0xA4, 0x40}, {"ANSI_936", 0xA0, 0x40},
        {"ANSI_949", 0x84, 0x41}, {"ANSI_950", 0xA4, 0x40},
        {"ANSI_950", 0x87, 0x40},
    };
    for (const auto& c : cases) {
        const std::string utf8 = decodeBytes(c.cp, {c.lead, c.trail});
        INFO(c.cp << " 0x" << std::hex << c.lead << c.trail);
        REQUIRE_FALSE(utf8.empty());
        if (utf8 == "?") {
            continue; // not in this codepage; nothing to round-trip
        }
        const std::string back = encodeUtf8(c.cp, utf8);
        REQUIRE(back.size() == 2);
        CHECK(static_cast<unsigned char>(back[0]) == c.lead);
        CHECK(static_cast<unsigned char>(back[1]) == c.trail);
    }
}
