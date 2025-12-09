/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD.org
** Copyright (C) 2025 Dongxu Li github.com/dxli
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
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
******************************************************************************/
// File: lc_hyperbola.cpp

#include <cmath>

#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_painter.h"

using std::cosh, std::sinh, std::atan2, std::cos, std::sin, std::atan;

//=====================================================================
// Construction
//=====================================================================

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : center((f0 + f1)*0.5)
{
  double d = f0.distanceTo(p) - f1.distanceTo(p);
  majorP = (d > 0 ? f0 - center : f1 - center);
  double dc = f0.distanceTo(f1), dd = std::abs(d);
  if (dc < RS_TOLERANCE || dd < RS_TOLERANCE) { majorP = {}; return; }
  ratio = dc / dd;
  majorP /= ratio;
  ratio = std::sqrt(ratio*ratio - 1.0);
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d)
    : RS_AtomicEntity(parent)
      , data(d)
      , m_bValid(d.majorP.squared() >= RS_TOLERANCE2) {
  LC_Hyperbola::calculateBorders();
}

LC_Hyperbola::LC_Hyperbola(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p))
{}

//=====================================================================
// Core Geometry
//=====================================================================

RS_Vector LC_Hyperbola::getPointExact(double phi) const {
  if (!m_bValid) return {};
  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE) return {};

         // Safe handling for very large |phi| (prevent NaN/inf)
  if (std::abs(phi) > 700.0) {  // Near double overflow limit
    double sign = data.reversed ? -1.0 : 1.0;
    double exp_phi = std::exp(phi);
    double exp_neg = std::exp(-phi);
    double ch = 0.5 * (exp_phi + exp_neg);
    double sh = 0.5 * (exp_phi - exp_neg);
    RS_Vector p(a * sign * ch, b * sh);
    p.rotate(getAngle());
    return data.center + p;
  }

  double ch = std::cosh(phi);
  double sh = std::sinh(phi);
  RS_Vector p(a * (data.reversed ? -ch : ch), b * sh);
  p.rotate(getAngle());
  return data.center + p;
}

RS_Vector LC_Hyperbola::getPoint(double phi, bool rev) const {
  return getPointExact(rev ? phi + M_PI : phi);
}

RS_VectorSolutions LC_Hyperbola::getFoci() const {
  if (!m_bValid) return {};
  double e = std::sqrt(1.0 + data.ratio*data.ratio);
  RS_Vector v = data.majorP * e;
  return {data.center + v, data.center - v};
}

double LC_Hyperbola::getLength() const {
  if (!m_bValid) return 0.0;
  double a = getMajorRadius(), e = std::sqrt(1.0 + data.ratio*data.ratio);
  double p1 = data.angle1 + (data.reversed ? M_PI : 0.0);
  double p2 = data.angle2 + (data.reversed ? M_PI : 0.0);
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) return INFINITY;
  if (p2 < p1) std::swap(p1, p2);
  auto L = [a,e](double t){ return a*(e*sinh(t) + t); };
  return L(p2) - L(p1);
}

RS_Vector LC_Hyperbola::getMiddlePoint() const {
  double L = getLength();
  return std::isfinite(L) && L > RS_TOLERANCE ? pointAtDistance(L*0.5) : RS_Vector(false);
}

RS_Vector LC_Hyperbola::pointAtDistance(double d) const {
  if (d <= 0.0) return getStartpoint();
  double L = getLength(); if (!std::isfinite(L) || d >= L) return getEndpoint();
  double a = getMajorRadius(), e = std::sqrt(1.0 + data.ratio*data.ratio);
  double p0 = data.reversed ? data.angle1 + M_PI : data.angle1, phi = p0;
  for (int i = 0; i < 12; ++i)
    phi -= (a*(e*sinh(phi) + phi) - a*(e*sinh(p0) + p0) - d) / (a*(e*cosh(phi) + 1.0));
  return getPointExact(phi);
}

double LC_Hyperbola::getParamFromPoint(const RS_Vector& p, bool rev) const {
  if (!p.valid || !m_bValid) return NAN;
  RS_Vector d = (p - data.center).rotate(-getAngle());
  double b = getMinorRadius();
  if (b < RS_TOLERANCE) return NAN;
  double phi = std::atanh(d.y / b);
  return rev ? phi + M_PI : phi;
}

//=====================================================================
// Snapping & Geometry
//=====================================================================

RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector& c, bool onEntity,
                                                double* dist, RS_Entity** ent) const {
  if (!m_bValid) return {};
  RS_Vector r = (c - data.center).rotate(-getAngle());
  double a = getMajorRadius(), b = getMinorRadius();
  double phi = atan2(r.y/b, r.x/a) + (data.reversed ? M_PI : 0.0);
  for (int i = 0; i < 20; ++i) {
    double ch = cosh(phi), sh = sinh(phi);
    RS_Vector p(a*(data.reversed ? -ch : ch), b*sh);
    RS_Vector dr = r - p;
    phi += (dr.x*a*sh + dr.y*b*ch) / (a*a*ch*ch + b*b*sh*sh + 1e-12);
  }
  RS_Vector nearest = getPointExact(phi);
  if (onEntity && !isPointOnEntity(nearest)) {
    auto s = getStartpoint(), e = getEndpoint();
    nearest = (!e.valid || c.distanceTo(s) < c.distanceTo(e)) ? s : e;
  }
  if (dist) *dist = c.distanceTo(nearest);
  if (ent) *ent = const_cast<LC_Hyperbola*>(this);
  return nearest;
}

RS_Vector LC_Hyperbola::getNearestCenter(const RS_Vector& c, double* d) const {
  if (d) *d = c.distanceTo(data.center);
  return data.center;
}

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector& c, double* d) const {
  RS_Vector s = getStartpoint(), e = getEndpoint();
  if (!s.valid) return e;
  if (!e.valid) return s;
  RS_Vector n = (c.distanceTo(s) < c.distanceTo(e)) ? s : e;
  if (d) *d = c.distanceTo(n);
  return n;
}

double LC_Hyperbola::getDistanceToPoint(const RS_Vector& c, RS_Entity** ent,
                                        RS2::ResolveLevel, double) const {
  double d; getNearestPointOnEntity(c, true, &d, ent); return d;
}

bool LC_Hyperbola::isPointOnEntity(const RS_Vector& c, double tol) const {
  return getDistanceToPoint(c) <= tol;
}

RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector& point) const
{
  RS_VectorSolutions sol;

  if (!m_bValid || !point.valid)
    return sol;

  LC_Quadratic hyperbola = getQuadratic();
  if (!hyperbola.isValid())
    return sol;

  std::vector<double> pointCoeffs = {
      0.0, 0.0, 0.0,
      -2.0 * point.x,
      -2.0 * point.y,
      point.x*point.x + point.y*point.y
  };
  LC_Quadratic pointConic(pointCoeffs);

  LC_Quadratic dualConic = hyperbola.getDualCurve();
  if (!dualConic.isValid())
    return sol;

  RS_VectorSolutions linePairPoints = LC_Quadratic::getIntersection(dualConic, pointConic);
  if (linePairPoints.getNumber() < 2)
    return sol;

  for (int i = 0; i < linePairPoints.getNumber(); i += 2) {
    RS_Vector p1 = linePairPoints.get(i);
    RS_Vector p2 = linePairPoints.get(i + 1);
    if (!p1.valid || !p2.valid)
      continue;

    RS_Line tangentLine(nullptr, RS_LineData(p1, p2));
    RS_VectorSolutions touchPoints = RS_Information::getIntersection(&tangentLine, this, true);

    for (const RS_Vector& tp : touchPoints) {
      if (tp.valid && isPointOnEntity(tp, RS_TOLERANCE * 10)) {
        // Manual duplicate check
        bool duplicate = false;
        for (const RS_Vector& existing : sol) {
          if (existing.distanceTo(tp) < RS_TOLERANCE) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate)
          sol.push_back(tp);
      }
    }
  }

  return sol;
}

RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector& point) const {
  if (!m_bValid || !point.valid) return {};
  double phi = getParamFromPoint(point, data.reversed);
  if (std::isnan(phi)) return {};
  double a = getMajorRadius(), b = getMinorRadius();
  double ch = cosh(phi), sh = sinh(phi);
  RS_Vector t(a * sh * (data.reversed ? -1 : 1), b * ch);
  t.rotate(getAngle());
  return t.normalized();
}

RS_VectorSolutions LC_Hyperbola::getRefPoints() const {
  RS_VectorSolutions s;
  s.push_back(data.center);
  s.push_back(getFoci().get(0));
  s.push_back(getFoci().get(1));
  RS_Vector sp = getStartpoint(), ep = getEndpoint();
  if (sp.valid) s.push_back(sp);
  if (ep.valid) s.push_back(ep);
  return s;
}

//=====================================================================
// Trimming & Transformations
//=====================================================================

RS2::Ending LC_Hyperbola::getTrimPoint(const RS_Vector& tc, const RS_Vector& tp) {
  RS_Vector s = getStartpoint(), e = getEndpoint();
  if (tp.distanceTo(s) < RS_TOLERANCE) return RS2::EndingStart;
  if (tp.distanceTo(e) < RS_TOLERANCE) return RS2::EndingEnd;
  return tc.distanceTo(s) < tc.distanceTo(e) ? RS2::EndingStart : RS2::EndingEnd;
}

RS_Vector LC_Hyperbola::prepareTrim(const RS_Vector& tc, const RS_VectorSolutions& sol) {
  if (sol.empty()) return {};
  RS_Vector best = sol.getClosest(tc);
  RS_Vector dir = best - tc;
  for (auto& p : sol)
    if (RS_Vector::dotP(p - tc, dir) > RS_TOLERANCE) return p;
  return best;
}

void LC_Hyperbola::move(const RS_Vector& o) { data.center += o; calculateBorders(); }
void LC_Hyperbola::rotate(const RS_Vector& c, double a) { data.center.rotate(c, a); data.majorP.rotate(a); calculateBorders(); }
void LC_Hyperbola::rotate(const RS_Vector& c, const RS_Vector& av) { rotate(c, av.angle()); }

void LC_Hyperbola::scale(const RS_Vector& c, const RS_Vector& f) {
  data.center.scale(c, f); data.majorP.scale(f); data.ratio *= std::abs(f.y/f.x);
  calculateBorders();
}

RS_Entity& LC_Hyperbola::shear(double k) {
  if (!m_bValid || std::abs(k) < RS_TOLERANCE) return *this;
  double ang = getAngle(); RS_Vector saved = data.center;
  data.center = {}; data.majorP.rotate(-ang);
  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE) { data.center = saved; data.majorP.rotate(ang); data.center.shear(k); data.majorP.shear(k); calculateBorders(); return *this; }
  double A = 1/(a*a), B = 1/(b*b);
  double x2 = A, xy = 2*k*A, y2 = k*k*A - B;
  double delta = x2 - y2;
  double theta = (std::abs(delta) < 1e-12) ? (xy > 0 ? M_PI/4 : -M_PI/4) : 0.5*std::atan(xy/delta);
  double ct = cos(theta), st = sin(theta);
  double Ap = x2*ct*ct + xy*ct*st + y2*st*st;
  double Cp = x2*st*st - xy*ct*st + y2*ct*ct;
  if (Ap <= 0 || Cp >= 0) { data.center = saved; data.majorP.rotate(ang); data.center.shear(k); data.majorP.shear(k); calculateBorders(); return *this; }
  data.majorP = RS_Vector(std::sqrt(1/Ap), 0);
  data.ratio = std::sqrt(-1/Cp) / data.majorP.x;
  data.majorP.rotate(theta + ang);
  data.center = saved; data.center.shear(k);
  if (data.angle1 || data.angle2) {
    RS_Vector s = getStartpoint(), e = getEndpoint();
    if (s.valid) data.angle1 = getParamFromPoint(s, data.reversed);
    if (e.valid) data.angle2 = getParamFromPoint(e, data.reversed);
  }
  calculateBorders(); return *this;
}

void LC_Hyperbola::mirror(const RS_Vector& a1, const RS_Vector& a2) {
  data.center.mirror(a1, a2);
  data.majorP.mirror({}, a2 - a1);
  data.reversed = !data.reversed;
  std::swap(data.angle1, data.angle2);
  data.angle1 += M_PI; data.angle2 += M_PI;
  calculateBorders();
}

//=====================================================================
// Rendering & Area
//=====================================================================

void LC_Hyperbola::calculateBorders() {
  minV = maxV = data.center;
  if (!m_bValid) return;
  constexpr double phis[] = {-3,-2.5,-2,-1.5,-1,-0.5,0,0.5,1,1.5,2,2.5,3};
  for (double t : phis) {
    RS_Vector v = getPointExact(t + (data.reversed ? M_PI : 0));
    if (v.valid) {
      minV = RS_Vector::minimum(minV, v);
      maxV = RS_Vector::maximum(maxV, v);
    }
  }
  for (const RS_Vector& f : getFoci()) {
    if (f.valid) {
      minV = RS_Vector::minimum(minV, f);
      maxV = RS_Vector::maximum(maxV, f);
    }
  }
}

void LC_Hyperbola::draw(RS_Painter* p) {
  if (!m_bValid || !p) return;
  double L = getLength(), n = std::max(40.0, L/4.0);
  double p1 = data.angle1 + (data.reversed ? M_PI : 0);
  double p2 = data.angle2 + (data.reversed ? M_PI : 0);
  if (p2 < p1) std::swap(p1, p2);
  double s = (p2 - p1)/n;
  RS_Vector prev = getPointExact(p1);
  for (int i = 1; i <= int(n); ++i) {
    RS_Vector cur = getPointExact(p1 + i*s);
    if (prev.valid && cur.valid) p->drawLineWCS(prev, cur);
    prev = cur;
  }
}

double LC_Hyperbola::areaLineIntegral() const {
  if (!m_bValid) return 0.0;
  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE) return 0.0;
  double p1 = data.angle1 + (data.reversed ? M_PI : 0);
  double p2 = data.angle2 + (data.reversed ? M_PI : 0);
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) return 0.0;
  if (p2 < p1) std::swap(p1, p2);
  auto F = [a,b](double t){ return a*b*(0.5*t + 0.5*sinh(t)*cosh(t)); };
  double I = F(p2) - F(p1);
  if (getAngle() != 0.0) I *= cos(getAngle());
  if (data.reversed != (data.angle2 >= data.angle1)) I = -I;
  return I;
}

RS_Entity* LC_Hyperbola::clone() const { return new LC_Hyperbola(*this); }

LC_Quadratic LC_Hyperbola::getQuadratic() const {
  double m = data.majorP.squared();
  std::vector<double> c = {1.0/m, 0, -data.ratio*data.ratio/m, 0, 0, -1};
  return LC_Quadratic(c).rotate(getAngle()).move(data.center);
}

RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector& coord,
                                         double* dist,
                                         int middlePoints) const
{
  if (middlePoints < 1 || !m_bValid) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

  double L = getLength();
  if (!std::isfinite(L) || L < RS_TOLERANCE) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

         // Divide into middlePoints equal parts
  RS_Vector best(false);
  double bestDist = RS_MAXDOUBLE;

  for (int i = 1; i <= middlePoints; ++i) {
    double d = L * i / (middlePoints + 1.0);
    RS_Vector p = pointAtDistance(d);
    if (!p.valid) continue;
    double dd = coord.distanceTo(p);
    if (dd < bestDist) {
      bestDist = dd;
      best = p;
    }
  }

  if (dist) *dist = bestDist;
  return best;
}

RS_Vector LC_Hyperbola::getNearestDist(double distance,
                                       const RS_Vector& coord,
                                       double* dist) const
{
  RS_Vector p = pointAtDistance(distance);
  if (dist && p.valid) *dist = coord.distanceTo(p);
  return p;
}
