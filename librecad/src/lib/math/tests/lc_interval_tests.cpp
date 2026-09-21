/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>

#include "lc_interval.h"

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kEps = std::numeric_limits<double>::epsilon();
}

TEST_CASE("LC_Interval encloses the exact result of each operation", "[curve-offset][d0][interval]") {
    // 0.1 + 0.2 is not representable: the rounded sum must lie inside the result,
    // and so must the exact value, which lies within half an ulp of it.
    const LC_Interval sum = LC_Interval::point(0.1) + LC_Interval::point(0.2);
    CHECK(sum.contains(0.1 + 0.2));
    CHECK(sum.lo() < 0.1 + 0.2);
    CHECK(sum.hi() > 0.1 + 0.2);
    CHECK(sum.width() <= 4.0 * kEps);

    const LC_Interval a = LC_Interval::hull(-2.0, 3.0);
    const LC_Interval b = LC_Interval::hull(-1.0, 4.0);
    const LC_Interval product = a * b;
    CHECK(product.contains(-8.0));  // -2 * 4
    CHECK(product.contains(12.0));  // 3 * 4
    CHECK(product.lo() <= -8.0);
    CHECK(product.hi() >= 12.0);

    const LC_Interval difference = a - b;
    CHECK(difference.contains(-6.0));
    CHECK(difference.contains(4.0));

    const LC_Interval quotient = LC_Interval::hull(1.0, 2.0) / LC_Interval::hull(4.0, 8.0);
    CHECK(quotient.contains(0.125));
    CHECK(quotient.contains(0.5));
    CHECK(quotient.isPositive());
}

TEST_CASE("LC_Interval squares and roots", "[curve-offset][d0][interval]") {
    // x^2 over an interval containing zero starts at exactly zero, while x * x
    // cannot know both factors are the same value.
    const LC_Interval x = LC_Interval::hull(-2.0, 3.0);
    CHECK(sqr(x).lo() == 0.0);
    CHECK(sqr(x).contains(9.0));
    CHECK((x * x).lo() < 0.0);
    CHECK(sqr(LC_Interval::hull(-3.0, -2.0)).contains(4.0));
    CHECK(sqr(LC_Interval::hull(-3.0, -2.0)).isPositive());

    CHECK(sqrt(LC_Interval::hull(4.0, 9.0)).contains(2.0));
    CHECK(sqrt(LC_Interval::hull(4.0, 9.0)).contains(3.0));
    // the domain boundary: the non-negative part is used, nothing below zero is not
    CHECK(sqrt(LC_Interval::hull(-1.0, 4.0)).lo() == 0.0);
    CHECK(sqrt(LC_Interval::hull(-1.0, 4.0)).contains(2.0));
    CHECK_FALSE(sqrt(LC_Interval::hull(-4.0, -1.0)).isValid());
    CHECK(sqrt(LC_Interval::point(2.0)).contains(std::sqrt(2.0)));
}

TEST_CASE("LC_Interval division by an interval containing zero excludes nothing",
          "[curve-offset][d0][interval]") {
    const LC_Interval q = LC_Interval::point(1.0) / LC_Interval::hull(-1.0, 1.0);
    CHECK(q.isValid());
    CHECK(q.lo() == -kInf);
    CHECK(q.hi() == kInf);
    CHECK_FALSE(q.excludesZero());
    CHECK((LC_Interval::point(1.0) / LC_Interval::point(0.0)).lo() == -kInf);
}

TEST_CASE("LC_Interval signed zero, NaN and overflow", "[curve-offset][d0][interval]") {
    const LC_Interval negativeZero = LC_Interval::point(-0.0);
    CHECK(negativeZero.containsZero());
    CHECK_FALSE(negativeZero.excludesZero());
    CHECK(LC_Interval::hull(-0.0, 0.0).containsZero());

    // NaN is never a bound: it invalidates, and invalid operands stay invalid.
    CHECK_FALSE(LC_Interval::point(kNaN).isValid());
    CHECK_FALSE(LC_Interval{}.isValid());
    CHECK_FALSE((LC_Interval::point(kNaN) + LC_Interval::point(1.0)).isValid());
    CHECK_FALSE((LC_Interval{} * LC_Interval::point(2.0)).isValid());
    CHECK_FALSE(LC_Interval{}.excludesZero());
    CHECK_FALSE(LC_Interval{}.containsZero());
    CHECK_FALSE((LC_Interval::point(kInf) + LC_Interval::point(-kInf)).isValid());

    // overflow keeps a bound on the correct side
    const double big = std::numeric_limits<double>::max();
    const LC_Interval overflow = LC_Interval::point(big) + LC_Interval::point(big);
    CHECK(overflow.isValid());
    CHECK(overflow.hi() == kInf);
    CHECK(overflow.lo() >= big);
    CHECK(overflow.isPositive());

    // a zero factor bounds the product at zero even against an infinite bound
    const LC_Interval halfOpen = LC_Interval::hull(0.0, 1.0) * LC_Interval::hull(1.0, kInf);
    CHECK(halfOpen.isValid());
    CHECK(halfOpen.lo() <= 0.0);
    CHECK(halfOpen.hi() == kInf);
}

TEST_CASE("LC_Interval sign predicates are strict", "[curve-offset][d0][interval]") {
    CHECK(LC_Interval::hull(1e-300, 1.0).excludesZero());
    CHECK(LC_Interval::hull(-1.0, -1e-300).excludesZero());
    CHECK_FALSE(LC_Interval::hull(0.0, 1.0).excludesZero());
    CHECK_FALSE(LC_Interval::hull(-1.0, 0.0).excludesZero());
    // a rounded result that touches zero is not positive
    CHECK_FALSE((LC_Interval::point(1.0) - LC_Interval::point(1.0)).excludesZero());
}
