// File: test_hyperbola_spline_roundtrip.cpp
// Catch2 unit tests for hyperbola ↔ DRW_Spline round-trip conversion

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include <iostream>

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "lc_hyperbola.h"
#include "lc_hyperbolaspline.h"
#include "drw_entities.h"
#include "rs_math.h"

constexpr double TOL = 1e-8;
constexpr double ANGLE_TOL = 1e-6;

// Helper to compare vectors
bool vectorsApproxEqual(const RS_Vector& a, const RS_Vector& b)
{
    return (a - b).squared() < TOL * TOL;
}

// Helper to compare doubles
bool doublesApproxEqual(double a, double b, double tolerance = TOL)
{
    return std::abs(a - b) < tolerance;
}

// Helper to compare hyperbola data
bool hyperbolaDataApproxEqual(const LC_HyperbolaData& a, const LC_HyperbolaData& b)
{
    return vectorsApproxEqual(a.center, b.center) &&
           doublesApproxEqual(a.majorP.magnitude(), b.majorP.magnitude()) &&
           doublesApproxEqual(a.ratio, b.ratio) &&
           a.reversed == b.reversed &&
           doublesApproxEqual(RS_Math::getAngleDifference(a.majorP.angle(), b.majorP.angle()), 0.0, ANGLE_TOL) &&
           doublesApproxEqual(a.angle1, b.angle1, ANGLE_TOL) &&
           doublesApproxEqual(a.angle2, b.angle2, ANGLE_TOL);
}

TEST_CASE("Hyperbola ↔ DRW_Spline round-trip validation", "[hyperbola][spline][roundtrip]")
{
  // Updated test case section with fixed expected y values and tolerance
// Updated test case section with analytical shoulder validation

SECTION("Limited arc hyperbola: y ∈ [-1, 2] on x² - 16 y² = 1 - validate spline output with analytical shoulder")
{
    LC_HyperbolaData original;
    original.center = RS_Vector(0.0, 0.0);
    original.majorP = RS_Vector(1.0, 0.0);  // a = 1
    original.ratio = 0.25;                  // b = 0.25
    original.reversed = false;

    double y_start = -1.0;
    double y_end   =  2.0;

    double phi_start = std::asinh(y_start / 0.25);  // ≈ -2.0947
    double phi_end   = std::asinh(y_end   / 0.25);  // ≈  2.7765
    original.angle1 = phi_start;
    original.angle2 = phi_end;

    DRW_Spline spl;
    REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

    // === Validate spline structure ===
    REQUIRE(spl.degree == 2);
    REQUIRE(spl.flags == 8);
    REQUIRE(spl.controllist.size() == 3);
    REQUIRE(spl.weightlist.size() == 3);

    // === Validate weights ===
    REQUIRE(doublesApproxEqual(spl.weightlist[0], 1.0));
    REQUIRE(doublesApproxEqual(spl.weightlist[2], 1.0));
    double w_middle = spl.weightlist[1];
    REQUIRE(w_middle > 1.0);

    // === Extract control points ===
    RS_Vector p0(spl.controllist[0]->x, spl.controllist[0]->y);
    RS_Vector p1(spl.controllist[1]->x, spl.controllist[1]->y);  // shoulder
    RS_Vector p2(spl.controllist[2]->x, spl.controllist[2]->y);

    // === Validate start/end points exactly ===
    REQUIRE(doublesApproxEqual(p0.y, y_start));
    REQUIRE(doublesApproxEqual(p2.y, y_end));

    // Points must lie on original hyperbola
    REQUIRE(doublesApproxEqual(p0.x * p0.x - 16.0 * p0.y * p0.y, 1.0));
    REQUIRE(doublesApproxEqual(p2.x * p2.x - 16.0 * p2.y * p2.y, 1.0));

    // === Analytical shoulder validation ===
    double a = 1.0;
    double b = 0.25;
    double phi_avg = (phi_start + phi_end) * 0.5;
    double expected_shoulder_y = -b * std::tanh(phi_avg);
    double expected_shoulder_x = a;  // analytical: shoulder x = a

    REQUIRE(doublesApproxEqual(p1.x, expected_shoulder_x));
    REQUIRE(doublesApproxEqual(p1.y, expected_shoulder_y));

    // === Round-trip recovery ===
    auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
    REQUIRE(recovered != nullptr);
    REQUIRE(recovered->isValid());

    const LC_HyperbolaData& rec = recovered->getData();
    REQUIRE(hyperbolaDataApproxEqual(rec, original));

    // Verify parametric range preservation
    REQUIRE(doublesApproxEqual(rec.angle1, phi_start, 1e-6));
    REQUIRE(doublesApproxEqual(rec.angle2, phi_end,   1e-6));
}

    SECTION("Left branch hyperbola")
    {
        LC_HyperbolaData original;
        original.center = RS_Vector(0.0, 0.0);
        original.majorP = RS_Vector(3.0, 0.0);
        original.ratio = 4.0 / 3.0;
        original.reversed = true;  // left branch
        original.angle1 = 0.0;
        original.angle2 = 0.0;

        DRW_Spline spl;
        REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

        auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
        REQUIRE(recovered != nullptr);

        const LC_HyperbolaData& rec = recovered->getData();
        REQUIRE(rec.reversed == true);  // branch correctly recovered
        REQUIRE(hyperbolaDataApproxEqual(rec, original));
    }

    SECTION("Non-hyperbola splines return nullptr")
    {
        // Parabola example
        DRW_Spline parabola;
        parabola.degree = 2;
        parabola.flags = 8;
        parabola.controllist.resize(3);
        parabola.controllist[0] = std::make_shared<DRW_Coord>(0.0, 0.0);
        parabola.controllist[1] = std::make_shared<DRW_Coord>(1.0, 1.0);
        parabola.controllist[2] = std::make_shared<DRW_Coord>(2.0, 0.0);
        parabola.weightlist = {1.0, 0.5, 1.0};
        parabola.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

        REQUIRE(LC_HyperbolaSpline::splineToHyperbola(parabola, nullptr) == nullptr);

        // Ellipse (quarter circle)
        DRW_Spline ellipse;
        ellipse.degree = 2;
        ellipse.flags = 8;
        ellipse.controllist.resize(3);
        ellipse.controllist[0] = std::make_shared<DRW_Coord>(1.0, 0.0);
        ellipse.controllist[1] = std::make_shared<DRW_Coord>(1.0, 1.0);
        ellipse.controllist[2] = std::make_shared<DRW_Coord>(0.0, 1.0);
        double w_ell = 1.0 / std::sqrt(2.0);
        ellipse.weightlist = {1.0, w_ell, 1.0};
        ellipse.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

        REQUIRE(LC_HyperbolaSpline::splineToHyperbola(ellipse, nullptr) == nullptr);
    }
}
