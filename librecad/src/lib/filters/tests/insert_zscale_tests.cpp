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

/**
 * Regression tests for issue #1428: LibreCAD wrote a zero Z scale (group code
 * 43) on INSERT entities, because inserts are built from two component
 * RS_Vectors whose z defaults to 0. A zero scale factor is degenerate - the
 * block transform becomes singular - and consumers reject it: FreeCAD refuses
 * the drawing, netDxf and ACadSharp throw on it, ezdxf rewrites it to 1.
 *
 * The scale is read straight out of the exported file rather than out of the
 * RS_Insert, so the normalization in RS_InsertData cannot mask a bad write.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include <QCoreApplication>

#include "rs_block.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

void ensureApp() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char *argv[] = {arg0, nullptr};
    static QCoreApplication *app = QCoreApplication::instance()
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

/// The 41/42/43 scale factors of the first INSERT in an ASCII DXF.
struct FileScale {
    bool found = false;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

FileScale readFirstInsertScale(const std::string &path) {
    std::ifstream file(path);
    FileScale scale;
    std::string code;
    std::string value;
    bool inInsert = false;
    while (std::getline(file, code) && std::getline(file, value)) {
        const int group = std::stoi(code);
        if (group == 0) {
            if (inInsert) {
                break; // end of the INSERT record
            }
            inInsert = value.find("INSERT") != std::string::npos;
            continue;
        }
        if (!inInsert) {
            continue;
        }
        switch (group) {
        case 41: scale.x = std::stod(value); scale.found = true; break;
        case 42: scale.y = std::stod(value); break;
        case 43: scale.z = std::stod(value); break;
        default: break;
        }
    }
    return scale;
}

/// Exports a graphic holding one block and one insert, and returns the scale
/// that landed in the file.
FileScale exportInsertScale(const RS_Vector &scale, const char *name) {
    const std::string path =
        (std::filesystem::temp_directory_path() /
         (std::string("insert_zscale_") + name + ".dxf")).string();
    std::filesystem::remove(path);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto *block = new RS_Block(
        &graphic, RS_BlockData(QStringLiteral("BASE"), RS_Vector(0.0, 0.0), false));
    block->addEntity(
        new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
    graphic.addBlock(block);
    graphic.addEntity(new RS_Insert(
        &graphic, RS_InsertData(QStringLiteral("BASE"), RS_Vector(1.0, 2.0), scale,
                                0.0, 1, 1, RS_Vector(0.0, 0.0))));

    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(path),
                              RS2::FormatDXFRW));
    const FileScale result = readFirstInsertScale(path);
    std::filesystem::remove(path);
    return result;
}

} // namespace

TEST_CASE("A degenerate Z scale never survives", "[insert][zscale]") {
    // Only values that leave the block transform singular or meaningless are
    // repaired: exact zero (either sign), NaN and infinity. A plain "!= 0.0"
    // would pass the last three straight through to the file.
    CHECK(RS_InsertData::usableScale(0.0) == Catch::Approx(1.0));
    CHECK(RS_InsertData::usableScale(-0.0) == Catch::Approx(1.0));
    CHECK(RS_InsertData::usableScale(
              std::numeric_limits<double>::quiet_NaN()) == Catch::Approx(1.0));
    CHECK(RS_InsertData::usableScale(
              std::numeric_limits<double>::infinity()) == Catch::Approx(1.0));
    CHECK(RS_InsertData::usableScale(
              -std::numeric_limits<double>::infinity()) == Catch::Approx(1.0));

    // Everything else is data, mirrors and tiny factors included. Every
    // consumer surveyed (ezdxf, ACadSharp, netDxf, and LibreCAD's own
    // LC_InsertTransform::fromInsert) rejects only an exact zero, so
    // rewriting a small nonzero factor would resize valid geometry.
    CHECK(RS_InsertData::usableScale(4.0) == Catch::Approx(4.0));
    CHECK(RS_InsertData::usableScale(-1.0) == Catch::Approx(-1.0));
    CHECK(RS_InsertData::usableScale(1.0e-13) == 1.0e-13);
    CHECK(RS_InsertData::usableScale(1.0e-300) == 1.0e-300);
}

TEST_CASE("A degenerate X or Y scale never survives either", "[insert][zscale]") {
    // Any zero component makes the block transform singular, so all three are
    // held to the same rule: netDxf rejects the entity with "None of the
    // vector scale components can be zero", ACadSharp throws on each setter.
    const RS_Vector repaired =
        RS_InsertData::usableScale(RS_Vector(0.0, 1.0e-13, 0.0));
    CHECK(repaired.x == Catch::Approx(1.0));
    CHECK(repaired.y == 1.0e-13); // tiny but nonzero: data, not a defect
    CHECK(repaired.z == Catch::Approx(1.0));

    const RS_Vector kept = RS_InsertData::usableScale(RS_Vector(2.0, -3.0, 4.0));
    CHECK(kept.x == Catch::Approx(2.0));
    CHECK(kept.y == Catch::Approx(-3.0)); // a mirror, not a defect
    CHECK(kept.z == Catch::Approx(4.0));

    // The repair is per-component; it must not manufacture a valid vector
    // out of one that was explicitly marked invalid.
    CHECK_FALSE(RS_InsertData::usableScale(RS_Vector(false)).valid);
    CHECK(RS_InsertData::usableScale(RS_Vector(1.0, 1.0)).valid);
}

TEST_CASE("RS_InsertData keeps the Z scale non-zero", "[insert][zscale]") {
    // How every 2D call site builds it: the third component defaults to 0.
    const RS_InsertData twoDim(QStringLiteral("BASE"), RS_Vector(1.0, 2.0),
                               RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0));
    CHECK(twoDim.scaleFactor.z == Catch::Approx(1.0));

    // An explicit Z scale is data, not a default, and must survive untouched.
    const RS_InsertData threeDim(QStringLiteral("BASE"), RS_Vector(1.0, 2.0),
                                 RS_Vector(2.0, 3.0, 4.0), 0.0, 1, 1,
                                 RS_Vector(0.0, 0.0));
    CHECK(threeDim.scaleFactor.z == Catch::Approx(4.0));
}

TEST_CASE("DXF export never writes a zero Z scale on INSERT", "[insert][zscale]") {
    ensureApp();

    const FileScale twoDim = exportInsertScale(RS_Vector(1.0, 1.0), "twodim");
    REQUIRE(twoDim.found);
    CHECK(twoDim.x == Catch::Approx(1.0));
    CHECK(twoDim.y == Catch::Approx(1.0));
    CHECK(twoDim.z == Catch::Approx(1.0)); // the regression: used to be 0

    const FileScale explicitZ =
        exportInsertScale(RS_Vector(2.0, 3.0, 4.0), "explicit");
    REQUIRE(explicitZ.found);
    CHECK(explicitZ.x == Catch::Approx(2.0));
    CHECK(explicitZ.y == Catch::Approx(3.0));
    CHECK(explicitZ.z == Catch::Approx(4.0));

    // A malformed import cannot carry a degenerate X or Y back out either.
    const FileScale degenerate =
        exportInsertScale(RS_Vector(0.0, 0.0, 0.0), "degenerate");
    REQUIRE(degenerate.found);
    CHECK(degenerate.x == Catch::Approx(1.0));
    CHECK(degenerate.y == Catch::Approx(1.0));
    CHECK(degenerate.z == Catch::Approx(1.0));
}
