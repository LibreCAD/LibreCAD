/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef LC_SPLINEHELPER_H
#define LC_SPLINEHELPER_H

#include <vector>
#include <cstddef>
#include "rs_spline.h"

/**
 * @class LC_SplineHelper
 * @brief Helper class for spline type manipulations, knot conversions, and wrapping.
 * Includes Boehm's algorithm for non-uniform knot insertion/removal.
 *
 * This class provides static methods to handle conversions between spline representations
 * (Standard, ClampedOpen, WrappedClosed) while preserving non-uniform knots via snapshots.
 * It supports rational B-splines (NURBS) and ensures round-trip integrity.
 */
class LC_SplineHelper {
public:
    // Knot conversions
    static std::vector<double> convertClosedToOpenKnotVector(const std::vector<double>& closedKnotVector,
                                                             size_t unwrappedControlCount,
                                                             size_t splineDegree);
    static std::vector<double> getNormalizedKnotVector(const std::vector<double>& inputKnotVector,
                                                       double newMinimum,
                                                       double newMaximum,
                                                       const std::vector<double>& fallbackKnotVector);
    static std::vector<double> unclampKnotVector(const std::vector<double>& inputKnotVector,
                                                 size_t controlPointCount,
                                                 size_t splineOrder);
    static std::vector<double> clampKnotVector(const std::vector<double>& inputKnotVector,
                                               size_t controlPointCount,
                                               size_t splineOrder);

    // Type conversions (non-uniform preserving)
    static void toClampedOpenFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount);
    static void toStandardFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount);
    static void toWrappedClosedFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount);
    static void toClampedOpenFromWrappedClosed(RS_SplineData& splineData, size_t& unwrappedControlCount);
    static void toWrappedClosedFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount);
    static void toStandardFromWrappedClosed(RS_SplineData& splineData, size_t unwrappedControlCount);

    // Wrapping
    static void addWrapping(RS_SplineData& splineData, bool isClosed);
    static void removeWrapping(RS_SplineData& splineData, bool isClosed, size_t& unwrappedControlCount);
    static void updateControlAndWeightWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount);
    static void updateKnotWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount);

    // Validation
    static bool validate(const RS_SplineData& splineData, size_t unwrappedControlCount);
    static bool isCustomKnotVector(const std::vector<double>& knotVector, size_t controlCount, size_t splineOrder);  // Non-uniform detection

    // Boehm's algorithms (non-uniform)
    static int findSpan(const std::vector<double>& knotVector, double parameterT, int splineDegree, size_t controlPointCount);
    static void insertKnotBoehm(RS_SplineData& splineData, double parameterT, double tolerance = RS_TOLERANCE);  // Inserts and refines controls
    static bool removeKnotBoehm(RS_SplineData& splineData, size_t knotIndexToRemove, double tolerance = RS_TOLERANCE);  // Removable if error < tol

    // Fallback non-uniform edits (heuristic for simple cases)
    static void insertKnotNonUniform(std::vector<double>& knotVector, size_t insertIndex, double newKnotValue, size_t splineOrder);
    static void removeKnotNonUniform(std::vector<double>& knotVector, size_t removeIndex, size_t splineOrder);

    // Generators
    static std::vector<double> knot(size_t controlPointCount, size_t splineOrder);  // Clamped uniform
    static std::vector<double> openUniformKnot(size_t controlPointCount, size_t splineOrder);  // Standard uniform
    static std::vector<double> computeAveragedKnots(const std::vector<double>& parameters, int degree);  // Averaged knots for interpolation

private:
};

#endif // LC_SPLINEHELPER_H
