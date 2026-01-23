/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for LC_Quadratic
**
** Copyright (C) 2015-2026 LibreCAD.org
** Copyright (C) 2015-2026 Dongxu Li (dongxuli2011@gmail.com)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>

#include "lc_quadratic.h"
#include "rs_vector.h"
#include "rs_line.h"
#include "rs_circle.h"

// Helper: build a circle quadratic (coefficient form) centered at (cx,cy) with radius r
static LC_Quadratic makeCircleCoeffs(double cx, double cy, double r) {
    // (x-cx)^2 + (y-cy)^2 - r^2 == 0
    // => x^2 + y^2 - 2cx x - 2cy y + (cx^2 + cy^2 - r^2) == 0
    double A = 1.0;
    double B = 0.0;
    double C = 1.0;
    double D = -2.0 * cx;
    double E = -2.0 * cy;
    double F = cx * cx + cy * cy - r * r;
    return LC_Quadratic({A, B, C, D, E, F});
}

TEST_CASE("License/header sanity test present", "[meta]") {
    // Dummy check to ensure test file included correctly.
    REQUIRE(true);
}

TEST_CASE("Perpendicular bisector constructor produces expected vertical line", "[constructor][bisector]") {
    RS_Vector p0(0.0, 0.0);
    RS_Vector p1(2.0, 0.0);

    LC_Quadratic bis = LC_Quadratic(p0, p1);
    REQUIRE(bis.isValid());
    REQUIRE(!bis.isQuadratic());

    auto ce = bis.getCoefficients();
    // Non-quadratic getCoefficients should return 3 entries D, E, F forming D x + E y + F == 0
    REQUIRE(ce.size() >= 3);
    // Vertical line x = 1  => 1*x + 0*y - 1 == 0
    CHECK(ce[0] == Catch::Approx(1.0).margin(1e-12));
    CHECK(ce[1] == Catch::Approx(0.0).margin(1e-12));
    CHECK(ce[2] == Catch::Approx(-1.0).margin(1e-12));
}

TEST_CASE("Line + point on line produces non-quadratic (line) result", "[constructor][line-point-on]") {
    // horizontal line y = 0 from (0,0) to (2,0)
    RS_Line line(RS_Vector(0.0, 0.0), RS_Vector(2.0, 0.0));
    RS_Vector point_on_line(1.0, 0.0);

    LC_Quadratic q(&line, point_on_line);
    REQUIRE(q.isValid());
    REQUIRE(!q.isQuadratic());

    auto ce = q.getCoefficients();
    REQUIRE(ce.size() >= 3);
    // Expect line y = 0 -> D = 0, E = -1, F = 0 OR an equivalent scaled representation.
    // The constructor sets linear as (direction.y, -direction.x) for horizontal line => (0, -1)
    CHECK(ce[0] == Catch::Approx(0.0).margin(1e-12));  // D
    CHECK(ce[1] != Catch::Approx(0.0).margin(1e-12)); // E
}

TEST_CASE("Line + point off-line produces quadratic (parabola) result", "[constructor][line-point-off]") {
    RS_Line line(RS_Vector(0.0, 0.0), RS_Vector(2.0, 0.0));
    RS_Vector point_off_line(1.0, 1.0);

    LC_Quadratic q(&line, point_off_line);
    REQUIRE(q.isValid());
    REQUIRE(q.isQuadratic());

    // Basic sanity: evaluate at the projection/midpoint region gives finite values
    RS_Vector projection = line.getNearestPointOnEntity(point_off_line, false);
    RS_Vector center = (projection + point_off_line) * 0.5;
    // evaluateAt should accept valid vectors
    double val = q.evaluateAt(center);
    // parabola equation in canonical form should not be NaN/inf
    CHECK(std::isfinite(val));
}

TEST_CASE("Circle + external point constructor (center path) produces valid conic", "[constructor][circle-point]") {
    RS_CircleData cd(RS_Vector(0.0, 0.0), 2.0);
    RS_Circle circle(cd);
    RS_Vector p(3.0, 0.0); // point outside the circle

    LC_Quadratic q(&circle, p);
    REQUIRE(q.isValid());
    REQUIRE(q.isQuadratic());

    // evaluateAt at the midpoint between circle center and point/expected center of locus
    RS_Vector mid = (circle.getCenter() + p) * 0.5;
    double v = q.evaluateAt(mid);
    CHECK(std::isfinite(v));
}

TEST_CASE("Two circles with equal radius produce degenerate hyperbola (treated as quadratic)", "[constructor][circle-circle-equal-radius]") {
    RS_CircleData cd1(RS_Vector(0.0, 0.0), 1.0);
    RS_CircleData cd2(RS_Vector(2.0, 0.0), 1.0);
    RS_Circle c1(cd1);
    RS_Circle c2(cd2);

    LC_Quadratic q(&c1, &c2, false);
    REQUIRE(q.isValid());
    REQUIRE(q.isQuadratic());

    // Coefficients are present and finite
    auto ce = q.getCoefficients();
    REQUIRE(ce.size() == 6);
    for (double coeff : ce) {
        CHECK(std::isfinite(coeff));
    }
}

TEST_CASE("Two circles intersection (coeff-based) gives two points (standard example)", "[intersection][circle-circle]") {
    // circle1 at (0,0) r=1 ; circle2 at (1,0) r=1
    LC_Quadratic c1 = makeCircleCoeffs(0.0, 0.0, 1.0);
    LC_Quadratic c2 = makeCircleCoeffs(1.0, 0.0, 1.0);

    auto sol = LC_Quadratic::getIntersection(c1, c2);
    REQUIRE(sol.size() == 2);

    double xexp = 0.5;
    double yexp = std::sqrt(3.0) / 2.0;
    bool found_pos = false, found_neg = false;
    for (auto const& p : sol) {
        if (p.x == Catch::Approx(xexp).margin(1e-9) && p.y == Catch::Approx(yexp).margin(1e-9)) found_pos = true;
        if (p.x == Catch::Approx(xexp).margin(1e-9) && p.y == Catch::Approx(-yexp).margin(1e-9)) found_neg = true;
    }
    CHECK(found_pos);
    CHECK(found_neg);
}

TEST_CASE("Line-line intersection: intersecting and parallel cases", "[intersection][line-line]") {
    // intersecting lines x=1 and y=2
    LC_Quadratic l1({1.0, 0.0, -1.0});
    LC_Quadratic l2({0.0, 1.0, -2.0});

    auto sol = LC_Quadratic::getIntersection(l1, l2);
    REQUIRE(sol.size() == 1);
    CHECK(sol[0].x == Catch::Approx(1.0).margin(1e-12));
    CHECK(sol[0].y == Catch::Approx(2.0).margin(1e-12));

    // parallel lines x=1 and x=2 -> no intersection
    LC_Quadratic p1({1.0, 0.0, -1.0});
    LC_Quadratic p2({1.0, 0.0, -2.0});
    auto solp = LC_Quadratic::getIntersection(p1, p2);
    CHECK(solp.empty());
}

TEST_CASE("Degenerate/invalid constructions return invalid LC_Quadratic", "[constructor][degenerate]") {
    // Passing null pointer should yield invalid quadratic
    LC_Quadratic invalid(nullptr, RS_Vector(0.0,0.0));
    CHECK(!invalid.isValid());

    // Two lines that are parallel: construct two RS_Line and pass to two-entity constructor
    RS_Line l0(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0));
    RS_Line l1(RS_Vector(0.0, 1.0), RS_Vector(1.0, 1.0));
    LC_Quadratic q(&l0, &l1, false);
    // According to implementation, no unique center -> m_bValid false
    CHECK(!q.isValid());
}

TEST_CASE("FlipXY symmetry: intersections stable under flip and flip-back", "[flipXY][intersection]") {
    LC_Quadratic circ = makeCircleCoeffs(0.0, 0.0, 2.0);
    LC_Quadratic line({0.0, 1.0, -1.0}); // y = 1

    auto sol1 = LC_Quadratic::getIntersection(circ, line);
    REQUIRE(sol1.size() == 2);

    auto sol2 = LC_Quadratic::getIntersection(circ.flipXY(), line.flipXY()).flipXY();
    REQUIRE(sol2.size() == sol1.size());

    for (auto const& p : sol1) {
        bool found = false;
        for (auto const& q : sol2) {
            if (p.x == Catch::Approx(q.x).margin(1e-9) && p.y == Catch::Approx(q.y).margin(1e-9)) {
                found = true;
                break;
            }
        }
        CHECK(found);
    }
}
