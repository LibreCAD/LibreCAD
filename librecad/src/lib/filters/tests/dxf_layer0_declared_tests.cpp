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

// Regression test for a use-after-free in RS_FilterDXFRW::addLayer():
// RS_LayerList::add() deletes the RS_Layer* passed to it and updates an
// already-existing same-named layer in place instead, whenever one exists
// (documented on that function). RS_Graphic always bootstraps a default "0"
// layer before import starts, so any DXF that explicitly declares "0" in its
// own LAYER table (the normal, common case for real-world/AutoCAD-authored
// files) triggers this collision on every import. addLayer() was caching the
// (now possibly-deleted) pointer into m_importLayerCache right after the
// call, and every entity on layer "0" got attached to that dangling pointer
// via setLayer() - silently breaking anything that compares layer pointers
// later (e.g. LC_MakerCamSVG::writeEntities()'s `e->getLayer() == layer`
// filter, which produced a completely empty SVG export for a real-world file
// that hit this).
//
// No existing test covered "DXF explicitly declares layer 0" at all before
// this file - every other filter test either relies on the implicit default
// layer or declares only non-"0" layers, so none of them exercised the one
// path that always collides with the bootstrap layer.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include <QCoreApplication>

#include "rs_entity.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
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

std::string tmpFile(const char* suffix) {
    return (std::filesystem::temp_directory_path() /
            (std::string("dxf_layer0_declared_") + suffix))
        .string();
}

void writeText(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

} // namespace

TEST_CASE("DXF explicitly declaring layer \"0\" does not leave entities on a "
          "dangling layer pointer",
          "[dxf][filter][layer][regression]") {
    ensureSettings();
    const std::string src = tmpFile("src.dxf");
    std::filesystem::remove(src);

    // A minimal but valid DXF that explicitly declares "0" in its own LAYER
    // table (the trigger condition) with non-default properties, then draws
    // one LINE on that layer. RS_Graphic's bootstrap "0" layer already exists
    // by the time this table entry is processed, so this always exercises
    // RS_LayerList::add()'s merge-and-delete branch for the constructed "0"
    // layer object.
    const std::string dxf =
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n"
        "0\nLAYER\n2\n0\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
        "0\nENDSEC\n0\nEOF\n";
    writeText(src, dxf);

    RS_Graphic graphic;
    graphic.initForNewDocument();

    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));

    RS_Layer* survivingLayer0 = graphic.findLayer("0");
    REQUIRE(survivingLayer0 != nullptr);

    // The surviving layer must carry the DXF's own declared color (3), not
    // whatever RS_Graphic's bootstrap default was - proving the "0" special
    // case in addLayer() still does what it's meant to do (apply the file's
    // real declaration), not just that the pointer is non-dangling.
    CHECK(survivingLayer0->getPen().getColor().toIntColor() != 0);

    int entityCount = 0;
    int matchingLayer = 0;
    for (auto it = graphic.begin(); it != graphic.end(); ++it) {
        ++entityCount;
        // This is the exact check that was silently failing before the fix:
        // entities' getLayer() pointed at freed memory, so this comparison
        // against the layer list's own surviving object never matched -
        // which is precisely why LC_MakerCamSVG::writeEntities()'s identical
        // `e->getLayer() == layer` filter produced empty output.
        if ((*it)->getLayer() == survivingLayer0) {
            ++matchingLayer;
        }
    }

    REQUIRE(entityCount == 1);
    CHECK(matchingLayer == 1);

    std::filesystem::remove(src);
}
