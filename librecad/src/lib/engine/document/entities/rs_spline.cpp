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
#include "rs_spline.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"

RS_SplineData::RS_SplineData(int _degree, bool _closed):
    degree(_degree)
    ,closed(_closed)
{
}

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld) {
    os << "( degree: " << ld.degree <<
        " closed: " << ld.closed;
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
    os  << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Spline::RS_Spline(RS_EntityContainer* parent,
                     const RS_SplineData& d)
    :RS_EntityContainer(parent), data(d)
{
    if (data.closed && !hasWrappedControlPoints()) {
        addWrapping();
    }
    calculateBorders();
}

RS_Entity* RS_Spline::clone() const{
    auto* l = new RS_Spline(*this);
    l->setOwner(isOwner());
    l->detach();
    return l;
}

RS_SplineData& RS_Spline::getData() {
    return data;
}

const RS_SplineData& RS_Spline::getData() const {
    return data;
}

size_t RS_Spline::getUnwrappedSize() const {
    size_t n = data.controlPoints.size();
    if (data.closed && hasWrappedControlPoints()) {
        if (n > static_cast<size_t>(data.degree)) {
            n -= data.degree;
        } else {
            n = 0; // Invalid wrapping
        }
    }
    return n;
}

std::vector<RS_Vector> RS_Spline::getUnwrappedControlPoints() const {
    size_t n = getUnwrappedSize();
    if (n == 0) return {};
    return {data.controlPoints.begin(), data.controlPoints.begin() + n};
}

std::vector<double> RS_Spline::getUnwrappedWeights() const {
    size_t n = getUnwrappedSize();
    if (n == 0) return {};
    return {data.weights.begin(), data.weights.begin() + n};
}

std::vector<double> RS_Spline::getUnwrappedKnotVector() const {
    size_t n = getUnwrappedSize();
    if (n == 0) return {};
    size_t base_size = n + static_cast<size_t>(data.degree) + 1;
    if (base_size > data.knotslist.size()) return {};
    return {data.knotslist.begin(), data.knotslist.begin() + base_size};
}

void RS_Spline::removeWrapping() {
    size_t wrap_size = static_cast<size_t>(data.degree);
    if (!data.closed || !hasWrappedControlPoints())
        return;
    data.controlPoints.resize(data.controlPoints.size() - wrap_size);
    data.weights.resize(data.weights.size() - wrap_size);
    if (data.knotslist.size() > wrap_size) {
        data.knotslist.resize(data.knotslist.size() - wrap_size);
    }
    data.wrapped = false;
}

void RS_Spline::addWrapping() {
    size_t wrap_size = static_cast<size_t>(data.degree);
    size_t base_n = data.controlPoints.size();
    if (!data.closed || base_n < wrap_size || data.wrapped)
        return;
    data.controlPoints.insert(data.controlPoints.end(), data.controlPoints.begin(), data.controlPoints.begin() + wrap_size);
    data.weights.insert(data.weights.end(), data.weights.begin(), data.weights.begin() + wrap_size);
    updateKnotWrapping();
    data.wrapped = true;
}

void RS_Spline::updateControlAndWeightWrapping() {
    size_t wrap_size = static_cast<size_t>(data.degree);
    size_t base_n = getUnwrappedSize();
    data.controlPoints.resize(base_n);
    data.weights.resize(base_n);
    if (base_n < 3 || base_n < wrap_size) return;
    data.controlPoints.insert(data.controlPoints.end(), data.controlPoints.begin(), data.controlPoints.begin() + wrap_size);
    data.weights.insert(data.weights.end(), data.weights.begin(), data.weights.begin() + wrap_size);
    data.wrapped = true;
}

void RS_Spline::updateKnotWrapping() {
    if (data.knotslist.empty()) {
        data.knotslist = data.closed ? knotu(data.controlPoints.size(), data.degree + 1) :
                             knot(data.controlPoints.size(), data.degree + 1);
    }
    if (!data.closed || data.knotslist.empty()) return;
    size_t base_n = getUnwrappedSize();
    size_t base_k_size = base_n + static_cast<size_t>(data.degree) + 1;
    data.knotslist.resize(base_k_size);
    double last = data.knotslist.back();
    double first = data.knotslist.front();
    double span = last - first;
    size_t wrap_size = static_cast<size_t>(data.degree);
    if (base_k_size <= wrap_size + 1) {  // +1 for safety, need at least wrap_size deltas
        // Fallback to average if not enough knots
        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;  // Default
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {  // non-zero delta
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;  // Update last non-zero for better local preservation
            }
        }
        double delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        // Use last_non_zero_delta if multiplicity at end, else average
        if (std::fabs(data.knotslist.back() - data.knotslist[base_k_size - 2]) < 1e-10) {
            delta = last_non_zero_delta;  // Preserve local spacing near closure
        }
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "Extending knots with delta: %f (average: %f, last: %f)", delta, total_delta / count_delta, last_non_zero_delta);
        double current = last;
        for (size_t i = 0; i < wrap_size; ++i) {
            current += delta;
            data.knotslist.push_back(current);
        }
    } else {
        // Shift deltas method for non-uniform periodic extension
        double current = last;
        for (size_t i = 0; i < wrap_size; ++i) {
            double delta = data.knotslist[i + 1] - data.knotslist[i];
            current += delta;
            data.knotslist.push_back(current);
        }
    }
}

void RS_Spline::calculateBorders() {
    resetBorders();
    size_t n = getUnwrappedSize();
    if (n == 0)
        return;
    for (size_t i = 0; i < n; ++i) {
        minV = RS_Vector::minimum(data.controlPoints[i], minV);
        maxV = RS_Vector::maximum(data.controlPoints[i], maxV);
    }
}

void RS_Spline::setDegree(int degree) {
    if (degree >= 1 && degree <= 3) {
        data.degree = degree;
    } else {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "%s(%d): invalid degree = %d", __func__, degree, degree);
        throw std::invalid_argument("Degree must be 1, 2, or 3");
    }
}

/** @return Degree of this spline curve (1-3).*/
int RS_Spline::getDegree() const {
    return data.degree;
}

size_t RS_Spline::getNumberOfControlPoints() const {
    return getUnwrappedSize();
}

size_t RS_Spline::getNumberOfKnots() const {
    return getUnwrappedSize() + static_cast<size_t>(data.degree) + 1;
}

/**
 * @retval true if the spline is closed.
 * @retval false otherwise.
 */
bool RS_Spline::isClosed() const {
    return data.closed;
}

/**
 * Sets the closed flag of this spline.
 */
void RS_Spline::setClosed(bool c) {
    if (data.closed == c) return;

    size_t n = getUnwrappedSize();
    if ((!c && n < static_cast<size_t>(data.degree + 1)) || (c && n < 3)) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::setClosed: Not enough control points");
        return;
    }

    removeWrapping();
    data.knotslist.clear();
    data.closed = c;
    if (data.closed) {
        addWrapping();
    } else {
        data.knotslist = knot(n, data.degree + 1);
    }
    update();
}

RS_VectorSolutions RS_Spline::getRefPoints() const {
    return {getUnwrappedControlPoints()};
}

RS_Vector RS_Spline::getNearestRef(const RS_Vector& coord,
                                   double* dist /*= nullptr*/) const{
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector& coord,
                                           double* dist /*= nullptr*/) const{
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

/**
 * Updates the internal polygon of this spline. Called when the
 * spline or it's data, position, .. changes.
 */
void RS_Spline::update() {
    RS_DEBUG->print("RS_Spline::update");

    clear();

    if (isUndone()) {
        return;
    }

    if (isClosed()) {
        addWrapping();
    } else {
        removeWrapping();
    }

    if (data.degree < 1 || data.degree > 3) {
        RS_DEBUG->print("RS_Spline::update: invalid degree: %d", data.degree);
        return;
    }

    // Issue #1689: allow closed splines by 3 control points
    // Issue #1960: DXF import of degree 1 spline with two control points fails in RC 2.2.1_rc4 on windows and MacOS
    size_t n = getUnwrappedSize();
    if ((!data.closed && n < static_cast<size_t>(data.degree) + 1) || (data.closed && n < 3)) {
        RS_DEBUG->print("RS_Spline::update: not enough control points");
        return;
    }

    resetBorders();

    int splineSegments = getGraphicVariableInt("$SPLINESEGS", 8);
    std::vector<RS_Vector> p;
    fillStrokePoints(splineSegments, p);

    RS_Vector prev{};
    for (auto const& vp: p) {
        if (prev.valid) {
            auto* line = new RS_Line{this, prev, vp};
            line->setLayer(nullptr);
            line->setPen(RS_Pen(RS2::FlagInvalid));
            addEntity(line);
        }
        prev = vp;
        minV = RS_Vector::minimum(prev, minV);
        maxV = RS_Vector::maximum(prev, maxV);
    }

    if (isClosed() && !p.empty()) {
        auto* closingLine = new RS_Line{this, p.back(), p.front()};
        closingLine->setLayer(nullptr);
        closingLine->setPen(RS_Pen(RS2::FlagInvalid));
        addEntity(closingLine);
    }
}

void RS_Spline::fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) {
    size_t base_n = data.closed ? getUnwrappedSize() : data.controlPoints.size();
    const size_t pointsCount = splineSegments * base_n;
    const size_t npts = data.controlPoints.size();
    const size_t k = data.degree + 1;

    std::vector<double> h = data.weights;
    points.resize(pointsCount);
    std::fill(points.begin(), points.end(), RS_Vector{0., 0.});
    if (data.closed) {
        rbsplinu(npts, k, pointsCount, data.controlPoints, h, points);
    } else {
        rbspline(npts, k, pointsCount, data.controlPoints, h, points);
    }
}

RS_Vector RS_Spline::getStartpoint() const {
    if (data.closed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->firstEntity())->getStartpoint();
}

RS_Vector RS_Spline::getEndpoint() const {
    if (data.closed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->lastEntity())->getEndpoint();
}

RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector ret(false);
    if (!data.closed) { // no endpoint for closed spline
        RS_Vector vp1(getStartpoint());
        RS_Vector vp2(getEndpoint());
        double d1((coord - vp1).squared());
        double d2((coord - vp2).squared());
        if (d1 < d2) {
            ret = vp1;
            minDist = std::sqrt(d1);
        } else {
            ret = vp2;
            minDist = std::sqrt(d2);
        }
        size_t n = getUnwrappedSize();
        for (size_t i = 0; i < n; i++) {
            double d = (data.controlPoints.at(i)).distanceTo(coord);
            if (d < minDist) {
                minDist = d;
                ret = data.controlPoints.at(i);
            }
        }
    }
    if (dist) {
        *dist = minDist;
    }
    return ret;
}

RS_Vector RS_Spline::getNearestPointOnEntity(const RS_Vector &coord, bool onEntity, double *dist, RS_Entity **entity) const {
    return RS_EntityContainer::getNearestPointOnEntity(coord, onEntity, dist, entity);
}

RS_Vector RS_Spline::getNearestCenter(const RS_Vector& /*coord*/,
                                      double* dist) const{
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& /*coord*/,
                                      double* dist,
                                      int /*middlePoints*/) const {
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_Spline::getNearestDist(double /*distance*/,
                                    const RS_Vector& /*coord*/,
                                    double* dist) const{
    if (dist)
        *dist = RS_MAXDOUBLE;
    return {};
}

void RS_Spline::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    for (RS_Vector& vp: data.controlPoints) {
        vp += offset;
    }
}

void RS_Spline::rotate(const RS_Vector& center, double angle) {
    rotate(center, RS_Vector(angle));
}

void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    for (RS_Vector& vp: data.controlPoints) {
        vp.rotate(center, angleVector);
    }
}

void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (RS_Vector& vp: data.controlPoints) {
        vp.scale(center, factor);
    }
    update();
}

RS_Entity& RS_Spline::shear(double k){
    for (RS_Vector& vp: data.controlPoints) {
        vp.shear(k);
    }
    update();
    return *this;
}

void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (RS_Vector& vp: data.controlPoints) {
        vp.mirror(axisPoint1, axisPoint2);
    }
    update();
}

void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    size_t n = getUnwrappedSize();
    for (size_t i = 0; i < n; ++i) {
        if (ref.distanceTo(data.controlPoints[i]) < 1.0e-4) {
            data.controlPoints[i] += offset;
        }
    }
    if (data.closed) {
        updateControlAndWeightWrapping();
    }
    update();
}

void RS_Spline::revertDirection() {
    removeWrapping();
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    std::reverse(data.weights.begin(), data.weights.end());
    if (!data.knotslist.empty()) {
        std::vector<double> rev_knots(data.knotslist.size());
        double max_k = data.knotslist.back();
        for (size_t i = 0; i < data.knotslist.size(); ++i) {
            rev_knots[i] = max_k - data.knotslist[data.knotslist.size() - 1 - i];
        }
        data.knotslist = rev_knots;
    }
    addWrapping();
}

void RS_Spline::draw(RS_Painter* painter) {
    painter->drawSplineWCS(*this);
}

/**
 * @return The reference points of the spline.
 */
const std::vector<RS_Vector>& RS_Spline::getControlPoints() const {
    static std::vector<RS_Vector> unwrappedPoints;
    unwrappedPoints = getUnwrappedControlPoints();
    return unwrappedPoints;
}

/**
 * @return The weights of the control points.
 */
const std::vector<double>& RS_Spline::getWeights() const {
    static std::vector<double> unwrappedWeights;
    unwrappedWeights = getUnwrappedWeights();
    return unwrappedWeights;
}

double RS_Spline::getWeight(size_t index) const {
    size_t n = getUnwrappedSize();
    if (index < n) {
        return data.weights[index];
    }
    return 1.0; // Default weight
}

/**
 * Appends the given point to the control points.
 */
void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    removeWrapping();
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
    addWrapping();
}
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
}

/**
 * Removes the control point that was last added.
 */
void RS_Spline::removeLastControlPoint() {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (n > 0) {
        data.controlPoints.pop_back();
        data.weights.pop_back();
    }
    addWrapping();
}

void RS_Spline::setWeight(size_t index, double w) {
    size_t n = getUnwrappedSize();
    if (index < n) {
        data.weights[index] = w;
        update();
    }
}

void RS_Spline::setWeights(const std::vector<double>& weights) {
    size_t n = getUnwrappedSize();
    if (weights.size() != n) {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "RS_Spline::setWeights: size mismatch between weights and control points");
        return;
    }
    data.weights = weights;
    update();
}

void RS_Spline::setControlPoint(size_t index, const RS_Vector& v) {
    size_t n = getUnwrappedSize();
    if (index < n) {
        data.controlPoints[index] = v;
        if (data.closed) {
            updateControlAndWeightWrapping();
        }
        update();
    }
}

void RS_Spline::setKnot(size_t index, double k) {
    size_t base_k_size = getUnwrappedSize() + data.degree + 1;
    if (index < base_k_size) {
        data.knotslist[index] = k;
        if (data.closed) {
            updateKnotWrapping();
        }
        update();
    }
}

void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (index > n) index = n;
    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);
    if (!data.knotslist.empty()) data.knotslist.clear();
    addWrapping();
    update();
}

void RS_Spline::removeControlPoint(size_t index) {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (index < n) {
        data.controlPoints.erase(data.controlPoints.begin() + index);
        if (index < data.weights.size()) {
            data.weights.erase(data.weights.begin() + index);
        }
        if (!data.knotslist.empty()) data.knotslist.clear();
        addWrapping();
        update();
    }
}

const std::vector<double>& RS_Spline::getKnotVector() const {
    static std::vector<double> unwrappedKnots;
    unwrappedKnots = getUnwrappedKnotVector();
    return unwrappedKnots;
}

void RS_Spline::setKnotVector(const std::vector<double>& knots) {
    size_t n = getUnwrappedSize();
    size_t baseSize = n + static_cast<size_t>(data.degree) + 1;
    if (knots.size() != baseSize) {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "RS_Spline::setKnotVector: expected %zu knots, got %zu",
                        baseSize, knots.size());
        throw std::invalid_argument("Knot vector size mismatch");
    }
    for (size_t i = 1; i < knots.size(); ++i) {
        if (knots[i] < knots[i - 1]) {
            RS_DEBUG->print(RS_Debug::D_CRITICAL, "RS_Spline::setKnotVector: non-decreasing knot vector required");
            throw std::invalid_argument("Knot vector must be non-decreasing");
        }
    }
    data.knotslist = knots;
    if (data.closed) {
        updateKnotWrapping();
    }
    update();
}

/**
 * Generates B-Spline open knot vector with multiplicity
 * equal to the order at the ends.
 */
std::vector<double> RS_Spline::knot(size_t num, size_t order) const {
    if (data.knotslist.size() == num + order) {
        //use custom knot vector
        return data.knotslist;
    }

    std::vector<double> knotVector(num + order, 0.);
    //use uniform knots
    std::iota(knotVector.begin() + order, knotVector.begin() + num + 1, 1);
    std::fill(knotVector.begin() + num + 1, knotVector.end(), knotVector[num]);
    return knotVector;
}

/**
 * Generates rational B-spline basis functions for an open knot vector.
 */
namespace {
std::vector<double> rbasis(int c, double t, int npts,
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
            if (temp[i] != 0) {
                double den = x[i+k-1]-x[i];
                if (den != 0.0) {
                    temp[i] = ((t-x[i])*temp[i])/den;
                } else {
                    temp[i] = 0.0;
                }
            }
            // if the lower order basis function is zero skip the calculation
            if (temp[i+1] != 0) {
                double den = x[i+k]-x[i+1];
                if (den != 0.0) {
                    temp[i] += ((x[i+k]-t)*temp[i+1])/den;
                }
            }
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
}

/**
 * Generates a rational B-spline curve using a uniform open knot vector.
 */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const{
    size_t const nplusc = npts + k;

    // generate the open knot vector
    auto const x = knot(npts, k);

    // calculate the points on the rational B-spline curve
    size_t degree = k - 1;
    double t = x[degree];
    double const step = (x[npts] - t) / (p1 - 1);

    for (auto& vp: p) {
        if (x[npts] - t < 5e-6) t = x[npts];

        // generate the basis function for this value of t
        auto const nbasis = rbasis(k, t, npts, x, h);

        // generate a point on the curve
        for (size_t i = 0; i < npts; i++)
            vp += b[i] * nbasis[i];

        t += step;
    }
}

std::vector<double> RS_Spline::knotu(size_t num, size_t order) const{
    if (data.knotslist.size() == num + order) {
        //use custom knot vector
        return data.knotslist;
    }
    std::vector<double> knotVector(num + order, 0.);
    //use uniform knots
    std::iota(knotVector.begin(), knotVector.begin() + num + order, 0);
    if (data.closed && hasWrappedControlPoints()) {
        // Extend with degree additional knots for wrapping
        knotVector.resize(num + order + data.degree);
        double delta = (num + order > 1) ? 1.0 : 1.0; // Use uniform spacing
        for (int i = 0; i < data.degree; ++i) {
            knotVector[num + order + i] = knotVector[num + order - 1] + (i + 1) * delta;
        }
    }
    return knotVector;
}

void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const{
    size_t const nplusc = npts + k;

    /* generate the periodic knot vector */
    std::vector<double> const x = knotu(npts, k);

    /* calculate the points on the rational B-spline curve */
    size_t degree = k - 1;
    double t = x[degree];
    double const step = (x[npts] - t) / (p1 - 1);

    for (auto& vp: p) {
        if (x[npts] - t < 5e-6) t = x[npts];

        /* generate the basis function for this value of t */
        auto const nbasis = rbasis(k, t, npts, x, h);
        /* generate a point on the curve, for x, y, z */
        for (size_t i = 0; i < npts; i++)
            vp += b[i] * nbasis[i];

        t += step;
    }
}

/**
 * Dumps the spline's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}

/**
 * @brief hasWrappedControlPoints whether the control points are wrapped, needed for a closed spline.
 *          only implemented for cubic splines
 * @return bool - true, if the control points are already wrapped.
 *          for a cubic spline with wrapped splines, the last three control points are the same as the first three.
 */
bool RS_Spline::hasWrappedControlPoints() const {
    return data.wrapped;
}
