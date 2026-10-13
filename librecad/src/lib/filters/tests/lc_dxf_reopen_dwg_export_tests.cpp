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

// A DXF with handles declares its active viewport: a VPORT record named
// "*Active" that has a handle of the file. The DWG writer writes an "*Active"
// record of its own and accepts another one in the drawing only when a reader
// delivered it. Only the DWG reader said so, and the writer took one such
// record: no drawing opened from a DXF with handles, LibreCAD's own included,
// and no drawing with tiled viewports could be saved as DWG.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
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

std::string tempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("lc_dxf_reopen_dwg_export_" + name))
        .string();
}

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDxfFormats[] = {
    {RS2::FormatDXFRW12, "dxf-r12"},       {RS2::FormatDXFRW14, "dxf-r14"},
    {RS2::FormatDXFRW2000, "dxf-2000"},    {RS2::FormatDXFRW2004, "dxf-2004"},
    {RS2::FormatDXFRW, "dxf-2007"},        {RS2::FormatDXFRW2010, "dxf-2010"},
    {RS2::FormatDXFRW2013, "dxf-2013"},    {RS2::FormatDXFRW2018, "dxf-2018"}};

const Format kDwgFormats[] = {
    {RS2::FormatDWG, "dwg-2000"},          {RS2::FormatDWG2004, "dwg-2004"},
    {RS2::FormatDWG2010, "dwg-2010"},      {RS2::FormatDWG2013, "dwg-2013"},
    {RS2::FormatDWG2018, "dwg-2018"}};

bool exportTo(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileExport(graphic, QString::fromStdString(path), type);
}

bool importFrom(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, QString::fromStdString(path), type);
}

std::size_t countActiveVports(const RS_Graphic& graphic) {
    const auto& vports = graphic.dwgAdvancedMetadata().vports();
    return static_cast<std::size_t>(
        std::count_if(vports.cbegin(), vports.cend(), [](const auto& record) {
            std::string name = record.name;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            return name == "*ACTIVE";
        }));
}

// The one line the drawings of this file hold, read back from a saved file.
void checkTheLine(RS_Graphic& graphic) {
    const RS_Line* line = nullptr;
    std::size_t entities = 0;
    for (RS_Entity* e : graphic) {
        ++entities;
        if (e != nullptr && e->rtti() == RS2::EntityLine)
            line = static_cast<const RS_Line*>(e);
    }
    CHECK(entities == 1);
    REQUIRE(line != nullptr);
    CHECK(line->getStartpoint().distanceTo(RS_Vector(0., 0.)) < 1e-9);
    CHECK(line->getEndpoint().distanceTo(RS_Vector(10., 5.)) < 1e-9);
}

// A VPORT record as a program that writes handles declares it.
std::string viewportRecord(const std::string& handle, const std::string& name) {
    return "0\nVPORT\n5\n" + handle +
           "\n330\n8\n100\nAcDbSymbolTableRecord\n"
           "100\nAcDbViewportTableRecord\n2\n" + name +
           "\n70\n0\n"
           "10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n12\n5.0\n22\n2.5\n40\n10.0\n41\n2.0\n";
}

// The line of this file in a drawing that has the VPORT records given.
std::string dxfWithViewports(const std::string& records) {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1015\n"
        "9\n$HANDSEED\n5\nFFFF\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nVPORT\n5\n8\n330\n0\n100\nAcDbSymbolTable\n70\n2\n" +
        records +
        "0\nENDTAB\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
        "0\nLAYER\n5\n10\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
        "10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n5.0\n31\n0.0\n"
        "0\nENDSEC\n0\nEOF\n";
}

} // namespace

TEST_CASE("a drawing saved as DXF and opened again is saved as DWG",
          "[dxf][dwg][filter][vport]") {
    ensureSettings();
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dxf.m_name, dwg.m_name);

    const std::string saved = tempPath(std::string(dxf.m_name) + ".dxf");
    const std::string out = tempPath(std::string(dxf.m_name) + "_" + dwg.m_name + ".dwg");
    {
        RS_Graphic drawing;
        drawing.initForNewDocument();
        drawing.addEntity(new RS_Line(&drawing, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
        REQUIRE(exportTo(drawing, saved, dxf.m_type));
        // The drawing that was never a file has no viewport record to trip over.
        REQUIRE(exportTo(drawing, out, dwg.m_type));
    }

    RS_Graphic reopened;
    reopened.initForNewDocument();
    REQUIRE(importFrom(reopened, saved, dxf.m_type));
    std::filesystem::remove(out);
    CHECK(exportTo(reopened, out, dwg.m_type));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    checkTheLine(fromDwg);
    CHECK(countActiveVports(fromDwg) == 1);

    std::filesystem::remove(saved);
    std::filesystem::remove(out);
}

TEST_CASE("a DXF that declares its viewports is saved as DWG",
          "[dxf][dwg][filter][vport]") {
    ensureSettings();
    struct Viewports {
        const char* m_label;
        std::string m_records;
        std::size_t m_read;    // VPORT records the DXF reader delivers
        std::size_t m_written; // VPORT records of the DWG
    };
    // AutoCAD writes one "*Active" record for each tiled viewport of the model
    // space; LibreCAD has one view, and writes one.
    const Viewports viewports = GENERATE(
        Viewports{"the active viewport", viewportRecord("29", "*Active"), 1, 1},
        Viewports{"two tiled viewports",
                  viewportRecord("29", "*Active") + viewportRecord("2A", "*Active"), 2, 1},
        Viewports{"the active viewport and a named one",
                  viewportRecord("29", "*Active") + viewportRecord("2A", "LEFT"), 2, 2});
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(viewports.m_label, dwg.m_name);

    const std::string src = tempPath("declared.dxf");
    const std::string out = tempPath(std::string("declared_") + dwg.m_name + ".dwg");
    std::ofstream(src, std::ios::binary) << dxfWithViewports(viewports.m_records);

    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, src, RS2::FormatDXFRW));
    // The records the writer has to tell from one it must refuse.
    REQUIRE(drawing.dwgAdvancedMetadata().vports().size() == viewports.m_read);
    REQUIRE(drawing.dwgAdvancedMetadata().vports().front().handle == 0x29u);
    std::filesystem::remove(out);
    CHECK(exportTo(drawing, out, dwg.m_type));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    checkTheLine(fromDwg);
    CHECK(countActiveVports(fromDwg) == 1);
    CHECK(fromDwg.dwgAdvancedMetadata().vports().size() == viewports.m_written);

    // What was read from the DWG is saved as DWG, as before.
    CHECK(exportTo(fromDwg, out, dwg.m_type));

    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
