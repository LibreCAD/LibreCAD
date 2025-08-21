

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

TEST_CASE("RS_Ellipse::areaLineIntegral") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, 2*M_PI, false});  // Full ellipse
    REQUIRE(std::abs(ellipse.areaLineIntegral() - (M_PI * 5.0 * 2.5)) < EPS);  // Area = pi * a * b

    RS_Ellipse halfEllipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, M_PI, false});  // Half ellipse
    REQUIRE(std::abs(halfEllipse.areaLineIntegral() - (M_PI * 5.0 * 2.5 / 2)) < EPS);
}

TEST_CASE("RS_Ellipse::switchMajorMinor") {
    RS_Ellipse ellipse(nullptr, {RS_Vector(0,0), RS_Vector(5,0), 0.5, 0, 2*M_PI, false});
    REQUIRE(ellipse.switchMajorMinor() == true);
    REQUIRE(ellipse.getMajorP() == RS_Vector(0.0, 2.5));  // Switched to vertical major
    REQUIRE(std::abs(ellipse.getRatio() - 2.0) < EPS);  // Inverse ratio 1/0.5 = 2
    REQUIRE(ellipse.switchMajorMinor());  // Switch back
    REQUIRE(ellipse.getMajorP().distanceTo(RS_Vector(-5.0, 0.0)) < EPS);
    REQUIRE(std::abs(ellipse.getRatio() - 0.5) < EPS);

}
