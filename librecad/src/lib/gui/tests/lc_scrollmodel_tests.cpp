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

// Issue #2945: the widget-free scrollbar model and the UCS box it is built on.

#include <cmath>
#include <limits>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lc_coordinates_mapper.h"
#include "lc_scrollmodel.h"
#include "lc_viewmath.h"
#include "rs_math.h"

using Catch::Approx;
using LC_ScrollModel::Axis;
using LC_ScrollModel::State;

namespace {
    double thumbFraction(const State& s) {
        return static_cast<double>(s.pageStep) / (s.maximum + s.pageStep);
    }

    /// a mapper with a UCS rotated so that its x axis points at \p degrees in WCS
    struct RotatedMapper : LC_CoordinatesMapper {
        explicit RotatedMapper(const double degrees) {
            update(RS_Vector(0, 0), -RS_Math::deg2rad(degrees));
            useUCS(true);
        }
    };
}

TEST_CASE("Scroll value never clamps the view", "[navigation][2169]") {
    // content 0..2000 px, view 800 px: inside, straddling, far outside on both sides
    for (const double viewStart : {-400.0, 0.0, 600.0, 1200.0, 1700.0, -900.0, 2300.0, -20000.0, 20000.0}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, viewStart, 800.0});
        INFO("view start " << viewStart);
        REQUIRE(s.valid);
        CHECK(s.pixelsPerTick == 1.0);
        CHECK(s.value >= 0);
        CHECK(s.value <= s.maximum);
        CHECK(s.viewStartFor(s.value) == viewStart);
    }
}

TEST_CASE("The thumb fraction is L/(E+L) and shrinks on zoom in", "[navigation][2945]") {
    double previous = 1.0;
    for (double factor = 1.0; factor <= 64.0; factor *= 2.0) {
        const double extent = 400.0 * factor;
        // view centred on the content
        const State s = LC_ScrollModel::compute({true, 0.0, extent, extent / 2 - 400.0, 800.0});
        CHECK(thumbFraction(s) == Approx(800.0 / (extent + 800.0)).margin(1e-3));
        CHECK(thumbFraction(s) < previous);
        previous = thumbFraction(s);
    }
    // worked example of the plan: 800 px view, 2000 px drawing
    const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
    CHECK(s.maximum == 2000);
    CHECK(s.pageStep == 800);
    CHECK(s.value == 1000);
}

TEST_CASE("The thumb position encodes the view centre", "[navigation][2945]") {
    for (const double centre : {0.0, 250.0, 1000.0, 1999.0, 2000.0}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, centre - 400.0, 800.0});
        CHECK(static_cast<double>(s.value) / s.maximum == Approx(centre / 2000.0).margin(1e-3));
    }
}

TEST_CASE("An empty drawing has nothing to scroll: the bar is full length", "[navigation][2945]") {
    // v2: region = view (not a UCS-origin anchor with a margin, which invited scrolling
    // into blank space): a full-length thumb wherever the view happens to be.
    for (const double viewStart : {-400.0, 0.0, 1000.0, -12345.0, 987654.0}) {
        const State s = LC_ScrollModel::compute({false, 123.0, 456.0, viewStart, 800.0});
        INFO("view start " << viewStart);
        REQUIRE(s.valid);
        CHECK(s.maximum == 0);
        CHECK(s.value == 0);
        CHECK(s.pageStep >= 1);
        CHECK(s.viewStartFor(0) == viewStart);
    }
    // invalid content (unordered or non-finite) is treated the same as no content
    const State unordered = LC_ScrollModel::compute({true, 500.0, -500.0, 0.0, 800.0});
    CHECK(unordered.maximum == 0);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const State nonFinite = LC_ScrollModel::compute({true, nan, 456.0, 0.0, 800.0});
    CHECK(nonFinite.maximum == 0);
}

TEST_CASE("Huge coordinates keep the scroll model int safe", "[navigation][2945]") {
    // extents +/-1e6 at factor 1e9
    const State s = LC_ScrollModel::compute({true, -1e15, 1e15, 0.0, 800.0});
    REQUIRE(s.valid);
    CHECK(s.maximum <= (1 << 24));
    CHECK(s.pageStep >= 1);
    CHECK(s.singleStep >= 1);
    CHECK(s.value >= 0);
    CHECK(s.value <= s.maximum);
    CHECK(std::abs(s.viewStartFor(s.value)) <= s.pixelsPerTick);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, nan}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, 0.0}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, -5.0}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, inf, 800.0}).valid);
    // non-finite content is treated as no content: region = view
    const State noContent = LC_ScrollModel::compute({true, -inf, nan, 0.0, 800.0});
    CHECK(noContent.valid);
    CHECK(noContent.origin == 0.0);
    CHECK(noContent.maximum == 0);

    CHECK(LC_ViewMath::saturatingRound(nan) == 0);
    CHECK(LC_ViewMath::saturatingRound(inf) == 0);
    CHECK(LC_ViewMath::saturatingRound(-inf) == 0);
    CHECK(LC_ViewMath::saturatingRound(1e300) == static_cast<int>(LC_ViewMath::kMaxViewPixel));
    CHECK(LC_ViewMath::saturatingRound(-2.5) == -3);
    CHECK(LC_ViewMath::saturatingRound(2.5) == 3);
}

TEST_CASE("The vertical scroll axis grows as the view moves down", "[navigation][2945]") {
    // V axis: content [-uMax.y*fy, -uMin.y*fy], view start oy - H
    const double fy = 5.0;
    const int height = 600;
    int previous = -1;
    // views that overlap the region, from above the drawing to below it
    for (int oy = 100; oy <= 1100; oy += 100) {
        const State s = LC_ScrollModel::compute({true, -50.0 * fy, 0.0, static_cast<double>(oy - height),
                                                 static_cast<double>(height)});
        CHECK(s.value > previous);
        previous = s.value;
    }
}

TEST_CASE("The UCS box of a WCS box contains all four corners", "[navigation][2945]") {
    const RS_Vector wcsMin(0, 0);
    const RS_Vector wcsMax(100, 10);
    {
        const RotatedMapper mapper(30.0);
        RS_Vector ucsMin, ucsMax;
        mapper.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
        CHECK(ucsMin.y == Approx(-50.0).margin(1e-9));
        CHECK(ucsMax.y == Approx(8.660254).margin(1e-6));
        CHECK(ucsMin.x == Approx(0.0).margin(1e-9));
        CHECK(ucsMax.x == Approx(91.60254).margin(1e-5));

        // guard: the zoom-auto box keeps its two-corner result
        RS_Vector oldMin, oldMax;
        mapper.ucsBoundingBox(wcsMin, wcsMax, oldMin, oldMax);
        CHECK(oldMin.y == Approx(-41.339746).margin(1e-6));
        CHECK(oldMax.y == Approx(0.0).margin(1e-9));
    }
    for (const double degrees : {45.0, -30.0, 120.0, 200.0}) {
        const RotatedMapper mapper(degrees);
        RS_Vector ucsMin, ucsMax;
        mapper.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
        for (const RS_Vector& corner : {RS_Vector(0, 0), RS_Vector(100, 0), RS_Vector(100, 10), RS_Vector(0, 10)}) {
            const RS_Vector u = mapper.toUCS(corner);
            INFO("angle " << degrees << " corner " << corner.x << "," << corner.y);
            CHECK(u.x >= ucsMin.x - 1e-9);
            CHECK(u.x <= ucsMax.x + 1e-9);
            CHECK(u.y >= ucsMin.y - 1e-9);
            CHECK(u.y <= ucsMax.y + 1e-9);
        }
    }
    // without a UCS the box is unchanged
    const LC_CoordinatesMapper identity;
    RS_Vector ucsMin, ucsMax;
    identity.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
    CHECK(ucsMin == wcsMin);
    CHECK(ucsMax == wcsMax);
}
