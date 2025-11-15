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
    REQUIRE(evalAtMid.y == Approx(1.5).epsilon(1e-6));

    SECTION("Rational Non-Uniform") {
        std::vector<double> weights = {1.0, 2.0, 1.0, 1.0};
        spline.setWeights(weights);

        // Re-evaluate
        RS_Vector rationalAtMid = spline.getPointAt(2.0);
        REQUIRE(rationalAtMid.x == Approx(1.6666666666666667).epsilon(1e-6));
        REQUIRE(rationalAtMid.y == Approx(1.7777777777777777).epsilon(1e-6));
    }

    SECTION("Non-Uniform Degree 3") {
        spline.setDegree(3);
        std::vector<double> nonUniformKnotsDegree3 = {0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 5.0, 5.0, 5.0, 5.0};
        spline.setKnotVector(nonUniformKnotsDegree3);

        // Evaluate at t=2.0
        RS_Vector evalAtTwo = spline.getPointAt(2.0);
        REQUIRE(evalAtTwo.x == Approx(1.8518518518518519).epsilon(1e-6));  // Computed precisely
        REQUIRE(evalAtTwo.y == Approx(1.4074074074074074).epsilon(1e-6));
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
        REQUIRE(evalAtMid.x == Approx(2.105263157894737).epsilon(1e-6));
        REQUIRE(evalAtMid.y == Approx(1.2105263157894737).epsilon(1e-6));
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

    // Check interpolation
    REQUIRE(compareVector(spline.getPointAt(spline.getKnotVector()[3]), fitPoints[0]));
    REQUIRE(compareVector(spline.getPointAt(spline.getKnotVector()[spline.getNumberOfControlPoints()]), fitPoints.back()));

    // Add more checks for internal points if needed
}

TEST_CASE("RS_Spline ChangeType Functionality", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    RS_Vector point1(0.0, 0.0);
    RS_Vector point2(1.0, 1.0);
    RS_Vector point3(2.0, 0.0);
    RS_Vector point4(3.0, 1.0);

    spline.addControlPoint(point1);
    spline.addControlPoint(point2);
    spline.addControlPoint(point3);
    spline.addControlPoint(point4);

    SECTION("Change to Standard") {
        spline.changeType(RS_SplineData::SplineType::Standard);
        REQUIRE(spline.getData().type == RS_SplineData::SplineType::Standard);
        REQUIRE(spline.validate());
        // Check unclamped knots
        auto knots = spline.getKnotVector();
        REQUIRE(knots[0] < 0.0); // Unclamped
    }

    SECTION("Change to WrappedClosed") {
        spline.changeType(RS_SplineData::SplineType::WrappedClosed);
        REQUIRE(spline.isClosed());
        REQUIRE(spline.getData().type == RS_SplineData::SplineType::WrappedClosed);
        REQUIRE(spline.validate());
        REQUIRE(spline.hasWrappedControlPoints());
    }

    SECTION("Change to ClampedOpen") {
        spline.changeType(RS_SplineData::SplineType::WrappedClosed);
        spline.changeType(RS_SplineData::SplineType::ClampedOpen);
        REQUIRE(!spline.isClosed());
        REQUIRE(spline.getData().type == RS_SplineData::SplineType::ClampedOpen);
        REQUIRE(spline.validate());
    }
}

TEST_CASE("RS_Spline Evaluation and Derivatives", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    RS_Vector point0(0.0, 0.0);
    RS_Vector point1(1.0, 3.0);
    RS_Vector point2(2.0, -1.0);
    RS_Vector point3(3.0, 2.0);

    spline.addControlPoint(point0);
    spline.addControlPoint(point1);
    spline.addControlPoint(point2);
    spline.addControlPoint(point3);

    SECTION("Get Point At") {
        RS_Vector p = spline.getPointAt(0.5);
        REQUIRE(p.x == Approx(1.5).margin(0.01));
        REQUIRE(p.y == Approx(1.0).margin(0.01)); // Approximate for cubic
    }

    SECTION("Find Derivative Zeros") {
        auto yzeros = spline.findDerivativeZeros(false);
        REQUIRE(yzeros.size() == 2);
        REQUIRE(yzeros[0] == Approx(0.311).margin(0.001));
        REQUIRE(yzeros[1] == Approx(0.689).margin(0.001));
    }

    SECTION("Calculate Tight Borders") {
        spline.calculateTightBorders();
        REQUIRE(spline.getMin().y == Approx(0.0).margin(0.001));
        REQUIRE(spline.getMax().y == Approx(1.189).margin(0.001)); // From extrema
    }
}

TEST_CASE("RS_Spline Operations", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    RS_Vector point1(0.0, 0.0);
    RS_Vector point2(1.0, 1.0);
    RS_Vector point3(2.0, 0.0);
    RS_Vector point4(3.0, 1.0);

    spline.addControlPoint(point1);
    spline.addControlPoint(point2);
    spline.addControlPoint(point3);
    spline.addControlPoint(point4);

    SECTION("Move") {
        RS_Vector offset(1.0, 1.0);
        spline.move(offset);
        auto controls = spline.getControlPoints();
        REQUIRE(compareVector(controls[0], RS_Vector(1.0, 1.0)));
        REQUIRE(compareVector(controls[1], RS_Vector(2.0, 2.0)));
    }

    SECTION("Rotate") {
        RS_Vector center(0.0, 0.0);
        double angle = M_PI / 2; // 90 degrees
        spline.rotate(center, angle);
        auto controls = spline.getControlPoints();
        REQUIRE(compareVector(controls[0], RS_Vector(0.0, 0.0)));
        REQUIRE(compareVector(controls[1], RS_Vector(-1.0, 1.0)));
    }

    SECTION("Scale") {
        RS_Vector center(0.0, 0.0);
        RS_Vector factor(2.0, 2.0);
        spline.scale(center, factor);
        auto controls = spline.getControlPoints();
        REQUIRE(compareVector(controls[0], RS_Vector(0.0, 0.0)));
        REQUIRE(compareVector(controls[1], RS_Vector(2.0, 2.0)));
    }

    SECTION("Mirror") {
        RS_Vector axis1(0.0, 0.0);
        RS_Vector axis2(1.0, 0.0); // X-axis
        spline.mirror(axis1, axis2);
        auto controls = spline.getControlPoints();
        REQUIRE(compareVector(controls[0], RS_Vector(0.0, 0.0)));
        REQUIRE(compareVector(controls[1], RS_Vector(1.0, -1.0)));
    }

    SECTION("Revert Direction") {
        spline.revertDirection();
        auto controls = spline.getControlPoints();
        REQUIRE(compareVector(controls[0], point4));
        REQUIRE(compareVector(controls[3], point1));
    }
}

TEST_CASE("RS_Spline Knot and Control Manipulation", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    RS_Vector point1(0.0, 0.0);
    RS_Vector point2(1.0, 1.0);
    RS_Vector point3(2.0, 0.0);
    RS_Vector point4(3.0, 1.0);

    spline.addControlPoint(point1);
    spline.addControlPoint(point2);
    spline.addControlPoint(point3);
    spline.addControlPoint(point4);

    SECTION("Insert Control Point") {
        RS_Vector newPoint(1.5, 1.5);
        spline.insertControlPoint(2, newPoint);
        auto controls = spline.getControlPoints();
        REQUIRE(controls.size() == 5);
        REQUIRE(compareVector(controls[2], newPoint));
    }

    SECTION("Remove Control Point") {
        spline.removeControlPoint(1);
        auto controls = spline.getControlPoints();
        REQUIRE(controls.size() == 3);
        REQUIRE(compareVector(controls[1], point3));
    }

    SECTION("Set Knot Vector") {
        std::vector<double> newKnots(8, 0.0);
        spline.setKnotVector(newKnots);
        REQUIRE(spline.getKnotVector().size() == 8);
    }
}

TEST_CASE("RS_Spline Validation", "[RS_Spline]") {
    RS_SplineData splineData(3, false);
    RS_Spline spline(nullptr, splineData);

    SECTION("Valid Open Spline") {
        spline.addControlPoint(RS_Vector(0.0, 0.0));
        spline.addControlPoint(RS_Vector(1.0, 1.0));
        spline.addControlPoint(RS_Vector(2.0, 0.0));
        spline.addControlPoint(RS_Vector(3.0, 1.0));
        REQUIRE(spline.validate());
    }

    SECTION("Invalid Degree") {
        spline.setDegree(4);
        REQUIRE(!spline.validate());
    }

    SECTION("Invalid Knot Multiplicity") {
        std::vector<double> invalidKnots(8, 0.0); // All same, multiplicity 8 > degree+1=4
        spline.setKnotVector(invalidKnots);
        REQUIRE(!spline.validate());
    }

    SECTION("Invalid Weights") {
        std::vector<double> invalidWeights = {1.0, -1.0, 1.0, 1.0};
        spline.setWeights(invalidWeights);
        REQUIRE(!spline.validate());
    }
}