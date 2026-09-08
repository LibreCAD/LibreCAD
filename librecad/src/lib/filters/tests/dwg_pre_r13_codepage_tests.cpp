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

// $DWGCODEPAGE handling for pre-R13 (R2.10/R9/R10/R11/R12) table records.
//
// Those records store names in fixed-width, NUL-padded fields of raw codepage
// bytes. The reader resolved the codepage correctly but assembled the fields
// byte by byte and never decoded them, so a GBK layer name reached the document
// as raw bytes and QString::fromUtf8() turned it into U+FFFD. preR13FixedText()
// is the single place that now reads and decodes those fields.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "drw_textcodec.h"
#include "dwgbuffer.h"
#include "dwgreaderR11.h"

namespace {

/// A fixed-width DWG name field: `bytes`, NUL-padded out to `width`.
std::vector<std::uint8_t> field(const std::string& bytes, const std::size_t width) {
    std::vector<std::uint8_t> raw(width, 0);
    for (std::size_t i = 0; i < bytes.size() && i < width; ++i) {
        raw[i] = static_cast<std::uint8_t>(bytes[i]);
    }
    return raw;
}

std::string readField(std::vector<std::uint8_t>& raw, const int width, DRW_TextCodec& codec,
                      std::uint64_t* endPos = nullptr) {
    dwgBuffer buf(raw.data(), raw.size());
    const std::string text = preR13FixedText(buf, width, codec);
    if (endPos != nullptr) {
        *endPos = buf.getPosition();
    }
    return text;
}

void useGbk(DRW_TextCodec& codec) {
    codec.setCodePage("ANSI_936", /*dxfFormat=*/false);
}

} // namespace

TEST_CASE("pre-R13 fixed name fields are decoded with the file codepage",
          "[dwg][pre_r13][codepage]") {
    DRW_TextCodec codec;
    useGbk(codec);

    SECTION("GBK bytes become UTF-8") {
        // "\xbd\xa8\xd6\xfe" is GBK for the two characters of the layer name
        // prefix seen on real drawings; U+5EFA U+7B51.
        auto raw = field("\xbd\xa8\xd6\xfe" "_1", 32);
        CHECK(readField(raw, 32, codec) == "\xe5\xbb\xba\xe7\xad\x91" "_1");
    }

    SECTION("ASCII is unchanged") {
        auto raw = field("LAYER_0", 32);
        CHECK(readField(raw, 32, codec) == "LAYER_0");
    }

    SECTION("the field stops at the first NUL but still consumes its full width") {
        // The pre-R13 record layout is strictly sequential: reading fewer bytes
        // than the field is wide desyncs every field after it.
        auto raw = field("AB", 32);
        std::uint64_t endPos = 0;
        CHECK(readField(raw, 32, codec, &endPos) == "AB");
        CHECK(endPos == 32);
    }

    SECTION("a field with no NUL uses its whole width") {
        auto raw = field(std::string(32, 'X'), 32);
        CHECK(readField(raw, 32, codec) == std::string(32, 'X'));
    }
}

TEST_CASE("a double-byte character truncated by the field width is not read past",
          "[dwg][pre_r13][codepage]") {
    // A fixed-width field can cut a double-byte character in half. Each case
    // keeps a valid trail byte in the backing buffer just past the end of the
    // view: if the decoder steps past the end to find one, it forms the whole
    // character and the check below sees it.

    SECTION("GBK") {
        DRW_TextCodec codec;
        useGbk(codec);
        const char backing[] = "\xbd\xa8";              // whole char: U+5EFA
        const std::string_view leadOnly(backing, 1);   // view stops after the lead
        INFO("decoding a lone lead byte must not consume the byte after the view");
        CHECK(codec.toUtf8(leadOnly) != "\xe5\xbb\xba");
    }

    SECTION("Shift-JIS") {
        DRW_TextCodec codec;
        codec.setCodePage("ANSI_932", /*dxfFormat=*/false);
        const char backing[] = "\x81\x40";              // whole char: U+3000
        const std::string_view leadOnly(backing, 1);
        CHECK(codec.toUtf8(leadOnly) != "\xe3\x80\x80");
    }

    SECTION("through a fixed-width field") {
        DRW_TextCodec codec;
        useGbk(codec);
        auto raw = field(std::string(31, 'A') + "\xbd", 32);
        std::uint64_t endPos = 0;
        const std::string out = readField(raw, 32, codec, &endPos);
        CHECK(endPos == 32);
        CHECK(out.compare(0, 31, std::string(31, 'A')) == 0);
    }
}
