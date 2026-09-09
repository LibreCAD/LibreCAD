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

// Three pre-R13 record-shape decisions, each of which failed a whole file
// when it was wrong rather than degrading. The expected values come from the
// on-disk records of real drawings (quoted per case) and from libredwg's
// dwg.h, not from the reader.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "dwgreaderR11.h"

TEST_CASE("the STYLE record width follows the file version",
          "[dwg][prer13][tables]") {
    // Measured across the local corpus: R11 declares 198, R10/R9/R2.6 declare
    // 194, and R2.10 declares 130. The reader may not demand more than the
    // file declares, or it rejects the file outright.
    CHECK(preR13StyleRecordMinSize(DRW::AC1009) <= 198);
    CHECK(preR13StyleRecordMinSize(DRW::AC1006) <= 194);
    CHECK(preR13StyleRecordMinSize(DRW::AC1004) <= 194);
    CHECK(preR13StyleRecordMinSize(DRW::AC1003) <= 194);
    CHECK(preR13StyleRecordMinSize(DRW::AC210) <= 130);

    SECTION("the widths are the field layouts, not round numbers") {
        // flag 1 + name 32 + textSize 8 + width 8 + oblique 8 + generation 1
        // + lastHeight 8 + font 64 = 130, plus bigfont 64 from R2.6, plus
        // `used` 2 at R11.
        constexpr std::uint16_t kThroughFont = 1 + 32 + 8 + 8 + 8 + 1 + 8 + 64;
        static_assert(kThroughFont == 130, "R2.10 STYLE record");
        CHECK(preR13StyleRecordMinSize(DRW::AC210) == kThroughFont);
        CHECK(preR13StyleRecordMinSize(DRW::AC1006) == kThroughFont + 64);
        CHECK(preR13StyleRecordMinSize(DRW::AC1009) == kThroughFont + 64 + 2);
    }

    SECTION("big fonts start at R2.6, so only R2.10 and older are short") {
        CHECK(preR13StyleRecordMinSize(DRW::AC1003) == 194);
        CHECK(preR13StyleRecordMinSize(DRW::AC210) == 130);
        CHECK(preR13StyleRecordMinSize(DRW::AC150) == 130);
        CHECK(preR13StyleRecordMinSize(DRW::MC00) == 130);
    }
}

TEST_CASE("the entity-record bound scales with the section",
          "[dwg][prer13][entities]") {
    // A record advances by its own size field, which the walker rejects below
    // 5 bytes, so a section of N bytes holds at most N/5 of them.
    CHECK(preR13MaxRecordCount(0, 100) == 21);
    CHECK(preR13MaxRecordCount(1000, 1100) == 21);

    SECTION("an empty or inverted range holds nothing") {
        CHECK(preR13MaxRecordCount(500, 500) == 0);
        CHECK(preR13MaxRecordCount(500, 400) == 0);
    }

    SECTION("a large real section needs more than the old fixed cap") {
        // The BLOCKS section of a 183 MB R11 drawing: 0x0031EF43..0xAF5C9FE.
        // It walked past 2,000,000 records with 36 MB still to read, and the
        // fixed cap that stopped it there failed the entire file.
        constexpr std::uint32_t kStart = 0x0031EF43u;
        constexpr std::uint32_t kEnd = 0x0AF5C9FEu;
        CHECK(preR13MaxRecordCount(kStart, kEnd) > 2000000u);
        CHECK(preR13MaxRecordCount(kStart, kEnd) == (kEnd - kStart) / 5 + 1);
    }
}

TEST_CASE("a pre-R13 VERTEX body follows its opts word",
          "[dwg][prer13][entities]") {
    SECTION("a point vertex, as seen in a real R11 polyline") {
        // opts 0x0008 on a 29-byte record: 4 header + 2 layer + 2 opts
        // + 2 handling + 16 point + 1 flag + 2 trailing.
        const PreR13VertexLayout layout = preR13VertexLayout(0x0008);
        CHECK(layout.hasPoint);
        CHECK(layout.hasFlag);
        CHECK_FALSE(layout.hasStartWidth);
        CHECK_FALSE(layout.hasEndWidth);
        CHECK_FALSE(layout.hasBulge);
        CHECK_FALSE(layout.hasTangent);
        CHECK_FALSE(layout.hasIndex1);
        CHECK_FALSE(layout.hasIndex4);
    }

    SECTION("a polyface face record carries indices instead of a point") {
        // opts 0x41E8 on a 21-byte record from the same drawing, holding the
        // face 1/2/5/4: 4 header + 2 layer + 2 opts + 2 handling + 1 flag
        // + 4x2 indices + 2 trailing. Reading a point here would need 16
        // bytes that the record does not have.
        const PreR13VertexLayout layout = preR13VertexLayout(0x41E8);
        CHECK_FALSE(layout.hasPoint);
        CHECK(layout.hasFlag);
        CHECK(layout.hasIndex1);
        CHECK(layout.hasIndex2);
        CHECK(layout.hasIndex3);
        CHECK(layout.hasIndex4);
        CHECK_FALSE(layout.hasStartWidth);
        CHECK_FALSE(layout.hasEndWidth);
        CHECK_FALSE(layout.hasBulge);
        CHECK_FALSE(layout.hasTangent);
    }

    SECTION("HAS_NOT_X_Y alone decides which arm applies") {
        // The width and index bits overlap in meaning between the two arms,
        // so each must be ignored on the wrong side of 0x4000.
        const PreR13VertexLayout point = preR13VertexLayout(0x01FF & ~0x0008);
        CHECK(point.hasPoint);
        CHECK(point.hasStartWidth);
        CHECK(point.hasEndWidth);
        CHECK(point.hasBulge);
        CHECK(point.hasTangent);
        CHECK_FALSE(point.hasFlag);
        CHECK_FALSE(point.hasIndex1);
        CHECK_FALSE(point.hasIndex2);
        CHECK_FALSE(point.hasIndex3);
        CHECK_FALSE(point.hasIndex4);

        const PreR13VertexLayout face = preR13VertexLayout(0x4000 | 0x01FF);
        CHECK_FALSE(face.hasPoint);
        CHECK_FALSE(face.hasStartWidth);
        CHECK_FALSE(face.hasEndWidth);
        CHECK_FALSE(face.hasBulge);
        CHECK_FALSE(face.hasTangent);
        CHECK(face.hasFlag);
        CHECK(face.hasIndex1);
        CHECK(face.hasIndex4);
    }

    SECTION("a bare point vertex reads only its point") {
        const PreR13VertexLayout layout = preR13VertexLayout(0);
        CHECK(layout.hasPoint);
        CHECK_FALSE(layout.hasStartWidth);
        CHECK_FALSE(layout.hasEndWidth);
        CHECK_FALSE(layout.hasBulge);
        CHECK_FALSE(layout.hasFlag);
        CHECK_FALSE(layout.hasTangent);
    }
}
