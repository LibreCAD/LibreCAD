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

// A DXF names the application of each XDATA group (group 1001) by a record of
// its APPID table, and a reader looks the name up there: AutoCAD refuses a
// file whose XDATA names an application the table lacks, and ODA File
// Converter stops with "Invalid RegApp". The DXF export registered its own
// few applications and those of the file the drawing was read from, but it
// writes XDATA under other names too: what the filter keeps of a polyline
// read with a width (LibreCAD_LWPOLYLINE), XDATA that LibreCAD attaches
// itself (LIBRECAD_OFFSET on a spline), and XDATA a file held without
// registering its application, as files of earlier LibreCAD versions do.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/dwgbufferw.h"
#include "lc_dimstyle.h"
#include "lc_dimstyleslist.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_filtertestsupport.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDxfFormats[] = {
    {RS2::FormatDXFRW12, "dxf-r12"},       {RS2::FormatDXFRW14, "dxf-r14"},
    {RS2::FormatDXFRW2000, "dxf-2000"},    {RS2::FormatDXFRW2004, "dxf-2004"},
    {RS2::FormatDXFRW, "dxf-2007"},        {RS2::FormatDXFRW2010, "dxf-2010"},
    {RS2::FormatDXFRW2013, "dxf-2013"},    {RS2::FormatDXFRW2018, "dxf-2018"}};

// APPID names compare without regard to case.
std::string upper(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return text;
}

// The names of the APPID records of a written DXF, with how often each comes.
std::map<std::string, int> registeredApplications(const std::filesystem::path& file) {
    std::map<std::string, int> names;
    for (const lc::test::DxfRecord& record : lc::test::dxfRecords(file, "APPID")) {
        for (const std::string& name : record.own(2)) {
            ++names[upper(name)];
        }
    }
    return names;
}

// The applications the XDATA of the records of a written DXF names.
std::set<std::string> xdataApplications(const std::filesystem::path& file) {
    std::set<std::string> names;
    for (const lc::test::DxfRecord& record : lc::test::dxfRecords(file)) {
        for (const lc::test::DxfGroup& group : record.m_groups) {
            if (group.m_code == 1001) {
                names.insert(upper(group.m_value));
            }
        }
    }
    return names;
}

// Every application the XDATA of the file names has exactly one APPID record,
// and no APPID record is there twice.
void checkEveryApplicationRegisteredOnce(const std::filesystem::path& file) {
    const std::map<std::string, int> registered = registeredApplications(file);
    for (const std::string& application : xdataApplications(file)) {
        CAPTURE(application);
        CHECK(registered.count(application) == 1);
    }
    for (const auto& [name, count] : registered) {
        CAPTURE(name);
        CHECK(count == 1);
    }
}

std::vector<std::shared_ptr<DRW_Variant>> xdata(const std::string& application, const std::string& value) {
    return {std::make_shared<DRW_Variant>(1001, application), std::make_shared<DRW_Variant>(1000, value)};
}

RS_Line* lineWithXData(RS_EntityContainer* parent, const double y,
                       std::vector<std::shared_ptr<DRW_Variant>> extData) {
    auto* line = new RS_Line(parent, RS_LineData(RS_Vector(0.0, y), RS_Vector(10.0, y)));
    line->setDrwExtData(std::move(extData));
    parent->addEntity(line);
    return line;
}

// A polyline with a width of 0.5; the width is kept as LibreCAD_LWPOLYLINE
// XDATA, which an APPID table may name in another case.
std::string drawingWithWidePolyline(const std::string& appIdTable,
                                    const std::string& extData =
                                        "1001\nLC_IMPORTED_POLYLINE\n1000\nkept with the width\n",
                                    bool lightweight = true) {
    const std::string polyline = lightweight
        ? "0\nLWPOLYLINE\n5\n100\n330\n1F\n100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n"
          "90\n3\n70\n0\n43\n0.5\n"
          "10\n0.0\n20\n0.0\n10\n10.0\n20\n0.0\n10\n10.0\n20\n5.0\n" + extData
        : "0\nPOLYLINE\n5\n100\n100\nAcDbEntity\n8\n0\n100\nAcDb2dPolyline\n"
          "66\n1\n70\n0\n10\n0\n20\n0\n30\n0\n40\n0.5\n41\n0.75\n" + extData +
          "0\nVERTEX\n8\n0\n10\n0\n20\n0\n30\n0\n"
          "0\nVERTEX\n8\n0\n10\n10\n20\n0\n30\n0\n"
          "0\nVERTEX\n8\n0\n10\n10\n20\n5\n30\n0\n0\nSEQEND\n8\n0\n";
    return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
           "0\nSECTION\n2\nTABLES\n" + appIdTable + "0\nENDSEC\n"
           "0\nSECTION\n2\nENTITIES\n" + polyline +
           "0\nENDSEC\n0\nEOF\n";
}

std::vector<lc::test::DxfGroup> applicationData(const lc::test::DxfRecord& record,
                                               const std::string& application) {
    std::vector<lc::test::DxfGroup> result;
    bool inGroup = false;
    for (const auto& group : record.m_groups) {
        if (group.m_code == 1001) {
            inGroup = upper(group.m_value) == upper(application);
        } else if (inGroup) {
            result.push_back(group);
        }
    }
    return result;
}

// R2000 and R2004 keep the line types of a dimension style in XDATA of
// ACAD_DSTYLE_DIM_LINETYPE and its two siblings. The file registers no
// application.
const char* const kDimStyleLineTypesR2000 =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n5\n5\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nLTYPE\n5\n40\n330\n5\n100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
    "2\nDASHED_XDIM\n70\n0\n3\nDashed\n72\n65\n73\n2\n40\n40.0\n49\n20.0\n74\n0\n49\n-20.0\n74\n0\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nLAYER\n5\n50\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n"
    "0\nDIMSTYLE\n105\n31\n330\nA\n100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
    "2\nXDIM\n70\n0\n"
    "1001\nACAD_DSTYLE_DIM_LINETYPE\n1070\n380\n1005\n40\n"
    "1001\nACAD_DSTYLE_DIM_EXT1_LINETYPE\n1070\n381\n1005\n40\n"
    "1001\nACAD_DSTYLE_DIM_EXT2_LINETYPE\n1070\n382\n1005\n40\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";

bool hasXDataApplication(const RS_Entity& entity, const std::string& application) {
    if (!entity.hasDrwExtData()) {
        return false;
    }
    for (const auto& value : entity.getDrwExtData()) {
        if (value != nullptr && value->code() == 1001 && value->type() == DRW_Variant::STRING &&
            value->content.s != nullptr && upper(*value->content.s) == upper(application)) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("the DXF export registers the application of every XDATA it writes, once",
          "[dxf][filter][xdata][appid]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    CAPTURE(dxf.m_name);

    RS_Graphic graphic;
    graphic.initForNewDocument();

    // A layer whose record was read with XDATA of an application the file
    // did not register.
    graphic.addLayer(new RS_Layer(QStringLiteral("WALLS")));
    {
        DRW_Layer source;
        source.name = "WALLS";
        REQUIRE(source.addExtData(std::make_unique<DRW_Variant>(1001, std::string("LC_TEST_LAYER_APP"))));
        REQUIRE(source.addExtData(std::make_unique<DRW_Variant>(1000, std::string("on a layer"))));
        graphic.dwgAdvancedMetadata().addLayerTableEntry(source);
    }

    // Entities of the model space: a new application, the same one in
    // another case, and the case of two the export always registers.
    lineWithXData(&graphic, 0.0, xdata("LIBRECAD_OFFSET", "offset"));
    lineWithXData(&graphic, 1.0, xdata("LibreCAD_Offset", "same application"));
    lineWithXData(&graphic, 2.0, xdata("librecad", "LibreCad, in other letters"));
    lineWithXData(&graphic, 3.0, xdata("acad", "ACAD, in other letters"));

    // An entity of a block, and an INSERT of the block.
    auto* block = new RS_Block(&graphic, RS_BlockData(QStringLiteral("PART"), RS_Vector(0.0, 0.0), false));
    lineWithXData(block, 0.0, xdata("LC_TEST_BLOCK_APP", "in a block"));
    graphic.addBlock(block);
    auto* insert = new RS_Insert(&graphic, RS_InsertData(QStringLiteral("PART"), RS_Vector(20.0, 0.0),
                                                         RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0, 0.0)));
    graphic.addEntity(insert);
    insert->update();

    const lc::test::OutputDirectory directory("lc_dxf_xdata_appid_export");
    const std::filesystem::path out = directory.file(std::string(dxf.m_name) + ".dxf");
    REQUIRE(lc::test::exportDrawing(graphic, out, dxf.m_type));

    // The XDATA is in the file...
    const std::set<std::string> written = xdataApplications(out);
    for (const char* application : {"LIBRECAD_OFFSET", "LC_TEST_BLOCK_APP", "LC_TEST_LAYER_APP"}) {
        CAPTURE(application);
        CHECK(written.count(application) == 1);
    }
    // ...and each application it names has a record of its own.
    checkEveryApplicationRegisteredOnce(out);
    const std::map<std::string, int> registered = registeredApplications(out);
    CHECK(registered.count("LIBRECAD_OFFSET") == 1);
    CHECK(registered.count("LC_TEST_BLOCK_APP") == 1);
    CHECK(registered.count("LC_TEST_LAYER_APP") == 1);
    CHECK(registered.count("LIBRECAD") == 1);
    CHECK(registered.count("ACAD") == 1);
}

TEST_CASE("a polyline read with a width is written with LibreCAD_LWPOLYLINE registered",
          "[dxf][filter][xdata][appid][lwpolyline]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    struct Source {
        const char* m_label;
        const char* m_appIdTable;
    };
    const Source source = GENERATE(
        Source{"a file that registers nothing", ""},
        Source{"a file that registers the name in capitals",
               "0\nTABLE\n2\nAPPID\n5\n9\n70\n1\n"
               "0\nAPPID\n5\n12\n330\n9\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n"
               "2\nLIBRECAD_LWPOLYLINE\n70\n0\n0\nENDTAB\n"});
    CAPTURE(source.m_label);
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    CAPTURE(dxf.m_name);

    const lc::test::OutputDirectory directory("lc_dxf_xdata_appid_lwpolyline");
    const std::filesystem::path in = directory.file("in.dxf");
    std::ofstream(in, std::ios::binary) << drawingWithWidePolyline(source.m_appIdTable);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    REQUIRE(lc::test::importDrawing(graphic, in, RS2::FormatDXFRW));
    std::size_t widePolylines = 0;
    for (const RS_Entity* entity : graphic) {
        if (entity != nullptr && entity->rtti() == RS2::EntityPolyline &&
            hasXDataApplication(*entity, "LibreCAD_LWPOLYLINE")) {
            ++widePolylines;
        }
    }
    REQUIRE(widePolylines == 1);

    const std::filesystem::path out = directory.file(std::string(dxf.m_name) + ".dxf");
    REQUIRE(lc::test::exportDrawing(graphic, out, dxf.m_type));

    CHECK(xdataApplications(out).count("LIBRECAD_LWPOLYLINE") == 1);
    CHECK(xdataApplications(out).count("LC_IMPORTED_POLYLINE") == 1);
    checkEveryApplicationRegisteredOnce(out);
    CHECK(registeredApplications(out).count("LIBRECAD_LWPOLYLINE") == 1);
    CHECK(registeredApplications(out).count("LC_IMPORTED_POLYLINE") == 1);
}

TEST_CASE("wide polylines keep imported XDATA through repeated DXF and DWG saves",
          "[dxf][dwg][filter][xdata][appid][lwpolyline][polyline][regression]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    const bool lightweight = GENERATE(false, true);
    const std::string oldMarker = GENERATE(std::string(), std::string("LibreCAD_LWPOLYLINE"),
                                           std::string("librecad_lwpolyline"));
    const Format format = GENERATE(
        Format{RS2::FormatDXFRW12, "dxf-r12"}, Format{RS2::FormatDXFRW14, "dxf-r14"},
        Format{RS2::FormatDXFRW2000, "dxf-2000"}, Format{RS2::FormatDXFRW2004, "dxf-2004"},
        Format{RS2::FormatDXFRW, "dxf-2007"}, Format{RS2::FormatDXFRW2010, "dxf-2010"},
        Format{RS2::FormatDXFRW2013, "dxf-2013"}, Format{RS2::FormatDXFRW2018, "dxf-2018"},
        Format{RS2::FormatDWG, "dwg-2000"}, Format{RS2::FormatDWG2004, "dwg-2004"},
        Format{RS2::FormatDWG2007, "dwg-2007"}, Format{RS2::FormatDWG2010, "dwg-2010"},
        Format{RS2::FormatDWG2013, "dwg-2013"}, Format{RS2::FormatDWG2018, "dwg-2018"});
    CAPTURE(lightweight, oldMarker, format.m_name);
    const bool dwg = std::string(format.m_name).rfind("dwg-", 0) == 0;
    std::string extData =
        "1001\nLC_TEST_POLYLINE_APP\n1000\nkeep this text\n1002\n{\n1002\n{\n"
        "1004\n007FFF\n1010\n1\n1020\n2\n1030\n3\n1040\n4.5\n"
        "1070\n12\n1071\n1234567\n1002\n}\n1002\n}\n";
    if (!oldMarker.empty()) {
        extData += "1001\n" + oldMarker + "\n1040\n999\n1000\nstale metadata\n";
    }
    extData += "1001\nLibreCAD_LWPOLYLINE_USER\n1000\nkeep this group too\n1070\n42\n";

    const lc::test::OutputDirectory directory("lc_polyline_imported_xdata");
    const auto in = directory.file("in.dxf");
    std::ofstream(in, std::ios::binary) << drawingWithWidePolyline("", extData, lightweight);
    const auto source = lc::test::dxfRecords(in, lightweight ? "LWPOLYLINE" : "POLYLINE");
    REQUIRE(source.size() == 1);
    auto graphic = std::make_unique<RS_Graphic>();
    REQUIRE(lc::test::importDrawing(*graphic, in, RS2::FormatDXFRW));

    for (int pass = 0; pass < 3; ++pass) {
        CAPTURE(pass);
        graphic->onLoadingCompleted();
        REQUIRE(graphic->count() == 1);
        REQUIRE(graphic->entityAt(0)->rtti() == RS2::EntityPolyline);
        std::vector<std::string> applications;
        for (const auto& value : graphic->entityAt(0)->getDrwExtData()) {
            if (value && value->code() == 1001 && value->type() == DRW_Variant::STRING && value->content.s) {
                applications.push_back(upper(*value->content.s));
            }
        }
        for (const char* application :
             {"LC_TEST_POLYLINE_APP", "LIBRECAD_LWPOLYLINE_USER", "LIBRECAD_LWPOLYLINE"}) {
            CAPTURE(application);
            CHECK(std::count(applications.begin(), applications.end(), application) == 1);
        }
        if (pass == 2)
            break;
        const auto out = directory.file("pass" + std::to_string(pass) + (dwg ? ".dwg" : ".dxf"));
        REQUIRE(lc::test::exportDrawing(*graphic, out, format.m_type));
        if (!dwg)
            checkEveryApplicationRegisteredOnce(out);
        auto reopened = std::make_unique<RS_Graphic>();
        REQUIRE(lc::test::importDrawing(*reopened, out, format.m_type));
        graphic = std::move(reopened);
    }

    const auto out = directory.file("final.dxf");
    REQUIRE(lc::test::exportDrawing(*graphic, out, RS2::FormatDXFRW2018));
    checkEveryApplicationRegisteredOnce(out);
    const auto polylines = lc::test::dxfRecords(out, "LWPOLYLINE");
    REQUIRE(polylines.size() == 1);
    REQUIRE(polylines.front().own(1001).size() == 3);
    for (const char* application : {"LC_TEST_POLYLINE_APP", "LibreCAD_LWPOLYLINE_USER"}) {
        CAPTURE(application);
        const auto expected = applicationData(source.front(), application);
        const auto actual = applicationData(polylines.front(), application);
        REQUIRE(actual.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(actual[i].m_code == expected[i].m_code);
            CHECK(lc::test::sameValues({expected[i].m_value}, {actual[i].m_value}));
        }
    }
    if (lightweight) {
        CHECK(lc::test::sameValues({"0.5"}, polylines.front().own(43)));
    } else {
        CHECK(lc::test::sameValues({"0.5", "0.5", "0.5"}, polylines.front().own(40)));
        CHECK(lc::test::sameValues({"0.75", "0.75", "0.75"}, polylines.front().own(41)));
    }
}

TEST_CASE("DWG EED accepts checked hexadecimal XDATA binary chunks",
          "[dwg][filter][eed][xdata][regression]") {
    const auto version = GENERATE(DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                   DRW::AC1024, DRW::AC1027, DRW::AC1032);
    const std::string hex = GENERATE(std::string("007FFF"), std::string("007fFf"),
                                     std::string(), std::string("0"), std::string("0G"),
                                     std::string(510, '0'), std::string(512, '0'));
    CAPTURE(version, hex);
    DRW_Variant app(1001, std::string("LC_TEST_POLYLINE_APP"));
    DRW_Variant chunk(1004, hex);
    dwgBufferW encoded;
    const bool valid = DRW_Entity::encodeDwgEed(version, {&app, &chunk}, {{0, 0x30}}, {}, 1252, encoded);
    if (hex == "0" || hex == "0G" || hex.size() > 510) {
        CHECK_FALSE(valid);
    } else {
        REQUIRE(valid);
        const std::vector<std::uint8_t> bytes = hex.size() == 510
            ? std::vector<std::uint8_t>(255, 0)
            : hex.empty() ? std::vector<std::uint8_t>{} : std::vector<std::uint8_t>{0, 0x7F, 0xFF};
        DRW_Variant binary(1004, bytes);
        dwgBufferW expected;
        REQUIRE(DRW_Entity::encodeDwgEed(version, {&app, &binary}, {{0, 0x30}}, {}, 1252, expected));
        CHECK(encoded.data() == expected.data());
    }
    DRW_Variant wrongType(1004, std::int32_t{7});
    dwgBufferW invalid;
    CHECK_FALSE(DRW_Entity::encodeDwgEed(version, {&app, &wrongType}, {{0, 0x30}}, {}, 1252, invalid));
}

TEST_CASE("the line types of a dimension style in R2000 and R2004 XDATA are registered",
          "[dxf][filter][xdata][appid][dimstyle]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    // A style keeps the XDATA of the record it was read from; a renamed one
    // has no record, and the export makes that XDATA itself.
    const bool renamed = GENERATE(false, true);
    CAPTURE(renamed);
    const Format dxf = GENERATE(Format{RS2::FormatDXFRW2000, "dxf-2000"}, Format{RS2::FormatDXFRW2004, "dxf-2004"});
    CAPTURE(dxf.m_name);

    const lc::test::OutputDirectory directory("lc_dxf_xdata_appid_dimstyle");
    const std::filesystem::path in = directory.file("in.dxf");
    std::ofstream(in, std::ios::binary) << kDimStyleLineTypesR2000;

    RS_Graphic graphic;
    graphic.initForNewDocument();
    REQUIRE(lc::test::importDrawing(graphic, in, RS2::FormatDXFRW));
    LC_DimStyle* style = graphic.getDimStyleList()->findByName(QStringLiteral("XDIM"));
    REQUIRE(style != nullptr);
    REQUIRE(style->dimensionLine()->lineTypeName() == QStringLiteral("DASHED_XDIM"));
    if (renamed) {
        style->setName(QStringLiteral("XDIM_RENAMED"));
    }

    const std::filesystem::path out = directory.file(std::string(dxf.m_name) + ".dxf");
    REQUIRE(lc::test::exportDrawing(graphic, out, dxf.m_type));

    const std::set<std::string> written = xdataApplications(out);
    for (const char* application :
         {"ACAD_DSTYLE_DIM_LINETYPE", "ACAD_DSTYLE_DIM_EXT1_LINETYPE", "ACAD_DSTYLE_DIM_EXT2_LINETYPE"}) {
        CAPTURE(application);
        CHECK(written.count(application) == 1);
    }
    checkEveryApplicationRegisteredOnce(out);
}
