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
//
// Also tests the LC_AreaMoments bundle (area + first + second), the
// LC_MomentQuadrature constants and signedSweep range reducer, and the
// moments of RS_Line/RS_Circle/RS_Arc/RS_Ellipse through the shared
// momentLineIntegrals(origin) interface. The arc/ellipse open-arc oracles
// are TDD cases: before this change, firstMomentLineIntegral and
// secondMomentLineIntegral integrated in the centred local frame and then
// shifted with the parallel-axis theorem (which is wrong for open arcs).

#include <array>
#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "lc_momentquadrature.h"
#include "lc_secondmoment.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
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

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("LC_AreaMoments region helpers match direct integration",
          "[lc-hatchfix][moments]") {
    // Rectangle [0, 3] x [0, 1] about the origin.
    const RS_Vector corners[4] = {
        RS_Vector(0.0, 0.0), RS_Vector(3.0, 0.0),
        RS_Vector(3.0, 1.0), RS_Vector(0.0, 1.0)};
    auto directSum = [&corners](const RS_Vector& o) {
        LC_AreaMoments total;
        for (int i = 0; i < 4; ++i) {
            total += LC_MomentQuadrature::segment(
                corners[i], corners[(i + 1) % 4], o);
        }
        return total;
    };
    const LC_AreaMoments base = directSum(RS_Vector{0.0, 0.0});
    REQUIRE_THAT(base.area, WithinAbs(3.0, 1e-13));
    REQUIRE_THAT(base.first.mx, WithinAbs(4.5, 1e-13));
    REQUIRE_THAT(base.first.my, WithinAbs(1.5, 1e-13));
    REQUIRE_THAT(base.second.ixx, WithinAbs(9.0, 1e-13));
    REQUIRE_THAT(base.second.iyy, WithinAbs(1.0, 1e-13));
    REQUIRE_THAT(base.second.ixy, WithinAbs(2.25, 1e-13));

    SECTION("regionTranslated matches a translated polygon") {
        const double h = 2.0, k = -1.0;
        const LC_AreaMoments moved = base.regionTranslated(h, k);
        // Direct: the rectangle [h, h+3] x [k, k+1] integrated about (0, 0).
        const RS_Vector shifted[4] = {
            corners[0] + RS_Vector(h, k), corners[1] + RS_Vector(h, k),
            corners[2] + RS_Vector(h, k), corners[3] + RS_Vector(h, k)};
        LC_AreaMoments direct;
        for (int i = 0; i < 4; ++i) {
            direct += LC_MomentQuadrature::segment(
                shifted[i], shifted[(i + 1) % 4], RS_Vector{0.0, 0.0});
        }
        REQUIRE_THAT(moved.area, WithinAbs(direct.area, 1e-13));
        REQUIRE_THAT(moved.first.mx, WithinAbs(direct.first.mx, 1e-13));
        REQUIRE_THAT(moved.first.my, WithinAbs(direct.first.my, 1e-13));
        REQUIRE_THAT(moved.second.ixx, WithinAbs(direct.second.ixx, 1e-12));
        REQUIRE_THAT(moved.second.iyy, WithinAbs(direct.second.iyy, 1e-13));
        REQUIRE_THAT(moved.second.ixy, WithinAbs(direct.second.ixy, 1e-13));
    }
    SECTION("regionRotated matches a rotated polygon") {
        for (double theta : {M_PI / 6.0, 0.6, -1.2}) {
            const LC_AreaMoments turned = base.regionRotated(theta);
            RS_Vector rotCorners[4];
            for (int i = 0; i < 4; ++i) {
                rotCorners[i] = corners[i];
                rotCorners[i].rotate(theta);
            }
            LC_AreaMoments direct;
            for (int i = 0; i < 4; ++i) {
                direct += LC_MomentQuadrature::segment(
                    rotCorners[i], rotCorners[(i + 1) % 4],
                    RS_Vector{0.0, 0.0});
            }
            REQUIRE_THAT(turned.area, WithinAbs(direct.area, 1e-13));
            REQUIRE_THAT(turned.first.mx, WithinRel(direct.first.mx, 1e-12));
            REQUIRE_THAT(turned.first.my, WithinRel(direct.first.my, 1e-12));
            REQUIRE_THAT(turned.second.ixx, WithinRel(direct.second.ixx, 1e-12));
            REQUIRE_THAT(turned.second.iyy, WithinRel(direct.second.iyy, 1e-12));
            REQUIRE_THAT(turned.second.ixy, WithinAbs(direct.second.ixy, 1e-12));
        }
    }
    SECTION("rotate forward then backward is the identity") {
        for (double theta : {0.3, 2.0, -1.1}) {
            const LC_AreaMoments r =
                base.regionRotated(theta).regionRotated(-theta);
            REQUIRE_THAT(r.area, WithinAbs(base.area, 1e-13));
            REQUIRE_THAT(r.first.mx, WithinAbs(base.first.mx, 1e-12));
            REQUIRE_THAT(r.first.my, WithinAbs(base.first.my, 1e-12));
            REQUIRE_THAT(r.second.ixx, WithinAbs(base.second.ixx, 1e-12));
            REQUIRE_THAT(r.second.iyy, WithinAbs(base.second.iyy, 1e-12));
            REQUIRE_THAT(r.second.ixy, WithinAbs(base.second.ixy, 1e-12));
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("LC_MomentQuadrature: rule constants and sweeps",
          "[lc-hatchfix][moments]") {
    SECTION("weights sum to one on [0, 1]") {
        double sum = 0.0;
        for (double w : LC_MomentQuadrature::kWeights) sum += w;
        REQUIRE_THAT(sum, WithinAbs(1.0, 4e-16));
    }
    SECTION("rule is exact for integrand t^14 on [-1, 1]") {
        double value = 0.0;
        for (std::size_t j = 0; j < LC_MomentQuadrature::kNodes.size(); ++j) {
            const double x = LC_MomentQuadrature::kNodes[j];
            const double w = LC_MomentQuadrature::kWeights[j];
            // Both symmetric nodes contribute: +x and -x give the same x^14.
            value += 2.0 * w * std::pow(x, 14);
        }
        REQUIRE_THAT(value, WithinAbs(2.0 / 15.0, 1e-15));
    }
    SECTION("signedSweep forward and reversed on simple spans") {
        REQUIRE(LC_MomentQuadrature::signedSweep(0.3, 2.4, false)
                == 2.4 - 0.3);
        REQUIRE(LC_MomentQuadrature::signedSweep(2.4, 0.3, true)
                == -(2.4 - 0.3));
        const double wrap =
            LC_MomentQuadrature::signedSweep(5.5, 1.2, false);
        REQUIRE_THAT(wrap, WithinAbs(1.2 - 5.5 + 2.0 * M_PI, 1e-15));
    }
    SECTION("exactly-equal parameters cover a full turn") {
        REQUIRE(LC_MomentQuadrature::signedSweep(1.3, 1.3, false)
                == 2.0 * M_PI);
        REQUIRE(LC_MomentQuadrature::signedSweep(1.3, 1.3, true)
                == -2.0 * M_PI);
    }
    SECTION("tiny forward sweep stays tiny") {
        const double a0 = 1.5707943267948965;
        const double a1 = 1.5707983267948964;
        REQUIRE(LC_MomentQuadrature::signedSweep(a0, a1, false)
                == a1 - a0);
    }
    SECTION("a nonzero short sweep is not promoted to 2 pi") {
        const double a0 = 1.0;
        const double a1 = 1.0 + 4e-10;
        const double s =
            LC_MomentQuadrature::signedSweep(a0, a1, false);
        REQUIRE(s > 0.0);
        REQUIRE(s < 1e-9);
    }
    SECTION("integrate(reversed) equals the negated forward run") {
        // A circular arc of radius 1 centred at the origin: segment [0.4, 1.7].
        auto eval = [](double t, const RS_Vector& /*org*/,
                       RS_Vector& pmo, RS_Vector& dp) {
            const double ct = std::cos(t);
            const double st = std::sin(t);
            pmo.x = ct;  pmo.y = st;  pmo.valid = true;
            dp.x = -st;  dp.y = ct;   dp.valid = true;
        };
        const LC_AreaMoments fwd = LC_MomentQuadrature::integrate(
            eval, 0.4, 1.7, M_PI / 8.0, RS_Vector{0.0, 0.0});
        const LC_AreaMoments rev = LC_MomentQuadrature::integrate(
            eval, 1.7, 0.4, M_PI / 8.0, RS_Vector{0.0, 0.0});
        REQUIRE_THAT(rev.area, WithinRel(-fwd.area, 1e-15));
        REQUIRE_THAT(rev.first.mx, WithinRel(-fwd.first.mx, 1e-15));
        REQUIRE_THAT(rev.first.my, WithinRel(-fwd.first.my, 1e-15));
        REQUIRE_THAT(rev.second.ixx, WithinRel(-fwd.second.ixx, 1e-15));
        REQUIRE_THAT(rev.second.iyy, WithinRel(-fwd.second.iyy, 1e-15));
        REQUIRE_THAT(rev.second.ixy, WithinRel(-fwd.second.ixy, 1e-15));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Ellipse moments: open arcs integrate in world coordinates",
          "[lc-hatchfix][moments]") {
    // c = (0, 5), majorP (3, 0), ratio 2/3, angle [0, pi].
    SECTION("half ellipse at (0, 5)") {
        RS_EllipseData ed;
        ed.center = RS_Vector(0.0, 5.0);
        ed.majorP = RS_Vector(3.0, 0.0);
        ed.ratio  = 2.0 / 3.0;
        ed.angle1 = 0.0;
        ed.angle2 = M_PI;
        ed.reversed = false;
        RS_Ellipse e(nullptr, ed);
        const LC_FirstMoment m = e.firstMomentLineIntegral();
        const LC_SecondMoment M = e.secondMomentLineIntegral();
        const double area = e.areaLineIntegral();
        REQUIRE_THAT(area, WithinAbs(9.42477796076938, 1e-10));
        REQUIRE_THAT(m.mx, WithinAbs(0.0, 1e-10));
        REQUIRE_THAT(m.my, WithinAbs(130.123889803847, 1e-10));
        REQUIRE_THAT(M.ixx, WithinAbs(21.2057504117311, 1e-10));
        REQUIRE_THAT(M.iyy, WithinAbs(575.044226980004, 1e-10));
        REQUIRE_THAT(M.ixy, WithinAbs(0.0, 1e-10));
    }
    SECTION("rotated elliptic arc at (10, 5) over [0.3, 2.4]") {
        RS_EllipseData ed;
        ed.center = RS_Vector(10.0, 5.0);
        const double phi = 30.0 * M_PI / 180.0;
        ed.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
        ed.ratio  = 2.0 / 3.0;
        ed.angle1 = 0.3;
        ed.angle2 = 2.4;
        ed.reversed = false;
        RS_Ellipse e(nullptr, ed);
        const auto m = e.momentLineIntegrals(RS_Vector(0.0, 0.0));
        REQUIRE_THAT(m.area, WithinAbs(-14.7191040924972, 1e-10));
        REQUIRE_THAT(m.first.mx, WithinAbs(-56.0297534333965, 1e-9));
        REQUIRE_THAT(m.first.my, WithinAbs(110.650884366711, 1e-9));
        REQUIRE_THAT(m.second.ixx, WithinAbs(-270.073318766809, 1e-8));
        REQUIRE_THAT(m.second.iyy, WithinAbs(507.122088507725, 1e-8));
        REQUIRE_THAT(m.second.ixy, WithinAbs(-333.487920451209, 1e-8));
    }
    SECTION("the same arc reversed negates every moment") {
        RS_EllipseData ed;
        ed.center = RS_Vector(10.0, 5.0);
        const double phi = 30.0 * M_PI / 180.0;
        ed.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
        ed.ratio  = 2.0 / 3.0;
        ed.angle1 = 2.4;
        ed.angle2 = 0.3;
        ed.reversed = true;
        RS_Ellipse e(nullptr, ed);
        const auto m = e.momentLineIntegrals(RS_Vector(0.0, 0.0));
        REQUIRE_THAT(m.area, WithinAbs(14.7191040924972, 1e-10));
        REQUIRE_THAT(m.first.mx, WithinAbs(56.0297534333965, 1e-9));
        REQUIRE_THAT(m.first.my, WithinAbs(-110.650884366711, 1e-9));
        REQUIRE_THAT(m.second.ixx, WithinAbs(270.073318766809, 1e-8));
        REQUIRE_THAT(m.second.iyy, WithinAbs(-507.122088507725, 1e-8));
        REQUIRE_THAT(m.second.ixy, WithinAbs(333.487920451209, 1e-8));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Arc moments: a nearly straight arc keeps its exact sweep",
          "[lc-hatchfix][moments]") {
    SECTION("regression guard on a short arc") {
        // c = (10, 5), r = 2, [0.3, 2.4]: Gauss-Legendre reproduces the old
        // closed-form oracle to machine precision.
        RS_Arc arc(nullptr, RS_ArcData(RS_Vector(10.0, 5.0), 2.0,
                                       0.3, 2.4, false));
        const auto m = arc.momentLineIntegrals(RS_Vector(0.0, 0.0));
        REQUIRE_THAT(m.area, WithinAbs(10.2380523955654, 1e-10));
        REQUIRE_THAT(m.first.mx, WithinAbs(65.5295024730824, 1e-9));
        REQUIRE_THAT(m.first.my, WithinAbs(76.1960615905801, 1e-9));
        REQUIRE_THAT(m.second.ixx, WithinAbs(542.014655410814, 1e-8));
        REQUIRE_THAT(m.second.iyy, WithinAbs(341.915419870663, 1e-8));
        REQUIRE_THAT(m.second.ixy, WithinAbs(399.556937091221, 1e-8));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Entity moments about an origin equal the moments of the moved entity",
          "[lc-hatchfix][moments]") {
    const RS_Vector origins[2] = {RS_Vector(3.0, -2.0),
                                   RS_Vector(1234.5, -678.25)};
    auto compareAtOrigin = [](const auto& e, auto makeMoved,
                              const RS_Vector& o) {
        const auto direct = e.momentLineIntegrals(o);
        auto moved = makeMoved(o);
        const auto shifted = moved.momentLineIntegrals(RS_Vector(0.0, 0.0));
        const double scale = 1.0 + std::fabs(direct.area)
                               + std::fabs(direct.first.mx)
                               + std::fabs(direct.first.my)
                               + std::fabs(direct.second.ixx)
                               + std::fabs(direct.second.iyy)
                               + std::fabs(direct.second.ixy);
        const double tol = 1e-10 * scale;
        REQUIRE_THAT(direct.area, WithinAbs(shifted.area, tol));
        REQUIRE_THAT(direct.first.mx, WithinAbs(shifted.first.mx, tol));
        REQUIRE_THAT(direct.first.my, WithinAbs(shifted.first.my, tol));
        REQUIRE_THAT(direct.second.ixx, WithinAbs(shifted.second.ixx, tol));
        REQUIRE_THAT(direct.second.iyy, WithinAbs(shifted.second.iyy, tol));
        REQUIRE_THAT(direct.second.ixy, WithinAbs(shifted.second.ixy, tol));
    };
    SECTION("line") {
        RS_Line base(nullptr, RS_Vector(1.0, 2.0), RS_Vector(5.0, 7.0));
        for (const auto& o : origins) {
            compareAtOrigin(base, [](const RS_Vector& oo) {
                return RS_Line(nullptr, RS_Vector(1.0, 2.0) - oo,
                               RS_Vector(5.0, 7.0) - oo);
            }, o);
        }
    }
    SECTION("circle") {
        RS_Circle base(nullptr, RS_CircleData(RS_Vector(4.0, 3.0), 2.5));
        for (const auto& o : origins) {
            compareAtOrigin(base, [](const RS_Vector& oo) {
                return RS_Circle(nullptr,
                                 RS_CircleData(RS_Vector(4.0, 3.0) - oo, 2.5));
            }, o);
        }
    }
    SECTION("arc") {
        RS_Arc base(nullptr, RS_ArcData(RS_Vector(10.0, 5.0), 2.0,
                                        0.3, 2.4, false));
        for (const auto& o : origins) {
            compareAtOrigin(base, [](const RS_Vector& oo) {
                return RS_Arc(nullptr, RS_ArcData(RS_Vector(10.0, 5.0) - oo,
                                                   2.0, 0.3, 2.4, false));
            }, o);
        }
    }
    SECTION("elliptic arc") {
        RS_EllipseData ed;
        ed.center = RS_Vector(10.0, 5.0);
        const double phi = 30.0 * M_PI / 180.0;
        ed.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
        ed.ratio  = 2.0 / 3.0;
        ed.angle1 = 0.3;
        ed.angle2 = 2.4;
        ed.reversed = false;
        RS_Ellipse base(nullptr, ed);
        for (const auto& o : origins) {
            compareAtOrigin(base, [phi](const RS_Vector& oo) {
                RS_EllipseData ed2;
                ed2.center = RS_Vector(10.0, 5.0) - oo;
                ed2.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
                ed2.ratio  = 2.0 / 3.0;
                ed2.angle1 = 0.3;
                ed2.angle2 = 2.4;
                ed2.reversed = false;
                return RS_Ellipse(nullptr, ed2);
            }, o);
        }
    }
    SECTION("whole ellipse") {
        RS_EllipseData ed;
        ed.center = RS_Vector(10.0, 5.0);
        const double phi = 30.0 * M_PI / 180.0;
        ed.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
        ed.ratio  = 2.0 / 3.0;
        ed.angle1 = 0.0;
        ed.angle2 = 0.0;
        ed.reversed = false;
        RS_Ellipse base(nullptr, ed);
        for (const auto& o : origins) {
            compareAtOrigin(base, [phi](const RS_Vector& oo) {
                RS_EllipseData ed2;
                ed2.center = RS_Vector(10.0, 5.0) - oo;
                ed2.majorP = RS_Vector(3.0 * std::cos(phi), 3.0 * std::sin(phi));
                ed2.ratio  = 2.0 / 3.0;
                ed2.angle1 = 0.0;
                ed2.angle2 = 0.0;
                ed2.reversed = false;
                return RS_Ellipse(nullptr, ed2);
            }, o);
        }
    }
}
