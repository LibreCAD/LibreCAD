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

// RS_FilterDXFRW::numberToWidth() and widthToNumber() are the two directions of
// the mapping between the DXF/DWG line-weight encoding and RS2::LineWidth.
// They used to be a pair of 27-case switches that had to be kept in step by
// hand; they are now two lookups over one table. These tests pin the property
// that made the pair correct - that the two directions agree - so a future edit
// to the table cannot silently break one direction, and cover the sentinel
// values that are easy to get wrong.

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <vector>

#include "drw_base.h"
#include "rs.h"
#include "rs_filterdxfrw.h"

namespace {

// Every DRW_LW_Conv::lineWidth the encoding defines, including the three
// sentinels at the end of the enum.
const std::vector<DRW_LW_Conv::lineWidth>& allDrwWidths() {
    static const std::vector<DRW_LW_Conv::lineWidth> widths{
        DRW_LW_Conv::width00, DRW_LW_Conv::width01, DRW_LW_Conv::width02,
        DRW_LW_Conv::width03, DRW_LW_Conv::width04, DRW_LW_Conv::width05,
        DRW_LW_Conv::width06, DRW_LW_Conv::width07, DRW_LW_Conv::width08,
        DRW_LW_Conv::width09, DRW_LW_Conv::width10, DRW_LW_Conv::width11,
        DRW_LW_Conv::width12, DRW_LW_Conv::width13, DRW_LW_Conv::width14,
        DRW_LW_Conv::width15, DRW_LW_Conv::width16, DRW_LW_Conv::width17,
        DRW_LW_Conv::width18, DRW_LW_Conv::width19, DRW_LW_Conv::width20,
        DRW_LW_Conv::width21, DRW_LW_Conv::width22, DRW_LW_Conv::width23,
        DRW_LW_Conv::widthByLayer, DRW_LW_Conv::widthByBlock,
        DRW_LW_Conv::widthDefault};
    return widths;
}

} // namespace

TEST_CASE("DXF line-width mapping round-trips in both directions",
          "[dxf][filter][linewidth]") {
    SECTION("every encoded width survives a round trip") {
        for (const DRW_LW_Conv::lineWidth lw : allDrwWidths()) {
            const RS2::LineWidth rs = RS_FilterDXFRW::numberToWidth(lw);
            INFO("encoded width " << static_cast<int>(lw));
            CHECK(RS_FilterDXFRW::widthToNumber(rs) == lw);
        }
    }

    SECTION("the mapping is injective, so no two widths collapse") {
        std::vector<RS2::LineWidth> seen;
        for (const DRW_LW_Conv::lineWidth lw : allDrwWidths()) {
            const RS2::LineWidth rs = RS_FilterDXFRW::numberToWidth(lw);
            for (const RS2::LineWidth other : seen) {
                INFO("encoded width " << static_cast<int>(lw)
                                      << " duplicates an earlier mapping");
                CHECK(other != rs);
            }
            seen.push_back(rs);
        }
        CHECK(seen.size() == allDrwWidths().size());
    }

    SECTION("the three sentinels keep their meaning") {
        CHECK(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthByLayer)
              == RS2::WidthByLayer);
        CHECK(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthByBlock)
              == RS2::WidthByBlock);
        CHECK(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthDefault)
              == RS2::WidthDefault);
        CHECK(RS_FilterDXFRW::widthToNumber(RS2::WidthByLayer)
              == DRW_LW_Conv::widthByLayer);
        CHECK(RS_FilterDXFRW::widthToNumber(RS2::WidthByBlock)
              == DRW_LW_Conv::widthByBlock);
        CHECK(RS_FilterDXFRW::widthToNumber(RS2::WidthDefault)
              == DRW_LW_Conv::widthDefault);
    }

    SECTION("an unmapped value falls back to the default width") {
        // The lookup has to answer something for a value outside the table;
        // the switches it replaced fell through to the default arm.  The enum
        // has no fixed underlying type, so its value range stops at its
        // largest enumerator (31): 24-28 are the only unmapped values that can
        // be formed without undefined behaviour.
        const auto unmapped = static_cast<DRW_LW_Conv::lineWidth>(25);
        CHECK(RS_FilterDXFRW::numberToWidth(unmapped) == RS2::WidthDefault);
    }
}

TEST_CASE("DXF line widths map to the standard lineweight values",
          "[dxf][filter][linewidth]") {
    // The round-trip and injectivity checks above hold just as well for a table
    // shifted by one, so pin the values themselves. RS2::LineWidth is the DXF
    // lineweight in hundredths of a millimetre, and DRW_LW_Conv::widthNN is the
    // index into AutoCAD's standard lineweight list, so the expected values are
    // that list and do not come from the mapping under test.
    static constexpr int kStandardLineweights[] = {
        0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50,
        53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211};

    for (int i = 0; i < static_cast<int>(std::size(kStandardLineweights)); ++i) {
        const auto encoded = static_cast<DRW_LW_Conv::lineWidth>(i);
        INFO("width" << (i < 10 ? "0" : "") << i);
        CHECK(static_cast<int>(RS_FilterDXFRW::numberToWidth(encoded))
              == kStandardLineweights[i]);
        CHECK(RS_FilterDXFRW::widthToNumber(
                  static_cast<RS2::LineWidth>(kStandardLineweights[i])) == encoded);
    }

    // The three sentinels carry the negative DXF codes.
    CHECK(static_cast<int>(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthByLayer)) == -1);
    CHECK(static_cast<int>(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthByBlock)) == -2);
    CHECK(static_cast<int>(RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::widthDefault)) == -3);
}
