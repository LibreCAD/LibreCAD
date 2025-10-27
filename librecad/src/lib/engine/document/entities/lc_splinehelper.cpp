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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>

#include "rs_debug.h"
#include "rs_math.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"

/**
 * Convert a closed wrapped knot vector to an open clamped knot vector.
 * Extracts internal knots and adds clamping multiplicities.
 */
std::vector<double> LC_SplineHelper::convertClosedToOpenKnotVector(const std::vector<double>& closedKnotVector, size_t unwrappedControlCount, size_t splineDegree) {
    const size_t n = unwrappedControlCount;
    const size_t m = splineDegree;
    if (closedKnotVector.size() < n + 2 * m + 1
        || n < m + 1) {
        return getNormalizedKnotVector(knot(m, m + 1), 0., {});
    }
    std::vector<double> openKnotVector(n + m + 1, 0.);
    std::copy(closedKnotVector.begin() + m, closedKnotVector.begin() + n + m + 1, openKnotVector.begin() + m);
    double delta = (closedKnotVector[n + m] - closedKnotVector[m - 1])/n;
    double knot= closedKnotVector[m - 1] - (m - 1) * delta;
    for(size_t i=0; i < m ; ++i, knot += delta)
        openKnotVector[i] = knot;
    double deltaEnd = closedKnotVector.back() - closedKnotVector[closedKnotVector.size() - 1 - m];
    for(size_t i=0; i < m ; ++i)
        openKnotVector[n + i] = closedKnotVector[n + m + i] - deltaEnd;
    return getNormalizedKnotVector(openKnotVector, 0.0, {});
}

/**
 * Convert open knot vector to closed (periodic) form.
 */
std::vector<double> LC_SplineHelper::convertOpenToClosedKnotVector(const std::vector<double>& openKnots, size_t n, size_t m) {
    if (openKnots.size() <= n)
        return {};

    // Detect if clamped using tolerance
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
            if (std::abs(openKnots[openKnots.size() - i - 1] - endValue) > RS_TOLERANCE) {
                isEndClamped = false;
                break;
            }
        }

        isClamped = isStartClamped && isEndClamped;
    }

    double period = openKnots.back() - openKnots.front();
    if (period <= 0.0) {
        // Invalid period; return empty vector
        return {};
    }

    size_t startIdx = isClamped ? m + 1 : 0;
    size_t lastIdx = n;
    if (startIdx + m > n || startIdx >= lastIdx) {
        // Not enough knots to append from startIdx
        return {};
    }
    std::vector<double> closedKnots(openKnots.begin() + startIdx, openKnots.begin() + lastIdx + 1);
    if (closedKnots.size() < 2)
        return {};
    const size_t newSize = n + 2 * m + 1;
    double current = closedKnots.back();
    size_t j = 1;
    while (closedKnots.size() < newSize) {
        double delta = (j < closedKnots.size()) ? closedKnots[j] - closedKnots[j - 1] : 1.0;
        current += delta;
        closedKnots.push_back(current);
        j++;
    }

    return closedKnots;
}

/**
 * Normalize knot vector by shifting to newMinimum (no scaling).
 */
std::vector<double> LC_SplineHelper::getNormalizedKnotVector(const std::vector<double>& inputKnotVector,
                                                             double newMinimum,
                                                             const std::vector<double>& fallbackKnotVector) {
    if (inputKnotVector.size() < 2) return fallbackKnotVector;
    double minKnot = *std::min_element(inputKnotVector.begin(), inputKnotVector.end());
    std::vector<double> shiftedKnotVector(inputKnotVector.size());
    std::transform(inputKnotVector.begin(), inputKnotVector.end(), shiftedKnotVector.begin(),
                   [minKnot, newMinimum](double knotValue) { return newMinimum + (knotValue - minKnot); });
    return shiftedKnotVector;
}

/**
 * Clamp a knot vector by adding endpoint multiplicity using internal values.
 */
std::vector<double> LC_SplineHelper::clampKnotVector(const std::vector<double>& inputKnotVector, size_t controlPointCount, size_t splineOrder) {
    if (inputKnotVector.size() != controlPointCount + splineOrder) return inputKnotVector;
    std::vector<double> clampedKnotVector = inputKnotVector;
    size_t splineDegree = splineOrder - 1;
    double leftClampValue = inputKnotVector[splineDegree];
    std::fill(clampedKnotVector.begin(), clampedKnotVector.begin() + splineOrder, leftClampValue);
    double rightClampValue = inputKnotVector[controlPointCount];
    std::fill(clampedKnotVector.begin() + controlPointCount + 1, clampedKnotVector.end(), rightClampValue);
    return clampedKnotVector;
}

/**
 * Unclamp a knot vector by removing endpoint multiplicity using adjacent spacing.
 */
std::vector<double> LC_SplineHelper::unclampKnotVector(const std::vector<double>& inputKnotVector, size_t controlPointCount, size_t splineOrder) {
    if (inputKnotVector.size() != controlPointCount + splineOrder)
        return inputKnotVector;
    std::vector<double> unclampedKnotVector = inputKnotVector;
    size_t m = splineOrder - 1;
    size_t iRight = inputKnotVector.size() - m - 1;
    if (iRight <= m)
        return inputKnotVector;
    double delta = (inputKnotVector[iRight] - inputKnotVector[m]) / (iRight - m);
    for (size_t i = 0; i < m; ++i) {
        unclampedKnotVector[i] = unclampedKnotVector[m] - delta * (m - i);
        unclampedKnotVector[inputKnotVector.size() - m + i] = unclampedKnotVector[iRight] + delta * (i + 1);
    }
    return unclampedKnotVector;
}

/**
 * Convert from standard (open uniform) to clamped open, saving knots.
 */
void LC_SplineHelper::toClampedOpenFromStandard(RS_SplineData& splineData) {
    assert(splineData.type == RS_SplineData::SplineType::Standard);
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
    splineData.savedOpenKnots = splineData.knotslist;
    splineData.knotslist = clampKnotVector(splineData.knotslist, splineData.controlPoints.size(), splineData.degree + 1);
}

/**
 * Convert from clamped open to standard, restoring knots.
 */
void LC_SplineHelper::toStandardFromClampedOpen(RS_SplineData& splineData) {
    assert(splineData.type == RS_SplineData::SplineType::ClampedOpen);
    splineData.type = RS_SplineData::SplineType::Standard;
    splineData.knotslist = unclampKnotVector(splineData.knotslist, splineData.controlPoints.size(), splineData.degree + 1);
    if (!splineData.savedOpenKnots.empty()) {
        splineData.knotslist = splineData.savedOpenKnots;
        splineData.savedOpenKnots.clear();
    }
}

/**
 * Convert from clamped open to wrapped closed.
 */
void LC_SplineHelper::toWrappedClosedFromClampedOpen(RS_SplineData& splineData) {
    toStandardFromClampedOpen(splineData);
    toWrappedClosedFromStandard(splineData);
}

/**
 * Convert from wrapped closed to clamped open.
 */
void LC_SplineHelper::toClampedOpenFromWrappedClosed(RS_SplineData& splineData) {
    toStandardFromWrappedClosed(splineData);
    toClampedOpenFromStandard(splineData);
}

/**
 * Convert from standard to wrapped closed.
 */
void LC_SplineHelper::toWrappedClosedFromStandard(RS_SplineData& splineData) {
    assert(splineData.type == RS_SplineData::SplineType::Standard);
    splineData.savedOpenType = RS_SplineData::SplineType::Standard;
    splineData.savedOpenKnots = splineData.knotslist;
    splineData.knotslist = clampKnotVector(splineData.knotslist, splineData.controlPoints.size(), splineData.degree + 1);
    splineData.type = RS_SplineData::SplineType::WrappedClosed;
    addWrapping(splineData);
}

/**
 * Convert from wrapped closed to standard.
 */
void LC_SplineHelper::toStandardFromWrappedClosed(RS_SplineData& splineData) {
    assert(splineData.type == RS_SplineData::SplineType::WrappedClosed);
    removeWrapping(splineData);
    splineData.type = RS_SplineData::SplineType::Standard;
    splineData.knotslist = unclampKnotVector(splineData.knotslist, splineData.controlPoints.size(), splineData.degree + 1);
    if (!splineData.savedOpenKnots.empty() && splineData.savedOpenType == RS_SplineData::SplineType::Standard) {
        splineData.knotslist = splineData.savedOpenKnots;
        splineData.savedOpenKnots.clear();
    }
}

/**
 * Add wrapping to control points and weights for closed splines.
 */
void LC_SplineHelper::addWrapping(RS_SplineData& splineData) {
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
    splineData.knotslist = convertOpenToClosedKnotVector(splineData.knotslist, n, m);
    splineData.knotslist = getNormalizedKnotVector(splineData.knotslist, 0., {});
}

/**
 * Remove wrapping from control points, weights, and knots.
 */
void LC_SplineHelper::removeWrapping(RS_SplineData& splineData) {
    const size_t n = splineData.controlPoints.size();
    const size_t m = splineData.degree;
    if (n <= m + 1)
        return;
    size_t unwrappedControlCount = n - m;
    splineData.controlPoints.resize(unwrappedControlCount);
    splineData.weights.resize(unwrappedControlCount);
    splineData.knotslist = convertClosedToOpenKnotVector(splineData.knotslist, unwrappedControlCount, splineData.degree);
}

/**
 * Update wrapping for control points and weights.
 */
void LC_SplineHelper::updateControlAndWeightWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount) {
    if (!isClosed) return;
    size_t degree = splineData.degree;
    for (size_t i = 0; i < degree; ++i) {
        splineData.controlPoints[unwrappedControlCount + i] = splineData.controlPoints[i];
        splineData.weights[unwrappedControlCount + i] = splineData.weights[i];
    }
}

/**
 * Update knot vector wrapping for closed splines.
 */
void LC_SplineHelper::updateKnotWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount) {
    if (!isClosed) return;
    splineData.knotslist = convertOpenToClosedKnotVector(splineData.knotslist, unwrappedControlCount, splineData.degree);
}

/**
 * Generate clamped uniform knot vector.
 */
std::vector<double> LC_SplineHelper::knot(size_t controlPointCount, size_t splineOrder) {
    std::vector<double> clampedKnotVector(controlPointCount + splineOrder, 0.0);
    size_t splineDegree = splineOrder - 1;
    for (size_t internalIndex = 0; internalIndex < controlPointCount - splineDegree; ++internalIndex) clampedKnotVector[splineOrder + internalIndex] = static_cast<double>(internalIndex + 1);
    std::fill(clampedKnotVector.begin() + controlPointCount + 1, clampedKnotVector.end(), static_cast<double>(controlPointCount - splineDegree));
    return clampedKnotVector;
}

/**
 * Generate open uniform knot vector.
 */
std::vector<double> LC_SplineHelper::generateOpenUniformKnotVector(size_t controlPointCount, size_t splineOrder) {
    std::vector<double> openKnotVector(controlPointCount + splineOrder);
    std::iota(openKnotVector.begin(), openKnotVector.end(), 0.0);
    return openKnotVector;
}

/**
 * Extend knot vector for appending a control point.
 */
void LC_SplineHelper::extendKnotVector(std::vector<double>& knots) {
    double delta = RS_TOLERANCE * 10;
    if (knots.size() >= 2) {
        delta = knots.back() - knots[knots.size() - 2];
    }
    double new_knot = knots.back() + std::max(delta, RS_TOLERANCE);
    knots.push_back(new_knot);
}

/**
 * Insert a knot at the specified index.
 */
void LC_SplineHelper::insertKnot(std::vector<double>& knots, size_t knot_index) {
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
        double delta = knots.size() >= 2 ? knots.back() - knots[knots.size() - 2] : 1.0;
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
void LC_SplineHelper::removeKnot(std::vector<double>& knots, size_t knot_index) {
    if (knot_index >= knots.size()) {
        return;
    }
    knots.erase(knots.begin() + knot_index);
}

/**
 * Ensure the knot vector is strictly monotonic.
 */
void LC_SplineHelper::ensureMonotonic(std::vector<double>& knots) {
    for (size_t i = 1; i < knots.size(); ++i) {
        if (knots[i] < knots[i - 1] + RS_TOLERANCE) {
            knots[i] = knots[i - 1] + RS_TOLERANCE * 10;
        }
    }
}
/**
 * Inserts a knot using Boehm's algorithm, refining the spline without changing its shape.
 * Updates control points, weights, and knot vector.
 *
 * @param data The spline data to modify (in/out).
 * @param t The parameter t where to insert the knot.
 * @throws std::invalid_argument If t out of domain or invalid data.
 */
/**
 * Inserts a knot (with multiplicity mult=1 by default) using Boehm's algorithm,
 * refining the spline without changing its shape. Updates controls, weights, and knots.
 *
 * @param data The spline data to modify (in/out).
 * @param t The parameter t where to insert the knot.
 * @param mult The multiplicity to insert (default 1, max degree).
 * @throws std::invalid_argument If t out of domain, mult invalid, or state invalid.
 */
void LC_SplineHelper::insertKnotBoehm(RS_SplineData& data, double t, int mult) {
    if (mult < 1 || mult > static_cast<int>(data.degree)) {
        throw std::invalid_argument("Invalid multiplicity (1 to degree)");
    }

    size_t n = data.controlPoints.size() - 1;  // Highest control index
    size_t p = data.degree;
    auto& U = data.knotslist;
    auto& P = data.controlPoints;
    auto& W = data.weights;

    // Find insertion span
    int k = findSpan(n, p, t, U);
    int s = 0;  // Existing multiplicity at t
    for (int i = k; i > 0 && fabs(U[i] - t) < RS_TOLERANCE; --i) ++s;
    if (t < U[p] || t > U[n + 1]) {
        throw std::invalid_argument("Insertion parameter t out of domain");
    }
    if (s + mult > static_cast<int>(p)) {
        throw std::invalid_argument("Total multiplicity exceeds degree");
    }

    // Allocate new arrays (grow by mult)
    std::vector<RS_Vector> Q(n + mult + 1);
    std::vector<double> Qw(n + mult + 1);
    std::vector<double> Uk(U.size() + mult);

    // Copy unchanged knots
    for (int i = 0; i <= k; ++i) Uk[i] = U[i];
    for (int i = k + 1; i < U.size(); ++i) Uk[i + mult] = U[i];
    // Insert t, mult times
    for (int i = 1; i <= mult; ++i) Uk[k + i] = t;

    // Boehm refinement: iterative convex combos
    // For rational: work in homogeneous coords (Pw = P * w, insert on Pw and w, dehomogenize Pw_new / w_new)
    std::vector<RS_Vector> Pw(n + 1);  // Homogeneous points
    for (size_t i = 0; i <= n; ++i) Pw[i] = P[i] * W[i];

    for (int r = 1; r <= mult; ++r) {  // For each insertion level
        int L = k - p + r;
        for (int j = k; j >= k - r + 1; --j) {
            double alpha = (t - U[j]) / (U[j + p - r + 1] - U[j]);
            Pw[j] = alpha * Pw[j] + (1.0 - alpha) * Pw[j - 1];
            W[j] = alpha * W[j] + (1.0 - alpha) * W[j - 1];
        }
        // Handle ends if needed (for s>0 existing mult, but simplified for new insert)
    }

    // Copy unchanged controls
    for (int i = 0; i <= k - p; ++i) {
        Q[i] = Pw[i] / W[i];  // Dehomogenize
        Qw[i] = W[i];
    }
    for (int i = k - s; i <= n; ++i) {
        Q[i + mult] = Pw[i] / W[i];
        Qw[i + mult] = W[i];
    }

    // Assign inserted (from temp Pw/W)
    for (int r = 0; r <= p - s - mult; ++r) {  // Unchanged middle
        Q[k - p + r] = Pw[k - p + r] / W[k - p + r];
        Qw[k - p + r] = W[k - p + r];
    }
    int L = k - p + mult;
    for (int r = 0; r <= s - 1; ++r) {  // Inserted segment
        Q[L + r] = Pw[k - s + r] / W[k - s + r];
        Qw[L + r] = W[k - s + r];
    }

    // Update data
    data.controlPoints = Q;
    data.weights = Qw;
    data.knotslist = Uk;

    ensureMonotonic(data.knotslist);
}

/**
 * Finds the knot span index i where U[i] <= u < U[i+1].
 * Uses binary search for efficiency O(log n).
 *
 * @param n Number of control points - 1 (highest index).
 * @param p Spline degree.
 * @param u Parameter value to find span for.
 * @param U Knot vector (non-decreasing).
 * @return The span index i.
 * @throws std::invalid_argument If U not monotonic or u out of [U[p], U[n+1]] (optional in improved).
 */
int LC_SplineHelper::findSpan(int n, int p, double u, const std::vector<double>& U) {
    // Edge cases: u at start or end
    if (u <= U[p] + RS_TOLERANCE) return p;  // First span (clamped start)
    if (u >= U[n + 1] - RS_TOLERANCE) return n;  // Last span (clamped end)

    // Binary search
    int low = p;
    int high = n + 1;
    int mid = (low + high) / 2;

    while (u < U[mid] || u >= U[mid + 1]) {
        if (u < U[mid]) {
            high = mid;
        } else {
            low = mid;
        }
        mid = (low + high) / 2;
    }

    return mid;
}
