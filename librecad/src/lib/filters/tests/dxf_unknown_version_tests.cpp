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

// Regression tests for the source-version default used while reading DXF.
//
// DRW_TextCodec::sourceVersion is the version of the file being read. Until
// $ACADVER is parsed it is genuinely unknown, and the reader's strictness
// checks are written to reflect that: requiresDxfSelfHandle() and
// dxfTableEntryComplete() (libdxfrw.cpp) both special-case DRW::UNKNOWNV and
// only demand a per-record handle for post-R12 sources.
//
// The member used to default to DRW::AC1021 instead, so those UNKNOWNV
// branches were unreachable: a DXF carrying no $ACADVER was treated as R2007
// and every table record without a group-5 handle was rejected, failing the
// whole import with DRW::BAD_CODE_PARSED. Hand-written DXF files and R12-era
// output legitimately omit both $ACADVER and table-record handles, so they
// could not be opened at all.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include <QCoreApplication>

#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
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

std::string writeTemp(const char* suffix, const std::string& content) {
    const std::string path =
        (std::filesystem::temp_directory_path() /
         (std::string("dxf_unknown_version_") + suffix + ".dxf"))
            .string();
    std::ofstream out(path);
    out << content;
    return path;
}

// One LINE on layer "0"; no HEADER section, so no $ACADVER.
const std::string kEntities =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF without $ACADVER imports when table records carry no handle",
          "[dxf][filter][version][regression]") {
    ensureSettings();

    // A LAYER table record with a name but no group-5 handle, which is what
    // R12-era and hand-written DXF files contain.
    const std::string dxf =
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n"
        "0\nLAYER\n2\n0\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n" +
        kEntities;
    const std::string src = writeTemp("no_acadver", dxf);

    RS_Graphic graphic;
    graphic.initForNewDocument();

    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));

    // The layer table entry and the entity both survive the import.
    RS_Layer* layer0 = graphic.findLayer("0");
    REQUIRE(layer0 != nullptr);

    int entityCount = 0;
    for (auto it = graphic.begin(); it != graphic.end(); ++it)
        ++entityCount;
    CHECK(entityCount == 1);

    std::filesystem::remove(src);
}

TEST_CASE("DXF declaring a modern $ACADVER still requires table-record handles",
          "[dxf][filter][version][regression]") {
    ensureSettings();

    // Same table record, but the file now claims to be AC1021. A post-R12
    // source must still be held to the handle requirement, so the lenient
    // unknown-version path must not weaken files that state their version.
    const std::string dxf =
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n"
        "0\nLAYER\n2\n0\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n" +
        kEntities;
    const std::string src = writeTemp("acadver_ac1021", dxf);

    RS_Graphic graphic;
    graphic.initForNewDocument();

    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(src),
                                  RS2::FormatDXFRW));

    std::filesystem::remove(src);
}
