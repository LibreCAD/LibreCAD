/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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
// File: rs_spline_tests.cpp

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "lc_splinehelper.h"
#include "lc_splinepoints.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_spline.h"
#include "rs_vector.h"

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

namespace {
    std::unique_ptr<RS_Spline> makeCubic(const bool closed) {
        auto spline = std::make_unique<RS_Spline>(nullptr, RS_SplineData(3, false));
        for (const RS_Vector& p : {RS_Vector(0., 0.), RS_Vector(10., 20.), RS_Vector(20., -20.), RS_Vector(30., 0.), RS_Vector(40., 15.)}) {
            spline->addControlPoint(p);
        }
        if (closed) {
            spline->setClosed(true);
            spline->update();
        }
        return spline;
    }

    // the lines the spline is drawn and hit-tested by match a fresh rebuild from its data
    void requireRebuilt(RS_Spline& spline) {
        const std::unique_ptr<RS_Spline> rebuilt{static_cast<RS_Spline*>(spline.clone())};
        rebuilt->update();
        REQUIRE(spline.count() > 0);
        REQUIRE(spline.count() == rebuilt->count());
        const unsigned last = spline.count() - 1;
        CHECK(compareVector(spline.entityAt(0)->getStartpoint(), rebuilt->entityAt(0)->getStartpoint(), 1e-9));
        CHECK(compareVector(spline.entityAt(last)->getEndpoint(), rebuilt->entityAt(last)->getEndpoint(), 1e-9));
        CHECK(compareVector(spline.getMin(), rebuilt->getMin(), 1e-9));
        CHECK(compareVector(spline.getMax(), rebuilt->getMax(), 1e-9));
    }
}

TEST_CASE("RS_Spline transforms rebuild the lines it is drawn with", "[RS_Spline]")
{
    const std::unique_ptr<RS_Spline> spline = makeCubic(false);

    spline->move(RS_Vector(1000., 0.));
    requireRebuilt(*spline);
    // a clamped spline starts at its first control point
    CHECK(compareVector(spline->entityAt(0)->getStartpoint(), RS_Vector(1000., 0.), 1e-9));

    spline->rotate(RS_Vector(5., 5.), 0.7);
    requireRebuilt(*spline);
    spline->scale(RS_Vector(3., 4.), RS_Vector(2., 0.5));
    requireRebuilt(*spline);
    spline->mirror(RS_Vector(0., 0.), RS_Vector(1., 1.));
    requireRebuilt(*spline);
    spline->shear(0.3);
    requireRebuilt(*spline);
}

TEST_CASE("RS_Spline::setDegree keeps the spline drawable", "[RS_Spline]")
{
    for (const bool closed : {false, true}) {
        for (const int degree : {2, 1, 3}) {
            const std::unique_ptr<RS_Spline> spline = makeCubic(closed);
            REQUIRE(spline->count() > 0);
            spline->setDegree(degree);
            INFO("closed " << closed << ", degree " << degree);
            CHECK(spline->getDegree() == static_cast<size_t>(degree));
            CHECK(spline->isClosed() == closed);
            CHECK(spline->getNumberOfControlPoints() == 5);
            requireRebuilt(*spline);
        }
    }
}

TEST_CASE("LC_SplinePoints offset keeps a usable spline", "[LC_SplinePoints]")
{
    LC_SplinePointsData data(false, false);
    data.splinePoints = {RS_Vector(0., 0.), RS_Vector(10., 10.), RS_Vector(20., 0.), RS_Vector(30., 10.)};
    LC_SplinePoints spline(nullptr, data);
    spline.update();

    spline.offset(RS_Vector(15., 20.), 5.);

    CHECK(spline.getData().controlPoints.size() == 4);
    CHECK(spline.getMin().valid);
    CHECK(spline.getMax().valid);
    CHECK(spline.getLength() > 0.);
    CHECK(spline.getStartpoint().distanceTo(RS_Vector(0., 0.)) == Approx(5.).margin(1e-6));
}

TEST_CASE("LC_SplinePoints length with fewer than four control points", "[LC_SplinePoints]")
{
    LC_SplinePointsData data(false, false);
    data.useControlPoints = true;
    data.controlPoints = {RS_Vector(0., 0.), RS_Vector(10., 20.), RS_Vector(20., 0.)};
    LC_SplinePoints spline(nullptr, data);
    spline.update();

    // a quadratic Bezier is longer than its chord and shorter than its control polygon
    CHECK(spline.getLength() > 20.);
    CHECK(spline.getLength() < 2. * std::hypot(10., 20.));
}


TEST_CASE("RS_Spline grips move its control points", "[RS_Spline]")
{
    // The reference points are the control points. The nearest one used to come from the ends of the
    // drawn lines, and moving a reference point moved only those ends, until the next rebuild.
    for (const bool closed : {false, true}) {
        INFO((closed ? "closed" : "open"));
        const std::unique_ptr<RS_Spline> spline = makeCubic(closed);
        const std::vector<RS_Vector> before = spline->getControlPoints();
        REQUIRE(before.size() == 5);
        // an inner control point lies off the drawn curve
        const RS_Vector inner = before[2];

        CHECK(compareVector(spline->getNearestRef(inner + RS_Vector(0.5, -0.5)), inner, 1e-9));
        // setSelected() is for the document; the selection flag is what getNearestSelectedRef() reads
        spline->setFlag(RS2::FlagSelected);
        CHECK(compareVector(spline->getNearestSelectedRef(inner + RS_Vector(0.5, -0.5)), inner, 1e-9));

        const RS_Vector offset(3., -4.);
        spline->moveRef(inner, offset);

        const std::vector<RS_Vector> after = spline->getControlPoints();
        REQUIRE(after.size() == before.size());
        for (size_t i = 0; i < after.size(); ++i) {
            CHECK(compareVector(after[i], i == 2 ? inner + offset : before[i], 1e-9));
        }
        requireRebuilt(*spline);
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

// Regression: setFitPoints with num == p+1 (single Bezier patch) used to
// short-circuit before solving the interior system, leaving interior control
// points as RS_Vector(false) (which read back as (0,0)).
//
// Reproduces the bug seen in ~/doc/dwg/Pool_Detail.dwg where a degree-3
// scenario-2 spline with 4 fit points rendered with control-point list
// [(223.754,152.933), (0,0), (0,0), (221.427,150.761)].
TEST_CASE("RS_Spline::setFitPoints fills interior CPs for num == p+1",
          "[RS_Spline][regression][fitpoints]") {
  SECTION("Pool_Detail.dwg cubic, 4 fit points") {
    RS_SplineData d(3, false);
    RS_Spline spline(nullptr, d);

    std::vector<RS_Vector> fps = {{223.754, 152.933},
                                  {221.937, 153.336},
                                  {221.244, 152.653},
                                  {221.427, 150.761}};
    spline.setFitPoints(fps);

    REQUIRE(spline.getNumberOfControlPoints() == 4);
    const auto &cps = spline.getData().controlPoints;

    // Endpoints: must equal the first/last fit points.
    REQUIRE(compareVector(cps.front(), fps.front()));
    REQUIRE(compareVector(cps.back(), fps.back()));

    // Interior: must be valid (was the bug — left as RS_Vector(false))
    // and must lie within a reasonable bounding box of the fit points.
    for (size_t i = 1; i + 1 < cps.size(); ++i) {
      REQUIRE(cps[i].valid);
      REQUIRE(std::abs(cps[i].x) > 1.0); // definitely not (0, 0)
      REQUIRE(std::abs(cps[i].y) > 1.0);
    }

    // Spline must actually pass through every fit point.
    for (const auto &fp : fps) {
      const RS_Vector nearest = spline.getNearestPointOnEntity(fp, true);
      REQUIRE(compareVector(nearest, fp, 1e-3));
    }
  }

  SECTION("Quadratic, 3 fit points (num == p+1)") {
    RS_SplineData d(2, false);
    RS_Spline spline(nullptr, d);

    std::vector<RS_Vector> fps = {{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}};
    spline.setFitPoints(fps);

    const auto &cps = spline.getData().controlPoints;
    REQUIRE(cps.size() == 3);
    REQUIRE(cps[1].valid);
    // Was the bug: interior CP left as RS_Vector(false) → (0,0).
    REQUIRE(!compareVector(cps[1], RS_Vector(0, 0)));
  }

  // Degenerate inputs: degree higher than num-1.  Without the
  // degree-clamping in setFitPoints, the basis-function loop indexes
  // A[row][idx-1] past sys=num-2 and corrupts memory / segfaults.
  SECTION("Cubic, 3 fit points (num == p) — degree clamped to 2") {
    RS_SplineData d(3, false);
    RS_Spline spline(nullptr, d);

    std::vector<RS_Vector> fps = {{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}};
    spline.setFitPoints(fps);

    // Degree must have been lowered so the spline is well-formed.
    REQUIRE(spline.getDegree() == 2);
    const auto &cps = spline.getData().controlPoints;
    REQUIRE(cps.size() == 3);
    REQUIRE(compareVector(cps.front(), fps.front()));
    REQUIRE(compareVector(cps.back(), fps.back()));
    REQUIRE(cps[1].valid);
    REQUIRE(!compareVector(cps[1], RS_Vector(0, 0)));
  }

  SECTION("Cubic, 2 fit points (num < p) — degree clamped to 1") {
    RS_SplineData d(3, false);
    RS_Spline spline(nullptr, d);

    std::vector<RS_Vector> fps = {{0.0, 0.0}, {10.0, 5.0}};
    spline.setFitPoints(fps);

    REQUIRE(spline.getDegree() == 1);
    const auto &cps = spline.getData().controlPoints;
    REQUIRE(cps.size() == 2);
    REQUIRE(compareVector(cps.front(), fps.front()));
    REQUIRE(compareVector(cps.back(), fps.back()));
  }
}

// ---------------------------------------------------------------------------
// Checked jet evaluation (tryEvaluateJet). Analytic fixtures are the oracle;
// central differences are only a secondary check away from knots.
// ---------------------------------------------------------------------------
namespace {
RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls,
                     const std::vector<double>& knots, std::vector<double> weights = {}) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights = weights.empty() ? std::vector<double>(controls.size(), 1.0) : std::move(weights);
    return RS_Spline(nullptr, data);
}

LC_CurveJet jetAt(const RS_Spline& spline, const double t,
                  const LC_CurveEvaluationSide side = LC_CurveEvaluationSide::Interior) {
    LC_CurveJet jet;
    REQUIRE(spline.tryEvaluateJet(t, side, jet));
    return jet;
}

// The cubic Bezier the review of the old evaluator used: it returned NaN
// derivatives for it at every parameter.
const std::vector<RS_Vector> g_bezier{{0.0, 0.0}, {1.0, 3.0}, {2.0, -3.0}, {3.0, 0.0}};

RS_Vector bezierPoint(const double t) {
    const double s = 1.0 - t;
    return g_bezier[0] * (s * s * s) + g_bezier[1] * (3.0 * s * s * t) +
           g_bezier[2] * (3.0 * s * t * t) + g_bezier[3] * (t * t * t);
}
RS_Vector bezierFirst(const double t) {
    const double s = 1.0 - t;
    return ((g_bezier[1] - g_bezier[0]) * (s * s) + (g_bezier[2] - g_bezier[1]) * (2.0 * s * t) +
            (g_bezier[3] - g_bezier[2]) * (t * t)) * 3.0;
}
RS_Vector bezierSecond(const double t) {
    return ((g_bezier[2] - g_bezier[1] * 2.0 + g_bezier[0]) * (1.0 - t) +
            (g_bezier[3] - g_bezier[2] * 2.0 + g_bezier[1]) * t) * 6.0;
}
} // namespace

TEST_CASE("RS_Spline::tryEvaluateJet matches a cubic Bezier exactly", "[spline][jet]") {
    const RS_Spline spline = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    for (const double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const LC_CurveJet jet = jetAt(spline, t);
        CHECK(compareVector(jet.point, bezierPoint(t), 1e-12));
        CHECK(compareVector(jet.first, bezierFirst(t), 1e-12));
        CHECK(compareVector(jet.second, bezierSecond(t), 1e-12));
    }
    // the values the old evaluator reported as NaN
    const LC_CurveJet quarter = jetAt(spline, 0.25);
    CHECK(compareVector(quarter.first, RS_Vector{3.0, -1.125}, 1e-12));
    CHECK(compareVector(quarter.second, RS_Vector{0.0, -27.0}, 1e-12));
}

TEST_CASE("RS_Spline::tryEvaluateJet on a degree-1 line", "[spline][jet]") {
    const RS_Spline line = makeSpline(1, {{1.0, 2.0}, {4.0, 6.0}}, {0, 0, 2, 2});
    const LC_CurveJet jet = jetAt(line, 0.5);
    CHECK(compareVector(jet.point, RS_Vector{1.75, 3.0}, 1e-12));
    CHECK(compareVector(jet.first, RS_Vector{1.5, 2.0}, 1e-12)); // (P1 - P0) / knot span 2
    CHECK(compareVector(jet.second, RS_Vector{0.0, 0.0}, 1e-12));
}

TEST_CASE("RS_Spline::tryEvaluateJet on a rational quarter circle", "[spline][jet]") {
    const double w = std::sqrt(0.5);
    const RS_Spline arc = makeSpline(2, {{1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}},
                                     {0, 0, 0, 1, 1, 1}, {1.0, w, 1.0});
    // End derivatives of a rational quadratic: C'(0) = 2 w1/w0 (P1 - P0).
    CHECK(compareVector(jetAt(arc, 0.0).first, RS_Vector{0.0, 2.0 * w}, 1e-12));
    CHECK(compareVector(jetAt(arc, 1.0).first, RS_Vector{-2.0 * w, 0.0}, 1e-12));
    for (const double t : {0.0, 0.1, 0.37, 0.5, 0.81, 1.0}) {
        const LC_CurveJet jet = jetAt(arc, t);
        CHECK(jet.point.magnitude() == Approx(1.0).margin(1e-12));
        // tangent perpendicular to the radius, curvature exactly 1 (counter-clockwise)
        CHECK(RS_Vector::dotP(jet.point, jet.first) == Approx(0.0).margin(1e-12));
        const double speed = jet.first.magnitude();
        const double cross = jet.first.x * jet.second.y - jet.first.y * jet.second.x;
        CHECK(cross / (speed * speed * speed) == Approx(1.0).margin(1e-10));
    }
}

TEST_CASE("RS_Spline::tryEvaluateJet takes one-sided limits at a repeated knot", "[spline][jet]") {
    // Two cubic Bezier pieces joined with a kink at t = 1 (knot multiplicity 3).
    const std::vector<RS_Vector> controls{{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 2}, {5, 2}, {6, 0}};
    const RS_Spline spline = makeSpline(3, controls, {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveJet left = jetAt(spline, 1.0, LC_CurveEvaluationSide::Left);
    const LC_CurveJet right = jetAt(spline, 1.0, LC_CurveEvaluationSide::Right);
    CHECK(compareVector(left.point, controls[3], 1e-12));
    CHECK(compareVector(right.point, controls[3], 1e-12));
    CHECK(compareVector(left.first, (controls[3] - controls[2]) * 3.0, 1e-12));
    CHECK(compareVector(right.first, (controls[4] - controls[3]) * 3.0, 1e-12));
    CHECK(compareVector(left.second, (controls[3] - controls[2] * 2.0 + controls[1]) * 6.0, 1e-12));
    CHECK(compareVector(right.second, (controls[5] - controls[4] * 2.0 + controls[3]) * 6.0, 1e-12));
    // Interior at an interior knot is the right limit.
    CHECK(compareVector(jetAt(spline, 1.0).first, right.first, 1e-12));
    CHECK(spline.getBreakParameters() == std::vector<double>{0.0, 1.0, 2.0});
}

TEST_CASE("RS_Spline::tryEvaluateJet agrees with central differences on non-uniform knots",
          "[spline][jet]") {
    const RS_Spline spline = makeSpline(
        3, {{0, 0}, {1, 2}, {3, 3}, {4, 1}, {6, 2}, {7, 0}}, {0, 0, 0, 0, 0.3, 1.2, 2, 2, 2, 2},
        {1.0, 0.7, 1.4, 1.0, 0.9, 1.0});
    const double h = 1e-5;
    for (const double t : {0.1, 0.55, 0.9, 1.6, 1.9}) {
        const LC_CurveJet jet = jetAt(spline, t);
        const LC_CurveJet before = jetAt(spline, t - h);
        const LC_CurveJet after = jetAt(spline, t + h);
        CHECK(compareVector(jet.first, (after.point - before.point) / (2.0 * h), 1e-6));
        CHECK(compareVector(jet.second, (after.first - before.first) / (2.0 * h), 1e-5));
        CHECK(compareVector(jet.point, spline.getPointAt(t), 1e-9));
    }
}

TEST_CASE("RS_Spline::tryEvaluateJet reports failure instead of a zero vector", "[spline][jet]") {
    const RS_Spline spline = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    LC_CurveJet jet;
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    CHECK(t0 == 0.0);
    CHECK(t1 == 1.0);

    CHECK_FALSE(spline.tryEvaluateJet(-0.1, LC_CurveEvaluationSide::Interior, jet));
    CHECK_FALSE(jet.point.valid);
    CHECK_FALSE(spline.tryEvaluateJet(1.1, LC_CurveEvaluationSide::Interior, jet));
    CHECK_FALSE(spline.tryEvaluateJet(std::nan(""), LC_CurveEvaluationSide::Interior, jet));
    CHECK_FALSE(spline.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Left, jet));
    CHECK_FALSE(spline.tryEvaluateJet(1.0, LC_CurveEvaluationSide::Right, jet));
    CHECK(spline.tryEvaluateJet(1.0, LC_CurveEvaluationSide::Left, jet));

    RS_Spline zeroWeight = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1}, {1.0, 0.0, 1.0, 1.0});
    CHECK_FALSE(zeroWeight.tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, jet));

    RS_Spline shortKnots = makeSpline(3, g_bezier, {0, 0, 0, 1, 1, 1});
    CHECK_FALSE(shortKnots.tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, jet));
    CHECK_FALSE(shortKnots.getParameterDomain(t0, t1));
    CHECK(shortKnots.getBreakParameters().empty());

    RS_Spline infinite = makeSpline(3, {{0, 0}, {1, 3}, {INFINITY, -3}, {3, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK_FALSE(infinite.tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, jet));
}

// ---------------------------------------------------------------------------
// Repaired legacy paths. These changed shipping behaviour: open splines now
// report their endpoints (they reported none), and the derivative-based helpers
// report values instead of NaN.
// ---------------------------------------------------------------------------
TEST_CASE("RS_Spline open endpoints are the curve ends; closed has none", "[spline][jet]") {
    const RS_Spline open = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK(compareVector(open.getStartpoint(), g_bezier.front(), 1e-12));
    CHECK(compareVector(open.getEndpoint(), g_bezier.back(), 1e-12));

    double dist = 0.0;
    CHECK(compareVector(open.getNearestEndpoint(RS_Vector{2.9, 0.2}, nullptr, &dist), g_bezier.back(), 1e-12));
    CHECK(dist == Approx(RS_Vector{2.9, 0.2}.distanceTo(g_bezier.back())));

    RS_SplineData closedData(3, true);
    closedData.controlPoints = {{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    closedData.weights = std::vector<double>(closedData.controlPoints.size(), 1.0);
    RS_Spline closed(nullptr, closedData);
    closed.changeType(RS_SplineData::SplineType::WrappedClosed);
    CHECK_FALSE(closed.getStartpoint().valid);
    CHECK_FALSE(closed.getEndpoint().valid);
    CHECK_FALSE(closed.getNearestEndpoint(RS_Vector{0, 0}, nullptr, &dist).valid);
}

TEST_CASE("RS_Spline endpoints include its corners, not its smooth joints", "[spline][jet][RS_Spline]") {
    // a polyline as a degree-1 spline: every vertex where it turns is a corner
    const RS_Spline polyline = makeSpline(1, {{0, 0}, {4, 0}, {4, 3}, {6, 3}}, {0, 0, 1, 2, 3, 3});
    double dist = 0.0;
    CHECK(compareVector(polyline.getNearestEndpoint(RS_Vector{3.8, 0.3}, nullptr, &dist), RS_Vector{4, 0}, 1e-12));
    CHECK(dist == Approx(RS_Vector{3.8, 0.3}.distanceTo(RS_Vector{4, 0})));
    CHECK(compareVector(polyline.getNearestEndpoint(RS_Vector{4.1, 2.8}), RS_Vector{4, 3}, 1e-12));
    CHECK(compareVector(polyline.getNearestEndpoint(RS_Vector{5.9, 3.2}), RS_Vector{6, 3}, 1e-12));
    // a vertex on a straight run is no corner
    const RS_Spline straight = makeSpline(1, {{0, 0}, {2, 0}, {5, 0}}, {0, 0, 1, 2, 2});
    CHECK(compareVector(straight.getNearestEndpoint(RS_Vector{2, 0.1}), RS_Vector{0, 0}, 1e-12));

    // two cubic pieces at a knot of multiplicity 3: a corner where the tangent turns
    const RS_Spline cornered =
        makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 1}, {5, 1}, {6, 0}}, {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    CHECK(compareVector(cornered.getNearestEndpoint(RS_Vector{3.1, 0.2}), RS_Vector{3, 0}, 1e-12));
    // and none where the handles line up
    const RS_Spline smooth =
        makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 0}}, {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    CHECK(compareVector(smooth.getNearestEndpoint(RS_Vector{3.0, 1.1}), RS_Vector{0, 0}, 1e-12));
}

TEST_CASE("RS_Spline::findDerivativeZeros finds two roots in one knot span", "[spline][jet]") {
    // y'(t) = 9 (6t^2 - 6t + 1): roots 1/2 -+ sqrt(3)/6. x'(t) = 3 has none.
    const RS_Spline spline = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    const std::vector<double> yZeros = spline.findDerivativeZeros(false);
    REQUIRE(yZeros.size() == 2);
    CHECK(yZeros[0] == Approx(0.5 - std::sqrt(3.0) / 6.0).margin(1e-10));
    CHECK(yZeros[1] == Approx(0.5 + std::sqrt(3.0) / 6.0).margin(1e-10));
    // A component without roots reports none, not a span midpoint.
    CHECK(spline.findDerivativeZeros(true).empty());
}

TEST_CASE("RS_Spline::calculateTightBorders bounds the curve, not its control polygon",
          "[spline][jet]") {
    RS_Spline spline = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    spline.calculateTightBorders();
    // y(t) = 9 t (1-t) (1-2t) peaks at +-sqrt(3)/2 inside the control hull [-3, 3].
    const double peak = std::sqrt(3.0) / 2.0;
    CHECK(spline.getMin().x == Approx(0.0).margin(1e-10));
    CHECK(spline.getMax().x == Approx(3.0).margin(1e-10));
    CHECK(spline.getMin().y == Approx(-peak).margin(1e-9));
    CHECK(spline.getMax().y == Approx(peak).margin(1e-9));
}

TEST_CASE("RS_Spline::calculateTightBorders finds an extreme at a closed spline's seam", "[spline][jet]") {
    // The seam is the highest point, y = 7/3, but y' there is a rounding error
    // either side of zero rather than zero, so no end interval brackets it.
    RS_SplineData data(3, true);
    data.type = RS_SplineData::SplineType::WrappedClosed;
    data.controlPoints = {{-4, 1}, {-4, 3}, {3, 1}, {4, -1}, {-5, -3}, {-2, 1}, {3, -3}, {-4, 1}, {-4, 3}, {3, 1}};
    data.weights.assign(data.controlPoints.size(), 1.0);
    for (int k = 0; k < 14; ++k) {
        data.knotslist.push_back(0.1 * k);
    }
    RS_Spline spline(nullptr, data);
    REQUIRE(spline.validate());
    spline.calculateTightBorders();
    CHECK(spline.getMax().y == Approx(7.0 / 3.0).margin(1e-9));
}

TEST_CASE("RS_Spline evaluates uniformly tiny weights as the same curve", "[spline][jet]") {
    // Scaling every weight leaves a rational curve unchanged; a denominator below
    // RS_TOLERANCE used to be skipped, shrinking the curve onto the origin.
    RS_SplineData data(3, false);
    data.controlPoints = g_bezier;
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights.assign(4, 1e-11);
    const RS_Spline spline(nullptr, data);
    LC_CurveJet jet;
    REQUIRE(spline.tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, jet));
    CHECK(jet.point.distanceTo(RS_Vector{1.5, 0.0}) < 1e-12);
    CHECK(spline.getEndpoint().distanceTo(RS_Vector{3.0, 0.0}) < 1e-12);
}

TEST_CASE("RS_Spline::getPointAt reports a failure as an invalid point", "[spline][jet]") {
    const RS_Spline spline = makeSpline(3, g_bezier, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK(compareVector(spline.getPointAt(0.25), bezierPoint(0.25), 1e-12));
    CHECK_FALSE(spline.getPointAt(2.0).valid);
}

TEST_CASE("RS_Spline::revertDirection keeps a valid curve, traversed backwards", "[spline][jet][RS_Spline]") {
    // It used to reverse the knot vector and only shift it, leaving it
    // decreasing: the reversed spline failed validation and was no longer drawn.
    RS_Spline forward = makeSpline(3, {{0, 0}, {1, 2}, {3, 3}, {4, 1}, {6, 2}, {7, 0}},
                                   {0, 0, 0, 0, 0.3, 1.2, 2, 2, 2, 2}, {1.0, 0.7, 1.4, 1.0, 0.9, 1.0});
    RS_Spline reversed = makeSpline(3, {{0, 0}, {1, 2}, {3, 3}, {4, 1}, {6, 2}, {7, 0}},
                                    {0, 0, 0, 0, 0.3, 1.2, 2, 2, 2, 2}, {1.0, 0.7, 1.4, 1.0, 0.9, 1.0});
    reversed.revertDirection();
    REQUIRE(reversed.validate());
    CHECK(reversed.count() > 0); // drawn
    CHECK(compareVector(reversed.getStartpoint(), forward.getEndpoint(), 1e-12));
    CHECK(compareVector(reversed.getEndpoint(), forward.getStartpoint(), 1e-12));
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(forward.getParameterDomain(t0, t1));
    for (const double t : {0.1, 0.55, 1.2, 1.6}) {
        const LC_CurveJet a = jetAt(forward, t);
        const LC_CurveJet b = jetAt(reversed, t0 + t1 - t, LC_CurveEvaluationSide::Left);
        CHECK(compareVector(a.point, b.point, 1e-12));
        CHECK(compareVector(a.first, -b.first, 1e-10));
        CHECK(compareVector(a.second, b.second, 1e-9));
    }

    // a closed spline stays closed and wrapped
    RS_Spline closed(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        closed.addControlPoint(p);
    }
    closed.setClosed(true);
    REQUIRE(closed.validate());
    REQUIRE(closed.getParameterDomain(t0, t1));
    std::vector<RS_Vector> onCurve;
    for (int i = 0; i < 8; ++i) {
        onCurve.push_back(closed.getPointAt(t0 + (t1 - t0) * (i + 0.5) / 8.0));
        REQUIRE(onCurve.back().valid);
    }
    closed.revertDirection();
    REQUIRE(closed.validate());
    CHECK(closed.isClosed());
    CHECK(closed.hasWrappedControlPoints());
    // still the same shape: every former curve point lies on the reversed curve
    REQUIRE(closed.getParameterDomain(t0, t1));
    for (const RS_Vector& p : onCurve) {
        double nearest = RS_MAXDOUBLE;
        for (int i = 0; i <= 4000; ++i) {
            nearest = std::min(nearest, closed.getPointAt(t0 + (t1 - t0) * i / 4000.0).distanceTo(p));
        }
        CHECK(nearest < 0.05);
    }
}

TEST_CASE("RS_Spline snaps to middle points and distances along it", "[spline][jet][RS_Spline]") {
    // Offsets of spline sources are RS_Splines; like the LC_SplinePoints they
    // replace in Draw > Parallel, they must offer middle and distance snaps.
    const RS_Spline line = makeSpline(1, {{0.0, 0.0}, {10.0, 0.0}}, {0, 0, 1, 1});
    double dist = 0.0;
    CHECK(compareVector(line.getNearestMiddle(RS_Vector{4.0, 1.0}, &dist, 1), RS_Vector{5.0, 0.0}, 1e-9));
    CHECK(dist == Approx(std::hypot(1.0, 1.0)));
    CHECK(compareVector(line.getNearestMiddle(RS_Vector{1.0, 0.0}, &dist, 3), RS_Vector{2.5, 0.0}, 1e-9));
    CHECK(compareVector(line.getNearestDist(2.0, RS_Vector{1.0, 0.0}, &dist), RS_Vector{2.0, 0.0}, 1e-9));
    CHECK(compareVector(line.getNearestDist(2.0, RS_Vector{9.0, 0.0}, &dist), RS_Vector{8.0, 0.0}, 1e-9));
    CHECK_FALSE(line.getNearestDist(11.0, RS_Vector{1.0, 0.0}, &dist).valid);

    // a rational arc is not parameterized by arc length: its middle is the true 45 degree point
    const double w = std::sqrt(0.5);
    const RS_Spline arc = makeSpline(2, {{10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, {0, 0, 0, 1, 1, 1}, {1.0, w, 1.0});
    CHECK(compareVector(arc.getNearestMiddle(RS_Vector{8.0, 8.0}, &dist, 1), RS_Vector{10.0 * w, 10.0 * w}, 1e-6));
    const double quarter = M_PI * 10.0 / 2.0;
    const RS_Vector third = arc.getNearestDist(quarter / 3.0, RS_Vector{10.0, 0.0}, &dist);
    CHECK(compareVector(third, RS_Vector{10.0 * std::cos(M_PI / 6.0), 10.0 * std::sin(M_PI / 6.0)}, 1e-6));

    RS_Spline closed(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50}}) {
        closed.addControlPoint(p);
    }
    closed.setClosed(true);
    CHECK_FALSE(closed.getNearestMiddle(RS_Vector{0, 0}, &dist, 1).valid);
    CHECK_FALSE(closed.getNearestDist(1.0, RS_Vector{0, 0}, &dist).valid);
}

TEST_CASE("RS_Spline::tryStroke bounds each chord by the second derivative", "[spline][jet][RS_Spline]") {
    // degree 1: no second derivative, so the vertices are the control points
    RS_SplineData polygon(1, false);
    polygon.controlPoints = {{0, 0}, {3, 1}, {5, -2}, {9, 4}};
    polygon.knotslist = {0, 0, 1, 2, 3, 3};
    polygon.weights.assign(4, 1.0);
    std::vector<RS_Vector> vertices;
    REQUIRE(RS_Spline(nullptr, polygon).tryStroke(1e-3, 100, vertices));
    REQUIRE(vertices.size() == 4);
    for (size_t i = 0; i < 4; ++i) {
        CHECK(vertices[i].distanceTo(polygon.controlPoints[i]) < 1e-12);
    }

    // a parabola arc, C(t) = (2t, 4t^2) on [0, 1], that is y = x^2: |C''| = 8
    // everywhere, so a chord of parameter length h strays at most h^2 / 8 * 8 = h^2;
    // its vertices lie on the curve
    RS_SplineData parabola(2, false);
    parabola.controlPoints = {{0, 0}, {1, 0}, {2, 4}};
    parabola.knotslist = {0, 0, 0, 1, 1, 1};
    parabola.weights.assign(3, 1.0);
    const RS_Spline arc(nullptr, parabola);
    const double tolerance = 1e-3;
    REQUIRE(arc.tryStroke(tolerance, 1000, vertices));
    CHECK(vertices.size() > 2);
    for (size_t i = 1; i < vertices.size(); ++i) {
        CHECK(std::abs(vertices[i].y - vertices[i].x * vertices[i].x) < 1e-12);
        const double h = (vertices[i].x - vertices[i - 1].x) / 2.0; // in parameter
        CHECK(h * h <= tolerance);
    }

    // past the vertex limit, or with no usable tolerance: nothing
    CHECK_FALSE(arc.tryStroke(tolerance, 3, vertices));
    CHECK(vertices.empty());
    CHECK_FALSE(arc.tryStroke(0.0, 1000, vertices));
    CHECK_FALSE(arc.tryStroke(std::nan(""), 1000, vertices));
}

TEST_CASE("RS_Spline::tryStroke starts a span after a break at its own point", "[spline][jet][RS_Spline]") {
    // a knot of multiplicity degree + 1 in the middle: two unconnected lines
    RS_SplineData broken(1, false);
    broken.controlPoints = {{0, 0}, {4, 0}, {4, 3}, {8, 3}};
    broken.knotslist = {0, 0, 1, 1, 2, 2};
    broken.weights.assign(4, 1.0);
    const RS_Spline spline(nullptr, broken);
    REQUIRE(spline.validate());
    std::vector<RS_Vector> vertices;
    REQUIRE(spline.tryStroke(1e-3, 100, vertices));
    REQUIRE(vertices.size() == 4);
    CHECK(vertices[1].distanceTo(RS_Vector{4, 0}) < 1e-12);
    CHECK(vertices[2].distanceTo(RS_Vector{4, 3}) < 1e-12);
}

namespace {
    // A Standard spline with a repeated inner knot where the periodic knot vector made from its knots
    // ends: closing it gives two equal last knots, which validate() rejects.
    RS_SplineData repeatedKnotSpline() {
        RS_SplineData data(3, false);
        data.type = RS_SplineData::SplineType::Standard;
        data.controlPoints = {{0., 0.}, {10., 20.}, {20., -20.}, {30., 0.}, {40., 15.}, {50., -5.}};
        data.weights.assign(data.controlPoints.size(), 1.);
        data.knotslist = {0., 1., 2., 3., 4., 5., 5., 6., 7., 8.};
        return data;
    }
}

TEST_CASE("RS_Spline stays drawn when a type change would make it invalid", "[RS_Spline]")
{
    const RS_SplineData data = repeatedKnotSpline();

    SECTION("setClosed")
    {
        RS_Spline spline(nullptr, data);
        REQUIRE(spline.validate());
        REQUIRE(spline.count() > 0);
        spline.setClosed(true);
        CHECK(spline.validate());
        CHECK(spline.count() > 0);
        CHECK_FALSE(spline.isClosed());
        CHECK(spline.getData().knotslist == data.knotslist);
        CHECK(spline.getData().controlPoints.size() == data.controlPoints.size());
    }

    SECTION("changeType")
    {
        RS_Spline spline(nullptr, data);
        spline.changeType(RS_SplineData::SplineType::WrappedClosed);
        spline.update();
        CHECK(spline.getData().type == RS_SplineData::SplineType::Standard);
        CHECK(spline.validate());
        CHECK(spline.count() > 0);
    }
}

TEST_CASE("RS_Spline keeps its type when the data cannot be converted", "[RS_Spline]")
{
    // no knot vector, which LC_SplineHelper refuses to convert: setting WrappedClosed anyway gave
    // control points that are not wrapped
    RS_SplineData data = repeatedKnotSpline();
    data.knotslist.clear();
    RS_Spline spline(nullptr, data);
    spline.changeType(RS_SplineData::SplineType::WrappedClosed);
    CHECK(spline.getData().type == RS_SplineData::SplineType::Standard);
    CHECK_FALSE(spline.isClosed());
    CHECK(spline.getData().controlPoints.size() == data.controlPoints.size());
}
