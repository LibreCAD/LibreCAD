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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

#include "rs_circle.h"

TEST_CASE("RS_Circle::scale keeps borders and length current", "[rs_circle][borders]") {
    // Scaling used to scale the old borders and keep the old length: the length stayed at the old
    // circumference, and a negative factor turned the borders inside out.
    using Catch::Matchers::WithinAbs;
    struct Case {
        RS_Vector factor;
        RS_Vector center;
        double radius = 0.;
    };
    // a circle at (10, 20) with radius 5, scaled about (1, 2); the radius follows the x factor
    for (const Case& expected : std::vector<Case>{{RS_Vector(2., 2.), RS_Vector(19., 38.), 10.},
                                                  {RS_Vector(0.5, 0.5), RS_Vector(5.5, 11.), 2.5},
                                                  {RS_Vector(-1., 1.), RS_Vector(-8., 20.), 5.},
                                                  {RS_Vector(2., 1.), RS_Vector(19., 20.), 10.}}) {
        INFO("factor " << expected.factor.x << ", " << expected.factor.y);
        RS_Circle circle(nullptr, RS_CircleData(RS_Vector(10., 20.), 5.));
        circle.scale(RS_Vector(1., 2.), expected.factor);

        REQUIRE(circle.getCenter().distanceTo(expected.center) < 1e-9);
        REQUIRE_THAT(circle.getRadius(), WithinAbs(expected.radius, 1e-9));
        const RS_Vector r(expected.radius, expected.radius);
        CHECK(circle.getMin().distanceTo(expected.center - r) < 1e-9);
        CHECK(circle.getMax().distanceTo(expected.center + r) < 1e-9);
        CHECK_THAT(circle.getLength(), WithinAbs(2. * M_PI * expected.radius, 1e-9));
    }
}
