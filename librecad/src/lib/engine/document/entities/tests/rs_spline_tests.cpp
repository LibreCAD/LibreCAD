#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"
#include "rs_math.h"

/**
 * Validates the spline data integrity, checking sizes, monotonicity, multiplicities,
 * positive weights, and minimum control points.
 *
 * @return true if valid, false otherwise (logs warnings via RS_DEBUG).
 */
bool RS_Spline::validate() const {
    size_t numControls = getUnwrappedSize();
    size_t expectedKnots = numControls + data.degree + 1;
    size_t numWeights = data.weights.size();

    // Check minimum controls
    if (numControls < data.degree + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Insufficient control points (need >= degree + 1)");
        return false;
    }

    // Check vector sizes
    if (data.knotslist.size() != expectedKnots) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot vector size mismatch (expected %zu, got %zu)", expectedKnots, data.knotslist.size());
        return false;
    }
    if (numWeights != data.controlPoints.size()) {  // Full size, including wrapping
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Weights size mismatch with controls");
        return false;
    }

    // Check knot monotonicity (non-decreasing)
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        if (data.knotslist[i] < data.knotslist[i - 1] - RS_TOLERANCE) {  // Allow approx equal for floats
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot vector not monotonic at index %zu", i);
            return false;
        }
    }

    // Check knot multiplicities (<= degree)
    size_t mult = 1;
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        if (fabs(data.knotslist[i] - data.knotslist[i - 1]) < RS_TOLERANCE) {
            ++mult;
        } else {
            if (mult > data.degree) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot multiplicity exceeds degree (%zu > %zu)", mult, data.degree);
                return false;
            }
            mult = 1;
        }
    }
    if (mult > data.degree) {  // Check last group
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot multiplicity exceeds degree at end");
        return false;
    }

    // Check positive weights (for rational stability; 0 or negative cause division issues)
    for (double w : data.weights) {
        if (w <= 0.0) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Non-positive weight found (%f)", w);
            return false;
        }
    }

    // Type-specific checks (e.g., wrapping consistency)
    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        if (!hasWrappedControlPoints()) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: WrappedClosed but no wrapping detected");
            return false;
        }
        // Verify last degree points/weights match first (within tolerance)
        for (size_t i = 0; i < data.degree; ++i) {
            if (!compareVector(data.controlPoints[numControls + i], data.controlPoints[i])) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Wrapped controls mismatch at index %zu", i);
                return false;
            }
            if (fabs(data.weights[numControls + i] - data.weights[i]) > RS_TOLERANCE) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Wrapped weights mismatch at index %zu", i);
                return false;
            }
        }
    }

    // ClampedOpen: Check endpoint multiplicities (p+1 at ends)
    if (data.type == RS_SplineData::SplineType::ClampedOpen) {
        size_t startMult = 0, endMult = 0;
        double startVal = data.knotslist[0];
        for (size_t i = 1; i < data.knotslist.size() && fabs(data.knotslist[i] - startVal) < RS_TOLERANCE; ++i) ++startMult;
        double endVal = data.knotslist.back();
        for (size_t i = data.knotslist.size() - 2; i > 0 && fabs(data.knotslist[i] - endVal) < RS_TOLERANCE; --i) ++endMult;
        if (startMult != data.degree + 1 || endMult != data.degree + 1) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: ClampedOpen endpoints multiplicity incorrect (expected %zu)", data.degree + 1);
            return false;
        }
    }

    return true;
}

TEST_CASE("RS_Spline Validation", "[RS_Spline]") {
    RS_SplineData data(3, false);
    data.controlPoints = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,0), RS_Vector(3,1)};
    data.weights = {1.0, 1.0, 1.0, 1.0};
    data.knotslist = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    RS_Spline spline(nullptr, data);

    SECTION("Valid Standard Spline") {
        REQUIRE(spline.validate());
    }

    SECTION("Invalid Degree") {
        data.degree = 0;
        REQUIRE(!spline.validate());
    }

    SECTION("Insufficient Controls") {
        data.controlPoints.resize(3);
        data.weights.resize(3);
        data.knotslist.resize(7);
        REQUIRE(!spline.validate());
    }

    SECTION("Size Mismatch - Knots") {
        data.knotslist.resize(6);
        REQUIRE(!spline.validate());
    }

    SECTION("Size Mismatch - Weights") {
        data.weights.resize(3);
        REQUIRE(!spline.validate());
    }

    SECTION("Non-Monotonic Knots") {
        data.knotslist = {0.0, 1.0, 3.0, 2.0, 4.0, 5.0, 6.0, 7.0};
        REQUIRE(!spline.validate());
    }

    SECTION("Excess Multiplicity") {
        data.knotslist = {0.0, 0.0, 0.0, 0.0, 0.0, 4.0, 5.0, 6.0};
        REQUIRE(!spline.validate());
    }

    SECTION("Non-Positive Weight") {
        data.weights[1] = 0.0;
        REQUIRE(!spline.validate());
    }

    SECTION("WrappedClosed - No Wrapping") {
        data.type = RS_SplineData::SplineType::WrappedClosed;
        REQUIRE(!spline.validate());
    }

    SECTION("WrappedClosed - Mismatch Wrapping") {
        data.type = RS_SplineData::SplineType::WrappedClosed;
        data.controlPoints = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,0), RS_Vector(3,1), RS_Vector(0,0), RS_Vector(1,1), RS_Vector(4,0)};
        data.weights = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        data.knotslist = {0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0};
        REQUIRE(!spline.validate());
    }

    SECTION("ClampedOpen - Incorrect Multiplicity") {
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.knotslist = {0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 4.0};  // Start 3, end 2
        REQUIRE(!spline.validate());
    }
}