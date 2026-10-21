/****************************************************************************
** This file is part of the LibreCAD project, a 2D CAD program.
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any later
** version. This program is distributed WITHOUT ANY WARRANTY. See the
** GNU General Public License for more details.
****************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#include <QFile>
#include <QTemporaryDir>

#include "drw_interface.h"
#include "libdxfrw.h"

namespace {

// Native callbacks only: no application-side decoding or name repair can
// conceal a mismatch between a definition and the entity that names it.
class NameReferences : public DRW_Interface {
public:
    std::vector<std::string> blocks;
    std::vector<std::string> dimensionBlocks;
    std::vector<std::string> mlineStyles;
    std::vector<std::string> mlineReferences;
    std::vector<std::string> textStyles;
    std::vector<std::string> mtextStyles;

    void addBlock(const DRW_Block& block) override { blocks.push_back(block.name); }
    void addDimAlign(const DRW_DimAligned* dimension) override { dimensionBlocks.push_back(DRW_Dimension(*dimension).getName()); }
    void addMLineStyle(const DRW_MLineStyle& style) override { mlineStyles.push_back(style.name); }
    void addMLine(const DRW_MLine* mline) override { mlineReferences.push_back(mline->styleName); }
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle& style) override { textStyles.push_back(style.name); }
    void addAppId(const DRW_AppId&) override {}
    void setBlock(int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText& mtext) override { mtextStyles.push_back(mtext.style); }
    void addText(const DRW_Text&) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial* dimension) override { dimensionBlocks.push_back(DRW_Dimension(*dimension).getName()); }
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimArc(const DRW_DimArc* dimension) override { dimensionBlocks.push_back(DRW_Dimension(*dimension).getName()); }
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
};

// A controlled R2000 drawing: a named block used by an aligned DIMENSION,
// and a named one-element MLINESTYLE used by a two-vertex MLINE. All names
// share the caller's byte interpretation; handles remain ASCII identities.
std::string drawing(const std::string& acute) {
    const std::string block = "DIM_" + acute;
    const std::string style = "ML_" + acute;
    const std::string textStyle = "STYLE_" + acute;
    return
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
        "9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nBLOCK_RECORD\n5\n21\n100\nAcDbSymbolTable\n70\n1\n"
        "0\nBLOCK_RECORD\n5\n20\n100\nAcDbSymbolTableRecord\n"
        "100\nAcDbBlockTableRecord\n2\n" + block + "\n"
        "0\nENDTAB\n0\nTABLE\n2\nSTYLE\n5\n22\n100\nAcDbSymbolTable\n70\n1\n"
        "0\nSTYLE\n5\n23\n100\nAcDbSymbolTableRecord\n100\nAcDbTextStyleTableRecord\n"
        "2\n" + textStyle + "\n70\n0\n40\n0\n41\n1\n50\n0\n71\n0\n42\n1\n3\ntxt\n4\n\n"
        "0\nENDTAB\n0\nENDSEC\n"
        "0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\n10\n330\n20\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbBlockBegin\n2\n" + block + "\n70\n0\n10\n0\n20\n0\n30\n0\n3\n" + block + "\n1\n\n"
        "0\nENDBLK\n5\n11\n330\n20\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nDIMENSION\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbDimension\n"
        "2\n" + block + "\n10\n0\n20\n0\n30\n0\n11\n0\n21\n1\n31\n0\n70\n1\n"
        "100\nAcDbAlignedDimension\n13\n0\n23\n0\n33\n0\n14\n1\n24\n0\n34\n0\n"
        "0\nMLINE\n5\n31\n100\nAcDbEntity\n8\n0\n100\nAcDbMline\n"
        "2\n" + style + "\n340\n40\n40\n1\n70\n0\n71\n1\n72\n2\n73\n1\n"
        "10\n0\n20\n0\n30\n0\n210\n0\n220\n0\n230\n1\n"
        "11\n0\n21\n0\n31\n0\n12\n1\n22\n0\n32\n0\n13\n0\n23\n1\n33\n0\n74\n2\n41\n0\n41\n0\n75\n0\n"
        "11\n1\n21\n0\n31\n0\n12\n1\n22\n0\n32\n0\n13\n0\n23\n1\n33\n0\n74\n2\n41\n0\n41\n0\n75\n0\n"
        "0\nMTEXT\n5\n32\n100\nAcDbEntity\n8\n0\n100\nAcDbMText\n"
        "10\n0\n20\n0\n40\n2.5\n41\n20\n71\n1\n1\nsample\n7\n" + textStyle + "\n"
        "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
        "0\nMLINESTYLE\n5\n40\n330\nC\n100\nAcDbMlineStyle\n2\n" + style + "\n"
        "70\n0\n3\n\n62\n256\n51\n90\n52\n90\n71\n1\n49\n0\n62\n256\n6\nBYLAYER\n"
        "0\nENDSEC\n0\nEOF\n";
}

void readNames(NameReferences& names, bool legacyUtf8) {
    std::string input = drawing(legacyUtf8 ? "\xC3\xA9" : "\xE9");
    dxfRW reader("legacy-name-reference-fixture.dxf");
    if (legacyUtf8)
        reader.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
    REQUIRE(reader.readAscii(&names, false, input));
}

class DimensionNameWriter : public NameReferences {
public:
    dxfRW* output = nullptr;
    int kind = 0;
    bool ok = true;
    const std::string blockName = "DIM_\xC3\xA9";
    const std::string styleName = "STYLE_\xC3\xA9";

    void writeHeader(DRW_Header& header) override {
        header.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
    }
    void writeBlockRecords() override { ok &= output->writeBlockRecord(blockName); }
    void writeTextstyles() override {
        DRW_Textstyle style;
        style.name = styleName;
        style.font = "txt";
        ok &= output->writeTextstyle(&style);
    }
    void writeBlocks() override {
        DRW_Block block;
        block.name = blockName;
        ok &= output->writeBlock(&block);
    }
    void writeEntities() override {
        if (kind == 0) {
            DRW_DimAligned dimension;
            dimension.setName(blockName);
            dimension.type = 1;
            ok &= output->writeDimension(&dimension);
        } else if (kind == 1) {
            DRW_DimArc dimension;
            dimension.setName(blockName);
            dimension.type = 5;
            ok &= output->writeDimension(&dimension);
        } else {
            DRW_DimLargeRadial dimension;
            dimension.setName(blockName);
            dimension.type = 4;
            ok &= output->writeDimension(&dimension);
        }
        DRW_MText mtext;
        mtext.text = "sample";
        mtext.style = styleName;
        mtext.height = 2.5;
        ok &= output->writeMText(&mtext);
    }
};

} // namespace

TEST_CASE("DXF dimension block names follow the document text encoding",
          "[dxf][codepage][legacy-utf8][name-reference]") {
    for (bool legacyUtf8 : {false, true}) {
        CAPTURE(legacyUtf8);
        NameReferences names;
        readNames(names, legacyUtf8);
        REQUIRE(names.blocks.size() == 1);
        REQUIRE(names.dimensionBlocks.size() == 1);
        CHECK(names.blocks.front() == "DIM_\xC3\xA9");
        CHECK(names.dimensionBlocks.front() == "DIM_\xC3\xA9");
        CHECK(names.dimensionBlocks.front() == names.blocks.front());
    }
}

TEST_CASE("DXF MLINE style names follow the document text encoding",
          "[dxf][codepage][legacy-utf8][name-reference]") {
    for (bool legacyUtf8 : {false, true}) {
        CAPTURE(legacyUtf8);
        NameReferences names;
        readNames(names, legacyUtf8);
        REQUIRE(names.mlineStyles.size() == 1);
        REQUIRE(names.mlineReferences.size() == 1);
        CHECK(names.mlineStyles.front() == "ML_\xC3\xA9");
        CHECK(names.mlineReferences.front() == "ML_\xC3\xA9");
        CHECK(names.mlineReferences.front() == names.mlineStyles.front());
    }
}

TEST_CASE("DXF dimension exports encode block references just like block definitions",
          "[dxf][codepage][legacy-utf8][name-reference][roundtrip]") {
    for (int kind : {0, 1, 2}) {
        DYNAMIC_SECTION("aligned, arc or large radial: " << kind) {
            QTemporaryDir directory;
            REQUIRE(directory.isValid());
            const QString path = directory.filePath("corrected.dxf");
            dxfRW output(path.toUtf8().constData());
            // Even a writer on which an input override was selected must
            // emit the output codepage, not legacy mislabeled UTF-8.
            output.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
            DimensionNameWriter writer;
            writer.output = &output;
            writer.kind = kind;
            REQUIRE(output.write(&writer, DRW::AC1015, false));
            REQUIRE(writer.ok);
            QFile file(path);
            REQUIRE(file.open(QIODevice::ReadOnly));
            QByteArray bytes = file.readAll();
            file.close();
            bytes.replace("\r\n", "\n");
            CHECK(bytes.contains("2\nDIM_\xE9\n"));
            CHECK_FALSE(bytes.contains("2\nDIM_\xC3\xA9\n"));
            CHECK(bytes.contains("7\nSTYLE_\xE9\n"));
            CHECK_FALSE(bytes.contains("7\nSTYLE_\xC3\xA9\n"));

            NameReferences names;
            dxfRW input(path.toUtf8().constData());
            REQUIRE(input.read(&names, false));
            REQUIRE(names.dimensionBlocks.size() == 1);
            CHECK(names.dimensionBlocks.front() == writer.blockName);
            CHECK(std::find(names.blocks.begin(), names.blocks.end(), writer.blockName) != names.blocks.end());
            REQUIRE(names.mtextStyles.size() == 1);
            CHECK(names.mtextStyles.front() == writer.styleName);
            CHECK(std::find(names.textStyles.begin(), names.textStyles.end(), writer.styleName) != names.textStyles.end());
        }
    }
}

TEST_CASE("DXF MTEXT style references follow the document text encoding",
          "[dxf][codepage][legacy-utf8][name-reference]") {
    for (bool legacyUtf8 : {false, true}) {
        CAPTURE(legacyUtf8);
        NameReferences names;
        readNames(names, legacyUtf8);
        REQUIRE(names.textStyles.size() == 1);
        REQUIRE(names.mtextStyles.size() == 1);
        CHECK(names.textStyles.front() == "STYLE_\xC3\xA9");
        CHECK(names.mtextStyles.front() == names.textStyles.front());
    }
}
