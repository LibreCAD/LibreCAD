/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 xAI
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm> // For std::min_element, std::max_element, std::transform, std::fill, std::copy, std::fill_n
#include <cmath>
#include <numeric>   // For std::iota
#include <vector>

#include "rs_debug.h"
#include "rs_math.h"
#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"

// Stub for LibreCAD types (for standalone test compilation)


// Stub evaluateNURBS (for remove checks)
RS_Vector evaluateNURBS(const RS_SplineData& data, double t) {
    return RS_Vector(0.0, 0.0); // Stub, adjust if needed for tests
}

// Stub LC_SplineHelper functions (implement as per updates)
namespace {

const double TOLERANCE = RS_TOLERANCE;

// Helper to compare vectors within tolerance
bool vectorsEqual(const std::vector<double>& a, const std::vector<double>& b, double tol = TOLERANCE) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i] - b[i]) > tol) return false;
    }
    return true;
}

// Unified helper to create RS_SplineData (concise with flags)
RS_SplineData createSplineData(int degree, bool closed, size_t numControlPoints, bool rational = false, bool customKnots = false) {
    RS_SplineData data;
    data.degree = degree;
    data.type = closed ? RS_SplineData::SplineType::WrappedClosed : RS_SplineData::SplineType::ClampedOpen;
    size_t actualPoints = closed ? numControlPoints - degree : numControlPoints;
    data.controlPoints.resize(actualPoints);
    data.weights.resize(actualPoints, 1.0);
    for (size_t i = 0; i < actualPoints; ++i) {
        data.controlPoints[i] = RS_Vector(static_cast<double>(i), static_cast<double>(i));
        if (rational) data.weights[i] = 1.0 + static_cast<double>(i) * 0.1;
    }
    data.knotslist = customKnots ? std::vector<double>{0.0, 0.0, 0.0, 0.5, 1.5, 3.0, 3.0, 3.0} : LC_SplineHelper::knot(actualPoints, degree + 1);
    if (closed) {
        LC_SplineHelper::addWrapping(data, true);
        LC_SplineHelper::updateKnotWrapping(data, true, actualPoints);
    }
    return data;
}

} // namespace

TEST_CASE("ConvertClosedToOpenKnotVector", "[LC_SplineHelperTest]") {
    std::vector<double> closedKnots = {0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 3.0, 3.0, 4.0, 5.0, 6.0};
    size_t unwrappedSize = 5;
    size_t degree = 2;
    std::vector<double> expected = {0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 3.0, 3.0};
    auto result = LC_SplineHelper::convertClosedToOpenKnotVector(closedKnots, unwrappedSize, degree);
    REQUIRE(vectorsEqual(result, expected));
}

TEST_CASE("GetNormalizedKnotVector", "[LC_SplineHelperTest]") {
    std::vector<double> knots = {2.0, 3.0, 4.0, 5.0};
    std::vector<double> expected = {0.0, 1.0, 2.0, 3.0};
    auto result = LC_SplineHelper::getNormalizedKnotVector(knots, 0.0, {});
    REQUIRE(vectorsEqual(result, expected, 1e-6));
    
    // Test invalid input (too few knots)
    knots = {1.0};
    result = LC_SplineHelper::getNormalizedKnotVector(knots, 0.0, {0.0, 1.0});
    REQUIRE(vectorsEqual(result, {0.0, 1.0}));
}

TEST_CASE("ClampKnotVector", "[LC_SplineHelperTest]") {
    std::vector<double> knots = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    size_t numControl = 4;
    size_t order = 3;
    std::vector<double> expected = {2.0, 2.0, 2.0, 3.0, 4.0, 4.0, 4.0};
    auto result = LC_SplineHelper::clampKnotVector(knots, numControl, order);
    REQUIRE(vectorsEqual(result, expected));
}

TEST_CASE("UnclampKnotVector", "[LC_SplineHelperTest]") {
    std::vector<double> knots = {0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 2.0};
    size_t numControl = 4;
    size_t order = 3;
    std::vector<double> expected = {-1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 3.0};
    auto result = LC_SplineHelper::unclampKnotVector(knots, numControl, order);
    REQUIRE(vectorsEqual(result, expected));
}

TEST_CASE("ToClampedOpenFromStandard", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, false, 5);
    data.type = RS_SplineData::SplineType::Standard;
    data.knotslist = LC_SplineHelper::openUniformKnot(5, 3);
    auto originalKnots = data.knotslist;
    
    LC_SplineHelper::toClampedOpenFromStandard(data, 5);
    REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
    REQUIRE(vectorsEqual(data.savedOpenKnots, originalKnots));
    REQUIRE(vectorsEqual(data.knotslist, LC_SplineHelper::knot(5, 3)));
}

TEST_CASE("ToStandardFromClampedOpen", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, false, 5);
    data.savedOpenKnots = LC_SplineHelper::openUniformKnot(5, 3);
    LC_SplineHelper::toStandardFromClampedOpen(data, 5);
    REQUIRE(data.type == RS_SplineData::SplineType::Standard);
    REQUIRE(data.savedOpenKnots.empty());
    REQUIRE(vectorsEqual(data.knotslist, LC_SplineHelper::openUniformKnot(5, 3)));
}

TEST_CASE("ToWrappedClosedFromClampedOpen", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, false, 5);
    LC_SplineHelper::toWrappedClosedFromClampedOpen(data, 5);
    REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
    REQUIRE(data.controlPoints.size() == 7); // 5 + degree (2)
    REQUIRE(data.weights.size() == 7);
    REQUIRE(data.knotslist.size() == 10); // 5 + 2 + 3
    for (size_t i = 0; i < 2; ++i) {
        REQUIRE(data.controlPoints[i] == data.controlPoints[5 + i]);
    }
}

TEST_CASE("ToClampedOpenFromWrappedClosed", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, true, 7);
    size_t unwrappedSize = 5;
    LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrappedSize);
    REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
    REQUIRE(data.controlPoints.size() == 5);
    REQUIRE(data.weights.size() == 5);
    REQUIRE(data.knotslist.size() == 8);
}

TEST_CASE("Validate", "[LC_SplineHelperTest]") {
    // Valid spline
    RS_SplineData data = createSplineData(2, false, 5);
    REQUIRE(LC_SplineHelper::validate(data, 5));

    // Invalid degree
    data.degree = 0;
    REQUIRE_FALSE(LC_SplineHelper::validate(data, 5));
    data.degree = 2;

    // Invalid control points size
    data.controlPoints.resize(3);
    REQUIRE_FALSE(LC_SplineHelper::validate(data, 5));

    // Invalid weights size
    data.controlPoints.resize(5);
    data.weights.resize(3);
    REQUIRE_FALSE(LC_SplineHelper::validate(data, 5));

    // Non-monotonic knots
    data.weights.resize(5);
    data.knotslist[2] = data.knotslist[1] - 0.1;
    REQUIRE_FALSE(LC_SplineHelper::validate(data, 5));
}

TEST_CASE("InsertKnotBoehm", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, false, 5);
    size_t initialKnots = data.knotslist.size();
    size_t initialControls = data.controlPoints.size();
    double t = 1.5; // Insert knot in middle
    LC_SplineHelper::insertKnotBoehm(data, t);
    REQUIRE(data.knotslist.size() == initialKnots + 1);
    REQUIRE(data.controlPoints.size() == initialControls + 1);
    REQUIRE(data.weights.size() == initialControls + 1);
    REQUIRE(LC_SplineHelper::validate(data, 6));
}

TEST_CASE("RemoveKnotBoehm", "[LC_SplineHelperTest]") {
    RS_SplineData data = createSplineData(2, false, 5);
    LC_SplineHelper::insertKnotBoehm(data, 1.5); // Insert to ensure removable knot
    size_t initialKnots = data.knotslist.size();
    size_t initialControls = data.controlPoints.size();
    size_t r = data.degree + 2; // Correct index for the inserted knot (4 for degree=2)
    bool removed = LC_SplineHelper::removeKnotBoehm(data, r);
    REQUIRE( removed );
    REQUIRE( data.knotslist.size() == initialKnots - 1 );
    REQUIRE( data.controlPoints.size() == initialControls - 1 );
    REQUIRE( data.weights.size() == initialControls - 1 );
    REQUIRE( LC_SplineHelper::validate(data, 5) );
}

TEST_CASE("ComputeAveragedKnots", "[LC_SplineHelperTest]") {
    std::vector<double> params = {0.0, 0.2, 0.5, 0.8, 1.0}; // n=4
    int degree = 2;
    std::vector<double> expected = {0.0, 0.0, 0.0, 0.35, 0.65, 1.0, 1.0, 1.0}; // Averaged internals: (0.2+0.5)/2=0.35, (0.5+0.8)/2=0.65
    auto result = LC_SplineHelper::computeAveragedKnots(params, degree);
    REQUIRE(vectorsEqual(result, expected, 1e-6));
}

TEST_CASE("ComputeAveragedKnots_Degree1", "[LC_SplineHelperTest]") {
    std::vector<double> params = {0.0, 0.3, 1.0}; // n=2
    int degree = 1;
    std::vector<double> expected = {0.0, 0.0, 0.3, 1.0, 1.0};
    auto result = LC_SplineHelper::computeAveragedKnots(params, degree);
    REQUIRE(vectorsEqual(result, expected, 1e-6));
}

TEST_CASE("ComputeAveragedKnots_Edge", "[LC_SplineHelperTest]") {
    std::vector<double> params = {0.0, 1.0}; // Minimal n=1
    int degree = 1;
    std::vector<double> expected = {0.0, 0.0, 1.0, 1.0};
    auto result = LC_SplineHelper::computeAveragedKnots(params, degree);
    REQUIRE(vectorsEqual(result, expected));
}