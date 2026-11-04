/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
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

// A DWG header names the current layer by handle (CLAYER). The reader read the handle and
// dropped it, so every DWG opened with layer "0" current, and the writer always wrote layer
// 0's handle, so every DWG LibreCAD saved said layer "0" was current (ODA File Converter read
// "$CLAYER=0" from all of them). The reader now names the layer of that handle once the LAYER
// table is read; the writer gives the layer $CLAYER names a handle before it writes the
// header, since the layers are added after it.
//
// The fixtures were written by the ODA File Converter (R14 and R2018) from a DXF with the
// layers "0", "Doors" and "Walls", "Walls" current.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

#include "drw_header.h"
#include "drw_objects.h"
#include "lc_actiontestsupport.h"
#include "libdwgr.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual.
class StubInterface : public DRW_Interface {
public:
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void addBlock(const DRW_Block &) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point &) override {}
    void addLine(const DRW_Line &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
    void addArc(const DRW_Arc &) override {}
    void addCircle(const DRW_Circle &) override {}
    void addEllipse(const DRW_Ellipse &) override {}
    void addLWPolyline(const DRW_LWPolyline &) override {}
    void addPolyline(const DRW_Polyline &) override {}
    void addSpline(const DRW_Spline *) override {}
    void addKnot(const DRW_Entity &) override {}
    void addInsert(const DRW_Insert &) override {}
    void addTrace(const DRW_Trace &) override {}
    void add3dFace(const DRW_3Dface &) override {}
    void addSolid(const DRW_Solid &) override {}
    void addMText(const DRW_MText &) override {}
    void addText(const DRW_Text &) override {}
    void addDimAlign(const DRW_DimAligned *) override {}
    void addDimLinear(const DRW_DimLinear *) override {}
    void addDimRadial(const DRW_DimRadial *) override {}
    void addDimDiametric(const DRW_DimDiametric *) override {}
    void addDimAngular(const DRW_DimAngular *) override {}
    void addDimAngular3P(const DRW_DimAngular3p *) override {}
    void addDimArc(const DRW_DimArc *) override {}
    void addDimOrdinate(const DRW_DimOrdinate *) override {}
    void addLeader(const DRW_Leader *) override {}
    void addHatch(const DRW_Hatch *) override {}
    void addViewport(const DRW_Viewport &) override {}
    void addImage(const DRW_Image *) override {}
    void addWipeout(const DRW_Wipeout *) override {}
    void addMLeader(const DRW_MLeader *) override {}
    void addMLeaderStyle(const DRW_MLeaderStyle *) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}
    void writeHeader(DRW_Header &) override {}
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

/// writes a header with $CLAYER and the given layers
class LayersWriter final : public StubInterface {
public:
    dwgRW* m_writer = nullptr;
    std::string m_current;
    std::vector<std::string> m_layers;

    void writeHeader(DRW_Header& header) override {
        header.addStr("$CLAYER", m_current, 8);
    }
    void writeLayers() override {
        for (const std::string& name : m_layers) {
            DRW_Layer layer;
            layer.name = name;
            REQUIRE(m_writer->addLayer(&layer));
        }
    }
};

/// the CLAYER a file's header gives, and its layers
class LayersReader final : public StubInterface {
public:
    std::string m_current = "<none>";
    std::vector<std::string> m_layers;

    void addHeader(const DRW_Header* header) override {
        for (const char* key : {"CLAYER", "$CLAYER"}) {
            const auto var = header->vars.find(key);
            if (var != header->vars.end() && var->second != nullptr && var->second->type() == DRW_Variant::STRING) {
                m_current = *var->second->content.s;
            }
        }
    }
    void addLayer(const DRW_Layer& layer) override {
        m_layers.push_back(layer.name);
    }
};

bool contains(const std::vector<std::string>& names, const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

QString openedCurrentLayer(const QString& path) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    // with a callback: without one, a failed import waits in a message box
    REQUIRE(RS_FileIO::instance()->fileImport(graphic, path, RS2::FormatDWG,
                                              [](bool, const QString&) { return false; }));
    graphic.onLoadingCompleted();
    REQUIRE(graphic.getActiveLayer() != nullptr);
    return graphic.getActiveLayer()->getName();
}

} // namespace

TEST_CASE("A DWG saved by LibreCAD opens with its current layer", "[dwg][layers][current-layer]") {
    (void)lc::test::application();
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    QString current;
    SECTION("a layer of its own") {
        current = QStringLiteral("Walls");
    }
    SECTION("layer 0") {
        current = QStringLiteral("0");
    }

    for (const RS2::FormatType format : {RS2::FormatDWG, RS2::FormatDWG2004, RS2::FormatDWG2010,
                                         RS2::FormatDWG2013, RS2::FormatDWG2018}) {
        CAPTURE(static_cast<int>(format));
        const QString path = QDir(directory.path()).filePath(QString("current-%1.dwg").arg(static_cast<int>(format)));
        {
            RS_Graphic graphic;
            graphic.initForNewDocument();
            for (const char* name : {"Doors", "Walls"}) {
                auto* layer = new RS_Layer(QString::fromLatin1(name));
                graphic.addLayer(layer);
                auto* line = new RS_Line(&graphic, RS_Vector(0, 0), RS_Vector(10, 0));
                line->setLayer(layer);
                graphic.addEntity(line);
            }
            graphic.activateLayer(current);
            REQUIRE(graphic.getActiveLayer() != nullptr);
            REQUIRE(graphic.getActiveLayer()->getName() == current);
            REQUIRE(RS_FileIO::instance()->fileExport(graphic, path, format));
        }
        CHECK(openedCurrentLayer(path) == current);
    }
}

TEST_CASE("A DWG written by another program opens with its current layer", "[dwg][layers][current-layer][fixture]") {
    (void)lc::test::application();
    for (const char* fixture : {"lc_current_layer_walls_ac1014.dwg", "lc_current_layer_walls_ac1032.dwg"}) {
        CAPTURE(fixture);
        const QString path = QDir(QStringLiteral(LIBRECAD_TEST_DIR)).filePath(QString::fromLatin1(fixture));
        REQUIRE(QFileInfo::exists(path));
        // an R14 file has its names in capitals: "WALLS"
        CHECK(openedCurrentLayer(path).compare(QStringLiteral("Walls"), Qt::CaseInsensitive) == 0);
    }
}

TEST_CASE("The DWG writer gives CLAYER the handle of the layer it names", "[dwg][dwg-write][current-layer]") {
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    std::string current;
    std::vector<std::string> layers;
    std::string expected;
    SECTION("named in another case than the layer") {
        current = "walls";
        layers = {"Doors", "Walls"};
        expected = "Walls";
    }
    SECTION("a layer that is never added, which is then written") {
        current = "Ghost";
        layers = {"Doors"};
        expected = "Ghost";
    }
    SECTION("layer 0") {
        current = "0";
        layers = {"Doors"};
        expected = "0";
    }

    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        CAPTURE(static_cast<int>(version));
        const std::string path =
            QDir(directory.path()).filePath(QString("writer-%1.dwg").arg(static_cast<int>(version))).toStdString();
        LayersWriter writer;
        writer.m_current = current;
        writer.m_layers = layers;
        {
            dwgRW rw(path.c_str());
            writer.m_writer = &rw;
            REQUIRE(rw.write(&writer, version, /*bin=*/false));
        }
        LayersReader reader;
        {
            dwgRW rw(path.c_str());
            REQUIRE(rw.read(&reader, /*ext=*/false));
        }
        CHECK(reader.m_current == expected);
        CHECK(contains(reader.m_layers, expected));
        for (const std::string& name : layers) {
            CHECK(contains(reader.m_layers, name));
        }
    }
}
