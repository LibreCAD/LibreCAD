/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 Dongxu Li (github.com/dxli)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/

#include <algorithm>
#include <cmath>
#include <vector>

#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

//=====================================================================
// Construction
//=====================================================================

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& c, const RS_Vector& m,
                                   double r, double a1, double a2, bool rev)
    : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev) {}

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : center((f0 + f1) * 0.5)
{
  double d = f0.distanceTo(p) - f1.distanceTo(p);
  majorP = (d > 0.0) ? (f0 - center) : (f1 - center);
  double dc = f0.distanceTo(f1);
  double dd = fabs(d);
  if (dc < RS_TOLERANCE || dd < RS_TOLERANCE) {
    majorP = RS_Vector(0, 0);
    return;
  }
  ratio = dc / dd;
  majorP /= ratio;
  ratio = sqrt(ratio * ratio - 1.0);
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d)
    : RS_AtomicEntity(parent), data(d), m_bValid(d.majorP.squared() >= RS_TOLERANCE2)
{
  calculateBorders();
}

LC_Hyperbola::LC_Hyperbola(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p)) {}

//=====================================================================
// Entity interface
//=====================================================================

RS_Entity* LC_Hyperbola::clone() const { return new LC_Hyperbola(*this); }

RS_VectorSolutions LC_Hyperbola::getFoci() const
{
  double e = sqrt(1.0 + data.ratio * data.ratio);
  RS_Vector vp = data.majorP * e;
  RS_VectorSolutions sol;
  sol.push_back(data.center + vp);
  sol.push_back(data.center - vp);
  return sol;
}

RS_VectorSolutions LC_Hyperbola::getRefPoints() const
{
  RS_VectorSolutions ret;
  ret.push_back(data.center);
  RS_VectorSolutions foci = getFoci();
  for (size_t i = 0; i < foci.getNumber(); ++i)
    ret.push_back(foci.get(i));
  return ret;
}

RS_Vector LC_Hyperbola::getStartpoint() const { return getPoint(data.angle1); }
RS_Vector LC_Hyperbola::getEndpoint() const   { return getPoint(data.angle2); }
RS_Vector LC_Hyperbola::getMiddlePoint() const { return RS_Vector(false); }

//=====================================================================
// Tangent methods
//=====================================================================

double LC_Hyperbola::getDirection1() const
{
  RS_Vector p = getStartpoint();
  if (!p.valid) return 0.0;
  return getTangentDirection(p).angle();
}

double LC_Hyperbola::getDirection2() const
{
  RS_Vector p = getEndpoint();
  if (!p.valid) return 0.0;
  return getTangentDirection(p).angle();
}

RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector& point) const
{
  if (!m_bValid || !point.valid) return RS_Vector(false);

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return RS_Vector(false);

  RS_Vector local = (point - data.center).rotate(-getAngle());
  double x = local.x, y = local.y;
  bool rev = data.reversed;
  if (rev) { x = -x; y = -y; }

  double phi = atan2(y * a, x * b);
  if (rev) phi += M_PI;

  double cp = cos(phi), sp = sin(phi);
  if (fabs(cp) < RS_TOLERANCE) return RS_Vector(false);

  double dx_dphi = a * sp / (cp * cp);
  double dy_dphi = b / cp;

  RS_Vector tangent_local(dx_dphi, dy_dphi);
  tangent_local.rotate(getAngle());
  return tangent_local.normalized();
}

RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector& point) const
{
  if (!m_bValid || !point.valid) return RS_VectorSolutions();

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  RS_Vector d = point - data.center;
  d.rotate(-getAngle());

  double px = d.x, py = d.y;
  double a2 = a*a, b2 = b*b;

  double A = px*px/a2 + py*py/b2 - 1.0;
  double B = -2.0 * (px/a2 + py*py/(b2*px));
  double C = 1.0/a2 + py*py/(b2*px*px);

  double disc = B*B - 4.0*A*C;
  if (disc < -RS_TOLERANCE) return RS_VectorSolutions();
  disc = std::max(0.0, disc);

  RS_VectorSolutions sol;
  double sqrtD = sqrt(disc);
  for (double sign : {-1.0, 1.0}) {
    double lambda = (-B + sign * sqrtD) / (2.0 * A);
    if (std::isnan(lambda) || std::isinf(lambda)) continue;

    double x = a2 / (px + lambda);
    double y = (b2 * py * x) / (a2 * px);

    RS_Vector local(x, y);
    local.rotate(getAngle());
    RS_Vector world = data.center + local;

    if (isPointOnEntity(world, RS_TOLERANCE * 10))
      sol.push_back(world);
  }
  return sol;
}

RS_Vector LC_Hyperbola::getNearestOrthTan(const RS_Vector& coord,
                                          const RS_Line& normal,
                                          bool onEntity) const
{
  if (!m_bValid) return RS_Vector(false);

  RS_Vector n = normal.getTangentDirection({});

  auto f = [&](double phi, bool rev) -> double {
    RS_Vector t = getTangentDirection(getPoint(phi, rev));
    return t.valid ? RS_Vector::dotP(t, n) : 1e10;
  };

  double phi = 0.0;
  for (int i = 0; i < 20; ++i) {
    double val = f(phi, data.reversed);
    if (fabs(val) < 1e-8) break;
    double eps = 1e-6;
    double deriv = (f(phi + eps, data.reversed) - f(phi - eps, data.reversed)) / (2.0 * eps);
    if (fabs(deriv) < 1e-10) break;
    phi -= val / deriv;
  }

  RS_Vector p = getPoint(phi);
  if (!p.valid || (onEntity && !isPointOnEntity(p))) return RS_Vector(false);
  return p;
}

//=====================================================================
// Rendering – analytic curvature adaptive sampling
//=====================================================================

void LC_Hyperbola::draw(RS_Painter* painter)
{
  if (!painter || !m_bValid) return;

  const LC_Rect& vp = painter->getWcsBoundingRect();
  const double tol = RS_TOLERANCE * 100.0;
  if (vp.isEmpty(tol) || vp.width() < tol || vp.height() < tol) {
    drawFullApproximation(painter);
    return;
  }

  const double xmin = vp.minP().x, xmax = vp.maxP().x;
  const double ymin = vp.minP().y, ymax = vp.maxP().y;

  const LC_Quadratic q = getQuadratic();
  if (!q.isValid()) return;

  const std::vector<Segment> vpSeg = {
      {{xmin,ymin},{xmax,ymin}}, {{xmin,ymax},{xmax,ymax}},
      {{xmin,ymin},{xmin,ymax}}, {{xmax,ymin},{xmax,ymax}}
  };

  const bool full = (fabs(data.angle1) < RS_TOLERANCE && fabs(data.angle2) < RS_TOLERANCE);
  const double alpha = atan(data.ratio);

  if (full) {
    drawClippedBranch(painter, q.getCoefficients(), vpSeg, xmin,xmax,ymin,ymax, -alpha, alpha, false);
    drawClippedBranch(painter, q.getCoefficients(), vpSeg, xmin,xmax,ymin,ymax, M_PI-alpha, M_PI+alpha, true);
  } else {
    double a1 = data.angle1, a2 = data.angle2;
    if (data.reversed) { a1 += M_PI; a2 += M_PI; }
    drawClippedBranch(painter, q.getCoefficients(), vpSeg, xmin,xmax,ymin,ymax,
                      std::min(a1,a2), std::max(a1,a2), data.reversed);
  }
}

void LC_Hyperbola::drawClippedBranch(RS_Painter* painter,
                                     const std::vector<double>& m,
                                     const std::vector<Segment>& vpSeg,
                                     double xmin, double xmax, double ymin, double ymax,
                                     double phiMin, double phiMax,
                                     bool branchReversed) const
{
  std::vector<double> phis = {phiMin, phiMax};

         // Intersect with viewport edges
  for (const auto& seg : vpSeg) {
    const RS_Vector d = seg.p2 - seg.p1;
    const double dx = d.x, dy = d.y;
    const double x0 = seg.p1.x, y0 = seg.p1.y;

    const double A = m[0]*dx*dx + m[1]*dx*dy + m[2]*dy*dy;
    const double B = 2.0*(m[0]*x0*dx + m[1]*(x0*dy + y0*dx) + m[2]*y0*dy) + m[3]*dx + m[4]*dy;
    const double C = m[0]*x0*x0 + m[1]*x0*y0 + m[2]*y0*y0 + m[3]*x0 + m[4]*y0 + m[5];

    if (fabs(A) < RS_TOLERANCE) {
      if (fabs(B) < RS_TOLERANCE) continue;
      const double s = -C/B;
      if (s >= -RS_TOLERANCE && s <= 1.0 + RS_TOLERANCE) {
        const RS_Vector ip = seg.p1 + d * std::clamp(s, 0.0, 1.0);
        const double phi = getParamFromPoint(ip, branchReversed);
        if (isValidPhi(phi, phiMin, phiMax)) phis.push_back(phi);
      }
      continue;
    }

    const double disc = std::max(0.0, B*B - 4.0*A*C);
    const double sd = sqrt(disc);
    for (double s : {(-B - sd)/(2.0*A), (-B + sd)/(2.0*A)}) {
      if (s >= -RS_TOLERANCE && s <= 1.0 + RS_TOLERANCE) {
        s = std::clamp(s, 0.0, 1.0);
        const RS_Vector ip = seg.p1 + d * s;
        const double phi = getParamFromPoint(ip, branchReversed);
        if (isValidPhi(phi, phiMin, phiMax)) phis.push_back(phi);
      }
    }
  }

  if (phis.size() < 3) samplePhis(phis, phiMin, phiMax, 12);
  std::sort(phis.begin(), phis.end());
  phis.erase(std::unique(phis.begin(), phis.end(),
                         [](double a,double b){return fabs(a-b)<2.0*RS_TOLERANCE;}), phis.end());

  for (size_t i = 0; i + 1 < phis.size(); ++i) {
    const double p1 = phis[i], p2 = phis[i+1];
    if (fabs(p2 - p1) < RS_TOLERANCE) continue;

    const RS_Vector mid = getPoint(0.5*(p1 + p2), branchReversed);
    if (!mid.valid || !isInClipRect(mid, xmin,xmax,ymin,ymax)) continue;

    drawSplineSegment(painter, p1, p2, branchReversed, xmin,xmax,ymin,ymax);
  }
}

void LC_Hyperbola::drawSplineSegment(RS_Painter* painter,
                                     double phiStart, double phiEnd,
                                     bool branchReversed,
                                     double xmin, double xmax, double ymin, double ymax) const
{
  if (!painter || fabs(phiEnd - phiStart) < RS_TOLERANCE) return;

  const double maxPixelError = 1.0;
  const double scaleX = fabs(painter->toGuiDX(1.0));
  const double scaleY = fabs(painter->toGuiDY(1.0));
  const double scale = std::min(scaleX, scaleY);
  if (scale < 1e-8) return;

  const double worldTol = maxPixelError / scale;

  const double a = getMajorRadius();
  const double b = getMinorRadius();

  auto curvatureAt = [&](double phi) -> double {
    if (branchReversed) phi += M_PI;
    phi = fmod(phi + 4.0*M_PI, 2.0*M_PI) - 2.0*M_PI;
    double cp = cos(phi), sp = sin(phi);
    double denom = b*b*cp*cp + a*a*sp*sp;
    if (denom < RS_TOLERANCE2) return 1e10;
    return (a * b) / pow(denom, 1.5);
  };

  double maxK = 0.0;
  for (int i = 0; i <= 50; ++i) {
    double t = i / 50.0;
    double k = curvatureAt(phiStart + t * (phiEnd - phiStart));
    if (k > maxK) maxK = k;
  }

  double Lmax = (maxK > 1e-10) ? cbrt(24.0 * worldTol / maxK) : 1e6;
  double arcLen = segmentLength(phiStart, phiEnd, branchReversed, 100);
  int samples = std::max(4, std::min(256, static_cast<int>(ceil(arcLen / Lmax)) + 1));

  std::vector<RS_Vector> pts;
  const double delta = (phiEnd - phiStart) / (samples - 1);
  for (int i = 0; i < samples; ++i) {
    RS_Vector p = getPoint(phiStart + i * delta, branchReversed);
    if (p.valid && isInClipRect(p, xmin,xmax,ymin,ymax))
      pts.push_back(p);
  }
  if (pts.size() >= 2)
    painter->drawSplinePointsWCS(pts, false);
}

void LC_Hyperbola::drawFullApproximation(RS_Painter* painter)
{
  const double a = atan(data.ratio);
  drawSplineSegment(painter, -a, a, false, 0,0,0,0);
  drawSplineSegment(painter, M_PI-a, M_PI+a, true, 0,0,0,0);
}

//=====================================================================
// Arc length & point at distance
//=====================================================================

double LC_Hyperbola::segmentLength(double phiStart, double phiEnd, bool branchReversed, int samples) const
{
  if (samples < 2 || fabs(phiEnd - phiStart) < RS_TOLERANCE) return 0.0;
  double len = 0.0;
  const double delta = (phiEnd - phiStart) / (samples - 1);
  RS_Vector prev = getPoint(phiStart, branchReversed);
  for (int i = 1; i < samples; ++i) {
    RS_Vector cur = getPoint(phiStart + i * delta, branchReversed);
    if (prev.valid && cur.valid) len += prev.distanceTo(cur);
    prev = cur;
  }
  return len;
}

double LC_Hyperbola::getLength() const
{
  if (!m_bValid || getMajorRadius() < RS_TOLERANCE) return 0.0;

  const int samples = 1000;

  if (fabs(data.angle1) < RS_TOLERANCE && fabs(data.angle2) < RS_TOLERANCE) {
    const double a = atan(data.ratio);
    return segmentLength(-a, a, false, samples) +
           segmentLength(M_PI - a, M_PI + a, true, samples);
  }

  double a1 = data.angle1, a2 = data.angle2;
  if (data.reversed) { a1 += M_PI; a2 += M_PI; }
  return segmentLength(std::min(a1,a2), std::max(a1,a2), data.reversed, samples);
}

RS_Vector LC_Hyperbola::pointAtDistance(double distance) const
{
  if (!m_bValid || distance <= 0.0) return RS_Vector(false);
  double L = getLength();
  if (distance >= L) return RS_Vector(false);

  const int samples = 1000;

  auto scan = [&](double start, double end, bool rev) -> RS_Vector {
    const double delta = (end - start) / (samples - 1);
    double curDist = 0.0;
    RS_Vector prev = getPoint(start, rev);

    for (int i = 1; i < samples; ++i) {
      double phi = start + i * delta;
      RS_Vector cur = getPoint(phi, rev);
      if (!prev.valid || !cur.valid) { prev = cur; continue; }
      double seg = prev.distanceTo(cur);
      if (curDist + seg >= distance) {
        double t = (distance - curDist) / seg;
        return prev + (cur - prev) * t;
      }
      curDist += seg;
      prev = cur;
    }
    return RS_Vector(false);
  };

  if (fabs(data.angle1) < RS_TOLERANCE && fabs(data.angle2) < RS_TOLERANCE) {
    const double a = atan(data.ratio);
    RS_Vector p = scan(-a, a, false);
    if (p.valid) return p;
    return scan(M_PI - a, M_PI + a, true);
  }

  double a1 = data.angle1, a2 = data.angle2;
  if (data.reversed) { a1 += M_PI; a2 += M_PI; }
  return scan(std::min(a1,a2), std::max(a1,a2), data.reversed);
}

RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector& coord,
                                         double* dist,
                                         int middlePoints) const
{
  if (!m_bValid || middlePoints < 1) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }
  double L = getLength();
  if (L < RS_TOLERANCE) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }
  RS_Vector p = pointAtDistance(L * 0.5 / middlePoints);
  if (dist && p.valid) *dist = p.distanceTo(coord);
  return p;
}

RS_Vector LC_Hyperbola::getNearestDist(double distance,
                                       const RS_Vector& coord,
                                       double* dist) const
{
  RS_Vector p = pointAtDistance(distance);
  if (dist && p.valid) *dist = p.distanceTo(coord);
  return p;
}

//=====================================================================
// Point evaluation
//=====================================================================

RS_Vector LC_Hyperbola::getPoint(double phi, bool useReversed) const
{
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return RS_Vector(false);

  if (useReversed) phi += M_PI;
  phi = fmod(phi + 4.0*M_PI, 2.0*M_PI) - 2.0*M_PI;

  const double cp = cos(phi), sp = sin(phi);
  const double denom = cp*cp/(a*a) - sp*sp/(b*b);
  if (denom <= RS_TOLERANCE2) return RS_Vector(false);

  const double t = 1.0 / sqrt(denom);
  RS_Vector local(cp*t, sp*t);
  local.rotate(getAngle());
  return data.center + local;
}

RS_Vector LC_Hyperbola::getPoint(double phi) const
{
  return getPoint(phi, data.reversed);
}

//=====================================================================
// Helpers
//=====================================================================

double LC_Hyperbola::getParamFromPoint(const RS_Vector& p, bool branchReversed) const
{
  if (!p.valid) return NAN;
  RS_Vector d = p - data.center;
  if (d.squared() < RS_TOLERANCE2) return NAN;
  double phi = d.rotate(-getAngle()).angle();
  const double proj = RS_Vector::dotP(d, RS_Vector(cos(getAngle()), sin(getAngle())));
  if ((proj < -RS_TOLERANCE) != branchReversed)
    phi = RS_Math::correctAngle(phi + M_PI);
  return phi;
}

bool LC_Hyperbola::isValidPhi(double phi, double minPhi, double maxPhi) const
{
  return !std::isnan(phi) && !std::isinf(phi) &&
         phi >= minPhi - 2.0*RS_TOLERANCE && phi <= maxPhi + 2.0*RS_TOLERANCE;
}

void LC_Hyperbola::samplePhis(std::vector<double>& phis, double minPhi, double maxPhi, int n) const
{
  if (n < 2) return;
  const double delta = (maxPhi - minPhi) / (n - 1);
  for (int i = 0; i < n; ++i) phis.push_back(minPhi + i * delta);
}

bool LC_Hyperbola::isInClipRect(const RS_Vector& p,
                                double xmin, double xmax, double ymin, double ymax) const
{
  return p.valid &&
         p.x >= xmin - RS_TOLERANCE && p.x <= xmax + RS_TOLERANCE &&
         p.y >= ymin - RS_TOLERANCE && p.y <= ymax + RS_TOLERANCE;
}

//=====================================================================
// Minimal overrides
//=====================================================================

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector&, double*) const { return RS_Vector(false); }
RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector&, bool, double*, RS_Entity**) const { return RS_Vector(false); }
double LC_Hyperbola::getDistanceToPoint(const RS_Vector&, RS_Entity**, RS2::ResolveLevel, double) const { return RS_MAXDOUBLE; }
bool LC_Hyperbola::isPointOnEntity(const RS_Vector&, double) const { return false; }

void LC_Hyperbola::move(const RS_Vector& offset) { data.center += offset; }
void LC_Hyperbola::rotate(const RS_Vector& center, double angle)
{
  data.center.rotate(center, angle);
  data.majorP.rotate(angle);
}
void LC_Hyperbola::rotate(const RS_Vector& center, const RS_Vector& angleVector)
{
  rotate(center, angleVector.angle());
}
void LC_Hyperbola::scale(const RS_Vector& center, const RS_Vector& factor)
{
  data.center.scale(center, factor);
  data.majorP.scale(factor);
  data.ratio *= fabs(factor.y / factor.x);
}
void LC_Hyperbola::mirror(const RS_Vector& a1, const RS_Vector& a2)
{
  data.center.mirror(a1, a2);
  data.majorP.mirror(RS_Vector(0,0), a2 - a1);
  data.reversed = !data.reversed;
}

LC_Quadratic LC_Hyperbola::getQuadratic() const
{
  std::vector<double> c(6, 0.0);
  c[0] = data.majorP.squared();
  c[2] = -data.ratio*data.ratio*c[0];
  if (c[0] > RS_TOLERANCE2) c[0] = 1.0/c[0];
  if (fabs(c[2]) > RS_TOLERANCE2) c[2] = 1.0/c[2];
  c[5] = -1.0;
  LC_Quadratic q(c);
  q.rotate(data.majorP.angle());
  q.move(data.center);
  return q;
}
