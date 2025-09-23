/**
* @file rs_spline.cpp
* @brief Implements the action class for spline drawing.
*
* @author R. van Twisk (librecad@rvt.dds.nl)
* @author Dongxu Li (dongxuli2011@gmail.com)
*
*/

#include <cmath>
#include <iostream>

#include "rs_spline.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

RS_SplineData::RS_SplineData(int degree, bool closed):
    degree{degree}
    , closed{closed}
{}


RS_Spline::RS_Spline(RS_EntityContainer* parent, const RS_SplineData& d)
    : RS_EntityContainer(parent)
    , data(d) {
    // Ensure weights and knots are consistent if needed
}

RS_Entity* RS_Spline::clone() const {
    RS_Spline* s = new RS_Spline(*this);
    s->initId();
    return s;
}

void RS_Spline::setDegree(int deg) {
    if (deg >= 1 && deg <= 3) {
        data.degree = deg;
        update();
    }
}

int RS_Spline::getDegree() const {
    return data.degree;
}

size_t RS_Spline::getNumberOfControlPoints() const {
    return data.controlPoints.size();
}

bool RS_Spline::isClosed() const {
    return data.closed;
}

void RS_Spline::setClosed(bool c) {
    data.closed = c;
    update();
}

const std::vector<double>& RS_Spline::getWeights() const {
    return data.weights;
}

void RS_Spline::setWeights(const std::vector<double>& w) {
    data.weights = w;
    update();
}

std::vector<double> RS_Spline::getEffectiveWeights() const {
    std::vector<double> w = data.weights;
    size_t n = getNumberOfControlPoints();
    if (w.size() != n) {
        w.resize(n, 1.0);
    }
    return w;
}

std::vector<double> RS_Spline::getKnotVector() const {
    if (data.knotslist.empty()) {
        size_t num = getNumberOfControlPoints();
        size_t order = getDegree() + 1;
        return isClosed() ? knotu(num, order) : knot(num, order);
    } else {
        return data.knotslist;
    }
}

void RS_Spline::setKnotVector(const std::vector<double>& knots) {
    data.knotslist = knots;
    update();
}

void RS_Spline::setControlPoints(const std::vector<RS_Vector>& cp) {
    data.controlPoints = cp;
    update();
}

const std::vector<RS_Vector>& RS_Spline::getControlPoints() const {
    return data.controlPoints;
}

RS_VectorSolutions RS_Spline::getRefPoints() const {
    RS_VectorSolutions ret;
    for (const auto& cp : data.controlPoints) ret.push_back(cp);
    return ret;
}

RS_Vector RS_Spline::getNearestRef(const RS_Vector& coord, double* dist) const {
    return getNearestPointOnEntity(coord, false, dist, nullptr);
}

RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return getNearestRef(coord, dist);
}

RS_Vector RS_Spline::getNearestPointOnEntity(const RS_Vector &coord, bool onEntity, double *dist, RS_Entity **entity) const {
    if (entity) *entity = const_cast<RS_Spline*>(this);
    // For now, approximate with control points; better to evaluate curve
    double minDist = RS_MAXDOUBLE;
    RS_Vector closest = RS_Vector(false);
    for (const auto& cp : data.controlPoints) {
        double d = coord.distanceTo(cp);
        if (d < minDist) {
            minDist = d;
            closest = cp;
        }
    }
    if (dist) *dist = minDist;
    return closest;
}

RS_Vector RS_Spline::getStartpoint() const {
    if (data.controlPoints.empty()) return RS_Vector(false);
    return data.controlPoints.front();
}

RS_Vector RS_Spline::getEndpoint() const {
    if (data.controlPoints.empty()) return RS_Vector(false);
    return data.controlPoints.back();
}

void RS_Spline::update() {
    calculateBorders();
}

RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord,
                                 double* dist)const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector closest = RS_Vector(false);

    double distToStart = coord.distanceTo(getStartpoint());
    if (distToStart < minDist) {
        minDist = distToStart;
        closest = getStartpoint();
    }

    double distToEnd = coord.distanceTo(getEndpoint());
    if (distToEnd < minDist) {
        minDist = distToEnd;
        closest = getEndpoint();
    }

    if (dist) *dist = minDist;
    return closest;
}

RS_Vector RS_Spline::getNearestCenter(const RS_Vector& coord,
                               double* dist)const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& coord,
                               double* dist,
                               int middlePoints)const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

RS_Vector RS_Spline::getNearestDist(double distance,
                             const RS_Vector& coord,
                             double* dist)const {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
}

void RS_Spline::addControlPoint(const RS_Vector& v, double w) {
    data.controlPoints.push_back(v);
    if (data.weights.size() == data.controlPoints.size() - 1) {
        data.weights.push_back(w);
    }
    update();
}

void RS_Spline::removeLastControlPoint() {
    if (!data.controlPoints.empty()) {
        data.controlPoints.pop_back();
        if (data.weights.size() == data.controlPoints.size() + 1) {
            data.weights.pop_back();
        }
    }
    update();
}

void RS_Spline::move(const RS_Vector& offset) {
    for (auto& cp : data.controlPoints) {
        cp += offset;
    }
    calculateBorders();
}

void RS_Spline::rotate(const RS_Vector& center, double angle) {
    for (auto& cp : data.controlPoints) {
        cp.rotate(center, angle);
    }
    calculateBorders();
}

void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& cp : data.controlPoints) {
        cp.rotate(center, angleVector);
    }
    calculateBorders();
}

void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& cp : data.controlPoints) {
        cp.scale(center, factor);
    }
    calculateBorders();
}

void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& cp : data.controlPoints) {
        cp.mirror(axisPoint1, axisPoint2);
    }
    calculateBorders();
}

RS_Entity& RS_Spline::shear(double k) {
    for (auto& cp : data.controlPoints) {
        cp.x += k * cp.y;
    }
    calculateBorders();
    return *this;
}

void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    for (auto& cp : data.controlPoints) {
        if (cp.distanceTo(ref) < RS_TOLERANCE) {
            cp += offset;
        }
    }
    update();
}

void RS_Spline::revertDirection() {
    std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    if (!data.weights.empty()) std::reverse(data.weights.begin(), data.weights.end());
    if (!data.knotslist.empty()) std::reverse(data.knotslist.begin(), data.knotslist.end());
    update();
}

void RS_Spline::draw(RS_Painter* painter) {
    if (painter == nullptr) return;
    std::vector<RS_Vector> points;
    fillStrokePoints(20, points); // Adjust segments for smoothness
    for (size_t i = 0; i < points.size() - 1; ++i) {
        RS_Line lineSegment{nullptr, {points[i], points[i+1]}};
        lineSegment.setPen(getPen());
        lineSegment.draw(painter);
    }
}

std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << "Spline: " << l.getData();
    return os;
}

void RS_Spline::calculateBorders() {
    resetBorders();
    for (const auto& cp : data.controlPoints) {
        minV = RS_Vector::minimum(minV, cp);
        maxV = RS_Vector::maximum(maxV, cp);
    }
}

void RS_Spline::fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) {
    size_t npts = splineSegments * (getNumberOfControlPoints() - getDegree());
    size_t k = getDegree() + 1;
    size_t p1 = npts;
    const auto& b = getControlPoints();
    const auto& h = getEffectiveWeights();
    points.reserve(npts);
    if (isClosed()) rbsplinu(npts, k, p1, b, h, points);
    else rbspline(npts, k, p1, b, h, points);
}

std::vector<double> RS_Spline::knot(size_t num, size_t order) const {
    std::vector<double> kn(num + order, 0.0);
    double step = 1.0 / static_cast<double>(num - order + 1);
    for (size_t i = order - 1; i < num + 1; ++i) {
        kn[i] = static_cast<double>(i - order + 1) * step;
    }
    for (size_t i = num + 1; i < num + order; ++i) kn[i] = 1.0;
    return kn;
}

std::vector<double> RS_Spline::knotu(size_t num, size_t order) const {
    std::vector<double> kn(num + order);
    for (size_t i = 0; i < num + order; ++i) {
        kn[i] = static_cast<double>(i) / static_cast<double>(num);
    }
    return kn;
}

void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const {
    size_t n = b.size() - 1;
    auto knots = knot(n + 1, k);
    double step = (knots[n + 1] - knots[k - 1]) / (p1 - 1.0);

    for (double t = knots[k - 1]; t <= knots[n + 1] + 1e-6; t += step) {
        // find interval
        int l = k - 1;
        for (int j = k - 1; j <= n; ++j) {
            if (t >= knots[j] && t < knots[j + 1]) {
                l = j;
                break;
            }
        }
        if (l < k - 1 || l > n) continue;

        int j = l - k + 1;
        std::vector<RS_Vector> rw(k); // rational weighted points
        for (int i = 0; i < k; ++i) {
            rw[i] = b[j + i] * h[j + i];
        }
        std::vector<double> a(k, 0.0); // alpha
        for (int r = 1; r < k; ++r) {
            for (int i = r; i < k; ++i) {
                a[i] = (t - knots[j + i]) / (knots[j + i + k - r] - knots[j + i]);
                rw[i] = rw[i] * a[i] + rw[i - 1] * (1.0 - a[i]);
            }
        }

        double wt = 0.0;
        for (int i = 0; i < k; ++i) {
            wt += a[i] * h[j + i]; // Approximate weight, but for accuracy, need separate weight recursion
        }
        p.push_back(rw[k - 1] / wt);
    }
}

void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const {
    size_t n = b.size() - 1;
    auto knots = knotu(n + 1, k);
    double step = knots[n + 1] / p1;

    for (double t = 0.0; t < knots[n + 1] + step; t += step) {
        double tt = std::fmod(t, knots[n + 1]);
        // find interval with wrap
        int l = k - 1;
        for (int j = k - 1; j <= n; ++j) {
            if (tt >= knots[j] && tt < knots[j + 1]) {
                l = j;
                break;
            }
        }
        if (l < k - 1) l = n; // wrap

        int j = (l - k + 1 + n + 1) % (n + 1);
        std::vector<RS_Vector> rw(k);
        for (int i = 0; i < k; ++i) {
            int idx = (j + i) % (n + 1);
            rw[i] = b[idx] * h[idx];
        }
        std::vector<double> a(k, 0.0);
        for (int r = 1; r < k; ++r) {
            for (int i = r; i < k; ++i) {
                int idx1 = (j + i + n + 1) % (n + 1);
                int idx2 = (j + i + k - r + n + 1) % (n + 1);
                a[i] = (tt - knots[idx1]) / (knots[idx2] - knots[idx1]);
                rw[i] = rw[i] * a[i] + rw[i - 1] * (1.0 - a[i]);
            }
        }

        double wt = 0.0;
        for (int i = 0; i < k; ++i) {
            int idx = (j + i) % (n + 1);
            wt += a[i] * h[idx]; // Approximate
        }
        p.push_back(rw[k - 1] / wt);
    }
}

bool RS_Spline::hasWrappedControlPoints() const {
    if (getDegree() != 3 || !isClosed() || data.controlPoints.size() < 6) return false;
    size_t n = data.controlPoints.size();
    return data.controlPoints[n - 1] == data.controlPoints[2] &&
           data.controlPoints[n - 2] == data.controlPoints[1] &&
           data.controlPoints[n - 3] == data.controlPoints[0];
}

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld) {
    os << "degree: " << ld.degree << " closed: " << ld.closed << " controlPoints: " << ld.controlPoints.size() << " knots: " << ld.knotslist.size() << " weights: " << ld.weights.size();
    return os;
}
