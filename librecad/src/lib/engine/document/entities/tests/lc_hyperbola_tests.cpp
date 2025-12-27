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

// File: lc_hyperbola_tests.cpp
// Updated Catch2 unit tests for hyperbola ↔ DRW_Spline round-trip conversion

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "lc_hyperbola.h"
#include "lc_hyperbolaspline.h"
#include "drw_entities.h"
#include "rs_math.h"

constexpr double TOL = 1e-8;
constexpr double ANGLE_TOL = 1e-6;

// Helper to compare hyperbola data
bool hyperbolaDataApproxEqual(const LC_HyperbolaData& a, const LC_HyperbolaData& b)
{
    return (a.center - b.center).squared() < TOL * TOL &&
           RS_Math::equal(a.majorP.magnitude(), b.majorP.magnitude(), TOL) &&
           RS_Math::equal(a.ratio, b.ratio, TOL) &&
           a.reversed == b.reversed &&
           RS_Math::equal(RS_Math::getAngleDifference(a.majorP.angle(), b.majorP.angle()), 0.0, ANGLE_TOL) &&
           RS_Math::equal(a.angle1, b.angle1, ANGLE_TOL) &&
           RS_Math::equal(a.angle2, b.angle2, ANGLE_TOL);
}

TEST_CASE("Hyperbola ↔ DRW_Spline round-trip tests", "[hyperbola][spline][roundtrip]")
{
    SECTION("Right branch bounded arc: analytical shoulder validation")
    {
        LC_HyperbolaData original;
        original.center = RS_Vector(0.0, 0.0);
        original.majorP = RS_Vector(2.0, 0.0);  // a = 2
        original.ratio = 0.5;                   // b = 1
        original.reversed = false;

        original.angle1 = -1.0;
        original.angle2 =  1.5;

        DRW_Spline spl;
        REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

        // Basic spline structure checks
        REQUIRE(spl.degree == 2);
        REQUIRE(spl.flags == 8);
        REQUIRE(spl.controllist.size() == 3);
        REQUIRE(spl.weightlist.size() == 3);
        REQUIRE(spl.knotslist.size() == 6);

        // Endpoint weights must be 1.0
        REQUIRE(RS_Math::equal(spl.weightlist[0], 1.0));
        REQUIRE(RS_Math::equal(spl.weightlist[2], 1.0));
        REQUIRE(spl.weightlist[1] > 1.0);  // middle weight > 1 for proper hyperbola

        // Round-trip recovery
        auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
        REQUIRE(recovered != nullptr);
        REQUIRE(recovered->isValid());

        const LC_HyperbolaData& rec = recovered->getData();
        REQUIRE(hyperbolaDataApproxEqual(rec, original));

        // Parametric range should be preserved
        REQUIRE(RS_Math::equal(rec.angle1, original.angle1, ANGLE_TOL));
        REQUIRE(RS_Math::equal(rec.angle2, original.angle2, ANGLE_TOL));
    }

    SECTION("Rotated and translated bounded arc")
    {
        LC_HyperbolaData original;
        original.center = RS_Vector(10.0, 20.0);
        original.majorP = RS_Vector(4.0, 0.0).rotate(M_PI / 6.0);  // 30° rotation
        original.ratio = 0.75;
        original.reversed = false;
        original.angle1 = -0.8;
        original.angle2 = 1.2;

        DRW_Spline spl;
        REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

        auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
        REQUIRE(recovered != nullptr);
        REQUIRE(recovered->isValid());

        const LC_HyperbolaData& rec = recovered->getData();
        REQUIRE(hyperbolaDataApproxEqual(rec, original));
    }

    SECTION("Very small arc near vertex")
    {
        LC_HyperbolaData original;
        original.center = RS_Vector(0.0, 0.0);
        original.majorP = RS_Vector(1.0, 0.0);
        original.ratio = 0.3;
        original.reversed = false;
        original.angle1 = -0.1;
        original.angle2 =  0.1;

        DRW_Spline spl;
        REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

        // Middle weight should be close to 1 (almost parabolic behavior near vertex)
        REQUIRE(spl.weightlist[1] > 1.0);
        REQUIRE(spl.weightlist[1] < 1.1);  // small deviation

        auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
        REQUIRE(recovered != nullptr);
        REQUIRE(recovered->isValid());

        REQUIRE(hyperbolaDataApproxEqual(recovered->getData(), original));
    }

    SECTION("Large parameter range (tests numerical stability)")
    {
        LC_HyperbolaData original;
        original.center = RS_Vector(0.0, 0.0);
        original.majorP = RS_Vector(1.0, 0.0);
        original.ratio = 0.6;
        original.reversed = false;
        original.angle1 = -3.0;
        original.angle2 =  4.0;

        DRW_Spline spl;
        REQUIRE(LC_HyperbolaSpline::hyperbolaToSpline(original, spl));

        // Middle weight will be large due to wide range
        REQUIRE(spl.weightlist[1] > 10.0);

        auto recovered = LC_HyperbolaSpline::splineToHyperbola(spl, nullptr);
        REQUIRE(recovered != nullptr);
        REQUIRE(recovered->isValid());

        REQUIRE(hyperbolaDataApproxEqual(recovered->getData(), original));
    }
}
