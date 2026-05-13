// File: lc_parabola_tests.cpp
// Catch2 unit tests for LC_Parabola and related functionality.

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <array>
#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "drw_entities.h"
#include "lc_parabola.h"
#include "lc_parabolaspline.h"
#include "lc_quadratic.h"
#include "lc_secondmoment.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_vector.h"

using Catch::Approx;

namespace {

constexpr double kTol = 1e-9;

// Build a canonical "y = x^2 / (4h) on x in [-a, a]" parabola.
LC_Parabola makeCanonicalParabola(double a, double h) {
  const double yEnds = a * a / (4.0 * h);
  LC_ParabolaData d{std::array<RS_Vector, 3>{
      RS_Vector{-a, yEnds}, RS_Vector{0.0, -yEnds}, RS_Vector{a, yEnds}}};
  return LC_Parabola{nullptr, d};
}

// Sample a quadratic Bezier control polygon at parameter t.
RS_Vector bezierAt(const std::array<RS_Vector, 3> &cps, double t) {
  const double mt = 1.0 - t;
  return cps[0] * (mt * mt) + cps[1] * (2.0 * mt * t) + cps[2] * (t * t);
}

} // namespace

TEST_CASE("LC_Parabola: CalculatePrimitives derives focus/vertex/axis",
          "[parabola][primitives]") {
  SECTION("y = x^2/(4h) with a=1, h=1") {
    auto p = makeCanonicalParabola(1.0, 1.0);
    const auto &d = p.getData();
    REQUIRE(d.isValid());
    REQUIRE(d.m_vertex.x == Approx(0.0).margin(kTol));
    REQUIRE(d.m_vertex.y == Approx(0.0).margin(kTol));
    // Focus at (0, h); accessor and cached primitive must agree
    REQUIRE(d.m_focus.x == Approx(0.0).margin(kTol));
    REQUIRE(d.m_focus.y == Approx(1.0).margin(kTol));
    REQUIRE(d.GetFocus().x == Approx(0.0).margin(kTol));
    REQUIRE(d.GetFocus().y == Approx(1.0).margin(kTol));
    // Axis points from vertex to focus, magnitude == h
    REQUIRE(d.m_axis.magnitude() == Approx(1.0).margin(kTol));
    // Directrix is perpendicular to the axis at y = -h
    const RS_LineData dx = d.GetDirectrix();
    REQUIRE(dx.startpoint.y == Approx(-1.0).margin(kTol));
    REQUIRE(dx.endpoint.y == Approx(-1.0).margin(kTol));
    // Direction must be perpendicular to the axis vector
    const RS_Vector dxDir = dx.endpoint - dx.startpoint;
    REQUIRE(std::abs(dxDir.dotP(d.m_axis)) < kTol);
  }

  SECTION("Larger scale a=100, h=50") {
    auto p = makeCanonicalParabola(100.0, 50.0);
    const auto &d = p.getData();
    REQUIRE(d.isValid());
    REQUIRE(d.m_axis.magnitude() == Approx(50.0).margin(1e-6));
  }
}

TEST_CASE("LC_Parabola: getQuadratic vanishes on sampled curve points",
          "[parabola][quadratic]") {
  auto p = makeCanonicalParabola(1.0, 1.0);
  LC_Quadratic q = p.getQuadratic();
  const auto &cps = p.getData().m_controlPoints;
  for (double t = 0.0; t <= 1.0; t += 0.1) {
    const RS_Vector pt = bezierAt(cps, t);
    // Evaluate m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5
    const auto &c = q.getCoefficients();
    const double v = c[0] * pt.x * pt.x + c[1] * pt.x * pt.y +
                     c[2] * pt.y * pt.y + c[3] * pt.x + c[4] * pt.y + c[5];
    REQUIRE(v == Approx(0.0).margin(1e-9));
  }
}

TEST_CASE("LC_Parabola: line integrals match analytic values",
          "[parabola][integral][regression]") {
  // Analytic values for y = x^2/(4h) on x in [-1, 1] with h = 1:
  //   area  = a^3 / (3h)  = 1/3
  //   mx    = 0  (symmetry)
  //   my    = -1 / (80 h^2) = -1/80
  //   ixx   = 1 / (15 h)    =  1/15
  //   iyy   = -1 / (672 h^3) = -1/672
  //   ixy   = 0  (symmetry)
  auto p = makeCanonicalParabola(1.0, 1.0);

  REQUIRE(p.areaLineIntegral() == Approx(1.0 / 3.0).margin(1e-12));

  const auto m1 = p.firstMomentLineIntegral();
  REQUIRE(m1.mx == Approx(0.0).margin(1e-12));
  REQUIRE(m1.my == Approx(-1.0 / 80.0).margin(1e-12));

  const auto m2 = p.secondMomentLineIntegral();
  REQUIRE(m2.ixx == Approx(1.0 / 15.0).margin(1e-12));
  REQUIRE(m2.iyy == Approx(-1.0 / 672.0).margin(1e-12));
  REQUIRE(m2.ixy == Approx(0.0).margin(1e-12));
}

TEST_CASE("LC_Parabola: approximateOffset is perpendicular at endpoints",
          "[parabola][offset]") {
  auto p = makeCanonicalParabola(1.0, 1.0);
  const double dist = 0.1;
  auto offsetPtr = p.approximateOffset(dist);
  REQUIRE(offsetPtr != nullptr);
  REQUIRE(offsetPtr->getData().isValid());

  const auto &origCps = p.getData().m_controlPoints;
  const auto &offCps = offsetPtr->getData().m_controlPoints;

  // The endpoints of the offset must be at perpendicular distance `dist`
  // from the corresponding original endpoints (tangent-preserving offset
  // is exact at the endpoints).
  const double d0 = origCps.front().distanceTo(offCps.front());
  const double d2 = origCps.back().distanceTo(offCps.back());
  REQUIRE(d0 == Approx(dist).margin(1e-12));
  REQUIRE(d2 == Approx(dist).margin(1e-12));

  // Offset endpoint - original endpoint must be perpendicular to the
  // tangent at that endpoint.
  const RS_Vector tan0 = (origCps[1] - origCps[0]).normalized();
  const RS_Vector tan2 = (origCps[2] - origCps[1]).normalized();
  const RS_Vector r0 = (offCps.front() - origCps.front()).normalized();
  const RS_Vector r2 = (offCps.back() - origCps.back()).normalized();
  REQUIRE(std::abs(r0.dotP(tan0)) < 1e-12);
  REQUIRE(std::abs(r2.dotP(tan2)) < 1e-12);
}

TEST_CASE("LC_Parabola: getNearestOrthTan onEntity check uses point coords",
          "[parabola][tangent][regression]") {
  // For the canonical y=x^2/4h parabola on x in [-1,1], the tangent
  // perpendicular to the y-axis (line direction (0,1)) lies at the vertex
  // (x=0, y=0), which is on the entity. The check should return that
  // point, not reject it because of a unit-vector vs. coordinate mix-up.
  auto p = makeCanonicalParabola(1.0, 1.0);
  RS_Line normal{nullptr,
                 RS_LineData{RS_Vector{0.0, 0.0}, RS_Vector{0.0, 1.0}}};
  const RS_Vector tp = p.getNearestOrthTan(RS_Vector{0.0, 0.0}, normal, true);
  REQUIRE(tp.valid);
  REQUIRE(tp.x == Approx(0.0).margin(1e-12));
  REQUIRE(tp.y == Approx(0.0).margin(1e-12));
}

TEST_CASE("LC_Parabola: From4Points reconstructs at realistic scale",
          "[parabola][reconstruction][regression]") {
  // Construct a parabola, sample 4 points spread over a 200-unit span,
  // and verify that From4Points returns at least one valid candidate
  // matching the original. Pre-fix, the fitTolerance test rejected every
  // candidate because RS_TOLERANCE2 (~1e-20) is unreachable at this scale.
  const double a = 100.0;
  const double h = 50.0;
  auto orig = makeCanonicalParabola(a, h);
  const auto &cps = orig.getData().m_controlPoints;

  std::vector<RS_Vector> samples = {bezierAt(cps, 0.0), bezierAt(cps, 0.3),
                                    bezierAt(cps, 0.7), bezierAt(cps, 1.0)};

  auto candidates = LC_ParabolaData::From4Points(samples);
  REQUIRE_FALSE(candidates.empty());

  bool foundMatch = false;
  for (const auto &cand : candidates) {
    if (!cand.isValid())
      continue;
    if (cand.m_vertex.distanceTo(orig.getData().m_vertex) < 1e-3 &&
        std::abs(cand.m_axis.magnitude() - orig.getData().m_axis.magnitude()) <
            1e-3) {
      foundMatch = true;
      break;
    }
  }
  REQUIRE(foundMatch);
}

TEST_CASE("LC_Parabola: getLength matches numerical integration",
          "[parabola][length]") {
  auto p = makeCanonicalParabola(1.0, 1.0);
  const auto &cps = p.getData().m_controlPoints;

  // Composite Simpson on 1000 sub-intervals of the speed function:
  //   |dP/dt| dt over t in [0,1]
  auto speedAt = [&](double t) {
    const double mt = 1.0 - t;
    const RS_Vector dp =
        (cps[1] - cps[0]) * (2.0 * mt) + (cps[2] - cps[1]) * (2.0 * t);
    return dp.magnitude();
  };
  const int n = 1000;
  const double dt = 1.0 / n;
  double s = speedAt(0.0) + speedAt(1.0);
  for (int i = 1; i < n; ++i) {
    const double t = i * dt;
    s += (i % 2 ? 4.0 : 2.0) * speedAt(t);
  }
  const double numeric = s * dt / 3.0;

  REQUIRE(p.getLength() == Approx(numeric).margin(1e-6));
}

TEST_CASE("LC_Parabola: revertDirection preserves area-magnitude / shape",
          "[parabola][revert]") {
  auto p1 = makeCanonicalParabola(1.0, 1.0);
  auto p2 = p1;
  p2.revertDirection();

  // Reversing the direction flips the sign of the line integral.
  REQUIRE(p2.areaLineIntegral() ==
          Approx(-p1.areaLineIntegral()).margin(1e-12));

  // The set of control points (start/middle/end) is the same up to swap.
  const auto &c1 = p1.getData().m_controlPoints;
  const auto &c2 = p2.getData().m_controlPoints;
  REQUIRE(c1[0].distanceTo(c2[2]) < 1e-12);
  REQUIRE(c1[2].distanceTo(c2[0]) < 1e-12);
  REQUIRE(c1[1].distanceTo(c2[1]) < 1e-12);
}

// ---- LC_ParabolaSpline roundtrip + rejection ------------------------------

namespace {

LC_ParabolaData parabolaFromControlPoints(const std::array<RS_Vector, 3> &cps) {
  return LC_ParabolaData{cps};
}

// Encode-then-decode a parabola via parabolaToSpline / splineToParabola and
// assert that the reconstructed control points coincide with the originals.
void assertRoundtrip(const std::array<RS_Vector, 3> &cps) {
  LC_ParabolaData pd0{cps};
  REQUIRE(pd0.isValid());

  DRW_Spline spl;
  REQUIRE(LC_ParabolaSpline::parabolaToSpline(pd0, spl));
  REQUIRE(spl.degree == 2);
  REQUIRE(spl.controllist.size() == 3u);
  // Per project decision the encoder emits a non-rational quadratic (no
  // weights) — mathematically equivalent to weights {1,1,1}.
  REQUIRE(spl.weightlist.empty());
  REQUIRE(spl.knotslist.size() == 6u);

  auto pPtr = LC_ParabolaSpline::splineToParabola(spl, nullptr);
  REQUIRE(pPtr != nullptr);

  const auto &cps2 = pPtr->getData().m_controlPoints;
  for (size_t i = 0; i < 3; ++i) {
    REQUIRE(cps[i].distanceTo(cps2[i]) < 1e-9);
  }

  // Focus / vertex / axis recovery must agree within tolerance.
  REQUIRE(pd0.m_focus.distanceTo(pPtr->getData().m_focus) < 1e-9);
  REQUIRE(pd0.m_vertex.distanceTo(pPtr->getData().m_vertex) < 1e-9);
  REQUIRE(pd0.m_axis.distanceTo(pPtr->getData().m_axis) < 1e-9);
}

} // namespace

TEST_CASE("LC_ParabolaSpline: encode/decode roundtrip preserves geometry",
          "[parabola][spline][roundtrip]") {
  SECTION("vertex at origin, axis +Y, symmetric") {
    assertRoundtrip(
        {RS_Vector{-1.0, 1.0}, RS_Vector{0.0, -1.0}, RS_Vector{1.0, 1.0}});
  }

  SECTION("vertex offset, axis rotated 30°") {
    // Take the canonical parabola, then rotate+translate every control
    // point so the parabola has a non-trivial axis direction and origin.
    const double a = 30.0 * M_PI / 180.0;
    const double cs = std::cos(a), sn = std::sin(a);
    auto rot = [cs, sn](RS_Vector v) {
      return RS_Vector{v.x * cs - v.y * sn, v.x * sn + v.y * cs};
    };
    const RS_Vector off{2.5, -1.25};
    std::array<RS_Vector, 3> base{RS_Vector{-2.0, 4.0}, RS_Vector{0.0, -4.0},
                                  RS_Vector{2.0, 4.0}};
    std::array<RS_Vector, 3> cps{rot(base[0]) + off, rot(base[1]) + off,
                                 rot(base[2]) + off};
    assertRoundtrip(cps);
  }

  SECTION("very flat parabola (large axis magnitude)") {
    assertRoundtrip(
        {RS_Vector{-10.0, 0.01}, RS_Vector{0.0, -0.01}, RS_Vector{10.0, 0.01}});
  }

  SECTION("tight parabola (small axis magnitude)") {
    assertRoundtrip(
        {RS_Vector{-0.1, 1.0}, RS_Vector{0.0, -1.0}, RS_Vector{0.1, 1.0}});
  }
}

TEST_CASE("LC_ParabolaSpline: rejects non-parabola splines",
          "[parabola][spline][rejection]") {
  auto makeSpline = [](int degree, std::vector<RS_Vector> cps,
                       std::vector<double> weights = {},
                       std::vector<double> knots = {0, 0, 0, 1, 1, 1}) {
    DRW_Spline s;
    s.degree = degree;
    s.knotslist = std::move(knots);
    s.weightlist = std::move(weights);
    for (auto &v : cps) {
      s.controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y, 0.0));
    }
    s.ncontrol = static_cast<int>(s.controllist.size());
    s.nknots = static_cast<int>(s.knotslist.size());
    return s;
  };

  SECTION("rational with w=0.5 (ellipse arc) → not a parabola") {
    DRW_Spline s =
        makeSpline(2, {RS_Vector{-1, 0}, RS_Vector{0, 1}, RS_Vector{1, 0}},
                   {1.0, 0.5, 1.0});
    REQUIRE_FALSE(LC_ParabolaSpline::isParabolaSpline(s));
    REQUIRE(LC_ParabolaSpline::splineToParabola(s, nullptr) == nullptr);
  }

  SECTION("rational with w=2 (hyperbola) → not a parabola") {
    DRW_Spline s =
        makeSpline(2, {RS_Vector{-1, 0}, RS_Vector{0, 1}, RS_Vector{1, 0}},
                   {1.0, 2.0, 1.0});
    REQUIRE_FALSE(LC_ParabolaSpline::isParabolaSpline(s));
    REQUIRE(LC_ParabolaSpline::splineToParabola(s, nullptr) == nullptr);
  }

  SECTION("collinear control points → degenerate, not a parabola") {
    DRW_Spline s =
        makeSpline(2, {RS_Vector{0, 0}, RS_Vector{1, 0}, RS_Vector{2, 0}});
    REQUIRE_FALSE(LC_ParabolaSpline::isParabolaSpline(s));
    REQUIRE(LC_ParabolaSpline::splineToParabola(s, nullptr) == nullptr);
  }

  SECTION("degree 3 → not a quadratic conic") {
    DRW_Spline s = makeSpline(
        3, {RS_Vector{0, 0}, RS_Vector{1, 1}, RS_Vector{2, 1}, RS_Vector{3, 0}},
        {}, {0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE_FALSE(LC_ParabolaSpline::isParabolaSpline(s));
    REQUIRE(LC_ParabolaSpline::splineToParabola(s, nullptr) == nullptr);
  }

  SECTION("rational with w=1 explicitly → still a parabola (accepted)") {
    DRW_Spline s =
        makeSpline(2, {RS_Vector{-1, 1}, RS_Vector{0, -1}, RS_Vector{1, 1}},
                   {1.0, 1.0, 1.0});
    REQUIRE(LC_ParabolaSpline::isParabolaSpline(s));
    auto p = LC_ParabolaSpline::splineToParabola(s, nullptr);
    REQUIRE(p != nullptr);
    REQUIRE(p->getData().isValid());
  }

  SECTION("non-canonical knot vector → not a single-segment quadratic Bézier") {
    DRW_Spline s =
        makeSpline(2, {RS_Vector{-1, 1}, RS_Vector{0, -1}, RS_Vector{1, 1}}, {},
                   {0, 0, 0, 0.5, 1, 1});
    REQUIRE_FALSE(LC_ParabolaSpline::isParabolaSpline(s));
  }
}
