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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
****************************************************************************/

// Tests for RS_Information::isPointInsideContour after the half-open-parity
// rewrite: deterministic answers, orientation-free selection, and the
// absolute 1e-4 ON band on both small and large scales.

#include <array>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "lc_hatchtestsupport.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_vector.h"

using lc::test::exactInsideInteger;

namespace {

// Builds a 12-point star of integer vertices (outer radius 15, inner 6,
// centre (20,20), rounded to the nearest integer).
std::vector<RS_Vector> buildIntegerStar() {
    const int points = 12;
    const double outerR = 15.0;
    const double innerR = 6.0;
    const RS_Vector centre(20, 20);
    std::vector<RS_Vector> ring;
    ring.reserve(2 * points);
    for (int i = 0; i < 2 * points; ++i) {
        const double ang = i * M_PI / points - M_PI / 2.0;
        const double r = (i % 2 == 0) ? outerR : innerR;
        const RS_Vector v = centre + RS_Vector::polar(r, ang);
        ring.emplace_back(std::round(v.x), std::round(v.y));
    }
    return ring;
}

std::unique_ptr<RS_EntityContainer> makeContainerPolygon(
    const std::vector<RS_Vector>& ring) {
    auto cont = std::make_unique<RS_EntityContainer>(nullptr);
    const size_t n = ring.size();
    for (size_t i = 0; i < n; ++i) {
        cont->addEntity(new RS_Line(cont.get(), ring[i], ring[(i + 1) % n]));
    }
    cont->calculateBorders();
    return cont;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("isPointInsideContour gives one correct answer in 10,000 calls near a star vertex",
          "[lc-hatchfix][classifier]") {
    auto ring = buildIntegerStar();
    auto cont = makeContainerPolygon(ring);

    // The outer star vertex 0 points up: at (centre.x, centre.y + outerR), i.e. (20, 35).
    // Build points 1e-3 along the inward and outward bisectors near this vertex.
    const RS_Vector v0(20, 35);
    const RS_Vector bisector(0, -1); // inward direction

    const RS_Vector inPt = v0 + bisector * 1.0e-3;
    const RS_Vector outPt = v0 - bisector * 1.0e-3;

    // Oracle answers (point strictly inside or strictly outside the polygon).
    // Use exactInsideInteger with a sub-pixel-stable shift to the integer
    // polygon by scaling 1e6 and rounding; the near-vertex offset of 1e-3 is
    // not integer, so we use the half-open crossing on the exact polygon
    // vertices via a reliable in/out test built from them.
    std::vector<std::array<long long, 2>> polyLong;
    for (const RS_Vector& v : ring) {
        polyLong.push_back({static_cast<long long>(std::round(v.x * 1000)),
                            static_cast<long long>(std::round(v.y * 1000))});
    }
    const long long xIn = static_cast<long long>(std::round(inPt.x * 1000));
    const long long yIn = static_cast<long long>(std::round(inPt.y * 1000));
    bool onBIn = false;
    const bool oracleIn = exactInsideInteger(polyLong, xIn, yIn, &onBIn);
    REQUIRE_FALSE(onBIn);

    const long long xOut = static_cast<long long>(std::round(outPt.x * 1000));
    const long long yOut = static_cast<long long>(std::round(outPt.y * 1000));
    bool onBOut = false;
    const bool oracleOut = exactInsideInteger(polyLong, xOut, yOut, &onBOut);
    REQUIRE_FALSE(onBOut);

    for (int i = 0; i < 10000; ++i) {
        bool onContour = false;
        const bool got = RS_Information::isPointInsideContour(inPt, cont.get(), &onContour);
        REQUIRE(got == oracleIn);
    }
    for (int i = 0; i < 10000; ++i) {
        bool onContour = false;
        const bool got = RS_Information::isPointInsideContour(outPt, cont.get(), &onContour);
        REQUIRE(got == oracleOut);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("isPointInsideContour keeps its absolute 1e-4 band",
          "[lc-hatchfix][classifier]") {
    // Rectangle (0,0)-(10,5), a line edge type is sufficient for the band.
    auto cont = std::make_unique<RS_EntityContainer>(nullptr);
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 0), RS_Vector(10, 0)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 0), RS_Vector(10, 5)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 5), RS_Vector(0, 5)));
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 5), RS_Vector(0, 0)));
    cont->calculateBorders();

    // 0.5e-4 above the bottom edge: ON (within band).
    {
        bool onC = false;
        const bool got = RS_Information::isPointInsideContour(
            RS_Vector(5.0, 0.5e-4), cont.get(), &onC);
        REQUIRE(got);
        REQUIRE(onC);
    }
    // 2e-4 above: not ON (outside the band), still inside polygon.
    {
        bool onC = false;
        const bool got = RS_Information::isPointInsideContour(
            RS_Vector(5.0, 2.0e-4), cont.get(), &onC);
        REQUIRE(got);
        REQUIRE_FALSE(onC);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("isPointInsideContour is orientation-free on an unordered selection",
          "[lc-hatchfix][classifier]") {
    // Rectangle (0,0)-(10,5) with lines in {2, 0, 3, 1} order and one reversed.
    // Insert in order: edge2, edge0 (reversed), edge3, edge1.
    auto cont = std::make_unique<RS_EntityContainer>(nullptr);
    // edge2: (10,5) -> (0,5)
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 5), RS_Vector(0, 5)));
    // edge0 reversed: (10,0) -> (0,0)
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 0), RS_Vector(0, 0)));
    // edge3: (0,5) -> (0,0)
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(0, 5), RS_Vector(0, 0)));
    // edge1: (10,0) -> (10,5)
    cont->addEntity(new RS_Line(cont.get(), RS_Vector(10, 0), RS_Vector(10, 5)));
    cont->calculateBorders();

    std::mt19937 gen(1);
    std::uniform_real_distribution<double> dX(-1.0, 11.0);
    std::uniform_real_distribution<double> dY(-1.0, 6.0);

    int wrong = 0;
    for (int i = 0; i < 5000; ++i) {
        const RS_Vector p(dX(gen), dY(gen));
        const bool expected = (p.x > 1.0e-4 && p.x < 10.0 - 1.0e-4 &&
                               p.y > 1.0e-4 && p.y < 5.0 - 1.0e-4);
        const bool nearBoundary = (p.x > -1.0e-4 && p.x < 10.0 + 1.0e-4 &&
                                   p.y > -1.0e-4 && p.y < 5.0 + 1.0e-4 && !expected);
        if (nearBoundary) continue; // the 1e-4 band is a tolerance zone; skip
        bool onC = false;
        const bool got = RS_Information::isPointInsideContour(p, cont.get(), &onC);
        if (got != expected && !onC) ++wrong;
    }
    REQUIRE(wrong == 0);
}
