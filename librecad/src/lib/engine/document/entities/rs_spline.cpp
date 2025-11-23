/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY. See the GNU General Public License for details.
**
**********************************************************************/
#include <algorithm>
#include <iostream>

#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_spline.h"

namespace {
constexpr double g_knotTolerance = 5e-6;

bool compareVector(const RS_Vector &va, const RS_Vector &vb,
                   double tol = RS_TOLERANCE) {
  return va.distanceTo(vb) <= tol;
}
} // namespace

/** Constructor for RS_SplineData */
RS_SplineData::RS_SplineData(int _degree, bool _closed) : degree(_degree) {
  type = _closed ? SplineType::WrappedClosed : SplineType::ClampedOpen;
  savedOpenType = type;
}

/** Stream operator for RS_SplineData */
std::ostream &operator<<(std::ostream &os, const RS_SplineData &ld) {
  os << "(degree:" << ld.degree << " type:" << static_cast<int>(ld.type);
  if (!ld.controlPoints.empty())
    os << " cps:" << ld.controlPoints.size();
  if (!ld.knotslist.empty())
    os << " knots:" << ld.knotslist.size();
  if (!ld.weights.empty())
    os << " weights:" << ld.weights.size();
  if (!ld.fitPoints.empty())
    os << " fit:" << ld.fitPoints.size();
  os << ")";
  return os;
}

/** Constructor */
RS_Spline::RS_Spline(RS_EntityContainer *parent, const RS_SplineData &d)
    : RS_EntityContainer(parent), data(d) {
  if (data.type == RS_SplineData::SplineType::WrappedClosed &&
      !hasWrappedControlPoints())
    addWrapping();
  RS_Spline::calculateBorders();
  RS_Spline::update();
}

/** Clone */
RS_Entity *RS_Spline::clone() const {
  auto *l = new RS_Spline(*this);
  l->setOwner(isOwner());
  l->detach();
  return l;
}

/** Data access */
RS_SplineData &RS_Spline::getData() { return data; }
const RS_SplineData &RS_Spline::getData() const { return data; }

/** Unwrapped size */
size_t RS_Spline::getUnwrappedSize() const {
  size_t s = data.controlPoints.size();
  if (s <= data.degree)
    return s;
  if (data.type == RS_SplineData::SplineType::WrappedClosed) {
    s -= (s > data.degree) ? data.degree : s;
  }
  return s;
}

/** Unwrapped vectors */
std::vector<RS_Vector> RS_Spline::getUnwrappedControlPoints() const {
  size_t s = getUnwrappedSize();
  return s ? std::vector<RS_Vector>(data.controlPoints.begin(),
                                    data.controlPoints.begin() + s)
           : std::vector<RS_Vector>{};
}

std::vector<double> RS_Spline::getUnwrappedWeights() const {
  size_t s = getUnwrappedSize();
  return s ? std::vector<double>(data.weights.begin(), data.weights.begin() + s)
           : std::vector<double>{};
}

std::vector<double> RS_Spline::getUnwrappedKnotVector() const {
  size_t s = getUnwrappedSize();
  if (!s)
    return {};
  size_t bs = s + data.degree + 1;
  return bs <= data.knotslist.size()
             ? std::vector<double>(data.knotslist.begin(),
                                   data.knotslist.begin() + bs)
             : std::vector<double>{};
}

/** Wrapping helpers */
void RS_Spline::removeWrapping() {
  LC_SplineHelper::removeWrapping(data);
}

void RS_Spline::addWrapping() {
  LC_SplineHelper::addWrapping(data);
}

void RS_Spline::updateControlAndWeightWrapping() {
  LC_SplineHelper::updateControlAndWeightWrapping(data, isClosed(),
                                                  getUnwrappedSize());
}

void RS_Spline::updateKnotWrapping() {
  LC_SplineHelper::updateKnotWrapping(data, isClosed(), getUnwrappedSize());
}

/** Borders */
void RS_Spline::calculateBorders() {
  resetBorders();
  size_t s = getUnwrappedSize();
  if (!s)
    return;
  for (size_t i = 0; i < s; ++i) {
    minV = RS_Vector::minimum(data.controlPoints[i], minV);
    maxV = RS_Vector::maximum(data.controlPoints[i], maxV);
  }
}

/** Degree */
void RS_Spline::setDegree(int d) {
  if (d < 1 || d > 3)
    throw std::invalid_argument("Degree must be 1-3");
  data.degree = d;
}
int RS_Spline::getDegree() const { return data.degree; }

/** Counts */
size_t RS_Spline::getNumberOfControlPoints() const {
  return getUnwrappedSize();
}
size_t RS_Spline::getNumberOfKnots() const { return data.knotslist.size(); }

/** Closed */
bool RS_Spline::isClosed() const {
  return data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Set closed */
void RS_Spline::setClosed(bool c) {
  if (c == isClosed())
    return;
  if (getUnwrappedSize() <= data.degree) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Spline::setClosed: insufficient points");
    data.type = c ? RS_SplineData::SplineType::WrappedClosed : RS_SplineData::SplineType::ClampedOpen;
    return;
  }
  if (c) {
    if (data.type == RS_SplineData::SplineType::ClampedOpen)
      LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
    else if (data.type == RS_SplineData::SplineType::Standard)
      LC_SplineHelper::toWrappedClosedFromStandard(data);
    else
      assert(false && "unknown spline type for closing");
  } else {
    if (data.type == RS_SplineData::SplineType::WrappedClosed)
      LC_SplineHelper::toClampedOpenFromWrappedClosed(data);
    else if (data.type == RS_SplineData::SplineType::Standard)
      LC_SplineHelper::toClampedOpenFromStandard(data);
    else
      assert(false && "unknown spline type for closing");
  }
  update();
}

/** Change type */
void RS_Spline::changeType(RS_SplineData::SplineType newType) {
  if (data.type == newType)
    return;
  RS_SplineData::SplineType oldType = data.type;
  if (data.controlPoints.size() < data.degree + 1) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Spline::changeType: insufficient points");
    data.type = newType;
    return;
  }

  if (newType == RS_SplineData::SplineType::Standard) {
    if (oldType == RS_SplineData::SplineType::ClampedOpen)
      LC_SplineHelper::toStandardFromClampedOpen(data);
    else if (oldType == RS_SplineData::SplineType::WrappedClosed)
      LC_SplineHelper::toStandardFromWrappedClosed(data);
  } else if (newType == RS_SplineData::SplineType::ClampedOpen) {
    if (oldType == RS_SplineData::SplineType::Standard)
      LC_SplineHelper::toClampedOpenFromStandard(data);
    else if (oldType == RS_SplineData::SplineType::WrappedClosed)
      LC_SplineHelper::toClampedOpenFromWrappedClosed(data);
  } else if (newType == RS_SplineData::SplineType::WrappedClosed) {
    if (oldType == RS_SplineData::SplineType::Standard)
      LC_SplineHelper::toWrappedClosedFromStandard(data);
    else if (oldType == RS_SplineData::SplineType::ClampedOpen)
      LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
  }

         // even if LC_SplineHelper fails to change the type, still change the enum
  data.type = newType;
}

/** Clamped knot adjustment */
std::vector<double>
RS_Spline::adjustToOpenClamped(const std::vector<double> &knots,
                               size_t num_control, size_t order, bool) const {
  return LC_SplineHelper::clampKnotVector(knots, num_control, order);
}

/** Reference points */
RS_VectorSolutions RS_Spline::getRefPoints() const {
  return RS_VectorSolutions(getControlPoints());
}
RS_Vector RS_Spline::getNearestRef(const RS_Vector &coord, double *dist) const {
  return RS_EntityContainer::getNearestRef(coord, dist);
}
RS_Vector RS_Spline::getNearestSelectedRef(const RS_Vector &coord,
                                           double *dist) const {
  return RS_EntityContainer::getNearestSelectedRef(coord, dist);
}

/** Update approximation */
void RS_Spline::update() {
  clear();
  if (!validate())
    return;
  std::vector<RS_Vector> points;
  fillStrokePoints(32, points);
  for (size_t i = 0; i + 1 < points.size(); ++i)
    addEntity(new RS_Line(this, points[i], points[i + 1]));
  if (isClosed() && points.size() > 1)
    addEntity(new RS_Line(this, points.back(), points.front()));
}

/** Stroke points */
void RS_Spline::fillStrokePoints(int segments, std::vector<RS_Vector> &points) {
  const auto &kv = data.knotslist;
  double tmin = kv[data.degree];
  // Fixed: correct tmax that works for both open and wrapped-closed splines
  double tmax = data.knotslist[data.knotslist.size() - data.degree - 1];
  double step = (tmax - tmin) / segments;
  for (int i = 0; i <= segments; ++i)
    points.push_back(getPointAt(tmin + i * step));
}

/** Endpoints (invalid if closed) */
RS_Vector RS_Spline::getStartpoint() const { return RS_Vector(false); }
RS_Vector RS_Spline::getEndpoint() const { return RS_Vector(false); }

/** Nearest (invalid overrides) */
RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector &, double *) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::getNearestCenter(const RS_Vector &, double *) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::getNearestMiddle(const RS_Vector &, double *, int) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::getNearestDist(double, const RS_Vector &, double *) const {
  return RS_Vector(false);
}

/** Transformations */
void RS_Spline::move(const RS_Vector &offset) {
  for (auto &cp : data.controlPoints)
    cp += offset;
  for (auto &fp : data.fitPoints)
    fp += offset;
  calculateBorders();
}

void RS_Spline::rotate(const RS_Vector &center, double angle) {
  for (auto &cp : data.controlPoints)
    cp.rotate(center, angle);
  for (auto &fp : data.fitPoints)
    fp.rotate(center, angle);
  calculateBorders();
}

void RS_Spline::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  for (auto &cp : data.controlPoints)
    cp.rotate(center, angleVector);
  for (auto &fp : data.fitPoints)
    fp.rotate(center, angleVector);
  calculateBorders();
}

void RS_Spline::scale(const RS_Vector &center, const RS_Vector &factor) {
  for (auto &cp : data.controlPoints)
    cp.scale(center, factor);
  for (auto &fp : data.fitPoints)
    fp.scale(center, factor);
  calculateBorders();
}

RS_Entity &RS_Spline::shear(double k) {
  for (auto &cp : data.controlPoints)
    cp.shear(k);
  for (auto &fp : data.fitPoints)
    fp.shear(k);
  calculateBorders();
  return *this;
}

void RS_Spline::mirror(const RS_Vector &a1, const RS_Vector &a2) {
  for (auto &cp : data.controlPoints)
    cp.mirror(a1, a2);
  for (auto &fp : data.fitPoints)
    fp.mirror(a1, a2);
  calculateBorders();
}

void RS_Spline::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
  RS_EntityContainer::moveRef(ref, offset);
}

/** Revert direction */
void RS_Spline::revertDirection() {
  std::reverse(data.controlPoints.begin(), data.controlPoints.end());
  std::reverse(data.weights.begin(), data.weights.end());
  std::reverse(data.knotslist.begin(), data.knotslist.end());
  normalizeKnots();
  update();
}

/** Draw */
void RS_Spline::draw(RS_Painter *painter) { RS_EntityContainer::draw(painter); }

/** Accessors */
std::vector<RS_Vector> RS_Spline::getControlPoints() const {
  return getUnwrappedControlPoints();
}
std::vector<double> RS_Spline::getWeights() const {
  return getUnwrappedWeights();
}
double RS_Spline::getWeight(size_t i) const {
  return i < data.weights.size() ? data.weights[i] : 1.0;
}

/** Add control point */
void RS_Spline::addControlPoint(const RS_Vector &v, double w) {
  RS_SplineData::SplineType newType = data.type;
  changeType(RS_SplineData::SplineType::Standard);
  addControlPointRaw(v, w);
  size_t n = data.controlPoints.size();
  if (n >= static_cast<size_t>(data.degree + 1)) {
    if (data.knotslist.empty()) {
      data.knotslist =
          LC_SplineHelper::generateOpenUniformKnotVector(n, data.degree + 1);
    }
    else
      LC_SplineHelper::extendKnotVector(data.knotslist);
    LC_SplineHelper::ensureMonotonic(data.knotslist);
  }
  changeType(newType);
  calculateBorders();
  update();
}

void RS_Spline::addControlPointRaw(const RS_Vector &v, double w) {
  data.controlPoints.push_back(v);
  data.weights.push_back(w);
}

/** Remove last */
void RS_Spline::removeLastControlPoint() {
  if (!data.controlPoints.empty()) {
    data.controlPoints.pop_back();
    data.weights.pop_back();
  }
  if (isClosed())
    updateControlAndWeightWrapping();
  update();
}

/** Set weights/control/knots */
void RS_Spline::setWeight(size_t i, double w) {
  if (i < data.weights.size()) {
    data.weights[i] = w;
    if (isClosed())
      updateControlAndWeightWrapping();
    update();
  }
}
void RS_Spline::setWeights(const std::vector<double> &w) {
  data.weights = w;
  if (isClosed())
    updateControlAndWeightWrapping();
  update();
}
void RS_Spline::setControlPoint(size_t i, const RS_Vector &v) {
  if (i < data.controlPoints.size()) {
    data.controlPoints[i] = v;
    if (isClosed())
      updateControlAndWeightWrapping();
    update();
  }
}
void RS_Spline::setKnot(size_t i, double k) {
  if (i < data.knotslist.size()) {
    data.knotslist[i] = k;
    LC_SplineHelper::ensureMonotonic(data.knotslist);
    if (isClosed())
      updateKnotWrapping();
    update();
  }
}

/** Insert/remove control point */
void RS_Spline::insertControlPoint(size_t i, const RS_Vector &v, double w,
                                   bool preserve) {
  data.controlPoints.insert(data.controlPoints.begin() + i, v);
  data.weights.insert(data.weights.begin() + i, w);
  if (!preserve)
    data.knotslist.clear();
  if (isClosed())
    updateControlAndWeightWrapping();
  update();
}

void RS_Spline::removeControlPoint(size_t i) {
  if (i < data.controlPoints.size()) {
    data.controlPoints.erase(data.controlPoints.begin() + i);
    data.weights.erase(data.weights.begin() + i);
    data.knotslist.clear();
  }
  if (isClosed())
    updateControlAndWeightWrapping();
  update();
}

/** Knot vector access */
std::vector<double> RS_Spline::getKnotVector() const {
  return getUnwrappedKnotVector();
}
void RS_Spline::setKnotVector(const std::vector<double> &k) {
  data.knotslist = k;
  LC_SplineHelper::ensureMonotonic(data.knotslist);
  if (isClosed())
    updateKnotWrapping();
  if (!validate())
    RS_DEBUG->print(RS_Debug::D_WARNING, "invalid knot vector");
  update();
}

/** Knot generators */
std::vector<double> RS_Spline::knot(size_t num, size_t order) const {
  return LC_SplineHelper::knot(num, order);
}
std::vector<double> RS_Spline::openUniformKnot(size_t num, size_t order) const {
  return LC_SplineHelper::generateOpenUniformKnotVector(num, order);
}
std::vector<double> RS_Spline::knotu(size_t num, size_t order) const {
  return LC_SplineHelper::convertOpenToClosedKnotVector(
      openUniformKnot(num, order), num, order - 1);
}

/** Rational B-spline point generation */
void RS_Spline::rbspline(size_t npts, size_t k, size_t numPoints,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
  int nplusc = npts + k;
  std::vector<double> x(nplusc, 0.0);
  for (int i = 1; i < nplusc; ++i)
    x[i] = (i >= static_cast<int>(k) && i <= static_cast<int>(npts))
               ? x[i - 1] + 1.0
               : x[i - 1];
  double t = 0.0, step = x.back() / numPoints;
  for (int idx = 1; idx <= static_cast<int>(numPoints); ++idx) {
    if (x.back() - t < g_knotTolerance)
      t = x.back() - g_knotTolerance;
    auto nb = rbasis(k, t, npts, x, h);
    RS_Vector pt(0.0, 0.0);
    for (int i = 0; i < static_cast<int>(npts); ++i)
      pt += b[i] * nb[i];
    p.push_back(pt);
    t += step;
  }
}

void RS_Spline::rbsplinu(size_t npts, size_t k, size_t numPoints,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
  int nplusc = npts + k;
  std::vector<double> x(nplusc, 0.0);
  for (int i = 1; i < nplusc; ++i)
    x[i] = x[i - 1] + 1.0;
  double t = k - 1, step = static_cast<double>(npts) / numPoints;
  for (int idx = 1; idx <= static_cast<int>(numPoints); ++idx) {
    if (x.back() - t < g_knotTolerance)
      t = x.back() - g_knotTolerance;
    auto nb = rbasis(k, t, npts, x, h);
    RS_Vector pt(0.0, 0.0);
    for (int i = 0; i < static_cast<int>(npts); ++i)
      pt += b[i] * nb[i];
    p.push_back(pt);
    t += step;
  }
}

/** Wrapped check */
bool RS_Spline::hasWrappedControlPoints() const {
  size_t total = data.controlPoints.size();
  if (total <= data.degree ||
      data.type != RS_SplineData::SplineType::WrappedClosed)
    return false;
  size_t unwrapped = total - data.degree;
  for (size_t i = 0; i < data.degree; ++i) {
    if (!compareVector(data.controlPoints[unwrapped + i],
                       data.controlPoints[i]))
      return false;
  }
  return true;
}

/** Output */
std::ostream &operator<<(std::ostream &os, const RS_Spline &l) {
  os << "RS_Spline: " << l.getData();
  return os;
}

/** Evaluation */
RS_Vector RS_Spline::getPointAt(double t) const {
  return evaluateNURBS(data, t);
}

/** Derivative zeros */
std::vector<double> RS_Spline::findDerivativeZeros(bool isX) const {
  std::vector<double> zeros;
  const auto& U = data.knotslist;
  size_t p = data.degree;
  size_t n = data.controlPoints.size() - 1;
  if (n < p)
    return zeros;

  auto d = [this, isX](double t){ return getDerivative(t, isX); };

  auto add_if = [&](double a, double b, double fa, double fb){
    if ((fa * fb <= 0.0 || std::abs(fa) < 1e-9 || std::abs(fb) < 1e-9) && b - a > 1e-12)
      zeros.push_back(bisectDerivativeZero(a, b, fa, isX));
  };

  double f0 = d(U[p]);
  for (size_t i = p; i <= n; ++i) {
    double t1 = U[i + 1];
    double fm = d((U[i] + t1) * 0.5);
    double f1 = d(t1);

    add_if(U[i],   t1, f0, fm);   // left half (f0 reused from previous)
    add_if(t1, t1, fm, f1);      // right half

    f0 = f1;                       // chain for next span
  }

         // endpoints if derivative ≈ 0
  if (std::abs(d(U[p]))   < 1e-9) zeros.push_back(U[p]);
  if (std::abs(d(U[n+1])) < 1e-9) zeros.push_back(U[n+1]);

  std::sort(zeros.begin(), zeros.end());
  zeros.erase(std::unique(zeros.begin(), zeros.end(), [](double a, double b){
                return std::abs(a - b) < 1e-8; }), zeros.end());

  return zeros;
}

/** Tight borders */
void RS_Spline::calculateTightBorders() {
  resetBorders();
  if (!isClosed()) {
    minV = RS_Vector::minimum(getStartpoint(), minV);
    maxV = RS_Vector::maximum(getStartpoint(), maxV);
    minV = RS_Vector::minimum(getEndpoint(), minV);
    maxV = RS_Vector::maximum(getEndpoint(), maxV);
  }
  for (double t : findDerivativeZeros(true)) {
    RS_Vector pt = getPointAt(t);
    minV = RS_Vector::minimum(pt, minV);
    maxV = RS_Vector::maximum(pt, maxV);
  }
  for (double t : findDerivativeZeros(false)) {
    RS_Vector pt = getPointAt(t);
    minV = RS_Vector::minimum(pt, minV);
    maxV = RS_Vector::maximum(pt, maxV);
  }
}

/** NURBS evaluation */
RS_Vector RS_Spline::evaluateNURBS(const RS_SplineData &d, double t) {
  size_t p = d.degree;
  size_t n = d.controlPoints.size() - 1;
  if (n < p)
    return RS_Vector(false);
  size_t span = findSpan(n, p, t, d.knotslist);
  auto basis = basisFunctions(span, t, p, d.knotslist);
  RS_Vector pt(0.0, 0.0);
  double wsum = 0.0;
  for (size_t i = 0; i <= p; ++i) {
    size_t idx = span - p + i;
    double w = d.weights[idx];
    pt += d.controlPoints[idx] * basis[i] * w;
    wsum += basis[i] * w;
  }
  return wsum > RS_TOLERANCE ? pt / wsum : pt;
}

/** rbasis */
std::vector<double> RS_Spline::rbasis(int c, double t, int npts,
                                      const std::vector<double> &x,
                                      const std::vector<double> &h) {
  const int nplusc = npts + c;
  std::vector<double> alpha(nplusc, 0.0);
  for (int i = 0; i < nplusc - 1; ++i)
    if (t >= x[i] && t < x[i + 1])
      alpha[i] = 1.0;
  for (int k = 2; k <= c; ++k) {
    for (int i = 0; i < nplusc - k; ++i) {
      if (alpha[i] != 0.0)
        alpha[i] = ((t - x[i]) * alpha[i]) / (x[i + k - 1] - x[i]);
      if (alpha[i + 1] != 0.0)
        alpha[i] += ((x[i + k] - t) * alpha[i + 1]) / (x[i + k] - x[i + 1]);
    }
  }
  if (t >= x[nplusc - 1])
    alpha[npts - 1] = 1.0;
  double sum = 0.0;
  for (int i = 0; i < npts; ++i)
    sum += alpha[i] * h[i];
  std::vector<double> r(npts, 0.0);
  if (sum != 0.0)
    for (int i = 0; i < npts; ++i)
      r[i] = alpha[i] * h[i] / sum;
  return r;
}

/** Fit points interpolation */
void RS_Spline::setFitPoints(const std::vector<RS_Vector> &fp,
                             bool centripetal) {
  if (fp.size() < static_cast<size_t>(data.degree + 1)) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Spline::setFitPoints: not enough points");
    return;
  }
  data.controlPoints.clear();
  data.weights.clear();
  data.knotslist.clear();
  data.fitPoints = fp;
  size_t np = fp.size();
  int n = np - 1;
  int p = data.degree;
  bool closed = isClosed();

  std::vector<double> t(np + (closed ? 1 : 0), 0.0);
  double total = 0.0, alpha = centripetal ? 0.5 : 1.0;
  for (size_t k = 1; k < np; ++k)
    total += std::pow(fp[k].distanceTo(fp[k - 1]), alpha);
  if (closed)
    total += std::pow(fp.back().distanceTo(fp.front()), alpha);
  if (total < RS_TOLERANCE) {
    for (size_t k = 1; k < np; ++k)
      t[k] = static_cast<double>(k) / n;
    if (closed)
      t[np] = 1.0;
  } else {
    double cum = 0.0;
    for (size_t k = 1; k < np; ++k) {
      cum += std::pow(fp[k].distanceTo(fp[k - 1]), alpha);
      t[k] = cum / total;
    }
    if (closed) {
      cum += std::pow(fp.back().distanceTo(fp.front()), alpha);
      t[np] = cum / total;
    }
  }

  std::vector<double> U(np + p + 1, 0.0);
  if (closed) {
    for (int j = p; j < static_cast<int>(np); ++j) {
      double sum = 0.0;
      for (int i = j - p; i < j; ++i)
        sum += t[i + 1];
      U[j] = sum / p;
    }
    for (int j = 0; j < p; ++j)
      U[j] = U[j + static_cast<int>(np) - p] - 1.0;
    for (size_t j = np; j < U.size(); ++j)
      U[j] = U[j - np + p] + 1.0;
  } else {
    std::fill(U.begin(), U.begin() + p + 1, 0.0);
    for (int j = 1; j <= n - p; ++j) {
      double sum = 0.0;
      for (int i = j; i <= j + p - 1; ++i)
        sum += t[i];
      U[j + p] = sum / p;
    }
    std::fill(U.end() - p - 1, U.end(), 1.0);
  }
  data.knotslist = U;

  std::vector<std::vector<double>> N(np, std::vector<double>(np, 0.0));
  for (size_t k = 0; k < np; ++k)
    N[k] = getBSplineBasis(t[k], U, p, np);

  std::vector<double> bx(np), by(np);
  for (size_t i = 0; i < np; ++i) {
    bx[i] = fp[i].x;
    by[i] = fp[i].y;
  }

  std::vector<double> px(np), py(np);
  std::vector<std::vector<double>> mx(np, std::vector<double>(np + 1, 0.0));
  std::vector<std::vector<double>> my(np, std::vector<double>(np + 1, 0.0));
  for (size_t i = 0; i < np; ++i) {
    for (size_t j = 0; j < np; ++j) {
      mx[i][j] = my[i][j] = N[i][j];
    }
    mx[i][np] = bx[i];
    my[i][np] = by[i];
  }
  if (!RS_Math::linearSolver(mx, px) || !RS_Math::linearSolver(my, py)) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "singular matrix");
    return;
  }

  for (size_t i = 0; i < np; ++i)
    data.controlPoints.push_back(RS_Vector(px[i], py[i]));
  data.weights.assign(np, 1.0);
  if (closed)
    addWrapping();
  update();
}

/** B-spline basis */
std::vector<double> RS_Spline::getBSplineBasis(double t,
                                               const std::vector<double> &knots,
                                               int degree,
                                               size_t numControls) const {
  int order = degree + 1, np = static_cast<int>(numControls), c = order,
      nplusc = np + c;
  std::vector<double> bf(nplusc, 0.0);
  for (int i = 0; i < nplusc - 1; ++i)
    if (t >= knots[i] - RS_TOLERANCE && t < knots[i + 1] + RS_TOLERANCE)
      bf[i] = 1.0;
  for (int k = 2; k <= c; ++k) {
    for (int i = 0; i < nplusc - k; ++i) {
      double d1 =
          bf[i] != 0.0 && std::abs(knots[i + k - 1] - knots[i]) > RS_TOLERANCE
              ? (t - knots[i]) * bf[i] / (knots[i + k - 1] - knots[i])
              : 0.0;
      double d2 =
          bf[i + 1] != 0.0 &&
                  std::abs(knots[i + k] - knots[i + 1]) > RS_TOLERANCE
              ? (knots[i + k] - t) * bf[i + 1] / (knots[i + k] - knots[i + 1])
              : 0.0;
      bf[i] = d1 + d2;
    }
  }
  if (t + RS_TOLERANCE >= knots[nplusc - 1])
    bf[np - 1] = 1.0;
  return {bf.begin(), bf.begin() + np};
}

/** Validate */
// rs_spline.cpp - only the changed part of validate() function (ClampedOpen block)

// rs_spline.cpp - only the changed validate() function (full function shown for clarity)

bool RS_Spline::validate() const
{
  const size_t degree = data.degree;

         // Basic checks
  const size_t num_cp = data.controlPoints.size();           // total (wrapped if closed)
  if (num_cp < degree + 1)
    return false;

  const bool closed = isClosed();                             // WrappedClosed
  const size_t extra_cp = closed ? degree : 0;
  const size_t unwrapped_cp = num_cp - extra_cp;

  if (unwrapped_cp < degree + 1)
    return false;

  const size_t expected_knots = num_cp + degree + 1;          // works for open and wrapped closed
  if (data.knotslist.size() != expected_knots)
    return false;
  if (data.weights.size() != num_cp)
    return false;

         // Weights must be positive
  for (double w : data.weights)
    if (w <= 0.0)
      return false;

         // Knots must be non-decreasing
  for (size_t i = 1; i < data.knotslist.size(); ++i)
    if (data.knotslist[i] < data.knotslist[i - 1] - RS_TOLERANCE)
      return false;

         // Maximum multiplicity anywhere ≤ degree+1
  {
    size_t mult = 1;
    for (size_t i = 1; i < data.knotslist.size(); ++i) {
      if (std::abs(data.knotslist[i] - data.knotslist[i - 1]) < RS_TOLERANCE) {
        ++mult;
        if (mult > degree + 1)
          return false;
      } else {
        mult = 1;
      }
    }
  }

         // ---------------------- Type-specific validation ----------------------

  if (data.type == RS_SplineData::SplineType::ClampedOpen) {
    // Must have exactly degree+1 knots at start AND at end
    const double k_start = data.knotslist.front();
    const double k_end   = data.knotslist.back();

    size_t mult_start = 0;
    for (size_t j = 0; j < data.knotslist.size(); ++j) {
      if (std::abs(data.knotslist[j] - k_start) < RS_TOLERANCE)
        ++mult_start;
      else
        break;
    }

    size_t mult_end = 0;
    for (int j = static_cast<int>(data.knotslist.size()) - 1; j >= 0; --j) {
      if (std::abs(data.knotslist[j] - k_end) < RS_TOLERANCE)
        ++mult_end;
      else
        break;
    }

    if (mult_start != degree + 1 || mult_end != degree + 1)
      return false;
  }
  else if (data.type == RS_SplineData::SplineType::Standard) {
    // Standard (non-clamped) must NOT have repeated knots at the ends
    const double k_start = data.knotslist.front();
    const double k_end   = data.knotslist.back();

    if (std::abs(data.knotslist[1] - k_start) < RS_TOLERANCE)
      return false;     // mult ≥ 2 at start
    if (std::abs(data.knotslist[data.knotslist.size() - 2] - k_end) < RS_TOLERANCE)
      return false;
  }
  else if (data.type == RS_SplineData::SplineType::WrappedClosed) {
    // Must really be wrapped (control points + weights)
    if (!hasWrappedControlPoints()) return false;

           // Periodic splines normally have multiplicity = 1 everywhere
           // (allow up to degree to be tolerant, but disallow clamping-style degree+1 at the artificial ends)
    const double k_start = data.knotslist.front();
    const double k_end   = data.knotslist.back();

    if (std::abs(data.knotslist[1] - k_start) < RS_TOLERANCE)
      return false;
    if (std::abs(data.knotslist[data.knotslist.size() - 2] - k_end) < RS_TOLERANCE)
      return false;
  }

  return true;
}

/** Derivative approximation */
double RS_Spline::getDerivative(double t, bool isX) const {
  double d = 1e-8;
  RS_Vector p1 = getPointAt(t);
  RS_Vector p2 = getPointAt(t + d);
  return isX ? (p2.x - p1.x) / d : (p2.y - p1.y) / d;
}

/** Bisection for zero */
/** Robust bracketed root finding with bisection + safe midpoint (no overflow, early exact-zero exit) */
double RS_Spline::bisectDerivativeZero(double low, double high, double f_low, bool isX) const {
  double f_high = getDerivative(high, isX);

         // Ensure bracketing (caller guarantees sign change or near-zero, but be defensive)
  if (f_low * f_high > 0.0 && std::abs(f_low) >= RS_TOLERANCE && std::abs(f_high) >= RS_TOLERANCE)
    return low + (high - low) * 0.5; // no root → return midpoint

  while (high - low > RS_TOLERANCE * (1.0 + std::abs(low + high))) { // relative + absolute tolerance
    double mid = low + (high - low) * 0.5;
    double f_mid = getDerivative(mid, isX);

    if (f_mid == 0.0)
      return mid;                                 // exact zero → instant win

    if (std::signbit(f_low) != std::signbit(f_mid)) {
      high = mid;
    } else {
      low = mid;
      f_low = f_mid;
    }
  }

         // Return the endpoint with smaller |f| (best approximation when finite-difference noise exists)
  return std::abs(f_low) < std::abs(f_high) ? low : high;
}

void RS_Spline::resetBorders() {
  minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
  maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);
}

void RS_Spline::normalizeKnots() {
  data.knotslist = LC_SplineHelper::getNormalizedKnotVector(
      data.knotslist, data.knotslist.front(), {});
}

double RS_Spline::estimateParamAtIndex(size_t index) const {
  if (data.knotslist.empty())
    return 0.0;
  return data.knotslist[index + data.degree];
}

/** Knot span */
size_t RS_Spline::findSpan(size_t n, size_t p, double u, const std::vector<double> &U) {
  // Clamp to the valid parameter domain
  if (u >= U[n + 1])
    return n;
  if (u <= U[p])
    return p;

         // subtracting 1 gives the largest index i such that U[i] ≤ u < U[i+1]
         // preferring the right interval when u == knot
  auto it = std::upper_bound(U.cbegin(), U.cend(), u);
  return std::distance(U.cbegin(), it) - 1;
}

/** Basis functions */
std::vector<double> RS_Spline::basisFunctions(size_t i, double u, size_t p,
                                              const std::vector<double> &U) {
  std::vector<double> N(p + 1, 0.0);
  std::vector<double> left(p + 1, 0.0);
  std::vector<double> right(p + 1, 0.0);

  N[0] = 1.0;

  for (size_t j = 1; j <= p; ++j) {
    left[j]  = u - U[i + 1 - j];
    right[j] = U[i + j] - u;
    double saved = 0.0;

    for (size_t r = 0; r < j; ++r) {
      // alpha = barycentric-like weight for the current segment in de Boor recurrence
      double alpha = N[r] / (right[r + 1] + left[j - r]);

      N[r] = saved + right[r + 1] * alpha;
      saved = left[j - r] * alpha;
    }
    N[j] = saved;
  }
  return N;
}

/** Insert a single knot u (Boehm's algorithm for NURBS) - concise modern version */
void RS_Spline::insertKnot(double u) {
  if (!validate())
    return;

  bool const was_closed = isClosed();
  if (was_closed) setClosed(false);

  auto const& knots = data.knotslist;
  auto const& cp = data.controlPoints;
  auto& w = data.weights;

  if (w.size() != cp.size()) w.assign(cp.size(), 1.0);

  size_t const deg = data.degree;
  size_t const ncp = cp.size();
  if (ncp <= deg) return;

  size_t const n = ncp - 1; // last control index
  if (knots.size() != ncp + deg + 1) return;

         // Valid domain: full knot vector range (clamped uses repeats at ends, standard doesn't – both handled correctly)
  double const umin = knots.front();
  double const umax = knots.back();
  if (u <= umin + RS_TOLERANCE || u >= umax - RS_TOLERANCE) return;

  int const span = findSpan(static_cast<int>(n), static_cast<int>(deg), u, knots);

         // Compute multiplicity s at u
  int s = 0;
  for (int j = span; j >= 0; --j) {
    if (std::abs(knots[j] - u) > RS_TOLERANCE)
      break;
    ++s;
  }
  if (s >= static_cast<int>(deg))
    return; // multiplicity already ≥ deg → no effect

         // Homogeneous coordinates (rational handling)
  struct H {
    double x=0., y=0., w=0.;
    H operator*(double s) const { return {x*s, y*s, w*s}; }
    H operator+(H o) const { return {x + o.x, y + o.y, w + o.w}; }
  };

  std::vector<H> P(ncp);
  for (size_t i = 0; i < ncp; ++i) {
    double const wi = w[i];
    P[i] = {cp[i].x * wi, cp[i].y * wi, wi};
  }

  std::vector<H> Q(ncp + 1);

         // Left unchanged part: Q[0 … k-deg]
  std::copy(P.begin(), P.begin() + span - deg + 1, Q.begin());

         // Right shifted part: Q[k-s+1 … n+1] = P[k-s … n]
  size_t const right_src = span - s;
  std::copy(P.begin() + right_src, P.end(), Q.begin() + right_src + 1);

         // Blend affected part: k-s down to k-deg+1
  for (int i = span - s; i >= static_cast<int>(span - deg + 1); --i) {
    double const denom = knots[i + deg] - knots[i];
    double alpha = (denom > RS_TOLERANCE) ? (u - knots[i]) / denom : 0.5; // safe default
    Q[i] = P[i] * alpha + P[i - 1] * (1.0 - alpha);
  }

         // Insert the knot
  data.knotslist.insert(data.knotslist.begin() + span + 1, u);

         // Project back to Cartesian + weights
  data.controlPoints.resize(ncp + 1);
  data.weights.resize(ncp + 1);
  for (size_t i = 0; i < Q.size(); ++i) {
    double const wi = Q[i].w;
    data.weights[i] = (wi > 0.0) ? wi : 1.0;
    double const inv = (wi > 0.0) ? 1.0 / wi : 1.0;
    data.controlPoints[i] = RS_Vector(Q[i].x * inv, Q[i].y * inv);
  }

  if (was_closed)
    setClosed(true);

  calculateBorders();
  update();
}
