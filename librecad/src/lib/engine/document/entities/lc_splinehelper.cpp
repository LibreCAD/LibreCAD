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
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>

#include "rs_debug.h"
#include "rs_math.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"

namespace {

// Homogeneous coordinates (x*w, y*w, w)
struct Homog {
    RS_Vector coord;
    double w;
    Homog() : coord(0,0), w(1.0) {}
    Homog(const RS_Vector& v, double ww) : coord(v * ww), w(ww) {}
    RS_Vector toPoint() const { return w > RS_TOLERANCE ? coord / w : RS_Vector(false); }
    Homog operator+(const Homog& other) const {
        return {coord + other.coord, w + other.w};
    }
    Homog operator-(const Homog& other) const {
        return {coord - other.coord, w - other.w};
    }

    Homog operator * (double scale) const {
        return {coord * scale, w * scale};
    }

    Homog operator / (double scale) const {
        if (std::abs(scale) < RS_TOLERANCE) {
            return *this;
        }
        return {coord / scale, w / scale};
    }
    double magnitude() const {
        return std::sqrt(coord.x * coord.x + coord.y * coord.y); // Only XY for error
    }
};

bool vectorsEqual(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() != b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](double x, double y) { return std::abs(x - y) < RS_TOLERANCE; });
}

/** Compute rational B-spline basis functions (copied for evaluate) */
std::vector<double> rbasis_helper(int c, double t, int npts,
                                  const std::vector<double>& x,
                                  const std::vector<double>& h) {

    int const nplusc = npts + c;

    std::vector<double> temp(nplusc,0.);

    // calculate the first order nonrational basis functions n[i]
    for (int i = 0; i< nplusc-1; i++)
        if ((t >= x[i]) && (t < x[i+1])) temp[i] = 1;

    /* calculate the higher order nonrational basis functions */

    for (int k = 2; k <= c; k++) {
        for (int i = 0; i < nplusc-k; i++) {
            // if the lower order basis function is zero skip the calculation
            if (temp[i] != 0)
                temp[i] = ((t-x[i])*temp[i])/(x[i+k-1]-x[i]);
            // if the lower order basis function is zero skip the calculation
            if (temp[i+1] != 0)
                temp[i] += ((x[i+k]-t)*temp[i+1])/(x[i+k]-x[i+1]);
        }
    }

    // pick up last point
    if (t >= x[nplusc-1]) temp[npts-1] = 1;

    // calculate sum for denominator of rational basis functions
    double sum = 0.;
    for (int i = 0; i < npts; i++) {
        sum += temp[i]*h[i];
    }

    std::vector<double> r(npts, 0);
    // form rational basis functions and put in r vector
    if (sum != 0) {
        for (int i = 0; i < npts; i++)
            r[i] = (temp[i]*h[i])/sum;
    }
    return r;
}

/** Evaluate NURBS at t using rbasis_helper. */
RS_Vector evaluateNURBS(const RS_SplineData& data, double t) {
    size_t numControlPoints = data.controlPoints.size();
    size_t splineOrder = data.degree + 1;
    auto knotVector = data.knotslist;
    auto weightVector = data.weights;
    auto basisFunctions = rbasis_helper(splineOrder, t, numControlPoints, knotVector, weightVector);
    RS_Vector point(0.0, 0.0);
    for (size_t basisIndex = 0; basisIndex < numControlPoints; basisIndex++) {
        point += data.controlPoints[basisIndex] * basisFunctions[basisIndex];
    }
    return point;
}

/** Convert open knot vector to closed (periodic) form. */
std::vector<double> convertOpenToClosedKnotVector(const std::vector<double>& openKnots, size_t n, size_t m) {
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
    double delta = period; // Use period for periodic extension
    const size_t newSize = n + 2 * m + 1;
    for (size_t i = 0; closedKnots.size() < newSize; ++i) {
        closedKnots.push_back(closedKnots[i] + delta);
    }

    return closedKnots;
}

}

/**
 * Convert a closed wrapped knot vector to an open clamped knot vector.
 * Extracts internal knots and adds clamping multiplicities.
 */
std::vector<double> LC_SplineHelper::convertClosedToOpenKnotVector(const std::vector<double>& closedKnotVector, size_t unwrappedControlCount, size_t splineDegree) {
    size_t splineOrder = splineDegree + 1;
    if (closedKnotVector.size() < unwrappedControlCount + 2 * splineDegree + 1) return {};
    std::vector<double> openKnotVector(splineOrder, closedKnotVector[splineDegree]);
    std::copy(closedKnotVector.begin() + splineOrder, closedKnotVector.begin() + unwrappedControlCount + 1, std::back_inserter(openKnotVector));
    double lastKnotValue = openKnotVector.back();
    std::fill_n(std::back_inserter(openKnotVector), splineOrder - 1, lastKnotValue);
    return getNormalizedKnotVector(openKnotVector, 0.0, {});
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
    if (inputKnotVector.size() != controlPointCount + splineOrder) return inputKnotVector;
    std::vector<double> unclampedKnotVector = inputKnotVector;
    size_t splineDegree = splineOrder - 1;
    double leftIntervalSpacing = inputKnotVector[splineDegree + 1] - inputKnotVector[splineDegree];
    unclampedKnotVector[0] = unclampedKnotVector[splineDegree] - leftIntervalSpacing;
    double rightIntervalSpacing = inputKnotVector[controlPointCount] - inputKnotVector[controlPointCount - 1];
    unclampedKnotVector.back() = unclampedKnotVector[controlPointCount] + rightIntervalSpacing;
    return unclampedKnotVector;
}

/**
 * Convert from standard (open uniform) to clamped open, saving knots.
 */
void LC_SplineHelper::toClampedOpenFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount) {
    if (splineData.type != RS_SplineData::SplineType::Standard) return;
    splineData.savedOpenKnots = splineData.knotslist;
    splineData.knotslist = knot(unwrappedControlCount, splineData.degree + 1);
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
}

/**
 * Convert from clamped open to standard, restoring knots.
 */
void LC_SplineHelper::toStandardFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount) {
    if (splineData.type != RS_SplineData::SplineType::ClampedOpen) return;
    if (splineData.savedOpenKnots.empty()) {
        splineData.knotslist = unclampKnotVector(splineData.knotslist, unwrappedControlCount, splineData.degree + 1);
        splineData.knotslist = getNormalizedKnotVector(splineData.knotslist, 0.0, {});
    } else {
        splineData.knotslist = splineData.savedOpenKnots;
        splineData.savedOpenKnots.clear();
    }
    splineData.type = RS_SplineData::SplineType::Standard;
}

/**
 * Convert from clamped open to wrapped closed.
 */
void LC_SplineHelper::toWrappedClosedFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount) {
    if (splineData.type == RS_SplineData::SplineType::WrappedClosed) return;
    splineData.savedOpenType = splineData.type;
    addWrapping(splineData, true);
    updateKnotWrapping(splineData, true, unwrappedControlCount);
    splineData.type = RS_SplineData::SplineType::WrappedClosed;
}

/**
 * Convert from wrapped closed to clamped open.
 */
void LC_SplineHelper::toClampedOpenFromWrappedClosed(RS_SplineData& splineData, size_t unwrappedControlCount) {
    if (splineData.type != RS_SplineData::SplineType::WrappedClosed) return;
    removeWrapping(splineData, true, unwrappedControlCount);
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
}

/**
 * Convert from standard to wrapped closed.
 */
void LC_SplineHelper::toWrappedClosedFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount) {
    toClampedOpenFromStandard(splineData, unwrappedControlCount);
    toWrappedClosedFromClampedOpen(splineData, unwrappedControlCount);
}

/**
 * Convert from wrapped closed to standard.
 */
void LC_SplineHelper::toStandardFromWrappedClosed(RS_SplineData& splineData, size_t unwrappedControlCount) {
    toClampedOpenFromWrappedClosed(splineData, unwrappedControlCount);
    toStandardFromClampedOpen(splineData, unwrappedControlCount);
}

/**
 * Add wrapping to control points and weights for closed splines.
 */
void LC_SplineHelper::addWrapping(RS_SplineData& splineData, bool isClosed) {
    if (!isClosed) return;
    size_t degree = splineData.degree;
    for (size_t i = 0; i < degree; ++i) {
        splineData.controlPoints.push_back(splineData.controlPoints[i]);
        splineData.weights.push_back(splineData.weights[i]);
    }
}

/**
 * Remove wrapping from control points, weights, and knots.
 */
void LC_SplineHelper::removeWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount) {
    if (!isClosed) return;
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
 * Validate spline data integrity.
 */
bool LC_SplineHelper::validate(const RS_SplineData& splineData, size_t unwrappedControlCount) {
    size_t order = splineData.degree + 1;
    if (splineData.degree < 1 || splineData.degree > 3) return false;
    size_t controlSize = splineData.controlPoints.size();
    if (splineData.type == RS_SplineData::SplineType::WrappedClosed) {
        if (controlSize != unwrappedControlCount + static_cast<size_t>(splineData.degree)) return false;
    } else {
        if (controlSize != unwrappedControlCount) return false;
    }
    if (splineData.weights.size() != controlSize) return false;
    if (splineData.knotslist.size() != controlSize + order) return false;
    for (size_t i = 1; i < splineData.knotslist.size(); ++i) {
        if (splineData.knotslist[i] < splineData.knotslist[i - 1] - RS_TOLERANCE) return false;
    }
    return true;
}

/**
 * Detect if knot vector is custom (non-uniform).
 */
bool LC_SplineHelper::isCustomKnotVector(const std::vector<double>& knotVector, size_t controlCount, size_t splineOrder) {
    auto uniformKnots = knot(controlCount, splineOrder);
    return !vectorsEqual(knotVector, uniformKnots);
}

/**
 * Find knot span for parameter t.
 */
int LC_SplineHelper::findSpan(const std::vector<double>& knotVector, double parameterT, int splineDegree, size_t controlPointCount) {
    int lowIndex = splineDegree;
    int highIndex = controlPointCount;
    int midIndex = (lowIndex + highIndex) / 2;
    while (parameterT < knotVector[midIndex] || parameterT >= knotVector[midIndex + 1]) {
        if (parameterT < knotVector[midIndex]) highIndex = midIndex;
        else lowIndex = midIndex;
        midIndex = (lowIndex + highIndex) / 2;
    }
    return midIndex;
}

/**
 * Insert knot using Boehm's algorithm.
 */
void LC_SplineHelper::insertKnotBoehm(RS_SplineData& splineData, double parameterT, double tol) {
    size_t p = splineData.degree;
    size_t np = splineData.controlPoints.size();
    size_t nk = splineData.knotslist.size();

    if (nk != np + p + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "insertKnotBoehm: Invalid size");
        return;
    }

    int k = findSpan(splineData.knotslist, parameterT, p, np);

    bool rational = std::any_of(splineData.weights.begin(), splineData.weights.end(),
                                [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });

    std::vector<Homog> Pw(np);
    for (size_t i = 0; i < np; ++i) {
        double ww = rational ? splineData.weights[i] : 1.0;
        Pw[i] = Homog(splineData.controlPoints[i], ww);
    }

    std::vector<Homog> Qw(np + 1);
    for (int i = 0; i <= k - static_cast<int>(p); ++i) Qw[i] = Pw[i];
    for (int i = k; i < static_cast<int>(np); ++i) Qw[i + 1] = Pw[i];

    for (int j = k; j > k - static_cast<int>(p); --j) {
        if (j < 0 || j >= static_cast<int>(np)) continue;
        double denom = splineData.knotslist[j + p] - splineData.knotslist[j];
        if (std::abs(denom) < tol) continue;
        double alpha = (parameterT - splineData.knotslist[j]) / denom;

        Qw[j] = Qw[j] * alpha + Qw[j - 1] * (1.0 - alpha);
    }

    splineData.controlPoints.resize(np + 1);
    splineData.weights.resize(np + 1);
    for (size_t i = 0; i < np + 1; ++i) {
        splineData.controlPoints[i] = Qw[i].toPoint();
        splineData.weights[i] = Qw[i].w;
    }

    std::vector<double> Uk;
    Uk.reserve(nk + 1);
    for (size_t i = 0; i <= static_cast<size_t>(k); ++i) Uk.push_back(splineData.knotslist[i]);
    Uk.push_back(parameterT);
    for (size_t i = k + 1; i < nk; ++i) Uk.push_back(splineData.knotslist[i]);
    splineData.knotslist = Uk;
}

/**
 * Remove knot using simplified Boehm's algorithm (NURBS Book A5.8 for s=1).
 * Checks removability via control point difference; no sampling for speed.
 */
bool LC_SplineHelper::removeKnotBoehm(RS_SplineData& splineData, size_t r, double tol) {
    size_t p = splineData.degree;
    size_t np = splineData.controlPoints.size();
    size_t nk = splineData.knotslist.size();

    if (nk != np + p + 1 || r >= nk || r < p || r > nk - p - 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Invalid index or size");
        return false;
    }

    if (np <= p + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Cannot remove from minimal spline");
        return false;
    }

    // Assume multiplicity s=1 for simplification
    int s = 1;
    double u = splineData.knotslist[r];

    bool rational = std::any_of(splineData.weights.begin(), splineData.weights.end(),
                                [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });

    std::vector<Homog> Pw(np);
    for (size_t i = 0; i < np; ++i) {
        double ww = rational ? splineData.weights[i] : 1.0;
        Pw[i] = Homog(splineData.controlPoints[i], ww);
    }

    int k = findSpan(splineData.knotslist, u, p, np);

    // Simplified Boehm removal
    int first = k - p;
    int last = k - s + 1;

    std::vector<Homog> temp(2 * (p - s) + 1);

    int off = first - 1;
    temp[0] = Pw[off];
    temp[last + s - off - 1] = Pw[last];

    int i = first;
    int j = last;
    int ii = 1;
    int jj = last + s - off - 2;

    int remflag = 0;
    while (j - i > remflag) {
        double alfi = (u - splineData.knotslist[i]) / (splineData.knotslist[i + p + 1 - remflag] - splineData.knotslist[i]);
        double alfj = (u - splineData.knotslist[j]) / (splineData.knotslist[j + p + 1 - remflag] - splineData.knotslist[j]);
        temp[ii] = (Pw[i] - temp[ii - 1] * (1.0 - alfi)) / alfi;
        temp[jj] = (Pw[j] - temp[jj + 1] * alfj) / (1.0 - alfj);
        ++i;
        --j;
        ++ii;
        --jj;
    }

    // Check removability
    if ((temp[ii - 1] - temp[jj + 1]).magnitude() > tol) return false;

    i = first;
    j = last;
    while (j - i > remflag) {
        Pw[i] = temp[i - off];
        Pw[j] = temp[j - off];
        ++i;
        --j;
    }

    // Shift remaining controls
    for (size_t l = k - p + 1; l < np - 1; ++l) {
        Pw[l] = Pw[l + 1];
    }
    Pw.resize(np - 1);

    // Update splineData
    splineData.controlPoints.resize(np - 1);
    splineData.weights.resize(np - 1);
    for (size_t l = 0; l < np - 1; ++l) {
        splineData.controlPoints[l] = Pw[l].toPoint();
        splineData.weights[l] = Pw[l].w;
    }

    // Remove knot
    splineData.knotslist.erase(splineData.knotslist.begin() + r);

    return true;
}

/**
 * Fallback non-uniform knot insertion.
 */
void LC_SplineHelper::insertKnotNonUniform(std::vector<double>& knotVector, size_t insertIndex, double newKnotValue, size_t splineOrder) {
    if (insertIndex > knotVector.size()) insertIndex = knotVector.size();
    knotVector.insert(knotVector.begin() + insertIndex, newKnotValue);
}

/**
 * Fallback non-uniform knot removal.
 */
void LC_SplineHelper::removeKnotNonUniform(std::vector<double>& knotVector, size_t removeIndex, size_t splineOrder) {
    if (removeIndex >= knotVector.size()) return;
    knotVector.erase(knotVector.begin() + removeIndex);
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
std::vector<double> LC_SplineHelper::openUniformKnot(size_t controlPointCount, size_t splineOrder) {
    std::vector<double> openKnotVector(controlPointCount + splineOrder);
    std::iota(openKnotVector.begin(), openKnotVector.end(), 0.0);
    return openKnotVector;
}

/**
 * Compute averaged knots for B-spline interpolation.
 */
std::vector<double> LC_SplineHelper::computeAveragedKnots(const std::vector<double>& parameters, int degree) {
    size_t numPoints = parameters.size();
    if (numPoints < static_cast<size_t>(degree + 1)) return {};
    int n = static_cast<int>(numPoints) - 1;
    size_t m = numPoints + static_cast<size_t>(degree) + 1;
    std::vector<double> knots(m, 0.0);
    for (int j = 1; j <= n - degree; ++j) {
        double sum = 0.0;
        for (int i = j; i <= j + degree - 1; ++i) {
            sum += parameters[i];
        }
        knots[j + degree] = sum / degree;
    }
    std::fill(knots.end() - degree - 1, knots.end(), 1.0);
    return knots;
}
