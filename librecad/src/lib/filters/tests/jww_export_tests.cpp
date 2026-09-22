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
 * JWW is an import-only format.
 *
 * RS_FilterJWW used to report canExport(FormatJWW) == true although its
 * writer never produced a file: DL_Jww::out() returned NULL (since late 2011;
 * before that it had no return statement at all), so fileExport() gave up
 * before writing a byte, and every DL_Jww::write* was an empty stub.  The writer was also the wrong
 * kind: a dxflib DXF writer, so completing it would have put DXF text into a
 * .jww file, which a JWW reader rejects (a real .jww is a binary archive
 * that starts with "JwwData.").
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "rs.h"
#include "rs_circle.h"
#include "rs_fileio.h"
#include "rs_filterjww.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_spline.h"

namespace {

// RS_Graphic's ctor reads RS_Settings, which needs a QCoreApplication.
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

// A line, a circle and a spline: the drawing from the original report.
void addSampleEntities(RS_Graphic& graphic) {
    graphic.addEntity(new RS_Line(&graphic, RS_LineData({0., 0.}, {10., 5.})));
    graphic.addEntity(new RS_Circle(&graphic, RS_CircleData({20., 0.}, 4.)));
    RS_SplineData splineData(3, false);
    splineData.controlPoints = {{0., 10.}, {5., 20.}, {10., 5.}, {15., 15.}, {20., 10.}};
    graphic.addEntity(new RS_Spline(&graphic, splineData));
}

std::filesystem::path scratchDir() {
    auto dir = std::filesystem::temp_directory_path() / "librecad_jww_export_tests";
    std::filesystem::create_directories(dir);
    return dir;
}

std::string readBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("RS_FilterJWW imports JWW but does not claim to export it", "[jww][filter]") {
    const RS_FilterJWW filter;
    CHECK(filter.canImport("drawing.jww", RS2::FormatJWW));
    CHECK_FALSE(filter.canExport("drawing.jww", RS2::FormatJWW));
    CHECK_FALSE(filter.canExport("drawing.jww", RS2::FormatJWC));
}

TEST_CASE("RS_FileIO has an import filter but no export filter for JWW", "[jww][fileio]") {
    CHECK(RS_FileIO::instance()->getImportFilter("drawing.jww", RS2::FormatJWW) != nullptr);
    CHECK(RS_FileIO::instance()->getExportFilter("drawing.jww", RS2::FormatJWW) == nullptr);
}

TEST_CASE("Exporting a drawing as JWW fails without creating a file", "[jww][fileio]") {
    ensureQtContext();
    RS_Graphic graphic;
    addSampleEntities(graphic);
    REQUIRE(graphic.count() == 3);

    const auto path = scratchDir() / "export.jww";
    std::filesystem::remove(path);
    const QString file = QString::fromStdString(path.string());

    CHECK_FALSE(RS_FileIO::instance()->fileExport(graphic, file, RS2::FormatJWW));
    CHECK_FALSE(std::filesystem::exists(path));

    RS_FilterJWW filter;
    CHECK_FALSE(filter.fileExport(graphic, file, RS2::FormatJWW));
    CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("A refused JWW export leaves an existing .jww untouched", "[jww][fileio]") {
    ensureQtContext();
    RS_Graphic graphic;
    addSampleEntities(graphic);

    const auto path = scratchDir() / "existing.jww";
    const std::string original = std::string("JwwData.") + std::string(24, '\x5a');
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << original;
    }
    const QString file = QString::fromStdString(path.string());

    CHECK_FALSE(RS_FileIO::instance()->fileExport(graphic, file, RS2::FormatJWW));
    CHECK(readBytes(path) == original);

    RS_FilterJWW filter;
    CHECK_FALSE(filter.fileExport(graphic, file, RS2::FormatJWW));
    CHECK(readBytes(path) == original);

    std::filesystem::remove(path);
}
