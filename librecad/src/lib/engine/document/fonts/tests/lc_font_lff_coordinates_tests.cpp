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
 * Issue #3285: an LFF glyph line with a point that has no coordinate,
 * such as ",;0,0", crashed RS_Font::generateLffFont(), which read the
 * first coordinate of an empty list. Such a point is skipped now, and a
 * line left with fewer than two points draws nothing.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include "rs_block.h"
#include "rs_font.h"
#include "rs_polyline.h"
#include "rs_settings.h"

namespace {

void ensureLffTestApp() {
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

bool writeLffFont(const QString &path, const QByteArray &glyphs) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray header = "# Format: LibreCAD Font 1\n"
                              "# Name: Issue 3285\n"
                              "# LetterSpacing: 3\n"
                              "# WordSpacing: 6.75\n"
                              "# LineSpacingFactor: 1\n"
                              "\n";
    return file.write(header + glyphs) == header.size() + glyphs.size();
}

// the single polyline of a glyph, or nullptr
const RS_Polyline *onlyPolyline(RS_Block *letter) {
    if (letter == nullptr || letter->count() != 1) {
        return nullptr;
    }
    return dynamic_cast<const RS_Polyline *>(letter->entityAt(0));
}

} // namespace

TEST_CASE("LFF points without coordinates are skipped instead of crashing",
          "[font][lff][issue3285]") {
    ensureLffTestApp();

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("issue3285.lff");
    REQUIRE(writeLffFont(path,
                         // the reproducer from the issue: one real point left
                         "[0041] A\n"
                         ",;0,0\n"
                         "\n"
                         // an empty point between two real ones
                         "[0042] B\n"
                         "1,2;,;3,4\n"
                         "\n"
                         // a missing y-coordinate still means 0 (issue #2045)
                         "[0043] C\n"
                         "5;6,7\n"
                         "\n"
                         // a bad line does not drop the good ones
                         "[0044] D\n"
                         ",;,,\n"
                         "0,0;8,0\n"
                         "\n"));

    RS_Font font(path);
    REQUIRE(font.loadFont());

    SECTION("a line left with one point draws nothing") {
        CHECK(font.findLetter("A") == nullptr);
    }

    SECTION("the empty point is left out of the polyline") {
        const RS_Polyline *line = onlyPolyline(font.findLetter("B"));
        REQUIRE(line != nullptr);
        CHECK(line->getStartpoint() == RS_Vector(1., 2.));
        CHECK(line->getEndpoint() == RS_Vector(3., 4.));
        CHECK(line->count() == 1);
    }

    SECTION("a point with only x still gets y = 0") {
        const RS_Polyline *line = onlyPolyline(font.findLetter("C"));
        REQUIRE(line != nullptr);
        CHECK(line->getStartpoint() == RS_Vector(5., 0.));
        CHECK(line->getEndpoint() == RS_Vector(6., 7.));
    }

    SECTION("the other lines of the glyph are kept") {
        const RS_Polyline *line = onlyPolyline(font.findLetter("D"));
        REQUIRE(line != nullptr);
        CHECK(line->getStartpoint() == RS_Vector(0., 0.));
        CHECK(line->getEndpoint() == RS_Vector(8., 0.));
    }
}
