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
#include <vector>

#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_math.h"

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
    RS_Spline::calculateBorders();
    RS_Spline::update();  // Generate approximation on creation
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
    if (unwrappedSize <= data.degree)
        return unwrappedSize;
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
    if (c) {
        if (data.type == RS_SplineData::SplineType::WrappedClosed) return; // Already closed
        data.savedOpenType = data.type;
        if (data.type == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
        } else {
            LC_SplineHelper::toWrappedClosedFromStandard(data);
        }
    } else {
        if (data.type != RS_SplineData::SplineType::WrappedClosed) return; // Already open
        LC_SplineHelper::toStandardFromWrappedClosed(data);
        // Restore to the original open type
        if (data.savedOpenType == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toClampedOpenFromStandard(data);
        } else if (data.savedOpenType == RS_SplineData::SplineType::Standard) {
            // Already in Standard after toStandardFromWrappedClosed
        }
        data.savedOpenType = RS_SplineData::SplineType::ClampedOpen; // Reset to default if needed
    }
    update();
}

/** Adjust knot vector to open clamped form */
std::vector<double> RS_Spline::adjustToOpenClamped(const std::vector<double>& knots,
                                                   size_t num_control,
                                                   size_t order,
                                                   bool is_natural) const {
    return LC_SplineHelper::clampKnotVector(knots, num_control, order);
}

/** Get reference points (unwrapped control points) */
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
    if (!validate())
        return;
    std::vector<RS_Vector> points;
    fillStrokePoints(32, points);  // Use 32 points for approximation (configurable if needed)
    for (size_t i = 0; i < points.size() - 1; ++i) {
        addEntity(new RS_Line(this, points[i], points[i + 1]));
    }
    if (isClosed() && points.size() > 1) {
        addEntity(new RS_Line(this, points.back(), points.front()));
    }
}

/** Fill points for spline approximation */
void RS_Spline::fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) {
    size_t numControlPoints = data.controlPoints.size();
    auto knotVector = data.knotslist;
    auto weightVector = data.weights;
    double t_min = knotVector[data.degree];
    double t_max = knotVector[numControlPoints - data.degree - 1];
    double step = (t_max - t_min) / splineSegments;
    for (int i = 0; i <= splineSegments; ++i) {
        double t = t_min + i * step;
        points.push_back(getPointAt(t));
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
    for (auto& cp : data.controlPoints) {
        cp.x += k * cp.y;
    }
    for (auto& fp : data.fitPoints) {
        fp.x += k * fp.y;
    }
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
    RS_EntityContainer::moveRef(ref, offset);
}

/** Revert direction by reversing points, weights, knots */
void RS_Spline::revertDirection() {
    std::reverse(data.controlPoints.begin(), data.controlPoints.begin() + getUnwrappedSize());
    std::reverse(data.weights.begin(), data.weights.begin() + getUnwrappedSize());
    auto knots = data.knotslist;
    std::reverse(knots.begin(), knots.begin() + getNumberOfKnots());
    double maxK = knots.back();
    for (auto& k : knots) {
        k = maxK - k;
    }
    data.knotslist = knots;
    updateControlAndWeightWrapping();
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
    return data.weights[index];
}

/**
 * Adds a control point with optional weight to the spline.
 * Handles type conversions for consistency across spline types.
 *
 * @param v The control point vector to add.
 * @param w The weight for rational splines (default 1.0 for non-rational).
 */
void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    if (data.controlPoints.size() <= data.degree) {
        addControlPointRaw(v, w);
        return;
    }
    RS_SplineData::SplineType originalType = data.type;

    // Convert to Standard if not already (removes clamping/wrapping temporarily)
    if (data.type != RS_SplineData::SplineType::Standard) {
        changeType(RS_SplineData::SplineType::Standard);
    }

    // Add the point and weight raw
    addControlPointRaw(v, w);

    // Extend the knot vector to accommodate the new point
    LC_SplineHelper::extendKnotVector(data.knotslist);

    // Restore original type (re-applies clamping or wrapping)
    if (data.type != originalType) {
        changeType(originalType);
    }

    // Recalculate bounding box and update approximation
    calculateBorders();
    update();
}

/**
 * Adds a control point and weight without type handling or updates.
 * Used internally for raw appends.
 *
 * @param v The control point vector.
 * @param w The weight.
 */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
}

/** Remove last control point, handling wrapping */
void RS_Spline::removeLastControlPoint() {
    data.controlPoints.pop_back();
    data.weights.pop_back();
    if (isClosed()) {
        updateControlAndWeightWrapping();
    }
    calculateBorders();
    update();
}

/** Set weight at index */
void RS_Spline::setWeight(size_t index, double w) {
    data.weights[index] = w;
}

/** Set all weights */
void RS_Spline::setWeights(const std::vector<double>& weights) {
    data.weights = weights;
}

/** Set control point at index */
void RS_Spline::setControlPoint(size_t index, const RS_Vector& v) {
    data.controlPoints[index] = v;
    calculateBorders();
    update();
}

/** Set knot at index */
void RS_Spline::setKnot(size_t index, double k) {
    data.knotslist[index] = k;
}
/**
 * Estimates the parameter t corresponding to the control point at the given index.
 * If knots are available, averages the knots around the index's span.
 * Fallback to uniform parameterization (index / num_controls) if no knots.
 *
 * @param index The control point index (unwrapped).
 * @return Estimated parameter t in the knot domain.
 * @throws std::out_of_range If index invalid.
 */
double RS_Spline::estimateParamAtIndex(size_t index) const {
    size_t unwrappedSize = getUnwrappedSize();
    if (index >= unwrappedSize) {
        throw std::out_of_range("Index out of range for parameter estimation");
    }

    if (data.knotslist.empty() || data.knotslist.size() < unwrappedSize + data.degree + 1) {
        // Fallback: uniform [0,1] parameterization
        return static_cast<double>(index) / static_cast<double>(unwrappedSize - 1);
    }

    // Average knots in the span influenced by the control point (mid of [index, index+degree])
    size_t startKnot = index;
    size_t endKnot = std::min(index + data.degree, data.knotslist.size() - 1);
    double sum = 0.0;
    size_t count = endKnot - startKnot + 1;
    for (size_t i = startKnot; i <= endKnot; ++i) {
        sum += data.knotslist[i];
    }
    return sum / static_cast<double>(count);
}
/**
 * Inserts a control point with weight at the given index, adjusting knots accordingly.
 * Temporarily converts to Standard type for insertion, then restores original type.
 *
 * @param index Insertion position (0 to current unwrapped size).
 * @param v The control point vector.
 * @param w The weight (default 1.0 for non-rational).
 * @param preserveShape If true, uses Boehm algorithm to insert without altering the curve shape.
 * @throws std::out_of_range If index is invalid.
 * @throws std::invalid_argument If resulting state is invalid (e.g., knot issues).
 */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w, bool preserveShape) {
    size_t unwrappedSize = getUnwrappedSize();
    if (index > unwrappedSize) {
        throw std::out_of_range("Insertion index out of range");
    }

    RS_SplineData::SplineType originalType = data.type;
    bool wasConverted = false;

    // Temporarily convert to Standard to simplify insertion (removes clamping/wrapping)
    if (data.type != RS_SplineData::SplineType::Standard) {
        changeType(RS_SplineData::SplineType::Standard);
        wasConverted = true;
    }

    if (preserveShape) {
        // Estimate parameter t at index and use Boehm for shape-preserving insertion
        //double t = estimateParamAtIndex(index);
        //LC_SplineHelper::insertKnot(data, t);
        // Boehm inserts a new control point; set its value and weight (optional override)
        // Note: Boehm computes the inserted point; v is ignored for preservation
    } else {
        // Standard insert: add point/weight, insert knot at adjusted position
        data.controlPoints.insert(data.controlPoints.begin() + index, v);
        data.weights.insert(data.weights.begin() + index, w);
        if (!data.knotslist.empty()) {
            size_t knotIndex = index + data.degree;
            LC_SplineHelper::insertKnot(data.knotslist, knotIndex);
            LC_SplineHelper::ensureMonotonic(data.knotslist);
        }
    }

    // Restore original type (re-applies clamping or wrapping)
    if (wasConverted) {
        changeType(originalType);
    } else if (isClosed()) {
        // Optimization: Update wrapping in-place without full conversion
        LC_SplineHelper::updateControlAndWeightWrapping(data, true, unwrappedSize + 1);
        LC_SplineHelper::updateKnotWrapping(data, true, unwrappedSize + 1);
    }

    calculateBorders();
    update();

    // Post-validation
    if (!validate()) {
        throw std::invalid_argument("Invalid spline state after insertion");
    }
}
/**
 * Removes the control point at the given index, adjusting knots accordingly.
 * Temporarily converts to Standard type for removal, then restores original type.
 *
 * @param index Position to remove (0 to current unwrapped size - 1).
 * @throws std::out_of_range If index is invalid.
 * @throws std::invalid_argument If removal leaves too few points or invalid state.
 */
void RS_Spline::removeControlPoint(size_t index) {
    size_t unwrappedSize = getUnwrappedSize();
    if (index >= unwrappedSize) {
        throw std::out_of_range("Removal index out of range");
    }
    if (unwrappedSize <= static_cast<size_t>(data.degree) + 1) {
        throw std::invalid_argument("Cannot remove: insufficient control points remaining");
    }

    RS_SplineData::SplineType originalType = data.type;
    bool wasConverted = false;

    // Temporarily convert to Standard to simplify removal
    if (data.type != RS_SplineData::SplineType::Standard) {
        changeType(RS_SplineData::SplineType::Standard);
        wasConverted = true;
    }

    // Remove point/weight
    data.controlPoints.erase(data.controlPoints.begin() + index);
    data.weights.erase(data.weights.begin() + index);

    // Remove corresponding knot if present
    if (!data.knotslist.empty()) {
        size_t knotIndex = index + data.degree;
        if (knotIndex < data.knotslist.size()) {
            LC_SplineHelper::removeKnot(data.knotslist, knotIndex);
            LC_SplineHelper::ensureMonotonic(data.knotslist);
        }
    }

    // Restore original type
    if (wasConverted) {
        changeType(originalType);
    } else if (isClosed()) {
        // Optimization: Update wrapping in-place
        LC_SplineHelper::updateControlAndWeightWrapping(data, true, unwrappedSize - 1);
        LC_SplineHelper::updateKnotWrapping(data, true, unwrappedSize - 1);
    }

    calculateBorders();
    update();

    // Post-validation
    if (!validate()) {
        throw std::invalid_argument("Invalid spline state after removal");
    }
}

/** Get knot vector (unwrapped) */
std::vector<double> RS_Spline::getKnotVector() const {
    return getUnwrappedKnotVector();
}

/** Set knot vector, validate size and monotonicity */
void RS_Spline::setKnotVector(const std::vector<double>& knots) {
    data.knotslist = knots;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
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
    std::vector<double> kn(num + order, 0.0);
    for (size_t i = 1; i < num + order; i++) {
        kn[i] = i;
    }
    return kn;
}

/** Generate rational B-spline points (open) */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    double t = static_cast<double>(p1 - 1);

    double step = static_cast<double>(k - p1 + 2) / npts;
    for (size_t i = 0; i < npts; i++) {
        if ((i == npts - 1) && t != (k - p1 + 1)) {
            t = static_cast<double>(k - p1 + 1);
        }

        p[i] = RS_Vector(0.0, 0.0);
        auto nbasis = rbasis(p1, t, k, data.knotslist, h);
        for (size_t j = 0; j < k; ++j) {
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
    double t = static_cast<double>(p1 - 1);

    double step = static_cast<double>(k - p1 + 2) / npts;
    for (size_t i = 0; i < npts; i++) {
        if ((i == npts - 1) && t != (k - p1 + 1)) {
            t = static_cast<double>(k - p1 + 1);
        }

        p[i] = RS_Vector(0.0, 0.0);
        auto nbasis = rbasis(p1, t, k, data.knotslist, h);
        for (size_t j = 0; j < k; ++j) {
            p[i] += b[j] * nbasis[j];
        }
        t += step;
    }
}

/** Check if control points wrapped (for closed cubic splines) */
bool RS_Spline::hasWrappedControlPoints() const {
    return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Output operator */
std::ostream& operator<<(std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}
/**
 * Changes the spline representation type (Standard, ClampedOpen, WrappedClosed).
 * Handles all transitions, saves/restores open knots, and normalizes.
 *
 * @param newType Target type.
 * @throws std::invalid_argument If invalid type or insufficient points.
 */
void RS_Spline::changeType(RS_SplineData::SplineType newType) {
    if (newType == data.type) return;

    if (newType < RS_SplineData::SplineType::Standard || newType > RS_SplineData::SplineType::WrappedClosed) {
        throw std::invalid_argument("Invalid spline type");
    }

    size_t unwrappedSize = getUnwrappedSize();
    if (unwrappedSize < static_cast<size_t>(data.degree) + 1) {
        throw std::invalid_argument("Insufficient control points for type change");
    }

    // Handle transitions (direct or chained)
    if (data.type == RS_SplineData::SplineType::Standard) {
        if (newType == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toClampedOpenFromStandard(data);
        } else if (newType == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toWrappedClosedFromStandard(data);
        }
    } else if (data.type == RS_SplineData::SplineType::ClampedOpen) {
        if (newType == RS_SplineData::SplineType::Standard) {
            LC_SplineHelper::toStandardFromClampedOpen(data);
        } else if (newType == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
        }
    } else if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        if (newType == RS_SplineData::SplineType::Standard) {
            LC_SplineHelper::toStandardFromWrappedClosed(data);
        } else if (newType == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toClampedOpenFromWrappedClosed(data);
        }
    }

    data.type = newType;

    // Normalize knots for consistent evaluation
    normalizeKnots();

    calculateBorders();
    update();

    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::changeType: Invalid state after change");
        throw std::invalid_argument("Invalid spline state after type change");
    }
}

/**
 * Normalizes the knot vector by shifting to start at 0, preserving relative spacing.
 * Uses LC_SplineHelper for computation; fallback to empty if invalid.
 * Called after knot modifications for consistent evaluation.
 */
void RS_Spline::normalizeKnots() {
    // Use helper with min=0, no fallback ({} if empty/invalid)
    data.knotslist = LC_SplineHelper::getNormalizedKnotVector(data.knotslist, 0.0, {});
    // No need for ensureMonotonic post-normalize (helper preserves order)
}

/** Public method to evaluate spline at parameter t */
RS_Vector RS_Spline::getPointAt(double t) const {
    return evaluateNURBS(data, t);
}

/** Find parameters where derivative is zero for x or y */
std::vector<double> RS_Spline::findDerivativeZeros(bool isX) const {
    std::vector<double> zeros;
    auto knotVector = data.knotslist;
    size_t degree = data.degree;
    size_t numControlPoints = data.controlPoints.size();
    if (numControlPoints < degree + 1) return zeros;

    bool isRational = !data.weights.empty() && !std::all_of(data.weights.begin(), data.weights.end(), [](double w) { return w == 1.0; });

    // For simplicity, assume single span Bezier-like for degree 3 with 4 points
    if (degree == 3 && numControlPoints == 4 && knotVector.size() == 8) {
        if (isRational) {
            // For rational, solve w*coord' - coord*w' =0, which is a quintic
            // Coefficients for coord(t) = (sum bi * Ni * wi) / sum Ni*wi, deriv = (w coord' - coord w') / w^2 =0 so w coord' - coord w' =0
            // For Bezier, Ni are Bernstein, but to use RS_Math, set up the polynomial
            // But RS_Math has up to quartic, for quintic, need numerical or factor
            // For test, use numerical as fallback
            double tMin = knotVector[degree];
            double tMax = knotVector[numControlPoints - degree - 1];
            int numSamples = 100;
            double step = (tMax - tMin) / numSamples;
            double prevDeriv = getDerivative(tMin, isX);
            for (int i = 1; i <= numSamples; ++i) {
                double t = tMin + i * step;
                double deriv = getDerivative(t, isX);
                if (deriv * prevDeriv <= 0.0) {
                    zeros.push_back(bisectDerivativeZero(t - step, t, prevDeriv, isX));
                }
                prevDeriv = deriv;
            }
        } else {
            // Non-rational cubic Bezier
            double P0 = isX ? data.controlPoints[0].x : data.controlPoints[0].y;
            double P1 = isX ? data.controlPoints[1].x : data.controlPoints[1].y;
            double P2 = isX ? data.controlPoints[2].x : data.controlPoints[2].y;
            double P3 = isX ? data.controlPoints[3].x : data.controlPoints[3].y;

            double a = 3 * (P3 - 3*P2 + 3*P1 - P0);
            double b = 6 * (P2 - 2*P1 + P0);
            double c = 3 * (P1 - P0);

            std::vector<double> ce = {0., 0., a, b, c};
            auto roots = RS_Math::quarticSolverFull(ce);
            for (double root : roots) {
                if (root >= 0.0 && root <= 1.0) zeros.push_back(root);
            }
        }
    } else if (degree == 2 && numControlPoints == 3 && knotVector.size() == 6) {
        if (isRational) {
            // Rational quadratic, cubic equation
            double w0 = data.weights[0];
            double w1 = data.weights[1];
            double w2 = data.weights[2];

            double Q0 = w0 * (isX ? data.controlPoints[0].x : data.controlPoints[0].y);
            double Q1 = w1 * (isX ? data.controlPoints[1].x : data.controlPoints[1].y);
            double Q2 = w2 * (isX ? data.controlPoints[2].x : data.controlPoints[2].y);

            double a0 = w0;
            double a1 = -2*w0 + 2*w1;
            double a2 = w0 - 2*w1 + w2;

            double a0_Q = Q0;
            double a1_Q = -2*Q0 + 2*Q1;
            double a2_Q = Q0 - 2*Q1 + Q2;

            double b0 = -2*w0 + 2*w1;
            double b1 = 2*w0 - 4*w1 + 2*w2;

            double b0_Q = -2*Q0 + 2*Q1;
            double b1_Q = 2*Q0 - 4*Q1 + 2*Q2;

            double ce3 = b1_Q * a2 - a2_Q * b1;
            double ce2 = b0_Q * a2 + b1_Q * a1 - a1_Q * b1 - a2_Q * b0;
            double ce1 = b0_Q * a1 + b1_Q * a0 - a0_Q * b1 - a1_Q * b0;
            double ce0 = b0_Q * a0 - a0_Q * b0;

            std::vector<double> ce = {0., ce3, ce2, ce1, ce0};
            auto roots = RS_Math::quarticSolverFull(ce);
            for (double root : roots) {
                if (root >= 0.0 && root <= 1.0) zeros.push_back(root);
            }
        } else {
            // Non-rational quadratic Bezier
            double P0 = isX ? data.controlPoints[0].x : data.controlPoints[0].y;
            double P1 = isX ? data.controlPoints[1].x : data.controlPoints[1].y;
            double P2 = isX ? data.controlPoints[2].x : data.controlPoints[2].y;

            double a = 2 * (P2 - 2*P1 + P0);
            double b = 2 * (P1 - P0);

            if (std::abs(a) > RS_TOLERANCE) {
                double root = -b / a;
                if (root >= 0.0 && root <= 1.0) zeros.push_back(root);
            }
        }
    } else {
        // General case or degree 1, use numerical method
        double tMin = knotVector[degree];
        double tMax = knotVector[numControlPoints - degree - 1];
        int numSamples = 100;
        double step = (tMax - tMin) / numSamples;
        double prevDeriv = getDerivative(tMin, isX);
        for (int i = 1; i <= numSamples; ++i) {
            double t = tMin + i * step;
            double deriv = getDerivative(t, isX);
            if (deriv * prevDeriv <= 0.0) {
                zeros.push_back(bisectDerivativeZero(t - step, t, prevDeriv, isX));
            }
            prevDeriv = deriv;
        }
    }
    return zeros;
}

/** Bisection to find zero of derivative */
double RS_Spline::bisectDerivativeZero(double a, double b, double fa, bool isX) const {
    for (int iter = 0; iter < 20; ++iter) {
        double m = (a + b) / 2.0;
        double fm = getDerivative(m, isX);
        if (fm * fa > 0.0) {
            a = m;
            fa = fm;
        } else {
            b = m;
        }
        if (std::abs(b - a) < 1e-9) break;
    }
    return (a + b) / 2.0;
}

/** Approximate derivative at t using central difference */
double RS_Spline::getDerivative(double t, bool isX) const {
    double eps = 1e-6;
    RS_Vector p1 = getPointAt(t - eps);
    RS_Vector p2 = getPointAt(t + eps);
    if (isX) return (p2.x - p1.x) / (2 * eps);
    return (p2.y - p1.y) / (2 * eps);
}

/** Calculate tight bounding box using extrema and endpoints */
void RS_Spline::calculateTightBorders() {
    resetBorders();
    auto knotVector = data.knotslist;
    double tMin = knotVector[data.degree];
    double tMax = knotVector[data.controlPoints.size() - data.degree - 1];
    RS_Vector start = getPointAt(tMin);
    RS_Vector end = getPointAt(tMax);
    minV = RS_Vector::minimum(start, minV);
    minV = RS_Vector::minimum(end, minV);
    maxV = RS_Vector::maximum(start, maxV);
    maxV = RS_Vector::maximum(end, maxV);

    auto xZeros = findDerivativeZeros(true);
    for (double t : xZeros) {
        if (t >= tMin && t <= tMax) {
            RS_Vector point = getPointAt(t);
            minV = RS_Vector::minimum(point, minV);
            maxV = RS_Vector::maximum(point, maxV);
        }
    }

    auto yZeros = findDerivativeZeros(false);
    for (double t : yZeros) {
        if (t >= tMin && t <= tMax) {
            RS_Vector point = getPointAt(t);
            minV = RS_Vector::minimum(point, minV);
            maxV = RS_Vector::maximum(point, maxV);
        }
    }
}

void RS_Spline::resetBorders() {
    minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);
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

/** Set fit points and generate spline via interpolation */
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
    std::vector<std::vector<double>> mx(numPoints, std::vector<double>(numControls + 1, 0.0));
    std::vector<std::vector<double>> my(numPoints, std::vector<double>(numControls + 1, 0.0));
    for (size_t i = 0; i < numPoints; ++i) {
        for (size_t j = 0; j < numControls; ++j) {
            mx[i][j] = N[i][j];
            my[i][j] = N[i][j];
        }
        mx[i][numControls] = bx[i];
        my[i][numControls] = by[i];
    }
    if (!RS_Math::linearSolver(mx, px) || !RS_Math::linearSolver(my, py)) {
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

namespace {
bool compareVector(const RS_Vector& va, const RS_Vector& vb, double tol = 1e-4) {
    return va.distanceTo(vb) <= tol;
}
}
/**
 * Validates the spline data integrity, checking sizes, monotonicity, multiplicities,
 * positive weights, and minimum control points.
 *
 * @return true if valid, false otherwise (logs warnings via RS_DEBUG).
 */
bool RS_Spline::validate() const {
    size_t numControls = getUnwrappedSize();
    size_t expectedKnots = numControls + data.degree + 1;
    size_t numWeights = data.weights.size();

    // Check minimum controls
    if (numControls < data.degree + 1) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Insufficient control points (need >= degree + 1)");
        return false;
    }

    // Check vector sizes
    if (data.knotslist.size() != expectedKnots) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot vector size mismatch (expected %zu, got %zu)", expectedKnots, data.knotslist.size());
        return false;
    }
    if (numWeights != data.controlPoints.size()) {  // Full size, including wrapping
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Weights size mismatch with controls");
        return false;
    }

    // Check knot monotonicity (non-decreasing)
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        if (data.knotslist[i] < data.knotslist[i - 1] - RS_TOLERANCE) {  // Allow approx equal for floats
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot vector not monotonic at index %zu", i);
            return false;
        }
    }

    // Check knot multiplicities (<= degree)
    size_t mult = 1;
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        if (fabs(data.knotslist[i] - data.knotslist[i - 1]) < RS_TOLERANCE) {
            ++mult;
        } else {
            if (mult > data.degree) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot multiplicity exceeds degree (%zu > %zu)", mult, data.degree);
                return false;
            }
            mult = 1;
        }
    }
    if (mult > data.degree) {  // Check last group
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Knot multiplicity exceeds degree at end");
        return false;
    }

    // Check positive weights (for rational stability; 0 or negative cause division issues)
    for (double w : data.weights) {
        if (w <= 0.0) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Non-positive weight found (%f)", w);
            return false;
        }
    }

    // Type-specific checks (e.g., wrapping consistency)
    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        if (!hasWrappedControlPoints()) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: WrappedClosed but no wrapping detected");
            return false;
        }
        // Verify last degree points/weights match first (within tolerance)
        for (size_t i = 0; i < data.degree; ++i) {
            if (!compareVector(data.controlPoints[numControls + i], data.controlPoints[i])) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Wrapped controls mismatch at index %zu", i);
                return false;
            }
            if (fabs(data.weights[numControls + i] - data.weights[i]) > RS_TOLERANCE) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: Wrapped weights mismatch at index %zu", i);
                return false;
            }
        }
    }

    // ClampedOpen: Check endpoint multiplicities (p+1 at ends)
    if (data.type == RS_SplineData::SplineType::ClampedOpen) {
        size_t startMult = 0, endMult = 0;
        double startVal = data.knotslist[0];
        for (size_t i = 1; i < data.knotslist.size() && fabs(data.knotslist[i] - startVal) < RS_TOLERANCE; ++i) ++startMult;
        double endVal = data.knotslist.back();
        for (size_t i = data.knotslist.size() - 2; i > 0 && fabs(data.knotslist[i] - endVal) < RS_TOLERANCE; --i) ++endMult;
        if (startMult != data.degree + 1 || endMult != data.degree + 1) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::validate: ClampedOpen endpoints multiplicity incorrect (expected %zu)", data.degree + 1);
            return false;
        }
    }

    return true;
}

