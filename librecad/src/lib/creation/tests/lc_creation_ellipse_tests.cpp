/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

// Draw Ellipse (4 Points) makes the ellipse with its axes along x and y through four points. Since
// the constructions moved to LC_CreationEllipse (#2540) createEllipseFrom4P() was a copy of
// createEllipseFromCenter3Points(): it took the first of the four points as the center and fitted the
// other three, so the ellipse did not pass through the first click and had another center.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include "lc_actiontestsupport.h"
#include "lc_creation_ellipse.h"
#include "rs_actiondrawellipse4points.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_vector.h"

namespace {

using lc::test::ActionFixture;
using lc::test::eventAt;

constexpr double kTwoPi = 2. * M_PI;

/// how far \p point is from lying on the ellipse of \p data: the implicit equation in the frame of the axes, minus 1
double residual(const RS_EllipseData& data, const RS_Vector& point) {
    const double a = data.majorP.magnitude();
    const double b = a * data.ratio;
    const RS_Vector u = data.majorP / a;
    const RS_Vector delta = point - data.center;
    const double x = (delta.x * u.x + delta.y * u.y) / a;
    const double y = (-delta.x * u.y + delta.y * u.x) / b;
    return (x * x) + (y * y) - 1.;
}

RS_VectorSolutions points(std::initializer_list<RS_Vector> list) {
    RS_VectorSolutions solutions;
    for (const RS_Vector& point : list) {
        solutions.push_back(point);
    }
    return solutions;
}

struct FourPointsProbe final : RS_ActionDrawEllipse4Points {
    explicit FourPointsProbe(LC_ActionContext* context) : RS_ActionDrawEllipse4Points(context) {}
    using RS_ActionDrawEllipse4Points::onMouseLeftButtonRelease;
};

void click(FourPointsProbe& action, const RS_Vector& point) {
    const LC_MouseEvent event = eventAt(point.x, point.y);
    action.onMouseLeftButtonRelease(action.getStatus(), &event);
}

} // namespace

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P finds the ellipse with axes along x and y", "[creation-ellipse]") {
    // center (2, 3), semi-axes 5 (along x) and 3
    RS_EllipseData data;
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(
        points({{7., 3.}, {2., 6.}, {2. + (5. * std::cos(0.7)), 3. + (3. * std::sin(0.7))}, {2. - 5., 3.}}), data));
    CHECK(data.center.x == Catch::Approx(2.));
    CHECK(data.center.y == Catch::Approx(3.));
    CHECK(data.majorP.x == Catch::Approx(5.));
    CHECK(data.majorP.y == Catch::Approx(0.).margin(1e-9));
    CHECK(data.ratio == Catch::Approx(0.6));
    CHECK_FALSE(data.reversed);
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P puts the longer axis on the major axis", "[creation-ellipse]") {
    // center (-4, 1), semi-axes 3 (along x) and 8 (along y)
    RS_EllipseData data;
    const double a = 3.;
    const double b = 8.;
    auto onEllipse = [&](const double t) { return RS_Vector{-4. + (a * std::cos(t)), 1. + (b * std::sin(t))}; };
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({onEllipse(0.3), onEllipse(1.9), onEllipse(3.5), onEllipse(5.1)}), data));
    CHECK(data.center.x == Catch::Approx(-4.));
    CHECK(data.center.y == Catch::Approx(1.));
    CHECK(data.majorP.x == Catch::Approx(0.).margin(1e-9));
    CHECK(data.majorP.y == Catch::Approx(8.));
    CHECK(data.ratio == Catch::Approx(3. / 8.));
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P passes through the four points", "[creation-ellipse]") {
    std::mt19937 rng(2966);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 500; ++n) {
        const RS_Vector center{(unit(rng) * 100.) - 50., (unit(rng) * 100.) - 50.};
        const double a = 5. + (unit(rng) * 45.);
        const double b = 5. + (unit(rng) * 45.);
        // spread round the ellipse, so that the four points have a unique ellipse with these axes
        const double start = unit(rng) * kTwoPi;
        std::vector<RS_Vector> given;
        for (int i = 0; i < 4; ++i) {
            const double t = start + (i * M_PI_2) + ((unit(rng) - 0.5) * 0.6);
            given.emplace_back(center.x + (a * std::cos(t)), center.y + (b * std::sin(t)));
        }
        RS_EllipseData data;
        CAPTURE(n, a, b);
        REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({given[0], given[1], given[2], given[3]}), data));
        REQUIRE(data.ratio <= 1.);
        REQUIRE(data.majorP.magnitude() == Catch::Approx(std::max(a, b)).epsilon(1e-6));
        REQUIRE(data.ratio == Catch::Approx(std::min(a, b) / std::max(a, b)).epsilon(1e-6));
        REQUIRE(data.center.x == Catch::Approx(center.x).epsilon(1e-6).margin(1e-6));
        REQUIRE(data.center.y == Catch::Approx(center.y).epsilon(1e-6).margin(1e-6));
        for (const RS_Vector& point : given) {
            REQUIRE(std::abs(residual(data, point)) < 1e-6);
        }
    }
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P finds an ellipse through the origin", "[creation-ellipse]") {
    // center (10, 0), semi-axes 10 and 5: through (0, 0), where an equation with 1 on its right has none
    RS_EllipseData data;
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({{0., 0.}, {20., 0.}, {10., 5.}, {10., -5.}}), data));
    CHECK(data.center.x == Catch::Approx(10.));
    CHECK(data.center.y == Catch::Approx(0.).margin(1e-9));
    CHECK(data.majorP.x == Catch::Approx(10.));
    CHECK(data.ratio == Catch::Approx(0.5));
    // a circle through the origin
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({{0., 0.}, {10., 10.}, {20., 0.}, {10., -10.}}), data));
    CHECK(data.center.x == Catch::Approx(10.));
    CHECK(data.majorP.magnitude() == Catch::Approx(10.));
    CHECK(data.ratio == Catch::Approx(1.));
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P does not depend on the distance from the origin", "[creation-ellipse]") {
    // semi-axes 10 and 6, four points spread round it, moved away from the origin
    for (const double distance : {0., 1e3, 1e5, 1e6, 1e7, 5e7, -3e7}) {
        std::vector<RS_Vector> given;
        for (const double t : {0.4, 1.9, 3.3, 5.2}) {
            given.emplace_back(distance + (10. * std::cos(t)), distance + (6. * std::sin(t)));
        }
        RS_EllipseData data;
        CAPTURE(distance);
        REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({given[0], given[1], given[2], given[3]}), data));
        // the points are only known to about 1e-16 of their distance from the origin: a thousand times that
        // is the tolerance, which the fit without the centroid and the spread misses from 1e6 on
        const double tolerance = 1e-12 * std::max(1., std::abs(distance));
        CHECK(std::abs(data.center.x - distance) < tolerance);
        CHECK(std::abs(data.center.y - distance) < tolerance);
        CHECK(std::abs(data.majorP.magnitude() - 10.) < tolerance);
        CHECK(std::abs(data.ratio - 0.6) < tolerance);
    }
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P finds a small ellipse and a large one", "[creation-ellipse]") {
    for (const double scale : {1e-6, 1e-3, 1., 1e3, 1e6, 1e9}) {
        std::vector<RS_Vector> given;
        for (const double t : {0.4, 1.9, 3.3, 5.2}) {
            given.emplace_back(scale * (20. + (10. * std::cos(t))), scale * (-5. + (6. * std::sin(t))));
        }
        RS_EllipseData data;
        CAPTURE(scale);
        REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({given[0], given[1], given[2], given[3]}), data));
        CHECK(data.center.x == Catch::Approx(20. * scale).epsilon(1e-9));
        CHECK(data.center.y == Catch::Approx(-5. * scale).epsilon(1e-9));
        CHECK(data.majorP.magnitude() == Catch::Approx(10. * scale).epsilon(1e-9));
        CHECK(data.ratio == Catch::Approx(0.6).epsilon(1e-9));
    }
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P finds an ellipse through the origin from points round it", "[creation-ellipse]") {
    // center (10, 0), semi-axes 10 and 5: through (0, 0), with the four points elsewhere on it
    RS_EllipseData data;
    std::vector<RS_Vector> given;
    for (const double t : {1., 2., 4., 5.}) {
        given.emplace_back(10. + (10. * std::cos(t)), 5. * std::sin(t));
    }
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({given[0], given[1], given[2], given[3]}), data));
    CHECK(data.center.x == Catch::Approx(10.));
    CHECK(data.center.y == Catch::Approx(0.).margin(1e-9));
    CHECK(data.majorP.x == Catch::Approx(10.));
    CHECK(data.ratio == Catch::Approx(0.5));
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P resets what it was given", "[creation-ellipse]") {
    RS_EllipseData data;
    data.reversed = true;
    data.setAngle1(1.);
    data.setAngle2(2.);
    REQUIRE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {2., 6.}, {-3., 3.}, {2., 0.}}), data));
    CHECK_FALSE(data.reversed);
    CHECK(data.angle1 == 0.);
    CHECK(data.angle2 == 0.);
    CHECK_FALSE(data.isArc);
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P gives no ellipse where there is none", "[creation-ellipse]") {
    RS_EllipseData data;
    // not four points
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {2., 6.}, {-3., 3.}}), data));
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {2., 6.}, {-3., 3.}, {2., 0.}, {6., 5.}}), data));
    // the same point four times
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {7., 3.}, {7., 3.}, {7., 3.}}), data));
    // on a line
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{0., 0.}, {1., 1.}, {2., 2.}, {3., 3.}}), data));
    // on the hyperbola x^2 - y^2 = 1
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{1., 0.}, {-1., 0.}, {std::sqrt(2.), 1.}, {std::sqrt(2.), -1.}}), data));
    // not numbers
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {2., 6.}, {-3., 3.}, {nan, 0.}}), data));
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{7., 3.}, {2., inf}, {-3., 3.}, {2., 0.}}), data));
}

TEST_CASE("LC_CreationEllipse::createEllipseFrom4P refuses the corners of a rectangle", "[creation-ellipse]") {
    // a family of ellipses with axes along x and y passes through them: no unique one, however big they are
    // and whether or not the corner coordinates are exact in binary
    RS_EllipseData data;
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{0., 0.}, {20., 0.}, {20., 6.}, {0., 6.}}), data));
    CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{0.1, 0.3}, {20000.6, 0.3}, {20000.6, 6001.0}, {0.1, 6001.0}}), data));
    std::mt19937 rng(2966);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 2000; ++n) {
        // of any size, at a distance from the origin of a few times the size (the points keep what tells the
        // rectangle from an ellipse only as long as their coordinates are not much bigger than the sides)
        const double size = std::pow(10., (unit(rng) * 12.) - 6.);
        const double x = (unit(rng) - 0.5) * 10. * size;
        const double y = (unit(rng) - 0.5) * 10. * size;
        const double w = size * (0.2 + unit(rng));
        const double h = size * (0.2 + unit(rng));
        CAPTURE(n, x, y, w, h);
        CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}), data));
    }
    // sides of 10 to 1e6 (drawing units) in a drawing around the origin
    for (int n = 0; n < 2000; ++n) {
        const double size = std::pow(10., 1. + (unit(rng) * 5.));
        const double x = (unit(rng) - 0.5) * 1e5;
        const double y = (unit(rng) - 0.5) * 1e5;
        const double w = size * (0.2 + unit(rng));
        const double h = size * (0.2 + unit(rng));
        CAPTURE(n, x, y, w, h);
        CHECK_FALSE(LC_CreationEllipse::createEllipseFrom4P(points({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}), data));
    }
}

TEST_CASE("Draw Ellipse (4 Points) draws the ellipse through the four clicks", "[creation-ellipse]") {
    ActionFixture<FourPointsProbe> f;
    FourPointsProbe& action = *f.m_action;
    action.init(0);
    // center (10, -5), semi-axes 20 along x and 12
    const RS_Vector clicks[] = {{30., -5.},
                                {10. + (20. * std::cos(1.3)), -5. + (12. * std::sin(1.3))},
                                {-10., -5.},
                                {10. + (20. * std::cos(4.4)), -5. + (12. * std::sin(4.4))}};
    for (const RS_Vector& point : clicks) {
        click(action, point);
    }
    const RS_Ellipse* ellipse = nullptr;
    for (const RS_Entity* entity : f.m_graphic) {
        if (const auto* candidate = dynamic_cast<const RS_Ellipse*>(entity)) {
            ellipse = candidate;
        }
    }
    REQUIRE(ellipse != nullptr);
    REQUIRE_FALSE(ellipse->isEllipticArc());
    const RS_EllipseData& data = ellipse->getData();
    CHECK(data.center.x == Catch::Approx(10.));
    CHECK(data.center.y == Catch::Approx(-5.));
    CHECK(data.majorP.magnitude() == Catch::Approx(20.));
    CHECK(data.ratio == Catch::Approx(0.6));
    for (const RS_Vector& point : clicks) {
        CHECK(std::abs(residual(data, point)) < 1e-9);
    }
}

TEST_CASE("Draw Ellipse (4 Points) draws nothing for four clicks on a line", "[creation-ellipse]") {
    ActionFixture<FourPointsProbe> f;
    FourPointsProbe& action = *f.m_action;
    action.init(0);
    for (const RS_Vector& point : {RS_Vector{0., 0.}, {10., 10.}, {20., 20.}, {30., 30.}}) {
        click(action, point);
    }
    for (const RS_Entity* entity : f.m_graphic) {
        CHECK(dynamic_cast<const RS_Ellipse*>(entity) == nullptr);
        CHECK(dynamic_cast<const RS_Circle*>(entity) == nullptr);
    }
}

TEST_CASE("Draw Ellipse (4 Points) takes a click on the origin", "[creation-ellipse]") {
    ActionFixture<FourPointsProbe> f;
    FourPointsProbe& action = *f.m_action;
    action.init(0);
    // center (10, 0), semi-axes 10 and 5, through the origin
    const RS_Vector clicks[] = {{0., 0.}, {20., 0.}, {10., 5.}, {10., -5.}};
    for (const RS_Vector& point : clicks) {
        click(action, point);
    }
    const RS_Ellipse* ellipse = nullptr;
    for (const RS_Entity* entity : f.m_graphic) {
        if (const auto* candidate = dynamic_cast<const RS_Ellipse*>(entity)) {
            ellipse = candidate;
        }
    }
    REQUIRE(ellipse != nullptr);
    CHECK(ellipse->getCenter().distanceTo(RS_Vector{10., 0.}) < 1e-9);
    CHECK(ellipse->getMajorRadius() == Catch::Approx(10.));
}

TEST_CASE("Draw Ellipse (4 Points) draws a circle when the fourth click repeats the third", "[creation-ellipse]") {
    ActionFixture<FourPointsProbe> f;
    FourPointsProbe& action = *f.m_action;
    action.init(0);
    click(action, {10., 0.});
    click(action, {0., 10.});
    click(action, {-10., 0.});
    click(action, {-10., 0.});
    const RS_Circle* circle = nullptr;
    for (const RS_Entity* entity : f.m_graphic) {
        if (const auto* candidate = dynamic_cast<const RS_Circle*>(entity)) {
            circle = candidate;
        }
    }
    REQUIRE(circle != nullptr);
    CHECK(circle->getCenter().distanceTo(RS_Vector{0., 0.}) < 1e-9);
    CHECK(circle->getRadius() == Catch::Approx(10.));
}
