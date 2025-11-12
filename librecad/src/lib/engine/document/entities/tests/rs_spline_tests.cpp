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

TEST_CASE("RS_Spline Basic Functionality", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    SECTION("Construction and Getters") {
        REQUIRE(spline.getDegree() == 3);
        REQUIRE(!spline.isClosed());
        REQUIRE(spline.getNumberOfControlPoints() == 0);
        REQUIRE(spline.getNumberOfKnots() == 0);
    }

    SECTION("Set Degree") {
        spline.setDegree(2);
        REQUIRE(spline.getDegree() == 2);

        REQUIRE_THROWS_AS(spline.setDegree(0), std::invalid_argument);
        REQUIRE_THROWS_AS(spline.setDegree(4), std::invalid_argument);
    }

    SECTION("Add Control Points - Open Spline") {
        RS_Vector point1(0.0, 0.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(2.0, 0.0);
        RS_Vector point4(3.0, 1.0);

        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        spline.addControlPoint(point4);

        REQUIRE(spline.getNumberOfControlPoints() == 4);
        auto controlPoints = spline.getControlPoints();
        REQUIRE(controlPoints.size() == 4);
        REQUIRE(controlPoints[0] == point1);
        REQUIRE(controlPoints[3] == point4);

        // Default knots should be generated (clamped uniform)
        auto knotVector = spline.getKnotVector();
        REQUIRE(knotVector.size() == 8); // n + degree + 1 = 4 + 3 + 1
        REQUIRE(knotVector[0] == Approx(0.0));
        REQUIRE(knotVector[3] == Approx(0.0));
        REQUIRE(knotVector[4] == Approx(1.0));
        REQUIRE(knotVector[7] == Approx(1.0));
    }

    SECTION("Set Closed and Wrapping") {
        RS_Vector point1(0.0, 0.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(2.0, 0.0);
        RS_Vector point4(3.0, 1.0);

        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        spline.addControlPoint(point4);

        spline.setClosed(true);
        REQUIRE(spline.isClosed());
        REQUIRE(spline.getUnwrappedSize() == 4);
        REQUIRE(spline.getData().controlPoints.size() == 7); // 4 + 3 wrapping

        auto unwrappedPoints = spline.getUnwrappedControlPoints();
        REQUIRE(unwrappedPoints.size() == 4);
        REQUIRE(unwrappedPoints[0] == point1);

        // Check wrapping: last 3 should match first 3
        auto allPoints = spline.getData().controlPoints;
        REQUIRE(allPoints[4] == allPoints[0]);
        REQUIRE(allPoints[5] == allPoints[1]);
        REQUIRE(allPoints[6] == allPoints[2]);
    }

    SECTION("Remove Wrapping") {
        // Setup closed
        RS_Vector point1(0.0, 0.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(2.0, 0.0);
        RS_Vector point4(3.0, 1.0);

        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        spline.addControlPoint(point4);
        spline.setClosed(true);

        spline.setClosed(false);
        REQUIRE(!spline.isClosed());
        REQUIRE(spline.getNumberOfControlPoints() == 4);
        REQUIRE(spline.getData().controlPoints.size() == 4);
    }
}

TEST_CASE("RS_Spline Non-Uniform Splines", "[RS_Spline]") {
    RS_SplineData splineData(2, false); // Quadratic
    RS_Spline spline(nullptr, splineData);

    RS_Vector point1(0.0, 0.0);
    RS_Vector point2(1.0, 2.0);
    RS_Vector point3(3.0, 1.0);
    RS_Vector point4(4.0, 0.0);

    spline.addControlPoint(point1);
    spline.addControlPoint(point2);
    spline.addControlPoint(point3);
    spline.addControlPoint(point4);

    // Set non-uniform knots
    std::vector<double> nonUniformKnots = {0.0, 0.0, 0.0, 1.0, 3.0, 4.0, 4.0, 4.0};
    spline.setKnotVector(nonUniformKnots);

    // Evaluate at specific parameters
    // For quadratic, evaluateNURBS should interpolate correctly
    // At t=0: point1
    RS_Vector evalAtStart = spline.getPointAt(0.0);
    REQUIRE(evalAtStart.x == Approx(0.0));
    REQUIRE(evalAtStart.y == Approx(0.0));

    // At t=4: point4
    RS_Vector evalAtEnd = spline.getPointAt(4.0);
    REQUIRE(evalAtEnd.x == Approx(4.0));
    REQUIRE(evalAtEnd.y == Approx(0.0));

    // At t=2 (mid between 1 and 3)
    RS_Vector evalAtMid = spline.getPointAt(2.0);
    REQUIRE(evalAtMid.x == Approx(2.0));
    REQUIRE(evalAtMid.y == Approx(1.5).margin(0.01));

    SECTION("Rational Non-Uniform") {
        std::vector<double> weights = {1.0, 2.0, 1.0, 1.0};
        spline.setWeights(weights);

        // Re-evaluate
        RS_Vector rationalAtMid = spline.getPointAt(2.0);
        REQUIRE(rationalAtMid.x == Approx(1.666).margin(0.01));
        REQUIRE(rationalAtMid.y == Approx(1.777).margin(0.01));
    }

    SECTION("Non-Uniform Degree 3") {
        spline.setDegree(3);
        std::vector<double> nonUniformKnotsDegree3 = {0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 5.0, 5.0, 5.0, 5.0};
        spline.setKnotVector(nonUniformKnotsDegree3);

        // Evaluate at t=2.0
        RS_Vector evalAtTwo = spline.getPointAt(2.0);
        REQUIRE(evalAtTwo.x > 1.0);
        REQUIRE(evalAtTwo.x < 3.0);
        REQUIRE(evalAtTwo.y > 0.5);
        REQUIRE(evalAtTwo.y < 1.5);
    }

    SECTION("Non-Uniform Closed") {
        spline.setClosed(true);
        REQUIRE(spline.isClosed());
        // Knots need to be adjusted for closed
        spline.updateKnotWrapping();

        // Check continuity at closure
        auto knotVector = spline.getKnotVector();
        double tMin = knotVector[spline.getDegree()];
        double tMax = knotVector[spline.getUnwrappedSize()];
        RS_Vector evalAtMin = spline.getPointAt(tMin);
        RS_Vector evalAtMax = spline.getPointAt(tMax);
        REQUIRE(evalAtMin.distanceTo(evalAtMax) < RS_TOLERANCE);
    }

    SECTION("Non-Uniform Rational B-Spline (NURBS) with Varying Weights") {
        std::vector<double> weights = {1.0, 0.5, 2.0, 1.5};
        spline.setWeights(weights);

        RS_Vector evalAtMid = spline.getPointAt(2.0);
        REQUIRE(evalAtMid.x == Approx(2.1).margin(0.1));
        REQUIRE(evalAtMid.y == Approx(1.2).margin(0.1));
    }
}

TEST_CASE("RS_Spline Fit Points", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    std::vector<RS_Vector> fitPoints = {
        RS_Vector(0.0, 0.0),
        RS_Vector(1.0, 1.0),
        RS_Vector(2.0, 0.0),
        RS_Vector(3.0, 1.0)
    };

    spline.setFitPoints(fitPoints);
    REQUIRE(spline.getNumberOfControlPoints() == 4);

    // Check if it interpolates fit points
    auto knotVector = spline.getKnotVector();
    RS_Vector evalAtFirst = spline.getPointAt(knotVector[3]);
    REQUIRE(evalAtFirst.distanceTo(fitPoints[0]) < RS_TOLERANCE);

    RS_Vector evalAtLast = spline.getPointAt(knotVector[knotVector.size() - 4]);
    REQUIRE(evalAtLast.distanceTo(fitPoints.back()) < RS_TOLERANCE);

    // Recompute parameters to check all fit points
    size_t numPoints = fitPoints.size();
    double alpha = 0.5; // centripetal
    std::vector<double> params(numPoints, 0.0);
    double totalLength = 0.0;
    for (size_t k = 1; k < numPoints; ++k) {
        double distance = fitPoints[k].distanceTo(fitPoints[k - 1]);
        totalLength += std::pow(distance, alpha);
    }
    double cumulative = 0.0;
    for (size_t k = 1; k < numPoints; ++k) {
        double distance = fitPoints[k].distanceTo(fitPoints[k - 1]);
        cumulative += std::pow(distance, alpha);
        params[k] = cumulative / totalLength;
    }

    for (size_t k = 0; k < numPoints; ++k) {
        double param = params[k];
        RS_Vector evalPoint = spline.getPointAt(param);
        REQUIRE(evalPoint.distanceTo(fitPoints[k]) < RS_TOLERANCE);
    }

    SECTION("Fit Points Closed") {
        RS_SplineData closedData(3, true);
        RS_Spline closedSpline(nullptr, closedData);
        closedSpline.setFitPoints(fitPoints, true); // centripetal
        REQUIRE(closedSpline.isClosed());
        REQUIRE(closedSpline.getUnwrappedSize() == 4);

        // Recompute params for closed
        std::vector<double> closedParams(numPoints + 1, 0.0);
        double closedTotal = totalLength;
        double closingDist = fitPoints.back().distanceTo(fitPoints[0]);
        closedTotal += std::pow(closingDist, alpha);
        cumulative = 0.0;
        for (size_t k = 1; k < numPoints; ++k) {
            double dist = fitPoints[k].distanceTo(fitPoints[k - 1]);
            cumulative += std::pow(dist, alpha);
            closedParams[k] = cumulative / closedTotal;
        }
        cumulative += std::pow(closingDist, alpha);
        closedParams[numPoints] = cumulative / closedTotal;

        // Check interpolation
        auto closedKnots = closedSpline.getKnotVector();
        for (size_t k = 0; k < numPoints; ++k) {
            double param = closedParams[k];
            RS_Vector evalPoint = closedSpline.getPointAt(param);
            REQUIRE(evalPoint.distanceTo(fitPoints[k]) < RS_TOLERANCE);
        }
        // Check closure: eval at closedParams[numPoints] should ≈ eval at 0
        RS_Vector evalAtClose = closedSpline.getPointAt(closedParams[numPoints]);
        RS_Vector evalAtZero = closedSpline.getPointAt(0.0);
        REQUIRE(evalAtClose.distanceTo(evalAtZero) < RS_TOLERANCE);
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
        REQUIRE(openKnots[3] == Approx(0.0));
        REQUIRE(openKnots[4] == Approx(1.0));
        REQUIRE(openKnots[5] == Approx(3.0));
        REQUIRE(openKnots[6] == Approx(5.0));
        REQUIRE(openKnots[7] == Approx(5.0));
    }

    SECTION("Convert Open to Closed - Clamped Uniform") {
        std::vector<double> openKnots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
        auto closedKnots = LC_SplineHelper::convertOpenToClosedKnotVector(openKnots, unwrappedControlCount, splineDegree);
        REQUIRE(closedKnots.size() == 11);
        REQUIRE(closedKnots[0] == Approx(1.0));
        REQUIRE(closedKnots[1] == Approx(1.0));
        REQUIRE(closedKnots[2] == Approx(1.0));
        REQUIRE(closedKnots[3] == Approx(1.0));
        REQUIRE(closedKnots[4] == Approx(1.0));
        REQUIRE(closedKnots[5] == Approx(1.0));
        REQUIRE(closedKnots[6] == Approx(1.0));
        REQUIRE(closedKnots[7] == Approx(1.0));
        REQUIRE(closedKnots[8] == Approx(1.0));
        REQUIRE(closedKnots[9] == Approx(1.0));
        REQUIRE(closedKnots[10] == Approx(1.0)); // Since deltas=0
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
}

TEST_CASE("LC_SplineHelper Wrapping and Type Conversions", "[LC_SplineHelper]") {
    RS_SplineData splineData;
    splineData.degree = 3;
    splineData.type = RS_SplineData::SplineType::Standard;
    splineData.controlPoints = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,0), RS_Vector(3,1)};
    splineData.weights = {1.0, 2.0, 3.0, 4.0};
    splineData.knotslist = {0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0};

    SECTION("Add Wrapping - Non-Uniform Weights") {
        LC_SplineHelper::addWrapping(splineData);
        REQUIRE(splineData.controlPoints.size() == 7);
        REQUIRE(splineData.controlPoints[4] == RS_Vector(0,0));
        REQUIRE(splineData.controlPoints[5] == RS_Vector(1,1));
        REQUIRE(splineData.controlPoints[6] == RS_Vector(2,0));
        REQUIRE(splineData.weights[4] == Approx(1.0));
        REQUIRE(splineData.weights[5] == Approx(2.0));
        REQUIRE(splineData.weights[6] == Approx(3.0));
        REQUIRE(splineData.knotslist.size() == 11);
    }

    SECTION("Remove Wrapping") {
        LC_SplineHelper::addWrapping(splineData);
        LC_SplineHelper::removeWrapping(splineData);
        REQUIRE(splineData.controlPoints.size() == 4);
        REQUIRE(splineData.weights.size() == 4);
        REQUIRE(splineData.knotslist.size() == 8);
    }

    SECTION("Update Control and Weight Wrapping") {
        size_t unwrappedCount = 4;
        splineData.controlPoints.resize(7);
        splineData.weights.resize(7);
        LC_SplineHelper::updateControlAndWeightWrapping(splineData, true, unwrappedCount);
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
        // Check if converted to closed
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
        LC_SplineHelper::insertKnot(knots, 5); // Beyond, treat as end
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
}

TEST_CASE("RS_Spline Cubic Specific Tests", "[RS_Spline][degree3]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    SECTION("Open Cubic Equivalent to Bezier") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(0.0, 1.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(1.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);

        auto knotVector = spline.getKnotVector();
        REQUIRE(knotVector == std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});

        // Evaluate at t=0.0
        RS_Vector evalAtZero = spline.getPointAt(0.0);
        REQUIRE(evalAtZero.x == Approx(0.0));
        REQUIRE(evalAtZero.y == Approx(0.0));

        // At t=1.0
        RS_Vector evalAtOne = spline.getPointAt(1.0);
        REQUIRE(evalAtOne.x == Approx(1.0));
        REQUIRE(evalAtOne.y == Approx(0.0));

        // At t=0.5
        RS_Vector evalAtHalf = spline.getPointAt(0.5);
        REQUIRE(evalAtHalf.x == Approx(0.5));
        REQUIRE(evalAtHalf.y == Approx(0.75));
    }

    SECTION("Open Cubic Rational") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(0.0, 1.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(1.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        std::vector<double> weights = {1.0, 1.0, 2.0, 1.0};
        spline.setWeights(weights);

        // At t=0.5, expected (0.636, 0.818)
        RS_Vector evalAtHalf = spline.getPointAt(0.5);
        REQUIRE(evalAtHalf.x == Approx(0.636).margin(0.001));
        REQUIRE(evalAtHalf.y == Approx(0.818).margin(0.001));
    }

    SECTION("Closed Cubic Evaluation Continuity") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(0.0, 1.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(1.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        spline.setClosed(true);

        auto knotVector = spline.getKnotVector();
        REQUIRE(knotVector.size() == 11);
        double tMin = knotVector[3];
        double tMax = knotVector[7];

        RS_Vector evalAtMin = spline.getPointAt(tMin);
        RS_Vector evalAtMax = spline.getPointAt(tMax);
        REQUIRE(evalAtMin.distanceTo(evalAtMax) < RS_TOLERANCE);
    }

    SECTION("Change Type for Cubic") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(1.0, 1.0);
        RS_Vector point2(2.0, 0.0);
        RS_Vector point3(3.0, 1.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);

        auto originalUnwrappedPoints = spline.getUnwrappedControlPoints();
        spline.changeType(RS_SplineData::SplineType::WrappedClosed);
        REQUIRE(spline.isClosed());

        auto closedUnwrappedPoints = spline.getUnwrappedControlPoints();
        REQUIRE(closedUnwrappedPoints == originalUnwrappedPoints);

        spline.changeType(RS_SplineData::SplineType::ClampedOpen);
        REQUIRE(!spline.isClosed());

        auto openUnwrappedPoints = spline.getUnwrappedControlPoints();
        REQUIRE(openUnwrappedPoints == originalUnwrappedPoints);
    }
}

TEST_CASE("RS_Spline Extremum Positions and Tight Bounding Box", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    SECTION("Bezier Cubic Extremum") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(0.0, 1.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(1.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);

        // X extrema
        auto xZeros = spline.findDerivativeZeros(true);
        REQUIRE(xZeros.empty());

        // Y extrema
        auto yZeros = spline.findDerivativeZeros(false);
        REQUIRE(yZeros.size() == 1);
        REQUIRE(yZeros[0] == Approx(0.5));

        RS_Vector extremaPoint = spline.getPointAt(yZeros[0]);
        REQUIRE(extremaPoint.x == Approx(0.5));
        REQUIRE(extremaPoint.y == Approx(0.75));

        // Tight bounds
        spline.calculateTightBorders();
        REQUIRE(spline.getMin() == RS_Vector(0.0, 0.0));
        REQUIRE(spline.getMax() == RS_Vector(1.0, 0.75));
    }

    SECTION("Rational Bezier Cubic Extremum") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(0.0, 1.0);
        RS_Vector point2(1.0, 1.0);
        RS_Vector point3(1.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        std::vector<double> weights = {1.0, 1.0, 2.0, 1.0};
        spline.setWeights(weights);

        // X extrema
        auto xZeros = spline.findDerivativeZeros(true);
        REQUIRE(xZeros.empty());

        // Y extrema
        auto yZeros = spline.findDerivativeZeros(false);
        REQUIRE(yZeros.size() == 1);
        REQUIRE(yZeros[0] == Approx(0.52));

        RS_Vector extremaPoint = spline.getPointAt(yZeros[0]);
        REQUIRE(extremaPoint.x == Approx(0.662).margin(0.001));
        REQUIRE(extremaPoint.y == Approx(0.819).margin(0.001));

        // Check derivative is zero
        double eps = 0.001;
        RS_Vector p1 = spline.getPointAt(yZeros[0] - eps);
        RS_Vector p2 = spline.getPointAt(yZeros[0] + eps);
        double dyDt = (p2.y - p1.y) / (2 * eps);
        REQUIRE(dyDt == Approx(0.0).margin(0.01));

        // Tight bounds
        spline.calculateTightBorders();
        REQUIRE(compareVector(spline.getMin(), RS_Vector(0.0, 0.0)));
        REQUIRE(compareVector(spline.getMax(), RS_Vector(1.0, 0.819)));

        // Validate with sampling
        auto knotVector = spline.getKnotVector();
        double tMin = knotVector[splineData.degree];
        double tMax = knotVector[spline.getNumberOfControlPoints() - splineData.degree - 1];
        int numSamples = 100;
        double step = (tMax - tMin) / numSamples;
        double sampleMaxY = -std::numeric_limits<double>::infinity();
        for (int i = 0; i <= numSamples; ++i) {
            double t = tMin + i * step;
            RS_Vector point = spline.getPointAt(t);
            sampleMaxY = std::max(sampleMaxY, point.y);
        }
        REQUIRE(spline.getMax().y == Approx(sampleMaxY).margin(0.01));
    }

    SECTION("Rational Non-Uniform Cubic Extremum") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(1.0, 2.0);
        RS_Vector point2(3.0, 1.0);
        RS_Vector point3(4.0, 0.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        std::vector<double> weights = {1.0, 0.5, 2.0, 1.5};
        spline.setWeights(weights);
        std::vector<double> nonUniformKnots = {0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 5.0, 5.0, 5.0, 5.0};
        spline.setKnotVector(nonUniformKnots);

        // X extrema
        auto xZeros = spline.findDerivativeZeros(true);
        REQUIRE(xZeros.empty());

        // Y extrema
        auto yZeros = spline.findDerivativeZeros(false);
        REQUIRE(yZeros.size() == 1);
        REQUIRE(yZeros[0] == Approx(0.61).margin(0.01));

        RS_Vector extremaPoint = spline.getPointAt(yZeros[0]);
        REQUIRE(extremaPoint.x == Approx(2.112).margin(0.001));
        REQUIRE(extremaPoint.y == Approx(1.272).margin(0.001));

        // Check derivative is zero
        double eps = 0.001;
        RS_Vector p1 = spline.getPointAt(yZeros[0] - eps);
        RS_Vector p2 = spline.getPointAt(yZeros[0] + eps);
        double dyDt = (p2.y - p1.y) / (2 * eps);
        REQUIRE(dyDt == Approx(0.0).margin(0.01));

        // Tight bounds
        spline.calculateTightBorders();

        // Validate with sampling
        auto knotVector = spline.getKnotVector();
        double tMin = knotVector[splineData.degree];
        double tMax = knotVector[spline.getNumberOfControlPoints() - splineData.degree - 1];
        int numSamples = 100;
        double step = (tMax - tMin) / numSamples;
        double sampleMaxY = -std::numeric_limits<double>::infinity();
        for (int i = 0; i <= numSamples; ++i) {
            double t = tMin + i * step;
            RS_Vector point = spline.getPointAt(t);
            sampleMaxY = std::max(sampleMaxY, point.y);
        }
        REQUIRE(spline.getMax().y == Approx(sampleMaxY).margin(0.01));
    }

    SECTION("Rational Cubic with Multiple Extrema") {
        RS_Vector point0(0.0, 0.0);
        RS_Vector point1(1.0, 3.0);
        RS_Vector point2(2.0, -1.0);
        RS_Vector point3(3.0, 2.0);

        spline.addControlPoint(point0);
        spline.addControlPoint(point1);
        spline.addControlPoint(point2);
        spline.addControlPoint(point3);
        std::vector<double> weights = {1.0, 1.0, 1.0, 1.0}; // Start with non-rational
        spline.setWeights(weights);

        // Y extrema
        auto yZeros = spline.findDerivativeZeros(false);
        REQUIRE(yZeros.size() == 2);
        REQUIRE(yZeros[0] == Approx(0.311).margin(0.001));
        REQUIRE(yZeros[1] == Approx(0.689).margin(0.001));

        RS_Vector extremaPoint1 = spline.getPointAt(yZeros[0]);
        REQUIRE(extremaPoint1.y == Approx(1.189).margin(0.001));

        RS_Vector extremaPoint2 = spline.getPointAt(yZeros[1]);
        REQUIRE(extremaPoint2.y == Approx(0.811).margin(0.001));

        // Now make rational
        weights = {1.0, 2.0, 0.5, 1.5};
        spline.setWeights(weights);

        yZeros = spline.findDerivativeZeros(false);
        REQUIRE(yZeros.size() == 2);
        REQUIRE(yZeros[0] == Approx(0.4).margin(0.001));
        REQUIRE(yZeros[1] == Approx(0.788).margin(0.001));

        extremaPoint1 = spline.getPointAt(yZeros[0]);
        REQUIRE(extremaPoint1.y == Approx(2.0).margin(0.001));

        extremaPoint2 = spline.getPointAt(yZeros[1]);
        REQUIRE(extremaPoint2.y == Approx(1.654).margin(0.001));

        // Tight bounds
        spline.calculateTightBorders();
        REQUIRE(spline.getMin().y == Approx(0.0).margin(0.001));
        REQUIRE(spline.getMax().y == Approx(2.0).margin(0.001));

        // Validate with sampling
        auto knotVector = spline.getKnotVector();
        double tMin = knotVector[splineData.degree];
        double tMax = knotVector[spline.getNumberOfControlPoints() - splineData.degree - 1];
        int numSamples = 100;
        double step = (tMax - tMin) / numSamples;
        double sampleMaxY = -std::numeric_limits<double>::infinity();
        double sampleMinY = std::numeric_limits<double>::infinity();
        for (int i = 0; i <= numSamples; ++i) {
            double t = tMin + i * step;
            RS_Vector point = spline.getPointAt(t);
            sampleMaxY = std::max(sampleMaxY, point.y);
            sampleMinY = std::min(sampleMinY, point.y);
        }
        REQUIRE(spline.getMax().y == Approx(sampleMaxY).margin(0.01));
        REQUIRE(spline.getMin().y == Approx(sampleMinY).margin(0.01));
    }
}
