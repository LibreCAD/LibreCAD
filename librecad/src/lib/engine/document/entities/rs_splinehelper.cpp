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
#include "rs_splinehelper.h"
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
}

/**
 * Convert a closed wrapped knot vector to an open clamped knot vector.
 * Extracts internal knots and adds clamping multiplicities.
 *
 * @param closedKnots    Wrapped closed knot vector
 * @param unwrappedSize  Number of unwrapped control points
 * @param degree         Spline degree
 * @return               Open clamped knot vector
 */
std::vector<double> RS_SplineHelper::convertClosedToOpenKnotVector(const std::vector<double>& closedKnots,
                                                                   size_t unwrappedSize,
                                                                   size_t degree) {
    size_t order = degree + 1;
    size_t expectedSize = unwrappedSize + order + degree;
    if (closedKnots.size() < expectedSize) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "convertClosedToOpenKnotVector: Invalid size");
        return {};
    }

    double period = closedKnots[unwrappedSize] - closedKnots[degree];
    if (period <= RS_TOLERANCE) {
        return {};
    }

    std::vector<double> openKnots(order, closedKnots[degree] - period);
    std::copy(closedKnots.begin() + degree,
              closedKnots.begin() + degree + unwrappedSize - degree + 1,
              std::back_inserter(openKnots));
    std::fill(openKnots.end() - order, openKnots.end(), openKnots.back() + period);
    return openKnots;
}

/**
 * Normalize knot vector to [newMin, newMax] while preserving relative spacing.
 *
 * @param knots     Input knots
 * @param newMin    Target min
 * @param newMax    Target max
 * @param fallback  Fallback if invalid
 * @return          Normalized knots
 */
std::vector<double> RS_SplineHelper::getNormalizedKnotVector(const std::vector<double>& knots,
                                                             double newMin,
                                                             double newMax,
                                                             const std::vector<double>& fallback) {
    if (knots.size() < 2) return fallback;
    double minK = *std::min_element(knots.begin(), knots.end());
    double maxK = *std::max_element(knots.begin(), knots.end());
    double range = maxK - minK;
    if (range < RS_TOLERANCE) return fallback;

    std::vector<double> normalized(knots.size());
    double scale = (newMax - newMin) / range;
    std::transform(knots.begin(), knots.end(), normalized.begin(),
                   [minK, newMin, scale](double k) { return newMin + (k - minK) * scale; });
    return normalized;
}

/**
 * Unclamp a knot vector by removing endpoint multiplicity using adjacent spacing.
 *
 * @param knots        Clamped knots
 * @param numControl   Number of control points
 * @param order        Order (degree + 1)
 * @return             Unclamped knots
 */
std::vector<double> RS_SplineHelper::unclampKnotVector(const std::vector<double>& knots,
                                                       size_t numControl,
                                                       size_t order) {
    size_t degree = order - 1;
    if (knots.size() != numControl + order) return knots;

    std::vector<double> newKnots = knots;
    double domainStart = knots[degree];
    double domainEnd = knots[numControl];

    // Estimate spacing from first internal interval
    double sLeft = (degree + 1 < knots.size() && std::abs(knots[degree + 1] - knots[degree]) > RS_TOLERANCE)
                       ? knots[degree + 1] - knots[degree]
                       : (domainEnd - domainStart) / (numControl - degree ? numControl - degree : 1);

    for (int i = static_cast<int>(degree) - 1; i >= 0; --i) {
        newKnots[i] = newKnots[i + 1] - sLeft;
    }

    double sRight = sLeft;
    if (numControl > 0 && std::abs(knots[numControl] - knots[numControl - 1]) > RS_TOLERANCE) {
        sRight = knots[numControl] - knots[numControl - 1];
    }
    for (size_t i = numControl + 1; i < newKnots.size(); ++i) {
        newKnots[i] = newKnots[i - 1] + sRight;
    }

    return newKnots;
}

/**
 * Clamp a knot vector by setting endpoint multiplicity without rescaling.
 *
 * @param knots        Input knots
 * @param numControl   Number of control points
 * @param order        Order
 * @return             Clamped knots
 */
std::vector<double> RS_SplineHelper::clampKnotVector(const std::vector<double>& knots,
                                                     size_t numControl,
                                                     size_t order) {
    if (knots.size() != numControl + order) return knots;
    std::vector<double> clamped = knots;
    double start = clamped[order - 1];  // First internal
    double end = clamped[numControl];  // Last internal
    std::fill(clamped.begin(), clamped.begin() + order, start);
    std::fill(clamped.end() - order, clamped.end(), end);
    return clamped;
}

/**
 * Convert Standard to ClampedOpen, preserving original knots in snapshot.
 */
void RS_SplineHelper::toClampedOpenFromStandard(RS_SplineData& data, size_t unwrappedSize) {
    size_t order = data.degree + 1;
    size_t expectedSize = unwrappedSize + order;

    data.savedOpenKnots = data.knotslist;
    data.knotslist = clampKnotVector(data.knotslist, unwrappedSize, order);
    data.type = RS_SplineData::SplineType::ClampedOpen;
}

/**
 * Convert ClampedOpen to Standard, restoring from snapshot if available.
 */
void RS_SplineHelper::toStandardFromClampedOpen(RS_SplineData& data, size_t unwrappedSize) {
    size_t order = data.degree + 1;
    size_t expectedSize = unwrappedSize + order;

    if (data.savedOpenKnots.size() == expectedSize) {
        data.knotslist = data.savedOpenKnots;
        data.savedOpenKnots.clear();
    } else {
        data.knotslist = unclampKnotVector(data.knotslist, unwrappedSize, order);
        data.knotslist = getNormalizedKnotVector(data.knotslist, 0.0, static_cast<double>(expectedSize - 1),
                                                 openUniformKnot(unwrappedSize, order));
    }
    data.type = RS_SplineData::SplineType::Standard;
}

/**
 * Convert ClampedOpen to WrappedClosed.
 */
void RS_SplineHelper::toWrappedClosedFromClampedOpen(RS_SplineData& data, size_t unwrappedSize) {
    size_t wrapSize = static_cast<size_t>(data.degree);
    if (unwrappedSize < 3 || wrapSize >= unwrappedSize) return;

    // Wrap controls
    data.controlPoints.insert(data.controlPoints.end(),
                              data.controlPoints.begin(), data.controlPoints.begin() + wrapSize);
    data.weights.insert(data.weights.end(),
                        data.weights.begin(), data.weights.begin() + wrapSize);

    // Extend knots
    size_t order = data.degree + 1;
    double period = data.knotslist[unwrappedSize] - data.knotslist[order - 1];
    if (period > RS_TOLERANCE) {
        size_t baseSize = unwrappedSize + order;
        size_t targetSize = baseSize + wrapSize;
        std::vector<double> extended = data.knotslist;
        extended.resize(targetSize);
        for (size_t i = 0; i < wrapSize; ++i) {
            extended[baseSize + i] = data.knotslist[order + i] + period;
        }
        data.knotslist = extended;
    }
    data.type = RS_SplineData::SplineType::WrappedClosed;
}

/**
 * Convert WrappedClosed to ClampedOpen.
 */
void RS_SplineHelper::toClampedOpenFromWrappedClosed(RS_SplineData& data, size_t& unwrappedSize) {
    size_t wrapSize = static_cast<size_t>(data.degree);
    size_t order = data.degree + 1;
    size_t baseSize = unwrappedSize + order;

    // Unwrap controls
    if (data.controlPoints.size() > unwrappedSize) {
        data.controlPoints.resize(unwrappedSize);
        data.weights.resize(unwrappedSize);
    }

    // Trim knots
    if (data.knotslist.size() > baseSize) {
        data.knotslist.resize(baseSize);
    }

    data.type = RS_SplineData::SplineType::ClampedOpen;
}

/**
 * Convert Standard to WrappedClosed directly.
 */
void RS_SplineHelper::toWrappedClosedFromStandard(RS_SplineData& data, size_t unwrappedSize) {
    data.savedOpenKnots = data.knotslist;
    toClampedOpenFromStandard(data, unwrappedSize);
    toWrappedClosedFromClampedOpen(data, unwrappedSize);
}

/**
 * Convert WrappedClosed to Standard directly.
 */
void RS_SplineHelper::toStandardFromWrappedClosed(RS_SplineData& data, size_t unwrappedSize) {
    toClampedOpenFromWrappedClosed(data, unwrappedSize);
    toStandardFromClampedOpen(data, unwrappedSize);

    size_t order = data.degree + 1;
    size_t expectedSize = unwrappedSize + order;
    if (data.savedOpenKnots.size() == expectedSize) {
        data.knotslist = data.savedOpenKnots;
        data.savedOpenKnots.clear();
    }
}

/**
 * Add wrapping to control points and weights.
 */
void RS_SplineHelper::addWrapping(RS_SplineData& data, bool isClosed) {
    if (!isClosed || data.degree <= 0) return;
    size_t wrapSize = static_cast<size_t>(data.degree);
    size_t n = data.controlPoints.size();
    if (n < wrapSize) return;

    data.controlPoints.insert(data.controlPoints.end(),
                              data.controlPoints.begin(), data.controlPoints.begin() + wrapSize);
    data.weights.insert(data.weights.end(),
                        data.weights.begin(), data.weights.begin() + wrapSize);
}

/**
 * Remove wrapping.
 */
void RS_SplineHelper::removeWrapping(RS_SplineData& data, bool isClosed, size_t& unwrappedSize) {
    if (!isClosed || data.degree <= 0) return;
    size_t wrapSize = static_cast<size_t>(data.degree);
    size_t n = data.controlPoints.size();
    if (n > wrapSize) {
        unwrappedSize = n - wrapSize;
        data.controlPoints.resize(unwrappedSize);
        data.weights.resize(unwrappedSize);
    }
}

/**
 * Update control and weight wrapping.
 */
void RS_SplineHelper::updateControlAndWeightWrapping(RS_SplineData& data, bool isClosed, size_t unwrappedSize) {
    removeWrapping(data, isClosed, unwrappedSize);
    addWrapping(data, isClosed);
}

/**
 * Update knot wrapping for closed splines.
 */
void RS_SplineHelper::updateKnotWrapping(RS_SplineData& data, bool isClosed, size_t unwrappedSize) {
    if (!isClosed || data.degree <= 0) return;
    size_t order = data.degree + 1;
    size_t baseSize = unwrappedSize + order;
    if (data.knotslist.size() < baseSize) return;

    double period = data.knotslist[unwrappedSize] - data.knotslist[data.degree];
    if (period <= RS_TOLERANCE) return;

    size_t wrapSize = static_cast<size_t>(data.degree);
    size_t targetSize = baseSize + wrapSize;
    if (data.knotslist.size() >= targetSize) return;

    std::vector<double> extended = data.knotslist;
    extended.resize(targetSize);
    for (size_t i = 0; i < wrapSize; ++i) {
        extended[baseSize + i] = data.knotslist[order + i] + period;
    }
    data.knotslist = extended;
}

/**
 * Check if knot vector is custom (non-uniform/non-standard).
 */
bool RS_SplineHelper::isCustomKnotVector(const std::vector<double>& knots, size_t numCtrl, size_t order) {
    auto uniform = openUniformKnot(numCtrl, order);
    auto clamped = knot(numCtrl, order);
    return !vectorsEqual(knots, uniform) && !vectorsEqual(knots, clamped);
}

/**
 * Find span index for parameter t.
 */
int RS_SplineHelper::findSpan(const std::vector<double>& knots, double t, int degree, size_t numCtrl) {
    size_t n = numCtrl - 1;
    if (t >= knots[n + 1] - RS_TOLERANCE) return static_cast<int>(n);
    if (t <= knots[degree] + RS_TOLERANCE) return degree;

    auto it = std::upper_bound(knots.begin() + degree, knots.begin() + n + 1, t);
    return static_cast<int>(it - knots.begin()) - 1;
}

/**
 * Boehm's knot insertion algorithm.
 */
void RS_SplineHelper::insertKnotBoehm(RS_SplineData& data, double t, double tol) {
    size_t p = data.degree;
    size_t np = data.controlPoints.size();
    if (data.knotslist.size() != np + p + 1) return;

    int k = findSpan(data.knotslist, t, p, np);
    if (std::abs(t - data.knotslist[k]) < tol || std::abs(t - data.knotslist[k + 1]) < tol) return;

    // Rational handling via homogeneous
    bool rational = std::any_of(data.weights.begin(), data.weights.end(),
                                [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });

    std::vector<RS_Vector> hom(np);
    std::vector<double> w(np);
    for (size_t i = 0; i < np; ++i) {
        w[i] = rational ? data.weights[i] : 1.0;
        hom[i] = data.controlPoints[i] * w[i];
    }

    std::vector<RS_Vector> newHom(np + 1);
    std::vector<double> newW(np + 1);
    for (size_t i = 0; i <= static_cast<size_t>(k - p); ++i) { newHom[i] = hom[i]; newW[i] = w[i]; }
    for (size_t i = k + 1; i < np; ++i) { newHom[i + 1] = hom[i]; newW[i + 1] = w[i]; }

    for (int j = k - p + 1; j <= k; ++j) {
        double denom = data.knotslist[j + p] - data.knotslist[j];
        double alpha = (denom > RS_TOLERANCE) ? (t - data.knotslist[j]) / denom : 0.0;
        newHom[j] = hom[j - 1] * (1 - alpha) + hom[j] * alpha;
        newW[j] = w[j - 1] * (1 - alpha) + w[j] * alpha;
    }

    std::vector<double> newKnots(data.knotslist.size() + 1);
    std::copy(data.knotslist.begin(), data.knotslist.begin() + k + 1, newKnots.begin());
    newKnots[k + 1] = t;
    std::copy(data.knotslist.begin() + k + 1, data.knotslist.end(), newKnots.begin() + k + 2);

    data.knotslist = newKnots;
    data.weights = newW;
    data.controlPoints.resize(np + 1);
    for (size_t i = 0; i <= np; ++i) {
        data.controlPoints[i] = (std::abs(newW[i]) > RS_TOLERANCE) ? newHom[i] / newW[i] : RS_Vector(false);
    }
}

/**
 * Boehm's knot removal algorithm.
 * Removes knot at index r if possible without exceeding error tolerance.
 * Returns true if removed, false otherwise.
 *
 * @param data  Spline data to modify
 * @param r     Index of knot to remove (0 to knots.size()-1)
 * @param tol   Maximum allowed geometric error (default RS_TOLERANCE)
 * @return      true if knot was removed
 */
bool RS_SplineHelper::removeKnotBoehm(RS_SplineData& data, size_t r, double tol) {
    size_t p = data.degree;
    size_t np = data.controlPoints.size();
    size_t nk = data.knotslist.size();

    // Validate sizes
    if (nk != np + p + 1 || r >= nk || r < p || r > nk - p - 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Invalid index or size");
        return false;
    }

    // Check if minimal spline, cannot remove
    if (np <= p + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Cannot remove from minimal spline");
        return false;
    }

    // Check multiplicity (only remove if multiplicity == 1)
    size_t mult = 1;
    for (size_t i = r + 1; i < nk && RS_Math::equal(data.knotslist[i], data.knotslist[r]); ++i) ++mult;
    for (int i = static_cast<int>(r) - 1; i >= 0 && RS_Math::equal(data.knotslist[i], data.knotslist[r]); --i) ++mult;
    if (mult > 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Knot multiplicity > 1, cannot remove exactly");
        return false;
    }

    // Rational handling
    bool rational = std::any_of(data.weights.begin(), data.weights.end(),
                                [](double w) { return std::abs(w - 1.0) > RS_TOLERANCE; });


    std::vector<Homog> Pw(np);
    for (size_t i = 0; i < np; ++i) {
        double ww = rational ? data.weights[i] : 1.0;
        Pw[i] = Homog(data.controlPoints[i], ww);
    }

    // Find span and set multiplicity to remove (1)
    double u = data.knotslist[r];
    int k = findSpan(data.knotslist, u, p, np);
    int s = 1; // Removing one instance

    // Error check: For i from k-p to k-s, compute error
    double maxError = 0.0;
    int ord = static_cast<int>(p) + 1;
    for (int i = k - static_cast<int>(p); i <= k - s; ++i) {
        if (i < 0 || i >= static_cast<int>(np)) continue;
        double denom = data.knotslist[k + ord] - data.knotslist[i + ord - 1];
        if (std::abs(denom) < RS_TOLERANCE) continue;
        double alpha = (data.knotslist[k + ord] - data.knotslist[i]) / denom;
        if (std::abs(alpha) < RS_TOLERANCE || std::abs(1.0 - alpha) < RS_TOLERANCE) continue;

        Homog estimated = Pw[i + 1] * alpha + Pw[i] * (1.0 - alpha);
        Homog diff = Pw[i] - estimated;
        maxError = std::max(maxError, diff.magnitude());
    }
    if (maxError > tol) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "removeKnotBoehm: Error %f exceeds tolerance %f", maxError, tol);
        return false;
    }

    // Perform removal: Update controls backward
    for (int j = k - p + 1; j <= k - s; ++j) {
        if (j < 0 || j >= static_cast<int>(np)) continue;
        double denom = data.knotslist[k + ord] - data.knotslist[j + ord - 1];
        if (std::abs(denom) < RS_TOLERANCE) return false;
        double alpha = (data.knotslist[k + ord] - data.knotslist[j]) / denom;

        // Solve for Pw[j-1]
        Pw[j - 1] = (Pw[j] - Pw[j - 1] * (1.0 - alpha)) / alpha;
    }

    // Shift controls to remove one
    for (size_t j = static_cast<size_t>(k - s + 1); j < np; ++j) {
        Pw[j - 1] = Pw[j];
    }
    Pw.resize(np - 1);

    // Remove knot
    std::vector<double> newKnots;
    newKnots.reserve(nk - 1);
    for (size_t j = 0; j < nk; ++j) {
        if (j != r) newKnots.push_back(data.knotslist[j]);
    }
    data.knotslist = newKnots;

    // Update data
    data.controlPoints.resize(np - 1);
    data.weights.resize(np - 1);
    for (size_t i = 0; i < np - 1; ++i) {
        data.controlPoints[i] = Pw[i].toPoint();
        data.weights[i] = Pw[i].w;
    }

    return true;
}

/**
 * Fallback non-uniform knot insertion.
 */
void RS_SplineHelper::insertKnotNonUniform(std::vector<double>& knots, size_t index, double newKnot, size_t order) {
    if (index > knots.size()) index = knots.size();
    knots.insert(knots.begin() + index, newKnot);
}

/**
 * Fallback non-uniform knot removal.
 */
void RS_SplineHelper::removeKnotNonUniform(std::vector<double>& knots, size_t index, size_t order) {
    if (index >= knots.size()) return;
    knots.erase(knots.begin() + index);
}

/**
 * Generate clamped uniform knot vector.
 */
std::vector<double> RS_SplineHelper::knot(size_t num, size_t order) {
    std::vector<double> k(num + order, 0.0);
    std::iota(k.begin() + order - 1, k.begin() + num + 1, 1.0);
    std::fill(k.begin() + num + 1, k.end(), static_cast<double>(num - order + 2));
    return k;
}

/**
 * Generate open uniform knot vector.
 */
std::vector<double> RS_SplineHelper::openUniformKnot(size_t num, size_t order) {
    std::vector<double> k(num + order);
    std::iota(k.begin(), k.end(), 0.0);
    return k;
}

/**
 * Validate spline data consistency.
 */
bool RS_SplineHelper::validate(const RS_SplineData& data, size_t unwrappedSize) {
    RS_DEBUG->setLevel(RS_Debug::D_WARNING);
    QString errorMsg = "";
    std::shared_ptr<QString> msgOutput{&errorMsg, [](QString* pointer) {
                                           if (!pointer || pointer->isEmpty()) {
                                               return;
                                           }
                                           std::cout<<pointer->toLatin1().data()<<std::endl;
                                           LC_ERR<<"validate(): "<< *pointer;
                                       }};
    if (data.degree < 1 || data.degree > 3 || unwrappedSize == 0) {
        errorMsg = "Invalid degree (must be 1-3) or unwrappedSize (must be >0)";
        return false;
    }
    size_t order = data.degree + 1;
    size_t wrapSize = data.isClosed() ? static_cast<size_t>(data.degree) : 0;
    size_t expectedControls = unwrappedSize + wrapSize;
    size_t expectedKnots = unwrappedSize + order + wrapSize;

    if (data.controlPoints.size() != expectedControls) {
        errorMsg = QString{"Control points size mismatch: expected "} + QString::number(expectedControls)
        + ", got " + QString::number(data.controlPoints.size());
        return false;
    }
    if (!data.weights.empty() && data.weights.size() != expectedControls) {
        errorMsg = "Weights size mismatch: expected " + QString::number(expectedControls)
        + ", got " + QString::number(data.weights.size());
        return false;
    }
    if (data.knotslist.size() != expectedKnots) {
        errorMsg = "Knots size mismatch: expected " + QString::number(expectedKnots) + ", got " + QString::number(data.knotslist.size());
        return false;
    }

    // Monotonicity (applies to full vector)
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        if (data.knotslist[i] < data.knotslist[i - 1] - RS_TOLERANCE) {
            errorMsg = "Knot vector not monotonic at index " + QString::number(i) + ": "
                       + QString::number(data.knotslist[i]) + " < " + QString::number(data.knotslist[i - 1]);
            return false;
        }
    }

    // Weights positive (applies to full)
    for (size_t i = 0; i < data.weights.size(); ++i) {
        if (data.weights[i] <= RS_TOLERANCE) {
            errorMsg = "Non-positive weight at index " + QString::number(i) + ": " + QString::number(data.weights[i]);
            return false;
        }
    }

    if (data.type == RS_SplineData::SplineType::ClampedOpen) {
        if (data.knotslist.empty()) {
            errorMsg = "ClampedOpen: Empty knot vector";
            return false;
        }
        double start = data.knotslist[0];
        for (size_t i = 1; i < order; ++i) {
            if (std::abs(data.knotslist[i] - start) > RS_TOLERANCE) {
                errorMsg = "ClampedOpen: Start knot multiplicity violation at index " + QString::number(i);
                return false;
            }
        }
        double end = data.knotslist.back();
        for (size_t i = 1; i < order; ++i) {
            size_t idx = data.knotslist.size() - i - 1;
            if (std::abs(data.knotslist[idx] - end) > RS_TOLERANCE) {
                errorMsg = "ClampedOpen: End knot multiplicity violation at index " + QString::number(idx);
                return false;
            }
        }
    }

    // For closed, check wrapping consistency
    if (data.isClosed()) {
        for (size_t i = 0; i < wrapSize; ++i) {
            if (data.controlPoints[unwrappedSize + i] != data.controlPoints[i]) {
                errorMsg = "Closed: Control point wrapping mismatch at index " + QString::number(i);
                return false;
            }
            if (!data.weights.empty() && data.weights[unwrappedSize + i] != data.weights[i]) {
                errorMsg = "Closed: Weight wrapping mismatch at index " + QString::number(i);
                return false;
            }
        }
        // Optional: Verify knot period
        double period = data.knotslist[unwrappedSize] - data.knotslist[data.degree];
        if (period <= RS_TOLERANCE) {
            errorMsg = "Closed: Invalid knot period <= tolerance";
            return false;
        }
        for (size_t i = 0; i < wrapSize; ++i) {
            double expected = data.knotslist[order + i] + period;
            if (std::abs(data.knotslist[unwrappedSize + order + i] - expected) > RS_TOLERANCE) {
                errorMsg = "Closed: Knot period mismatch at wrapped index " + QString::number(i);
                return false;
            }
        }
    }

    return true;
}
