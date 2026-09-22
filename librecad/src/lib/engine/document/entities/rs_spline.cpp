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
#include <cmath>
#include <iostream>
#include <limits>

#include "lc_curveoffset.h"
#include "lc_splinehelper.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

namespace {
constexpr double g_knotTolerance = 5e-6;

// update() draws a spline with at least this many segments, more where it bends
// by more than the relative tolerance, and never more than the cap.
constexpr int g_minimumDisplaySegments = 32;
constexpr size_t g_maximumDisplaySegments = 4096;
constexpr double g_displayRelativeTolerance = 1e-3;

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

constexpr size_t g_maxDegree = 3;

/**
 * Whether the degree, control points, weights and knot vector have the sizes a
 * curve needs. Values are checked separately, where they are used.
 */
bool hasEvaluableLayout(const RS_SplineData &d) {
  const size_t p = d.degree;
  const size_t ncp = d.controlPoints.size();
  return p >= 1 && p <= g_maxDegree && ncp >= p + 1 &&
         d.knotslist.size() == ncp + p + 1 && d.weights.size() == ncp;
}

/**
 * The non-zero B-spline basis functions of degree p on knot span s, and their
 * first and second derivatives, at t (Piegl & Tiller, The NURBS Book, A2.3).
 * ders[k][j] is the k-th derivative of N_{s-p+j,p}(t).
 *
 * The knot differences are kept in the lower triangle of ndu, because the
 * derivative recurrence divides by them. On a span with U[s] < U[s+1] every one
 * of them spans that interval and so is positive.
 *
 * @return false if a knot difference is not positive or a value is not finite.
 */
bool dersBasisFunctions(const size_t s, const double t, const size_t p,
                        const std::vector<double> &U, double ders[3][g_maxDegree + 1]) {
  double ndu[g_maxDegree + 1][g_maxDegree + 1] = {};
  double left[g_maxDegree + 1] = {};
  double right[g_maxDegree + 1] = {};
  ndu[0][0] = 1.0;
  for (size_t j = 1; j <= p; ++j) {
    left[j] = t - U[s + 1 - j];
    right[j] = U[s + j] - t;
    double saved = 0.0;
    for (size_t r = 0; r < j; ++r) {
      ndu[j][r] = right[r + 1] + left[j - r];
      if (!(ndu[j][r] > 0.0)) {
        return false;
      }
      const double temp = ndu[r][j - 1] / ndu[j][r];
      ndu[r][j] = saved + (right[r + 1] * temp);
      saved = left[j - r] * temp;
    }
    ndu[j][j] = saved;
  }

  for (size_t j = 0; j <= p; ++j) {
    ders[0][j] = ndu[j][p];
    ders[1][j] = 0.0;
    ders[2][j] = 0.0;
  }

  // derivatives above the degree vanish
  const int nd = static_cast<int>(std::min<size_t>(2, p));
  const int ip = static_cast<int>(p);
  double a[2][g_maxDegree + 1] = {};
  for (int r = 0; r <= ip; ++r) {
    int s1 = 0;
    int s2 = 1;
    a[0][0] = 1.0;
    for (int k = 1; k <= nd; ++k) {
      double d = 0.0;
      const int rk = r - k;
      const int pk = ip - k;
      if (r >= k) {
        a[s2][0] = a[s1][0] / ndu[pk + 1][rk];
        d = a[s2][0] * ndu[rk][pk];
      }
      const int j1 = (rk >= -1) ? 1 : -rk;
      const int j2 = (r - 1 <= pk) ? k - 1 : ip - r;
      for (int j = j1; j <= j2; ++j) {
        a[s2][j] = (a[s1][j] - a[s1][j - 1]) / ndu[pk + 1][rk + j];
        d += a[s2][j] * ndu[rk + j][pk];
      }
      if (r <= pk) {
        a[s2][k] = -a[s1][k - 1] / ndu[pk + 1][r];
        d += a[s2][k] * ndu[r][pk];
      }
      ders[k][r] = d;
      std::swap(s1, s2);
    }
  }

  double factor = static_cast<double>(p);
  for (int k = 1; k <= nd; ++k) {
    for (size_t j = 0; j <= p; ++j) {
      ders[k][j] *= factor;
    }
    factor *= static_cast<double>(ip - k);
  }

  for (int k = 0; k <= 2; ++k) {
    for (size_t j = 0; j <= p; ++j) {
      if (!std::isfinite(ders[k][j])) {
        return false;
      }
    }
  }
  return true;
}

bool isFinite(const RS_Vector &v) {
  return v.valid && std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

/**
 * Arc length of the curve from the start of its domain, tabulated at knots and
 * at subdivisions of each knot span (5-point Gauss-Legendre per piece).
 */
struct ArcLengthTable {
  std::vector<double> t;
  std::vector<double> length;
};

double speedAt(const RS_Spline &spline, const double t) {
  LC_CurveJet jet;
  return spline.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet) ? jet.first.magnitude()
                                                                          : std::numeric_limits<double>::quiet_NaN();
}

/** Arc length over [a, b] inside one knot span. */
double pieceLength(const RS_Spline &spline, const double a, const double b) {
  static constexpr double nodes[] = {-0.9061798459386640, -0.5384693101056831, 0.0, 0.5384693101056831,
                                     0.9061798459386640};
  static constexpr double weights[] = {0.2369268850561891, 0.4786286704993665, 0.5688888888888889,
                                       0.4786286704993665, 0.2369268850561891};
  const double half = 0.5 * (b - a);
  const double middle = 0.5 * (a + b);
  double sum = 0.0;
  for (int i = 0; i < 5; ++i) {
    sum += weights[i] * speedAt(spline, middle + half * nodes[i]);
  }
  return sum * half;
}

bool buildArcLengthTable(const RS_Spline &spline, ArcLengthTable &table) {
  const std::vector<double> breaks = spline.getBreakParameters();
  if (breaks.size() < 2) {
    return false;
  }
  constexpr int piecesPerSpan = 8;
  table.t = {breaks.front()};
  table.length = {0.0};
  for (size_t i = 0; i + 1 < breaks.size(); ++i) {
    for (int k = 1; k <= piecesPerSpan; ++k) {
      const double a = table.t.back();
      const double b = (k == piecesPerSpan) ? breaks[i + 1] : breaks[i] + (breaks[i + 1] - breaks[i]) * k / piecesPerSpan;
      const double length = pieceLength(spline, a, b);
      if (!std::isfinite(length)) {
        return false;
      }
      table.t.push_back(b);
      table.length.push_back(table.length.back() + length);
    }
  }
  return table.length.back() > 0.0;
}

/** The parameter at arc length s from the start, refined by Newton steps. */
double parameterAtLength(const RS_Spline &spline, const ArcLengthTable &table, const double s) {
  const auto upper = std::upper_bound(table.length.begin(), table.length.end(), s);
  if (upper == table.length.begin()) {
    return table.t.front();
  }
  if (upper == table.length.end()) {
    return table.t.back();
  }
  const auto i = static_cast<size_t>(std::distance(table.length.begin(), upper)) - 1;
  const double a = table.t[i];
  const double b = table.t[i + 1];
  const double span = table.length[i + 1] - table.length[i];
  double t = (span > 0.0) ? a + (b - a) * (s - table.length[i]) / span : a;
  for (int iteration = 0; iteration < 4; ++iteration) {
    const double speed = speedAt(spline, t);
    if (!(speed > 0.0)) {
      break;
    }
    t = std::clamp(t - (table.length[i] + pieceLength(spline, a, t) - s) / speed, a, b);
  }
  return t;
}

/** A homogeneous point (w x, w y, w) as intervals. */
struct HomogeneousBox {
  LC_Interval x;
  LC_Interval y;
  LC_Interval w;
};

HomogeneousBox combine(const LC_Interval &alpha, const HomogeneousBox &a, const HomogeneousBox &b) {
  const LC_Interval beta = LC_Interval::point(1.0) - alpha;
  return {beta * a.x + alpha * b.x, beta * a.y + alpha * b.y, beta * a.w + alpha * b.w};
}

HomogeneousBox difference(const HomogeneousBox &a, const HomogeneousBox &b) {
  return {a.x - b.x, a.y - b.y, a.w - b.w};
}

HomogeneousBox scaled(const LC_Interval &factor, const HomogeneousBox &a) {
  return {factor * a.x, factor * a.y, factor * a.w};
}

/**
 * The blossom f(u[0], ..., u[q-1]) of a homogeneous B-spline segment of degree
 * q on knot span s, from its q + 1 control points there, pts[0 .. q] (control
 * points s - q .. s), by de Boor's algorithm with a different parameter at each
 * level; knot(k) is the spline's knot k. With a repeated (q - m) times and b
 * repeated m times it is Bezier control point m of the segment restricted to
 * [a, b]. pts is overwritten.
 */
template <typename Knot>
HomogeneousBox blossom(HomogeneousBox *pts, const size_t q, const size_t s, const double *u, const Knot &knot) {
  for (size_t r = 1; r <= q; ++r) {
    for (size_t j = q; j >= r; --j) {
      const LC_Interval lo = LC_Interval::point(knot(s - q + j));
      const LC_Interval hi = LC_Interval::point(knot(s + 1 + j - r));
      const LC_Interval alpha = (LC_Interval::point(u[r - 1]) - lo) / (hi - lo);
      pts[j] = combine(alpha, pts[j - 1], pts[j]);
    }
  }
  return pts[q];
}

/** The hull of values[0 .. count-1]. */
LC_Interval hullOf(const LC_Interval *values, const size_t count) {
  LC_Interval result = values[0];
  for (size_t i = 1; i < count; ++i) {
    result = LC_Interval::hull(result, values[i]);
  }
  return result;
}

/**
 * The hull of the Bezier net over [a, b] of a homogeneous B-spline segment of
 * degree q on knot span s with control points pts[0 .. q]: it contains every
 * value the segment takes there (convex hull property).
 */
template <typename Knot>
HomogeneousBox boundSegment(const HomogeneousBox *pts, const size_t q, const size_t s, const double a,
                            const double b, const Knot &knot) {
  LC_Interval xs[g_maxDegree + 1];
  LC_Interval ys[g_maxDegree + 1];
  LC_Interval ws[g_maxDegree + 1];
  for (size_t m = 0; m <= q; ++m) {
    double u[g_maxDegree];
    for (size_t k = 0; k < q; ++k) {
      u[k] = (k < q - m) ? a : b;
    }
    HomogeneousBox work[g_maxDegree + 1];
    std::copy(pts, pts + q + 1, work);
    const HomogeneousBox point = blossom(work, q, s, u, knot);
    xs[m] = point.x;
    ys[m] = point.y;
    ws[m] = point.w;
  }
  return {hullOf(xs, q + 1), hullOf(ys, q + 1), hullOf(ws, q + 1)};
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
  return (s != 0u) ? std::vector<RS_Vector>(m_data.controlPoints.begin(),
                                    m_data.controlPoints.begin() + s)
           : std::vector<RS_Vector>{};
}

std::vector<double> RS_Spline::getUnwrappedWeights() const {
  const size_t s = getUnwrappedSize();
  return (s != 0u) ? std::vector<double>(m_data.weights.begin(), m_data.weights.begin() + s)
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
  if (m_data.degree == static_cast<size_t>(degree)) {
      return;
  }
  // The knot vector, and the control points a closed spline wraps, are sized by the degree: a closed
  // spline is unwrapped with its old degree and wrapped again with the new one, and knots made for
  // the old degree are replaced by uniform knots for the spline's type. Setting the degree alone left
  // a spline that failed validate(), so update() drew nothing.
  const bool closed = isClosed();
  if (closed) {
      removeWrapping();
  }
  m_data.degree = degree;
  const size_t count = m_data.controlPoints.size();
  if (count < m_data.degree + 1) {
      m_data.knotslist.clear();
  }
  else if (m_data.type == RS_SplineData::SplineType::Standard) {
      m_data.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(count, m_data.degree + 1);
  }
  else {
      m_data.knotslist = LC_SplineHelper::knot(count, m_data.degree + 1);
  }
  if (closed) {
      addWrapping();
  }
  update();
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
// the nearest of the reference points above, not of the ends of the drawn lines, which moveRef() does not move
RS_Vector RS_Spline::doGetNearestRef(const RS_Vector &coord, double *dist) const {
  return RS_Entity::doGetNearestRef(coord, dist);
}

RS_Vector RS_Spline::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_Entity::doGetNearestSelectedRef(coord, dist);
}

/** Update approximation */
void RS_Spline::update() {
  clear();
  if (!validate()) {
      return;
  }
  std::vector<RS_Vector> points;
  fillDisplayPoints(points);
  for (size_t i = 0; i + 1 < points.size(); ++i) {
      addEntity(new RS_Line(this, points[i], points[i + 1]));
  }
  if (isClosed() && points.size() > 1) {
      addEntity(new RS_Line(this, points.back(), points.front()));
  }
}

void RS_Spline::fillDisplayPoints(std::vector<RS_Vector> &points) const {
  points.clear();
  double t0 = 0.0;
  double t1 = 0.0;
  const std::vector<double> breaks = getBreakParameters();
  RS_Vector lo = m_data.controlPoints.empty() ? RS_Vector{} : m_data.controlPoints.front();
  RS_Vector hi = lo;
  for (const RS_Vector &v : m_data.controlPoints) {
    lo = RS_Vector::minimum(lo, v);
    hi = RS_Vector::maximum(hi, v);
  }
  const double tolerance = g_displayRelativeTolerance * lo.distanceTo(hi);
  const auto uniform = [&] {
    points.clear();
    fillStrokePoints(g_minimumDisplaySegments, points);
  };
  if (!getParameterDomain(t0, t1) || breaks.size() < 2 || !std::isfinite(tolerance) || !(tolerance > 0.0)) {
    uniform();
    return;
  }
  const auto append = [&](const double t, const LC_CurveEvaluationSide side) {
    LC_CurveJet jet;
    if (!tryEvaluateJet(t, side, jet)) {
      return false;
    }
    points.push_back(jet.point);
    return true;
  };
  if (!append(t0, LC_CurveEvaluationSide::Right)) {
    uniform();
    return;
  }
  // Span by span, uniformly: a share of the minimum count, or more where a
  // chord, within h^2/8 |C''| of its arc, would stray past the tolerance.
  const auto &U = m_data.knotslist;
  for (size_t k = 0; k + 1 < breaks.size(); ++k) {
    const double a = breaks[k];
    const double b = breaks[k + 1];
    if (k > 0 && static_cast<size_t>(std::count(U.begin(), U.end(), a)) >= m_data.degree) {
      // a knot of full multiplicity may break the curve: start at its right limit
      LC_CurveJet start;
      if (!tryEvaluateJet(a, LC_CurveEvaluationSide::Right, start)) {
        uniform();
        return;
      }
      if (start.point != points.back()) {
        points.push_back(start.point);
      }
    }
    LC_CurveJetBounds bounds;
    if (!tryBoundJet(a, b, bounds)) {
      uniform();
      return;
    }
    const double bend = std::hypot(std::max(std::abs(bounds.ddx.lo()), std::abs(bounds.ddx.hi())),
                                   std::max(std::abs(bounds.ddy.lo()), std::abs(bounds.ddy.hi())));
    const double share = std::ceil(g_minimumDisplaySegments * (b - a) / (t1 - t0));
    const double needed = std::ceil((b - a) * std::sqrt(bend / (8.0 * tolerance)));
    const double left = static_cast<double>(g_maximumDisplaySegments) - static_cast<double>(points.size());
    const int count = static_cast<int>(std::clamp(std::max({1.0, share, needed}), 1.0, std::max(1.0, left)));
    for (int i = 1; i <= count; ++i) {
      const bool end = i == count;
      if (!append(end ? b : a + (b - a) * i / count,
                  end ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Interior)) {
        uniform();
        return;
      }
    }
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
RS_Vector RS_Spline::getStartpoint() const {
  double t0 = 0.0;
  double t1 = 0.0;
  LC_CurveJet jet;
  if (isClosed() || !getParameterDomain(t0, t1) ||
      !tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, jet)) {
    return RS_Vector(false);
  }
  return jet.point;
}

RS_Vector RS_Spline::getEndpoint() const {
  double t0 = 0.0;
  double t1 = 0.0;
  LC_CurveJet jet;
  if (isClosed() || !getParameterDomain(t0, t1) ||
      !tryEvaluateJet(t1, LC_CurveEvaluationSide::Left, jet)) {
    return RS_Vector(false);
  }
  return jet.point;
}

/** Nearest (invalid overrides) */
RS_Vector RS_Spline::doGetNearestEndpoint(const RS_Vector &coord, double *dist, RS_Entity** entity) const {
  if (dist != nullptr) {
    *dist = RS_MAXDOUBLE;
  }
  const RS_Vector start = getStartpoint();
  const RS_Vector end = getEndpoint();
  if (!start.valid || !end.valid) {
    return RS_Vector(false); // closed or not a curve: no endpoints
  }
  const double toStart = coord.distanceTo(start);
  const double toEnd = coord.distanceTo(end);
  if (dist != nullptr) {
    *dist = std::min(toStart, toEnd);
  }
  if (entity != nullptr) {
    *entity = const_cast<RS_Spline *>(this);
  }
  return (toStart <= toEnd) ? start : end;
}
RS_Vector RS_Spline::doGetNearestCenter(const RS_Vector &, double *, RS_Entity** centerEntity) const {
  return RS_Vector(false);
}
RS_Vector RS_Spline::doGetNearestMiddle(const RS_Vector &coord, double *dist, const int middlePoints) const {
  if (dist != nullptr) {
    *dist = RS_MAXDOUBLE;
  }
  // the points dividing an open curve into middlePoints + 1 parts of equal length
  ArcLengthTable table;
  if (isClosed() || middlePoints < 1 || !buildArcLengthTable(*this, table)) {
    return RS_Vector(false);
  }
  const double total = table.length.back();
  RS_Vector best(false);
  double bestDistance = RS_MAXDOUBLE;
  for (int k = 1; k <= middlePoints; ++k) {
    const RS_Vector p = getPointAt(parameterAtLength(*this, table, total * k / (middlePoints + 1)));
    if (p.valid && coord.distanceTo(p) < bestDistance) {
      bestDistance = coord.distanceTo(p);
      best = p;
    }
  }
  if (dist != nullptr && best.valid) {
    *dist = bestDistance;
  }
  return best;
}
RS_Vector RS_Spline::doGetNearestDist(const double distance, const RS_Vector &coord, double *dist) const {
  if (dist != nullptr) {
    *dist = RS_MAXDOUBLE;
  }
  // the point that far along the curve from the end nearer to coord
  ArcLengthTable table;
  const RS_Vector start = getStartpoint();
  const RS_Vector end = getEndpoint();
  if (!start.valid || !end.valid || !std::isfinite(distance) || !buildArcLengthTable(*this, table)) {
    return RS_Vector(false);
  }
  const double total = table.length.back();
  if (distance < 0.0 || distance > total) {
    return RS_Vector(false);
  }
  const bool fromStart = coord.distanceTo(start) <= coord.distanceTo(end);
  const RS_Vector p = getPointAt(parameterAtLength(*this, table, fromStart ? distance : total - distance));
  if (dist != nullptr && p.valid) {
    *dist = coord.distanceTo(p);
  }
  return p;
}

/** Transformations
 * Each transforms the control and fit points, then update() rebuilds the lines the spline is drawn,
 * hit-tested and bounded by; transforming the points alone left those lines where they were.
 */
void RS_Spline::move(const RS_Vector &offset) {
  for (auto &cp : m_data.controlPoints) {
      cp += offset;
  }
  for (auto &fp : m_data.fitPoints) {
      fp += offset;
  }
  update();
}

void RS_Spline::rotate(const RS_Vector &center, const double angle) {
  for (auto &cp : m_data.controlPoints) {
      cp.rotate(center, angle);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.rotate(center, angle);
  }
  update();
}

void RS_Spline::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  for (auto &cp : m_data.controlPoints) {
      cp.rotate(center, angleVector);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.rotate(center, angleVector);
  }
  update();
}

void RS_Spline::scale(const RS_Vector &center, const RS_Vector &factor) {
  for (auto &cp : m_data.controlPoints) {
      cp.scale(center, factor);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.scale(center, factor);
  }
  update();
}

RS_Entity &RS_Spline::shear(const double k) {
  for (auto &cp : m_data.controlPoints) {
      cp.shear(k);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.shear(k);
  }
  update();
  return *this;
}

void RS_Spline::mirror(const RS_Vector &a1, const RS_Vector &a2) {
  for (auto &cp : m_data.controlPoints) {
      cp.mirror(a1, a2);
  }
  for (auto &fp : m_data.fitPoints) {
      fp.mirror(a1, a2);
  }
  update();
}

void RS_Spline::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
  // The reference points are the control points. Moving the ends of the drawn lines instead, as the
  // container does, left the control points in place and lasted only until the lines were rebuilt.
  bool moved = false;
  for (size_t i = 0; i < getUnwrappedSize(); ++i) {
      RS_Vector &point = m_data.controlPoints[i];
      if (std::abs(point.x - ref.x) < 1.0e-4 && std::abs(point.y - ref.y) < 1.0e-4) {
          point.move(offset);
          moved = true;
      }
  }
  if (!moved) {
      return;
  }
  if (isClosed()) {
      updateControlAndWeightWrapping();
  }
  update();
}

/** Revert direction */
void RS_Spline::revertDirection() {
  // The reversed curve runs over the mirrored parameter t' = U[0] + U[m] - t.
  // Reversing a knot vector alone leaves it decreasing, which no spline accepts.
  auto mirror = [](std::vector<double> &knots) {
    if (knots.empty()) {
      return;
    }
    const double sum = knots.front() + knots.back();
    std::reverse(knots.begin(), knots.end());
    for (double &k : knots) {
      k = sum - k;
    }
  };
  // Reversing the whole control array keeps a wrapped closed spline wrapped:
  // its first and last degree entries still repeat each other.
  std::reverse(m_data.controlPoints.begin(), m_data.controlPoints.end());
  std::reverse(m_data.weights.begin(), m_data.weights.end());
  std::reverse(m_data.fitPoints.begin(), m_data.fitPoints.end());
  mirror(m_data.knotslist);
  mirror(m_data.savedOpenKnots);
  update();
}

std::vector<RS_Entity *> RS_Spline::createOffset(const RS_Vector &coord, const double &distance) const {
  return LC_CurveOffset::createLegacyOffset(*this, coord, distance);
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
  const double step = x.back() / numPoints;
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
std::vector<double> RS_Spline::findDerivativeZeros(const bool isX) const {
  std::vector<double> zeros;
  const std::vector<double> breaks = getBreakParameters();
  if (breaks.size() < 2) {
    return zeros;
  }

  // The derivative component on the span being searched: at a break it takes the
  // limit from inside that span, since it may jump there. NaN on failure.
  auto derivative = [this, isX](const double t, const LC_CurveEvaluationSide side) {
    LC_CurveJet jet;
    if (!tryEvaluateJet(t, side, jet)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    return isX ? jet.first.x : jet.first.y;
  };
  auto addIfBracketed = [&](const double a, const double b, const double fa, const double fb) {
    if (fa == 0.0) {
      zeros.push_back(a);
    } else if (fb == 0.0) {
      zeros.push_back(b);
    } else {
      const double root = bisectDerivativeZero(a, b, fa, fb, isX);
      if (std::isfinite(root)) {
        zeros.push_back(root);
      }
    }
  };

  // A derivative component of a cubic span has up to two roots; sampling each
  // span several times separates them unless they almost coincide.
  constexpr int samplesPerSpan = 8;
  for (size_t i = 0; i + 1 < breaks.size(); ++i) {
    const double a = breaks[i];
    const double b = breaks[i + 1];
    double t0 = a;
    double f0 = derivative(a, LC_CurveEvaluationSide::Right);
    for (int k = 1; k <= samplesPerSpan; ++k) {
      const bool last = (k == samplesPerSpan);
      const double t1 = last ? b : a + (b - a) * k / samplesPerSpan;
      const double f1 = derivative(t1, last ? LC_CurveEvaluationSide::Left
                                            : LC_CurveEvaluationSide::Interior);
      addIfBracketed(t0, t1, f0, f1);
      t0 = t1;
      f0 = f1;
    }
  }
  // A closed spline's component can change sign across the seam itself, which
  // neither end interval brackets unless it evaluates to exactly zero.
  if (isClosed()) {
    const double first = derivative(breaks.front(), LC_CurveEvaluationSide::Right);
    const double last = derivative(breaks.back(), LC_CurveEvaluationSide::Left);
    if (std::isfinite(first) && std::isfinite(last) && first != 0.0 && last != 0.0 &&
        std::signbit(first) != std::signbit(last)) {
      zeros.push_back(breaks.front());
    }
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

  size_t num = fp.size();
  // A clamped degree-p B-spline needs at least p+1 control points. When the
  // caller asks for degree p with fewer than p+1 fit points, lower the working
  // degree to (num-1) so the interpolation produces a valid spline (single
  // Bezier patch of the highest degree the data supports).  Without this
  // clamp, the basis-function loop below indexes A[row][idx-1] past sys=num-2
  // and corrupts memory.
  int p = std::clamp(static_cast<int>(m_data.degree), 1, 3);
  if (p > static_cast<int>(num) - 1) {
      p = static_cast<int>(num) - 1;
  }
  m_data.degree = static_cast<size_t>(p);
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

  // Only short-circuit when there is no interior system to solve (sys == 0).
  // The previous cutoff `num <= p+1` skipped the solve for cases like num=4,
  // p=3 (a single cubic Bezier interpolating 4 fit points), leaving the
  // interior control points as RS_Vector(false) — which read back as (0,0)
  // and produced a collapsed spline.
  if (num <= 2) {
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
  const int order = degree + 1, np = static_cast<int>(numControls), c = order,
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

bool RS_Spline::getParameterDomain(double &t0, double &t1) const {
  if (!hasEvaluableLayout(m_data)) {
    return false;
  }
  const auto &U = m_data.knotslist;
  const double lo = U[m_data.degree];
  const double hi = U[m_data.controlPoints.size()];
  if (!std::isfinite(lo) || !std::isfinite(hi) || !(lo < hi)) {
    return false;
  }
  t0 = lo;
  t1 = hi;
  return true;
}

std::vector<double> RS_Spline::getBreakParameters() const {
  double t0 = 0.0;
  double t1 = 0.0;
  if (!getParameterDomain(t0, t1)) {
    return {};
  }
  const auto &U = m_data.knotslist;
  std::vector<double> breaks{t0};
  for (size_t i = m_data.degree + 1; i < m_data.controlPoints.size(); ++i) {
    const double u = U[i];
    if (!std::isfinite(u) || u < breaks.back()) {
      return {}; // decreasing knots: not a curve
    }
    if (u > breaks.back() && u < t1) {
      breaks.push_back(u);
    }
  }
  breaks.push_back(t1);
  return breaks;
}

bool RS_Spline::tryBoundJet(const double a, const double b, LC_CurveJetBounds &bounds) const {
  bounds = LC_CurveJetBounds{};
  double t0 = 0.0;
  double t1 = 0.0;
  if (!std::isfinite(a) || !std::isfinite(b) || !(a < b) || !getParameterDomain(t0, t1) ||
      a < t0 || b > t1) {
    return false;
  }
  const auto &U = m_data.knotslist;
  const size_t p = m_data.degree;
  const size_t ncp = m_data.controlPoints.size();
  const auto domainBegin = U.begin() + static_cast<std::ptrdiff_t>(p);
  const auto domainEnd = U.begin() + static_cast<std::ptrdiff_t>(ncp + 1);
  const std::ptrdiff_t index = std::distance(U.begin(), std::upper_bound(domainBegin, domainEnd, a)) - 1;
  if (index < static_cast<std::ptrdiff_t>(p) || index > static_cast<std::ptrdiff_t>(ncp) - 1) {
    return false;
  }
  const auto span = static_cast<size_t>(index);
  if (b > U[span + 1]) {
    return false; // the box crosses a knot
  }
  bool equalWeights = true;
  for (size_t i = span + 1 - p; i < span + p; ++i) {
    if (!(U[i] <= U[i + 1]) || !std::isfinite(U[i]) || !std::isfinite(U[i + 1])) {
      return false;
    }
  }
  for (size_t i = span - p; i <= span; ++i) {
    const double w = m_data.weights[i];
    if (!std::isfinite(w) || !(w > 0.0) || !isFinite(m_data.controlPoints[i])) {
      return false;
    }
    equalWeights = equalWeights && w == m_data.weights[span - p];
  }

  // The homogeneous control points of the span, and those of its first and
  // second derivative splines. Bounding a derivative from its own control
  // points, differences of the curve's, keeps a small box from dividing
  // rounding in nearly equal Bezier points by its width.
  // Relative to the span's first control point, so that the rational quotient
  // bounds scale with the span's size rather than with its coordinates.
  const RS_Vector origin = m_data.controlPoints[span - p];
  HomogeneousBox h[g_maxDegree + 1];
  for (size_t j = 0; j <= p; ++j) {
    const size_t i = span - p + j;
    const LC_Interval w = LC_Interval::point(m_data.weights[i]);
    h[j] = {(LC_Interval::point(m_data.controlPoints[i].x) - LC_Interval::point(origin.x)) * w,
            (LC_Interval::point(m_data.controlPoints[i].y) - LC_Interval::point(origin.y)) * w, w};
  }
  HomogeneousBox h1[g_maxDegree];
  for (size_t j = 0; j < p; ++j) {
    const LC_Interval gap = LC_Interval::point(U[span + j + 1]) - LC_Interval::point(U[span - p + j + 1]);
    h1[j] = scaled(LC_Interval::point(static_cast<double>(p)) / gap, difference(h[j + 1], h[j]));
  }
  HomogeneousBox h2[g_maxDegree];
  for (size_t j = 0; j + 1 < p; ++j) {
    const LC_Interval gap = LC_Interval::point(U[span + j + 1]) - LC_Interval::point(U[span - p + j + 2]);
    h2[j] = scaled(LC_Interval::point(static_cast<double>(p - 1)) / gap, difference(h1[j + 1], h1[j]));
  }
  // a derivative spline's knots are the curve's without the first (and last)
  const HomogeneousBox value = boundSegment(h, p, span, a, b, [&U](const size_t k) { return U[k]; });
  const HomogeneousBox first = boundSegment(h1, p - 1, span - 1, a, b, [&U](const size_t k) { return U[k + 1]; });
  const HomogeneousBox zero{LC_Interval::point(0.0), LC_Interval::point(0.0), LC_Interval::point(0.0)};
  const HomogeneousBox second =
      (p < 2) ? zero : boundSegment(h2, p - 2, span - 2, a, b, [&U](const size_t k) { return U[k + 2]; });
  const LC_Interval &ax = value.x;
  const LC_Interval &ay = value.y;
  const LC_Interval &ax1 = first.x;
  const LC_Interval &ay1 = first.y;
  const LC_Interval &ax2 = second.x;
  const LC_Interval &ay2 = second.y;

  LC_CurveJetBounds result;
  if (equalWeights) {
    // a polynomial curve: C = A / w with constant w
    const LC_Interval w = LC_Interval::point(m_data.weights[span - p]);
    result = {ax / w, ay / w, ax1 / w, ay1 / w, ax2 / w, ay2 / w};
  } else {
    const LC_Interval &w = value.w;
    const LC_Interval &w1 = first.w;
    const LC_Interval &w2 = second.w;
    if (!w.isPositive()) {
      return false;
    }
    // C = A/W, C' = (A' - W'C)/W, C'' = (A'' - 2W'C' - W''C)/W
    const LC_Interval two = LC_Interval::point(2.0);
    result.x = ax / w;
    result.y = ay / w;
    result.dx = (ax1 - w1 * result.x) / w;
    result.dy = (ay1 - w1 * result.y) / w;
    result.ddx = (ax2 - two * w1 * result.dx - w2 * result.x) / w;
    result.ddy = (ay2 - two * w1 * result.dy - w2 * result.y) / w;
  }
  result.x = result.x + LC_Interval::point(origin.x);
  result.y = result.y + LC_Interval::point(origin.y);
  if (!result.isValid()) {
    return false;
  }
  bounds = result;
  return true;
}

bool RS_Spline::tryStroke(const double tolerance, const size_t maxVertices,
                          std::vector<RS_Vector> &vertices) const {
  vertices.clear();
  const std::vector<double> breaks = getBreakParameters();
  if (breaks.size() < 2 || !std::isfinite(tolerance) || tolerance <= 0.0 || maxVertices < 2) {
    return false;
  }
  const auto fail = [&vertices] {
    vertices.clear();
    return false;
  };
  const auto append = [&](const double t, const LC_CurveEvaluationSide side) {
    LC_CurveJet jet;
    if (vertices.size() >= maxVertices || !tryEvaluateJet(t, side, jet)) {
      return false;
    }
    vertices.push_back(jet.point);
    return true;
  };
  if (!append(breaks.front(), LC_CurveEvaluationSide::Right)) {
    return fail();
  }
  // A knot of full multiplicity may break the curve: a span starts at its own
  // right limit when that is more than jumpSlack away. Chords get the rest.
  const double jumpSlack = tolerance / 8.0;
  const double chordTolerance = tolerance - jumpSlack;
  std::vector<std::pair<double, double>> pending;
  for (size_t i = 1; i < breaks.size(); ++i) {
    if (i > 1) {
      LC_CurveJet start;
      if (!tryEvaluateJet(breaks[i - 1], LC_CurveEvaluationSide::Right, start)) {
        return fail();
      }
      if (start.point.distanceTo(vertices.back()) > jumpSlack &&
          !append(breaks[i - 1], LC_CurveEvaluationSide::Right)) {
        return fail();
      }
    }
    pending.assign(1, {breaks[i - 1], breaks[i]});
    while (!pending.empty()) {
      const auto [a, b] = pending.back();
      pending.pop_back();
      LC_CurveJetBounds bounds;
      if (!tryBoundJet(a, b, bounds)) {
        return fail();
      }
      const double ddx = std::max(std::abs(bounds.ddx.lo()), std::abs(bounds.ddx.hi()));
      const double ddy = std::max(std::abs(bounds.ddy.lo()), std::abs(bounds.ddy.hi()));
      const double h = b - a;
      if (h * h / 8.0 * std::hypot(ddx, ddy) > chordTolerance) {
        const double mid = a + 0.5 * h;
        if (!(mid > a && mid < b)) {
          return fail(); // no parameter left to split
        }
        pending.emplace_back(mid, b);
        pending.emplace_back(a, mid);
        continue;
      }
      const bool spanEnd = b == breaks[i];
      if (!append(b, spanEnd ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Interior)) {
        return fail();
      }
    }
  }
  return true;
}

bool RS_Spline::tryEvaluateJet(double t, const LC_CurveEvaluationSide side, LC_CurveJet &jet) const {
  jet = LC_CurveJet{};
  double t0 = 0.0;
  double t1 = 0.0;
  if (!std::isfinite(t) || !getParameterDomain(t0, t1)) {
    return false;
  }

  // A parameter computed from the domain ends can miss them by a few ulps.
  const double slack = 4.0 * std::numeric_limits<double>::epsilon() *
                       std::max({std::abs(t0), std::abs(t1), t1 - t0});
  if (t < t0) {
    if (t < t0 - slack) {
      return false;
    }
    t = t0;
  } else if (t > t1) {
    if (t > t1 + slack) {
      return false;
    }
    t = t1;
  }

  LC_CurveEvaluationSide limit = side;
  if (limit == LC_CurveEvaluationSide::Interior) {
    limit = (t < t1) ? LC_CurveEvaluationSide::Right : LC_CurveEvaluationSide::Left;
  }

  // Knot span s with U[s] <= t < U[s+1] (right limit) or U[s] < t <= U[s+1]
  // (left limit), searched among the spans of the domain.
  const auto &U = m_data.knotslist;
  const size_t p = m_data.degree;
  const size_t ncp = m_data.controlPoints.size();
  const auto domainBegin = U.begin() + static_cast<std::ptrdiff_t>(p);
  const auto domainEnd = U.begin() + static_cast<std::ptrdiff_t>(ncp + 1);
  const auto bound = (limit == LC_CurveEvaluationSide::Right)
                         ? std::upper_bound(domainBegin, domainEnd, t)
                         : std::lower_bound(domainBegin, domainEnd, t);
  const std::ptrdiff_t index = std::distance(U.begin(), bound) - 1;
  if (index < static_cast<std::ptrdiff_t>(p) || index > static_cast<std::ptrdiff_t>(ncp) - 1) {
    return false; // left limit at t0 or right limit at t1
  }
  const auto span = static_cast<size_t>(index);
  for (size_t i = span + 1 - p; i < span + p; ++i) {
    if (!(U[i] <= U[i + 1])) {
      return false;
    }
  }

  double ders[3][g_maxDegree + 1];
  if (!dersBasisFunctions(span, t, p, U, ders)) {
    return false;
  }

  // Homogeneous numerator A and denominator W with their derivatives.
  RS_Vector a[3]{RS_Vector{0.0, 0.0, 0.0}, RS_Vector{0.0, 0.0, 0.0}, RS_Vector{0.0, 0.0, 0.0}};
  double w[3]{0.0, 0.0, 0.0};
  for (size_t j = 0; j <= p; ++j) {
    const size_t i = span - p + j;
    const double wi = m_data.weights[i];
    const RS_Vector &pi = m_data.controlPoints[i];
    if (!std::isfinite(wi) || !(wi > 0.0) || !isFinite(pi)) {
      return false;
    }
    for (int k = 0; k <= 2; ++k) {
      a[k] += pi * (wi * ders[k][j]);
      w[k] += wi * ders[k][j];
    }
  }
  if (!std::isfinite(w[0]) || !(w[0] > 0.0)) {
    return false;
  }

  // C = A/W, C' = (A' - W'C)/W, C'' = (A'' - 2W'C' - W''C)/W
  // by the reciprocal: RS_Vector's operator/ leaves a vector undivided by
  // anything below RS_TOLERANCE, and uniform tiny weights are a valid curve
  const double inverse = 1.0 / w[0];
  const RS_Vector point = a[0] * inverse;
  const RS_Vector first = (a[1] - point * w[1]) * inverse;
  const RS_Vector second = (a[2] - first * (2.0 * w[1]) - point * w[2]) * inverse;
  if (!isFinite(point) || !isFinite(first) || !isFinite(second)) {
    return false;
  }
  jet.point = point;
  jet.first = first;
  jet.second = second;
  return true;
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
  const double vx = d.der1.x, vy = d.der1.y;
  const double ax = d.der2.x, ay = d.der2.y;
  const double speed2 = (vx * vx) + (vy * vy);
  if (speed2 < 1e-20) {
      return 0.0;
  }
  return std::abs((vx * ay) - (vy * ax)) / std::pow(speed2, 1.5);
}

double RS_Spline::getSignedCurvature(const double t) const {
  const auto d = evaluateWithDerivs(t);
  const double vx = d.der1.x, vy = d.der1.y;
  const double ax = d.der2.x, ay = d.der2.y;
  const double speed = std::hypot(vx, vy);
  if (speed < 1e-10) {
      return 0.0;
  }
  return (vx * ay - vy * ax) / (speed * speed * speed);
}

RS_Spline::SplineDerivs RS_Spline::evaluateWithDerivs(const double t) const {
  SplineDerivs res;
  LC_CurveJet jet;
  if (tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet)) {
    res.pos = jet.point;
    res.der1 = jet.first;
    res.der2 = jet.second;
  } else {
    // a failure is invalid, never a curve point at the origin
    res.pos = RS_Vector(false);
    res.der1 = RS_Vector(false);
    res.der2 = RS_Vector(false);
  }
  return res;
}

/**
 * Bisection for a root of one derivative component on [low, high], given its
 * values at both ends. Returns NaN unless the ends have opposite signs, so a
 * missing root is never reported as the midpoint.
 */
double RS_Spline::bisectDerivativeZero(double low, double high, double fLow, double fHigh,
                                       const bool isX) const {
  if (!std::isfinite(fLow) || !std::isfinite(fHigh) ||
      std::signbit(fLow) == std::signbit(fHigh)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  for (int iteration = 0; iteration < 200; ++iteration) {
    const double mid = low + ((high - low) * 0.5);
    if (!(mid > low && mid < high)) {
      break; // the bracket cannot shrink further
    }
    const double fMid = getDerivative(mid, isX);
    if (!std::isfinite(fMid)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    if (fMid == 0.0) {
      return mid;
    }
    if (std::signbit(fMid) == std::signbit(fLow)) {
      low = mid;
      fLow = fMid;
    } else {
      high = mid;
      fHigh = fMid;
    }
  }
  return std::abs(fLow) < std::abs(fHigh) ? low : high;
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

      N[r] = saved + (right[r + 1] * alpha);
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

  const size_t p = m_data.degree, n = P.size();
  if (n <= p + 1) {
      return;
  }

  // Valid interior parameter range only – endpoint insertion has no effect
  const double umin = K[p], umax = K[n];
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
