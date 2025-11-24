/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 librecad.org
** Copyright (C) 2025 Dongxu Li (github.com/dxli)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
// File: rs_spline_tests.cpp

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "rs_debug.h"
#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"
#include "rs_math.h"

using Catch::Approx;

namespace {
    bool compareVector(const RS_Vector& va, const RS_Vector& vb, double tol = 1e-4) {
        return va.distanceTo(vb) <= tol;
    }
}

TEST_CASE("RS_Spline Basic Functionality", "[RS_Spline]")
{
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    SECTION("Construction and Getters")
    {
        REQUIRE(spline.getDegree() == 3);
        REQUIRE(!spline.isClosed());
        REQUIRE(spline.getNumberOfControlPoints() == 0);
        REQUIRE(spline.getNumberOfKnots() == 0);
    }

    SECTION("Set Degree")
    {
        spline.setDegree(2);
        REQUIRE(spline.getDegree() == 2);

        REQUIRE_THROWS_AS(spline.setDegree(0), std::invalid_argument);
        REQUIRE_THROWS_AS(spline.setDegree(4), std::invalid_argument);
    }
}


TEST_CASE("Non-uniform knot vectors - validation and type handling", "[RS_Spline][nonuniform]")
{
    SECTION("ClampedOpen non-uniform knots - valid")
    {
        RS_SplineData d(3, false);
        d.type = RS_SplineData::SplineType::ClampedOpen;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(10,20), RS_Vector(30,30), RS_Vector(50,20), RS_Vector(60,0), RS_Vector(70,10), RS_Vector(80,0)
        };
        d.weights = {1.0, 2.0, 1.5, 1.0, 1.0, 1.2, 1.0};

        d.knotslist = {0.0, 0.0, 0.0, 0.0, 8.0, 25.0, 55.0, 100.0, 100.0, 100.0, 100.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == true);
        REQUIRE(s.isClosed() == false);
        REQUIRE(s.getDegree() == 3);
        REQUIRE(s.getNumberOfControlPoints() == 7);
    }

    SECTION("ClampedOpen non-uniform - invalid (wrong end multiplicity)")
    {
        RS_SplineData d(3, false);
        d.type = RS_SplineData::SplineType::ClampedOpen;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(10,20), RS_Vector(30,30), RS_Vector(50,20), RS_Vector(60,0), RS_Vector(70,10), RS_Vector(80,0)
        };
        d.weights = {1.0, 2.0, 1.5, 1.0, 1.0, 1.2, 1.0};

        // Note: end multiplicity is only 3 instead of 4 → invalid for ClampedOpen
        d.knotslist = {0.0, 0.0, 0.0, 0.0, 8.0, 25.0, 55.0, 90.0, 100.0, 100.0, 100.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == false);
    }

    SECTION("Standard (open non-clamped non-uniform) - valid")
    {
        RS_SplineData d(3, false);
        d.type = RS_SplineData::SplineType::Standard;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(15,25), RS_Vector(40,35), RS_Vector(80,0)
        };
        d.weights.assign(4, 1.0);
        d.knotslist = {0.0, 12.0, 35.0, 60.0, 100.0, 140.0, 180.0, 220.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == true);
    }

    SECTION("Standard non-uniform - invalid (accidental clamping at start)")
    {
        RS_SplineData d(3, false);
        d.type = RS_SplineData::SplineType::Standard;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(15,25), RS_Vector(40,35), RS_Vector(80,0)
        };
        d.weights.assign(4, 1.0);
        d.knotslist = {0.0, 0.0, 0.0, 0.0, 20.0, 50.0, 100.0, 150.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == false);
    }

    SECTION("WrappedClosed non-uniform knots - valid")
    {
        RS_SplineData d(3, true);
        d.type = RS_SplineData::SplineType::WrappedClosed;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(15,25), RS_Vector(40,35), RS_Vector(70,20), RS_Vector(80,0),
            RS_Vector(0,0), RS_Vector(15,25), RS_Vector(40,35)
        };
        d.weights = {1.0, 1.5, 2.0, 1.5, 1.0, 1.0, 1.5, 2.0};
        d.knotslist = {0.0, 12.0, 35.0, 60.0, 100.0, 140.0, 180.0, 220.0, 260.0, 290.0, 320.0, 350.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == true);
        REQUIRE(s.isClosed() == true);
        REQUIRE(s.hasWrappedControlPoints() == true);
    }

    SECTION("WrappedClosed non-uniform - invalid (clamped-style ends)")
    {
        RS_SplineData d(3, true);
        d.type = RS_SplineData::SplineType::WrappedClosed;
        d.controlPoints.resize(8);
        d.weights.assign(8, 1.0);
        d.knotslist = {0.0, 0.0, 0.0, 0.0, 20.0, 50.0, 100.0, 150.0, 150.0, 150.0, 150.0};

        RS_Spline s(nullptr, d);
        REQUIRE(s.validate() == false);
    }

    SECTION("WrappedClosed non-uniform - valid (missing wrapped control points)")
    {
        RS_SplineData d(3, true);
        d.type = RS_SplineData::SplineType::WrappedClosed;
        d.controlPoints = {
            RS_Vector(0,0), RS_Vector(10,10), RS_Vector(20,10), RS_Vector(30,10), RS_Vector(40,0),
            RS_Vector(99,99), RS_Vector(99,99), RS_Vector(99,99)
        };
        d.weights.assign(8, 1.0);
        d.knotslist = {0.0, 10.0, 20.0, 35.0, 55.0, 80.0, 110.0, 140.0, 170.0, 200.0, 230.0, 260.0};

        // contor will add control point wrapping
        RS_Spline s(nullptr, d);
        REQUIRE(s.validate());
    }
}
