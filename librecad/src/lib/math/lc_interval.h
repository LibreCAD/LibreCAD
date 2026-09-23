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

#ifndef LC_INTERVAL_H
#define LC_INTERVAL_H

#include <algorithm>
#include <cmath>
#include <limits>

/**
 * A closed interval [lo, hi] of doubles for conservative enclosures.
 *
 * Every computed bound is rounded one ulp outward, which covers the at most
 * half-ulp error of a correctly rounded operation, so the result encloses the
 * exact result of the same operation on any values inside the operands. An
 * interval with a NaN bound is invalid, and invalid operands give an invalid
 * result, so a failed computation can never be mistaken for a bound.
 *
 * Division by an interval that contains zero, and any other operation whose
 * result is unbounded, gives [-inf, +inf]: valid, but excluding nothing.
 */
class LC_Interval {
public:
    /** An invalid interval. */
    constexpr LC_Interval() = default;

    /** The exact value @p v. Invalid for NaN. */
    static LC_Interval point(const double v) {
        return LC_Interval{v, v, Exact{}};
    }

    /** The smallest interval containing @p a and @p b. */
    static LC_Interval hull(const double a, const double b) {
        return LC_Interval{std::min(a, b), std::max(a, b), Exact{}};
    }

    static LC_Interval entire() {
        constexpr double inf = std::numeric_limits<double>::infinity();
        return LC_Interval{-inf, inf, Exact{}};
    }

    double lo() const {
        return m_lo;
    }

    double hi() const {
        return m_hi;
    }

    bool isValid() const {
        return m_lo <= m_hi; // false for NaN bounds
    }

    bool contains(const double v) const {
        return isValid() && m_lo <= v && v <= m_hi;
    }

    bool containsZero() const {
        return contains(0.0);
    }

    /** True only when the interval provably contains no zero. */
    bool excludesZero() const {
        return isValid() && (m_lo > 0.0 || m_hi < 0.0);
    }

    bool isPositive() const {
        return isValid() && m_lo > 0.0;
    }

    bool isNegative() const {
        return isValid() && m_hi < 0.0;
    }

    double width() const {
        return isValid() ? m_hi - m_lo : std::numeric_limits<double>::quiet_NaN();
    }

    /** The hull of two intervals; invalid if either is. */
    static LC_Interval hull(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid()) {
            return {};
        }
        return LC_Interval{std::min(a.m_lo, b.m_lo), std::max(a.m_hi, b.m_hi), Exact{}};
    }

    LC_Interval operator-() const {
        return isValid() ? LC_Interval{-m_hi, -m_lo, Exact{}} : LC_Interval{};
    }

    friend LC_Interval operator+(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid()) {
            return {};
        }
        return LC_Interval{a.m_lo + b.m_lo, a.m_hi + b.m_hi};
    }

    friend LC_Interval operator-(const LC_Interval& a, const LC_Interval& b) {
        return a + (-b);
    }

    friend LC_Interval operator*(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid()) {
            return {};
        }
        const double p1 = product(a.m_lo, b.m_lo);
        const double p2 = product(a.m_lo, b.m_hi);
        const double p3 = product(a.m_hi, b.m_lo);
        const double p4 = product(a.m_hi, b.m_hi);
        return LC_Interval{std::min({p1, p2, p3, p4}), std::max({p1, p2, p3, p4})};
    }

    friend LC_Interval operator/(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid()) {
            return {};
        }
        if (b.containsZero()) {
            return entire();
        }
        return a * LC_Interval{1.0 / b.m_hi, 1.0 / b.m_lo};
    }

    /** x^2, which unlike x * x knows both factors are the same value. */
    friend LC_Interval sqr(const LC_Interval& x) {
        if (!x.isValid()) {
            return {};
        }
        const double a = x.m_lo * x.m_lo;
        const double b = x.m_hi * x.m_hi;
        if (x.containsZero()) {
            return LC_Interval{0.0, std::max(a, b), Rounded{}, true};
        }
        return LC_Interval{std::min(a, b), std::max(a, b)};
    }

    /** Square root of the part of @p x that is not negative; invalid if none is. */
    friend LC_Interval sqrt(const LC_Interval& x) {
        if (!x.isValid() || x.m_hi < 0.0) {
            return {};
        }
        const double lo = std::max(x.m_lo, 0.0);
        return LC_Interval{std::sqrt(lo), std::sqrt(x.m_hi), Rounded{}, lo == 0.0};
    }

private:
    struct Exact {};
    struct Rounded {};

    /** Bounds from exact data: no rounding. NaN makes the interval invalid. */
    LC_Interval(const double lo, const double hi, Exact)
        : m_lo{lo},
          m_hi{hi} {
    }

    /** Bounds computed with rounding: widened one ulp outward. */
    LC_Interval(const double lo, const double hi)
        : m_lo{down(lo)},
          m_hi{up(hi)} {
        makeInvalidIfNaN();
    }

    /** As above, but a lower bound of exactly zero that is known to be exact
     *  (a square or a square root cannot be negative) is kept. */
    LC_Interval(const double lo, const double hi, Rounded, const bool exactZeroLow)
        : m_lo{(exactZeroLow && lo == 0.0) ? 0.0 : down(lo)},
          m_hi{up(hi)} {
        makeInvalidIfNaN();
    }

    void makeInvalidIfNaN() {
        if (std::isnan(m_lo) || std::isnan(m_hi)) {
            m_lo = std::numeric_limits<double>::quiet_NaN();
            m_hi = std::numeric_limits<double>::quiet_NaN();
        }
    }

    static double down(const double v) {
        return std::nextafter(v, -std::numeric_limits<double>::infinity());
    }

    static double up(const double v) {
        return std::nextafter(v, std::numeric_limits<double>::infinity());
    }

    /** a * b, with 0 * inf = 0: a zero factor bounds the product at zero. */
    static double product(const double a, const double b) {
        return (a == 0.0 || b == 0.0) ? 0.0 : a * b;
    }

    double m_lo = std::numeric_limits<double>::quiet_NaN();
    double m_hi = std::numeric_limits<double>::quiet_NaN();
};

#endif
