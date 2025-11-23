/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 librecad.org
** Copyright (C) 2025 Dongxu Li (github.com/dxli)

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
// File: lc_splinehelper.h

#ifndef LC_SPLINEHELPER_H
#define LC_SPLINEHELPER_H

#include <cstddef>
#include <vector>
#include "rs_spline.h"

/**
 * @class LC_SplineHelper
 * @brief Helper class for spline type manipulations, knot conversions, and wrapping.
 */
class LC_SplineHelper {
public:
    // Knot conversions
    static std::vector<double> convertClosedToOpenKnotVector(const std::vector<double>& closedKnotVector,
                                                             size_t unwrappedControlCount,
                                                             size_t splineDegree);
    static std::vector<double> convertOpenToClosedKnotVector(const std::vector<double>& openKnots,
                                                             size_t n,
                                                             size_t splineDegree);
    static std::vector<double> getNormalizedKnotVector(const std::vector<double>& inputKnotVector,
                                                       double newMinimum,
                                                       const std::vector<double>& fallbackKnotVector);
    static std::vector<double> unclampKnotVector(const std::vector<double>& inputKnotVector,
                                                 size_t controlPointCount,
                                                 size_t splineOrder);
    static std::vector<double> clampKnotVector(const std::vector<double>& inputKnotVector,
                                               size_t controlPointCount,
                                               size_t splineOrder);

    // Type conversions (non-uniform preserving)
    static void toClampedOpenFromStandard(RS_SplineData& splineData);
    static void toStandardFromClampedOpen(RS_SplineData& splineData);
    static void toWrappedClosedFromStandard(RS_SplineData& splineData);
    static void toStandardFromWrappedClosed(RS_SplineData& splineData);
    static void toClampedOpenFromWrappedClosed(RS_SplineData& splineData);
    static void toWrappedClosedFromClampedOpen(RS_SplineData& splineData);

    // Wrapping
    static void addWrapping(RS_SplineData& splineData);
    static void removeWrapping(RS_SplineData& splineData);
    static void updateControlAndWeightWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount);
    static void updateKnotWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount);

    // Generators
    static std::vector<double> knot(size_t controlPointCount, size_t splineOrder);  // Clamped uniform
    static std::vector<double> generateOpenUniformKnotVector(size_t controlPointCount, size_t splineOrder);

    // Knot manipulation
    static void extendKnotVector(std::vector<double>& knots);
    static void insertKnot(std::vector<double>& knots, size_t knot_index);
    static void removeKnot(std::vector<double>& knots, size_t knot_index);
    static void ensureMonotonic(std::vector<double>& knots);

    // Validation
    static bool validate(const RS_SplineData& data);

private:
};

#endif // LC_SPLINEHELPER_H
