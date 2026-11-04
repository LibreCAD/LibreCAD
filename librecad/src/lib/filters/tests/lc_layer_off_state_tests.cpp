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

// AutoCAD hides a layer two ways: frozen (DXF code 70 bit 1) and turned off (a negative
// colour), and a layer can be both. LibreCAD has one hidden flag and knew only frozen: a layer
// turned off opened shown, its colour became ByLayer (numberToColor() of a negative number),
// and a save wrote it shown with colour 256, which is no layer colour. Now such a layer opens
// hidden with its own colour, remembers how it was hidden (RS_Layer::HiddenAs), and is saved
// back so; a hidden current layer is saved turned off, as AutoCAD cannot freeze it.
//
// The DWG fixtures were written by the ODA File Converter (R14 and R2018) from a DXF with the
// layers "Plain" (colour 1), "Off" (3, turned off), "Frozen" (2, frozen) and "OffFrozen"
// (4, both); R14 keeps the names in capitals.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "lc_colornumbers.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {

using HiddenAs = RS_Layer::HiddenAs;

struct Expected {
    const char* m_name;
    int m_colorIndex;
    bool m_hidden;
    HiddenAs m_hiddenAs;
};

const Expected expectedLayers[] = {
    {"Plain", 1, false, HiddenAs::Frozen},
    {"Off", 3, true, HiddenAs::Off},
    {"Frozen", 2, true, HiddenAs::Frozen},
    {"OffFrozen", 4, true, HiddenAs::FrozenAndOff},
};

// "Plain", "Off", "Frozen" and "OffFrozen" as AutoCAD writes them
const QByteArray dxfWithHiddenLayers =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n5\n"
    "0\nLAYER\n5\n10\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n11\n2\nPlain\n70\n0\n62\n1\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n12\n2\nOff\n70\n0\n62\n-3\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n13\n2\nFrozen\n70\n1\n62\n2\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n14\n2\nOffFrozen\n70\n1\n62\n-4\n6\nCONTINUOUS\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n20\n8\nPlain\n10\n0\n20\n0\n11\n10\n21\n0\n"
    "0\nLINE\n5\n21\n8\nOff\n10\n0\n20\n1\n11\n10\n21\n1\n"
    "0\nENDSEC\n0\nEOF\n";

bool open(RS_Graphic& graphic, const QString& path, const RS2::FormatType format) {
    graphic.initForNewDocument();
    // with a callback: without one, a failed import waits in a message box
    const bool opened = RS_FileIO::instance()->fileImport(graphic, path, format,
                                                          [](bool, const QString&) { return false; });
    graphic.onLoadingCompleted();
    return opened;
}

/// the layer of that name, whatever its case (R13/R14 keep names in capitals)
RS_Layer* layerNamed(RS_Graphic& graphic, const QString& name) {
    for (unsigned i = 0; i < graphic.countLayers(); ++i) {
        if (graphic.layerAt(i)->getName().compare(name, Qt::CaseInsensitive) == 0) {
            return graphic.layerAt(i);
        }
    }
    return nullptr;
}

void checkHiddenLayers(RS_Graphic& graphic) {
    for (const Expected& expected : expectedLayers) {
        CAPTURE(expected.m_name);
        RS_Layer* layer = layerNamed(graphic, QString::fromLatin1(expected.m_name));
        REQUIRE(layer != nullptr);
        CHECK(layer->isFrozen() == expected.m_hidden);
        if (expected.m_hidden) {
            CHECK(layer->getHiddenAs() == expected.m_hiddenAs);
        }
        const RS_Color color = layer->getPen().getColor();
        const RS_Color own = LC_ColorNumbers::numberToColor(expected.m_colorIndex);
        CHECK_FALSE(color.isByLayer());
        CHECK(color.red() == own.red());
        CHECK(color.green() == own.green());
        CHECK(color.blue() == own.blue());
    }
}

QString writeDxf(const QTemporaryDir& directory) {
    const QString path = QDir(directory.path()).filePath(QStringLiteral("hidden-layers.dxf"));
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(dxfWithHiddenLayers) == dxfWithHiddenLayers.size());
    file.close();
    return path;
}

} // namespace

TEST_CASE("Layers turned off in a DXF open hidden, in their own colour", "[layers][layer-off]") {
    (void)lc::test::application();
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    REQUIRE(open(graphic, writeDxf(directory), RS2::FormatDXFRW));
    checkHiddenLayers(graphic);
}

TEST_CASE("Layers turned off in a DWG open hidden, in their own colour", "[layers][layer-off][fixture]") {
    (void)lc::test::application();
    for (const char* fixture : {"lc_layer_off_ac1014.dwg", "lc_layer_off_ac1032.dwg"}) {
        CAPTURE(fixture);
        const QString path = QDir(QStringLiteral(LIBRECAD_TEST_DIR)).filePath(QString::fromLatin1(fixture));
        REQUIRE(QFileInfo::exists(path));
        RS_Graphic graphic;
        REQUIRE(open(graphic, path, RS2::FormatDWG));
        checkHiddenLayers(graphic);
    }
}

TEST_CASE("Hidden layers are saved frozen, turned off or both, as they were read", "[layers][layer-off]") {
    (void)lc::test::application();
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString source = writeDxf(directory);
    struct Format {
        RS2::FormatType m_format;
        const char* m_extension;
    };
    for (const Format& format : {Format{RS2::FormatDXFRW12, "dxf"}, Format{RS2::FormatDXFRW2000, "dxf"},
                                 Format{RS2::FormatDXFRW, "dxf"}, Format{RS2::FormatDXFRW2018, "dxf"},
                                 Format{RS2::FormatDWG, "dwg"}, Format{RS2::FormatDWG2004, "dwg"},
                                 Format{RS2::FormatDWG2010, "dwg"}, Format{RS2::FormatDWG2018, "dwg"}}) {
        CAPTURE(static_cast<int>(format.m_format));
        const QString path = QDir(directory.path())
                                 .filePath(QString("saved-%1.%2").arg(static_cast<int>(format.m_format)).arg(format.m_extension));
        {
            RS_Graphic graphic;
            REQUIRE(open(graphic, source, RS2::FormatDXFRW));
            REQUIRE(RS_FileIO::instance()->fileExport(graphic, path, format.m_format));
        }
        RS_Graphic reopened;
        REQUIRE(open(reopened, path, format.m_extension == QString("dwg") ? RS2::FormatDWG : RS2::FormatDXFRW));
        checkHiddenLayers(reopened);
    }
}

TEST_CASE("A hidden current layer is saved turned off", "[layers][layer-off]") {
    // AutoCAD cannot freeze the current layer; LibreCAD lets it be hidden (#3229)
    (void)lc::test::application();
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    for (const RS2::FormatType format : {RS2::FormatDXFRW, RS2::FormatDWG2018}) {
        CAPTURE(static_cast<int>(format));
        const bool dwg = format == RS2::FormatDWG2018;
        const QString path = QDir(directory.path()).filePath(dwg ? "current.dwg" : "current.dxf");
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
            graphic.activateLayer(QStringLiteral("Walls"));
            REQUIRE(graphic.getActiveLayer() == graphic.findLayer(QStringLiteral("Walls")));
            for (const char* name : {"Doors", "Walls"}) {
                graphic.findLayer(QString::fromLatin1(name))->freeze(true);
            }
            REQUIRE(RS_FileIO::instance()->fileExport(graphic, path, format));
        }
        RS_Graphic reopened;
        REQUIRE(open(reopened, path, dwg ? RS2::FormatDWG : RS2::FormatDXFRW));
        RS_Layer* walls = reopened.findLayer(QStringLiteral("Walls"));
        RS_Layer* doors = reopened.findLayer(QStringLiteral("Doors"));
        REQUIRE(walls != nullptr);
        REQUIRE(doors != nullptr);
        CHECK(walls->isFrozen());
        CHECK(walls->getHiddenAs() == HiddenAs::Off);
        CHECK(doors->isFrozen());
        CHECK(doors->getHiddenAs() == HiddenAs::Frozen);
    }
}

TEST_CASE("A layer keeps how it is hidden when it is shown, copied or added again", "[layers][layer-off]") {
    RS_Layer layer(QStringLiteral("Off"));
    layer.freeze(true);
    layer.setHiddenAs(HiddenAs::Off);

    SECTION("shown and hidden again") {
        layer.toggle();
        CHECK_FALSE(layer.isFrozen());
        layer.toggle();
        CHECK(layer.isFrozen());
        CHECK(layer.getHiddenAs() == HiddenAs::Off);
    }
    SECTION("copied") {
        const RS_Layer copy(layer);
        CHECK(copy.getHiddenAs() == HiddenAs::Off);
        RS_Layer assigned(QStringLiteral("Other"));
        assigned = layer;
        CHECK(assigned.getHiddenAs() == HiddenAs::Off);
        const std::unique_ptr<RS_Layer> clone(layer.clone());
        CHECK(clone->getHiddenAs() == HiddenAs::Off);
    }
    SECTION("added again to a drawing that has it, as a file is read into it") {
        (void)lc::test::application();
        RS_Graphic graphic;
        graphic.initForNewDocument();
        graphic.addLayer(new RS_Layer(QStringLiteral("Off")));
        graphic.addLayer(new RS_Layer(layer));
        RS_Layer* merged = graphic.findLayer(QStringLiteral("Off"));
        REQUIRE(merged != nullptr);
        CHECK(merged->isFrozen());
        CHECK(merged->getHiddenAs() == HiddenAs::Off);
    }
}
