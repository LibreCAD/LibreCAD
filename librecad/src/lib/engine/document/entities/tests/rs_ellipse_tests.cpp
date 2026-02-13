/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD (librecad.org)
** Copyright (C) 2025 Dongxu Li github.com/dxli
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
#include <cmath>  // For std::abs, M_PI
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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
    double dist;
    RS_Vector nearest = ellipse.getNearestEndpoint(queryPoint, &dist);
    REQUIRE(nearest == RS_Vector(5.0, 0.0));  // Endpoint1
    REQUIRE(std::abs(dist - 5.0) < EPS);

    queryPoint = RS_Vector(-10.0, 0.0);  // Near negative x endpoint
    nearest = ellipse.getNearestEndpoint(queryPoint, &dist);
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



using namespace Catch;

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
}
