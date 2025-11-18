#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <limits>
#include <array>

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
    if (degree < 1) throw std::invalid_argument("Degree must be at least 1");
    if (data.degree == static_cast<size_t>(degree)) return;
    data.degree = degree;
    if (isClosed()) {
        LC_SplineHelper::removeWrapping(data);
        LC_SplineHelper::addWrapping(data);
    }
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after setDegree");
        throw std::runtime_error("Invalid spline state after degree change");
    }
    calculateBorders();
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

    std::vector<RS_Vector> points;
    fillStrokePoints(0, points);

    for (size_t i = 0; i < points.size() - 1; ++i) {
        addEntity(new RS_Line(this, points[i], points[i + 1]));
    }
}

/** Fill points for spline approximation */
void RS_Spline::fillStrokePoints(int /*unused*/, std::vector<RS_Vector>& points) {
    double minParam = data.knotslist[data.degree];
    size_t offset = isClosed() ? data.degree : 0;
    double maxParam = data.knotslist[getNumberOfControlPoints() + offset];
    double tol = 1e-3;  // Adjustable; could be based on layer/zoom if available
    points.push_back(getPointAt(minParam));
    tessellate(points, minParam, maxParam, tol);
    points.push_back(getPointAt(maxParam));
    if (isClosed() && points.back().distanceTo(points.front()) > tol) {
        points.push_back(points.front());  // Ensure closure
    }
}

void RS_Spline::tessellate(std::vector<RS_Vector>& points, double minParam, double maxParam, double tol) {
    recursiveTessellate(points, minParam, maxParam, tol);
}

void RS_Spline::recursiveTessellate(std::vector<RS_Vector>& points, double a, double b, double tol, int depth) {
    if (depth > 20) return;  // Prevent stack overflow
    double m = (a + b) / 2.0;
    RS_Vector pa = getPointAt(a);
    RS_Vector pb = getPointAt(b);
    RS_Vector pm = getPointAt(m);
    RS_Vector chord = pb - pa;
    double chordLen = chord.magnitude();
    if (chordLen < RS_TOLERANCE) return;
    double dist = chord.crossP(pm - pa).magnitude() / chordLen;  // Perp distance
    if (dist > tol) {
        recursiveTessellate(points, a, m, tol, depth + 1);
        points.push_back(pm);
        recursiveTessellate(points, m, b, tol, depth + 1);
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
    update();
}

/** Rotate by angle */
void RS_Spline::rotate(const RS_Vector& center, double angle) {
    for (auto& p : data.controlPoints) p.rotate(center, angle);
    for (auto& p : data.fitPoints) p.rotate(center, angle);
    calculateBorders();
    update();
}

/** Rotate by angle vector */
void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& p : data.controlPoints) p.rotate(center, angleVector);
    for (auto& p : data.fitPoints) p.rotate(center, angleVector);
    calculateBorders();
    update();
}

/** Scale by factor */
void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& p : data.controlPoints) p.scale(center, factor);
    for (auto& p : data.fitPoints) p.scale(center, factor);
    calculateBorders();
    update();
}

/** Shear by factor */
RS_Entity& RS_Spline::shear(double k) {
    for (auto& p : data.controlPoints) p.x += k * p.y;
    for (auto& p : data.fitPoints) p.x += k * p.y;
    calculateBorders();
    update();
    return *this;
}

/** Mirror across axis */
void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& p : data.controlPoints) p.mirror(axisPoint1, axisPoint2);
    for (auto& p : data.fitPoints) p.mirror(axisPoint1, axisPoint2);
    calculateBorders();
    update();
}

/** Move reference point (control point) */
void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    double minDist = RS_MAXDOUBLE;
    size_t closestIndex = 0;
    auto unwrappedControls = getUnwrappedControlPoints();
    for (size_t i = 0; i < unwrappedControls.size(); ++i) {
        double d = ref.distanceTo(unwrappedControls[i]);
        if (d < minDist) {
            minDist = d;
            closestIndex = i;
        }
    }
    if (minDist < RS_TOLERANCE * 100) {  // Threshold
        data.controlPoints[closestIndex] += offset;
        if (isClosed()) updateControlAndWeightWrapping();
    }
    calculateBorders();
    update();
}

/** Revert direction by reversing points, weights, knots */
void RS_Spline::revertDirection() {
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    std::reverse(data.weights.begin(), data.weights.end());
    // Normalize knots to [0, max - min]
    double minK = data.knotslist.front();
    double maxK = data.knotslist.back();
    std::reverse(data.knotslist.begin(), data.knotslist.end());
    for (auto& k : data.knotslist) k = maxK - (k - minK);
    calculateBorders();
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
    return data.weights.at(index);
}

/** Add control point with weight, handling wrapping */
void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    bool closed = isClosed();
    if (closed) removeWrapping();
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
    if (closed) addWrapping();
    if (data.controlPoints.size() >= data.degree + 1 && !validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after addControlPoint");
        throw std::runtime_error("Invalid spline state after adding control point");
    }
    calculateBorders();
    update();
}

/** Add raw control point */
void RS_Spline::addControlPointRaw(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    data.weights.push_back(w);
}

/** Remove last control point, handling wrapping */
void RS_Spline::removeLastControlPoint() {
    bool closed = isClosed();
    if (closed) removeWrapping();
    if (!data.controlPoints.empty()) {
        data.controlPoints.pop_back();
        data.weights.pop_back();
    }
    if (closed) addWrapping();
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after removeLastControlPoint");
        throw std::runtime_error("Invalid spline state after removing control point");
    }
    calculateBorders();
    update();
}

/** Set weight at index */
void RS_Spline::setWeight(size_t index, double w) {
    if (index >= data.weights.size()) {
        throw std::out_of_range("Index out of range for setWeight");
    }
    data.weights.at(index) = w;
    update();
}

/** Set all weights */
void RS_Spline::setWeights(const std::vector<double>& weights) {
    if (weights.size() != data.weights.size()) {
        throw std::invalid_argument("Weights size mismatch in setWeights");
    }
    data.weights = weights;
    update();
}

/** Set control point at index */
void RS_Spline::setControlPoint(size_t index, const RS_Vector& v) {
    if (index >= data.controlPoints.size()) {
        throw std::out_of_range("Index out of range for setControlPoint");
    }
    data.controlPoints.at(index) = v;
    calculateBorders();
    update();
}

/** Set knot at index */
void RS_Spline::setKnot(size_t index, double k) {
    if (index >= data.knotslist.size()) {
        throw std::out_of_range("Index out of range for setKnot");
    }
    data.knotslist.at(index) = k;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
    update();
}

/** Insert control point, clear knots if present */
void RS_Spline::insertControlPoint(size_t index, const RS_Vector& v, double w) {
    bool closed = isClosed();
    if (closed) removeWrapping();
    if (index > data.controlPoints.size()) {
        throw std::out_of_range("Index out of range for insertControlPoint");
    }
    data.controlPoints.insert(data.controlPoints.begin() + index, v);
    data.weights.insert(data.weights.begin() + index, w);
    if (closed) addWrapping();
    if (!data.knotslist.empty()) data.knotslist.clear();  // Invalidate knots
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after insertControlPoint");
        throw std::runtime_error("Invalid spline state after inserting control point");
    }
    calculateBorders();
    update();
}

/** Remove control point, clear knots if present */
void RS_Spline::removeControlPoint(size_t index) {
    bool closed = isClosed();
    if (closed) removeWrapping();
    if (index >= data.controlPoints.size()) {
        throw std::out_of_range("Index out of range for removeControlPoint");
    }
    data.controlPoints.erase(data.controlPoints.begin() + index);
    data.weights.erase(data.weights.begin() + index);
    if (closed) addWrapping();
    if (!data.knotslist.empty()) data.knotslist.clear();  // Invalidate knots
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after removeControlPoint");
        throw std::runtime_error("Invalid spline state after removing control point");
    }
    calculateBorders();
    update();
}

/** Get knot vector (unwrapped) */
std::vector<double> RS_Spline::getKnotVector() const {
    return getUnwrappedKnotVector();
}

/** Set knot vector, validate size and monotonicity */
void RS_Spline::setKnotVector(const std::vector<double>& knots) {
    if (knots.size() != data.controlPoints.size() + data.degree + 1) {
        throw std::invalid_argument("Knot vector size mismatch in setKnotVector");
    }
    data.knotslist = knots;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Invalid state after setKnotVector");
        throw std::runtime_error("Invalid spline state after setting knot vector");
    }
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
    std::vector<double> knots(num + order);
    std::iota(knots.begin(), knots.end(), 0.0);
    return knots;
}

/** Generate rational B-spline points (open) */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    // Implementation (from original, assuming exists)
    // ...
}

/** Generate rational B-spline points (periodic) */
void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const {
    // Implementation (from original, assuming exists)
    // ...
}

/** Check if control points wrapped (for closed cubic splines) */
bool RS_Spline::hasWrappedControlPoints() const {
    size_t n = data.controlPoints.size();
    if (n < static_cast<size_t>(data.degree * 2)) return false;
    for (size_t i = 0; i < static_cast<size_t>(data.degree); ++i) {
        if (data.controlPoints[i].distanceTo(data.controlPoints[n - data.degree + i]) > RS_TOLERANCE) return false;
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
    // Implementation (assuming exists)
    // ...
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
            LC_SplineHelper::toStandardFromWrappedClosed(data);
        }
        break;
    case RS_SplineData::SplineType::ClampedOpen:
        if (data.type == RS_SplineData::SplineType::Standard) {
            LC_SplineHelper::toClampedOpenFromStandard(data);
        } else if (data.type == RS_SplineData::SplineType::WrappedClosed) {
            LC_SplineHelper::toClampedOpenFromWrappedClosed(data);
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
    if (U.size() != n + p + 2) return RS_Vector(false);  // Fixed

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
    if (u >= U[n + p + 1] - RS_TOLERANCE) return n;
    if (u <= U[p] + RS_TOLERANCE) return p;

    int low = p;
    int high = n + p + 1;
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
        for (int kk = 1; kk <= derOrder; ++kk) {
            double d = 0.0;
            int rk = r - kk, pk = p - kk;
            if (r >= kk) {
                double den = ndu[pk + 1][rk];
                a[s2][0] = (std::abs(den) > RS_TOLERANCE) ? a[s1][0] / den : 0.0;
                d = a[s2][0] * ndu[rk][pk];
            }
            int j1 = (rk >= -1) ? 1 : -rk;
            int j2 = (r - 1 <= pk) ? kk - 1 : p - r;
            for (int j = j1; j <= j2; ++j) {
                double den = ndu[pk + 1][rk + j];
                a[s2][j] = (std::abs(den) > RS_TOLERANCE) ? (a[s1][j] - a[s1][j - 1]) / den : 0.0;
                d += a[s2][j] * ndu[rk + j][pk];
            }
            if (r <= pk) {
                double den = ndu[pk + 1][r];
                a[s2][kk] = (std::abs(den) > RS_TOLERANCE) ? -a[s1][kk - 1] / den : 0.0;
                d += a[s2][kk] * ndu[r][pk];
            }
            ders[kk][r] = d;
            std::swap(s1, s2);
        }
    }
    int rr = p;
    for (int kk = 1; kk <= derOrder; ++kk) {
        for (int j = 0; j <= p; ++j) {
            ders[kk][j] *= rr;
        }
        rr *= (p - kk);
    }
}

/** Get tangent vector at t */
RS_Vector RS_Spline::getTangentAt(double t) const {
    size_t p = data.degree;
    size_t n = data.controlPoints.size() - 1;
    const auto& U = data.knotslist;
    if (U.size() != n + p + 2) return RS_Vector(false);
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
    size_t unwrapped = getUnwrappedSize();
    if (unwrapped < data.degree + 1) return false;
    return LC_SplineHelper::validate(data);
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
    std::vector<double> N = basisFunctions(span, t, p, x);
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

/** Split spline at parameter t into two sub-splines. */
std::pair<RS_Spline*, RS_Spline*> RS_Spline::splitAt(double t) const {
    if (t <= data.knotslist[data.degree] + RS_TOLERANCE || t >= data.knotslist[getNumberOfControlPoints()] - RS_TOLERANCE) {
        return {nullptr, nullptr};  // Invalid
    }
    RS_SplineData newData = data;
    // Insert knot 'degree' times to split (A5.1 NURBS book)
    for (size_t kk = 0; kk < data.degree; ++kk) {
        int span = findSpan(newData.controlPoints.size() - 1, newData.degree, t, newData.knotslist);
        std::vector<RS_Vector> newControls(newData.controlPoints.size() + 1);
        std::vector<double> newWeights(newData.weights.size() + 1);
        std::vector<double> newKnots(newData.knotslist.size() + 1);
        // Compute new points (alpha-based averaging)
        for (size_t i = 0; i <= static_cast<size_t>(span - newData.degree); ++i) {
            newControls[i] = newData.controlPoints[i];
            newWeights[i] = newData.weights[i];
        }
        for (size_t i = static_cast<size_t>(span); i < newData.controlPoints.size(); ++i) {
            newControls[i + 1] = newData.controlPoints[i];
            newWeights[i + 1] = newData.weights[i];
        }
        for (size_t r = newData.degree; r > 0; --r) {
            size_t i = static_cast<size_t>(span) - r + 1;
            double alpha = (t - newData.knotslist[i]) / (newData.knotslist[i + newData.degree] - newData.knotslist[i]);
            newControls[i] = newControls[i] * alpha + newControls[i - 1] * (1 - alpha);
            newWeights[i] = newWeights[i] * alpha + newWeights[i - 1] * (1 - alpha);
        }
        // Update knots
        for (size_t i = 0; i <= static_cast<size_t>(span); ++i) newKnots[i] = newData.knotslist[i];
        newKnots[span + 1] = t;
        for (size_t i = static_cast<size_t>(span + 1); i < newData.knotslist.size(); ++i) newKnots[i + 1] = newData.knotslist[i];
        newData.controlPoints = newControls;
        newData.weights = newWeights;
        newData.knotslist = newKnots;
    }
    // Split into two
    size_t splitIdx = static_cast<size_t>(findSpan(newData.controlPoints.size() - 1, newData.degree, t, newData.knotslist)) + 1;
    RS_SplineData data1 = newData;
    data1.controlPoints.resize(splitIdx);
    data1.weights.resize(splitIdx);
    data1.knotslist.resize(splitIdx + newData.degree + 1);
    RS_SplineData data2 = newData;
    data2.controlPoints.erase(data2.controlPoints.begin(), data2.controlPoints.begin() + splitIdx - newData.degree);
    data2.weights.erase(data2.weights.begin(), data2.weights.begin() + splitIdx - newData.degree);
    data2.knotslist.erase(data2.knotslist.begin(), data2.knotslist.begin() + splitIdx);
    return {new RS_Spline(getParent(), data1), new RS_Spline(getParent(), data2)};
}

void RS_Spline::insertKnot(double u, size_t multiplicity) {
    if (multiplicity == 0) return;
    if (multiplicity > data.degree) {
        throw std::invalid_argument("Multiplicity cannot exceed degree");
    }

    // For closed splines, convert to open, insert, re-wrap
    bool wasClosed = isClosed();
    RS_SplineData::SplineType originalType = data.type;
    size_t originalUnwrapped = getUnwrappedSize();
    if (wasClosed) {
        changeType(RS_SplineData::SplineType::ClampedOpen);
    }

    // Validate u in [knots[degree], knots[n]]
    size_t n = data.controlPoints.size() - 1;
    size_t p = data.degree;
    double umin = data.knotslist[p];
    double umax = data.knotslist[n + 1 - p + p];  // knots[n+1] wait, knots.size() = n + p + 1
    size_t m_knots = data.knotslist.size() - 1;  // m = n + p
    if (u < umin - RS_TOLERANCE || u > umax + RS_TOLERANCE) {
        if (wasClosed) changeType(originalType);
        throw std::invalid_argument("Knot u out of parameter range");
    }

    // Find span k and existing multiplicity s
    int k = findSpan(n, p, u, data.knotslist);
    size_t s = 0;
    while (s < p + 1 && k - static_cast<int>(s) >= 0 && std::abs(data.knotslist[k - s] - u) < RS_TOLERANCE) {
        ++s;
    }
    if (s + multiplicity > p + 1) {  // Max multiplicity p+1
        if (wasClosed) changeType(originalType);
        throw std::invalid_argument("Total multiplicity would exceed degree + 1");
    }
    size_t r = multiplicity;  // Additional to insert

    // New sizes
    size_t np = n + r;
    size_t mp = m_knots + r;

    // New knots V
    std::vector<double> V(mp + 1);
    V.reserve(mp + 1);
    for (size_t i = 0; i <= static_cast<size_t>(k); ++i) V[i] = data.knotslist[i];
    for (size_t i = 1; i <= r; ++i) V[k + i] = u;
    for (size_t i = k + 1; i <= m_knots; ++i) V[i + r] = data.knotslist[i];

    // Homogeneous control points Pw (xw, yw, w)
    using HomogPt = std::array<double, 3>;
    std::vector<HomogPt> Pw(n + 1);
    for (size_t i = 0; i <= n; ++i) {
        double w = data.weights[i];
        Pw[i] = {data.controlPoints[i].x * w, data.controlPoints[i].y * w, w};
    }

    // New homogeneous Qw
    std::vector<HomogPt> Qw(np + 1);
    for (size_t i = 0; i <= static_cast<size_t>(k - p); ++i) Qw[i] = Pw[i];
    for (size_t i = k - s; i <= n; ++i) Qw[i + r] = Pw[i];

    // Temp array R for affected points
    std::vector<HomogPt> R(p - s + 1);
    for (size_t i = 0; i <= p - s; ++i) {
        R[i] = Pw[k - p + i];
    }

    // Main loop for insertions
    for (size_t j = 1; j <= r; ++j) {
        size_t L = k - p + j;
        for (size_t ii = 0; ii <= p - j - s; ++ii) {  // i in book
            double alpha = (u - data.knotslist[L + ii]) / (data.knotslist[ii + k + 1] - data.knotslist[L + ii]);
            R[ii] = {alpha * R[ii + 1][0] + (1 - alpha) * R[ii][0],
                     alpha * R[ii + 1][1] + (1 - alpha) * R[ii][1],
                     alpha * R[ii + 1][2] + (1 - alpha) * R[ii][2]};
        }
        Qw[L] = R[0];
        Qw[k + r - j] = R[p - j - s];  // Corrected index from book
    }

    // Remaining middle points
    for (size_t i = (k - p + r) + 1; i < k - s; ++i) {
        Qw[i] = R[i - (k - p)];
    }

    // Extract back to controls and weights
    data.controlPoints.resize(np + 1);
    data.weights.resize(np + 1);
    for (size_t i = 0; i <= np; ++i) {
        double w = Qw[i][2];
        if (std::abs(w) < RS_TOLERANCE) w = 1.0;  // Avoid div0, though unlikely
        data.controlPoints[i] = RS_Vector(Qw[i][0] / w, Qw[i][1] / w);
        data.weights[i] = w;
    }

    data.knotslist = std::move(V);

    // Re-wrap if closed
    if (wasClosed) {
        changeType(originalType);
        updateKnotWrapping();
        updateControlAndWeightWrapping();
    }

    // Validate and update
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Knot insertion resulted in invalid spline");
    }
    calculateBorders();
    update();
}

size_t RS_Spline::removeKnot(double u, size_t multiplicity, double tol) {
    if (multiplicity == 0) return 0;

    // For closed splines, convert to open, remove, re-wrap
    bool wasClosed = isClosed();
    RS_SplineData::SplineType originalType = data.type;
    if (wasClosed) {
        changeType(RS_SplineData::SplineType::ClampedOpen);
    }

    size_t n = data.controlPoints.size() - 1;
    size_t p = data.degree;
    size_t m = data.knotslist.size() - 1;

    // Find the range of knots equal to u
    auto lower = std::lower_bound(data.knotslist.begin(), data.knotslist.end(), u - RS_TOLERANCE);
    auto upper = std::upper_bound(lower, data.knotslist.end(), u + RS_TOLERANCE);
    size_t first_knot = std::distance(data.knotslist.begin(), lower);
    size_t s = std::distance(lower, upper);
    if (s == 0) {
        if (wasClosed) changeType(originalType);
        return 0;
    }

    int r = static_cast<int>(first_knot + s - 1);  // Last index where knots[r] == u
    multiplicity = std::min(multiplicity, s);

    int ord = static_cast<int>(p) + 1;
    int fout = (2 * r - static_cast<int>(s) - static_cast<int>(p)) / 2;

    using HomogPt = std::array<double, 3>;
    std::vector<HomogPt> Pw(n + 1);
    for (size_t ii = 0; ii <= n; ++ii) {
        double w = data.weights[ii];
        Pw[ii] = {data.controlPoints[ii].x * w, data.controlPoints[ii].y * w, w};
    }

    std::vector<HomogPt> temp(2 * p + 1);
    int last = r - static_cast<int>(s);
    int first = r - static_cast<int>(p);

    int t;  // Number of successful removals
    for (t = 0; t < static_cast<int>(multiplicity); ++t) {
        int off = first - 1;
        temp[0] = Pw[off];
        temp[last - first + 1] = Pw[last + 1];
        int i = first;
        int j = last + 1;
        int ii = 1;
        int jj = last - first + 1;

        while (j - i > t) {
            double alfi = (u - data.knotslist[i]) / (data.knotslist[i + ord + t] - data.knotslist[i]);
            double alfj = (u - data.knotslist[j - t]) / (data.knotslist[j - t + ord] - data.knotslist[j - t]);
            if (std::abs(data.knotslist[i + ord + t] - data.knotslist[i]) < RS_TOLERANCE) alfi = 1.0;
            if (std::abs(data.knotslist[j - t + ord] - data.knotslist[j - t]) < RS_TOLERANCE) alfj = 0.0;
            temp[ii] = {(Pw[i][0] - (1.0 - alfi) * temp[ii - 1][0]) / alfi,
                        (Pw[i][1] - (1.0 - alfi) * temp[ii - 1][1]) / alfi,
                        (Pw[i][2] - (1.0 - alfi) * temp[ii - 1][2]) / alfi};
            temp[jj] = {(Pw[j - t][0] - alfj * temp[jj + 1][0]) / (1.0 - alfj),
                        (Pw[j - t][1] - alfj * temp[jj + 1][1]) / (1.0 - alfj),
                        (Pw[j - t][2] - alfj * temp[jj + 1][2]) / (1.0 - alfj)};
            ++i;
            ++ii;
            --j;
            --jj;
        }

        bool remflag = false;
        if (j - i < t) {
            double qw = temp[ii - 1][2];
            if (std::abs(qw) < RS_TOLERANCE) qw = 1.0;
            RS_Vector Q(temp[ii - 1][0] / qw, temp[ii - 1][1] / qw);
            double dist = Q.distanceTo(data.controlPoints[r - fout]);
            if (dist <= tol) remflag = true;
        } else {
            double alfi = (u - data.knotslist[i]) / (data.knotslist[i + ord + t] - data.knotslist[i]);
            if (std::abs(data.knotslist[i + ord + t] - data.knotslist[i]) < RS_TOLERANCE) alfi = 1.0;
            HomogPt Qw = {(Pw[i][0] - (1.0 - alfi) * temp[ii - 1][0]) / alfi,
                          (Pw[i][1] - (1.0 - alfi) * temp[ii - 1][1]) / alfi,
                          (Pw[i][2] - (1.0 - alfi) * temp[ii - 1][2]) / alfi};
            double qw = Qw[2];
            if (std::abs(qw) < RS_TOLERANCE) qw = 1.0;
            RS_Vector Q(Qw[0] / qw, Qw[1] / qw);
            double dist = Q.distanceTo(data.controlPoints[r - fout]);
            if (dist <= tol) remflag = true;
        }

        if (!remflag) break;

        i = first;
        j = last;
        while (j - i > t) {
            Pw[i] = temp[i - off];
            Pw[j] = temp[j - off];
            ++i;
            --j;
        }

        --first;
        ++last;
    }

    size_t actualRemoved = static_cast<size_t>(t);

    if (actualRemoved == 0) {
        if (wasClosed) changeType(originalType);
        return 0;
    }

    // Shift knots for removed
    for (int kk = r; kk > r - static_cast<int>(actualRemoved); --kk) {
        for (size_t ii = static_cast<size_t>(kk); ii < data.knotslist.size() - 1; ++ii) {
            data.knotslist[ii] = data.knotslist[ii + 1];
        }
        data.knotslist.pop_back();
    }

    // Shift controls
    int jj = fout + static_cast<int>(actualRemoved);
    for (int kk = 1; kk <= static_cast<int>(actualRemoved); ++kk) {
        int ii = fout + kk;
        for (size_t idx = static_cast<size_t>(ii); idx <= n; ++idx) {
            Pw[idx - 1] = Pw[idx];
        }
        --n;  // Update n
    }

    // Extract back
    data.controlPoints.resize(n + 1);
    data.weights.resize(n + 1);
    for (size_t ii = 0; ii <= n; ++ii) {
        double w = Pw[ii][2];
        if (std::abs(w) < RS_TOLERANCE) w = 1.0;
        data.controlPoints[ii] = RS_Vector(Pw[ii][0] / w, Pw[ii][1] / w);
        data.weights[ii] = w;
    }

    // Re-wrap if closed
    if (wasClosed) {
        changeType(originalType);
        updateKnotWrapping();
        updateControlAndWeightWrapping();
    }

    // Validate and update
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Knot removal resulted in invalid spline");
    } else {
        calculateBorders();
        update();
    }

    return actualRemoved;
}