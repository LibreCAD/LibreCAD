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

// A DXF SHAPE names a shape of a shape file in group 2. A DWG holds the number
// of the shape instead; its name is in the .shx file, which a program that
// converts the DWG usually does not have. LibreCAD therefore writes a SHAPE it
// read from a DWG without group 2, and LibreDWG and ACadSharp write an empty
// name. The DXF reader refused the whole file over such a SHAPE, so a DWG with
// a shape, saved as DXF, could not be opened again.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "drw_entities.h"
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
    return (std::filesystem::temp_directory_path() / ("lc_dxf_shape_name_" + name))
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

std::size_t countLines(RS_Graphic& graphic) {
    std::size_t lines = 0;
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityLine)
            ++lines;
    }
    return lines;
}

// A SHAPE between two lines; nameGroup is what the file has for group 2.
std::string drawingWithShape(const std::string& nameGroup) {
    return
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n100\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n5.0\n"
        "0\nSHAPE\n5\n101\n330\n1F\n100\nAcDbEntity\n8\n0\n100\nAcDbShape\n"
        "10\n15.25\n20\n-20.5\n30\n0.0\n40\n2.5\n" +
        nameGroup +
        "50\n0.0\n"
        "0\nLINE\n5\n102\n8\n0\n10\n4.0\n20\n5.0\n11\n6.0\n21\n7.0\n"
        "0\nENDSEC\n0\nEOF\n";
}

} // namespace

TEST_CASE("a DXF with a SHAPE that has no name is read",
          "[dxf][filter][shape]") {
    ensureSettings();
    struct NameGroup {
        const char* m_label;
        const char* m_group;
        const char* m_expected;
    };
    const NameGroup nameGroup = GENERATE(
        NameGroup{"no group 2, as LibreCAD writes it", "", ""},
        NameGroup{"an empty name, as LibreDWG and ACadSharp write it", "2\n\n", ""},
        NameGroup{"the name AutoCAD writes", "2\nMY-SHAPE\n", "MY-SHAPE"});
    CAPTURE(nameGroup.m_label);

    const std::string src = tempPath("read.dxf");
    std::ofstream(src, std::ios::binary) << drawingWithShape(nameGroup.m_group);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        CHECK(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    }
    std::filesystem::remove(src);

    // The entities around the SHAPE, and what the SHAPE does say.
    CHECK(countLines(graphic) == 2);
    const auto& shapes = graphic.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().handle == 0x101u);
    CHECK(shapes.front().styleName == nameGroup.m_expected);
    CHECK(shapes.front().insertionPoint.x == 15.25);
    CHECK(shapes.front().insertionPoint.y == -20.5);
    CHECK(shapes.front().scale == 2.5);
}

TEST_CASE("a SHAPE read from a DWG is saved as DXF and the DXF opens",
          "[dxf][filter][shape][roundtrip]") {
    ensureSettings();
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    CAPTURE(dxf.m_name);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    {
        // What the DWG reader delivers: the number of the shape and the STYLE
        // record of its shape file, a record that has no name.
        DRW_Shape shape;
        shape.handle = 0x889;
        shape.parentHandle = 0x1F;
        shape.m_shapeFileHandle = 0x887;
        shape.m_shapeIndex = 130;
        shape.m_insertionPoint = DRW_Coord(15.25, -20.5, 0.0);
        shape.m_scale = 2.5;
        graphic.dwgAdvancedMetadata().addShape(shape);
    }

    const std::string out = tempPath(std::string(dxf.m_name) + ".dxf");
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), dxf.m_type));
    }

    RS_Graphic reopened;
    reopened.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        CHECK(filter.fileImport(reopened, QString::fromStdString(out), dxf.m_type));
    }
    std::filesystem::remove(out);

    CHECK(countLines(reopened) == 1);
    const auto& shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().styleName.empty());
    CHECK(shapes.front().insertionPoint.x == 15.25);
    CHECK(shapes.front().insertionPoint.y == -20.5);
    CHECK(shapes.front().scale == 2.5);
}
