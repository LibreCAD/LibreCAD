/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ******************************************************************************/

#ifndef LC_ELLIPSEOFFSETGEOMETRY_H
#define LC_ELLIPSEOFFSETGEOMETRY_H

#include <optional>
#include <vector>

#include "rs_ellipse.h"

struct LC_EllipseOffsetDefinition {
    RS_EllipseData basis;
    double distance = 0.0;
    bool towardCentre = false;
    bool cacheReversed = false;
    double relTolerance = 0.0;
};

namespace LC_EllipseOffsetGeometry {

struct Sweep {
    double start = 0.0;
    double signedSweep = 0.0;
    bool whole = false;

    bool contains(double parameter, double angularAllowance = 0.0) const;
};

bool isFinite2D(const RS_Vector& point);
/** Validate raw fields before RS_Ellipse can canonicalize tiny angles. Unchanged on refusal. */
bool normaliseBasis(RS_EllipseData& basis);
/** The source parameter convention used by the offset engine, including its whole-ellipse seam. */
std::optional<Sweep> sweep(const RS_EllipseData& basis);

// These operations require a normalized basis. Invalid/unrepresentable results are invalid vectors or NaN.
RS_Vector ellipsePoint(const RS_EllipseData& basis, double parameter);
RS_Vector ellipseDerivative(const RS_EllipseData& basis, double parameter);
RS_Vector outwardNormal(const RS_EllipseData& basis, double parameter);
RS_Vector exactOffsetPoint(const RS_EllipseData& basis, double parameter, double signedDistance);
double radiusOfCurvature(const RS_EllipseData& basis, double parameter);
double minRadiusOfCurvature(const RS_EllipseData& basis);
bool isInside(const RS_EllipseData& basis, const RS_Vector& point);

/** First-quadrant source parameter of a whole inward lens corner; no value for circles/non-lenses. */
std::optional<double> lensTrimAngle(const RS_EllipseData& basis, double distance);
std::vector<RS_Vector> lensCorners(const RS_EllipseData& basis, double distance);

/** Analytic nearest-point query with a bounded exterior solve, not a conservative cache-validation proof. */
double distanceToOriginal(const RS_EllipseData& basis, const RS_Vector& point, double* parameter = nullptr);

} // namespace LC_EllipseOffsetGeometry

#endif
