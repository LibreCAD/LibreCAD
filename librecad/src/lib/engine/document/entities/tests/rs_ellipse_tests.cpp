/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD (librecad.org)
** Copyright (C) 2025 Dongxu Li (github.com/dxli)
**
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <algorithm>
#include <cmath>  // For std::abs, M_PI
#include <limits>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "lc_splinepoints.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_vector.h"
#include "rs_math.h"  // For M_PI if needed

const double EPS = 1e-6;  // Tolerance for floating-point comparisons

TEST_CASE("RS_Ellipse::Constructor and Getters") {
    RS_Vector center(0.0, 0.0);
    RS_Vector majorP(5.0, 0.0);  // Major axis length 5 along x
    double ratio = 0.5;  // Minor/major ratio
    double angle1 = 0.0;
    double angle2 = 2 * M_PI;  // Full ellipse
    bool reversed = false;

    RS_Ellipse ellipse(nullptr, {center, majorP, ratio, angle1, angle2, reversed});

    REQUIRE(ellipse.getCenter() == center);
    REQUIRE(ellipse.getMajorP() == majorP);
    REQUIRE(std::abs(ellipse.getRatio() - ratio) < EPS);
    REQUIRE(std::abs(ellipse.getAngle1() - angle1) < EPS);
    REQUIRE(std::abs(ellipse.getAngle2() - angle2) < EPS);
    REQUIRE(ellipse.isReversed() == reversed);
    REQUIRE(std::abs(ellipse.getMajorRadius() - 5.0) < EPS);
    REQUIRE(std::abs(ellipse.getMinorRadius() - 2.5) < EPS);  // 5 * 0.5
    REQUIRE(std::abs(ellipse.getAngle() - 0.0) < EPS);  // Rotation angle
    REQUIRE(std::abs(ellipse.getAngleLength() - 2 * M_PI) < EPS);
}

TEST_CASE("RS_Ellipse::isValid") {
    RS_Vector center(0.0, 0.0);
    RS_Vector majorP(0.0, 0.0);  // Invalid major
    double ratio = 0.5;
    double angle1 = 0.0;
    double angle2 = 2 * M_PI;

    RS_Ellipse invalidEllipse(nullptr, {center, majorP, ratio, angle1, angle2, false});

    majorP = RS_Vector(5.0, 0.0);
    RS_Ellipse validEllipse(nullptr, {center, majorP, ratio, angle1, angle2, false});
}

TEST_CASE("RS_Ellipse::setReversed and isReversed") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, 2*M_PI, false});
    REQUIRE(ellipse.isReversed() == false);

    ellipse.setReversed(true);
    REQUIRE(ellipse.isReversed() == true);
}

TEST_CASE("RS_Ellipse::isPointOnEntity") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, 2*M_PI, false});

    // Point on ellipse (parametric: angle 0)
    RS_Vector onPoint(5.0, 0.0);
    REQUIRE(ellipse.isPointOnEntity(onPoint, EPS) == true);

    // Point inside ellipse
    RS_Vector insidePoint(1.0, 0.0);
    REQUIRE(ellipse.isPointOnEntity(insidePoint, EPS) == false);

    // Point outside
    RS_Vector outsidePoint(6.0, 0.0);
    REQUIRE(ellipse.isPointOnEntity(outsidePoint, EPS) == false);

    // Point on minor axis
    RS_Vector minorPoint(0.0, 2.5);
    REQUIRE(ellipse.isPointOnEntity(minorPoint, EPS) == true);
}

TEST_CASE("RS_Ellipse::getNearestEndpoint") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, M_PI, false});  // Half ellipse from 0 to 180 deg

    RS_Vector queryPoint(10.0, 0.0);  // Near positive x endpoint
    double dist = 0.0;
    RS_Vector nearest = ellipse.getNearestEndpoint(queryPoint, nullptr, &dist);
    REQUIRE(nearest == RS_Vector(5.0, 0.0));  // Endpoint1
    REQUIRE(std::abs(dist - 5.0) < EPS);

    queryPoint = RS_Vector(-10.0, 0.0);  // Near negative x endpoint
    nearest = ellipse.getNearestEndpoint(queryPoint, nullptr, &dist);
    REQUIRE(nearest.distanceTo(RS_Vector(-5.0, 0.0)) < EPS);  // Endpoint2 (for half ellipse)
    REQUIRE(std::abs(dist - 5.0) < EPS);
}

TEST_CASE("RS_Ellipse::getNearestPointOnEntity") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, 2*M_PI, false});

    RS_Vector queryPoint(6.0, 0.0);  // Outside on major axis
    RS_Vector nearest = ellipse.getNearestPointOnEntity(queryPoint);
    REQUIRE(nearest == RS_Vector(5.0, 0.0));
    REQUIRE(nearest.valid == true);

    queryPoint = RS_Vector(0.0, 3.0);  // Outside on minor axis
    nearest = ellipse.getNearestPointOnEntity(queryPoint);
    REQUIRE(nearest == RS_Vector(0.0, 2.5));
    REQUIRE(nearest.valid == true);
}

TEST_CASE("RS_Ellipse::getNearestPointOnEntity for points on the axes", "[rs_ellipse][nearest]") {
    // For a point on an axis the quartic has double roots that the solver can miss: at the major vertex
    // it found no real root, and inside the ellipse on the major axis it missed the nearest points off
    // the axis. The sine solved from the derivative also put nearest points of the major axis off the
    // ellipse.
    using Catch::Matchers::WithinAbs;
    const auto sampledDistance = [](const RS_Ellipse& e, const RS_Vector& p) {
        double best = std::numeric_limits<double>::infinity();
        constexpr int samples = 40000;
        for (int i = 0; i < samples; ++i) {
            best = std::min(best, e.getEllipsePoint(2. * M_PI * i / samples).distanceTo(p));
        }
        return best;
    };
    for (const double ratio : {0.5, 0.2, 0.9, 2.0}) {
        for (const double rotation : {0., 0.7, M_PI / 2.}) {
            const RS_Vector center(3., -4.);
            const RS_Vector major = RS_Vector::polar(500., rotation);
            const RS_Vector minor = RS_Vector::polar(500. * ratio, rotation + M_PI / 2.);
            RS_Ellipse e(nullptr, {center, major, ratio, 0., 0., false});
            INFO("ratio " << ratio << ", rotation " << rotation);

            for (const RS_Vector& vertex : {center + major, center - major, center + minor, center - minor}) {
                INFO("vertex " << vertex.x << ", " << vertex.y);
                double dist = -1.;
                const RS_Vector nearest = e.getNearestPointOnEntity(vertex, true, &dist);
                CHECK_THAT(dist, WithinAbs(0., 1e-6));
                CHECK_THAT(nearest.distanceTo(vertex), WithinAbs(0., 1e-6));
                CHECK_THAT(e.getDistanceToPoint(vertex), WithinAbs(0., 1e-6));
            }

            for (const RS_Vector& axis : {major, minor}) {
                for (const double along : {0.2, -0.6, 1.3}) {
                    const RS_Vector p = center + axis * along;
                    INFO("point " << p.x << ", " << p.y);
                    double dist = -1.;
                    const RS_Vector nearest = e.getNearestPointOnEntity(p, true, &dist);
                    CHECK(e.isPointOnEntity(nearest, 1e-6));
                    CHECK_THAT(nearest.distanceTo(p), WithinAbs(dist, 1e-6));
                    CHECK_THAT(dist, WithinAbs(sampledDistance(e, p), 0.1));
                }
            }
        }
    }
}

TEST_CASE("RS_Ellipse::getNearestPointOnEntity near the vertices", "[rs_ellipse][nearest]") {
    // A point at h along the normal at an elliptic angle is nearest to the point at that angle, for |h| within
    // the radius of curvature. Near a vertex the angle from a root of the quartic carries the root's error
    // divided by the small sine, which missed the distance by up to 1e-5.
    using Catch::Matchers::WithinAbs;
    for (const double ratio : {0.2, 0.5, 2.0}) {
        for (const double rotation : {0., 0.7}) {
            const RS_Vector center(3., -4.);
            const double a = 500.;
            const double b = a * ratio;
            RS_Ellipse e(nullptr, {center, RS_Vector::polar(a, rotation), ratio, 0., 0., false});
            for (const double vertex : {0., M_PI / 2., M_PI, 1.5 * M_PI}) {
                for (const double delta : {1e-7, -1e-5, 1e-3}) {
                    for (const double h : {1e-7, -1e-5, 1e-3, -0.5, 3.}) {
                        const double t = vertex + delta;
                        const RS_Vector normal(b * std::cos(t), a * std::sin(t));
                        RS_Vector local = RS_Vector(a * std::cos(t), b * std::sin(t)) + normal * (h / normal.magnitude());
                        const RS_Vector p = center + local.rotate(rotation);
                        INFO("ratio " << ratio << ", rotation " << rotation << ", angle " << t << ", offset " << h);
                        double dist = -1.;
                        const RS_Vector nearest = e.getNearestPointOnEntity(p, true, &dist);
                        CHECK_THAT(dist, WithinAbs(std::abs(h), 1e-9));
                        CHECK_THAT(nearest.distanceTo(p), WithinAbs(std::abs(h), 1e-9));
                    }
                }
            }
        }
    }
}

TEST_CASE("RS_Ellipse::scale and setReversed keep borders and length current", "[rs_ellipse][borders]") {
    using Catch::Matchers::WithinAbs;
    const auto expectFresh = [](const RS_Ellipse& e) {
        RS_Ellipse fresh(e);
        fresh.calculateBorders();
        CHECK_THAT(e.getMin().distanceTo(fresh.getMin()), WithinAbs(0., 1e-9));
        CHECK_THAT(e.getMax().distanceTo(fresh.getMax()), WithinAbs(0., 1e-9));
        CHECK_THAT(e.getLength(), WithinAbs(fresh.getLength(), 1e-9));
    };
    for (const bool arc : {false, true}) {
        for (const RS_Vector& factor : {RS_Vector(2., 2.), RS_Vector(0.5, 0.5), RS_Vector(2., 1.), RS_Vector(-1., 1.), RS_Vector(1., -3.)}) {
            RS_Ellipse e(nullptr, {RS_Vector(20., 10.), RS_Vector::polar(40., 0.5), 0.4, arc ? 0.3 : 0., arc ? 2.5 : 0., false});
            e.calculateBorders();
            e.scale(RS_Vector(3., 4.), factor);
            INFO("elliptic arc " << arc << ", factor " << factor.x << " by " << factor.y);
            expectFresh(e);
        }
    }

    // reversed, the arc from 0 to 90 degrees runs through the other three quadrants
    RS_Ellipse e(nullptr, {RS_Vector(0., 0.), RS_Vector(100., 0.), 0.5, 0., M_PI / 2., false});
    e.calculateBorders();
    e.setReversed(true);
    expectFresh(e);
    CHECK_THAT(e.getMin().x, WithinAbs(-100., 1e-9));
}



using namespace Catch;

TEST_CASE("RS_Ellipse::revertDirection keeps the shape of an elliptic arc", "[rs_ellipse]") {
    // "Is Reversed" in the ellipse dialog reverts the direction of an elliptic arc. setReversed() alone
    // turns it into the rest of the ellipse.
    using Catch::Matchers::WithinAbs;
    for (const bool reversed : {false, true}) {
        INFO("reversed " << reversed);
        RS_Ellipse e(nullptr, {RS_Vector(3., -4.), RS_Vector::polar(100., 0.4), 0.5, 0.2, 1.7, reversed});
        const RS_Vector start = e.getStartpoint();
        const RS_Vector end = e.getEndpoint();
        const RS_Vector min = e.getMin();
        const RS_Vector max = e.getMax();
        const double length = e.getLength();

        e.revertDirection();

        CHECK(e.isReversed() != reversed);
        CHECK(e.getStartpoint().distanceTo(end) < 1e-9);
        CHECK(e.getEndpoint().distanceTo(start) < 1e-9);
        CHECK(e.getMin().distanceTo(min) < 1e-9);
        CHECK(e.getMax().distanceTo(max) < 1e-9);
        CHECK_THAT(e.getLength(), WithinAbs(length, 1e-6));
    }
}

TEST_CASE("RS_Ellipse::areaLineIntegral()", "[rs_ellipse]") {
    double tol = 1e-8;

    SECTION("Full ellipse, center (0,0), ratio 1.0, rotation 0") {
        RS_Vector center(0.0, 0.0);
        RS_Vector majorP(5.0, 0.0);
        double ratio = 1.0;
        double angle1 = 0.0;
        double angle2 = 0.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = M_PI * 5.0 * 5.0;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Full ellipse, center (3,4), ratio 0.5, rotation pi/4") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 4.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 0.0;
        double angle2 = 0.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = M_PI * 2.0 * 1.0;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 0.5, angles 0 to pi/2, rotation pi/4") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 4.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 0.0;
        double angle2 = M_PI / 2.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -1.8005240167647454;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 0.5, angles 0 to pi/2, rotation pi/4") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 4.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 0.0;
        double angle2 = M_PI / 2.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -8.083709323944333;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 0.5, angles pi/4 to 3pi/4, rotation pi/4") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 4.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = M_PI / 4.0;
        double angle2 = 3.0 * M_PI / 4.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -4.4292036732051026;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 2.0, angles 0 to pi/2, rotation pi/4") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 4.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 2.0;
        double angle1 = 0.0;
        double angle2 = M_PI / 2.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = 5.525825994298873;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 2.0, angles pi/4 to 3pi/4, rotation pi/6") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 6.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 2.0;
        double angle1 = M_PI / 4.0;
        double angle2 = 3.0 * M_PI / 4.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -25.092196608658043;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 0.75, angles 0 to pi, rotation 0, center (0,0)") {
        RS_Vector center(0.0, 0.0);
        double a = 2.0;
        double alpha = 0.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.75;
        double angle1 = 0.0;
        double angle2 = M_PI;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = 4.71238898038469;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 0.75, angles 0 to pi, rotation 0, center (0,0)") {
        RS_Vector center(0.0, 0.0);
        double a = 2.0;
        double alpha = 0.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.75;
        double angle1 = 0.0;
        double angle2 = M_PI;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -4.71238898038469;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 1.5, angles pi/2 to 3pi/2, rotation pi/3, center (3,4)") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 3.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 1.5;
        double angle1 = M_PI / 2.0;
        double angle2 = 3.0 * M_PI / 2.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = 0.4247779607693788;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 1.5, angles pi/2 to 3pi/2, rotation pi/3, center (3,4)") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 3.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 1.5;
        double angle1 = M_PI / 2.0;
        double angle2 = 3.0 * M_PI / 2.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -18.42477796076938;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 0.5, angles 7pi/4 to pi/4 (crossing 0), rotation pi/2, center (0,0)") {
        RS_Vector center(0.0, 0.0);
        double a = 2.0;
        double alpha = M_PI / 2.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 7.0 * M_PI / 4.0;
        double angle2 = M_PI / 4.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = 0.5707963267948966;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 0.5, angles 7pi/4 to pi/4 (crossing 0), rotation pi/2, center (0,0)") {
        RS_Vector center(0.0, 0.0);
        double a = 2.0;
        double alpha = M_PI / 2.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 7.0 * M_PI / 4.0;
        double angle2 = M_PI / 4.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -5.71238898038469;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, ratio 1.0, angles 0 to 3pi/2, rotation 0, center (3,4)") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = 0.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 1.0;
        double angle1 = 0.0;
        double angle2 = 3.0 * M_PI / 2.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = 3.4247779607693793;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 1.0, angles 0 to 3pi/2, rotation 0, center (3,4)") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = 0.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 1.0;
        double angle1 = 0.0;
        double angle2 = 3.0 * M_PI / 2.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);
        double result = ellipse.areaLineIntegral();
        double expected = -9.14159265358979312;
        REQUIRE_THAT(result, Matchers::WithinAbs(expected, tol));
    }
}

TEST_CASE("Elliptic arc segment area", "[rs_ellipse]") {
    double tol = 1e-8;

    SECTION("Arc non-reversed, ratio 0.5, angles 0 to pi/2, rotation 0, center 0") {
        RS_Vector center(0.0, 0.0);
        double a = 2.0;
        double alpha = 0.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 0.5;
        double angle1 = 0.0;
        double angle2 = M_PI / 2.0;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);

        double theta = ellipse.getAngleLength();
        double sector = 0.5 * a * (a * ratio) * theta;  // 0.5 a b theta
        RS_Vector p1 = ellipse.getStartpoint();  // (2,0)
        RS_Vector p2 = ellipse.getEndpoint();    // (0,1)
        double triangle = 0.5 * (p1.x * p2.y - p2.x * p1.y);  // Signed area
        double segment = sector - triangle;
        double expected = M_PI / 2.0 - 1.0;  // ~0.5708
        REQUIRE_THAT(segment, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc reversed, ratio 2.5, angles pi/6 to 3pi/7, rotation pi/5, center (3,4)") {
        RS_Vector center(3.0, 4.0);
        double a = 2.0;
        double alpha = M_PI / 5.0;
        RS_Vector majorP(a * cos(alpha), a * sin(alpha));
        double ratio = 2.5;
        double angle1 = M_PI / 6.0;
        double angle2 = 3.0 * M_PI / 7.0;
        bool reversed = true;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);

        double segment = ellipse.areaLineIntegral();
        double expected = -28.9718193637963;
        REQUIRE_THAT(segment, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, center (3,4), majorP=(2,1), ratio 0.75, angles pi/6 to 0.8pi, rotation from majorP; line connecting begin/end") {
        RS_Vector center(3.0, 4.0);
        RS_Vector majorP(2.0, 1.0);
        double ratio = 0.75;
        double angle1 = M_PI / 6.0;
        double angle2 = 0.8 * M_PI;
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);

        RS_Line line{nullptr, RS_LineData{ellipse.getEndpoint(), ellipse.getStartpoint()}};

        double area = ellipse.areaLineIntegral() + line.areaLineIntegral();
        double expected = 2.0177435430580033;
        REQUIRE_THAT(area, Matchers::WithinAbs(expected, tol));
    }

    SECTION("Arc non-reversed, center (81.36434599, 21.83868864), majorP=(-111.51516953, 0), minor radius=4.911886562, angles in degrees:250.5718813 to ,441.6127417, reversed=false; line connecting begin/end") {
        RS_Vector center(81.36434599, 21.83868864);
        double a = 11.51516953;
        double b = 4.911886562;
        RS_Vector majorP(-a, 0);
        double ratio = b/a;
        double angle1 = RS_Math::deg2rad(250.5718813);
        double angle2 = RS_Math::deg2rad(441.6127417);
        bool reversed = false;
        RS_EllipseData data{center, majorP, ratio, angle1, angle2, reversed};
        RS_Ellipse ellipse(nullptr, data);

        RS_Line line{nullptr, RS_LineData{ellipse.getEndpoint(), ellipse.getStartpoint()}};

        double area = ellipse.areaLineIntegral() + line.areaLineIntegral();
        double expected = 99.7117795862;
        REQUIRE_THAT(area, Matchers::WithinAbs(expected, 1e-7));
    }
}

namespace {

// Helper: signed distance from `p` to the ellipse, positive outside the
// ellipse and negative inside. Uses brute-force sampling because
// RS_Ellipse::getNearestPointOnEntity occasionally returns a far-side point
// for query points strictly inside the ellipse.
double signedDistanceToEllipse(const RS_Ellipse& ellipse, const RS_Vector& p) {
    const double a = ellipse.getMajorRadius();
    const double b = ellipse.getMinorRadius();
    const double majorAngle = ellipse.getMajorP().angle();

    constexpr int kSamples = 4000;
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < kSamples; ++i) {
        const double t = (2.0 * M_PI * i) / kSamples;
        RS_Vector q(a * std::cos(t), b * std::sin(t));
        q.rotate(majorAngle);
        q += ellipse.getCenter();
        const double d = q.distanceTo(p);
        if (d < best) best = d;
    }

    // Sign from the implicit form in the local frame.
    RS_Vector local = p - ellipse.getCenter();
    local.rotate(-majorAngle);
    const double quad = (local.x / a) * (local.x / a) + (local.y / b) * (local.y / b);
    return (quad >= 1.0) ? best : -best;
}

} // namespace

TEST_CASE("RS_Ellipse::createOffset full ellipse outward") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, 0.0, false});
    const double d = 1.0;
    const RS_Vector outsidePoint(20.0, 0.0);

    auto offsets = ellipse.createOffset(outsidePoint, d);
    REQUIRE(offsets.size() == 1);
    auto* sp = dynamic_cast<LC_SplinePoints*>(offsets.front());
    REQUIRE(sp != nullptr);
    REQUIRE(sp->isClosed());

    const auto& pts = sp->getPoints();
    REQUIRE(pts.size() >= 16);

    // Each spline point should sit at the requested distance from the source
    // ellipse, on the outside. Allow looser tolerance than the planning
    // 0.1% target because the chord-error budget bites at the major-axis
    // tips (high curvature region) for eccentric ellipses.
    for (const auto& q : pts) {
        const double s = signedDistanceToEllipse(ellipse, q);
        REQUIRE_THAT(s, Catch::Matchers::WithinAbs(d, 5.0e-2));
    }

    for (auto* o : offsets) delete o;
}

TEST_CASE("RS_Ellipse::createOffset full ellipse inward") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, 0.0, false});
    const double d = 0.5;  // safely below cusp limit b^2/a = (2)^2/5 = 0.8
    const RS_Vector insidePoint(0.0, 0.0);

    auto offsets = ellipse.createOffset(insidePoint, d);
    REQUIRE(offsets.size() == 1);
    auto* sp = dynamic_cast<LC_SplinePoints*>(offsets.front());
    REQUIRE(sp != nullptr);

    for (const auto& q : sp->getPoints()) {
        const double s = signedDistanceToEllipse(ellipse, q);
        REQUIRE_THAT(s, Catch::Matchers::WithinAbs(-d, 5.0e-2));
    }

    for (auto* o : offsets) delete o;
}

TEST_CASE("RS_Ellipse::createOffset rejects cusp-exceeding inward offset") {
    // a=5, b=2 → cusp limit b^2/a = 0.8. Asking for d=1.0 inward must fail.
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, 0.0, false});
    const RS_Vector insidePoint(0.0, 0.0);

    auto offsets = ellipse.createOffset(insidePoint, 1.0);
    REQUIRE(offsets.empty());
}

TEST_CASE("RS_Ellipse::createOffset elliptic arc, half ellipse, open spline") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, M_PI, false});
    const double d = 0.3;
    const RS_Vector outsidePoint(0.0, 20.0);

    auto offsets = ellipse.createOffset(outsidePoint, d);
    REQUIRE(offsets.size() == 1);
    auto* sp = dynamic_cast<LC_SplinePoints*>(offsets.front());
    REQUIRE(sp != nullptr);
    REQUIRE(sp->isClosed() == false);

    const auto& pts = sp->getPoints();
    REQUIRE(pts.size() >= 8);

    // Endpoints should sit at the exact arc start/end displaced by d along
    // the outward normal there. At angle 0: ellipse point = (5, 0); tangent
    // (0, b·1) → outward normal +x. So offset endpoint ≈ (5+d, 0).
    REQUIRE_THAT(pts.front().x, Catch::Matchers::WithinAbs(5.0 + d, 1.0e-9));
    REQUIRE_THAT(pts.front().y, Catch::Matchers::WithinAbs(0.0, 1.0e-9));
    // At angle π: ellipse point = (-5, 0); tangent (0, -b) → outward normal -x.
    REQUIRE_THAT(pts.back().x, Catch::Matchers::WithinAbs(-5.0 - d, 1.0e-9));
    REQUIRE_THAT(pts.back().y, Catch::Matchers::WithinAbs(0.0, 1.0e-9));

    for (auto* o : offsets) delete o;
}

TEST_CASE("RS_Ellipse::createOffset reversed arc preserves geometry") {
    RS_Ellipse forward(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, M_PI, false});
    RS_Ellipse reversed(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, M_PI, 0.0, true});
    const double d = 0.3;
    const RS_Vector outsidePoint(0.0, 20.0);

    auto fwd = forward.createOffset(outsidePoint, d);
    auto rev = reversed.createOffset(outsidePoint, d);
    REQUIRE(fwd.size() == 1);
    REQUIRE(rev.size() == 1);
    auto* spF = dynamic_cast<LC_SplinePoints*>(fwd.front());
    auto* spR = dynamic_cast<LC_SplinePoints*>(rev.front());
    REQUIRE(spF != nullptr);
    REQUIRE(spR != nullptr);

    // Reversed arc samples the same geometric curve in reverse order: front
    // and back endpoints should be swapped (modulo any small step-discretization
    // differences in the interior).
    REQUIRE_THAT(spF->getPoints().front().x,
                 Catch::Matchers::WithinAbs(spR->getPoints().back().x, 1.0e-9));
    REQUIRE_THAT(spF->getPoints().back().x,
                 Catch::Matchers::WithinAbs(spR->getPoints().front().x, 1.0e-9));

    for (auto* o : fwd) delete o;
    for (auto* o : rev) delete o;
}

TEST_CASE("RS_Ellipse::createOffset rejects degenerate inputs") {
    // Zero distance.
    {
        RS_Ellipse e(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.4, 0.0, 0.0, false});
        REQUIRE(e.createOffset(RS_Vector(20,0), 0.0).empty());
    }
    // Zero major radius.
    {
        RS_Ellipse e(nullptr, {RS_Vector(0,0), RS_Vector(0,0), 0.4, 0.0, 0.0, false});
        REQUIRE(e.createOffset(RS_Vector(20,0), 1.0).empty());
    }
}
