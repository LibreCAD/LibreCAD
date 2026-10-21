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

#ifndef LC_CURVEJET_H
#define LC_CURVEJET_H

#include "lc_interval.h"
#include "rs_vector.h"

/**
 * Which one-sided limit a curve evaluation takes at a parameter where its
 * derivatives may jump: a knot of an RS_Spline or a segment join of an
 * LC_SplinePoints.
 */
enum class LC_CurveEvaluationSide {
    /** Away from a break either side gives the same value; at a break the right
     *  limit is taken, except at the end of the domain, where only the left
     *  limit exists. */
    Interior,
    /** Limit from below. Undefined at the start of the domain. */
    Left,
    /** Limit from above. Undefined at the end of the domain. */
    Right
};

/**
 * A curve point with its first and second derivatives, all with respect to the
 * curve's own parameter. A failed evaluation leaves the three vectors invalid;
 * it never reports zero derivatives in place of an error.
 */
struct LC_CurveJet {
    RS_Vector point{false};
    RS_Vector first{false};
    RS_Vector second{false};
};

/**
 * Conservative enclosures of a curve's point and first two derivatives over a
 * parameter box, component by component. Each interval contains every value the
 * quantity takes on the box; it may be wider.
 */
struct LC_CurveJetBounds {
    LC_Interval x;
    LC_Interval y;
    LC_Interval dx;
    LC_Interval dy;
    LC_Interval ddx;
    LC_Interval ddy;
    /**
     * Optional enclosures of |C'|^2 and C' x C'' from the Bezier coefficients
     * of the products themselves, which the curve may fill in; invalid when
     * it does not. Next to a point where the tangent vanishes C' and C'' are
     * all but parallel, and the products of the component intervals cannot
     * resolve the sign of the curvature, however small the box.
     */
    LC_Interval speedSquaredProduct;
    LC_Interval crossProduct;

    bool isValid() const {
        return x.isValid() && y.isValid() && dx.isValid() && dy.isValid() && ddx.isValid() &&
               ddy.isValid();
    }

    /** |C'|^2: excludes zero only where the tangent provably exists. */
    LC_Interval speedSquared() const {
        return narrowest(sqr(dx) + sqr(dy), speedSquaredProduct);
    }

    /** C' x C'': signed, positive where the curve turns left. */
    LC_Interval cross() const {
        return narrowest(dx * ddy - dy * ddx, crossProduct);
    }

    /**
     * |C'|^3 - d (C' x C''), which has the sign of 1 - d * kappa wherever the
     * speed is positive: kappa = (C' x C'') / |C'|^3 is the signed curvature and
     * @p signedDistance is measured along the left normal. The offset
     * C + d N is regular where the speed is positive and this excludes zero.
     */
    LC_Interval offsetFactorNumerator(const double signedDistance) const {
        const LC_Interval s2 = speedSquared();
        return s2 * sqrt(s2) - LC_Interval::point(signedDistance) * cross();
    }

private:
    /** The common part of two enclosures of one quantity, or @p a alone without a valid @p b. */
    static LC_Interval narrowest(const LC_Interval& a, const LC_Interval& b) {
        if (!b.isValid() || !a.isValid() || b.hi() < a.lo() || a.hi() < b.lo()) {
            return a;
        }
        return LC_Interval::hull(std::max(a.lo(), b.lo()), std::min(a.hi(), b.hi()));
    }
};

#endif
