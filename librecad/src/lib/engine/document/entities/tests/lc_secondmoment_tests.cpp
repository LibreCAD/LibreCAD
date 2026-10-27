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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
****************************************************************************/

// Tests for the LC_SecondMoment region-rotation helper, the companion to
// LC_FirstMoment::rotated. The legacy rotated() turns by -theta; new callers
// use regionRotated(), which turns the closed-region tensor by +theta so
// hatch moments mapped from a loop frame rotate correctly back to WCS.

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "lc_secondmoment.h"
#include "rs_line.h"
#include "rs_vector.h"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace {

LC_SecondMoment summedLineIntegrals(
    const RS_Vector pts[], int n, double theta) {
    LC_SecondMoment total;
    for (int i = 0; i < n; ++i) {
        RS_Vector a = pts[i];
        RS_Vector b = pts[(i + 1) % n];
        a.rotate(theta);
        b.rotate(theta);
        RS_Line line(a, b);
        total += line.secondMomentLineIntegral();
    }
    return total;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("LC_SecondMoment::regionRotated turns the tensor counter-clockwise",
          "[lc-hatchfix][moments]") {
    // Rectangle [0,3] x [0,1] about the origin: A = 3,
    // ixx = integral x^2 over [0,3]x[0,1] = (3^3 / 3) * 1 = 9,
    // iyy = integral y^2 over [0,3]x[0,1] = 3 * (1 / 3) = 1,
    // ixy = integral x*y = (9/2) * (1/2) = 9/4.
    const LC_SecondMoment M{9.0, 1.0, 2.25};
    SECTION("30 degrees matches the closed form") {
        const double theta = M_PI / 6.0;
        const LC_SecondMoment r = M.regionRotated(theta);
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        const double cc = c * c, ss = s * s, cs = c * s;
        const double exIxx = cc * 9.0 - 2.0 * cs * 2.25 + ss * 1.0;
        const double exIyy = ss * 9.0 + 2.0 * cs * 2.25 + cc * 1.0;
        const double exIxy = cs * (9.0 - 1.0) + (cc - ss) * 2.25;
        REQUIRE_THAT(r.ixx, WithinAbs(exIxx, 1e-14));
        REQUIRE_THAT(r.iyy, WithinAbs(exIyy, 1e-14));
        REQUIRE_THAT(r.ixy, WithinAbs(exIxy, 1e-14));
    }
    SECTION("round-trip is the identity") {
        for (double theta : {0.3, 2.0, -1.1}) {
            const LC_SecondMoment r = M.regionRotated(theta).regionRotated(-theta);
            REQUIRE_THAT(r.ixx, WithinAbs(M.ixx, 1e-13));
            REQUIRE_THAT(r.iyy, WithinAbs(M.iyy, 1e-13));
            REQUIRE_THAT(r.ixy, WithinAbs(M.ixy, 1e-13));
        }
    }
    SECTION("equals the summed line integrals of the rotated polygon") {
        // Rectangle [0,3] x [0,1] rotated about the origin.
        const RS_Vector pts[] = {
            RS_Vector(0.0, 0.0), RS_Vector(3.0, 0.0),
            RS_Vector(3.0, 1.0), RS_Vector(0.0, 1.0)};
        for (double theta : {M_PI / 6.0, 0.6, -1.2}) {
            const LC_SecondMoment r = M.regionRotated(theta);
            const LC_SecondMoment direct = summedLineIntegrals(pts, 4, theta);
            REQUIRE_THAT(r.ixx, WithinRel(direct.ixx, 1e-13));
            REQUIRE_THAT(r.iyy, WithinRel(direct.iyy, 1e-13));
            REQUIRE_THAT(r.ixy, WithinRel(direct.ixy, 1e-13));
        }
    }
}
