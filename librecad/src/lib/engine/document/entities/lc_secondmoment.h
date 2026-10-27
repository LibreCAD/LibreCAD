/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2026 librecad (www.librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/
#ifndef LC_SECONDMOMENT_H
#define LC_SECONDMOMENT_H

#include <cmath>

/**
 * @brief First moments of area for a 2D region (0-order + 1st-order moments).
 *
 * mx = ∬ x dA   via  (1/2) ∮ x² dy
 * my = ∬ y dA   via -(1/2) ∮ y² dx
 *
 * These are the static moments used to compute the centroid: cx = mx / A, cy = my / A.
 */
struct LC_FirstMoment {
    double mx = 0.0;  ///< ∬ x dA
    double my = 0.0;  ///< ∬ y dA

    LC_FirstMoment& operator+=(const LC_FirstMoment& o) {
        mx += o.mx; my += o.my;
        return *this;
    }
    LC_FirstMoment& operator-=(const LC_FirstMoment& o) {
        mx -= o.mx; my -= o.my;
        return *this;
    }
    LC_FirstMoment operator+(const LC_FirstMoment& o) const {
        return {mx + o.mx, my + o.my};
    }
    LC_FirstMoment operator-(const LC_FirstMoment& o) const {
        return {mx - o.mx, my - o.my};
    }
    LC_FirstMoment operator-() const { return {-mx, -my}; }

    /**
     * @brief Rotate the first-moment vector by angle θ (radians, counterclockwise).
     */
    LC_FirstMoment rotated(double theta) const {
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return {mx * c - my * s, mx * s + my * c};
    }
};

/**
 * @brief Second moments of area for a 2D region, computed via Green's theorem contour integrals.
 *
 * All three components are area integrals obtained by summing the line integral
 * contributions of each boundary entity:
 *
 *   ixx = ∬ x² dA   via  ∮ (x³/3) dy
 *   iyy = ∬ y² dA   via  -∮ (y³/3) dx
 *   ixy = ∬ x·y dA  via  ∮ (x²·y/2) dy
 *
 * Full transforming support (translation via parallel-axis theorem + tensor rotation)
 * has been added. All formulas are exact and consistent with the Green's theorem
 * line integrals already implemented for lines and ellipses.
 */
struct LC_SecondMoment {
    double ixx = 0.0;  ///< ∬ x² dA
    double iyy = 0.0;  ///< ∬ y² dA
    double ixy = 0.0;  ///< ∬ x·y dA

    LC_SecondMoment& operator+=(const LC_SecondMoment& o) {
        ixx += o.ixx; iyy += o.iyy; ixy += o.ixy;
        return *this;
    }
    LC_SecondMoment& operator-=(const LC_SecondMoment& o) {
        ixx -= o.ixx; iyy -= o.iyy; ixy -= o.ixy;
        return *this;
    }
    LC_SecondMoment operator+(const LC_SecondMoment& o) const {
        return {ixx + o.ixx, iyy + o.iyy, ixy + o.ixy};
    }
    LC_SecondMoment operator-(const LC_SecondMoment& o) const {
        return {ixx - o.ixx, iyy - o.iyy, ixy - o.ixy};
    }
    LC_SecondMoment operator-() const { return {-ixx, -iyy, -ixy}; }

    /**
     * @brief Legacy rotation: turns by -theta. Kept while deprecated callers
     *        exist; new code should use regionRotated(theta) instead.
     */
    LC_SecondMoment rotated(double theta) const {
        const double c  = std::cos(theta);
        const double s  = std::sin(theta);
        const double cc = c * c;
        const double ss = s * s;
        const double cs = c * s;
        return {
            cc * ixx + ss * iyy + 2.0 * cs * ixy,
            ss * ixx + cc * iyy - 2.0 * cs * ixy,
            cs * (iyy - ixx) + (cc - ss) * ixy
        };
    }

    /**
     * @brief R(theta)*M*R(theta)^T: the tensor of a closed region turned
     *        counter-clockwise by theta about the origin. This is the
     *        companion to LC_FirstMoment::rotated, which also turns by
     *        +theta. Use this on hatch moments mapped from the loop frame
     *        back to WCS; do not use rotated() for that purpose (it turns
     *        the tensor by -theta).
     */
    LC_SecondMoment regionRotated(double theta) const {
        const double c  = std::cos(theta);
        const double s  = std::sin(theta);
        const double cc = c * c;
        const double ss = s * s;
        const double cs = c * s;
        return {
            cc * ixx - 2.0 * cs * ixy + ss * iyy,
            ss * ixx + 2.0 * cs * ixy + cc * iyy,
            cs * (ixx - iyy) + (cc - ss) * ixy
        };
    }

    /**
     * @brief Convert raw second moments (about global origin) to central moments.
     */
    LC_SecondMoment getCentral(double area, double cx, double cy) const {
        return {
            ixx - area * cx * cx,
            iyy - area * cy * cy,
            ixy - area * cx * cy
        };
    }
};

/**
 * @brief Area and raw first/second moments of a 2D region, bundled together.
 *
 * Returned by the Green's-theorem line integrals of (P − origin) summed around
 * a closed loop. The three members hold ∬1, (∬x, ∬y) and (∬x², ∬y², ∬xy) in
 * the WCS translated so that `origin` is at (0, 0). Loop code sums one such
 * object per edge; the regionTranslated/regionRotated helpers map a *closed*
 * region's moments to a different origin or frame.
 */
struct LC_AreaMoments {
    double area = 0.0;
    LC_FirstMoment first;
    LC_SecondMoment second;

    LC_AreaMoments& operator+=(const LC_AreaMoments& o) {
        area += o.area;
        first += o.first;
        second += o.second;
        return *this;
    }
    LC_AreaMoments& operator-=(const LC_AreaMoments& o) {
        area -= o.area;
        first -= o.first;
        second -= o.second;
        return *this;
    }
    LC_AreaMoments operator+(const LC_AreaMoments& o) const {
        return {area + o.area, first + o.first, second + o.second};
    }
    LC_AreaMoments operator-(const LC_AreaMoments& o) const {
        return {area - o.area, first - o.first, second - o.second};
    }
    LC_AreaMoments operator-() const { return {-area, -first, -second}; }

    /**
     * @brief Translate a *closed* region's moments: new_x = old_x + h,
     *        new_y = old_y + k. For an open contour this transform does not
     *        hold; sum the per-edge momentLineIntegrals around the loop first.
     *
     *   mx' = mx + h·A, my' = my + k·A
     *   Ixx' = Ixx + 2h·mx + h²·A
     *   Iyy' = Iyy + 2k·my + k²·A
     *   Ixy' = Ixy + h·my + k·mx + h·k·A
     */
    LC_AreaMoments regionTranslated(double h, double k) const {
        LC_AreaMoments out;
        out.area = area;
        out.first.mx = first.mx + h * area;
        out.first.my = first.my + k * area;
        out.second.ixx = second.ixx + 2.0 * h * first.mx + h * h * area;
        out.second.iyy = second.iyy + 2.0 * k * first.my + k * k * area;
        out.second.ixy = second.ixy + h * first.my + k * first.mx + h * k * area;
        return out;
    }

    /**
     * @brief Rotate a *closed* region's moments by +theta (counter-clockwise)
     *        about the current origin. The area is invariant, the first moment
     *        rotates as a vector, and the second-moment tensor transforms as
     *        R·M·R^T; this is the companion of LC_FirstMoment::rotated.
     */
    LC_AreaMoments regionRotated(double theta) const {
        return {area, first.rotated(theta), second.regionRotated(theta)};
    }
};

#endif // LC_SECONDMOMENT_H
