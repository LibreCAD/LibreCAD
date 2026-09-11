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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>

#include "lc_arrow_headopen.h"

namespace {
    // A copy is built from the position and the angle, so it matches the arrow only while its transforms
    // keep the vertices, the position, the angle and the point the stem is drawn to consistent.
    void requireMatchesCopy(const LC_DimArrow& arrow) {
        const std::unique_ptr<RS_Entity> copy{arrow.clone()};
        const auto& copied = static_cast<const LC_DimArrow&>(*copy);
        CHECK(copied.getMin().distanceTo(arrow.getMin()) < 1e-9);
        CHECK(copied.getMax().distanceTo(arrow.getMax()) < 1e-9);
        CHECK(copied.getDimLinePoint().distanceTo(arrow.getDimLinePoint()) < 1e-9);
    }
}

TEST_CASE("LC_DimArrow transforms keep the arrow consistent", "[dimension][arrow]") {
    const double ownAngle = M_PI / 12.;

    SECTION("rotate turns the arrow about the given center") {
        LC_ArrowHeadOpen arrow(nullptr, RS_Vector(10., 0.), 0., 2., ownAngle);
        arrow.rotate(RS_Vector(10., 10.), M_PI / 2.);
        // (10, 0) turns to (20, 10) about (10, 10); the position used to turn about the origin, to (0, 10)
        CHECK(arrow.getPosition().distanceTo(RS_Vector(20., 10.)) < 1e-9);
        CHECK(arrow.getAngle() == Catch::Approx(M_PI / 2.));
        requireMatchesCopy(arrow);
    }

    SECTION("move") {
        LC_ArrowHeadOpen arrow(nullptr, RS_Vector(10., 0.), 0.3, 2., ownAngle);
        arrow.move(RS_Vector(-4., 7.));
        CHECK(arrow.getPosition().distanceTo(RS_Vector(6., 7.)) < 1e-9);
        requireMatchesCopy(arrow);
    }

    SECTION("mirror") {
        LC_ArrowHeadOpen arrow(nullptr, RS_Vector(10., 0.), 0.3, 2., ownAngle);
        // across the line y = x + 5
        arrow.mirror(RS_Vector(0., 5.), RS_Vector(1., 6.));
        CHECK(arrow.getPosition().distanceTo(RS_Vector(-5., 15.)) < 1e-9);
        CHECK(arrow.getAngle() == Catch::Approx(M_PI / 2. - 0.3));
        requireMatchesCopy(arrow);
    }
}
