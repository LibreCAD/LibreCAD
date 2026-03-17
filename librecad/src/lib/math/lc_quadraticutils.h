/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**

** Copyright (C) 2015-2024 LibreCAD.org
** Copyright (C) 2015-2024 Dongxu Li (dongxuli2011@gmail.com)

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
**********************************************************************/

#ifndef LC_QUADRATIC_UTILS_H
#define LC_QUADRATIC_UTILS_H

#include "lc_quadratic.h"
#include "rs_entity.h"
#include "rs_vector.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "lc_parabola.h"
#include "lc_hyperbola.h"
#include "rs_polyline.h"

#include <array>
#include <utility>

namespace LC_QuadraticUtils {

/* ────────────────────────────────────────────────────────────────────────────
   Basic properties / classification
   ──────────────────────────────────────────────────────────────────────────── */

bool isValid(const LC_Quadratic& q);
bool isQuadratic(const LC_Quadratic& q);
bool isDegenerate(const LC_Quadratic& q);
bool isParabolaCondition(double discriminant, const LC_Quadratic& q);
/* ────────────────────────────────────────────────────────────────────────────
   Geometric properties
   ──────────────────────────────────────────────────────────────────────────── */

RS_Vector computeCenter(const LC_Quadratic& q);

double evaluateAt(const LC_Quadratic& q, const RS_Vector& point);

double computeDiscriminant(const LC_Quadratic& q);

double computeQuadNorm(const LC_Quadratic& q);

double computeScale(const LC_Quadratic& q);

double computeHomogeneousDeterminant(const LC_Quadratic& q);

double computeRotationAngle(const LC_Quadratic& q);

std::pair<double, double> computePrincipalCoefficients(const LC_Quadratic& q, double theta);

/* ────────────────────────────────────────────────────────────────────────────
   Entity creation helpers
   ──────────────────────────────────────────────────────────────────────────── */

RS_Entity* createDualAroundCenter( RS_Entity* entity, const RS_Vector& center);

RS_Entity* createLineFromLinearCoefficients(const LC_Quadratic& q);

RS_Entity* createDegeneratePointOrIntersecting(const LC_Quadratic& q, const RS_Vector& center, double v);

/* ────────────────────────────────────────────────────────────────────────────
   Conic factories (non-degenerate)
   ──────────────────────────────────────────────────────────────────────────── */

RS_Entity* createEllipseOrCircle(const LC_Quadratic& q, const RS_Vector& center, double v, double Ap, double Cp, double theta);

RS_Entity* createParabola(const LC_Quadratic& q, const RS_Vector& center, double v, double Ap, double Cp);

RS_Entity* createHyperbola(const LC_Quadratic& q, const RS_Vector& center, double v, double Ap, double Cp, double theta);

/* ────────────────────────────────────────────────────────────────────────────
   Parabola-specific utilities
   ──────────────────────────────────────────────────────────────────────────── */

std::array<RS_Vector, 3> computeParabolaControlPoints(
    const RS_Vector& center, double p, const RS_Vector& axis, const RS_Vector& perp);

/* ────────────────────────────────────────────────────────────────────────────
   Hyperbola-specific utilities
   ──────────────────────────────────────────────────────────────────────────── */

std::pair<RS_Vector, double> computeHyperbolaMajorPAndRatio(
    double Ap, double Cp, double v, double theta);

/* ────────────────────────────────────────────────────────────────────────────
   Ellipse-specific utilities
   ──────────────────────────────────────────────────────────────────────────── */

std::tuple<double, double, RS_Vector> computeEllipseSemiAxesAndMajorP(
    double Ap, double Cp, double v, double theta);

} // namespace LC_QuadraticUtils

#endif // LC_QUADRATIC_UTILS_H
//EOF
