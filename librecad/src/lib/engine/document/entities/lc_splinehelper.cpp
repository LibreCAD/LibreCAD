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
// File: lc_splinehelper.cpp

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_vector.h"

/**
 * Convert a closed wrapped knot vector to an open clamped knot vector.
 * Extracts internal knots and adds clamping multiplicities.
 */
std::vector<double> LC_SplineHelper::convertClosedToOpenKnotVector(
    const std::vector<double> &closedKnotVector, size_t unwrappedControlCount,
    size_t splineDegree) {
  const size_t n = unwrappedControlCount;
  const size_t m = splineDegree;
  if (closedKnotVector.size() < n + m + 1 || n < m + 1) {
    return {};
  }
  std::vector<double> openKnotVector(n + m + 1, 0.);
  std::copy(closedKnotVector.begin() + m, closedKnotVector.begin() + n + m + 1,
            openKnotVector.begin() + m);
  double delta = (closedKnotVector[n + m] - closedKnotVector[m]) / n;
  if (std::abs(delta) < RS_TOLERANCE) {
    delta = 1.0; // Default spacing for constant knots
  }
  double knot = closedKnotVector[m] - m * delta;
  for (size_t i = 0; i < m; ++i, knot += delta) {
    openKnotVector[i] = knot;
  }
  double deltaEnd = closedKnotVector.back() -
                    closedKnotVector[closedKnotVector.size() - 1 - m];
  if (std::abs(deltaEnd) < RS_TOLERANCE) {
    deltaEnd = 0.0; // Handle constant end
  }
  for (size_t i = 0; i <= m; ++i) {
    openKnotVector[n + i] = closedKnotVector[n + m + i] - deltaEnd;
  }
  return getNormalizedKnotVector(openKnotVector, 0.0, {});
}

/**
 * Convert open knot vector to closed (periodic) form.
 */
std::vector<double> LC_SplineHelper::convertOpenToClosedKnotVector(
    const std::vector<double> &openKnots, size_t n, size_t m) {
  if (openKnots.size() <= n)
    return {};

  bool isClamped = false;
  if (openKnots.size() >= 2 * (m + 1)) {
    double startValue = openKnots[0];
    bool isStartClamped = true;
    for (size_t i = 1; i <= m; ++i) {
      if (std::abs(openKnots[i] - startValue) > RS_TOLERANCE) {
        isStartClamped = false;
        break;
      }
    }

    double endValue = openKnots.back();
    bool isEndClamped = true;
    for (size_t i = 1; i <= m; ++i) {
      if (std::abs(openKnots[openKnots.size() - i - 1] - endValue) >
          RS_TOLERANCE) {
        isEndClamped = false;
        break;
      }
    }

    isClamped = isStartClamped && isEndClamped;
  }

  double period = openKnots.back() - openKnots.front();
  if (period <= 0.0) {
    return {};
  }

  size_t startIdx = isClamped ? m + 1 : 0;
  size_t lastIdx = n;
  if (startIdx > lastIdx) {
    return {};
  }
  std::vector<double> closedKnots(openKnots.begin() + startIdx,
                                  openKnots.begin() + lastIdx + 1);
  if (closedKnots.empty())
    return {};

  size_t initial_size = closedKnots.size();
  const size_t newSize = n + 2 * m + 1;
  double current = closedKnots.back();
  size_t j = 1;
  while (closedKnots.size() < newSize) {
    double delta;
    if (initial_size <= 1) {
      delta = 0.0;
    } else {
      size_t jj = (j - 1) % (initial_size - 1) + 1;
      delta = closedKnots[jj] - closedKnots[jj - 1];
    }
    current += delta;
    closedKnots.push_back(current);
    j++;
  }

  return closedKnots;
}

/**
 * Normalize knot vector by shifting to newMinimum (no scaling).
 */
std::vector<double> LC_SplineHelper::getNormalizedKnotVector(
    const std::vector<double> &inputKnotVector, double newMinimum,
    const std::vector<double> &fallbackKnotVector) {
  if (inputKnotVector.size() < 2)
    return fallbackKnotVector;
  double minKnot =
      *std::min_element(inputKnotVector.begin(), inputKnotVector.end());
  std::vector<double> shiftedKnotVector(inputKnotVector.size());
  std::transform(inputKnotVector.begin(), inputKnotVector.end(),
                 shiftedKnotVector.begin(),
                 [minKnot, newMinimum](double knotValue) {
                   return newMinimum + (knotValue - minKnot);
                 });
  return shiftedKnotVector;
}

/**
 * Clamp a knot vector by adding endpoint multiplicity using internal values.
 */
std::vector<double>
LC_SplineHelper::clampKnotVector(const std::vector<double> &inputKnotVector,
                                 size_t controlPointCount, size_t splineOrder) {
  if (inputKnotVector.size() != controlPointCount + splineOrder)
    return inputKnotVector;
  std::vector<double> clampedKnotVector = inputKnotVector;
  size_t splineDegree = splineOrder - 1;
  double leftClampValue = inputKnotVector[splineDegree];
  std::fill(clampedKnotVector.begin(),
            clampedKnotVector.begin() + splineDegree + 1, leftClampValue);
  double rightClampValue = inputKnotVector[controlPointCount];
  std::fill(clampedKnotVector.end() - (splineDegree + 1),
            clampedKnotVector.end(), rightClampValue);
  return clampedKnotVector;
}

/**
 * Unclamp a knot vector by removing endpoint multiplicity using internal
 * deltas.
 */
std::vector<double>
LC_SplineHelper::unclampKnotVector(const std::vector<double> &inputKnotVector,
                                   size_t controlPointCount,
                                   size_t splineOrder) {
  if (inputKnotVector.size() != controlPointCount + splineOrder)
    return inputKnotVector;
  std::vector<double> unclampedKnotVector = inputKnotVector;
  size_t splineDegree = splineOrder - 1;
  double leftDelta =
      (inputKnotVector[splineDegree + 1] - inputKnotVector[splineDegree]);
  if (std::abs(leftDelta) < RS_TOLERANCE)
    leftDelta = 1.0;
  double current = inputKnotVector[splineDegree];
  for (size_t i = 1; i <= splineDegree; ++i) {
    current -= leftDelta;
    unclampedKnotVector[splineDegree - i] = current;
  }
  double rightDelta = (inputKnotVector[controlPointCount] -
                       inputKnotVector[controlPointCount - 1]);
  if (std::abs(rightDelta) < RS_TOLERANCE)
    rightDelta = 1.0;
  current = inputKnotVector[controlPointCount];
  for (size_t i = 1; i <= splineDegree; ++i) {
    current += rightDelta;
    unclampedKnotVector[controlPointCount + i] = current;
  }
  return unclampedKnotVector;
}

/**
 * Convert from Standard to ClampedOpen.
 */
void LC_SplineHelper::toClampedOpenFromStandard(RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto originalKnots = splineData.knotslist;
  auto originalType = splineData.type;

  splineData.knotslist =
      clampKnotVector(splineData.knotslist, splineData.controlPoints.size(),
                      splineData.degree + 1);
  splineData.type = RS_SplineData::SplineType::ClampedOpen;

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.knotslist = originalKnots;
    splineData.type = originalType;
  }
}

/**
 * Convert from ClampedOpen to Standard.
 */
void LC_SplineHelper::toStandardFromClampedOpen(RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto originalKnots = splineData.knotslist;
  auto originalType = splineData.type;

  splineData.knotslist =
      unclampKnotVector(splineData.knotslist, splineData.controlPoints.size(),
                        splineData.degree + 1);
  splineData.type = RS_SplineData::SplineType::Standard;

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.knotslist = originalKnots;
    splineData.type = originalType;
  }
}

/**
 * Convert from Standard to WrappedClosed.
 */
void LC_SplineHelper::toWrappedClosedFromStandard(RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto savedKnots = splineData.knotslist;
  auto savedType = splineData.type;
  auto savedControls = splineData.controlPoints;
  auto savedWeights = splineData.weights;

  addWrapping(splineData);
  splineData.type = RS_SplineData::SplineType::WrappedClosed;

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.controlPoints = savedControls;
    splineData.weights = savedWeights;
    splineData.knotslist = savedKnots;
    splineData.type = savedType;
  }
}

/**
 * Convert from WrappedClosed to Standard.
 */
void LC_SplineHelper::toStandardFromWrappedClosed(RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto savedKnots = splineData.knotslist;
  auto savedType = splineData.type;
  auto savedControls = splineData.controlPoints;
  auto savedWeights = splineData.weights;

  removeWrapping(splineData);
  splineData.type = RS_SplineData::SplineType::Standard;

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.controlPoints = savedControls;
    splineData.weights = savedWeights;
    splineData.knotslist = savedKnots;
    splineData.type = savedType;
  }
}

/**
 * Convert from WrappedClosed to ClampedOpen via Standard.
 */
void LC_SplineHelper::toClampedOpenFromWrappedClosed(
    RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto savedKnots = splineData.knotslist;
  auto savedType = splineData.type;
  auto savedControls = splineData.controlPoints;
  auto savedWeights = splineData.weights;

  toStandardFromWrappedClosed(splineData);
  toClampedOpenFromStandard(splineData);

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.controlPoints = savedControls;
    splineData.weights = savedWeights;
    splineData.knotslist = savedKnots;
    splineData.type = savedType;
  }
}

/**
 * Convert from ClampedOpen to WrappedClosed via Standard.
 */
void LC_SplineHelper::toWrappedClosedFromClampedOpen(
    RS_SplineData &splineData) {
  // Pre-conversion validation: Full integrity check including sizes
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Pre-conversion validation failed: inconsistent sizes or integrity");
    return; // Skip conversion to preserve original state
  }

  auto savedKnots = splineData.knotslist;
  auto savedType = splineData.type;
  auto savedControls = splineData.controlPoints;
  auto savedWeights = splineData.weights;

  toStandardFromClampedOpen(splineData);
  toWrappedClosedFromStandard(splineData);

  // Post-conversion validation: Ensure sizes and integrity after conversion
  if (!validate(splineData)) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "Post-conversion validation failed: inconsistent sizes or integrity");
    // Revert changes
    splineData.controlPoints = savedControls;
    splineData.weights = savedWeights;
    splineData.knotslist = savedKnots;
    splineData.type = savedType;
  }
}

/**
 * Add wrapping to control points and weights for closed splines.
 */
void LC_SplineHelper::addWrapping(RS_SplineData &splineData) {
  size_t n = splineData.controlPoints.size();
  size_t m = splineData.degree;
  if (n < m + 1)
    return;
  for (size_t i = 0; i < m; ++i) {
    splineData.controlPoints.push_back(splineData.controlPoints[i]);
    splineData.weights.push_back(splineData.weights[i]);
  }
  if (splineData.knotslist.size() != n + m + 1) {
    splineData.knotslist = knot(n, m + 1);
  }
  splineData.knotslist =
      convertOpenToClosedKnotVector(splineData.knotslist, n, m);
}

/**
 * Remove wrapping from control points, weights, and knots.
 */
void LC_SplineHelper::removeWrapping(RS_SplineData &splineData) {
  const size_t n = splineData.controlPoints.size();
  const size_t m = splineData.degree;
  if (n <= m + 1)
    return;
  size_t unwrappedControlCount = n - m;
  splineData.controlPoints.resize(unwrappedControlCount);
  splineData.weights.resize(unwrappedControlCount);
  splineData.knotslist = convertClosedToOpenKnotVector(
      splineData.knotslist, unwrappedControlCount, splineData.degree);
}

/**
 * Update wrapping for control points and weights.
 */
void LC_SplineHelper::updateControlAndWeightWrapping(
    RS_SplineData &splineData, bool isClosed, size_t unwrappedControlCount) {
  if (!isClosed)
    return;
  size_t degree = splineData.degree;
  for (size_t i = 0; i < degree; ++i) {
    splineData.controlPoints[unwrappedControlCount + i] =
        splineData.controlPoints[i];
    splineData.weights[unwrappedControlCount + i] = splineData.weights[i];
  }
}

/**
 * Update knot vector wrapping for closed splines.
 */
void LC_SplineHelper::updateKnotWrapping(RS_SplineData &splineData,
                                         bool isClosed,
                                         size_t unwrappedControlCount) {
  if (!isClosed)
    return;
  splineData.knotslist = convertOpenToClosedKnotVector(
      splineData.knotslist, unwrappedControlCount, splineData.degree);
}

/**
 * Generate clamped uniform knot vector.
 */
std::vector<double> LC_SplineHelper::knot(size_t controlPointCount,
                                          size_t splineOrder) {
  std::vector<double> clampedKnotVector(controlPointCount + splineOrder, 0.0);
  size_t splineDegree = splineOrder - 1;
  for (size_t internalIndex = 0;
       internalIndex < controlPointCount - splineDegree; ++internalIndex)
    clampedKnotVector[splineOrder + internalIndex] =
        static_cast<double>(internalIndex + 1);
  std::fill(clampedKnotVector.begin() + controlPointCount + 1,
            clampedKnotVector.end(),
            static_cast<double>(controlPointCount - splineDegree));
  return clampedKnotVector;
}

/**
 * Generate open uniform knot vector.
 */
std::vector<double>
LC_SplineHelper::generateOpenUniformKnotVector(size_t controlPointCount,
                                               size_t splineOrder) {
  std::vector<double> openKnotVector(controlPointCount + splineOrder);
  std::iota(openKnotVector.begin(), openKnotVector.end(), 0.0);
  return openKnotVector;
}

/**
 * Extend knot vector for appending a control point.
 */
void LC_SplineHelper::extendKnotVector(std::vector<double> &knots) {
  double delta = RS_TOLERANCE * 10;
  double last = 0.0;
  if (!knots.empty()) {
    last = knots.back();
    if (knots.size() >= 2) {
      double prev_delta = last - knots[knots.size() - 2];
      if (prev_delta > 0.0) {
        delta = prev_delta;
      }
    }
  }
  double new_knot = last + delta;
  knots.push_back(new_knot);
}
/**
 * Insert a knot at the specified index.
 */
void LC_SplineHelper::insertKnot(std::vector<double> &knots,
                                 size_t knot_index) {
  if (knot_index > knots.size()) {
    knot_index = knots.size();
  }
  double new_knot = RS_TOLERANCE * 10;
  if (knots.empty()) {
    new_knot = 0.0;
  } else if (knot_index == 0) {
    // Edge case: insert at start
    double delta = knots.size() >= 2 ? knots[1] - knots[0] : 1.0;
    new_knot = knots[0] - std::max(delta, RS_TOLERANCE);
  } else if (knot_index >= knots.size()) {
    // Edge case: insert at end (like add)
    double delta =
        knots.size() >= 2 ? knots.back() - knots[knots.size() - 2] : 1.0;
    new_knot = knots.back() + std::max(delta, RS_TOLERANCE);
  } else {
    // Mid: average of surrounding
    double left = knots[knot_index - 1];
    double right = knots[knot_index];
    new_knot = (left + right) / 2.0;
    if (right - left < 2 * RS_TOLERANCE) {
      new_knot = right + RS_TOLERANCE;
    }
  }
  knots.insert(knots.begin() + knot_index, new_knot);
}

/**
 * Remove a knot at the specified index.
 */
void LC_SplineHelper::removeKnot(std::vector<double> &knots,
                                 size_t knot_index) {
  if (knot_index >= knots.size()) {
    return;
  }
  knots.erase(knots.begin() + knot_index);
}

/**
 * Ensure the knot vector is strictly monotonic.
 */
void LC_SplineHelper::ensureMonotonic(std::vector<double> &knots) {
  for (size_t i = 1; i < knots.size(); ++i) {
    if (knots[i] < knots[i - 1] + RS_TOLERANCE) {
      knots[i] = knots[i - 1] + RS_TOLERANCE * 10;
    }
  }
}

/**
 * Validate the spline data integrity.
 */
bool LC_SplineHelper::validate(const RS_SplineData &data) {
  size_t deg = data.degree;
  if (deg < 1 || deg > 3)
    return false;

  size_t ctrlSz = data.controlPoints.size();
  if (data.weights.size() != ctrlSz)
    return false;

  bool closed = (data.type == RS_SplineData::SplineType::WrappedClosed);
  size_t uwSz = closed ? (ctrlSz > deg ? ctrlSz - deg : 0) : ctrlSz;
  if (uwSz < deg + 1)
    return false;

  if (ctrlSz != (closed ? uwSz + deg : uwSz) ||
      data.knotslist.size() != (closed ? uwSz + 2 * deg + 1 : uwSz + deg + 1))
    return false;

  for (size_t i = 1; i < data.knotslist.size(); ++i) {
    if (data.knotslist[i] < data.knotslist[i - 1] - RS_TOLERANCE)
      return false;
  }

  if (data.type == RS_SplineData::SplineType::ClampedOpen) {
    double start = data.knotslist[0], end = data.knotslist.back();
    for (size_t i = 1; i <= deg; ++i) {
      if (fabs(data.knotslist[i] - start) > RS_TOLERANCE ||
          fabs(data.knotslist[data.knotslist.size() - i - 1] - end) >
              RS_TOLERANCE)
        return false;
    }
  } else if (closed) {
    for (size_t i = 0; i < deg; ++i) {
      if ((data.controlPoints[uwSz + i] - data.controlPoints[i]).magnitude() >
              RS_TOLERANCE ||
          fabs(data.weights[uwSz + i] - data.weights[i]) > RS_TOLERANCE)
        return false;
    }
  }
  return true;
}
