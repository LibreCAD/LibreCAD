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
#include <limits>

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
    } else {
        type = SplineType::ClampedOpen;
    }
}

/** Stream operator for RS_SplineData. */
std::ostream& operator << (std::ostream& os, const RS_SplineData& ld) {
    os << "( degree: " << ld.degree <<
        " type: " << static_cast<int>(ld.type);
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
        LC_SplineHelper::addWrapping(data);
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
    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        return LC_SplineHelper::convertClosedToOpenKnotVector(data.knotslist, unwrappedSize, data.degree);
    }
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

/** Calculate bounding box from control points */
void RS_Spline::calculateBorders() {
    resetBorders();
    if (data.controlPoints.empty()) return;

    minV = maxV = data.controlPoints[0];
    for (const auto& p : data.controlPoints) {
        minV = RS_Vector::minimum(minV, p);
        maxV = RS_Vector::maximum(maxV, p);
    }

    if (getDegree() > 1) {  // For higher degrees, use tight borders
        calculateTightBorders();
    }
}

/** Set degree (1-3), throws if invalid */
void RS_Spline::setDegree(int degree) {
    if (degree < 1 || degree > 3) throw std::invalid_argument("Degree must be 1-3");
    if (data.degree == static_cast<size_t>(degree)) return;
    data.degree = degree;
    if (isClosed()) {
        // Re-wrap with new degree
        LC_SplineHelper::removeWrapping(data);
        LC_SplineHelper::addWrapping(data);
    }
    update();
}

/** Get degree */
int RS_Spline::getDegree() const {
    return static_cast<int>(data.degree);
}

/** Get number of control points (unwrapped) */
size_t RS_Spline::getNumberOfControlPoints() const {
    return getUnwrappedSize();
}

/** Get number of knots */
size_t RS_Spline::getNumberOfKnots() const {
    return data.knotslist.size();
}

/** Check if closed */
bool RS_Spline::isClosed() const {
    return data.isClosed();
}

/** Set closed flag and adjust wrapping/knots */
void RS_Spline::setClosed(bool c) {
    if (c == isClosed()) return;
    RS_SplineData::SplineType target = c ? RS_SplineData::SplineType::WrappedClosed : RS_SplineData::SplineType::ClampedOpen;
    changeType(target);
    calculateBorders();
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
    return RS_VectorSolutions(getUnwrappedControlPoints());
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
    // Clear existing approximation
    clear();

    if (data.controlPoints.size() < static_cast<size_t>(data.degree + 1)) return;

    int segments = 20;  // Adjustable
    std::vector<RS_Vector> points;
    fillStrokePoints(segments, points);

    for (size_t i = 0; i < points.size() - 1; ++i) {
        addEntity(new RS_Line(this, points[i], points[i + 1]));
    }
}

/** Fill points for spline approximation */
void RS_Spline::fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) {
    double minParam = data.knotslist[data.degree];
    double maxParam = data.knotslist[getNumberOfControlPoints()];
    double step = (maxParam - minParam) / splineSegments;

    for (int i = 0; i <= splineSegments; ++i) {
        double t = minParam + i * step;
        points.push_back(getPointAt(t));
    }
}

/** Get start point (invalid if closed) */
RS_Vector RS_Spline::getStartpoint() const {
    if (isClosed()) return RS_Vector(false);
    return getPointAt(data.knotslist[data.degree]);
}

/** Get end point (invalid if closed) */
RS_Vector RS_Spline::getEndpoint() const {
    if (isClosed()) return RS_Vector(false);
    return getPointAt(data.knotslist[getNumberOfControlPoints()]);
}

/** Nearest endpoint or control point */
RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    RS_Vector nearest;
    double minDist = RS_MAXDOUBLE;

    RS_Vector start = getStartpoint();
    double d1 = coord.distanceTo(start);
    if (d1 < minDist) {
        minDist = d1;
        nearest = start;
    }

    RS_Vector end = getEndpoint();
    double d2 = coord.distanceTo(end);
    if (d2 < minDist) {
        minDist = d2;
        nearest = end;
    }

    if (dist) *dist = minDist;
    return nearest;
}

/** Nearest center (invalid) */
RS_Vector RS_Spline::getNearestCenter(const RS_Vector& coord, double* dist) const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

/** Nearest middle point (invalid) */
RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

/** Nearest point at distance (invalid) */
RS_Vector RS_Spline::getNearestDist(double distance, const RS_Vector& coord, double* dist) const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

/** Move by offset */
void RS_Spline::move(const RS_Vector& offset) {
    for (auto& p : data.controlPoints) p += offset;
    for (auto& p : data.fitPoints) p += offset;
    calculateBorders();
}

/** Rotate by angle */
void RS_Spline::rotate(const RS_Vector& center, double angle) {
    for (auto& p : data.controlPoints) p.rotate(center, angle);
    for (auto& p : data.fitPoints) p.rotate(center, angle);
    calculateBorders();
}

/** Rotate by angle vector */
void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    rotate(center, angleVector.angle());
}

/** Scale by factor */
void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& p : data.controlPoints) p.scale(center, factor);
    for (auto& p : data.fitPoints) p.scale(center, factor);
    calculateBorders();
}

/** Shear by factor */
RS_Entity& RS_Spline::shear(double k) {
    for (auto& p : data.controlPoints) {
        p.x += p.y * k;
    }
    for (auto& p : data.fitPoints) {
        p.x += p.y * k;
    }
    calculateBorders();
    update();
    return *this;
}

/** Mirror across axis */
void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& p : data.controlPoints) p.mirror(axisPoint1, axisPoint2);
    for (auto& p : data.fitPoints) p.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

/** Move reference point (control point) */
void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    double minDist = RS_MAXDOUBLE;
    size_t closestIndex = 0;
    auto controls = getControlPoints();
    for (size_t i = 0; i < controls.size(); ++i) {
        double d = ref.distanceTo(controls[i]);
        if (d < minDist) {
            minDist = d;
            closestIndex = i;
        }
    }
    data.controlPoints[closestIndex] += offset;
    if (isClosed()) updateControlAndWeightWrapping();
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
    addControlPointRaw(v, w);
    update();
}

/** Add raw control point */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
    LC_SplineHelper::extendKnotVector(data.knotslist);
}

/** Remove last control point, handling wrapping */
void RS_Spline::removeLastControlPoint() {
    if (data.controlPoints.empty()) return;
    data.controlPoints.pop_back();
    data.weights.pop_back();
    if (!data.knotslist.empty()) data.knotslist.pop_back();
    update();
}

/** Set weight at index */
void RS_Spline::setWeight(size_t index, double w) {
    if (index >= data.weights.size()) return;
    data.weights[index] = w;
}

/** Set all weights */
void RS_Spline::setWeights(const std::vector<double>& weights) {
    data.weights = weights;
}

/** Set control point at index */
void RS_Spline::setControlPoint(size_t index, const RS_Vector& v) {
    if (index >= data.controlPoints.size()) return;
    data.controlPoints[index] = v;
    update();
}

/** Set knot at index */
void RS_Spline::setKnot(size_t index, double k) {
    if (index >= data.knotslist.size()) return;
    data.knotslist[index] = k;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
}

/** Insert control point, clear knots if present */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    if (index > data.controlPoints.size()) return;
    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);
    LC_SplineHelper::insertKnot(data.knotslist, index);
    update();
}

/** Remove control point, clear knots if present */
void RS_Spline::removeControlPoint(size_t index) {
    if (index >= data.controlPoints.size()) return;
    data.controlPoints.erase(data.controlPoints.begin() + index);
    data.weights.erase(data.weights.begin() + index);
    LC_SplineHelper::removeKnot(data.knotslist, index);
    update();
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
    std::vector<double> knotVector(num + order);
    std::iota(knotVector.begin(), knotVector.end(), 0.0 - static_cast<double>(order - 1));
    return knotVector;
}

/** Generate rational B-spline points (open) */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    int degree = k - 1;
    size_t num_control = b.size();
    std::vector<double> knots = knot(num_control, k);
    RS_SplineData temp_data(degree, false);
    temp_data.controlPoints = b;
    temp_data.weights = h;
    temp_data.knotslist = knots;

    p.resize(npts);
    double min_t = knots[degree];
    double max_t = knots[num_control];
    double step = (max_t - min_t) / (npts - 1.0);
    for (size_t i = 0; i < npts; ++i) {
        double t = min_t + i * step;
        p[i] = evaluateNURBS(temp_data, t);
    }
}

/** Generate rational B-spline points (periodic) */
void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    int degree = k - 1;
    size_t num_control = b.size();
    std::vector<double> knots = knotu(num_control, k);
    RS_SplineData temp_data(degree, true);
    temp_data.controlPoints = b;
    temp_data.weights = h;
    temp_data.knotslist = knots;

    p.resize(npts);
    double min_t = knots[degree];
    double max_t = knots[num_control];
    double step = (max_t - min_t) / (npts - 1.0);
    for (size_t i = 0; i < npts; ++i) {
        double t = min_t + i * step;
        p[i] = evaluateNURBS(temp_data, t);
    }
}

/** Check if control points wrapped (for closed cubic splines) */
bool RS_Spline::hasWrappedControlPoints() const {
    size_t n = data.controlPoints.size();
    size_t m = data.degree;
    if (n <= m + 1) return false;
    for (size_t i = 0; i < m; ++i) {
        if (data.controlPoints[n - m + i] != data.controlPoints[i]) return false;
    }
    return true;
}

/** Output operator */
std::ostream& operator<<(std::ostream& os, const RS_Spline& l) {
    os << "RS_Spline: " << l.getData();
    return os;
}

/** Set fit points and generate spline via interpolation */
void RS_Spline::setFitPoints(const std::vector<RS_Vector>& fitPoints, bool useCentripetal) {
    size_t numPoints = fitPoints.size();
    if (numPoints < static_cast<size_t>(data.degree + 1)) return;

    data.fitPoints = fitPoints;
    data.controlPoints.clear();
    data.weights.clear();
    data.knotslist.clear();

    bool closed = (fitPoints.front().distanceTo(fitPoints.back()) < RS_TOLERANCE);

    std::vector<double> params(numPoints, 0.0);
    double total = 0.0;
    for (size_t i = 1; i < numPoints; ++i) {
        double dist = fitPoints[i].distanceTo(fitPoints[i - 1]);
        params[i] = params[i - 1] + (useCentripetal ? std::sqrt(dist) : dist);
        total += params[i] - params[i - 1];
    }
    if (total > RS_TOLERANCE) {
        for (auto& par : params) par /= total;
    }

    data.knotslist = LC_SplineHelper::knot(numPoints, data.degree + 1);

    std::vector<std::vector<double>> N(numPoints, std::vector<double>(numPoints, 0.0));
    for (size_t i = 0; i < numPoints; ++i) {
        N[i] = getBSplineBasis(params[i], data.knotslist, data.degree, numPoints);
    }

    std::vector<double> bx(numPoints), by(numPoints);
    for (size_t i = 0; i < numPoints; ++i) {
        bx[i] = fitPoints[i].x;
        by[i] = fitPoints[i].y;
    }

    std::vector<double> px(numPoints), py(numPoints);
    std::vector<std::vector<double>> mx(numPoints, std::vector<double>(numPoints + 1, 0.0));
    for (size_t i = 0; i < numPoints; ++i) {
        for (size_t j = 0; j < numPoints; ++j) {
            mx[i][j] = N[i][j];
        }
        mx[i][numPoints] = bx[i];
    }
    if (!RS_Math::linearSolver(mx, px)) return;

    mx = std::vector<std::vector<double>>(numPoints, std::vector<double>(numPoints + 1, 0.0));
    for (size_t i = 0; i < numPoints; ++i) {
        for (size_t j = 0; j < numPoints; ++j) {
            mx[i][j] = N[i][j];
        }
        mx[i][numPoints] = by[i];
    }
    if (!RS_Math::linearSolver(mx, py)) return;

    data.controlPoints.clear();
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

/** Change spline type (Standard, ClampedOpen, WrappedClosed) */
void RS_Spline::changeType(RS_SplineData::SplineType newType) {
    if (data.type == newType) return;

    // Pre-conversion validation
    size_t unwrapped = getUnwrappedSize();
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid spline state before type change");
        return;
    }

    auto savedData = data; // Backup for revert

    switch (newType) {
    case RS_SplineData::SplineType::Standard:
        if (data.type == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toStandardFromClampedOpen(data);
        } else if (data.type == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
        }
        break;
    case RS_SplineData::SplineType::ClampedOpen:
        if (data.type == RS_SplineData::SplineType::Standard) {
            LC_SplineHelper::toClampedOpenFromStandard(data);
        } else if (data.type == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
        }
        break;
    case RS_SplineData::SplineType::WrappedClosed:
        if (data.type == RS_SplineData::SplineType::Standard) {
            LC_SplineHelper::toWrappedClosedFromStandard(data);
        } else if (data.type == RS_SplineData::SplineType::ClampedOpen) {
            LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
        }
        break;
    }

    // Post-conversion validation
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid spline state after type change - reverting");
        data = savedData;
        return;
    }

    calculateBorders();
    update();
}

/** Public method to evaluate spline at parameter t */
RS_Vector RS_Spline::getPointAt(double t) const {
    return evaluateNURBS(data, t);
}

/** Find parameters where derivative is zero for x or y */
std::vector<double> RS_Spline::findDerivativeZeros(bool isX) const {
    std::vector<double> zeros;
    double tmin = data.knotslist[data.degree];
    double tmax = data.knotslist[getNumberOfControlPoints()];
    int numIntervals = 50; // Sufficient for degree <=3
    double step = (tmax - tmin) / numIntervals;
    for (int i = 0; i < numIntervals; ++i) {
        double ta = tmin + i * step;
        double tb = ta + step;
        double fa = getDerivative(ta, isX);
        double fb = getDerivative(tb, isX);
        if (std::abs(fa) < 1e-8) zeros.push_back(ta);
        if (std::abs(fb) < 1e-8) zeros.push_back(tb);
        if (fa * fb <= 0) {
            double zero = bisectDerivativeZero(ta, tb, fa, isX);
            zeros.push_back(zero);
        }
    }
    // Deduplicate
    std::sort(zeros.begin(), zeros.end());
    auto last = std::unique(zeros.begin(), zeros.end(), [](double a, double b){ return std::abs(a - b) < 1e-6; });
    zeros.erase(last, zeros.end());
    return zeros;
}

/** Calculate tight bounding box using extrema and endpoints */
void RS_Spline::calculateTightBorders() {
    double tmin = data.knotslist[data.degree];
    double tmax = data.knotslist[getNumberOfControlPoints()];
    RS_Vector pstart = getPointAt(tmin);
    RS_Vector pend = getPointAt(tmax);
    minV = RS_Vector::minimum(pstart, pend);
    maxV = RS_Vector::maximum(pstart, pend);
    auto xzeros = findDerivativeZeros(true);
    auto yzeros = findDerivativeZeros(false);
    for (double t : xzeros) {
        if (t >= tmin - RS_TOLERANCE && t <= tmax + RS_TOLERANCE) {
            RS_Vector p = getPointAt(t);
            minV.x = std::min(minV.x, p.x);
            maxV.x = std::max(maxV.x, p.x);
        }
    }
    for (double t : yzeros) {
        if (t >= tmin - RS_TOLERANCE && t <= tmax + RS_TOLERANCE) {
            RS_Vector p = getPointAt(t);
            minV.y = std::min(minV.y, p.y);
            maxV.y = std::max(maxV.y, p.y);
        }
    }
}

/** Robust NURBS evaluation using de Boor */
RS_Vector RS_Spline::evaluateNURBS(const RS_SplineData& data, double t) {
    size_t p = data.degree;
    size_t n = data.controlPoints.size() - 1;
    const auto& U = data.knotslist;
    if (U.size() != n + p + 1) return RS_Vector(false);

    int span = findSpan(n, p, t, U);
    if (span < 0) return RS_Vector(false);

    std::vector<double> N = basisFunctions(span, t, p, U);

    double w = 0.0;
    RS_Vector point(0.0, 0.0);
    for (size_t j = 0; j <= p; ++j) {
        size_t idx = span - p + j;
        if (idx >= data.controlPoints.size()) return RS_Vector(false);
        double tmp = N[j] * data.weights[idx];
        w += tmp;
        point += data.controlPoints[idx] * tmp;
    }
    if (std::abs(w) < RS_TOLERANCE) return RS_Vector(false);
    return point / w;
}

/** Find knot span */
int RS_Spline::findSpan(int n, int p, double u, const std::vector<double>& U) {
    if (u >= U[n + 1] - RS_TOLERANCE) return n;
    if (u <= U[p] + RS_TOLERANCE) return p;

    int low = p;
    int high = n + 1;
    int mid = (low + high) / 2;
    while (u < U[mid] - RS_TOLERANCE || u >= U[mid + 1] + RS_TOLERANCE) {
        if (u < U[mid] - RS_TOLERANCE) high = mid;
        else low = mid;
        mid = (low + high) / 2;
    }
    return mid;
}

/** Compute basis functions non-recursively */
std::vector<double> RS_Spline::basisFunctions(int i, double u, int p, const std::vector<double>& U) {
    std::vector<double> N(p+1);
    std::vector<double> left(p + 1), right(p + 1);

    N[0] = 1.0;

    for (int j = 1; j <= p; ++j) {
        left[j] = u - U[i + 1 - j];
        right[j] = U[i + j] - u;
        double saved = 0.0;
        for (int r = 0; r < j; ++r) {
            double den = right[r + 1] + left[j - r];
            double temp = (std::abs(den) > RS_TOLERANCE) ? N[r] / den : 0.0;
            N[r] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        N[j] = saved;
    }
    return N;
}

/** Compute basis derivatives (from NURBS book A2.3) */
void RS_Spline::dersBasisFunctions(int span, double u, int p, int derOrder, const std::vector<double>& U, std::vector<std::vector<double>>& ders) {
    ders.assign(derOrder + 1, std::vector<double>(p + 1, 0.0));
    std::vector<double> left(p + 1, 0.0), right(p + 1, 0.0);
    std::vector<std::vector<double>> ndu(p + 1, std::vector<double>(p + 1, 0.0));
    std::vector<std::vector<double>> a(2, std::vector<double>(p + 1, 0.0));

    ndu[0][0] = 1.0;
    for (int j = 1; j <= p; ++j) {
        left[j] = u - U[span + 1 - j];
        right[j] = U[span + j] - u;
        double saved = 0.0;
        for (int r = 0; r < j; ++r) {
            ndu[j][r] = right[r + 1] + left[j - r];
            double temp = 0.0;
            if (std::abs(ndu[j][r]) > RS_TOLERANCE) {
                temp = ndu[r][j - 1] / ndu[j][r];
            }
            ndu[r][j] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        ndu[j][j] = saved;
    }
    for (int j = 0; j <= p; ++j) {
        ders[0][j] = ndu[j][p];
    }

    for (int r = 0; r <= p; ++r) {
        int s1 = 0, s2 = 1;
        a[0][0] = 1.0;
        for (int k = 1; k <= derOrder; ++k) {
            double d = 0.0;
            int rk = r - k, pk = p - k;
            if (r >= k) {
                double den = ndu[pk + 1][rk];
                a[s2][0] = (std::abs(den) > RS_TOLERANCE) ? a[s1][0] / den : 0.0;
                d = a[s2][0] * ndu[rk][pk];
            }
            int j1 = (rk >= -1) ? 1 : -rk;
            int j2 = (r - 1 <= pk) ? k - 1 : p - r;
            for (int j = j1; j <= j2; ++j) {
                double den = ndu[pk + 1][rk + j];
                a[s2][j] = (std::abs(den) > RS_TOLERANCE) ? (a[s1][j] - a[s1][j - 1]) / den : 0.0;
                d += a[s2][j] * ndu[rk + j][pk];
            }
            if (r <= pk) {
                double den = ndu[pk + 1][r];
                a[s2][k] = (std::abs(den) > RS_TOLERANCE) ? -a[s1][k - 1] / den : 0.0;
                d += a[s2][k] * ndu[r][pk];
            }
            ders[k][r] = d;
            std::swap(s1, s2);
        }
    }
    int rr = p;
    for (int k = 1; k <= derOrder; ++k) {
        for (int j = 0; j <= p; ++j) {
            ders[k][j] *= rr;
        }
        rr *= (p - k);
    }
}

/** Get tangent vector at t */
RS_Vector RS_Spline::getTangentAt(double t) const {
    size_t p = data.degree;
    size_t n = data.controlPoints.size() - 1;
    const auto& U = data.knotslist;
    if (U.size() != n + p + 1) return RS_Vector(false);
    int span = findSpan(n, p, t, U);
    if (span < 0) return RS_Vector(false);
    std::vector<std::vector<double>> ders;
    dersBasisFunctions(span, t, p, 1, U, ders);
    RS_Vector C(0.0, 0.0), Cprime(0.0, 0.0);
    double w = 0.0, wprime = 0.0;
    for (size_t j = 0; j <= p; ++j) {
        size_t idx = span - p + j;
        double Ni = ders[0][j];
        double Nip = ders[1][j];
        double tmp = Ni * data.weights[idx];
        double tmp_p = Nip * data.weights[idx];
        C += data.controlPoints[idx] * tmp;
        Cprime += data.controlPoints[idx] * tmp_p;
        w += tmp;
        wprime += tmp_p;
    }
    if (std::abs(w) < RS_TOLERANCE) return RS_Vector(false);
    return (Cprime * w - C * wprime) / (w * w);
}

/** Get derivative component at t */
double RS_Spline::getDerivative(double t, bool isX) const {
    RS_Vector tan = getTangentAt(t);
    if (!tan.valid) return 0.0;
    return isX ? tan.x : tan.y;
}

/** Bisection to find zero of derivative */
double RS_Spline::bisectDerivativeZero(double a, double b, double fa, bool isX) const {
    double tol = 1e-6;
    double fb = getDerivative(b, isX);
    while (b - a > tol) {
        double m = (a + b) / 2.0;
        double fm = getDerivative(m, isX);
        if (std::abs(fm) < tol) return m;
        if (fm * fa > 0) {
            a = m;
            fa = fm;
        } else {
            b = m;
            fb = fm;
        }
    }
    return (a + b) / 2.0;
}


/** Validate the spline data integrity */
bool RS_Spline::validate() const {
    return LC_SplineHelper::validate(data, getUnwrappedSize());
}

void RS_Spline::resetBorders() {
    minV = RS_Vector(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    maxV = RS_Vector(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());
}

std::vector<double> RS_Spline::rbasis(int c, double t, int npts,
                                      const std::vector<double>& x,
                                      const std::vector<double>& h) {
    std::vector<double> nbasis(npts, 0.0);
    int p = c - 1;
    int span = findSpan(npts - 1, p, t, x);
    std::vector<double> N = basisFunctions(span, t, p, N);
    double sum = 0.0;
    for (int j = 0; j <= p; ++j) {
        int idx = span - p + j;
        nbasis[idx] = N[j] * h[idx];
        sum += nbasis[idx];
    }
    if (sum > 0.0) {
        for (int i = 0; i < npts; ++i) {
            nbasis[i] /= sum;
        }
    }
    return nbasis;
}
