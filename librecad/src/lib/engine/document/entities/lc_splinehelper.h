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

#include "rs_spline.h"
#include <cstddef>
#include <vector>

/**
 * @class LC_SplineHelper
 * @brief Helper class for spline type manipulations, knot conversions, and
 * wrapping.
 *
 * This namespace provides static utility functions for managing B-spline and
 * NURBS data in LibreCAD. It handles conversions between different spline
 * representations (e.g., open vs. closed, clamped vs. standard), knot vector
 * manipulations, wrapping for periodic splines, and validation. These helpers
 * ensure consistency and numerical stability when modifying spline properties.
 *
 * Key features:
 * - Knot vector normalization, clamping, and unclamping.
 * - Type-safe conversions between spline types without data loss.
 * - Wrapping/unwrapping for closed (periodic) splines.
 * - Monotonicity enforcement and basic validation.
 *
 * Usage: Call static functions directly on RS_SplineData instances.
 * Note: All functions assume RS_TOLERANCE for floating-point comparisons.
 */
namespace LC_SplineHelper {

// ============================================================================
// Knot Vector Conversions
// ============================================================================

/**
 * @brief Convert a closed wrapped knot vector to an open clamped knot vector.
 *
 * Extracts internal knots from a periodic (closed) knot vector and adds
 * clamping multiplicities at endpoints to create an open clamped form.
 * This is useful for converting closed splines to open representations.
 *
 * @param closedKnotVector Input knot vector from a wrapped closed spline.
 * @param unwrappedControlCount Number of unique (unwrapped) control points.
 * @param splineDegree Degree of the spline (e.g., 3 for cubic).
 * @return Clamped open knot vector, or empty if invalid input sizes.
 */
std::vector<double>
convertClosedToOpenKnotVector(const std::vector<double> &closedKnotVector,
                              size_t unwrappedControlCount,
                              size_t splineDegree);

/**
 * @brief Convert an open knot vector to a closed (periodic) form.
 *
 * Wraps the knot vector for periodic splines by repeating patterns and
 * extending with deltas. Handles both clamped and non-clamped inputs.
 *
 * @param openKnots Input open knot vector.
 * @param n Number of control points.
 * @param m Spline order (degree + 1).
 * @return Closed knot vector, or empty if period <= 0 or invalid sizes.
 */
std::vector<double>
convertOpenToClosedKnotVector(const std::vector<double> &openKnots, size_t n,
                              size_t splineDegree);

/**
 * @brief Normalize knot vector by shifting to a new minimum (no scaling).
 *
 * Shifts all knots so the smallest knot becomes @p newMinimum. Useful for
 * standardizing parameter ranges without altering relative spacing.
 *
 * @param inputKnotVector Input knot vector to normalize.
 * @param newMinimum Desired minimum knot value (default 0.0).
 * @param fallbackKnotVector Fallback if input size < 2.
 * @return Shifted knot vector.
 */
std::vector<double>
getNormalizedKnotVector(const std::vector<double> &inputKnotVector,
                        double newMinimum,
                        const std::vector<double> &fallbackKnotVector);

/**
 * @brief Unclamp a knot vector by removing endpoint multiplicity.
 *
 * Reverses clamping by extrapolating endpoint knots using internal deltas.
 * Assumes input is a valid clamped vector of correct size.
 *
 * @param inputKnotVector Clamped input knot vector.
 * @param controlPointCount Number of control points.
 * @param splineOrder Spline order (degree + 1).
 * @return Unclamped knot vector, unchanged if size mismatch.
 */
std::vector<double>
unclampKnotVector(const std::vector<double> &inputKnotVector,
                  size_t controlPointCount, size_t splineOrder);

/**
 * @brief Clamp a knot vector by adding endpoint multiplicity.
 *
 * Adds multiple knots at start/end using internal knot values for interpolation
 * at endpoints. Assumes input is unclamped and of correct size.
 *
 * @param inputKnotVector Unclamped input knot vector.
 * @param controlPointCount Number of control points.
 * @param splineOrder Spline order (degree + 1).
 * @return Clamped knot vector, unchanged if size mismatch.
 */
std::vector<double>
clampKnotVector(const std::vector<double> &inputKnotVector,
                size_t controlPointCount, size_t splineOrder);

       // ============================================================================
       // Spline Type Conversions
       // ============================================================================

/**
 * @brief Convert from Standard to ClampedOpen.
 *
 * Clamps the knot vector while preserving control points and weights.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toClampedOpenFromStandard(RS_SplineData &splineData);

/**
 * @brief Convert from ClampedOpen to Standard.
 *
 * Unclamps the knot vector while preserving control points and weights.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toStandardFromClampedOpen(RS_SplineData &splineData);

/**
 * @brief Convert from Standard to WrappedClosed.
 *
 * Adds wrapping to control points/weights and converts knots to periodic form.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toWrappedClosedFromStandard(RS_SplineData &splineData);

/**
 * @brief Convert from WrappedClosed to Standard.
 *
 * Removes wrapping from control points/weights and converts knots to open form.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toStandardFromWrappedClosed(RS_SplineData &splineData);

/**
 * @brief Convert from WrappedClosed to ClampedOpen via Standard.
 *
 * Two-step conversion: first to Standard, then to ClampedOpen.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toClampedOpenFromWrappedClosed(RS_SplineData &splineData);

/**
 * @brief Convert from ClampedOpen to WrappedClosed via Standard.
 *
 * Two-step conversion: first to Standard, then to WrappedClosed.
 * Validates pre/post integrity to avoid corruption.
 *
 * @param splineData RS_SplineData to modify.
 */
void toWrappedClosedFromClampedOpen(RS_SplineData &splineData);

       // ============================================================================
       // Wrapping Utilities
       // ============================================================================

/**
 * @brief Add wrapping to control points and weights for closed splines.
 *
 * Duplicates the first @p degree control points/weights at the end for
 * periodic continuity. Updates knots if necessary.
 *
 * @param splineData RS_SplineData to wrap.
 */
void addWrapping(RS_SplineData &splineData);

/**
 * @brief Remove wrapping from control points, weights, and knots.
 *
 * Truncates duplicated control points/weights and converts knots to open form.
 *
 * @param splineData RS_SplineData to unwrap.
 */
void removeWrapping(RS_SplineData &splineData);

/**
 * @brief Update wrapping for control points and weights.
 *
 * Ensures the wrapped section matches the unwrapped for closed splines.
 *
 * @param splineData RS_SplineData to update.
 * @param isClosed Whether the spline is closed (wrapped).
 * @param unwrappedControlCount Number of unique control points.
 */
void updateControlAndWeightWrapping(RS_SplineData &splineData,
                                    bool isClosed,
                                    size_t unwrappedControlCount);

/**
 * @brief Update knot vector wrapping for closed splines.
 *
 * Regenerates the periodic knot vector from the open form.
 *
 * @param splineData RS_SplineData to update.
 * @param isClosed Whether the spline is closed (wrapped).
 * @param unwrappedControlCount Number of unique control points.
 */
void updateKnotWrapping(RS_SplineData &splineData, bool isClosed,
                        size_t unwrappedControlCount);

       // ============================================================================
       // Knot Vector Generators
       // ============================================================================

/**
 * @brief Generate a clamped uniform knot vector.
 *
 * Creates knots with multiplicity at endpoints and uniform spacing internally.
 * Suitable for open interpolating splines.
 *
 * @param controlPointCount Number of control points.
 * @param splineOrder Spline order (degree + 1).
 * @return Clamped uniform knot vector.
 */
std::vector<double> knot(size_t controlPointCount,
                         size_t splineOrder); // Clamped uniform

/**
 * @brief Generate an open uniform knot vector without endpoint multiplicity.
 *
 * Simple uniform spacing from 0 to n, no clamping. For standard open splines.
 *
 * @param controlPointCount Number of control points.
 * @param splineOrder Spline order (degree + 1).
 * @return Open uniform knot vector.
 */
std::vector<double>
generateOpenUniformKnotVector(size_t controlPointCount, size_t splineOrder);

       // ============================================================================
       // Knot Manipulation
       // ============================================================================

/**
 * @brief Extend knot vector by appending a new knot.
 *
 * Adds a knot at the end using the last delta or a default step.
 *
 * @param knots Knot vector to extend.
 */
void extendKnotVector(std::vector<double> &knots);

/**
 * @brief Insert a knot at the specified index.
 *
 * Computes a suitable value (midpoint or extrapolated) and inserts it.
 * Clamps index to valid range.
 *
 * @param knots Knot vector to modify.
 * @param knot_index Insertion index (0 to size inclusive).
 */
void insertKnot(std::vector<double> &knots, size_t knot_index);

/**
 * @brief Remove a knot at the specified index.
 *
 * Erases the knot if index is valid; no-op otherwise.
 *
 * @param knots Knot vector to modify.
 * @param knot_index Index to remove.
 */
void removeKnot(std::vector<double> &knots, size_t knot_index);

/**
 * @brief Ensure the knot vector is strictly monotonic.
 *
 * Adjusts non-increasing knots by adding a small tolerance step.
 *
 * @param knots Knot vector to enforce monotonicity on.
 */
void ensureMonotonic(std::vector<double> &knots);

       // ============================================================================
       // Validation
       // ============================================================================

/**
 * @brief Validate the spline data integrity.
 *
 * Checks degree range, size consistency, monotonic knots, clamping (if applicable),
 * and wrapping equality (if closed). Uses RS_TOLERANCE for comparisons.
 *
 * @param data RS_SplineData to validate.
 * @return True if valid, false otherwise.
 */
bool validate(const RS_SplineData &data);

}; // namespace LC_SplineHelper

#endif // LC_SPLINEHELPER_H
