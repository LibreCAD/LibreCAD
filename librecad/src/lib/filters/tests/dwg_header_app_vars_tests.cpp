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
 * Phase 0A end-to-end header → app $-var regression harness.
 *
 * DWG-read header vars arrive in DRW_Header::vars under *bare* keys
 * ("LUPREC", "DIMASO", ...), whereas the application — and the DXF
 * import path — query them $-prefixed ("$LUPREC").  P0A-1 normalizes
 * the keys inside RS_FilterDXFRW::addHeader so DWG-stored vars become
 * reachable via RS_Graphic::getVariable*.
 *
 * These tests drive a DRW_Header carrying bare keys through the real
 * RS_FilterDXFRW::addHeader (against a live RS_Graphic) and assert the
 * vars resolve under their $-prefixed names — guarding the gap
 * `header-$prefix-normalization`.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>

#include "drw_header.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_settings.h"

/// Test-only accessor: RS_FilterDXFRW::addHeader operates on the private
/// m_graphic / m_currentContainer members.  Declared a friend by
/// rs_filterdxfrw.h so the harness can point the filter at a test graphic.
class RsFilterDxfRwHeaderTestAccess {
public:
    static void runAddHeader(RS_FilterDXFRW& filter, RS_Graphic& graphic,
                             const DRW_Header& header) {
        filter.m_graphic = &graphic;
        filter.m_currentContainer = &graphic;
        filter.addHeader(&header);
    }
};

namespace {

// RS_Graphic's ctor calls into LC_GROUP_GUARD("Defaults") + RS_Settings;
// both dereference null without a QCoreApplication context.  Mirrors the
// bootstrap rs_graphic_layouts_tests.cpp uses for the same reason.
void ensureQtContext() {
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = {qarg0, nullptr};
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    (void)qapp;
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
}

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG header bare keys reach the app $-prefixed",
          "[dwg-header][header-app-vars]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterDXFRW filter;

    DRW_Header header;
    // DWG store convention: bare keys, no '$'.
    header.addInt("DIMASO", 1, 70);
    header.addInt("LUPREC", 6, 70);

    RsFilterDxfRwHeaderTestAccess::runAddHeader(filter, graphic, header);

    // Resolve under the $-prefixed names the application queries.
    REQUIRE(graphic.getVariableInt("$DIMASO", -1) == 1);
    REQUIRE(graphic.getVariableInt("$LUPREC", -1) == 6);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG header normalization does not double-prefix already-$ keys",
          "[dwg-header][header-app-vars]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterDXFRW filter;

    DRW_Header header;
    // DXF-style key already carries '$'; must not become "$$ACADVER".
    header.addStr("$ACADVER", "AC1021", 1);

    RsFilterDxfRwHeaderTestAccess::runAddHeader(filter, graphic, header);

    REQUIRE(graphic.getVariableString("$ACADVER", "") == QString("AC1021"));
    // The double-prefixed form must NOT exist.
    REQUIRE(graphic.getVariableString("$$ACADVER", "MISSING")
            == QString("MISSING"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG header string and double vars normalize too",
          "[dwg-header][header-app-vars]") {
    ensureQtContext();
    RS_Graphic graphic;
    RS_FilterDXFRW filter;

    DRW_Header header;
    header.addStr("TEXTSTYLE", "Standard", 7);
    header.addDouble("DIMSCALE", 2.5, 40);

    RsFilterDxfRwHeaderTestAccess::runAddHeader(filter, graphic, header);

    REQUIRE(graphic.getVariableString("$TEXTSTYLE", "") == QString("Standard"));
    REQUIRE(graphic.getVariableDouble("$DIMSCALE", -1.0) == 2.5);
}
