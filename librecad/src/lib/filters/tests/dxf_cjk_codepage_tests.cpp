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

// The Big5, Big5-HKSCS and UHC tables had no drawing behind them: every local
// corpus file is ANSI_1252, ANSI_1251 or ANSI_936, so cp950 and cp949 were
// exercised only by unit tests over the tables themselves. These fixtures put
// a real file on each path - a layer name and a TEXT string encoded in the
// codepage the header declares - so the whole chain is covered: $DWGCODEPAGE
// selects the converter, the converter decodes the bytes, and the filter
// hands UTF-8 to the document.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QString>

#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_text.h"
#include "rs_settings.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance()
                                       ? QCoreApplication::instance()
                                       : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

QString fixture(const char* name) {
    return QString(LIBRECAD_TEST_DIR) + "/dxf/" + name;
}

// The layer the fixture declares, which is also the layer its TEXT sits on.
QString importedLayerName(const char* name) {
    ensureSettings();
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, fixture(name), RS2::FormatDXFRW));
    for (unsigned i = 0; i < graphic.getLayerList()->count(); ++i) {
        RS_Layer* layer = graphic.getLayerList()->at(i);
        if (layer != nullptr && layer->getName() != "0")
            return layer->getName();
    }
    return QString{};
}

// The single TEXT entity the fixture carries.
QString importedText(const char* name) {
    ensureSettings();
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, fixture(name), RS2::FormatDXFRW));
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityText)
            return static_cast<RS_Text*>(e)->getText();
    }
    return QString{};
}

} // namespace

TEST_CASE("a Big5 drawing decodes its traditional Chinese names",
          "[dxf][codec][cjk]") {
    // layer B9 CF BC 68, text A4 A4 A4 E5 B4 FA B8 D5 under ANSI_950.
    CHECK(importedLayerName("big5_traditional.dxf")
          == QString::fromUtf8("\xE5\x9C\x96\xE5\xB1\xA4"));      // U+5716 U+5C64
}

TEST_CASE("a Big5-HKSCS drawing decodes the Hong Kong additions",
          "[dxf][codec][cjk]") {
    // F9 D5 is base Big5; C8 7E is an hkscs row this branch added. The text
    // also carries 88 66 (U+00CA, the Hong Kong Latin block) and 9C 71, which
    // is a supplementary code point - it survives the table but the font
    // engine cannot draw it yet.
    CHECK(importedLayerName("big5_hkscs.dxf")
          == QString::fromUtf8("\xE9\xBE\x98\xE4\x92\x91"));      // U+9F98 U+4491

    SECTION("a sequence standing for two code points expands to both") {
        // 88 62 is one of the four big5-hkscs pointers the standard maps to a
        // pair rather than a single code point, so a one-int table cell packs
        // them. It used to decode as '?'.
        CHECK(importedText("big5_hkscs.dxf")
              == QString::fromUtf8("\xC3\x8A\xCC\x84"               // U+00CA U+0304
                                   "\xE4\x92\x91"                     // U+4491
                                   "\xF0\xA0\x80\xA1"));             // U+20021
    }
}

TEST_CASE("a UHC drawing decodes its Korean names", "[dxf][codec][cjk]") {
    // layer B5 B5 B8 E9 under ANSI_949.
    CHECK(importedLayerName("uhc_korean.dxf")
          == QString::fromUtf8("\xEB\x8F\x84\xEB\xA9\xB4"));      // U+B3C4 U+BA74
}
