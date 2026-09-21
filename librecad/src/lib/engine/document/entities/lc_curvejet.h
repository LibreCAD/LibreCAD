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

#endif
