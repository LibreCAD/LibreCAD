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

// The app name of a registered DWG class is written into the CLASSES section
// verbatim, so any change to one of these strings changes the bytes of every
// file that registers that class.
//
// AutoCAD pads the DIMASSOC app name into columns, and the runs of spaces are
// part of the value. They are invisible in review and a reformatter or a
// careless re-transcription silently collapses them, so pin the string here.

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>

#include "dwgwriter.h"

namespace {

const DwgTypedClassRow* rowFor(const char* key) {
    for (const DwgTypedClassRow& row : kDwgTypedClassRows) {
        if (std::strcmp(row.key, key) == 0) {
            return &row;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("the DIMASSOC class app name keeps AutoCAD's column padding",
          "[dwg][classes]") {
    const DwgTypedClassRow* row = rowFor("DimensionAssociation");
    REQUIRE(row != nullptr);

    // Five, ten and six spaces after the three labels.
    const std::string expected =
        "AcDbDimAssoc|Product Desc:     AcDim ARX App For Dimension|"
        "Company:          Autodesk, Inc.|WEB Address:      www.autodesk.com";

    INFO("the runs of spaces in this app name are significant");
    CHECK(std::string(row->appName) == expected);
    CHECK(std::string(row->appName).size() == 126);
}

TEST_CASE("every registered class carries non-empty app and class names",
          "[dwg][classes]") {
    for (const DwgTypedClassRow& row : kDwgTypedClassRows) {
        INFO("row " << row.key);
        CHECK(row.appName != nullptr);
        CHECK(row.className != nullptr);
        CHECK(row.recordName != nullptr);
        CHECK(std::strlen(row.appName) > 0);
        CHECK(std::strlen(row.className) > 0);
        CHECK(std::strlen(row.recordName) > 0);
    }
}
