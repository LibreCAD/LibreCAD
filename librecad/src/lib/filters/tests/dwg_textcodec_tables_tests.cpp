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
