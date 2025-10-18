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

namespace {


std::vector<double> convertOpenToClosedKnotVector(const std::vector<double>& openKnots,
                                                  size_t n, size_t m) {
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
    std::vector<double> closedKnots = {openKnots.cbegin() + startIdx, openKnots.cbegin() + lastIdx + 1};
    if (closedKnots.size() < 2)
        return {};
    double delta = period; // FIXED: Corrected delta to period for proper periodic extension and uniform spacing
    const size_t newSize = n + 2 * m + 1;
    for (size_t i = 0; closedKnots.size() < newSize; ++i) {
        closedKnots.push_back(closedKnots[i] + delta);
    }

    return closedKnots;
}

std::vector<double> convertClosedToOpenKnotVector(const std::vector<double>& closedKnots, size_t n, size_t m) {
    if (closedKnots.size() < n + 2*m + 1)
        return {};

    size_t openSize = n + m + 1;

    std::vector<double> openKnots(m + 1, closedKnots.front());
    std::copy(closedKnots.cbegin() + m + 1, closedKnots.cbegin() + n + 1, std::back_inserter(openKnots));
    while (openKnots.size() < openSize)
        openKnots.push_back(openKnots.back());
    return openKnots;
}
}

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
    size_t unwrappedSize = getUnwrappedSize();
    LC_SplineHelper::removeWrapping(data, isClosed(), unwrappedSize);
}

/** Add wrapping to control points and weights for closed splines. */
void RS_Spline::addWrapping() {
    LC_SplineHelper::addWrapping(data, isClosed());
}

/** Update wrapping for control points and weights. */
void RS_Spline::updateControlAndWeightWrapping() {
    size_t unwrappedSize = getUnwrappedSize();
    LC_SplineHelper::updateControlAndWeightWrapping(data, isClosed(), unwrappedSize);
}

/** Update knot vector wrapping for closed splines. */
void RS_Spline::updateKnotWrapping() {
    size_t unwrappedSize = getUnwrappedSize();
    LC_SplineHelper::updateKnotWrapping(data, isClosed(), unwrappedSize);
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
    return getUnwrappedSize() + static_cast<size_t>(data.degree) + 1;
}

/** Check if closed. */
bool RS_Spline::isClosed() const {
    return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Set closed flag and adjust wrapping/knots. */
void RS_Spline::setClosed(bool c) {
    changeType(c ? RS_SplineData::SplineType::WrappedClosed : data.savedOpenType);
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

    size_t unwrappedSize = getUnwrappedSize();
    if (!LC_SplineHelper::validate(data, unwrappedSize)) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Spline::update: Invalid spline data");
        return;
    }

    // Allow closed splines with 3 points (Issue #1689)
    size_t minPoints = data.type == RS_SplineData::SplineType::WrappedClosed ? 3 : static_cast<size_t>(data.degree + 1);
    if (unwrappedSize < minPoints) {
        RS_DEBUG->print("RS_Spline::update: not enough control points");
        return;
    }

    if (data.type == RS_SplineData::SplineType::WrappedClosed && !hasWrappedControlPoints()) {
        addWrapping();
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
    size_t base_n = data.type == RS_SplineData::SplineType::WrappedClosed ? getUnwrappedSize() : data.controlPoints.size();
    const size_t pointsCount = splineSegments * base_n;
    const size_t npts = data.controlPoints.size();
    const size_t k = data.degree + 1;

    std::vector<double> h = data.weights;
    points.resize(pointsCount);
    std::fill(points.begin(), points.end(), RS_Vector{0., 0.});
    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        rbsplinu(npts, k, pointsCount, data.controlPoints, h, points);
    } else {
        rbspline(npts, k, pointsCount, data.controlPoints, h, points);
    }
}

/** Get start point (invalid if closed). */
RS_Vector RS_Spline::getStartpoint() const {
    if (data.type == RS_SplineData::SplineType::WrappedClosed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->firstEntity())->getStartpoint();
}

/** Get end point (invalid if closed). */
RS_Vector RS_Spline::getEndpoint() const {
    if (data.type == RS_SplineData::SplineType::WrappedClosed) return RS_Vector(false);
    return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->lastEntity())->getEndpoint();
}

/** Nearest endpoint or control point. */
RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector ret(false);
    if (!isClosed()) { // no endpoint for closed spline
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
    for (RS_Vector& vp: data.fitPoints) {
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
    for (RS_Vector& vp: data.fitPoints) {
        vp.rotate(center, angleVector);
    }
}

/** Scale by factor. */
void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (RS_Vector& vp: data.controlPoints) {
        vp.scale(center, factor);
    }
    for (RS_Vector& vp: data.fitPoints) {
        vp.scale(center, factor);
    }
    update();
}

/** Shear by factor. */
RS_Entity& RS_Spline::shear(double k){
    for (RS_Vector& vp: data.controlPoints) {
        vp.shear(k);
    }
    for (RS_Vector& vp: data.fitPoints) {
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
    for (RS_Vector& vp: data.fitPoints) {
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
    if (isClosed()) {
        updateControlAndWeightWrapping();
    }
    update();
}

/** Revert direction by reversing points, weights, knots. */
void RS_Spline::revertDirection() {
    removeWrapping();
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    std::reverse(data.weights.begin(), data.weights.end());
    std::reverse(data.fitPoints.begin(), data.fitPoints.end());
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
    size_t old_size = getUnwrappedSize();
    bool was_empty = data.knotslist.empty();

    size_t new_unwrapped = old_size + 1;
    size_t min_size = data.type == RS_SplineData::SplineType::WrappedClosed ? 3 : static_cast<size_t>(data.degree + 1);
    if (new_unwrapped < min_size) {
        return;
    }

    // FIXED: Moved addition after min_size check to avoid adding invalid points
    if (data.type == RS_SplineData::SplineType::WrappedClosed && data.controlPoints.size() > old_size) {
        data.controlPoints.insert(data.controlPoints.begin() + old_size, v);
        data.weights.insert(data.weights.begin() + old_size, w);
    } else {
        data.controlPoints.push_back(v);
        data.weights.push_back(w);
    }

    addWrapping();

    if (was_empty) {
        data.knotslist = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(new_unwrapped, data.degree + 1) : knot(new_unwrapped, data.degree + 1);
    } else {
        // Custom non-uniform, append with fallback delta
        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;
        size_t expected_k_size = old_size + static_cast<size_t>(data.degree) + 1;
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;
            }
        }
        double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        if (std::abs(data.knotslist.back() - data.knotslist[expected_k_size - 2]) < 1e-10) {
            fallback_delta = last_non_zero_delta;
        }
        size_t pos = new_unwrapped;
        double last = data.knotslist[pos - 1];
        data.knotslist.insert(data.knotslist.begin() + pos, last + fallback_delta);
        for (size_t i = pos + 1; i < data.knotslist.size(); ++i)
            data.knotslist[i] += fallback_delta;
    }
}

/** Add raw control point. */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
}

/** Remove last control point, handling wrapping. */
void RS_Spline::removeLastControlPoint() {
    size_t old_size = getUnwrappedSize();
    size_t min_size = data.type == RS_SplineData::SplineType::WrappedClosed ? 3 : static_cast<size_t>(data.degree + 1);
    if (old_size <= min_size) {
        return; // FIXED: Check before removal to avoid dropping below min_size
    }

    std::vector<double> gen_knots = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(old_size, data.degree + 1) : knot(old_size, data.degree + 1);
    bool was_uniform = (data.knotslist == gen_knots);
    bool was_empty = data.knotslist.empty();

    removeWrapping();
    size_t n = data.controlPoints.size();
    if (n > 0) {
        data.controlPoints.pop_back();
        data.weights.pop_back();
    }

    if (!data.knotslist.empty() && !(was_empty || was_uniform)) {
        size_t remove_k_pos = n;
        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;
        size_t expected_k_size = old_size + static_cast<size_t>(data.degree) + 1;
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;
            }
        }
        double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        if (std::abs(data.knotslist.back() - data.knotslist[expected_k_size - 2]) < 1e-10) {
            fallback_delta = last_non_zero_delta;
        }
        if (remove_k_pos < data.knotslist.size()) {
            data.knotslist.erase(data.knotslist.begin() + remove_k_pos);
            for (size_t i = remove_k_pos; i < data.knotslist.size(); ++i) {
                data.knotslist[i] -= fallback_delta;
            }
        }
    }

    size_t new_size = data.controlPoints.size();
    if (was_empty || was_uniform) {
        data.knotslist = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(new_size, data.degree + 1) : knot(new_size, data.degree + 1);
    }

    if (data.type == RS_SplineData::SplineType::WrappedClosed) {
        updateKnotWrapping();
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
        if (isClosed()) {
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
        if (isClosed()) {
            updateKnotWrapping();
        }
        update();
    }
}

/** Insert control point, clear knots if present. */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    size_t old_size = getUnwrappedSize();
    size_t new_unwrapped = old_size + 1;
    size_t min_size = data.type == RS_SplineData::SplineType::WrappedClosed ? 3 : static_cast<size_t>(data.degree + 1);
    if (new_unwrapped < min_size) {
        return;
    }

    bool original_closed = data.type == RS_SplineData::SplineType::WrappedClosed;
    if (original_closed) {
        setClosed(false);
    }

    std::vector<double> gen_knots = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(old_size, data.degree + 1) : knot(old_size, data.degree + 1);
    bool was_uniform = (data.knotslist == gen_knots);
    bool was_empty = data.knotslist.empty();

    size_t n = data.controlPoints.size();
    if (index > n) index = n;

    // FIXED: Moved insertion after min_size check to avoid adding invalid points
    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);

    if (was_empty || was_uniform) {
        data.knotslist = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(new_unwrapped, data.degree + 1) : knot(new_unwrapped, data.degree + 1);
    } else {
        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;
        size_t expected_k_size = old_size + static_cast<size_t>(data.degree) + 1;
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;
            }
        }
        double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        if (std::abs(data.knotslist.back() - data.knotslist[expected_k_size - 2]) < 1e-10) {
            fallback_delta = last_non_zero_delta;
        }
        size_t pos = index + 1;
        if (pos > data.knotslist.size()) pos = data.knotslist.size();
        double last = (pos > 0) ? data.knotslist[pos - 1] : (data.knotslist.empty() ? 0.0 : data.knotslist[0] - fallback_delta);
        data.knotslist.insert(data.knotslist.begin() + pos, last + fallback_delta);
        for (size_t i = pos + 1; i < data.knotslist.size(); ++i) {
            data.knotslist[i] += fallback_delta;
        }
    }

    if (original_closed) {
        setClosed(true);
    }

    update();
}

/** Remove control point, clear knots if present. */
void RS_Spline::removeControlPoint(size_t index) {
    size_t old_size = getUnwrappedSize();
    size_t min_size = data.type == RS_SplineData::SplineType::WrappedClosed ? 3 : static_cast<size_t>(data.degree + 1);
    if (old_size <= min_size) {
        return; // FIXED: Check before removal to avoid dropping below min_size
    }

    bool original_closed = data.type == RS_SplineData::SplineType::WrappedClosed;
    if (original_closed) {
        setClosed(false);
    }

    std::vector<double> gen_knots = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(old_size, data.degree + 1) : knot(old_size, data.degree + 1);
    bool was_uniform = (data.knotslist == gen_knots);
    bool was_empty = data.knotslist.empty();

    size_t n = data.controlPoints.size();
    if (index >= n) {
        if (original_closed) {
            setClosed(true);
        }
        return;
    }
    data.controlPoints.erase(data.controlPoints.begin() + index);
    data.weights.erase(data.weights.begin() + index);

    size_t new_size = old_size - 1;

    if (was_empty || was_uniform) {
        data.knotslist = data.type == RS_SplineData::SplineType::WrappedClosed ? knotu(new_size, data.degree + 1) : knot(new_size, data.degree + 1);
    } else {
        double total_delta = 0.0;
        int count_delta = 0;
        double last_non_zero_delta = 1.0;
        size_t expected_k_size = old_size + static_cast<size_t>(data.degree) + 1;
        for (size_t i = 1; i < data.knotslist.size(); ++i) {
            double dd = data.knotslist[i] - data.knotslist[i - 1];
            if (dd > 1e-10) {
                total_delta += dd;
                count_delta++;
                last_non_zero_delta = dd;
            }
        }
        double fallback_delta = (count_delta > 0) ? total_delta / count_delta : 1.0;
        if (std::abs(data.knotslist.back() - data.knotslist[expected_k_size - 2]) < 1e-10) {
            fallback_delta = last_non_zero_delta;
        }
        size_t remove_k_pos = index + 1;
        if (remove_k_pos < data.knotslist.size()) {
            data.knotslist.erase(data.knotslist.begin() + remove_k_pos);
            for (size_t i = remove_k_pos; i < data.knotslist.size(); ++i) {
                data.knotslist[i] -= fallback_delta;
            }
        }
    }

    if (original_closed) {
        setClosed(true);
    }

    update();
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
    if (isClosed()) {
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

/** Generate open uniform knot vector without multiple knots at ends. */
std::vector<double> RS_Spline::openUniformKnot(size_t num, size_t order) const {
    std::vector<double> knotVector(num + order);
    std::iota(knotVector.begin(), knotVector.end(), 0.0);
    return knotVector;
}

/** Generate uniform knot vector for periodic splines. */
std::vector<double> RS_Spline::knotu(size_t num, size_t order) const{
    if (data.knotslist.size() == num + order) {
        //use custom knot vector
        return data.knotslist;
    }
    std::vector<double> knotVector(num + order, 0.);
    double delta = 1.0 / static_cast<double>(num);
    for (size_t i = 0; i < num + order; ++i) {
        knotVector[i] = static_cast<double>(i) * delta;
    }
    if (isClosed() && hasWrappedControlPoints()) {
        // Extend for wrapping
        knotVector.resize(num + order + data.degree);
        for (int i = 0; i < data.degree; ++i) {
            knotVector[num + order + i] = knotVector[num + order - 1] + (i + 1) * delta;
        }
    }
    return knotVector;
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
    return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Change spline type using helper */
void RS_Spline::changeType(RS_SplineData::SplineType newType) {
    if (data.type == newType) return;

    size_t unwrappedSize = getUnwrappedSize();
    bool isClosedNew = (newType == RS_SplineData::SplineType::WrappedClosed);

    if (isClosedNew) {
        data.savedOpenType = data.type;
        LC_SplineHelper::toWrappedClosedFromStandard(data, unwrappedSize);
    } else {
        if (data.type == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toStandardFromWrappedClosed(data, unwrappedSize);
        }
        data.type = newType;
    }
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

/** Solve linear system A x = b using Gaussian elimination with partial pivoting. */
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
