/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "lc_dimstyle.h"
#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
#include "libdxfrw.h"

namespace {

std::string normalizedName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return name;
}

using ApplicationRecords = std::map<std::string, lc::test::DxfRecord>;

ApplicationRecords checkRegisteredXData(const std::filesystem::path& file) {
    ApplicationRecords applications;
    std::set<std::string> handles;
    for (const auto& record : lc::test::dxfRecords(file, "APPID")) {
        const auto names = record.own(2);
        REQUIRE(names.size() == 1);
        CAPTURE(names.front());
        CHECK(applications.emplace(normalizedName(names.front()), record).second);
        const auto recordHandles = record.own(5);
        if (!recordHandles.empty()) {
            CHECK(recordHandles.front() != "0");
            CHECK(handles.insert(recordHandles.front()).second);
        }
    }
    std::size_t applicationUses = 0;
    for (const auto& record : lc::test::dxfRecords(file)) {
        for (const auto& group : record.m_groups) {
            if (group.m_code == 1001) {
                ++applicationUses;
                CAPTURE(record.type(), group.m_value);
                CHECK(applications.count(normalizedName(group.m_value)) == 1);
            }
        }
    }
    CHECK(applicationUses > 0);
    return applications;
}

void writeFixture(const std::filesystem::path& file, const std::string& content) {
    std::ofstream output(file);
    output << content;
    REQUIRE(output.good());
}

// The source contains native entities, with none of the LibreCAD markers.
// Import generates the sidecars that export subsequently writes as XDATA.
class MarkerFixture : public lc::test::NullDrwInterface {
public:
    static constexpr std::uint32_t underlayDefinitionHandle = 0xA10;
    dxfRW* m_writer = nullptr;
    bool m_ok = true;

    void writeEntities() override {
        DRW_Solid solid;
        solid.basePoint = {0, 0, 0};
        solid.secPoint = {3, 0, 0};
        solid.thirdPoint = {0, 2, 0};
        solid.fourPoint = solid.thirdPoint;
        m_ok = m_writer->writeSolid(&solid) && m_ok;

        DRW_LWPolyline polyline;
        polyline.width = 0.5;
        polyline.addVertex(DRW_Vertex2D(0, 0, 0));
        polyline.addVertex(DRW_Vertex2D(4, 0, 0));
        m_ok = m_writer->writeLWPolyline(&polyline) && m_ok;

        DRW_MLine mline;
        mline.numLines = 2;
        mline.numVerts = 2;
        mline.styleName = "STANDARD";
        for (int i = 0; i < 2; ++i) {
            DRW_MLineVertex vertex;
            vertex.position = {static_cast<double>(i) * 4, 0, 0};
            vertex.vertexDir = {1, 0, 0};
            vertex.miterDir = {0, 1, 0};
            vertex.segParms = {{0.0}, {0.0}};
            vertex.areaFillParms.resize(2);
            mline.vertlist.push_back(vertex);
        }
        m_ok = m_writer->writeMLine(&mline) && m_ok;

        DRW_Polyline polyface;
        polyface.flags = 64;
        polyface.vertexcount = 3;
        polyface.facecount = 1;
        for (const DRW_Coord& point : {DRW_Coord{0, 0, 0}, DRW_Coord{3, 0, 0}, DRW_Coord{0, 2, 0}}) {
            DRW_Vertex vertex(point.x, point.y, point.z, 0);
            vertex.flags = 192;
            polyface.addVertex(vertex);
        }
        DRW_Vertex face;
        face.flags = 128;
        face.vindex1 = 1;
        face.vindex2 = 2;
        face.vindex3 = 3;
        polyface.addVertex(face);
        m_ok = m_writer->writePolyline(&polyface) && m_ok;

        DRW_Polyline mesh;
        mesh.flags = 16;
        mesh.vertexcount = 2;
        mesh.facecount = 2;
        for (const DRW_Coord& point : {DRW_Coord{0, 0, 0}, DRW_Coord{3, 0, 0},
                                      DRW_Coord{0, 2, 0}, DRW_Coord{3, 2, 0}}) {
            DRW_Vertex vertex(point.x, point.y, point.z, 0);
            vertex.flags = 64;
            mesh.addVertex(vertex);
        }
        m_ok = m_writer->writePolyline(&mesh) && m_ok;

        DRW_Underlay underlay;
        underlay.definitionHandle = underlayDefinitionHandle;
        underlay.clipBoundary = {{0, 0, 0}, {3, 2, 0}};
        m_ok = m_writer->writeUnderlay(&underlay) && m_ok;
    }

    void writeObjects() override {
        DRW_UnderlayDefinition definition;
        definition.handle = underlayDefinitionHandle;
        definition.filename = "sheet.pdf";
        definition.sheetName = "1";
        m_ok = m_writer->writeUnderlayDefinition(&definition) && m_ok;
    }
};

} // namespace

TEST_CASE("DXF registers sidecar applications generated on import", "[dxf][appid-registration]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    lc::test::OutputDirectory output("lc_dxf_appid_generated");
    const auto source = output.file("native.dxf");
    MarkerFixture fixture;
    dxfRW writer(source.string().c_str());
    fixture.m_writer = &writer;
    REQUIRE(writer.reserveHandle(MarkerFixture::underlayDefinitionHandle));
    REQUIRE(writer.write(&fixture, DRW::AC1027, false));
    REQUIRE(fixture.m_ok);
    CHECK(lc::test::fileBytes(source).find("LibreCAD_") == std::string::npos);
    REQUIRE(lc::test::dxfRecords(source, "PDFUNDERLAY").size() == 1);
    CHECK(lc::test::dxfRecords(source, "PDFUNDERLAY").front().own(340) == std::vector<std::string>{"A10"});
    REQUIRE(lc::test::dxfRecords(source, "PDFDEFINITION").size() == 1);

    RS_Graphic drawing;
    REQUIRE(lc::test::importDrawing(drawing, source, RS2::FormatDXFRW));
    REQUIRE(drawing.count() >= 6);
    const auto exported = output.file("exported.dxf");
    REQUIRE(lc::test::exportDrawing(drawing, exported, RS2::FormatDXFRW2013));
    const auto applications = checkRegisteredXData(exported);
    CHECK(lc::test::dxfRecords(exported, "PDFUNDERLAY").size() == 1);
    CHECK(lc::test::dxfRecords(exported, "PDFDEFINITION").size() == 1);
    for (const char* name : {"LibreCAD_SOLID", "LibreCAD_LWPOLYLINE", "LibreCAD_MLINE",
                             "LibreCAD_POLYLINE_PFACE", "LibreCAD_POLYLINE_MESH", "LibreCAD_UNDERLAY"}) {
        CAPTURE(name);
        CHECK(applications.count(normalizedName(name)) == 1);
    }
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, exported, RS2::FormatDXFRW));
    const auto secondSave = output.file("second-save.dxf");
    REQUIRE(lc::test::exportDrawing(reopened, secondSave, RS2::FormatDXFRW2013));
    checkRegisteredXData(secondSave);
}

TEST_CASE("DXF registers nested and duplicate XDATA names using imported APPID identities",
          "[dxf][appid-registration]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    lc::test::OutputDirectory output("lc_dxf_appid_nested");
    const auto source = output.file("source.dxf");
    writeFixture(source,
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nAPPID\n70\n2\n"
        "0\nAPPID\n5\n900\n2\nImported.Custom\n70\n16\n71\n1\n"
        "0\nAPPID\n5\n901\n2\nACAD\n70\n0\n0\nENDTAB\n"
        "0\nTABLE\n2\nBLOCK_RECORD\n70\n2\n"
        "0\nBLOCK_RECORD\n5\n910\n2\nInner\n70\n0\n"
        "0\nBLOCK_RECORD\n5\n920\n2\nOuter\n70\n0\n0\nENDTAB\n0\nENDSEC\n"
        "0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\n911\n330\n910\n8\n0\n2\nInner\n70\n0\n10\n0\n20\n0\n3\nInner\n"
        "0\nLINE\n5\n912\n330\n910\n8\n0\n10\n0\n20\n0\n11\n3\n21\n2\n"
        "1001\nOnly.In.Nested\n1000\nnested payload\n"
        "1001\nIMPORTED.CUSTOM\n1000\nduplicate payload\n"
        "0\nENDBLK\n5\n913\n330\n910\n8\n0\n"
        "0\nBLOCK\n5\n921\n330\n920\n8\n0\n2\nOuter\n70\n0\n10\n0\n20\n0\n3\nOuter\n"
        "0\nINSERT\n5\n922\n330\n920\n8\n0\n2\nInner\n10\n0\n20\n0\n"
        "0\nENDBLK\n5\n923\n330\n920\n8\n0\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nINSERT\n5\n930\n8\n0\n2\nOuter\n10\n0\n20\n0\n"
        "0\nLINE\n5\n931\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "1001\nImported.Custom\n1005\n900\n1001\nACAD\n1005\n901\n"
        "1001\nNew.Custom.App\n1000\nfirst\n1001\nnew.custom.app\n1000\nsecond\n"
        "0\nENDSEC\n0\nEOF\n");
    RS_Graphic drawing;
    REQUIRE(lc::test::importDrawing(drawing, source, RS2::FormatDXFRW));
    REQUIRE(drawing.countBlocks() == 2);
    for (const auto format : {RS2::FormatDXFRW2000, RS2::FormatDXFRW2013, RS2::FormatDXFRW2018}) {
        CAPTURE(format);
        const auto exported = output.file(std::to_string(format) + ".dxf");
        REQUIRE(lc::test::exportDrawing(drawing, exported, format));
        const auto applications = checkRegisteredXData(exported);
        CHECK(applications.count("ONLY.IN.NESTED") == 1);
        CHECK(applications.count("NEW.CUSTOM.APP") == 1);
        const auto& imported = applications.at("IMPORTED.CUSTOM");
        CHECK(imported.own(2) == std::vector<std::string>{"Imported.Custom"});
        CHECK(imported.own(70) == std::vector<std::string>{"16"});
        CHECK(imported.own(71) == std::vector<std::string>{"1"});
        CHECK(applications.at("ACAD").own(5) == std::vector<std::string>{"12"});
        bool sawRemappedReferences = false;
        for (const auto& line : lc::test::dxfRecords(exported, "LINE")) {
            if (line.own(1005).size() == 2) {
                sawRemappedReferences = true;
                CHECK(line.own(1005) == std::vector<std::string>{imported.own(5).front(), "12"});
            }
        }
        CHECK(sawRemappedReferences);
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, exported, RS2::FormatDXFRW));
        CHECK(reopened.countBlocks() == 2);
    }
}

TEST_CASE("DXF registers XDATA names carried by tables and opaque objects", "[dxf][appid-registration]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    lc::test::OutputDirectory output("lc_dxf_appid_metadata");
    const auto source = output.file("source.dxf");
    writeFixture(source,
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nAPPID\n70\n1\n"
        "0\nAPPID\n5\n900\n2\nCarrier\n70\n0\n1001\nAppid.Metadata\n1000\npayload\n"
        "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n1\n"
        "0\nLAYER\n5\n902\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "1001\nLayer.Metadata\n1000\npayload\n0\nENDTAB\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nAPPID_TEST_OPAQUE\n5\nA00\n1001\nOpaque.Metadata\n1000\npayload\n"
        "0\nENDSEC\n0\nEOF\n");
    RS_Graphic drawing;
    REQUIRE(lc::test::importDrawing(drawing, source, RS2::FormatDXFRW));
    const auto exported = output.file("exported.dxf");
    REQUIRE(lc::test::exportDrawing(drawing, exported, RS2::FormatDXFRW2013));
    const auto applications = checkRegisteredXData(exported);
    for (const char* name : {"APPID.METADATA", "LAYER.METADATA", "OPAQUE.METADATA"}) {
        CAPTURE(name);
        CHECK(applications.count(name) == 1);
    }
    CHECK(lc::test::dxfRecords(exported, "APPID_TEST_OPAQUE").size() == 1);
}

TEST_CASE("DXF registers dimension XDATA generated after the APPID table", "[dxf][appid-registration]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    lc::test::OutputDirectory output("lc_dxf_appid_dimstyle");
    RS_Graphic drawing;
    auto* style = new LC_DimStyle("Generated");
    style->dimensionLine()->setLineType(QStringLiteral("DASHED"));
    style->extensionLine()->setLineTypeFirst(QStringLiteral("DASHED"));
    style->extensionLine()->setLineTypeSecond(QStringLiteral("DASHED"));
    drawing.addDimStyle(style);
    const auto exported = output.file("generated.dxf");
    REQUIRE(lc::test::exportDrawing(drawing, exported, RS2::FormatDXFRW2000));
    const auto applications = checkRegisteredXData(exported);
    std::set<std::string> generated;
    for (const auto& record : lc::test::dxfRecords(exported, "DIMSTYLE")) {
        for (const auto& name : record.own(1001))
            generated.insert(normalizedName(name));
    }
    for (const char* name : {"ACAD_DSTYLE_DIM_LINETYPE", "ACAD_DSTYLE_DIM_EXT1_LINETYPE",
                             "ACAD_DSTYLE_DIM_EXT2_LINETYPE"}) {
        CAPTURE(name);
        CHECK(generated.count(name) == 1);
        CHECK(applications.count(name) == 1);
    }
}
