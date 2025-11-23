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
// File: lc_splinehelper_tests.cpp

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

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

TEST_CASE("LC_SplineHelper Knot Conversions", "[LC_SplineHelper]") {
    size_t unwrappedControlCount = 4;
    size_t splineDegree = 3;

    SECTION("Convert Closed to Open - Uniform") {
        std::vector<double> closedKnots = {0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0};
        auto openKnots = LC_SplineHelper::convertClosedToOpenKnotVector(closedKnots, unwrappedControlCount, splineDegree);
        REQUIRE(openKnots.size() == 8);
        REQUIRE(openKnots[0] == Approx(0.0));
        REQUIRE(openKnots[1] == Approx(1.0));
        REQUIRE(openKnots[2] == Approx(2.0));
        REQUIRE(openKnots[3] == Approx(3.0));
        REQUIRE(openKnots[4] == Approx(4.0));
        REQUIRE(openKnots[5] == Approx(5.0));
        REQUIRE(openKnots[6] == Approx(6.0));
        REQUIRE(openKnots[7] == Approx(7.0));
    }

    SECTION("Convert Closed to Open - Non-Uniform") {
        std::vector<double> closedKnots = {-3.0, -1.5, -0.5, 0.0, 1.0, 3.0, 5.0, 6.0, 7.5, 8.5, 10.0};
        auto openKnots = LC_SplineHelper::convertClosedToOpenKnotVector(closedKnots, unwrappedControlCount, splineDegree);
        REQUIRE(openKnots.size() == 8);
        REQUIRE(openKnots[0] == Approx(0.0));
        REQUIRE(openKnots[1] == Approx(1.5));
        REQUIRE(openKnots[2] == Approx(3.0));
        REQUIRE(openKnots[3] == Approx(4.5));
        REQUIRE(openKnots[4] == Approx(6.5));
        REQUIRE(openKnots[5] == Approx(8.0));
        REQUIRE(openKnots[6] == Approx(9.0));
        REQUIRE(openKnots[7] == Approx(10.5));
    }


    SECTION("Convert Open to Closed - Non-Clamped Non-Uniform") {
        std::vector<double> openKnots = {0.0, 0.5, 1.0, 2.0, 3.0, 4.5, 6.0, 8.0};
        auto closedKnots = LC_SplineHelper::convertOpenToClosedKnotVector(openKnots, unwrappedControlCount, splineDegree);
        REQUIRE(closedKnots.size() == 11);
        REQUIRE(closedKnots[0] == Approx(0.0));
        REQUIRE(closedKnots[1] == Approx(0.5));
        REQUIRE(closedKnots[2] == Approx(1.0));
        REQUIRE(closedKnots[3] == Approx(2.0));
        REQUIRE(closedKnots[4] == Approx(3.0));
        REQUIRE(closedKnots[5] == Approx(3.5));
        REQUIRE(closedKnots[6] == Approx(4.0));
        REQUIRE(closedKnots[7] == Approx(5.0));
        REQUIRE(closedKnots[8] == Approx(6.0));
        REQUIRE(closedKnots[9] == Approx(6.5));
        REQUIRE(closedKnots[10] == Approx(7.0));
    }

    SECTION("Convert Open to Closed - Edge Case: Invalid Period") {
        std::vector<double> invalidOpen = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        auto closedKnots = LC_SplineHelper::convertOpenToClosedKnotVector(invalidOpen, unwrappedControlCount, splineDegree);
        REQUIRE(closedKnots.empty());
    }

    SECTION("Normalize Knot Vector") {
        std::vector<double> inputKnots = {2.0, 3.0, 4.0};
        std::vector<double> fallbackKnots = {0.0, 1.0};
        auto normalizedKnots = LC_SplineHelper::getNormalizedKnotVector(inputKnots, 0.0, fallbackKnots);
        REQUIRE(normalizedKnots.size() == 3);
        REQUIRE(normalizedKnots[0] == Approx(0.0));
        REQUIRE(normalizedKnots[1] == Approx(1.0));
        REQUIRE(normalizedKnots[2] == Approx(2.0));
    }

    SECTION("Normalize Knot Vector - Fallback") {
        std::vector<double> inputKnots = {2.0};
        std::vector<double> fallbackKnots = {0.0, 1.0, 2.0};
        auto normalizedKnots = LC_SplineHelper::getNormalizedKnotVector(inputKnots, 0.0, fallbackKnots);
        REQUIRE(normalizedKnots == fallbackKnots);
    }

    SECTION("Normalize Knot Vector - Edge Case: Empty Input") {
        std::vector<double> inputKnots = {};
        std::vector<double> fallbackKnots = {0.0, 1.0};
        auto normalizedKnots = LC_SplineHelper::getNormalizedKnotVector(inputKnots, 0.0, fallbackKnots);
        REQUIRE(normalizedKnots == fallbackKnots);
    }

    SECTION("Clamp and Unclamp - Non-Uniform") {
        std::vector<double> uniformKnots = {0.0, 0.5, 1.0, 2.0, 3.0, 4.0, 4.5, 5.0};
        size_t controlPointCount = 4;
        size_t splineOrder = 4;
        auto clampedKnots = LC_SplineHelper::clampKnotVector(uniformKnots, controlPointCount, splineOrder);
        REQUIRE(clampedKnots[0] == Approx(2.0));
        REQUIRE(clampedKnots[1] == Approx(2.0));
        REQUIRE(clampedKnots[2] == Approx(2.0));
        REQUIRE(clampedKnots[3] == Approx(2.0));
        REQUIRE(clampedKnots[4] == Approx(3.0));
        REQUIRE(clampedKnots[5] == Approx(3.0));
        REQUIRE(clampedKnots[6] == Approx(3.0));
        REQUIRE(clampedKnots[7] == Approx(3.0));

        auto unclampedKnots = LC_SplineHelper::unclampKnotVector(clampedKnots, controlPointCount, splineOrder);
        REQUIRE(unclampedKnots[0] == Approx(-1.0));
        REQUIRE(unclampedKnots[1] == Approx(0.0));
        REQUIRE(unclampedKnots[2] == Approx(1.0));
        REQUIRE(unclampedKnots[3] == Approx(2.0));
        REQUIRE(unclampedKnots[4] == Approx(3.0));
        REQUIRE(unclampedKnots[5] == Approx(4.0));
        REQUIRE(unclampedKnots[6] == Approx(5.0));
        REQUIRE(unclampedKnots[7] == Approx(6.0));
    }

    SECTION("Clamp and Unclamp - Uniform") {
        std::vector<double> uniformKnots = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
        size_t controlPointCount = 4;
        size_t splineOrder = 4;
        auto clampedKnots = LC_SplineHelper::clampKnotVector(uniformKnots, controlPointCount, splineOrder);
        REQUIRE(clampedKnots[0] == Approx(3.0));
        REQUIRE(clampedKnots[1] == Approx(3.0));
        REQUIRE(clampedKnots[2] == Approx(3.0));
        REQUIRE(clampedKnots[3] == Approx(3.0));
        REQUIRE(clampedKnots[4] == Approx(4.0));
        REQUIRE(clampedKnots[5] == Approx(4.0));
        REQUIRE(clampedKnots[6] == Approx(4.0));
        REQUIRE(clampedKnots[7] == Approx(4.0));

        auto unclampedKnots = LC_SplineHelper::unclampKnotVector(clampedKnots, controlPointCount, splineOrder);
        REQUIRE(unclampedKnots[0] == Approx(0.0));
        REQUIRE(unclampedKnots[1] == Approx(1.0));
        REQUIRE(unclampedKnots[2] == Approx(2.0));
        REQUIRE(unclampedKnots[3] == Approx(3.0));
        REQUIRE(unclampedKnots[4] == Approx(4.0));
        REQUIRE(unclampedKnots[5] == Approx(5.0));
        REQUIRE(unclampedKnots[6] == Approx(6.0));
        REQUIRE(unclampedKnots[7] == Approx(7.0));
    }

    SECTION("Clamp and Unclamp - Edge Case: Constant Knots") {
        std::vector<double> constantKnots = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
        size_t controlPointCount = 4;
        size_t splineOrder = 4;
        auto clampedKnots = LC_SplineHelper::clampKnotVector(constantKnots, controlPointCount, splineOrder);
        REQUIRE(clampedKnots == constantKnots);

        auto unclampedKnots = LC_SplineHelper::unclampKnotVector(clampedKnots, controlPointCount, splineOrder);
        REQUIRE(unclampedKnots[0] == Approx(2.0));
        REQUIRE(unclampedKnots[1] == Approx(3.0));
        REQUIRE(unclampedKnots[2] == Approx(4.0));
        REQUIRE(unclampedKnots[3] == Approx(5.0));
        REQUIRE(unclampedKnots[4] == Approx(5.0));
        REQUIRE(unclampedKnots[5] == Approx(6.0));
        REQUIRE(unclampedKnots[6] == Approx(7.0));
        REQUIRE(unclampedKnots[7] == Approx(8.0));
    }
}

TEST_CASE("LC_SplineHelper Type Conversions", "[LC_SplineHelper]") {
    RS_SplineData splineData(3, false);
    splineData.type = RS_SplineData::SplineType::Standard;
    splineData.controlPoints = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,0), RS_Vector(3,1)};
    splineData.weights = {1.0, 1.0, 1.0, 1.0};
    splineData.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(4, 4);

    SECTION("Validation") {
        REQUIRE(LC_SplineHelper::validate(splineData));
    }

    SECTION("To Clamped Open From Standard") {
        LC_SplineHelper::toClampedOpenFromStandard(splineData);
        REQUIRE(splineData.type == RS_SplineData::SplineType::ClampedOpen);
        REQUIRE(splineData.knotslist[0] == splineData.knotslist[1]);
        REQUIRE(splineData.knotslist[0] == splineData.knotslist[2]);
        REQUIRE(splineData.knotslist[0] == splineData.knotslist[3]);
        REQUIRE(splineData.knotslist.back() == splineData.knotslist[splineData.knotslist.size() - 2]);
        REQUIRE(splineData.knotslist.back() == splineData.knotslist[splineData.knotslist.size() - 3]);
        REQUIRE(splineData.knotslist.back() == splineData.knotslist[splineData.knotslist.size() - 4]);
    }

    SECTION("To Standard From Clamped Open") {
        splineData.type = RS_SplineData::SplineType::ClampedOpen;
        splineData.knotslist = LC_SplineHelper::clampKnotVector(splineData.knotslist, 4, 4);
        LC_SplineHelper::toStandardFromClampedOpen(splineData);
        REQUIRE(splineData.type == RS_SplineData::SplineType::Standard);
        REQUIRE(splineData.knotslist[0] != splineData.knotslist[3]);
        REQUIRE(splineData.knotslist.back() != splineData.knotslist[splineData.knotslist.size() - 4]);
    }

    SECTION("To Wrapped Closed From Standard") {
        LC_SplineHelper::toWrappedClosedFromStandard(splineData);
        REQUIRE(splineData.type == RS_SplineData::SplineType::WrappedClosed);
        REQUIRE(splineData.controlPoints.size() == 7);
        REQUIRE(splineData.weights.size() == 7);
        REQUIRE(splineData.knotslist.size() == 11);
        REQUIRE(splineData.controlPoints[4] == splineData.controlPoints[0]);
        REQUIRE(splineData.weights[4] == splineData.weights[0]);
        REQUIRE(splineData.controlPoints[5] == splineData.controlPoints[1]);
        REQUIRE(splineData.weights[5] == splineData.weights[1]);
        REQUIRE(splineData.controlPoints[6] == splineData.controlPoints[2]);
        REQUIRE(splineData.weights[6] == splineData.weights[2]);
    }

    SECTION("Update Knot Wrapping") {
        size_t unwrappedCount = 4;
        LC_SplineHelper::updateKnotWrapping(splineData, true, unwrappedCount);
        REQUIRE(splineData.knotslist.size() == 11);
    }

    SECTION("To Wrapped Closed From Standard") {
        LC_SplineHelper::toWrappedClosedFromStandard(splineData);
        REQUIRE(splineData.type == RS_SplineData::SplineType::WrappedClosed);
        REQUIRE(splineData.controlPoints.size() == 7);
        REQUIRE(splineData.knotslist.size() == 11);
    }

    SECTION("To Clamped Open From Standard") {
        LC_SplineHelper::toClampedOpenFromStandard(splineData);
        REQUIRE(splineData.type == RS_SplineData::SplineType::ClampedOpen);
        REQUIRE(splineData.knotslist[0] == Approx(splineData.knotslist[3]));
        REQUIRE(splineData.knotslist.back() == Approx(splineData.knotslist[splineData.knotslist.size() - 4]));
    }

    SECTION("Round Trip Type Conversion") {
        auto originalKnots = splineData.knotslist;
        LC_SplineHelper::toClampedOpenFromStandard(splineData);
        LC_SplineHelper::toStandardFromClampedOpen(splineData);
        REQUIRE(splineData.knotslist == originalKnots);
    }
}

TEST_CASE("LC_SplineHelper Knot Generators and Manipulations", "[LC_SplineHelper]") {
    SECTION("Generate Clamped Uniform Knot Vector") {
        auto clampedKnots = LC_SplineHelper::knot(4, 4);
        REQUIRE(clampedKnots.size() == 8);
        REQUIRE(clampedKnots[0] == Approx(0.0));
        REQUIRE(clampedKnots[3] == Approx(0.0));
        REQUIRE(clampedKnots[4] == Approx(1.0));
        REQUIRE(clampedKnots[7] == Approx(1.0));
    }

    SECTION("Generate Clamped Uniform - Edge Case: Min Controls") {
        auto minClamped = LC_SplineHelper::knot(4, 4);
        REQUIRE(minClamped.size() == 8);
    }

    SECTION("Generate Open Uniform Knot Vector") {
        auto openUniformKnots = LC_SplineHelper::generateOpenUniformKnotVector(4, 4);
        REQUIRE(openUniformKnots.size() == 8);
        REQUIRE(openUniformKnots[0] == Approx(0.0));
        REQUIRE(openUniformKnots[1] == Approx(1.0));
        REQUIRE(openUniformKnots[7] == Approx(7.0));
    }

    SECTION("Extend Knot Vector") {
        std::vector<double> knots = {0.0, 1.0, 3.0};
        LC_SplineHelper::extendKnotVector(knots);
        REQUIRE(knots.size() == 4);
        REQUIRE(knots[3] > knots[2]);
    }

    SECTION("Extend Knot Vector - Edge Case: Single Knot") {
        std::vector<double> singleKnot = {0.0};
        LC_SplineHelper::extendKnotVector(singleKnot);
        REQUIRE(singleKnot.size() == 2);
        REQUIRE(singleKnot[1] == Approx(RS_TOLERANCE * 10));
    }

    SECTION("Insert Knot - Mid") {
        std::vector<double> knots = {0.0, 1.0, 2.0, 3.0};
        LC_SplineHelper::insertKnot(knots, 2);
        REQUIRE(knots.size() == 5);
        REQUIRE(knots[2] == Approx(1.5));
    }

    SECTION("Insert Knot - Start") {
        std::vector<double> knots = {0.0, 1.0, 2.0};
        LC_SplineHelper::insertKnot(knots, 0);
        REQUIRE(knots.size() == 4);
        REQUIRE(knots[0] == Approx(-1.0));
        REQUIRE(knots[1] == Approx(0.0));
    }

    SECTION("Insert Knot - End") {
        std::vector<double> knots = {0.0, 1.0, 2.0};
        LC_SplineHelper::insertKnot(knots, 3);
        REQUIRE(knots.size() == 4);
        REQUIRE(knots[3] == Approx(3.0));
    }

    SECTION("Insert Knot - Empty Vector") {
        std::vector<double> knots = {};
        LC_SplineHelper::insertKnot(knots, 0);
        REQUIRE(knots.size() == 1);
        REQUIRE(knots[0] == Approx(0.0));
    }

    SECTION("Insert Knot - Small Difference") {
        std::vector<double> knots = {0.0, 1e-12, 1.0};
        LC_SplineHelper::insertKnot(knots, 1);
        REQUIRE(knots.size() == 4);
        REQUIRE(knots[1] == Approx(1e-12 + 1e-10));
        REQUIRE(knots[0] == Approx(0.0));
        REQUIRE(knots[2] == Approx(1e-12));
        REQUIRE(knots[3] == Approx(1.0));
    }

    SECTION("Insert Knot - Multiple at Same Position") {
        std::vector<double> knots = {0.0, 1.0, 2.0};
        LC_SplineHelper::insertKnot(knots, 1);
        REQUIRE(knots[1] == Approx(0.5));
        LC_SplineHelper::insertKnot(knots, 1);
        REQUIRE(knots[1] == Approx(0.25));
        REQUIRE(knots[2] == Approx(0.5));
    }

    SECTION("Insert Knot - Beyond Size") {
        std::vector<double> knots = {0.0, 1.0};
        LC_SplineHelper::insertKnot(knots, 5);
        REQUIRE(knots.size() == 3);
        REQUIRE(knots[2] == Approx(2.0));
    }

    SECTION("Insert Knot - Negative Knots") {
        std::vector<double> knots = {-2.0, -1.0, 0.0};
        LC_SplineHelper::insertKnot(knots, 1);
        REQUIRE(knots.size() == 4);
        REQUIRE(knots[1] == Approx(-1.5));
    }

    SECTION("Remove Knot") {
        std::vector<double> knots = {0.0, 1.0, 2.0, 3.0};
        LC_SplineHelper::removeKnot(knots, 1);
        REQUIRE(knots.size() == 3);
        REQUIRE(knots[1] == Approx(2.0));
    }

    SECTION("Remove Knot - Edge Case: Empty") {
        std::vector<double> knots = {};
        LC_SplineHelper::removeKnot(knots, 0);
        REQUIRE(knots.empty());
    }

    SECTION("Remove Knot - Edge Case: Single Knot") {
        std::vector<double> knots = {0.0};
        LC_SplineHelper::removeKnot(knots, 0);
        REQUIRE(knots.empty());
    }

    SECTION("Ensure Monotonic - With Duplicates") {
        std::vector<double> knots = {0.0, 1.0, 1.0, 2.0};
        LC_SplineHelper::ensureMonotonic(knots);
        REQUIRE(knots[2] > knots[1]);
        REQUIRE(knots[2] == Approx(1.0 + RS_TOLERANCE * 10));
    }

    SECTION("Ensure Monotonic - Decreasing") {
        std::vector<double> knots = {0.0, 2.0, 1.0, 3.0};
        LC_SplineHelper::ensureMonotonic(knots);
        REQUIRE(knots[2] > knots[1]);
        REQUIRE(knots[2] == Approx(2.0 + RS_TOLERANCE * 10));
    }

    SECTION("Ensure Monotonic - All Equal") {
        std::vector<double> knots = {1.0, 1.0, 1.0};
        LC_SplineHelper::ensureMonotonic(knots);
        REQUIRE(knots[1] > knots[0]);
        REQUIRE(knots[2] > knots[1]);
    }
}

TEST_CASE("RS_Spline Cubic Specific Tests", "[RS_Spline][degree3]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

}
