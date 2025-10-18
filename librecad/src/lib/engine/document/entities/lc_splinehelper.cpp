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
    std::fill_n(std::back_inserter(openKnotVector), splineOrder - (openKnotVector.size() - (unwrappedControlCount + 1)), lastKnotValue);
    return getNormalizedKnotVector(openKnotVector, 0.0, static_cast<double>(unwrappedControlCount - splineDegree), {});
}

/**
 * Normalize knot vector to [newMin, newMax] while preserving relative spacing.
 */
std::vector<double> LC_SplineHelper::getNormalizedKnotVector(const std::vector<double>& inputKnotVector,
                                                             double newMinimum,
                                                             double newMaximum,
                                                             const std::vector<double>& fallbackKnotVector) {
    if (inputKnotVector.size() < 2) return fallbackKnotVector;
    double minKnot = *std::min_element(inputKnotVector.begin(), inputKnotVector.end());
    double maxKnot = *std::max_element(inputKnotVector.begin(), inputKnotVector.end());
    double knotRange = maxKnot - minKnot;
    if (knotRange < RS_TOLERANCE) return fallbackKnotVector;

    std::vector<double> normalizedKnotVector(inputKnotVector.size());
    double scaleFactor = (newMaximum - newMinimum) / knotRange;
    std::transform(inputKnotVector.begin(), inputKnotVector.end(), normalizedKnotVector.begin(),
                   [minKnot, newMinimum, scaleFactor](double knotValue) { return newMinimum + (knotValue - minKnot) * scaleFactor; });
    return normalizedKnotVector;
}

/**
 * Unclamp a knot vector by removing endpoint multiplicity using adjacent spacing.
 */
std::vector<double> LC_SplineHelper::unclampKnotVector(const std::vector<double>& inputKnotVector, size_t controlPointCount, size_t splineOrder) {
    if (inputKnotVector.size() != controlPointCount + splineOrder) return inputKnotVector;
    std::vector<double> unclampedKnotVector = inputKnotVector;
    size_t splineDegree = splineOrder - 1;
    double leftIntervalSpacing = inputKnotVector[splineDegree + 1] - inputKnotVector[splineDegree];
    for (int extensionIndex = splineDegree - 1; extensionIndex >= 0; --extensionIndex) unclampedKnotVector[extensionIndex] = unclampedKnotVector[extensionIndex + 1] - leftIntervalSpacing;
    double rightIntervalSpacing = inputKnotVector[controlPointCount] - inputKnotVector[controlPointCount - 1];
    for (size_t extensionIndex = controlPointCount + 1; extensionIndex < unclampedKnotVector.size(); ++extensionIndex) unclampedKnotVector[extensionIndex] = unclampedKnotVector[extensionIndex - 1] + rightIntervalSpacing;
    return getNormalizedKnotVector(unclampedKnotVector, 0.0, static_cast<double>(controlPointCount - splineDegree), inputKnotVector);
}

/**
 * Clamp a knot vector by setting endpoint multiplicity without rescaling.
 */
std::vector<double> LC_SplineHelper::clampKnotVector(const std::vector<double>& inputKnotVector,
                                                     size_t controlPointCount,
                                                     size_t splineOrder) {
    if (inputKnotVector.size() != controlPointCount + splineOrder) return inputKnotVector;
    std::vector<double> clampedKnotVector = inputKnotVector;
    double startValue = clampedKnotVector[splineOrder - 1];  // First internal
    double endValue = clampedKnotVector[controlPointCount];  // Last internal
    std::fill(clampedKnotVector.begin(), clampedKnotVector.begin() + splineOrder, startValue);
    std::fill(clampedKnotVector.end() - splineOrder, clampedKnotVector.end(), endValue);
    return clampedKnotVector;
}

/**
 * Convert to clamped open from standard, preserving knots.
 */
void LC_SplineHelper::toClampedOpenFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount) {
    size_t splineOrder = splineData.degree + 1;
    splineData.savedOpenKnots = splineData.knotslist;
    splineData.knotslist = clampKnotVector(splineData.knotslist, unwrappedControlCount, splineOrder);
    splineData.knotslist = getNormalizedKnotVector(splineData.knotslist, 0.0, static_cast<double>(unwrappedControlCount - splineData.degree), splineData.savedOpenKnots);
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
    splineData.savedOpenType = RS_SplineData::SplineType::Standard;
}

/**
 * Convert to standard from clamped open, restoring knots.
 */
void LC_SplineHelper::toStandardFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount) {
    splineData.knotslist = unclampKnotVector(splineData.knotslist, unwrappedControlCount, splineData.degree + 1);
    if (!splineData.savedOpenKnots.empty()) {
        splineData.knotslist = splineData.savedOpenKnots;
        splineData.savedOpenKnots.clear();
    }
    splineData.type = RS_SplineData::SplineType::Standard;
    splineData.savedOpenType = splineData.type;
}

/**
 * Convert to wrapped closed from clamped open.
 */
void LC_SplineHelper::toWrappedClosedFromClampedOpen(RS_SplineData& splineData, size_t unwrappedControlCount) {
    addWrapping(splineData, true);
    updateKnotWrapping(splineData, true, unwrappedControlCount);
    splineData.type = RS_SplineData::SplineType::WrappedClosed;
}

/**
 * Convert to clamped open from wrapped closed.
 */
void LC_SplineHelper::toClampedOpenFromWrappedClosed(RS_SplineData& splineData, size_t& unwrappedControlCount) {
    removeWrapping(splineData, true, unwrappedControlCount);
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
}

/**
 * Convert to wrapped closed from standard.
 */
void LC_SplineHelper::toWrappedClosedFromStandard(RS_SplineData& splineData, size_t unwrappedControlCount) {
    toClampedOpenFromStandard(splineData, unwrappedControlCount);
    toWrappedClosedFromClampedOpen(splineData, unwrappedControlCount);
}

/**
 * Convert to standard from wrapped closed.
 */
void LC_SplineHelper::toStandardFromWrappedClosed(RS_SplineData& splineData, size_t unwrappedControlCount) {
    toClampedOpenFromWrappedClosed(splineData, unwrappedControlCount);
    toStandardFromClampedOpen(splineData, unwrappedControlCount);
}

/**
 * Add wrapping for closed splines.
 */
void LC_SplineHelper::addWrapping(RS_SplineData& splineData, bool isClosed) {
    if (!isClosed) return;
    size_t wrapCount = splineData.degree;
    splineData.controlPoints.reserve(splineData.controlPoints.size() + wrapCount);
    std::copy(splineData.controlPoints.begin(), splineData.controlPoints.begin() + wrapCount,
              std::back_inserter(splineData.controlPoints));
    if (!splineData.weights.empty()) {
        splineData.weights.reserve(splineData.weights.size() + wrapCount);
        std::copy(splineData.weights.begin(), splineData.weights.begin() + wrapCount,
                  std::back_inserter(splineData.weights));
    }
}

/**
 * Remove wrapping for closed splines.
 */
void LC_SplineHelper::removeWrapping(RS_SplineData& splineData, bool isClosed, size_t& unwrappedControlCount) {
    if (!isClosed) return;
    size_t wrapCount = splineData.degree;
    unwrappedControlCount = splineData.controlPoints.size() - wrapCount;
    splineData.controlPoints.resize(unwrappedControlCount);
    if (!splineData.weights.empty()) splineData.weights.resize(unwrappedControlCount);
    splineData.knotslist = convertClosedToOpenKnotVector(splineData.knotslist, unwrappedControlCount, splineData.degree);
}

/**
 * Update control and weight wrapping.
 */
void LC_SplineHelper::updateControlAndWeightWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount) {
    if (!isClosed) return;
    size_t wrapCount = splineData.degree;
    for (size_t wrapIndex = 0; wrapIndex < wrapCount; ++wrapIndex) {
        splineData.controlPoints[unwrappedControlCount + wrapIndex] = splineData.controlPoints[wrapIndex];
        if (!splineData.weights.empty()) {
            splineData.weights[unwrappedControlCount + wrapIndex] = splineData.weights[wrapIndex];
        }
    }
}

/**
 * Update knot wrapping for closed splines.
 */
void LC_SplineHelper::updateKnotWrapping(RS_SplineData& splineData, bool isClosed, size_t unwrappedControlCount) {
    if (!isClosed) return;
    size_t splineOrder = splineData.degree + 1;
    double periodValue = splineData.knotslist[unwrappedControlCount] - splineData.knotslist[splineData.degree];
    splineData.knotslist.resize(unwrappedControlCount + splineOrder + splineData.degree);
    for (size_t wrapIndex = 0; wrapIndex < splineData.degree; ++wrapIndex) {
        splineData.knotslist[unwrappedControlCount + splineOrder + wrapIndex] = splineData.knotslist[splineOrder + wrapIndex] + periodValue;
    }
}

/**
 * Validate spline data consistency.
 */
bool LC_SplineHelper::validate(const RS_SplineData& splineData, size_t unwrappedControlCount) {
    RS_DEBUG->setLevel(RS_Debug::D_WARNING);
    QString errorMessage = "";
    std::shared_ptr<QString> messageOutput{&errorMessage, [](QString* pointer) {
                                               if (!pointer || pointer->isEmpty()) {
                                                   return;
                                               }
                                               std::cout<<pointer->toLatin1().data()<<std::endl;
                                               LC_ERR<<"validate(): "<< *pointer;
                                           }};
    if (splineData.degree < 1 || splineData.degree > 3 || unwrappedControlCount == 0) {
        errorMessage = "Invalid degree (must be 1-3) or unwrappedControlCount (must be >0)";
        return false;
    }
    size_t splineOrder = splineData.degree + 1;
    size_t wrapCount = splineData.isClosed() ? static_cast<size_t>(splineData.degree) : 0;
    size_t expectedControlCount = unwrappedControlCount + wrapCount;
    size_t expectedKnotCount = unwrappedControlCount + splineOrder + wrapCount;

    if (splineData.controlPoints.size() != expectedControlCount) {
        errorMessage = QString{"Control points size mismatch: expected "} + QString::number(expectedControlCount)
        + ", got " + QString::number(splineData.controlPoints.size());
        return false;
    }
    if (!splineData.weights.empty() && splineData.weights.size() != expectedControlCount) {
        errorMessage = "Weights size mismatch: expected " + QString::number(expectedControlCount)
        + ", got " + QString::number(splineData.weights.size());
        return false;
    }
    if (splineData.knotslist.size() != expectedKnotCount) {
        errorMessage = "Knots size mismatch: expected " + QString::number(expectedKnotCount) + ", got " + QString::number(splineData.knotslist.size());
        return false;
    }

    // Monotonicity (applies to full vector)
    for (size_t knotIndex = 1; knotIndex < splineData.knotslist.size(); ++knotIndex) {
        if (splineData.knotslist[knotIndex] < splineData.knotslist[knotIndex - 1] - RS_TOLERANCE) {
            errorMessage = "Knot vector not monotonic at index " + QString::number(knotIndex) + ": "
                           + QString::number(splineData.knotslist[knotIndex]) + " < " + QString::number(splineData.knotslist[knotIndex - 1]);
            return false;
        }
    }

    // Weights positive (applies to full)
    for (size_t weightIndex = 0; weightIndex < splineData.weights.size(); ++weightIndex) {
        if (splineData.weights[weightIndex] <= RS_TOLERANCE) {
            errorMessage = "Non-positive weight at index " + QString::number(weightIndex) + ": " + QString::number(splineData.weights[weightIndex]);
            return false;
        }
    }

    if (splineData.type == RS_SplineData::SplineType::ClampedOpen) {
        if (splineData.knotslist.empty()) {
            errorMessage = "ClampedOpen: Empty knot vector";
            return false;
        }
        double startValue = splineData.knotslist[0];
        for (size_t multiplicityIndex = 1; multiplicityIndex < splineOrder; ++multiplicityIndex) {
            if (std::abs(splineData.knotslist[multiplicityIndex] - startValue) > RS_TOLERANCE) {
                errorMessage = "ClampedOpen: Start knot multiplicity violation at index " + QString::number(multiplicityIndex);
                return false;
            }
        }
        double endValue = splineData.knotslist.back();
        for (size_t multiplicityIndex = 1; multiplicityIndex < splineOrder; ++multiplicityIndex) {
            size_t checkIndex = splineData.knotslist.size() - multiplicityIndex - 1;
            if (std::abs(splineData.knotslist[checkIndex] - endValue) > RS_TOLERANCE) {
                errorMessage = "ClampedOpen: End knot multiplicity violation at index " + QString::number(checkIndex);
                return false;
            }
        }
    }

    // For closed, check wrapping consistency
    if (splineData.isClosed()) {
        for (size_t wrapIndex = 0; wrapIndex < wrapCount; ++wrapIndex) {
            if (splineData.controlPoints[unwrappedControlCount + wrapIndex] != splineData.controlPoints[wrapIndex]) {
                errorMessage = "Closed: Control point wrapping mismatch at index " + QString::number(wrapIndex);
                return false;
            }
            if (!splineData.weights.empty() && splineData.weights[unwrappedControlCount + wrapIndex] != splineData.weights[wrapIndex]) {
                errorMessage = "Closed: Weight wrapping mismatch at index " + QString::number(wrapIndex);
                return false;
            }
        }
        // Optional: Verify knot period
        double periodValue = splineData.knotslist[unwrappedControlCount] - splineData.knotslist[splineData.degree];
        if (periodValue <= RS_TOLERANCE) {
            errorMessage = "Closed: Invalid knot period <= tolerance";
            return false;
        }
        for (size_t wrapIndex = 0; wrapIndex < wrapCount; ++wrapIndex) {
            double expectedKnot = splineData.knotslist[splineOrder + wrapIndex] + periodValue;
            if (std::abs(splineData.knotslist[unwrappedControlCount + splineOrder + wrapIndex] - expectedKnot) > RS_TOLERANCE) {
                errorMessage = "Closed: Knot period mismatch at wrapped index " + QString::number(wrapIndex);
                return false;
            }
        }
    }

    // Append multiplicity check
    size_t currentMultiplicity = 1;
    double previousKnotValue = splineData.knotslist[0];
    for (size_t knotIndex = 1; knotIndex < splineData.knotslist.size(); ++knotIndex) {
        if (std::abs(splineData.knotslist[knotIndex] - previousKnotValue) < RS_TOLERANCE) {
            if (++currentMultiplicity > splineData.degree + 1) return false;
        } else {
            currentMultiplicity = 1;
            previousKnotValue = splineData.knotslist[knotIndex];
        }
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
void LC_SplineHelper::insertKnotBoehm(RS_SplineData& splineData, double parameterT, double tolerance) {
    size_t splineDegree = splineData.degree;
    size_t numControlPoints = splineData.controlPoints.size();
    size_t numKnots = splineData.knotslist.size();

    if (numKnots != numControlPoints + splineDegree + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "insertKnotBoehm: Invalid size");
        return;
    }

    int knotSpan = findSpan(splineData.knotslist, parameterT, splineDegree, numControlPoints);
    bool isRational = std::any_of(splineData.weights.begin(), splineData.weights.end(),
                                  [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });

    std::vector<Homog> homogeneousPoints(numControlPoints);
    for (size_t pointIndex = 0; pointIndex < numControlPoints; ++pointIndex) {
        double weight = isRational ? splineData.weights[pointIndex] : 1.0;
        homogeneousPoints[pointIndex] = Homog(splineData.controlPoints[pointIndex], weight);
    }

    std::vector<Homog> newHomogeneousPoints(numControlPoints + 1);
    for (int copyIndex = 0; copyIndex <= knotSpan - static_cast<int>(splineDegree); ++copyIndex) {
        newHomogeneousPoints[copyIndex] = homogeneousPoints[copyIndex];
    }
    for (int copyIndex = knotSpan; copyIndex < static_cast<int>(numControlPoints); ++copyIndex) {
        newHomogeneousPoints[copyIndex + 1] = homogeneousPoints[copyIndex];
    }

    for (int j = knotSpan; j > knotSpan - static_cast<int>(splineDegree); --j) {
        if (j < 0 || j >= static_cast<int>(numControlPoints)) continue;
        double denominator = splineData.knotslist[j + splineDegree] - splineData.knotslist[j];
        if (std::abs(denominator) < tolerance) continue;
        double alpha = (parameterT - splineData.knotslist[j]) / denominator;

        newHomogeneousPoints[j] = newHomogeneousPoints[j] * alpha + newHomogeneousPoints[j - 1] * (1.0 - alpha);
    }

    splineData.controlPoints.resize(numControlPoints + 1);
    splineData.weights.resize(numControlPoints + 1);
    for (size_t updateIndex = 0; updateIndex < numControlPoints + 1; ++updateIndex) {
        splineData.controlPoints[updateIndex] = newHomogeneousPoints[updateIndex].toPoint();
        splineData.weights[updateIndex] = newHomogeneousPoints[updateIndex].w;
    }

    std::vector<double> newKnotVector;
    newKnotVector.reserve(numKnots + 1);
    for (size_t insertIndex = 0; insertIndex <= static_cast<size_t>(knotSpan); ++insertIndex) {
        newKnotVector.push_back(splineData.knotslist[insertIndex]);
    }
    newKnotVector.push_back(parameterT);
    for (size_t insertIndex = knotSpan + 1; insertIndex < numKnots; ++insertIndex) {
        newKnotVector.push_back(splineData.knotslist[insertIndex]);
    }
    splineData.knotslist = newKnotVector;
}

/**
 * Remove knot using Boehm's algorithm if error < tol.
 */
bool LC_SplineHelper::removeKnotBoehm(RS_SplineData& splineData, size_t knotIndexToRemove, double tolerance) {
    size_t splineDegree = splineData.degree;
    size_t numControlPoints = splineData.controlPoints.size();
    size_t numKnots = splineData.knotslist.size();

    if (numKnots != numControlPoints + splineDegree + 1 || knotIndexToRemove >= numKnots || knotIndexToRemove < splineDegree || knotIndexToRemove > numKnots - splineDegree - 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Invalid index or size");
        return false;
    }

    if (numControlPoints <= splineDegree + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Cannot remove from minimal spline");
        return false;
    }

    size_t currentMultiplicity = 1;
    for (size_t checkIndex = knotIndexToRemove + 1; checkIndex < numKnots && RS_Math::equal(splineData.knotslist[checkIndex], splineData.knotslist[knotIndexToRemove]); ++checkIndex) ++currentMultiplicity;
    for (int checkIndex = static_cast<int>(knotIndexToRemove) - 1; checkIndex >= 0 && RS_Math::equal(splineData.knotslist[checkIndex], splineData.knotslist[knotIndexToRemove]); --checkIndex) ++currentMultiplicity;
    if (currentMultiplicity > 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Knot multiplicity > 1, cannot remove exactly");
        return false;
    }

    bool isRational = std::any_of(splineData.weights.begin(), splineData.weights.end(),
                                  [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });

    std::vector<Homog> homogeneousPoints(numControlPoints);
    for (size_t pointIndex = 0; pointIndex < numControlPoints; ++pointIndex) {
        double weight = isRational ? splineData.weights[pointIndex] : 1.0;
        homogeneousPoints[pointIndex] = Homog(splineData.controlPoints[pointIndex], weight);
    }

    int knotSpan = findSpan(splineData.knotslist, splineData.knotslist[knotIndexToRemove], splineDegree, numControlPoints);
    int multiplicityToRemove = 1; // Removing one instance

    RS_SplineData reducedSplineData = splineData;  // Copy for tentative removal
    std::vector<Homog> reducedHomogeneousPoints = homogeneousPoints;  // Copy for reduced

    // Perform tentative removal
    for (int j = knotSpan - multiplicityToRemove; j >= knotSpan - static_cast<int>(splineDegree) + 1; --j) {
        if (j < 0 || j >= static_cast<int>(numControlPoints)) continue;
        double denominator = reducedSplineData.knotslist[j + splineDegree] - reducedSplineData.knotslist[j];
        if (std::abs(denominator) < tolerance) return false;
        double alpha = (splineData.knotslist[knotIndexToRemove] - reducedSplineData.knotslist[j]) / denominator;

        reducedHomogeneousPoints[j] = (reducedHomogeneousPoints[j] - reducedHomogeneousPoints[j - 1] * alpha) / (1.0 - alpha);
    }

    // Shift controls
    for (size_t shiftIndex = static_cast<size_t>(knotSpan - multiplicityToRemove + 1); shiftIndex < numControlPoints; ++shiftIndex) {
        reducedHomogeneousPoints[shiftIndex - 1] = reducedHomogeneousPoints[shiftIndex];
    }
    reducedHomogeneousPoints.resize(numControlPoints - 1);

    // Remove knot
    std::vector<double> newKnotVector;
    newKnotVector.reserve(numKnots - 1);
    for (size_t knotIndex = 0; knotIndex < numKnots; ++knotIndex) {
        if (knotIndex != knotIndexToRemove) newKnotVector.push_back(reducedSplineData.knotslist[knotIndex]);
    }
    reducedSplineData.knotslist = newKnotVector;

    // Update reduced data
    reducedSplineData.controlPoints.resize(numControlPoints - 1);
    reducedSplineData.weights.resize(numControlPoints - 1);
    for (size_t updateIndex = 0; updateIndex < numControlPoints - 1; ++updateIndex) {
        reducedSplineData.controlPoints[updateIndex] = reducedHomogeneousPoints[updateIndex].toPoint();
        reducedSplineData.weights[updateIndex] = reducedHomogeneousPoints[updateIndex].w;
    }

    // Compute max error in affected span
    double lowerBoundU = splineData.knotslist[knotSpan - splineDegree + 1];
    double upperBoundU = splineData.knotslist[knotSpan + 1];
    const int sampleCount = 50;
    double parameterStepSize = (upperBoundU - lowerBoundU) / (sampleCount - 1.0);
    double maximumError = 0.0;
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double evaluationParameter = lowerBoundU + sampleIndex * parameterStepSize;
        maximumError = std::max(maximumError, evaluateNURBS(splineData, evaluationParameter).distanceTo(evaluateNURBS(reducedSplineData, evaluationParameter)));
    }

    if (maximumError > tolerance) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Error %f exceeds tolerance %f", maximumError, tolerance);
        return false;
    }

    splineData = reducedSplineData;  // Apply if success
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
