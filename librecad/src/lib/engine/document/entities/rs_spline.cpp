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

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"

/** Constructor for RS_SplineData with degree and closed flag. */
RS_SplineData::RS_SplineData(int _degree, bool _closed):
    degree(_degree)
{
    if (_closed) {
        type = SplineType::WrappedClosed;
        savedOpenType = SplineType::ClampedOpen;
    } else {
        type = SplineType::ClampedOpen;
        savedOpenType = type;
    }
}

/** Stream operator for RS_SplineData. */
std::ostream& operator << (std::ostream& os, const RS_SplineData& ld) {
    os << "( degree: " << ld.degree <<
        " type: " << static_cast<int>(ld.type) <<
        " savedOpenType: " << static_cast<int>(ld.savedOpenType);
    if (ld.controlPoints.size()) {
        os << "\n(control points:\n";
        for (auto const& v: ld.controlPoints)
            os<<v;
        os<<")\n";
    }
    if (ld.knotslist.size()) {
        os << "\n(knot vector:\n";
        for (auto const& v: ld.knotslist)
            os<<v;
        os<<")\n";
    }
    if (ld.weights.size()) {
        os << "\n(weights:\n";
        for (auto const& w: ld.weights)
            os<<w << " ";
        os<<")\n";
    }
    if (ld.fitPoints.size()) {
        os << "\n(fit points:\n";
        for (auto const& v: ld.fitPoints)
            os<<v;
        os<<")\n";
    }
    os  << ")";
    return os;
}

/** Constructor for RS_Spline; adds wrapping if closed and calculates borders. */
RS_Spline::RS_Spline(RS_EntityContainer* parent,
                     const RS_SplineData& d)
    :RS_EntityContainer(parent), data(d)
{
    if (data.type == RS_SplineData::SplineType::WrappedClosed && !hasWrappedControlPoints()) {
        addWrapping();
    }
    calculateBorders();
    update();  // Generate approximation on creation
}

/** Clone the spline. */
RS_Entity* RS_Spline::clone() const{
    auto* l = new RS_Spline(*this);
    l->setOwner(isOwner());
    l->detach();
    return l;
}

/** Get spline data reference. */
RS_SplineData& RS_Spline::getData() {
    return data;
}

/** Get const spline data reference. */
const RS_SplineData& RS_Spline::getData() const {
    return data;
}

/** Get unwrapped size (control points without wrapping). */
size_t RS_Spline::getUnwrappedSize() const {
    size_t unwrappedSize = data.controlPoints.size();
    if (data.type == RS_SplineData::SplineType::WrappedClosed && hasWrappedControlPoints()) {
        if (unwrappedSize > static_cast<size_t>(data.degree)) {
            unwrappedSize -= data.degree;
        } else {
            unwrappedSize = 0; // Invalid wrapping
        }
    }
    return unwrappedSize;
}

/** Get unwrapped control points. */
std::vector<RS_Vector> RS_Spline::getUnwrappedControlPoints() const {
    size_t unwrappedSize = getUnwrappedSize();
    if (unwrappedSize == 0) return {};
    return {data.controlPoints.begin(), data.controlPoints.begin() + unwrappedSize};
}

/** Get unwrapped weights. */
std::vector<double> RS_Spline::getUnwrappedWeights() const {
    size_t unwrappedSize = getUnwrappedSize();
    if (unwrappedSize == 0)
        return {};
    return {data.weights.begin(), data.weights.begin() + unwrappedSize};
}

/** Get unwrapped knot vector. */
std::vector<double> RS_Spline::getUnwrappedKnotVector() const {
    size_t unwrappedSize = getUnwrappedSize();
    if (unwrappedSize == 0)
        return {};
    size_t baseSize = unwrappedSize + static_cast<size_t>(data.degree) + 1;
    if (baseSize > data.knotslist.size())
        return {};
    return {data.knotslist.begin(), data.knotslist.begin() + baseSize};
}

/** Remove wrapping from control points, weights, and knots. */
void RS_Spline::removeWrapping() {
    LC_SplineHelper::removeWrapping(data);
}

/** Add wrapping to control points and weights for closed splines. */
void RS_Spline::addWrapping() {
    LC_SplineHelper::addWrapping(data);
}

/** Update wrapping for control points and weights. */
void RS_Spline::updateControlAndWeightWrapping() {
    LC_SplineHelper::updateControlAndWeightWrapping(data, isClosed(), getUnwrappedSize());
}

/** Update knot vector wrapping for closed splines. */
void RS_Spline::updateKnotWrapping() {
    LC_SplineHelper::updateKnotWrapping(data, isClosed(), getUnwrappedSize());
}

/** Calculate bounding box from control points. */
void RS_Spline::calculateBorders() {
    resetBorders();
    size_t unwrappedSize = getUnwrappedSize();
    if (unwrappedSize == 0)
        return;
    for (size_t i = 0; i < unwrappedSize; ++i) {
        minV = RS_Vector::minimum(data.controlPoints[i], minV);
        maxV = RS_Vector::maximum(data.controlPoints[i], maxV);
    }
}

/** Set degree (1-3), throws if invalid. */
void RS_Spline::setDegree(int degree) {
    if (degree < 1 || degree > 3) {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "%s(%d): invalid degree = %d", __func__, __LINE__, degree);
        throw std::invalid_argument("Degree must be 1, 2, or 3");
    }
    data.degree = degree;
}

/** Get degree. */
int RS_Spline::getDegree() const {
    return data.degree;
}

/** Get number of control points (unwrapped). */
size_t RS_Spline::getNumberOfControlPoints() const {
    return getUnwrappedSize();
}

/** Get number of knots. */
size_t RS_Spline::getNumberOfKnots() const {
    return data.knotslist.size();
}

/** Check if closed. */
bool RS_Spline::isClosed() const {
    return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Set closed flag and adjust wrapping/knots. */
void RS_Spline::setClosed(bool c) {
    size_t unwrapped = getUnwrappedSize();
    if (c) {
        data.savedOpenType = data.type;
        LC_SplineHelper::toWrappedClosedFromStandard(data);
    } else {
        if (data.type == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toStandardFromWrappedClosed(data);
        }
        data.type = RS_SplineData::SplineType::ClampedOpen;
    }
    update();
}

/** Get reference points (unwrapped control points). */
RS_VectorSolutions RS_Spline::getRefPoints() const {
    RS_VectorSolutions ret = RS_VectorSolutions(getControlPoints());
    return ret;
}

/** Nearest reference point (overrides container method) */
RS_Vector RS_Spline::getNearestRef(const RS_Vector& coord, double* dist) const {
    return RS_EntityContainer::getNearestRef(coord, dist);
}

/** Nearest selected reference (overrides container method) */
RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_EntityContainer::getNearestSelectedRef(coord, dist);
}

/** Update polyline approximation */
void RS_Spline::update() {
    clear();  // Clear existing child entities
    std::vector<RS_Vector> points;
    fillStrokePoints(32, points);  // Use 32 points for approximation (configurable if needed)
    for (size_t i = 0; i < points.size() - 1; ++i) {
        addEntity(new RS_Line(this, points[i], points[i + 1]));
    }
}

/** Fill points for spline approximation */
void RS_Spline::fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) {
    size_t numControlPoints = data.controlPoints.size();
    size_t splineOrder = data.degree + 1;
    auto knotVector = data.knotslist;
    auto weightVector = data.weights;
    double t_min = knotVector[data.degree];
    double t_max = knotVector[numControlPoints];
    double step = (t_max - t_min) / splineSegments;
    for (int i = 0; i <= splineSegments; ++i) {
        double t = t_min + i * step;
        points.push_back(evaluateNURBS(data, t));
    }
}

/** Get start point (invalid if closed) */
RS_Vector RS_Spline::getStartpoint() const {
    return RS_Vector(false); // Invalid for closed
}

/** Get end point (invalid if closed) */
RS_Vector RS_Spline::getEndpoint() const {
    return RS_Vector(false); // Invalid for closed
}

/** Nearest endpoint or control point */
RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    return RS_Vector(false);
}

/** Nearest center (invalid) */
RS_Vector RS_Spline::getNearestCenter(const RS_Vector& coord, double* dist) const {
    return RS_Vector(false);
}

/** Nearest middle point (invalid) */
RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const {
    return RS_Vector(false);
}

/** Nearest point at distance (invalid) */
RS_Vector RS_Spline::getNearestDist(double distance, const RS_Vector& coord, double* dist) const {
    return RS_Vector(false);
}

/** Move by offset */
void RS_Spline::move(const RS_Vector& offset) {
    for (auto& cp : data.controlPoints) cp += offset;
    for (auto& fp : data.fitPoints) fp += offset;
    calculateBorders();
}

/** Rotate by angle */
void RS_Spline::rotate(const RS_Vector& center, double angle) {
    for (auto& cp : data.controlPoints) cp.rotate(center, angle);
    for (auto& fp : data.fitPoints) fp.rotate(center, angle);
    calculateBorders();
}

/** Rotate by angle vector */
void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& cp : data.controlPoints) cp.rotate(center, angleVector);
    for (auto& fp : data.fitPoints) fp.rotate(center, angleVector);
    calculateBorders();
}

/** Scale by factor */
void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& cp : data.controlPoints) cp.scale(center, factor);
    for (auto& fp : data.fitPoints) fp.scale(center, factor);
    calculateBorders();
}

/** Shear by factor */
RS_Entity& RS_Spline::shear(double k) {
    for (auto& cp : data.controlPoints) cp.shear(k);
    for (auto& fp : data.fitPoints) fp.shear(k);
    calculateBorders();
    return *this;
}

/** Mirror across axis */
void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& cp : data.controlPoints) cp.mirror(axisPoint1, axisPoint2);
    for (auto& fp : data.fitPoints) fp.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

/** Move reference point (control point) */
void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    for (auto& cp : data.controlPoints) {
        if (cp == ref) cp += offset;
    }
    calculateBorders();
    update();
}

/** Revert direction by reversing points, weights, knots */
void RS_Spline::revertDirection() {
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    std::reverse(data.weights.begin(), data.weights.end());
    std::reverse(data.knotslist.begin(), data.knotslist.end());
    update();
}

/** Draw spline with painter */
void RS_Spline::draw(RS_Painter* painter) {
    RS_EntityContainer::draw(painter);
}

/** Get control points (unwrapped) */
std::vector<RS_Vector> RS_Spline::getControlPoints() const {
    return getUnwrappedControlPoints();
}

/** Get weights (unwrapped) */
std::vector<double> RS_Spline::getWeights() const {
    return getUnwrappedWeights();
}

/** Get weight at index */
double RS_Spline::getWeight(size_t index) const {
    if (index >= data.weights.size()) return 1.0;
    return data.weights[index];
}

/** Add control point with weight, handling wrapping */
void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    size_t old_unwrapped_size = getUnwrappedSize();
    bool closed = isClosed();
    if (closed) {
        data.controlPoints.insert(data.controlPoints.begin() + old_unwrapped_size, v);
        data.weights.insert(data.weights.begin() + old_unwrapped_size, w);
    } else {
        data.controlPoints.push_back(v);
        data.weights.push_back(w);
    }
    update();
}

/** Add raw control point */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    if (data.controlPoints.size() >= std::numeric_limits<size_t>::max() - 1) {
        return; // Edge case: overflow prevention
    }

    RS_SplineData::SplineType original_type = data.type;
    changeType(RS_SplineData::SplineType::Standard);

    data.controlPoints.push_back(v);
    data.weights.push_back(w);

    size_t current_control_size = data.controlPoints.size();
    size_t order = data.degree + 1;
    size_t expected_knot_size = current_control_size + order;
    bool valid_knots = (data.knotslist.size() == expected_knot_size - 1); // Check old size

    if (valid_knots) {
        LC_SplineHelper::extendKnotVector(data.knotslist);
        LC_SplineHelper::ensureMonotonic(data.knotslist);
    } else {
        // Edge case: regenerate if invalid
        data.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(current_control_size, order);
    }

    changeType(original_type);
    calculateBorders();
    update();
}

/** Remove last control point, handling wrapping */
void RS_Spline::removeLastControlPoint() {
    if (data.controlPoints.empty()) return;
    size_t old_unwrapped_size = getUnwrappedSize();
    bool closed = isClosed();
    if (closed) {
        data.controlPoints.erase(data.controlPoints.begin() + old_unwrapped_size - 1);
        data.weights.erase(data.weights.begin() + old_unwrapped_size - 1);
    } else {
        data.controlPoints.pop_back();
        data.weights.pop_back();
    }
    update();
}

/** Set weight at index */
void RS_Spline::setWeight(size_t index, double w) {
    if (index >= data.weights.size()) return;
    data.weights[index] = w;
    update();
}

/** Set all weights */
void RS_Spline::setWeights(const std::vector<double>& weights) {
    data.weights = weights;
    update();
}

/** Set control point at index */
void RS_Spline::setControlPoint(size_t index, const RS_Vector& v) {
    if (index >= data.controlPoints.size()) return;
    data.controlPoints[index] = v;
    calculateBorders();
    update();
}

/** Set knot at index */
void RS_Spline::setKnot(size_t index, double k) {
    if (index >= data.knotslist.size()) return;
    data.knotslist[index] = k;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
    update();
}

/** Insert control point, clear knots if present */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    if (index > data.controlPoints.size()) {
        index = data.controlPoints.size();
    }
    if (data.controlPoints.size() >= std::numeric_limits<size_t>::max() - 1) {
        return; // Edge case: overflow prevention
    }

    RS_SplineData::SplineType original_type = data.type;
    changeType(RS_SplineData::SplineType::Standard);

    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);

    size_t current_control_size = data.controlPoints.size();
    size_t order = data.degree + 1;
    size_t expected_knot_size = current_control_size + order;
    bool valid_knots = (data.knotslist.size() == expected_knot_size - 1);

    if (valid_knots) {
        size_t knot_index = index + data.degree;
        LC_SplineHelper::insertKnot(data.knotslist, knot_index);
        LC_SplineHelper::ensureMonotonic(data.knotslist);
    } else {
        // Edge case: regenerate if invalid
        data.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(current_control_size, order);
    }

    changeType(original_type);
    calculateBorders();
    update();
}

/** Remove control point, clear knots if present */
void RS_Spline::removeControlPoint(size_t index) {
    if (index >= data.controlPoints.size()) {
        return;
    }
    size_t min_controls = static_cast<size_t>(data.degree + 1);
    if (data.controlPoints.size() <= min_controls) {
        return; // Edge case: cannot reduce below minimum
    }

    RS_SplineData::SplineType original_type = data.type;
    changeType(RS_SplineData::SplineType::Standard);

    data.controlPoints.erase(data.controlPoints.begin() + index);
    data.weights.erase(data.weights.begin() + index);

    size_t current_control_size = data.controlPoints.size();
    size_t order = data.degree + 1;
    size_t expected_old_knot_size = current_control_size + order + 1; // Old size
    bool valid_knots = (data.knotslist.size() == expected_old_knot_size);

    if (valid_knots) {
        size_t knot_index = index + data.degree;
        if (knot_index >= data.knotslist.size()) {
            knot_index = data.knotslist.size() - 1;
        }
        LC_SplineHelper::removeKnot(data.knotslist, knot_index);
        LC_SplineHelper::ensureMonotonic(data.knotslist);
    } else {
        // Edge case: regenerate if invalid
        data.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(current_control_size, order);
    }

    changeType(original_type);
    calculateBorders();
    update();
}

/** Get knot vector (unwrapped) */
std::vector<double> RS_Spline::getKnotVector() const {
    return getUnwrappedKnotVector();
}

/** Set knot vector, validate size and monotonicity */
void RS_Spline::setKnotVector(const std::vector<double>& knots) {
    if (knots.size() != data.controlPoints.size() + data.degree + 1) return;
    data.knotslist = knots;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
    update();
}

/** Generate open knot vector */
std::vector<double> RS_Spline::knot(size_t num, size_t order) const {
    return LC_SplineHelper::knot(num, order);
}

/** Generate open uniform knot vector without multiple knots at ends */
std::vector<double> RS_Spline::openUniformKnot(size_t num, size_t order) const {
    return LC_SplineHelper::generateOpenUniformKnotVector(num, order);
}

/** Generate uniform knot vector for periodic splines */
std::vector<double> RS_Spline::knotu(size_t num, size_t order) const {
    std::vector<double> kn(num + order);
    std::iota(kn.begin(), kn.end(), 0.0);
    return kn;
}

/** Generate rational B-spline points (open) */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    double step;
    double t = 0.;
    int i;

    step = static_cast<double>(k - p1) / (npts - 1);
    for (i = 0; i < static_cast<int>(npts); i++) {
        if ((i == npts - 1) && t != (k - p1)) {
            t = static_cast<double>(k - p1);
        }

        p[i] = RS_Vector(0.0, 0.0);
        auto nbasis = rbasis(p1, t, k, data.knotslist, h);
        for (int j = 0; j < k; ++j) {
            p[i] += b[j] * nbasis[j];
        }
        t += step;
    }
}

/** Generate rational B-spline points (periodic) */
void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    double step;
    double t = static_cast<double>(p1 - 1);
    int i;

    step = static_cast<double>(k - p1 + 2) / npts;
    for (i = 0; i < static_cast<int>(npts); i++) {
        if ((i == npts - 1) && t != (k - p1 + 1)) {
            t = static_cast<double>(k - p1 + 1);
        }

        p[i] = RS_Vector(0.0, 0.0);
        auto nbasis = rbasis(p1, t, k, data.knotslist, h);
        for (int j = 0; j < k; ++j) {
            p[i] += b[j] * nbasis[j];
        }
        t += step;
    }
}

/** Check if control points wrapped (for closed cubic splines). */
bool RS_Spline::hasWrappedControlPoints() const {
    return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Stream operator for RS_Spline. */
std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}

/** Change spline type using helper */
void RS_Spline::changeType(RS_SplineData::SplineType newType) {
    if (data.type == newType) return;

    // Always convert to Standard as intermediate
    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        LC_SplineHelper::toStandardFromWrappedClosed(data);
    } else if (data.type == RS_SplineData::SplineType::ClampedOpen) {
        LC_SplineHelper::toStandardFromClampedOpen(data);
    }
    // Now data.type is Standard (or was already)

    if (newType == RS_SplineData::SplineType::WrappedClosed) {
        data.savedOpenType = RS_SplineData::SplineType::Standard;
        LC_SplineHelper::toWrappedClosedFromStandard(data);
    } else if (newType == RS_SplineData::SplineType::ClampedOpen) {
        LC_SplineHelper::toClampedOpenFromStandard(data);
    }
    // For Standard, already there

    update();
}

/** Set spline from fit points using global interpolation */
void RS_Spline::setFitPoints(const std::vector<RS_Vector>& fitPoints, bool useCentripetal) {
    size_t minPoints = static_cast<size_t>(data.degree + 1);
    if (fitPoints.size() < minPoints) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::setFitPoints: not enough fit points");
        return;
    }

    data.controlPoints.clear();
    data.weights.clear();
    data.knotslist.clear();
    data.fitPoints = fitPoints;

    size_t numPoints = fitPoints.size();
    int n = static_cast<int>(numPoints) - 1;
    int p = data.degree;
    bool closed = isClosed();

    // Parameterization
    std::vector<double> t(numPoints + (closed ? 1 : 0), 0.0);
    double total = 0.0;
    double alpha = useCentripetal ? 0.5 : 1.0;
    for (size_t k = 1; k < numPoints; ++k) {
        double dist = fitPoints[k].distanceTo(fitPoints[k - 1]);
        total += std::pow(dist, alpha);
    }
    if (closed) {
        double closingDist = fitPoints.back().distanceTo(fitPoints.front());
        total += std::pow(closingDist, alpha);
    }
    if (total < RS_TOLERANCE) {
        for (size_t k = 1; k < numPoints; ++k) {
            t[k] = static_cast<double>(k) / n;
        }
        if (closed) t[numPoints] = 1.0;
    } else {
        double cum = 0.0;
        for (size_t k = 1; k < numPoints; ++k) {
            double dist = fitPoints[k].distanceTo(fitPoints[k - 1]);
            cum += std::pow(dist, alpha);
            t[k] = cum / total;
        }
        if (closed) {
            double closingDist = fitPoints.back().distanceTo(fitPoints.front());
            cum += std::pow(closingDist, alpha);
            t[numPoints] = cum / total;
        }
    }

    // Knot vector
    std::vector<double> U(numPoints + p + 1, 0.0);
    if (closed) {
        for (int j = p; j < static_cast<int>(numPoints); ++j) {
            double sum = 0.0;
            for (int i = j - p; i < j; ++i) {
                sum += t[i + 1];
            }
            U[j] = sum / p;
        }
        for (int j = 0; j < p; ++j) {
            U[j] = U[j + static_cast<int>(numPoints) - p] - 1.0;
        }
        for (size_t j = numPoints; j < U.size(); ++j) {
            U[j] = U[j - numPoints + p] + 1.0;
        }
    } else {
        std::fill(U.begin(), U.begin() + p + 1, 0.0);
        for (int j = 1; j <= n - p; ++j) {
            double sum = 0.0;
            for (int i = j; i <= j + p - 1; ++i) {
                sum += t[i];
            }
            U[j + p] = sum / p;
        }
        std::fill(U.end() - p - 1, U.end(), 1.0);
    }

    data.knotslist = U;

    // Basis matrix
    size_t numControls = numPoints;
    std::vector<std::vector<double>> N(numPoints, std::vector<double>(numControls, 0.0));
    for (size_t k = 0; k < numPoints; ++k) {
        N[k] = getBSplineBasis(t[k], U, p, numControls);
    }

    // Solve for X and Y
    std::vector<double> bx(numPoints), by(numPoints);
    for (size_t i = 0; i < numPoints; ++i) {
        bx[i] = fitPoints[i].x;
        by[i] = fitPoints[i].y;
    }

    std::vector<double> px(numPoints), py(numPoints);
    if (!solveLinear(N, bx, px) || !solveLinear(N, by, py)) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::setFitPoints: singular matrix");
        return;
    }

    for (size_t i = 0; i < numPoints; ++i) {
        data.controlPoints.push_back(RS_Vector(px[i], py[i]));
    }

    data.weights.assign(numPoints, 1.0);

    if (closed) {
        addWrapping();
    }

    update();
}

/** Get non-rational B-spline basis functions at t. */
std::vector<double> RS_Spline::getBSplineBasis(double t, const std::vector<double>& knots, int degree, size_t numControls) const {
    int order = degree + 1;
    int numPoints = static_cast<int>(numControls);
    int c = order;
    int nplusc = numPoints + c;
    std::vector<double> basisFunctions(nplusc, 0.0);

    for (int i = 0; i < nplusc - 1; i++) {
        if ((t >= knots[i] - RS_TOLERANCE) && (t < knots[i + 1] + RS_TOLERANCE)) basisFunctions[i] = 1.0;
    }

    for (int k = 2; k <= c; k++) {
        for (int i = 0; i < nplusc - k; i++) {
            double d1 = 0.0;
            if (basisFunctions[i] != 0.0) {
                double den = knots[i + k - 1] - knots[i];
                if (std::abs(den) > RS_TOLERANCE) {
                    d1 = ((t - knots[i]) * basisFunctions[i]) / den;
                }
            }
            double d2 = 0.0;
            if (basisFunctions[i + 1] != 0.0) {
                double den = knots[i + k] - knots[i + 1];
                if (std::abs(den) > RS_TOLERANCE) {
                    d2 = ((knots[i + k] - t) * basisFunctions[i + 1]) / den;
                }
            }
            basisFunctions[i] = d1 + d2;
        }
    }

    if (t + RS_TOLERANCE >= knots[nplusc - 1]) basisFunctions[numPoints - 1] = 1.0;

    return {basisFunctions.begin(), basisFunctions.begin() + numPoints};
}

/** Solve linear system using Gaussian elimination with partial pivoting. */
bool RS_Spline::solveLinear(const std::vector<std::vector<double>>& A, const std::vector<double>& b, std::vector<double>& x) const {
    size_t n = A.size();
    if (n != A[0].size() || n != b.size()) return false;

    std::vector<std::vector<double>> mat(n, std::vector<double>(n + 1, 0.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            mat[i][j] = A[i][j];
        }
        mat[i][n] = b[i];
    }

    for (size_t i = 0; i < n; ++i) {
        // Find pivot
        size_t maxRow = i;
        for (size_t k = i + 1; k < n; ++k) {
            if (std::abs(mat[k][i]) > std::abs(mat[maxRow][i])) {
                maxRow = k;
            }
        }
        // Swap
        std::swap(mat[i], mat[maxRow]);
        // Singular check
        if (std::abs(mat[i][i]) < RS_TOLERANCE) return false;
        // Eliminate
        for (size_t k = i + 1; k < n; ++k) {
            double c = -mat[k][i] / mat[i][i];
            for (size_t j = i; j <= n; ++j) {
                mat[k][j] += c * mat[i][j];
            }
        }
    }

    x.resize(n);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        x[i] = mat[i][n];
        for (size_t j = i + 1; j < n; ++j) {
            x[i] -= mat[i][j] * x[j];
        }
        x[i] /= mat[i][i];
    }
    return true;
}


/** Compute rational B-spline basis functions */
std::vector<double> RS_Spline::rbasis(int c, double t, int npts,
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

RS_Vector RS_Spline::evaluateNURBS(const RS_SplineData& data, double t) {
    size_t numControlPoints = data.controlPoints.size();
    size_t splineOrder = data.degree + 1;
    auto knotVector = data.knotslist;
    auto weightVector = data.weights;
    auto basisFunctions = rbasis(static_cast<int>(splineOrder), t, static_cast<int>(numControlPoints), knotVector, weightVector);
    RS_Vector point(0.0, 0.0);
    for (size_t basisIndex = 0; basisIndex < numControlPoints; basisIndex++) {
        point += data.controlPoints[basisIndex] * basisFunctions[basisIndex];
    }
    return point;
}
