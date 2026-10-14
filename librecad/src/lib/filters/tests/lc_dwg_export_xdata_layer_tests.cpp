/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

// Application data (XDATA) may name a layer (group 1003). In a DWG the item
// holds the layer's handle, so a layer the drawing does not have cannot be
// written. The DWG export used to refuse the whole drawing over one such
// item, and said only that a PLOTSETTINGS object failed. It now leaves that
// application's data off the object, writes the rest, and says so in the
// export report.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <QString>

#include "lc_filtertestsupport.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

std::string tempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("lc_dwg_export_xdata_layer_" + name))
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

// A line with the data of two applications. LC_KEPT names the layer
// KEPT_LAYER of the drawing, LC_LOST a layer the drawing does not have.
const char kDxf[] =
    "0\nSECTION\n2\nHEADER\n"
    "9\n$ACADVER\n1\nAC1015\n"
    "9\n$HANDSEED\n5\nFFFF\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n2\n"
    "0\nLAYER\n5\n10\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n11\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\nKEPT_LAYER\n70\n0\n62\n1\n6\nCONTINUOUS\n"
    "0\nENDTAB\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
    "10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n5.0\n31\n0.0\n"
    "1001\nLC_KEPT\n1002\n{\n1003\nKEPT_LAYER\n1000\nkept\n1002\n}\n"
    "1001\nLC_LOST\n1002\n{\n1003\nNO_SUCH_LAYER\n1000\nlost\n1002\n}\n"
    "0\nENDSEC\n0\nEOF\n";

bool importFrom(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, QString::fromStdString(path), type);
}

const RS_Line* theLine(RS_Graphic& graphic) {
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityLine)
            return static_cast<const RS_Line*>(e);
    }
    return nullptr;
}

// The string values of an entity's application data.
std::vector<std::string> strings(const RS_Entity& entity) {
    std::vector<std::string> values;
    if (!entity.hasDrwExtData())
        return values;
    for (const auto& value : entity.getDrwExtData()) {
        if (value != nullptr && value->type() == DRW_Variant::STRING)
            values.emplace_back(value->c_str());
    }
    return values;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG export leaves out XDATA that names a missing layer and reports it",
          "[dwg][dxf][filter][xdata]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dwg.m_name);

    const std::string src = tempPath("source.dxf");
    const std::string out = tempPath(std::string(dwg.m_name) + ".dwg");
    std::ofstream(src, std::ios::binary) << kDxf;

    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, src, RS2::FormatDXFRW));
    const RS_Line* line = theLine(drawing);
    REQUIRE(line != nullptr);
    REQUIRE(strings(*line)
            == std::vector<std::string>{"LC_KEPT", "{", "KEPT_LAYER", "kept", "}",
                                        "LC_LOST", "{", "NO_SUCH_LAYER", "lost",
                                        "}"});

    std::filesystem::remove(out);
    RS_FilterDXFRW exporter;
    REQUIRE(exporter.fileExport(drawing, QString::fromStdString(out), dwg.m_type));
    CHECK(exporter.exportReport().contains(
        QStringLiteral("1 × application data naming a layer not in the drawing")));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    const RS_Line* reread = theLine(fromDwg);
    REQUIRE(reread != nullptr);
    CHECK(strings(*reread)
          == std::vector<std::string>{"LC_KEPT", "{", "KEPT_LAYER", "kept", "}"});

    // Nothing is left out of a drawing whose data names its own layers.
    CHECK(exporter.fileExport(fromDwg, QString::fromStdString(out), dwg.m_type));
    CHECK(exporter.exportReport().isEmpty());

    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
