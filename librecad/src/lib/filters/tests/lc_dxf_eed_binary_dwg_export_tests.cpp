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

// A DXF reader keeps a binary chunk of XDATA (group 1004) as the hex text of
// the file, and the drawing keeps what the reader gave. The DWG writer took
// the bytes of a chunk only, so a drawing opened from a DXF whose entities or
// table records had such XDATA could not be saved as DWG at all: no file was
// written.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QString>

#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_point.h"
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
            ("lc_dxf_eed_binary_dwg_export_" + name))
        .string();
}

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDwgFormats[] = {
    {RS2::FormatDWG, "dwg-2000"},          {RS2::FormatDWG2004, "dwg-2004"},
    {RS2::FormatDWG2007, "dwg-2007"},      {RS2::FormatDWG2010, "dwg-2010"},
    {RS2::FormatDWG2013, "dwg-2013"},      {RS2::FormatDWG2018, "dwg-2018"}};

bool exportTo(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileExport(graphic, QString::fromStdString(path), type);
}

bool importFrom(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, QString::fromStdString(path), type);
}

// The one point the drawings of the fixture hold.
const RS_Point* thePoint(RS_Graphic& graphic) {
    const RS_Point* point = nullptr;
    std::size_t entities = 0;
    for (RS_Entity* e : graphic) {
        ++entities;
        if (e != nullptr && e->rtti() == RS2::EntityPoint)
            point = static_cast<const RS_Point*>(e);
    }
    CHECK(entities == 1);
    return point;
}

void checkText(const DRW_Variant* value, int code, const std::string& text) {
    REQUIRE(value != nullptr);
    CHECK(value->code() == code);
    REQUIRE(value->type() == DRW_Variant::STRING);
    CHECK(std::string(value->c_str()) == text);
}

void checkBytes(const DRW_Variant* value, const std::vector<std::uint8_t>& bytes) {
    REQUIRE(value != nullptr);
    CHECK(value->code() == 1004);
    REQUIRE(value->type() == DRW_Variant::BINARY);
    CHECK(*value->binary() == bytes);
}

// A layer with binary XDATA, as a program that writes handles declares it.
const char* const kLayerDxf =
    "0\nSECTION\n2\nHEADER\n"
    "9\n$ACADVER\n1\nAC1015\n"
    "9\n$HANDSEED\n5\nFFFF\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n2\n"
    "0\nLAYER\n5\n10\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n40\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\nHEX_LAYER\n70\n0\n62\n1\n6\nCONTINUOUS\n"
    "1001\nLAYER_EED\n1000\nlayer payload\n1004\n0102\n1004\nA0B1C2\n"
    "0\nENDTAB\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n41\n100\nAcDbEntity\n8\nHEX_LAYER\n100\nAcDbLine\n"
    "10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n5.0\n31\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("binary XDATA of an entity read from a DXF is saved as DWG",
          "[dxf][dwg][filter][eed]") {
    ensureSettings();
    const std::string src = std::string(LIBRECAD_TEST_DIR) + "/dxf/eed_binary_r2007.dxf";
    REQUIRE(std::filesystem::is_regular_file(src));
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dwg.m_name);
    const std::string out = tempPath(std::string(dwg.m_name) + ".dwg");
    std::filesystem::remove(out);

    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, src, RS2::FormatDXFRW));
    CHECK(exportTo(drawing, out, dwg.m_type));
    REQUIRE(std::filesystem::is_regular_file(out));

    // The drawing keeps the chunks as the DXF reader gave them.
    const RS_Point* source = thePoint(drawing);
    REQUIRE(source != nullptr);
    REQUIRE(source->getDrwExtData().size() == 4);
    checkText(source->getDrwExtData()[2].get(), 1004, "0102");
    checkText(source->getDrwExtData()[3].get(), 1004, "A0B1C2");

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    const RS_Point* point = thePoint(fromDwg);
    REQUIRE(point != nullptr);
    CHECK(point->getPos().distanceTo(RS_Vector(12.5, -3.25)) < 1e-9);
    const auto& extData = point->getDrwExtData();
    REQUIRE(extData.size() == 4);
    checkText(extData[0].get(), 1001, "LIBRECAD_EED");
    checkText(extData[1].get(), 1000, "binary items stay distinct");
    checkBytes(extData[2].get(), {0x01, 0x02});
    checkBytes(extData[3].get(), {0xA0, 0xB1, 0xC2});

    std::filesystem::remove(out);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("binary XDATA of a layer read from a DXF is saved as DWG",
          "[dxf][dwg][filter][eed][layer]") {
    ensureSettings();
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dwg.m_name);
    const std::string src = tempPath("layer.dxf");
    const std::string out = tempPath(std::string("layer_") + dwg.m_name + ".dwg");
    std::ofstream(src, std::ios::binary) << kLayerDxf;
    std::filesystem::remove(out);

    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, src, RS2::FormatDXFRW));
    REQUIRE(drawing.dwgAdvancedMetadata().findLayerTableEntryByName("HEX_LAYER")
            != nullptr);
    CHECK(exportTo(drawing, out, dwg.m_type));
    REQUIRE(std::filesystem::is_regular_file(out));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    const DRW_Layer* layer =
        fromDwg.dwgAdvancedMetadata().findLayerTableEntryByName("HEX_LAYER");
    REQUIRE(layer != nullptr);
    REQUIRE(layer->extData.size() == 4);
    checkText(layer->extData[0], 1001, "LAYER_EED");
    checkText(layer->extData[1], 1000, "layer payload");
    checkBytes(layer->extData[2], {0x01, 0x02});
    checkBytes(layer->extData[3], {0xA0, 0xB1, 0xC2});

    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
