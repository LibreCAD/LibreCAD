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
    static std::vector<double> convertClosedToOpenKnotVector(const std::vector<double>& closedKnots,
                                                             size_t unwrappedSize,
                                                             size_t degree);
    static std::vector<double> getNormalizedKnotVector(const std::vector<double>& knots,
                                                       double newMin,
                                                       double newMax,
                                                       const std::vector<double>& fallback);
    static std::vector<double> unclampKnotVector(const std::vector<double>& knots,
                                                 size_t numControl,
                                                 size_t order);
    static std::vector<double> clampKnotVector(const std::vector<double>& knots,
                                               size_t numControl,
                                               size_t order);

    // Type conversions (non-uniform preserving)
    static void toClampedOpenFromStandard(RS_SplineData& data, size_t unwrappedSize);
    static void toStandardFromClampedOpen(RS_SplineData& data, size_t unwrappedSize);
    static void toWrappedClosedFromClampedOpen(RS_SplineData& data, size_t unwrappedSize);
    static void toClampedOpenFromWrappedClosed(RS_SplineData& data, size_t& unwrappedSize);
    static void toWrappedClosedFromStandard(RS_SplineData& data, size_t unwrappedSize);
    static void toStandardFromWrappedClosed(RS_SplineData& data, size_t unwrappedSize);

    // Wrapping
    static void addWrapping(RS_SplineData& data, bool isClosed);
    static void removeWrapping(RS_SplineData& data, bool isClosed, size_t& unwrappedSize);
    static void updateControlAndWeightWrapping(RS_SplineData& data, bool isClosed, size_t unwrappedSize);
    static void updateKnotWrapping(RS_SplineData& data, bool isClosed, size_t unwrappedSize);

    // Validation
    static bool validate(const RS_SplineData& data, size_t unwrappedSize);
    static bool isCustomKnotVector(const std::vector<double>& knots, size_t numCtrl, size_t order);  // Non-uniform detection

    // Boehm's algorithms (non-uniform)
    static int findSpan(const std::vector<double>& knots, double t, int degree, size_t numCtrl);
    static void insertKnotBoehm(RS_SplineData& data, double t, double tol = RS_TOLERANCE);  // Inserts and refines controls
    static bool removeKnotBoehm(RS_SplineData& data, size_t r, double tol = RS_TOLERANCE);  // Removable if error < tol

    // Fallback non-uniform edits (heuristic for simple cases)
    static void insertKnotNonUniform(std::vector<double>& knots, size_t index, double newKnot, size_t order);
    static void removeKnotNonUniform(std::vector<double>& knots, size_t index, size_t order);

    // Generators
    static std::vector<double> knot(size_t num, size_t order);  // Clamped uniform
    static std::vector<double> openUniformKnot(size_t num, size_t order);  // Standard uniform

private:
};

#endif // LC_SPLINEHELPER_H
