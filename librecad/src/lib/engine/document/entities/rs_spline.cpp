/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 librecad.org
** Copyright (C) 2025 Dongxu Li (github.com/dxli)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
* USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_spline.h"

#include <algorithm>
#include <iostream>

#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

namespace {
constexpr double g_knotTolerance = 5e-6;

    // fixme - sand - function is not used!
bool compareVector(const RS_Vector &va, const RS_Vector &vb, const double tol = RS_TOLERANCE) {
  return va.distanceTo(vb) <= tol;
}

/// Solves the dense linear system for spline interpolation (internal control
/// points). Wraps RS_Math::linearSolver with proper augmented matrix
/// construction. Returns true on success (unique solution), false if singular
/// or ill-conditioned.
bool solveSystem(const std::vector<std::vector<double>> &coef,
                 const std::vector<double> &rhs, std::vector<double> &sol) {
  const size_t n = coef.size();
  if (n == 0){
    return true; // trivial case, nothing to solve
  }

  std::vector<std::vector<double>> aug(n, std::vector<double>(n + 1));

  for (size_t i = 0; i < n; ++i) {
    std::copy(coef[i].begin(), coef[i].end(), aug[i].begin());
    aug[i].back() = rhs[i];
  }

  return RS_Math::linearSolver(aug, sol);
}
} // namespace

/** Constructor for RS_SplineData */
RS_SplineData::RS_SplineData(const int degree, const bool closed) : degree(degree) {
  type = closed ? SplineType::WrappedClosed : SplineType::ClampedOpen;
  savedOpenType = type;
}

/** Stream operator for RS_SplineData */
std::ostream &operator<<(std::ostream &os, const RS_SplineData &ld) {
  os << "(degree:" << ld.degree << " type:" << static_cast<int>(ld.type);
  if (!ld.controlPoints.empty()) {
      os << " cps:" << ld.controlPoints.size();
  }
  if (!ld.knotslist.empty()) {
      os << " knots:" << ld.knotslist.size();
  }
  if (!ld.weights.empty()) {
      os << " weights:" << ld.weights.size();
  }
  if (!ld.fitPoints.empty()) {
      os << " fit:" << ld.fitPoints.size();
  }
  os << ")";
  return os;
}

/** Constructor */
RS_Spline::RS_Spline(RS_EntityContainer *parent, const RS_SplineData &d)
    : RS_EntityContainer(parent), m_data(d) {
  if (m_data.type == RS_SplineData::SplineType::WrappedClosed &&
      !hasWrappedControlPoints()) {
    addWrapping();
  }
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

/** M_Data access */
RS_SplineData &RS_Spline::getData() { return m_data; }
const RS_SplineData &RS_Spline::getData() const { return m_data; }

/** Unwrapped size */
size_t RS_Spline::getUnwrappedSize() const {
  size_t s = m_data.controlPoints.size();
  if (s <= m_data.degree) {
      return s;
  }
  if (m_data.type == RS_SplineData::SplineType::WrappedClosed) {
    s -= (s > m_data.degree) ? m_data.degree : s;
  }
  return s;
}

/** Unwrapped vectors */
std::vector<RS_Vector> RS_Spline::getUnwrappedControlPoints() const {
  const size_t s = getUnwrappedSize();
  return s ? std::vector<RS_Vector>(m_data.controlPoints.begin(),
                                    m_data.controlPoints.begin() + s)
           : std::vector<RS_Vector>{};
}

std::vector<double> RS_Spline::getUnwrappedWeights() const {
  const size_t s = getUnwrappedSize();
  return s ? std::vector<double>(m_data.weights.begin(), m_data.weights.begin() + s)
           : std::vector<double>{};
}

std::vector<double> RS_Spline::getUnwrappedKnotVector() const {
  const size_t s = getUnwrappedSize();
  if (s == 0) {
      return {};
  }
  const size_t bs = s + m_data.degree + 1;
  return bs <= m_data.knotslist.size()
             ? std::vector<double>(m_data.knotslist.begin(),
                                   m_data.knotslist.begin() + bs)
             : std::vector<double>{};
}

/** Wrapping helpers */
void RS_Spline::removeWrapping() { LC_SplineHelper::removeWrapping(m_data); }

void RS_Spline::addWrapping() { LC_SplineHelper::addWrapping(m_data); }

void RS_Spline::updateControlAndWeightWrapping() {
  LC_SplineHelper::updateControlAndWeightWrapping(m_data, isClosed(),
                                                  getUnwrappedSize());
}

void RS_Spline::updateKnotWrapping() {
  LC_SplineHelper::updateKnotWrapping(m_data, isClosed(), getUnwrappedSize());
}

/** Borders */
void RS_Spline::calculateBorders() {
  resetBorders();
  const size_t s = getUnwrappedSize();
  if (s == 0) {
      return;
  }
  for (size_t i = 0; i < s; ++i) {
    m_minV = RS_Vector::minimum(m_data.controlPoints[i], m_minV);
    m_maxV = RS_Vector::maximum(m_data.controlPoints[i], m_maxV);
  }
}

/** Degree */
void RS_Spline::setDegree(const int degree) {
  if (degree < 1 || degree > 3) {
      throw std::invalid_argument("Degree must be 1-3");
  }
  m_data.degree = degree;
}
int RS_Spline::getDegree() const { return m_data.degree; }


/** Counts */
size_t RS_Spline::getNumberOfControlPoints() const {
  return getUnwrappedSize();
}
size_t RS_Spline::getNumberOfKnots() const { return m_data.knotslist.size(); }

/** Closed */
bool RS_Spline::isClosed() const {
  return m_data.type == RS_SplineData::SplineType::WrappedClosed;
}

/** Set closed */
void RS_Spline::setClosed(const bool c) {
  if (c == isClosed()) {
      return;
  }
  if (getUnwrappedSize() <= m_data.degree) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Spline::setClosed: insufficient points");
    m_data.type = c ? RS_SplineData::SplineType::WrappedClosed
                  : RS_SplineData::SplineType::ClampedOpen;
    return;
  }
  if (c) {
    if (m_data.type == RS_SplineData::SplineType::ClampedOpen) {
        LC_SplineHelper::toWrappedClosedFromClampedOpen(m_data);
    }
    else if (m_data.type == RS_SplineData::SplineType::Standard) {
        LC_SplineHelper::toWrappedClosedFromStandard(m_data);
    }
    else {
        assert(false && "unknown spline type for closing");
    }
  } else {
    if (m_data.type == RS_SplineData::SplineType::WrappedClosed) {
        LC_SplineHelper::toClampedOpenFromWrappedClosed(m_data);
    }
    else if (m_data.type == RS_SplineData::SplineType::Standard) {
        LC_SplineHelper::toClampedOpenFromStandard(m_data);
    }
    else {
        assert(false && "unknown spline type for closing");
    }
  }
  update();
}

/** Change type */
void RS_Spline::changeType(const RS_SplineData::SplineType newType) {
  if (m_data.type == newType) {
      return;
  }
  const RS_SplineData::SplineType oldType = m_data.type;
  if (m_data.controlPoints.size() < m_data.degree + 1) {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING,
                    "RS_Spline::changeType: insufficient points");
    m_data.type = newType;
    return;
  }

  if (newType == RS_SplineData::SplineType::Standard) {
    if (oldType == RS_SplineData::SplineType::ClampedOpen) {
        LC_SplineHelper::toStandardFromClampedOpen(m_data);
    }
    else if (oldType == RS_SplineData::SplineType::WrappedClosed) {
        LC_SplineHelper::toStandardFromWrappedClosed(m_data);
    }
  } else if (newType == RS_SplineData::SplineType::ClampedOpen) {
    if (oldType == RS_SplineData::SplineType::Standard) {
        LC_SplineHelper::toClampedOpenFromStandard(m_data);
    }
    else if (oldType == RS_SplineData::SplineType::WrappedClosed) {
        LC_SplineHelper::toClampedOpenFromWrappedClosed(m_data);
    }
  } else if (newType == RS_SplineData::SplineType::WrappedClosed) {
    if (oldType == RS_SplineData::SplineType::Standard) {
        LC_SplineHelper::toWrappedClosedFromStandard(m_data);
    }
    else if (oldType == RS_SplineData::SplineType::ClampedOpen) {
        LC_SplineHelper::toWrappedClosedFromClampedOpen(m_data);
    }
  }

  // even if LC_SplineHelper fails to change the type, still change the enum
  m_data.type = newType;
}

/** Clamped knot adjustment */
std::vector<double>
RS_Spline::adjustToOpenClamped(const std::vector<double> &knots, const size_t numControl, const size_t order, bool) const {
  return LC_SplineHelper::clampKnotVector(knots, numControl, order);
}

/** Reference points */
RS_VectorSolutions RS_Spline::getRefPoints() const {
  return RS_VectorSolutions(getControlPoints());
}
RS_Vector RS_Spline::doGetNearestRef(const RS_Vector &coord, double *dist) const {
  return RS_EntityContainer::doGetNearestRef(coord, dist);
}

RS_Vector RS_Spline::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_EntityContainer::doGetNearestSelectedRef(coord, dist);
}

/** Update approximation */
void RS_Spline::update() {
  clear();
  if (!validate()) {
      return;
  }
  std::vector<RS_Vector> points;
  fillStrokePoints(32, points);
  for (size_t i = 0; i + 1 < points.size(); ++i) {
      addEntity(new RS_Line(this, points[i], points[i + 1]));
  }
  if (isClosed() && points.size() > 1) {
      addEntity(new RS_Line(this, points.back(), points.front()));
  }
}

/** Stroke points */
void RS_Spline::fillStrokePoints(const int segments, std::vector<RS_Vector> &points) const {
  const auto &kv = m_data.knotslist;
  const double tmin = kv[m_data.degree];
  // Fixed: correct tmax that works for both open and wrapped-closed splines
  const double tmax = m_data.knotslist[m_data.knotslist.size() - m_data.degree - 1];
  const double step = (tmax - tmin) / segments;
  for (int i = 0; i <= segments; ++i) {
      points.push_back(getPointAt(tmin + i * step));
  }
}

/** Endpoints (invalid if closed) */
RS_Vector RS_Spline::getStartpoint() const { return RS_Vector(false); }
RS_Vector RS_Spline::getEndpoint() const { return RS_Vector(false); }

/** Nearest (invalid overrides) */
RS_Vector RS_Spline::doGetNearestEndpoint(const RS_Vector &, double *, RS_Entity** entity) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::doGetNearestCenter(const RS_Vector &, double *, RS_Entity** centerEntity) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::doGetNearestMiddle(const RS_Vector &, double *, int) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::doGetNearestDist(double, const RS_Vector &, double *) const {
  return RS_Vector(false);
}

/** Transformations */
void RS_Spline::move(const RS_Vector &offset) {
  for (auto &cp : m_data.controlPoints) {
      cp += offset;
  }
  for (auto &fp : m_data.fitPoints) {
      fp += offset;
  }
  calculateBorders();
}

void RS_Spline::rotate(const RS_Vector &center, const double angle) {
  for (auto &cp : m_data.controlPoints) {
      cp.rotate(center, angle);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.rotate(center, angle);
  }
  calculateBorders();
}

void RS_Spline::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  for (auto &cp : m_data.controlPoints) {
      cp.rotate(center, angleVector);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.rotate(center, angleVector);
  }
  calculateBorders();
}

void RS_Spline::scale(const RS_Vector &center, const RS_Vector &factor) {
  for (auto &cp : m_data.controlPoints) {
      cp.scale(center, factor);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.scale(center, factor);
  }
  calculateBorders();
}

RS_Entity &RS_Spline::shear(const double k) {
  for (auto &cp : m_data.controlPoints) {
      cp.shear(k);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.shear(k);
  }
  calculateBorders();
  return *this;
}

void RS_Spline::mirror(const RS_Vector &a1, const RS_Vector &a2) {
  for (auto &cp : m_data.controlPoints) {
      cp.mirror(a1, a2);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.mirror(a1, a2);
  }
  calculateBorders();
}

void RS_Spline::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
  RS_EntityContainer::moveRef(ref, offset);
}

/** Revert direction */
void RS_Spline::revertDirection() {
  std::reverse(m_data.controlPoints.begin(), m_data.controlPoints.end());
  std::reverse(m_data.weights.begin(), m_data.weights.end());
  std::reverse(m_data.knotslist.begin(), m_data.knotslist.end());
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
double RS_Spline::getWeight(const size_t i) const {
  return i < m_data.weights.size() ? m_data.weights[i] : 1.0;
}

/** Add control point */
void RS_Spline::addControlPoint(const RS_Vector &v, const double w) {
  const RS_SplineData::SplineType newType = m_data.type;
  changeType(RS_SplineData::SplineType::Standard);
  addControlPointRaw(v, w);
  const size_t n = m_data.controlPoints.size();
  if (n >= m_data.degree + 1) {
    if (m_data.knotslist.empty()) {
      m_data.knotslist =
          LC_SplineHelper::generateOpenUniformKnotVector(n, m_data.degree + 1);
    } else {
        LC_SplineHelper::extendKnotVector(m_data.knotslist);
    }
    LC_SplineHelper::ensureMonotonic(m_data.knotslist);
  }
  changeType(newType);
  calculateBorders();
  update();
}

void RS_Spline::addControlPointRaw(const RS_Vector &v, const double w) {
  m_data.controlPoints.push_back(v);
  m_data.weights.push_back(w);
}

/** Remove last */
void RS_Spline::removeLastControlPoint() {
  if (!m_data.controlPoints.empty()) {
    m_data.controlPoints.pop_back();
    m_data.weights.pop_back();
  }
  if (isClosed()) {
      updateControlAndWeightWrapping();
  }
  update();
}

/** Set weights/control/knots */
void RS_Spline::setWeight(const size_t i, const double w) {
  if (i < m_data.weights.size()) {
    m_data.weights[i] = w;
    if (isClosed()) {
        updateControlAndWeightWrapping();
    }
    // fixme - is it really necessary? That might be not good for bulk update, probably call update explicitly once after all modifications?
    // after all, this instance of entity will go to undoable and the clone will be added into the document - so probably update should be
    // called on clone later?
    update();
  }
}
void RS_Spline::setWeights(const std::vector<double> &w) {
  m_data.weights = w;
  if (isClosed()) {
      updateControlAndWeightWrapping();
  }
  update(); // fixme - is it really necessary? That might be not good for bulk update, probably call update explicitly once after all modifications?
}
void RS_Spline::setControlPoint(const size_t i, const RS_Vector &v) {
  if (i < m_data.controlPoints.size()) {
    m_data.controlPoints[i] = v;
    if (isClosed()) {
        updateControlAndWeightWrapping();
    }
    update();  // fixme - is it really necessary? That might be not good for bulk update, probably call update explicitly once after all modifications?
  }
}
void RS_Spline::setKnot(const size_t i, const double k) {
  if (i < m_data.knotslist.size()) {
    m_data.knotslist[i] = k;
    LC_SplineHelper::ensureMonotonic(m_data.knotslist);
    if (isClosed()) {
        updateKnotWrapping();
    }
    update();  // fixme - is it really necessary? That might be not good for bulk update, probably call update explicitly once after all modifications?
  }
}

/** Insert/remove control point */
void RS_Spline::insertControlPoint(const size_t i, const RS_Vector &v, const double w, const bool preserve) {
  m_data.controlPoints.insert(m_data.controlPoints.begin() + i, v);
  m_data.weights.insert(m_data.weights.begin() + i, w);
  if (!preserve) {
      m_data.knotslist.clear();
  }
  if (isClosed()) {
      updateControlAndWeightWrapping();
  }
  update();
}

void RS_Spline::removeControlPoint(const size_t i) {
  if (i < m_data.controlPoints.size()) {
    m_data.controlPoints.erase(m_data.controlPoints.begin() + i);
    m_data.weights.erase(m_data.weights.begin() + i);
    m_data.knotslist.clear();
  }
  if (isClosed()) {
      updateControlAndWeightWrapping();
  }
  update();
}

/** Knot vector access */
std::vector<double> RS_Spline::getKnotVector() const {
  return getUnwrappedKnotVector();
}
void RS_Spline::setKnotVector(const std::vector<double> &k) {
  m_data.knotslist = k;
  LC_SplineHelper::ensureMonotonic(m_data.knotslist);
  if (isClosed()) {
      updateKnotWrapping();
  }
  if (!validate()) {
      RS_DEBUG->print(RS_Debug::D_WARNING, "invalid knot vector");
  }
  update();
}

/** Knot generators */
std::vector<double> RS_Spline::knot(const size_t num, const size_t order) const {
  return LC_SplineHelper::knot(num, order);
}
std::vector<double> RS_Spline::openUniformKnot(const size_t num, const size_t order) const {
  return LC_SplineHelper::generateOpenUniformKnotVector(num, order);
}
std::vector<double> RS_Spline::knotu(const size_t num, const size_t order) const {
  return LC_SplineHelper::convertOpenToClosedKnotVector(
      openUniformKnot(num, order), num, order - 1);
}

/** Rational B-spline point generation */
void RS_Spline::rbspline(const size_t npts, const size_t k, const size_t numPoints,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
  const int nplusc = npts + k;
  std::vector<double> x(nplusc, 0.0);
  for (int i = 1; i < nplusc; ++i) {
      x[i] = (i >= static_cast<int>(k) && i <= static_cast<int>(npts))
                 ? x[i - 1] + 1.0
                 : x[i - 1];
  }
  double t = 0.0;
  double step = x.back() / numPoints;
  for (int idx = 1; idx <= static_cast<int>(numPoints); ++idx) {
    if (x.back() - t < g_knotTolerance) {
        t = x.back() - g_knotTolerance;
    }
    auto nb = rbasis(k, t, npts, x, h);
    RS_Vector pt(0.0, 0.0);
    for (int i = 0; i < static_cast<int>(npts); ++i) {
        pt += b[i] * nb[i];
    }
    p.push_back(pt);
    t += step;
  }
}

void RS_Spline::rbsplinu(const size_t npts, const size_t k, const size_t numPoints,
                         const std::vector<RS_Vector> &b,
                         const std::vector<double> &h,
                         std::vector<RS_Vector> &p) const {
  const int nplusc = npts + k;
  std::vector<double> x(nplusc, 0.0);
  for (int i = 1; i < nplusc; ++i) {
      x[i] = x[i - 1] + 1.0;
  }
  double t = k - 1, step = static_cast<double>(npts) / numPoints;
  for (int idx = 1; idx <= static_cast<int>(numPoints); ++idx) {
    if (x.back() - t < g_knotTolerance) {
        t = x.back() - g_knotTolerance;
    }
    auto nb = rbasis(k, t, npts, x, h);
    RS_Vector pt(0.0, 0.0);
    for (int i = 0; i < static_cast<int>(npts); ++i) {
        pt += b[i] * nb[i];
    }
    p.push_back(pt);
    t += step;
  }
}

/** Wrapped check */
bool RS_Spline::hasWrappedControlPoints() const {
  const size_t s = m_data.controlPoints.size();
  const size_t deg = m_data.degree;
  if (s <= deg) {
      return false;
  }
  for (size_t i = 0; i < deg; ++i) {
    if (m_data.controlPoints[s - deg + i] != m_data.controlPoints[i]) {
        return false;
    }
  }
  for (size_t i = 0; i < deg; ++i) {
    if (!RS_Math::equal(m_data.weights[s - deg + i], m_data.weights[i],
                        RS_TOLERANCE)) {
        return false;
    }
  }
  return true;
}

/** Output */
std::ostream &operator<<(std::ostream &os, const RS_Spline &l) {
  os << "RS_Spline: " << l.getData();
  return os;
}

/** Derivative zeros */
std::vector<double> RS_Spline::findDerivativeZeros(bool isX) const {
  std::vector<double> zeros;
  const auto &U = m_data.knotslist;
  const size_t p = m_data.degree;
  const size_t n = m_data.controlPoints.size() - 1;
  if (n < p) {
      return zeros;
  }

  auto d = [this, isX](const double t) { return getDerivative(t, isX); };

  auto funAddIf = [&](const double a, const double b, const double fa, const double fb) {
    if ((fa * fb <= 0.0 || std::abs(fa) < 1e-9 || std::abs(fb) < 1e-9) &&
        b - a > 1e-12) {
        zeros.push_back(bisectDerivativeZero(a, b, fa, isX));
    }
  };

  double f0 = d(U[p]);
  for (size_t i = p; i <= n; ++i) {
    const double t1 = U[i + 1];
    const double fm = d((U[i] + t1) * 0.5);
    const double f1 = d(t1);

    funAddIf(U[i], t1, f0, fm); // left half (f0 reused from previous)
    funAddIf(t1, t1, fm, f1);   // right half

    f0 = f1; // chain for next span
  }

  // endpoints if derivative ≈ 0
  if (std::abs(d(U[p])) < 1e-9) {
      zeros.push_back(U[p]);
  }
  if (std::abs(d(U[n + 1])) < 1e-9) {
      zeros.push_back(U[n + 1]);
  }

  std::sort(zeros.begin(), zeros.end());
  zeros.erase(
      std::unique(zeros.begin(), zeros.end(),
                  [](const double a, const double b) { return std::abs(a - b) < 1e-8; }),
      zeros.end());

  return zeros;
}

/** Tight borders */
void RS_Spline::calculateTightBorders() {
  resetBorders();
  if (!isClosed()) {
    m_minV = RS_Vector::minimum(getStartpoint(), m_minV);
    m_maxV = RS_Vector::maximum(getStartpoint(), m_maxV);
    m_minV = RS_Vector::minimum(getEndpoint(), m_minV);
    m_maxV = RS_Vector::maximum(getEndpoint(), m_maxV);
  }
  for (const double t : findDerivativeZeros(true)) {
    RS_Vector pt = getPointAt(t);
    m_minV = RS_Vector::minimum(pt, m_minV);
    m_maxV = RS_Vector::maximum(pt, m_maxV);
  }
  for (const double t : findDerivativeZeros(false)) {
    RS_Vector pt = getPointAt(t);
    m_minV = RS_Vector::minimum(pt, m_minV);
    m_maxV = RS_Vector::maximum(pt, m_maxV);
  }
}

/** NURBS evaluation */
RS_Vector RS_Spline::evaluateNURBS(const RS_SplineData &d, const double t) {
  const size_t p = d.degree;
  const size_t n = d.controlPoints.size() - 1;
  if (n < p) {
      return RS_Vector(false);
  }
  const size_t span = findSpan(n, p, t, d.knotslist);
  const auto basis = basisFunctions(span, t, p, d.knotslist);
  RS_Vector pt(0.0, 0.0);
  double wsum = 0.0;
  for (size_t i = 0; i <= p; ++i) {
    const size_t idx = span - p + i;
    const double w = d.weights[idx];
    pt += d.controlPoints[idx] * basis[i] * w;
    wsum += basis[i] * w;
  }
  return wsum > RS_TOLERANCE ? pt / wsum : pt;
}

/** rbasis */
std::vector<double> RS_Spline::rbasis(const int c, const double t, const int npts,
                                      const std::vector<double> &x,
                                      const std::vector<double> &h) {
  const int nplusc = npts + c;
  std::vector<double> alpha(nplusc, 0.0);
  for (int i = 0; i < nplusc - 1; ++i) {
      if (t >= x[i] && t < x[i + 1]) {
          alpha[i] = 1.0;
      }
  }
  for (int k = 2; k <= c; ++k) {
    for (int i = 0; i < nplusc - k; ++i) {
      if (alpha[i] != 0.0) {
          alpha[i] = ((t - x[i]) * alpha[i]) / (x[i + k - 1] - x[i]);
      }
      if (alpha[i + 1] != 0.0) {
          alpha[i] += ((x[i + k] - t) * alpha[i + 1]) / (x[i + k] - x[i + 1]);
      }
    }
  }
  if (t >= x[nplusc - 1]) {
      alpha[npts - 1] = 1.0;
  }
  double sum = 0.0;
  for (int i = 0; i < npts; ++i) {
      sum += alpha[i] * h[i];
  }
  std::vector<double> r(npts, 0.0);
  if (sum != 0.0) {
      for (int i = 0; i < npts; ++i) {
          r[i] = alpha[i] * h[i] / sum;
      }
  }
  return r;
}

/** Fit points interpolation */
// Updated setFitPoints (now uses the file-local solveSystem)

void RS_Spline::setFitPoints(const std::vector<RS_Vector> &fitPoints, const bool useCentripetal /*= true*/) {
  m_data.fitPoints = fitPoints;
  if (m_data.fitPoints.size() < 2) {
    update();
    return;
  }

  const bool closed = (m_data.fitPoints.front().distanceTo(m_data.fitPoints.back()) <
                 RS_TOLERANCE) &&
                m_data.fitPoints.size() > 2;
  std::vector<RS_Vector> fp = m_data.fitPoints;
  if (closed) {
      fp.pop_back();
  }

  // interpolate as open, close afterwards

  const size_t num = fp.size();
  const int p = m_data.degree; //= std::clamp(m_data.degree, 1, 3);
  assert(p >= 1 && p <= 3);
  const size_t n = num - 1;

  // Enhanced centripetal + second-order term → virtually oscillation-free
  std::vector<double> u(num, 0.0);
  double total = 0.0;
  for (size_t i = 1; i < num; ++i) {
    const double d = fp[i].distanceTo(fp[i - 1]);
    double term = useCentripetal ? std::sqrt(d) : d;

    if (useCentripetal && i >= 2) {
      const double sd = ((fp[i] - fp[i - 1]) - (fp[i - 1] - fp[i - 2])).magnitude();
      if (sd > RS_TOLERANCE) {
          term += 2.9 * std::sqrt(sd);
      }
    }

    total += term;
    u[i] = total;
  }

  if (total < RS_TOLERANCE) { // all points coincide
    m_data.controlPoints = {fp[0]};
    m_data.weights = {1.0};
    m_data.knotslist = {0, 0, 1, 1};
    update();
    return;
  }
  for (size_t i = 1; i < num; ++i) {
      u[i] /= total;
  }

  // Clamped averaged knot vector
  std::vector<double> U(n + p + 2, 0.0);
  for (int i = 0; i <= p; ++i) {
      U[i] = 0.0, U[n + 1 + i] = 1.0;
  }
  for (int j = 1; j <= static_cast<int>(n - p); ++j) {
    double s = 0.0;
    for (int k = 0; k < p; ++k) {
        s += u[j + k];
    }
    U[p + j] = s / p;
  }
  m_data.knotslist = std::move(U);

  // Fixed end control points
  m_data.controlPoints.assign(num, RS_Vector(false));
  m_data.weights.assign(num, 1.0);
  m_data.controlPoints[0] = fp.front();
  m_data.controlPoints[n] = fp.back();

  if (num <= static_cast<size_t>(p + 1)) {
    if (closed) {
        setClosed(true);
    }
    calculateBorders();
    update();
    return;
  }

  // Build system for internal control points
  const size_t sys = num - 2;
  std::vector<std::vector<double>> A(sys, std::vector<double>(sys, 0.0));
  std::vector<double> bx(sys), by(sys);

  for (size_t i = 1; i < num - 1; ++i) {
    const int span = findSpan(static_cast<int>(n), p, u[i], m_data.knotslist);
    auto bf = basisFunctions(span, u[i], p, m_data.knotslist);

    const size_t row = i - 1;
    bx[row] = fp[i].x;
    by[row] = fp[i].y;

    const int first = span - p;
    for (int j = 0; j <= p; ++j) {
      const int idx = first + j;
      const double v = bf[j];
      if (v == 0.0) {
          continue;
      }

      if (idx == 0) {
        bx[row] -= v * fp[0].x;
        by[row] -= v * fp[0].y;
      } else if (idx == static_cast<int>(n)) {
        bx[row] -= v * fp.back().x;
        by[row] -= v * fp.back().y;
      } else {
          A[row][idx - 1] = v;
      }
    }
  }

  // Solve using file-local helper
  std::vector<double> px(sys), py(sys);
  const bool ok = solveSystem(A, bx, px) && solveSystem(A, by, py);

  if (ok) {
    for (size_t i = 0; i < sys; ++i) {
        m_data.controlPoints[i + 1] = RS_Vector(px[i], py[i]);
    }
  } else {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Spline::setFitPoints: singular interpolation system");
  }

  m_data.type = RS_SplineData::SplineType::ClampedOpen;
  if (closed) {
      setClosed(true);
  }

  calculateBorders();
  update();
}

/** B-spline basis */
std::vector<double> RS_Spline::getBSplineBasis(const double t,
                                               const std::vector<double> &knots, const int degree, const size_t numControls) const {
  int order = degree + 1, np = static_cast<int>(numControls), c = order,
      nplusc = np + c;
  std::vector<double> bf(nplusc, 0.0);
  for (int i = 0; i < nplusc - 1; ++i) {
      if (t >= knots[i] - RS_TOLERANCE && t < knots[i + 1] + RS_TOLERANCE) {
          bf[i] = 1.0;
      }
  }
  for (int k = 2; k <= c; ++k) {
    for (int i = 0; i < nplusc - k; ++i) {
      const double d1 =
          bf[i] != 0.0 && std::abs(knots[i + k - 1] - knots[i]) > RS_TOLERANCE
              ? (t - knots[i]) * bf[i] / (knots[i + k - 1] - knots[i])
              : 0.0;
      const double d2 =
          bf[i + 1] != 0.0 &&
                  std::abs(knots[i + k] - knots[i + 1]) > RS_TOLERANCE
              ? (knots[i + k] - t) * bf[i + 1] / (knots[i + k] - knots[i + 1])
              : 0.0;
      bf[i] = d1 + d2;
    }
  }
  if (t + RS_TOLERANCE >= knots[nplusc - 1]) {
      bf[np - 1] = 1.0;
  }
  return {bf.begin(), bf.begin() + np};
}

/** Validate */
// rs_spline.cpp - only the changed part of validate() function (ClampedOpen
// block)

// rs_spline.cpp - only the changed validate() function (full function shown for
// clarity)

bool RS_Spline::validate() const {
  const size_t degree = m_data.degree;

  // Basic checks
  const size_t numControlPoints = m_data.controlPoints.size(); // total (wrapped if closed)
  if (numControlPoints < degree + 1) {
      return false;
  }

  const bool closed = isClosed(); // WrappedClosed
  const size_t extraControlPoints = closed ? degree : 0;
  const size_t unwrapped_cp = numControlPoints - extraControlPoints;

  if (unwrapped_cp < degree + 1) {
      return false;
  }

  const size_t expectedKnotsCount = numControlPoints + degree + 1; // works for open and wrapped closed
  if (m_data.knotslist.size() != expectedKnotsCount) {
      return false;
  }
  if (m_data.weights.size() != numControlPoints) {
      return false;
  }

  // Weights must be positive
  for (const double w : m_data.weights) {
      if (w <= 0.0) {
          return false;
      }
  }

  // Knots must be non-decreasing
  for (size_t i = 1; i < m_data.knotslist.size(); ++i) {
      if (m_data.knotslist[i] < m_data.knotslist[i - 1] - RS_TOLERANCE) {
          return false;
      }
  }

  // Maximum multiplicity anywhere ≤ degree+1
  {
    size_t mult = 1;
    for (size_t i = 1; i < m_data.knotslist.size(); ++i) {
      if (fabs(m_data.knotslist[i] - m_data.knotslist[i - 1]) < RS_TOLERANCE) {
        ++mult;
        if (mult > degree + 1) {
            return false;
        }
      } else {
        mult = 1;
      }
    }
  }

  // ---------------------- Type-specific validation ----------------------
  if (m_data.type == RS_SplineData::SplineType::ClampedOpen) {
    // Must have exactly degree+1 knots at start AND at end
    const double k_start = m_data.knotslist.front();
    const double k_end = m_data.knotslist.back();

    size_t mult_start = 0;
    for (size_t j = 0; j < m_data.knotslist.size(); ++j) {
      if (fabs(m_data.knotslist[j] - k_start) < RS_TOLERANCE) {
          ++mult_start;
      }
      else {
          break;
      }
    }

    size_t mult_end = 0;
    for (int j = static_cast<int>(m_data.knotslist.size()) - 1; j >= 0; --j) {
      if (fabs(m_data.knotslist[j] - k_end) < RS_TOLERANCE) {
          ++mult_end;
      }
      else {
          break;
      }
    }

    if (mult_start != degree + 1 || mult_end != degree + 1) {
        return false;
    }
  } else if (m_data.type == RS_SplineData::SplineType::Standard) {
    // Standard (non-clamped) must NOT have repeated knots at the ends
    const double k_start = m_data.knotslist.front();
    const double k_end = m_data.knotslist.back();

    if (fabs(m_data.knotslist[1] - k_start) < RS_TOLERANCE) {
        return false; // mult ≥ 2 at start
    }
    if (fabs(m_data.knotslist[m_data.knotslist.size() - 2] - k_end) < RS_TOLERANCE) {
        return false;
    }
  } else if (m_data.type == RS_SplineData::SplineType::WrappedClosed) {
    // Must really be wrapped (control points + weights)
    if (!hasWrappedControlPoints()) {
        return false;
    }

    // Periodic splines normally have multiplicity = 1 everywhere
    // (allow up to degree to be tolerant, but disallow clamping-style degree+1
    // at the artificial ends)
    const double k_start = m_data.knotslist.front();
    const double k_end = m_data.knotslist.back();

    if (std::abs(m_data.knotslist[1] - k_start) < RS_TOLERANCE) {
        return false;
    }
    if (std::abs(m_data.knotslist[m_data.knotslist.size() - 2] - k_end) <
        RS_TOLERANCE) {
        return false;
    }
  }

  return true;
}

RS_Vector RS_Spline::getPointAt(const double t) const {
  return evaluateWithDerivs(t).pos;
}

double RS_Spline::getDerivative(const double t, const bool isX) const {
  const auto d = evaluateWithDerivs(t);
  return isX ? d.der1.x : d.der1.y;
}

double RS_Spline::getSecondDerivative(const double t, const bool isX) const {
  const auto d = evaluateWithDerivs(t);
  return isX ? d.der2.x : d.der2.y;
}

double RS_Spline::getCurvature(const double t) const {
  const auto d = evaluateWithDerivs(t);
  double vx = d.der1.x, vy = d.der1.y;
  double ax = d.der2.x, ay = d.der2.y;
  const double speed2 = vx * vx + vy * vy;
  if (speed2 < 1e-20) {
      return 0.0;
  }
  return std::abs(vx * ay - vy * ax) / std::pow(speed2, 1.5);
}

double RS_Spline::getSignedCurvature(const double t) const {
  const auto d = evaluateWithDerivs(t);
  double vx = d.der1.x, vy = d.der1.y;
  double ax = d.der2.x, ay = d.der2.y;
  const double speed = std::hypot(vx, vy);
  if (speed < 1e-10) {
      return 0.0;
  }
  return (vx * ay - vy * ax) / (speed * speed * speed);
}

RS_Spline::SplineDerivs RS_Spline::evaluateWithDerivs(double t) const {
  SplineDerivs res{};
  size_t p = m_data.degree;
  if (p == 0 || m_data.controlPoints.empty()) {
      return res;
  }

  const auto &U = m_data.knotslist;
  const auto &P = m_data.controlPoints;
  const auto &W = m_data.weights;
  int ncp = static_cast<int>(P.size());
  int span = findSpan(ncp - 1, static_cast<int>(p), t, U);

  double ndu[4][4] = {};
  double left[4] = {};
  double right[4] = {};

  ndu[0][0] = 1.0;

  for (int j = 1; j <= static_cast<int>(p); ++j) {
    left[j] = t - U[span + 1 - j];
    right[j] = U[span + j] - t;
    double saved = 0.0;
    for (int r = 0; r < j; ++r) {
      double den = right[r + 1] + left[j - r];
      double tmp = ndu[r][j - 1] / den;
      ndu[r][j] = saved + right[r + 1] * tmp;
      saved = left[j - r] * tmp;
    }
    ndu[j][j] = saved;
  }

  double N0[4], N1[4], N2[4];
  for (int j = 0; j <= static_cast<int>(p); ++j) {
      N0[j] = ndu[j][p];
  }

  // Unrolled DersBasisFuns for order 1 & 2 only
  double a0[4], a1[4];
  for (int r = 0; r <= static_cast<int>(p); ++r) {
    a0[0] = 1.0;

    // k=1
    double d1 = 0.0;
    int rk = r - 1;
    int pk = p - 1;
    if (r >= 1) {
      a1[0] = a0[0] / ndu[pk + 1][rk];
      d1 = a1[0] * ndu[rk][pk];
    }
    int j1 = rk >= -1 ? 1 : -rk;
    int j2 = r - 1 <= pk ? 0 : p - r;
    for (int j = j1; j <= j2; ++j) {
      a1[j] = (a0[j] - a0[j - 1]) / ndu[pk + 1][rk + j];
      d1 += a1[j] * ndu[rk + j][pk];
    }
    if (r <= pk) {
      a1[1] = -a0[0] / ndu[pk + 1][r];
      d1 += a1[1] * ndu[r][pk];
    }
    N1[r] = d1 * p;

    // k=2 (only if p >= 2)
    if (p < 2) {
      N2[r] = 0.0;
      continue;
    }
    double d2 = 0.0;
    rk = r - 2;
    pk = p - 2;
    if (r >= 2) {
      a0[0] = a1[0] / ndu[pk + 1][rk];
      d2 = a0[0] * ndu[rk][pk];
    }
    j1 = rk >= -1 ? 1 : -rk;
    j2 = r - 1 <= pk ? 1 : p - r;
    for (int j = j1; j <= j2; ++j) {
      a0[j] = (a1[j] - a1[j - 1]) / ndu[pk + 1][rk + j];
      d2 += a0[j] * ndu[rk + j][pk];
    }
    if (r <= pk) {
      a0[2] = -a1[1] / ndu[pk + 1][r];
      d2 += a0[2] * ndu[r][pk];
    }
    N2[r] = d2 * p * (p - 1);
  }

  double w = 0, wd = 0, wdd = 0;
  double cx = 0, cy = 0, cxd = 0, cyd = 0, cxdd = 0, cydd = 0;

  for (int j = 0; j <= static_cast<int>(p); ++j) {
    size_t i = span - p + j;
    double wi = W[i];
    double wx = wi * P[i].x;
    double wy = wi * P[i].y;

    w += wi * N0[j];
    wd += wi * N1[j];
    wdd += wi * N2[j];
    cx += wx * N0[j];
    cy += wy * N0[j];
    cxd += wx * N1[j];
    cyd += wy * N1[j];
    cxdd += wx * N2[j];
    cydd += wy * N2[j];
  }

  if (w < 1e-12) {
      return res;
  }

  double w2 = w * w, w3 = w2 * w;

  res.pos = RS_Vector(cx / w, cy / w);
  res.der1 = RS_Vector((cxd * w - cx * wd) / w2, (cyd * w - cy * wd) / w2);
  res.der2 = RS_Vector((cxdd * w - 2 * wd * cxd + wdd * cx) / w3,
                       (cydd * w - 2 * wd * cyd + wdd * cy) / w3);

  return res;
}

/** Bisection for zero */
/** Robust bracketed root finding with bisection + safe midpoint (no overflow,
 * early exact-zero exit) */
double RS_Spline::bisectDerivativeZero(double low, double high, double f_low, const bool isX) const {
  const double f_high = getDerivative(high, isX);

  // Ensure bracketing (caller guarantees sign change or near-zero, but be
  // defensive)
  if (f_low * f_high > 0.0 && std::abs(f_low) >= RS_TOLERANCE &&
      std::abs(f_high) >= RS_TOLERANCE) {
      return low + (high - low) * 0.5; // no root → return midpoint
  }

  while (high - low >
         RS_TOLERANCE *
             (1.0 + std::abs(low + high))) { // relative + absolute tolerance
    const double mid = low + (high - low) * 0.5;
    const double f_mid = getDerivative(mid, isX);

    if (f_mid == 0.0) {
        return mid; // exact zero → instant win
    }

    if (std::signbit(f_low) != std::signbit(f_mid)) {
      high = mid;
    } else {
      low = mid;
      f_low = f_mid;
    }
  }

  // Return the endpoint with smaller |f| (best approximation when
  // finite-difference noise exists)
  return std::abs(f_low) < std::abs(f_high) ? low : high;
}

void RS_Spline::normalizeKnots() {
  m_data.knotslist = LC_SplineHelper::getNormalizedKnotVector(
      m_data.knotslist, m_data.knotslist.front(), {});
}

double RS_Spline::estimateParamAtIndex(const size_t index) const {
  if (m_data.knotslist.empty()) {
      return 0.0;
  }
  return m_data.knotslist[index + m_data.degree];
}

/** Knot span */
size_t RS_Spline::findSpan(const size_t n, const size_t p, const double u,
                           const std::vector<double> &U) {
  // Clamp to the valid parameter domain
  if (u >= U[n + 1]) {
      return n;
  }
  if (u <= U[p]) {
      return p;
  }

  // subtracting 1 gives the largest index i such that U[i] ≤ u < U[i+1]
  // preferring the right interval when u == knot
  const auto it = std::upper_bound(U.cbegin(), U.cend(), u);
  return std::distance(U.cbegin(), it) - 1;
}

/**
 * Compute non-zero B-spline basis functions N_{i-p,p} .. N_{i,p} at parameter u
 * using the stable de Boor / Cox–de Boor recurrence (Piegl & Tiller Algorithm
 * A2.2).
 *
 * This is the standard non-recursive implementation that avoids numerical
 * issues with the recursive formulation when p is large or knots are close.
 *
 * @param i  knot span index (from findSpan)
 * @param u parameter value
 * @param p degree
 * @param U knot vector
 * @return vector of size p+1 where return[j] = N_{i-p+j, p}(u)
 */
std::vector<double> RS_Spline::basisFunctions(const size_t i, const double u, const size_t p,
                                              const std::vector<double> &U) {
  std::vector<double> N(p + 1, 0.0);
  std::vector<double> left(p + 1, 0.0);
  std::vector<double> right(p + 1, 0.0);

  N[0] = 1.0;

  for (size_t j = 1; j <= p; ++j) {
    left[j] = u - U[i + 1 - j];
    right[j] = U[i + j] - u;
    double saved = 0.0;

    for (size_t r = 0; r < j; ++r) {
      // alpha = barycentric weight for current segment
      const double alpha = N[r] / (right[r + 1] + left[j - r]);

      N[r] = saved + right[r + 1] * alpha;
      saved = left[j - r] * alpha;
    }
    // highest basis function for this j
    N[j] = saved;
  }

  // N[0] = N_{i-p,p}, ..., N[p] = N_{i,p}
  return N;
}

/** Insert a single knot u (Boehm's algorithm for NURBS) - concise modern
 * version */
/** Insert knot u – Boehm's algorithm (rational, open/closed) */
void RS_Spline::insertKnot(const double u) {
  if (!validate()) {
      return;
  }

  bool wasClosed = isClosed();

  // Temporarily convert closed splines to open form – safest for knot/control
  // consistency
  if (wasClosed) {
      setClosed(false);
  }

  // RAII style to restore closed splines
  std::shared_ptr<bool> raiiPtr{&wasClosed, [wasClosed, this](const bool *pointer) {
                                  if (pointer != nullptr) {
                                      setClosed(wasClosed);
                                  }
  }};

  const std::vector<double> &K = m_data.knotslist;
  const std::vector<RS_Vector> &P = m_data.controlPoints;
  std::vector<double> &W = m_data.weights;
  if (W.size() != P.size()) {
      W.assign(P.size(), 1.0);
  }

  size_t p = m_data.degree, n = P.size();
  if (n <= p + 1) {
      return;
  }

  // Valid interior parameter range only – endpoint insertion has no effect
  double umin = K[p], umax = K[n];
  if (u <= umin + RS_TOLERANCE || u >= umax - RS_TOLERANCE) {
      return;
  }

  // Find span and existing multiplicity s of u
  const size_t k = findSpan(n - 1, p, u, K);

  size_t s = 0;
  for (int j = k; j >= 0 && std::abs(K[j] - u) <= RS_TOLERANCE; --j) {
      ++s;
  }
  if (s >= p) {
      return;
  }

  // Homogeneous coordinates (handles rational case correctly)
  struct HW {
    double x, y, w;
    HW operator*(const double s) const { return {x * s, y * s, w * s}; }
    HW operator+(const HW o) const { return {x + o.x, y + o.y, w + o.w}; }
  };

  std::vector<HW> Pw(n), Qw(n + 1);
  for (size_t i = 0; i < n; ++i) {
      Pw[i] = {P[i].x * W[i], P[i].y * W[i], W[i]};
  }

  // Copy unchanged parts
  std::copy(Pw.begin(), Pw.begin() + k - p + 1, Qw.begin());
  std::copy(Pw.begin() + k - s, Pw.end(), Qw.begin() + k - s + 1);

  // Boehm's local modification for affected control points
  for (size_t i = k - s; i >= k - p + 1; --i) {
    const double alpha =
        (K[i + p] - K[i] > RS_TOLERANCE) ? (u - K[i]) / (K[i + p] - K[i]) : 0.5;
    Qw[i] = Pw[i] * alpha + Pw[i - 1] * (1 - alpha);
  }

  // Insert the new knot
  m_data.knotslist.insert(m_data.knotslist.begin() + k + 1, u);

  // Project back from homogeneous to Cartesian coordinates
  m_data.controlPoints.resize(n + 1);
  m_data.weights.resize(n + 1);
  for (size_t i = 0; i <= n; ++i) {
    const double iw = Qw[i].w > RS_TOLERANCE ? 1.0 / Qw[i].w : 1.0;
    m_data.controlPoints[i] = RS_Vector(Qw[i].x * iw, Qw[i].y * iw);
    m_data.weights[i] = Qw[i].w;
  }

  calculateBorders();
  update();
}
