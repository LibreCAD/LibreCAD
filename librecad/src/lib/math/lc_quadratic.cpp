/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2024 LibreCAD.org
** Copyright (C) 2015-2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2026 Grok team (bug fixes, modern C++ cleanup, mirror(),
**                              getDualCurve fix, RAII support)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include <algorithm>
#include <cfloat>
#include <numeric>
#include <utility>          // std::move

#include "lc_quadratic.h"
#include "lc_quadraticutils.h"

#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif

/**
 * Default constructor – creates an invalid quadratic.
 */
LC_Quadratic::LC_Quadratic()
    : m_mQuad(2,2)
      , m_vLinear(2)
      , m_bValid(false)
{}

/**
 * Copy constructor.
 */
LC_Quadratic::LC_Quadratic(const LC_Quadratic& lc0)
    : m_bIsQuadratic(lc0.isQuadratic())
      , m_bValid(lc0.m_bValid)
{
  if (!m_bValid)
    return;
  if (m_bIsQuadratic)
    m_mQuad = lc0.getQuad();
  m_vLinear = lc0.getLinear();
  m_dConst = lc0.m_dConst;
}

/**
 * Move constructor (C++11).
 */
LC_Quadratic::LC_Quadratic(LC_Quadratic&& lc0) noexcept
    : m_mQuad(std::move(lc0.m_mQuad))
      , m_vLinear(std::move(lc0.m_vLinear))
      , m_dConst(lc0.m_dConst)
      , m_bIsQuadratic(lc0.m_bIsQuadratic)
      , m_bValid(lc0.m_bValid)
{}

/**
 * Copy assignment operator.
 */
LC_Quadratic& LC_Quadratic::operator=(const LC_Quadratic& lc0)
{
  if (this == &lc0) return *this;
  if (lc0.isQuadratic()) {
    m_mQuad.resize(2, 2, false);
    m_mQuad = lc0.getQuad();
  }
  m_vLinear = lc0.getLinear();
  m_dConst = lc0.m_dConst;
  m_bIsQuadratic = lc0.isQuadratic();
  m_bValid = lc0.m_bValid;
  return *this;
}

/**
 * Move assignment operator (C++11).
 */
LC_Quadratic& LC_Quadratic::operator=(LC_Quadratic&& lc0) noexcept
{
  if (this == &lc0) return *this;
  m_mQuad = std::move(lc0.m_mQuad);
  m_vLinear = std::move(lc0.m_vLinear);
  m_dConst = lc0.m_dConst;
  m_bIsQuadratic = lc0.m_bIsQuadratic;
  m_bValid = lc0.m_bValid;
  return *this;
}

LC_Quadratic::LC_Quadratic(std::vector<double> ce)
    : m_mQuad(2,2)
      , m_vLinear(2)
{
  if (ce.size() == 6) {
    // quadratic
    m_mQuad(0,0) = ce[0];
    m_mQuad(0,1) = 0.5 * ce[1];
    m_mQuad(1,0) = m_mQuad(0,1);
    m_mQuad(1,1) = ce[2];
    m_vLinear(0) = ce[3];
    m_vLinear(1) = ce[4];
    m_dConst = ce[5];
    m_bIsQuadratic = true;
    m_bValid = true;
    return;
  }
  if (ce.size() == 3) {
    m_vLinear(0) = ce[0];
    m_vLinear(1) = ce[1];
    m_dConst = ce[2];
    m_bIsQuadratic = false;
    m_bValid = true;
    return;
  }
  m_bValid = false;
}

/** construct a parabola, ellipse or hyperbola as the path of center of tangent circles
  passing the point */
LC_Quadratic::LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point)
    : m_mQuad(2,2)
      , m_vLinear(2)
      , m_bIsQuadratic(true)
      , m_bValid(true)
{
  if (circle == nullptr) {
    m_bValid = false;
    return;
  }
  switch (circle->rtti()) {
  case RS2::EntityArc:
  case RS2::EntityCircle:
  {
    RS_Vector center = circle->getCenter();
    double r = circle->getRadius();
    if (!center.valid) {
      m_bValid = false;
      return;
    }
    double c = 0.5 * (center.distanceTo(point));
    double d = 0.5 * r;
    if (std::abs(c) < RS_TOLERANCE || std::abs(d) < RS_TOLERANCE || std::abs(c - d) < RS_TOLERANCE) {
      m_bValid = false;
      return;
    }
    m_mQuad(0,0) = 1. / (d * d);
    m_mQuad(0,1) = 0.;
    m_mQuad(1,0) = 0.;
    m_mQuad(1,1) = 1. / (d * d - c * c);
    m_vLinear(0) = 0.;
    m_vLinear(1) = 0.;
    m_dConst = -1.;
    center = (center + point) * 0.5;
    rotate(center.angleTo(point));
    move(center);
    return;
  }
  case RS2::EntityLine:
  {
    const RS_Line* line = static_cast<const RS_Line*>(circle);
    RS_Vector direction = line->getEndpoint() - line->getStartpoint();
    double l2 = direction.squared();
    if (l2 < RS_TOLERANCE2) {
      m_bValid = false;
      return;
    }
    RS_Vector projection = line->getNearestPointOnEntity(point, false);
    double p2 = (projection - point).squared();
    if (p2 < RS_TOLERANCE2) {
      // point on line, return a straight line
      m_bIsQuadratic = false;
      m_vLinear(0) = direction.y;
      m_vLinear(1) = -direction.x;
      m_dConst = direction.x * point.y - direction.y * point.x;
      return;
    }
    RS_Vector center = (projection + point) * 0.5;
    double p = std::sqrt(p2);
    m_bIsQuadratic = true;
    m_bValid = true;
    m_mQuad(0,0) = 0.;
    m_mQuad(0,1) = 0.;
    m_mQuad(1,0) = 0.;
    m_mQuad(1,1) = 1.;
    m_vLinear(0) = -2. * p;
    m_vLinear(1) = 0.;
    m_dConst = 0.;
    rotate(center.angleTo(point));
    move(center);
    return;
  }
  default:
    m_bValid = false;
    return;
  }
}

/** construct a ellipse or hyperbola as the path of center of common tangent circles
  of this two given entities */
LC_Quadratic::LC_Quadratic(const RS_AtomicEntity* circle0,
                           const RS_AtomicEntity* circle1,
                           bool mirror)
    : m_mQuad(2,2)
      , m_vLinear(2)
      , m_bValid(false)
{
  if (!(circle0->isArcCircleLine() && circle1->isArcCircleLine())) {
    return;
  }

  if (circle1->rtti() != RS2::EntityLine)
    std::swap(circle0, circle1);

  if (circle0->rtti() == RS2::EntityLine) {
    // two lines
    auto* line0 = static_cast<const RS_Line*>(circle0);
    auto* line1 = static_cast<const RS_Line*>(circle1);
    auto centers = RS_Information::getIntersection(line0, line1);
    if (centers.size() != 1) return;
    double angle = 0.5 * (line0->getAngle1() + line1->getAngle1());
    m_bValid = true;
    m_bIsQuadratic = true;
    m_mQuad(0,0) = 0.;
    m_mQuad(0,1) = 0.5;
    m_mQuad(1,0) = 0.5;
    m_mQuad(1,1) = 0.;
    m_vLinear(0) = 0.;
    m_vLinear(1) = 0.;
    m_dConst = 0.;
    rotate(angle);
    move(centers.get(0));
    return;
  }

  if (circle1->rtti() == RS2::EntityLine) {
    // one line, one circle – dead code removed
    auto* line1 = static_cast<const RS_Line*>(circle1);
    RS_Vector normal = line1->getNormalVector() * circle0->getRadius();
    RS_Vector disp = line1->getNearestPointOnEntity(circle0->getCenter(), false)
                     - circle0->getCenter();
    if (normal.dotP(disp) > 0.) normal *= -1.;
    if (mirror) normal *= -1.;

    RS_Line directrix{line1->getStartpoint() + normal,
                      line1->getEndpoint() + normal};
    LC_Quadratic lc0(&directrix, circle0->getCenter());
    *this = lc0;
    return;
  }

         // two circles
  double const f = (circle0->getCenter() - circle1->getCenter()).magnitude() * 0.5;
  double const a = std::abs(circle0->getRadius() + circle1->getRadius()) * 0.5;
  double const c = std::abs(circle0->getRadius() - circle1->getRadius()) * 0.5;
  if (a < RS_TOLERANCE) return;

  RS_Vector center = (circle0->getCenter() + circle1->getCenter()) * 0.5;
  double angle = center.angleTo(circle0->getCenter());

  if (f < a) {
    // ellipse
    double const ratio = std::sqrt(a * a - f * f) / a;
    RS_Vector const& majorP = RS_Vector{angle} * a;
    RS_Ellipse const ellipse{nullptr, {center, majorP, ratio, 0., 0., false}};
    auto const& lc0 = ellipse.getQuadratic();
    m_mQuad = lc0.getQuad();
    m_vLinear = lc0.getLinear();
    m_bIsQuadratic = lc0.isQuadratic();
    m_bValid = lc0.isValid();
    m_dConst = lc0.m_dConst;
    return;
  }

  if (c < RS_TOLERANCE) {
    // degenerate hyperbola: straight lines (xy = 0)
    m_bValid = true;
    m_bIsQuadratic = true;
    m_mQuad(0,0) = 0.;
    m_mQuad(0,1) = 0.5;
    m_mQuad(1,0) = 0.5;
    m_mQuad(1,1) = 0.;
    m_vLinear(0) = 0.;
    m_vLinear(1) = 0.;
    m_dConst = 0.;
    rotate(angle);
    move(center);
    return;
  }

         // hyperbola
  double b2 = f * f - c * c;
  m_bValid = true;
  m_bIsQuadratic = true;
  m_mQuad(0,0) = 1. / (c * c);
  m_mQuad(0,1) = 0.;
  m_mQuad(1,0) = 0.;
  m_mQuad(1,1) = -1. / b2;
  m_vLinear(0) = 0.;
  m_vLinear(1) = 0.;
  m_dConst = -1.;
  rotate(angle);
  move(center);
}

/**
 * @brief LC_Quadratic, construct a Perpendicular bisector line
 */
LC_Quadratic::LC_Quadratic(const RS_Vector& point0, const RS_Vector& point1)
{
  RS_Vector vStart = (point0 + point1) * 0.5;
  RS_Vector vEnd = vStart + (point0 - vStart).rotate(0.5 * M_PI);
  *this = RS_Line(vStart, vEnd).getQuadratic();
}

std::vector<double> LC_Quadratic::getCoefficients() const
{
  std::vector<double> ret;
  if (!isValid()) return ret;
  if (m_bIsQuadratic) {
    ret.push_back(m_mQuad(0,0));
    ret.push_back(m_mQuad(0,1) + m_mQuad(1,0));
    ret.push_back(m_mQuad(1,1));
  }
  ret.push_back(m_vLinear(0));
  ret.push_back(m_vLinear(1));
  ret.push_back(m_dConst);
  return ret;
}

/**
 * Translates the quadratic / line by the given offset vector.
 */
LC_Quadratic& LC_Quadratic::move(const RS_Vector& offset)
{
  if (!isValid()) return *this;
  const double dx = offset.x;
  const double dy = offset.y;

  if (isQuadratic()) {
    const double D = getD();
    const double E = getE();
    m_vLinear(0) -= 2.0 * getA() * dx + getB() * dy;
    m_vLinear(1) -= getB() * dx + 2.0 * getC() * dy;
    m_dConst += getA() * dx * dx + getB() * dx * dy + getC() * dy * dy - D * dx - E * dy;
  } else {
    m_dConst -= getD() * dx + getE() * dy;
  }
  return *this;
}

/**
 * Non-uniform scaling from the given center.
 */
LC_Quadratic& LC_Quadratic::scale(const RS_Vector& center, const RS_Vector& factor)
{
  if (!isValid() || factor.magnitude() < RS_TOLERANCE) return *this;
  double sx = factor.x;
  double sy = factor.y;
  if (std::abs(sx) < RS_TOLERANCE || std::abs(sy) < RS_TOLERANCE) {
    m_bValid = false;
    return *this;
  }
  move(-center);
  if (isQuadratic()) {
    double A = getA(), B = getB(), C = getC();
    double D = getD(), E = getE();
    m_mQuad(0,0) = A / (sx * sx);
    m_mQuad(0,1) = B / (sx * sy) / 2.0;
    m_mQuad(1,1) = C / (sy * sy);
    m_vLinear(0) = D / (sx * sx);
    m_vLinear(1) = E / (sy * sy);
  } else {
    m_vLinear(0) /= sx;
    m_vLinear(1) /= sy;
  }
  move(center);
  return *this;
}

LC_Quadratic& LC_Quadratic::rotate(double angle)
{
  if (std::abs(angle) < RS_TOLERANCE) return *this;
  using namespace boost::numeric::ublas;
  matrix<double> R = rotationMatrix(angle);
  matrix<double> Rt = trans(R);
  m_vLinear = prod(Rt, m_vLinear);
  if (m_bIsQuadratic) {
    m_mQuad = prod(m_mQuad, R);
    m_mQuad = prod(Rt, m_mQuad);
  }
  return *this;
}

LC_Quadratic& LC_Quadratic::rotate(const RS_Vector& center, double angle)
{
  if (!isValid()) return *this;
  move(-center);
  rotate(angle);
  move(center);
  return *this;
}

LC_Quadratic& LC_Quadratic::shear(double k)
{
  if (isQuadratic()) {
    std::vector<double> ce = getCoefficients();
    const std::vector<double> sheared = {
        ce[0], -2.0 * k * ce[0] + ce[1], k * (k * ce[0] - ce[1]) + ce[2],
        ce[3], ce[4] - k * ce[3], ce[5]
    };
    *this = LC_Quadratic(sheared);
    return *this;
  }
  m_vLinear(1) -= k * m_vLinear(0);
  return *this;
}

/** switch x,y coordinates */
LC_Quadratic LC_Quadratic::flipXY() const
{
  LC_Quadratic qf(*this);
  if (isQuadratic()) {
    std::swap(qf.m_mQuad(0,0), qf.m_mQuad(1,1));
    std::swap(qf.m_mQuad(0,1), qf.m_mQuad(1,0));
  }
  std::swap(qf.m_vLinear(0), qf.m_vLinear(1));
  return qf;
}

/** Mirror over line defined by two points */
LC_Quadratic& LC_Quadratic::mirror(const RS_Vector& p1, const RS_Vector& p2)
{
  if (!isValid() || (p2 - p1).squared() < RS_TOLERANCE2) return *this;
  RS_Vector dir = p2 - p1;
  dir.normalize();
  move(-p1);
  rotate(-dir.angle());
  if (isQuadratic()) {
    m_mQuad(0,1) = -m_mQuad(0,1);
    m_vLinear(1) = -m_vLinear(1);
  } else {
    m_vLinear(1) = -m_vLinear(1);
  }
  rotate(dir.angle());
  move(p1);
  return *this;
}

/** Mirror over an existing RS_Line */
LC_Quadratic& LC_Quadratic::mirror(const RS_Line& axis)
{
  return mirror(axis.getStartpoint(), axis.getEndpoint());
}

bool LC_Quadratic::isQuadratic() const
{
  if (m_mQuad.size1() == 2 && m_mQuad.size2() == 2) {
    if (RS_Math::equal(m_mQuad(0,0), 0.)
        && RS_Math::equal(m_mQuad(0,1), 0.)
        && RS_Math::equal(m_mQuad(1,0), 0.)
        && RS_Math::equal(m_mQuad(1,1), 0.))
      return false;
  }
  return m_bIsQuadratic;
}

LC_Quadratic::operator bool() const { return m_bValid; }
bool LC_Quadratic::isValid() const { return m_bValid; }
void LC_Quadratic::setValid(bool value) { m_bValid = value; }
bool LC_Quadratic::operator==(bool valid) const { return m_bValid == valid; }
bool LC_Quadratic::operator!=(bool valid) const { return m_bValid != valid; }

boost::numeric::ublas::vector<double>& LC_Quadratic::getLinear() { return m_vLinear; }
const boost::numeric::ublas::vector<double>& LC_Quadratic::getLinear() const { return m_vLinear; }
boost::numeric::ublas::matrix<double>& LC_Quadratic::getQuad() { return m_mQuad; }
const boost::numeric::ublas::matrix<double>& LC_Quadratic::getQuad() const { return m_mQuad; }
double LC_Quadratic::constTerm() const { return m_dConst; }
double& LC_Quadratic::constTerm() { return m_dConst; }

double LC_Quadratic::evaluateAt(const RS_Vector& p) const
{
  if (!p.valid) return 0.0;
  double x = p.x, y = p.y;
  return m_mQuad(0,0) * x * x +
         2.0 * m_mQuad(0,1) * x * y +
         m_mQuad(1,1) * y * y +
         m_vLinear(0) * x +
         m_vLinear(1) * y +
         m_dConst;
}

RS_Entity* LC_Quadratic::toEntity() const
{
  using namespace LC_QuadraticUtils;
  if (!isValid()) return nullptr;
  if (!isQuadratic())
    return createLineFromLinearCoefficients(*this);
  RS_Vector center = computeCenter(*this);
  double v = evaluateAt(center);
  if (isDegenerate(*this))
    return createDegeneratePointOrIntersecting(*this, center, v);
  double theta = computeRotationAngle(*this);
  auto [Ap, Cp] = computePrincipalCoefficients(*this, theta);
  if (std::abs(Ap) < RS_TOLERANCE || std::abs(Cp) < RS_TOLERANCE)
    return nullptr;
  double disc = computeDiscriminant(*this);
  if (disc < 0)
    return createEllipseOrCircle(*this, center, v, Ap, Cp, theta);
  if (isParabolaCondition(disc, *this))
    return createParabola(*this, center, v, Ap, Cp);
  return createHyperbola(*this, center, v, Ap, Cp, theta);
}

/** RAII version of toEntity() */
std::unique_ptr<RS_Entity> LC_Quadratic::toEntityUnique() const
{
  return std::unique_ptr<RS_Entity>(toEntity());
}

RS_VectorSolutions LC_Quadratic::getIntersection(const LC_Quadratic& l1, const LC_Quadratic& l2)
{
  RS_VectorSolutions ret;
  if (l1 == false || l2 == false) {
    return ret;
  }
  auto p1 = &l1;
  auto p2 = &l2;
  if (p1->isQuadratic() == false) {
    std::swap(p1, p2);
  }
  if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
    DEBUG_HEADER
            std::cout << *p1 << std::endl;
    std::cout << *p2 << std::endl;
  }
  if (!p1->isQuadratic()) {
    // two lines
    std::vector<std::vector<double> > ce(2, std::vector<double>(3, 0.));
    ce[0][0] = p1->m_vLinear(0);
    ce[0][1] = p1->m_vLinear(1);
    ce[0][2] = -p1->m_dConst;
    ce[1][0] = p2->m_vLinear(0);
    ce[1][1] = p2->m_vLinear(1);
    ce[1][2] = -p2->m_dConst;
    std::vector<double> sn(2, 0.);
    if (RS_Math::linearSolver(ce, sn)) {
      ret.push_back(RS_Vector(sn[0], sn[1]));
    }
    return ret;
  }
  if (!p2->isQuadratic()) {
    // one line, one quadratic
    if (std::abs(p2->m_vLinear(0)) + DBL_EPSILON < std::abs(p2->m_vLinear(1))) {
      ret = getIntersection(p1->flipXY(), p2->flipXY()).flipXY();
      return ret;
    }
    std::vector<std::vector<double> > ce;
    if (std::abs(p2->m_vLinear(1)) < RS_TOLERANCE) {
      const double angle = 0.25 * M_PI;
      LC_Quadratic p11(*p1);
      LC_Quadratic p22(*p2);
      ce.push_back(p11.rotate(angle).getCoefficients());
      ce.push_back(p22.rotate(angle).getCoefficients());
      ret = RS_Math::simultaneousQuadraticSolverMixed(ce);
      ret.rotate(-angle);
      return ret;
    }
    ce.push_back(p1->getCoefficients());
    ce.push_back(p2->getCoefficients());
    ret = RS_Math::simultaneousQuadraticSolverMixed(ce);
    return ret;
  }
  if (std::abs(p1->m_mQuad(0,0)) < RS_TOLERANCE && std::abs(p1->m_mQuad(0,1)) < RS_TOLERANCE &&
      std::abs(p2->m_mQuad(0,0)) < RS_TOLERANCE && std::abs(p2->m_mQuad(0,1)) < RS_TOLERANCE) {
    if (std::abs(p1->m_mQuad(1,1)) < RS_TOLERANCE && std::abs(p2->m_mQuad(1,1)) < RS_TOLERANCE) {
      std::vector<double> ce;
      ce.push_back(p1->m_vLinear(0));
      ce.push_back(p1->m_vLinear(1));
      ce.push_back(p1->m_dConst);
      LC_Quadratic lc10(ce);
      ce.clear();
      ce.push_back(p2->m_vLinear(0));
      ce.push_back(p2->m_vLinear(1));
      ce.push_back(p2->m_dConst);
      LC_Quadratic lc11(ce);
      return getIntersection(lc10, lc11);
    }
    return getIntersection(p1->flipXY(), p2->flipXY()).flipXY();
  }
  // radical axis reduction for numerical stability
  {
    const auto c1 = p1->getCoefficients();
    const auto c2 = p2->getCoefficients();
    const double quadScale = std::max({std::abs(c1[0]), std::abs(c1[2]),
                                       std::abs(c2[0]), std::abs(c2[2]), 1.});
    double t = 0.;
    bool canReduce = false;
    if (std::abs(c1[0]) > RS_TOLERANCE * quadScale) {
      t = c2[0] / c1[0];
      canReduce = std::abs(c2[1] - t * c1[1]) <= RS_TOLERANCE * quadScale
                  && std::abs(c2[2] - t * c1[2]) <= RS_TOLERANCE * quadScale;
    }
    if (!canReduce && std::abs(c1[2]) > RS_TOLERANCE * quadScale) {
      t = c2[2] / c1[2];
      canReduce = std::abs(c2[0] - t * c1[0]) <= RS_TOLERANCE * quadScale
                  && std::abs(c2[1] - t * c1[1]) <= RS_TOLERANCE * quadScale;
    }
    if (canReduce) {
      std::vector<double> lineCoeffs = {
          c2[3] - t * c1[3],
          c2[4] - t * c1[4],
          c2[5] - t * c1[5]
      };
      const double lineScale = std::max({std::abs(lineCoeffs[0]),
                                         std::abs(lineCoeffs[1]),
                                         std::abs(lineCoeffs[2])});
      if (lineScale > RS_TOLERANCE) {
        LC_Quadratic radicalAxis(lineCoeffs);
        return getIntersection(*p1, radicalAxis);
      }
    }
  }

  std::vector<std::vector<double> > ce = {p1->getCoefficients(), p2->getCoefficients()};
  auto sol = RS_Math::simultaneousQuadraticSolverFull(ce);
  bool valid = sol.size() > 0;
  for (auto& v : sol) {
    if (v.magnitude() >= RS_MAXDOUBLE) {
      valid = false;
      break;
    }
    const std::vector<double> xyi = {v.x * v.x, v.x * v.y, v.y * v.y, v.x, v.y, 1.};
    const double e0 = std::inner_product(xyi.cbegin(), xyi.cend(), ce.front().cbegin(), 0.);
    const double e1 = std::inner_product(xyi.cbegin(), xyi.cend(), ce.back().cbegin(), 0.);
    LC_LOG << __func__ << "(): " << v.x << "," << v.y << ": equ0= " << e0;
    LC_LOG << __func__ << "(): " << v.x << "," << v.y << ": equ1= " << e1;
  }
  if (valid) return sol;

  ce.clear();
  ce.push_back(p1->getCoefficients());
  ce.push_back(p2->getCoefficients());
  sol = RS_Math::simultaneousQuadraticSolverFull(ce);
  ret.clear();
  for (auto const& v : sol) {
    if (v.magnitude() <= RS_MAXDOUBLE) {
      ret.push_back(v);
      if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        DEBUG_HEADER
                std::cout << v << std::endl;
      }
    }
  }
  return ret;
}

/**
 * rotation matrix
 */
boost::numeric::ublas::matrix<double> LC_Quadratic::rotationMatrix(double angle)
{
  boost::numeric::ublas::matrix<double> ret(2,2);
  ret(0,0) = cos(angle);
  ret(0,1) = sin(angle);
  ret(1,0) = -ret(0,1);
  ret(1,1) = ret(0,0);
  return ret;
}

/**
 * Fixed getDualCurve – now normalizes constant term to +1 (u x + v y + 1 = 0)
 */
LC_Quadratic LC_Quadratic::getDualCurve() const
{
  if (!isValid()) return {};
  if (!isQuadratic()) {
    double a = m_vLinear(0);
    double b = m_vLinear(1);
    double c = m_dConst;
    return LC_Quadratic({a*a, 2*a*b, b*b, 2*a*c, 2*b*c, c*c}).getDualCurve();
  }

  std::vector<double> primal = getCoefficients();
  double A = primal[0], B = primal[1], C = primal[2],
      D = primal[3], E = primal[4], F = primal[5];

  double Ap = 4 * C * F - E * E;
  double Bp = 2 * D * E - 4 * B * F;
  double Cp = 4 * A * F - D * D;
  double Dp = 2 * B * E - 4 * C * D;
  double Ep = 2 * B * D - 4 * A * E;
  double Fp = 4 * A * C - B * B;

  if (std::abs(Fp) < RS_TOLERANCE) return {}; // degenerate

  double scale = 1.0 / Fp;
  return LC_Quadratic({Ap*scale, Bp*scale, Cp*scale, Dp*scale, Ep*scale, 1.0});
}

/**
 * Dumps the quadratic to stdout.
 */
std::ostream& operator<<(std::ostream& os, const LC_Quadratic& q)
{
  os << " quadratic form: ";
  if (!q) {
    os << "invalid quadratic form" << std::endl;
    return os;
  }
  os << std::endl;
  auto ce = q.getCoefficients();
  unsigned short i = 0;
  if (ce.size() == 6) {
    os << ce[0] << "*x^2 " << ((ce[1] >= 0.) ? "+" : " ") << ce[1] << "*x*y "
       << ((ce[2] >= 0.) ? "+" : " ") << ce[2] << "*y^2 ";
    i = 3;
  }
  if (q.isQuadratic() && ce[i] >= 0.) os << "+";
  os << ce[i] << "*x " << ((ce[i+1] >= 0.) ? "+" : " ") << ce[i+1] << "*y "
     << ((ce[i+2] >= 0.) ? "+" : " ") << ce[i+2] << " == 0" << std::endl;
  return os;
}
//EOF