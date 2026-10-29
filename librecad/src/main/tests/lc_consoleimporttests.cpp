// File: lc_consoleimporttests.cpp

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QTemporaryDir>

#include "console_command_utils.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "rs_graphic.h"

namespace {

// A drawing holding one line.
constexpr const char* kLineDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n100.0\n21\n50.0\n31\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

QString writeFile(const QTemporaryDir& dir, const QString& name, const char* content) {
    const QString path = dir.filePath(name);
    QFile file{path};
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(content) > 0);
    return path;
}

} // namespace

TEST_CASE("console import reads a drawing", "[console][import]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    auto graphic = LC_Console::importGraphic(writeFile(dir, "line.dxf", kLineDxf));
    REQUIRE(graphic);
    CHECK(graphic->count() == 1);
}

TEST_CASE("console staged import validates and restores a native offset DXF",
          "[console][import][offsetcurve]") {
    REQUIRE(lc::test::application() != nullptr);
    const QString path = QStringLiteral(LIBRECAD_TEST_DIR) +
        "/offsetcurve_independent/dxf-binary-R2018-native-degree1.dxf";

    auto graphic = LC_Console::importGraphic(path);
    REQUIRE(graphic);
    REQUIRE(graphic->count() == 1);
    CHECK(dynamic_cast<LC_OffsetCurve*>(graphic->entityAt(0)) != nullptr);
}

TEST_CASE("console staged import validates and restores a native offset DWG",
          "[console][import][offsetcurve][dwg]") {
    REQUIRE(lc::test::application() != nullptr);
    const QString path = QStringLiteral(LIBRECAD_TEST_DIR) +
        "/offsetcurve_independent/oda-dwg-R2018-native-degree1.dwg";

    auto graphic = LC_Console::importGraphic(path);
    REQUIRE(graphic);
    REQUIRE(graphic->count() == 1);
    CHECK(dynamic_cast<LC_OffsetCurve*>(graphic->entityAt(0)) != nullptr);
}

// Before the import reported errors itself, a file it could not read opened a
// message box, and a console command waited for it forever. This test would
// hang rather than fail.
TEST_CASE("console import fails on a file it cannot read", "[console][import]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    CHECK_FALSE(LC_Console::importGraphic(writeFile(dir, "garbage.dxf", "this is not a drawing\n")));

    CHECK_FALSE(LC_Console::importGraphic(dir.filePath("missing.dxf")));
}
