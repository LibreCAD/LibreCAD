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

/** Constructor for RS_SplineData with degree and closed flag. */
RS_SplineData::RS_SplineData(int _degree, bool _closed):
    degree(_degree)
    ,closed(_closed)
{
}

/** Stream operator for RS_SplineData. */
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

/** Constructor for RS_Spline; adds wrapping if closed and calculates borders. */
RS_Spline::RS_Spline(RS_EntityContainer* parent,
                     const RS_SplineData& d)
    :RS_EntityContainer(parent), data(d)
{
    if (data.closed && !hasWrappedControlPoints()) {
        addWrapping();
    }
    calculateBorders();
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

/** Get unwrapped control points. */
std::vector<RS_Vector> RS_Spline::getUnwrappedControlPoints() const {
    size_t n = getUnwrappedSize();
    if (n == 0) return {};
    return {data.controlPoints.begin(), data.controlPoints.begin() + n};
}

/** Get unwrapped weights. */
std::vector<double> RS_Spline::getUnwrappedWeights() const {
    size_t n = getUnwrappedSize();
    if (n == 0) return {};
    return {data.weights.begin(), data.weights.begin() + n};
}

/** Get unwrapped knot vector. */
std::vector<double> RS_Spline::getUnwrappedKnotVector() const {
    size_t n = getUnwrappedSize();
    if (n == 0)
        return {};
    size_t base_size = n + static_cast<size_t>(data.degree) + 1;
    if (base_size > data.knotslist.size())
        return {};
    return {data.knotslist.begin(), data.knotslist.begin() + base_size};
}

/** Remove wrapping from control points, weights, and knots. */
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

/** Add wrapping to control points and weights for closed splines. */
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

/** Update wrapping for control points and weights. */
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

/** Update knot vector wrapping for closed splines. */
void RS_Spline::updateKnotWrapping() {
    if (!data.closed || data.knotslist.empty())
        return;

    size_t base_n = getUnwrappedSize();
    size_t base_k_size = base_n + static_cast<size_t>(data.degree) + 1;
    data.knotslist.resize(base_k_size);
    double last = data.knotslist.back();
    double first = data.knotslist.front();
    size_t wrap_size = static_cast<size_t>(data.degree);

    // Compute fallback delta
    double total_delta = 0.0;
    int count_delta = 0;
    double last_non_zero_delta = 1.0;
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
        double dd = data.knotslist[i] - data.knotslist[i - 1];
        if (dd > 1e-10) {
            total_delta += dd;
            count_delta++;
            last_non_zero_delta = dd;
        }
    }
    double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
    if (std::fabs(data.knotslist.back() - data.knotslist[base_k_size - 2]) < 1e-10) {
        fallback_delta = last_non_zero_delta;
    }

    if (base_k_size <= wrap_size + 1) {
        // Fallback extension
        double current = last;
        for (size_t i = 0; i < wrap_size; ++i) {
            current += fallback_delta;
            data.knotslist.push_back(current);
        }
    } else {
        // Shift deltas with fallback
        double current = last;
        for (size_t i = 0; i < wrap_size; ++i) {
            double delta = data.knotslist[i + 1] - data.knotslist[i];
            if (delta <= 1e-10) delta = fallback_delta;
            current += delta;
            data.knotslist.push_back(current);
        }
    }
}

/** Calculate bounding box from control points. */
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

/** Set degree (1-3), throws if invalid. */
void RS_Spline::setDegree(int degree) {
    if (degree >= 1 && degree <= 3) {
        data.degree = degree;
    } else {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "%s(%d): invalid degree = %d", __func__, degree, degree);
        throw std::invalid_argument("Degree must be 1, 2, or 3");
    }
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
    return getUnwrappedSize() + static_cast<size_t>(data.degree) + 1;
}

/** Check if closed. */
bool RS_Spline::isClosed() const {
    return data.closed;
}

/** Set closed flag and adjust wrapping/knots. */
void RS_Spline::setClosed(bool c) {
    if (data.closed == c) return;

    size_t n = getUnwrappedSize();
    if ((!c && n < static_cast<size_t>(data.degree + 1)) || (c && n < 3)) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::setClosed: Not enough control points");
        return;
    }

    removeWrapping();
    data.closed = c;

    size_t base_k_size = n + static_cast<size_t>(data.degree) + 1;

    if (data.closed) {

        // Follow TinySpline: force uniform non-clamped knots for closed
        data.knotslist.resize(base_k_size);
        size_t order = data.degree + 1;
        double delta = 1.0 / static_cast<double>(n);
        for (size_t i = 0; i < base_k_size; ++i) {
            data.knotslist[i] = static_cast<double>(i) * delta;
        }

        addWrapping();
        updateKnotWrapping();
    } else {

        // For open, force clamping at both ends
        if (data.knotslist.empty()) {
            data.knotslist = knot(n, data.degree + 1);
        } else {
            // Clamp both start and end
            double first = data.knotslist.front();
            double last = data.knotslist.back();
            size_t order = static_cast<size_t>(data.degree + 1);
            for (size_t i = 1; i < order; ++i) {
                data.knotslist[i] = first;
                data.knotslist[base_k_size - i - 1] = last;
            }
        }
    }

    update();
}

/** Get reference points (unwrapped control points). */
RS_VectorSolutions RS_Spline::getRefPoints() const {
    return {getUnwrappedControlPoints()};
}

/** Nearest reference point (overrides container method). */
RS_Vector RS_Spline::getNearestRef(const RS_Vector& coord,
                                   double* dist /*= nullptr*/) const{
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestRef(coord, dist);
}

/** Nearest selected reference (overrides container method). */
RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector& coord,
                                           double* dist /*= nullptr*/) const{
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

/** Update polyline approximation. */
void RS_Spline::update() {
    RS_DEBUG->print("RS_Spline::update");

    clear();

    if (isUndone()) {
        return;
    }

    if (data.closed) {
        addWrapping();
    } else {
        removeWrapping();
    }

    if (data.degree < 1 || data.degree > 3) {
        RS_DEBUG->print("RS_Spline::update: invalid degree: %d", data.degree);
        return;
    }

    // Allow closed splines with 3 points (Issue #1689)
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

    if (isClosed() && !p.empty() && p.back().squaredTo(p.front()) > RS_TOLERANCE2) {
        auto* closingLine = new RS_Line{this, p.back(), p.front()};
        closingLine->setLayer(nullptr);
        closingLine->setPen(RS_Pen(RS2::FlagInvalid));
        addEntity(closingLine);
    }
}

/** Fill points for spline approximation. */
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

/** Get start point (invalid if closed). */
RS_Vector RS_Spline::getStartpoint() const {
    if (data.closed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->firstEntity())->getStartpoint();
}

/** Get end point (invalid if closed). */
RS_Vector RS_Spline::getEndpoint() const {
    if (data.closed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->lastEntity())->getEndpoint();
}

/** Nearest endpoint or control point. */
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

/** Nearest center (invalid). */
RS_Vector RS_Spline::getNearestCenter(const RS_Vector& /*coord*/,
                                      double* dist) const{
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

/** Nearest middle point (invalid). */
RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& /*coord*/,
                                      double* dist,
                                      int /*middlePoints*/) const {
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

/** Nearest point at distance (invalid). */
RS_Vector RS_Spline::getNearestDist(double /*distance*/,
                                    const RS_Vector& /*coord*/,
                                    double* dist) const{
    if (dist)
        *dist = RS_MAXDOUBLE;
    return {};
}

/** Move by offset. */
void RS_Spline::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    for (RS_Vector& vp: data.controlPoints) {
        vp += offset;
    }
}

/** Rotate by angle. */
void RS_Spline::rotate(const RS_Vector& center, double angle) {
    rotate(center, RS_Vector(angle));
}

/** Rotate by angle vector. */
void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    for (RS_Vector& vp: data.controlPoints) {
        vp.rotate(center, angleVector);
    }
}

/** Scale by factor. */
void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (RS_Vector& vp: data.controlPoints) {
        vp.scale(center, factor);
    }
    update();
}

/** Shear by factor. */
RS_Entity& RS_Spline::shear(double k){
    for (RS_Vector& vp: data.controlPoints) {
        vp.shear(k);
    }
    update();
    return *this;
}

/** Mirror across axis. */
void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (RS_Vector& vp: data.controlPoints) {
        vp.mirror(axisPoint1, axisPoint2);
    }
    update();
}

/** Move reference point (control point). */
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

/** Revert direction by reversing points, weights, knots. */
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

/** Draw spline with painter. */
void RS_Spline::draw(RS_Painter* painter) {
    painter->drawSplineWCS(*this);
}

/** Get control points (unwrapped). */
std::vector<RS_Vector> RS_Spline::getControlPoints() const {
    return getUnwrappedControlPoints();
}

/** Get weights (unwrapped). */
std::vector<double> RS_Spline::getWeights() const {
    return getUnwrappedWeights();
}

/** Get weight at index. */
double RS_Spline::getWeight(size_t index) const {
    size_t n = getUnwrappedSize();
    if (index < n) {
        return data.weights[index];
    }
    return 1.0; // Default weight
}

/** Add control point with weight, handling wrapping. */
void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    removeWrapping();

    size_t old_size = data.controlPoints.size();

    bool was_empty = data.knotslist.empty();

    bool was_uniform = false;

    size_t expected_k_size = old_size + static_cast<size_t>(data.degree) + 1;

    if (!was_empty && data.knotslist.size() == expected_k_size) {
        auto gen_knots = data.closed ? knotu(old_size, data.degree + 1) : knot(old_size, data.degree + 1);
        if (data.knotslist == gen_knots) {
            was_uniform = true;
        }
    }

    data.controlPoints.push_back(v);
    data.weights.push_back(w);

    size_t new_size = data.controlPoints.size();

    size_t min_size = data.closed ? 3 : static_cast<size_t>(data.degree + 1);
    if (new_size < min_size) {
        addWrapping();
        return;
    }

    if (was_empty || was_uniform) {
        data.knotslist = data.closed ? knotu(new_size, data.degree + 1) : knot(new_size, data.degree + 1);
    } else {
        // custom non-uniform, append with fallback delta
        double last = data.knotslist.back();

        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;
            }
        }
        double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        if (std::fabs(data.knotslist.back() - data.knotslist[expected_k_size - 2]) < 1e-10) {
            fallback_delta = last_non_zero_delta;
        }
        data.knotslist.push_back(last + fallback_delta);
    }

    addWrapping();
}

/** Add raw control point. */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
}

/** Remove last control point, handling wrapping. */
void RS_Spline::removeLastControlPoint() {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (n > 0) {
        data.controlPoints.pop_back();
        data.weights.pop_back();
    }
    addWrapping();
}

/** Set weight at index. */
void RS_Spline::setWeight(size_t index, double w) {
    size_t n = getUnwrappedSize();
    if (index < n) {
        data.weights[index] = w;
        update();
    }
}

/** Set all weights. */
void RS_Spline::setWeights(const std::vector<double>& weights) {
    size_t n = getUnwrappedSize();
    if (weights.size() != n) {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "RS_Spline::setWeights: size mismatch between weights and control points");
        return;
    }
    data.weights = weights;
    update();
}

/** Set control point at index. */
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

/** Set knot at index. */
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

/** Insert control point, clear knots if present. */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (index > n) index = n;
    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);
    if (!data.knotslist.empty()) {
        size_t insert_k_pos = index + data.degree;
        double new_k;
        if (insert_k_pos > 0 && insert_k_pos < data.knotslist.size()) {
            new_k = (data.knotslist[insert_k_pos - 1] + data.knotslist[insert_k_pos]) / 2.0;
        } else if (insert_k_pos == 0) {
            // Insert at beginning
            double fallback_delta = 1.0; // Default
            if (!data.knotslist.empty()) {
                fallback_delta = data.knotslist[1] - data.knotslist[0];
                if (fallback_delta <= 1e-10) fallback_delta = 1.0;
            }
            new_k = data.knotslist[0] - fallback_delta;
        } else {
            // Append at end
            double fallback_delta = 1.0; // Default
            if (!data.knotslist.empty()) {
                fallback_delta = data.knotslist.back() - data.knotslist[data.knotslist.size() - 2];
                if (fallback_delta <= 1e-10) fallback_delta = 1.0;
            }
            new_k = data.knotslist.back() + fallback_delta;
        }
        data.knotslist.insert(data.knotslist.begin() + insert_k_pos, new_k);
    }
    addWrapping();
    update();
}

/** Remove control point, clear knots if present. */
void RS_Spline::removeControlPoint(size_t index) {
    removeWrapping();
    size_t n = data.controlPoints.size();
    if (index < n) {
        data.controlPoints.erase(data.controlPoints.begin() + index);
        if (index < data.weights.size()) {
            data.weights.erase(data.weights.begin() + index);
        }
        if (!data.knotslist.empty()) {
            size_t remove_k_pos = index + data.degree;
            if (remove_k_pos < data.knotslist.size()) {
                data.knotslist.erase(data.knotslist.begin() + remove_k_pos);
            }
        }
        addWrapping();
        update();
    }
}

/** Get knot vector (unwrapped). */
std::vector<double> RS_Spline::getKnotVector() const {
    return getUnwrappedKnotVector();
}

/** Set knot vector, validate size and monotonicity. */
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

/** Generate open knot vector. */
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

/** Helper: Generate rational basis functions. */
namespace {
std::vector<double> rbasis(int c, double t, int npts,
                           const std::vector<double>& x,
                           const std::vector<double>& h) {

    int const nplusc = npts + c;

    std::vector<double> temp(nplusc,0.);

    // First order basis
    for (int i = 0; i< nplusc-1; i++)
        if ((t >= x[i]) && (t < x[i+1])) temp[i] = 1;

    // Higher order basis
    for (int k = 2; k <= c; k++) {
        for (int i = 0; i < nplusc-k; i++) {
            if (temp[i] != 0) {
                double den = x[i+k-1]-x[i];
                if (den != 0.0) {
                    temp[i] = ((t-x[i])*temp[i])/den;
                } else {
                    temp[i] = 0.0;
                }
            }
            if (temp[i+1] != 0) {
                double den = x[i+k]-x[i+1];
                if (den != 0.0) {
                    temp[i] += ((x[i+k]-t)*temp[i+1])/den;
                }
            }
        }
    }

    // Last point
    if (t >= x[nplusc-1]) temp[npts-1] = 1;

    // Denominator sum
    double sum = 0.;
    for (int i = 0; i < npts; i++) {
        sum += temp[i]*h[i];
    }

    std::vector<double> r(npts, 0);
    // Rational basis
    if (sum != 0) {
        for (int i = 0; i < npts; i++)
            r[i] = (temp[i]*h[i])/sum;
    }
    return r;
}
}

/** Generate rational B-spline points (open). */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const
{

    // Open knot vector
    auto const x = knot(npts, k);

    // Points on curve
    size_t degree = k - 1;
    double t = x[degree];
    double const step = (x[npts] - t) / (p1 - 1);

    for (auto& vp: p) {
        if (x[npts] - t < 5e-6) t = x[npts];

        // Basis functions
        auto const nbasis = rbasis(k, t, npts, x, h);

        // Curve point
        for (size_t i = 0; i < npts; i++)
            vp += b[i] * nbasis[i];

        t += step;
    }
}

/** Generate uniform knot vector for periodic splines. */
std::vector<double> RS_Spline::knotu(size_t num, size_t order) const{
    if (data.knotslist.size() == num + order) {
        //use custom knot vector
        return data.knotslist;
    }
    std::vector<double> knotVector(num + order, 0.);
    //use uniform knots
    std::iota(knotVector.begin(), knotVector.begin() + num + order, 0);
    if (data.closed && hasWrappedControlPoints()) {
        // Extend for wrapping
        knotVector.resize(num + order + data.degree);
        double delta = (num + order > 1) ? 1.0 : 1.0; // Uniform spacing
        for (int i = 0; i < data.degree; ++i) {
            knotVector[num + order + i] = knotVector[num + order - 1] + (i + 1) * delta;
        }
    }
    return knotVector;
}

/** Generate rational B-spline points (periodic). */
void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const
{

    /* Periodic knot vector */
    std::vector<double> const x = knotu(npts, k);

    /* Points on curve */
    size_t degree = k - 1;
    double t = x[degree];
    double const step = (x[npts] - t) / (p1 - 1);

    for (auto& vp: p) {
        if (x[npts] - t < 5e-6) t = x[npts];

        /* Basis functions */
        auto const nbasis = rbasis(k, t, npts, x, h);
        /* Curve point */
        for (size_t i = 0; i < npts; i++)
            vp += b[i] * nbasis[i];

        t += step;
    }
}

/** Stream operator for RS_Spline. */
std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}

/** Check if control points wrapped (for closed cubic splines). */
bool RS_Spline::hasWrappedControlPoints() const {
    return data.wrapped;
}
