/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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

// Issue #2131: LC_GraphicViewport::doZoomAuto() framed a rotated UCS drawing using
// LC_CoordinatesMapper::ucsBoundingBox(), which only transforms two of the WCS box's
// four corners into UCS space -- for a rotated UCS this can be narrower than the box's
// true UCS extent (see lc_scrollmodel_tests.cpp's "The UCS box of a WCS box contains
// all four corners", which documents the same 30 degree case at the mapper level).
// doZoomAuto() now uses ucsBoundsOfWcsBox() (the four-corner box already used to size
// the scrollbars), so Zoom Extents frames the whole rotated drawing.

#include <climits>

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_math.h"

TEST_CASE("Zoom Extents frames the whole drawing under a rotated UCS", "[navigation][2131]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    // a wide, short rectangle: rotating its box makes a two-corner-vs-four-corner UCS
    // bounding box mismatch large and unambiguous (matches lc_scrollmodel_tests.cpp's
    // 30 degree case exactly, so the numbers below can be cross-checked against it).
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(0, 0), RS_Vector(100, 0)));
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(100, 0), RS_Vector(100, 10)));
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(100, 10), RS_Vector(0, 10)));
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(0, 10), RS_Vector(0, 0)));
    graphic.calculateBorders();

    LC_GraphicViewport viewport;
    viewport.setSize(800, 600);
    viewport.setBorders(10, 10, 10, 10);
    viewport.setDocument(&graphic);
    viewport.loadSettings();

    // A UCS whose x-axis points at 30 degrees in WCS. LC_GraphicViewport::createUCS()'s
    // angle is that WCS direction: doSetUCS() negates it before storing, the same way
    // lc_scrollmodel_tests.cpp's RotatedMapper(degrees) does.
    viewport.createUCS(RS_Vector(0, 0), RS_Math::deg2rad(30.0));
    REQUIRE(viewport.hasUCS());

    // keepAspectRatio=false: each axis is fit to its own UCS extent independently, so
    // the resulting view's UCS range is exactly the box doZoomAuto() computed -- this
    // makes the two-corner box's shortfall observable regardless of aspect-ratio
    // slack (with keepAspectRatio=true the axis that ISN'T the narrow one can happen
    // to have enough slack to hide the bug for a particular rectangle).
    viewport.zoomAuto(false, false);

    // Ground truth, from the real toUCS() math (verified against
    // lc_scrollmodel_tests.cpp's "The UCS box of a WCS box contains all four corners"
    // 30 degree case): the correct (four-corner) UCS bounds are x in [0, 91.60254],
    // y in [-50, 8.660254]. The OLD, buggy two-corner ucsBoundingBox() gave
    // y in [-41.339746, 0] instead -- too narrow at both ends, by nearly 9 units at
    // the bottom and by 8.66 units at the top.
    for (const RS_Vector& corner : {RS_Vector(0, 0), RS_Vector(100, 0), RS_Vector(100, 10), RS_Vector(0, 10)}) {
        const RS_Vector ucs = viewport.toUCS(corner);
        const double gx = viewport.toGuiX(ucs.x);
        const double gy = viewport.toGuiY(ucs.y);
        INFO("corner " << corner.x << "," << corner.y << " -> ucs " << ucs.x << "," << ucs.y
             << " -> gui " << gx << "," << gy);
        CHECK(gx >= -0.5);
        CHECK(gx <= viewport.getWidth() + 0.5);
        CHECK(gy >= -0.5);
        CHECK(gy <= viewport.getHeight() + 0.5);
    }

    // The corner the old two-corner box cut off at the bottom: WCS (100, 0) is UCS
    // (86.60254, -50). The old box's minY was only -41.339746, so this corner's UCS y
    // fell 8.66 units below the framed view -- it must now land inside it.
    const RS_Vector lowCorner = viewport.toUCS(RS_Vector(100, 0));
    CHECK(lowCorner.y < -41.0);
    CHECK(viewport.toGuiY(lowCorner.y) <= viewport.getHeight() + 0.5);
    CHECK(viewport.toGuiY(lowCorner.y) >= -0.5);
}

// The view offset is an int, and a conversion from a double out of the range of
// int is undefined. A small drawing far from the origin puts the offset of its
// extents there, so it is brought within half the range, with room for the
// borders and for the sums made with offsets.
TEST_CASE("Zoom Extents keeps the offset in range for a small drawing far away", "[navigation][viewport-range]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(500000., 4000000.), RS_Vector(500000.001, 4000000.001)));
    graphic.calculateBorders();

    LC_GraphicViewport viewport;
    viewport.setSize(800, 600);
    viewport.setBorders(10, 10, 10, 10);
    viewport.setDocument(&graphic);
    viewport.loadSettings();

    viewport.zoomAuto(false);

    for (const double offset : {double(viewport.getOffsetX()), double(viewport.getOffsetY())}) {
        CHECK(offset >= -INT_MAX / 2 - 10.);
        CHECK(offset <= INT_MAX / 2 + 10.);
    }
}

// The same limit decides whether a zoom window is refused: 800 pixels for 0.001
// units puts x = 2000 at 1.6e9 pixels, within the range of int but not half of it.
TEST_CASE("Zoom Window refuses a window out of the range of the offset", "[navigation][viewport-range]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(800, 600);
    const RS_Vector factor = viewport.getFactor();
    const int offsetX = viewport.getOffsetX();

    viewport.zoomWindow(RS_Vector(2000., 0.), RS_Vector(2000.001, 0.00075));

    CHECK(viewport.getFactor().x == factor.x);
    CHECK(viewport.getOffsetX() == offsetX);
}
