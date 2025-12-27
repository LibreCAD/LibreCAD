// lc_hyperbola.cpp
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
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_line.h"

//=====================================================================
// Construction
//=====================================================================

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& c, const RS_Vector& m,
                                   double r, double a1, double a2, bool rev)
    : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev) {}

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : center((f0 + f1) * 0.5)
{
  if (!p.valid || !f0.valid || !f1.valid) {
    majorP = RS_Vector(0, 0);
    return;
  }

  double d1 = f0.distanceTo(p);
  double d2 = f1.distanceTo(p);

  bool pCloserToF1 = (d1 < d2);

  RS_Vector fartherFocus = pCloserToF1 ? f1 : f0;
  RS_Vector closerFocus  = pCloserToF1 ? f0 : f1;

  double diff = fartherFocus.distanceTo(p) - closerFocus.distanceTo(p);

  double dc = f0.distanceTo(f1);
  double dd = diff;

  if (dc < RS_TOLERANCE || dd < RS_TOLERANCE) {
    majorP = RS_Vector(0, 0);
  LC_ERR<<__LINE__<<": "<<majorP.x<<" "<<majorP.y<<", r="<<ratio<<", dd="<<dd<<", dc="<<dc;
    return;
  }

  majorP = closerFocus - center;

  ratio = dc / dd;
  majorP /= ratio;
  ratio = std::sqrt(ratio * ratio - 1.0);
  // LC_ERR<<__LINE__<<": "<<majorP.x<<" "<<majorP.y<<", r="<<ratio<<", dd="<<dd<<", dc="<<dc;
}

bool LC_HyperbolaData::isValid() const
{
  LC_Hyperbola tempHb{nullptr, *this};
  return tempHb.isValid();
}

RS_Vector LC_HyperbolaData::getFocus1() const
{
  RS_Vector df = majorP * std::sqrt(1. + ratio * ratio);
  return center + df;
}
RS_Vector LC_HyperbolaData::getFocus2() const
{
  RS_Vector df = majorP * std::sqrt(1. + ratio * ratio);
  return center - df;
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d)
    : LC_CachedLengthEntity(parent), data(d), m_bValid(d.majorP.squared() >= RS_TOLERANCE2)
{
  LC_Hyperbola::calculateBorders();
}

LC_Hyperbola::LC_Hyperbola(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p)) {}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const std::vector<double>& coeffs)
    : LC_CachedLengthEntity(parent), m_bValid(false)
{
  createFromQuadratic(coeffs);
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_Quadratic& q)
    : LC_CachedLengthEntity(parent), m_bValid(false)
{
  createFromQuadratic(q);
}


//=====================================================================
// Factory methods from quadratic
//=====================================================================

// In lc_hyperbola.cpp - improved createFromQuadratic() with robustness fixes

// File: lc_hyperbola.cpp - improved createFromQuadratic() with robust center handling

bool LC_Hyperbola::createFromQuadratic(const LC_Quadratic& q)
{
  std::vector<double> ce = q.getCoefficients();
  if (ce.size() < 6) return false;

  double A = ce[0], B = ce[1], C = ce[2];
  double D = ce[3], E = ce[4], F = ce[5];

         // Hyperbola discriminant B² - 4AC > 0
  double disc = B * B - 4.0 * A * C;
  if (disc <= 0.0) return false;

         // Determinant for center: 4AC - B² = -disc
  double det = 4.0 * A * C - B * B;  // negative for hyperbola
  constexpr double tol = 1e-10;
  if (std::abs(det) < tol) return false;

         // Center coordinates - robust computation
  double cx = (B * E - 2.0 * C * D) / det;
  double cy = (B * D - 2.0 * A * E) / det;

  RS_Vector center(cx, cy);

         // Translate quadratic to origin
  LC_Quadratic translated = q;
  translated.move(-center);

  std::vector<double> ct = translated.getCoefficients();
  double At = ct[0], Bt = ct[1], Ct = ct[2], Ft = ct[5];

         // Rotation angle to eliminate xy term
  double theta = 0.0;
  if (std::abs(Bt) > tol) {
    theta = 0.5 * std::atan2(Bt, At - Ct);
  }

         // Rotate to align axes
  LC_Quadratic rotated = translated;
  rotated.rotate(theta);

  std::vector<double> cr = rotated.getCoefficients();
  double Ar = cr[0], Cr = cr[2], Fr = cr[5];

         // Hyperbola: opposite signs
  if (Ar * Cr >= -tol) return false;  // allow small negative due to precision

         // Compute a² and b²
  double a2 = -Fr / Ar;
  double b2 = -Fr / Cr;

         // Clamp tiny negative values from precision error
  if (a2 <= -tol || b2 <= -tol) return false;
  a2 = std::max(a2, 0.0);
  b2 = std::max(b2, 0.0);

  if (a2 < tol || b2 < tol) return false;

  double a = std::sqrt(a2);
  double b = std::sqrt(b2);

         // Determine major axis orientation
  bool horizontal = (Ar > 0.0);

  RS_Vector majorP(a, 0.0);
  double finalAngle = theta;
  if (!horizontal) {
    std::swap(a, b);
    finalAngle += M_PI / 2.0;
  }
  majorP.rotate(finalAngle);

         // Build hyperbola data
  data.center = center;
  data.majorP = majorP;
  data.ratio = b / a;
  data.reversed = false;  // default right branch
  data.angle1 = 0.0;
  data.angle2 = 0.0;      // full branch

  m_bValid = true;
  calculateBorders();
  return true;
}

bool LC_Hyperbola::createFromQuadratic(const std::vector<double>& coeffs)
{
  if (coeffs.size() < 6) return false;
  LC_Quadratic q(coeffs);
  return createFromQuadratic(q);
}

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

// In lc_hyperbola.cpp – current getRefPoints() implementation

RS_VectorSolutions LC_Hyperbola::getRefPoints() const
{
  RS_VectorSolutions sol;

  if (!m_bValid) {
    return sol;
  }

         // Center (always included)
  sol.push_back(data.center);

         // Primary vertex (closest vertex on the selected branch)
  RS_Vector primaryVertex = getPrimaryVertex();
  if (primaryVertex.valid) {
    sol.push_back(primaryVertex);
  }

         // Foci
  RS_Vector f1 = data.getFocus1();
  RS_Vector f2 = data.getFocus2();
  if (f1.valid) sol.push_back(f1);
  if (f2.valid) sol.push_back(f2);

         // Start and end points (only for bounded arcs)
  if (std::abs(data.angle1) >= RS_TOLERANCE || std::abs(data.angle2) >= RS_TOLERANCE) {
    RS_Vector start = getStartpoint();
    RS_Vector end   = getEndpoint();

    if (start.valid) sol.push_back(start);
    if (end.valid)   sol.push_back(end);
  }

  return sol;
}

RS_Vector LC_Hyperbola::getStartpoint() const
{
  if (data.angle1 == 0.0 && data.angle2 == 0.0) return RS_Vector(false);
  return getPoint(data.angle1, data.reversed);
}

RS_Vector LC_Hyperbola::getEndpoint() const
{
  if (data.angle1 == 0.0 && data.angle2 == 0.0) return RS_Vector(false);
  return getPoint(data.angle2, data.reversed);
}

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
RS_Vector LC_Hyperbola::getTangentDirectionParam(double parameter) const
{
  double a = getMajorRadius();
  double b = getMinorRadius();

  double dx = a * std::sinh(parameter);
  double dy = b * std::cosh(parameter);
  if (data.reversed)
    dx = -dx;

  RS_Vector tangent{dx, dy};
  tangent.rotate(data.majorP.angle());
  return tangent.normalized();
}

RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector& point) const
{
  double phi = getParamFromPoint(point, data.reversed);
  return getTangentDirectionParam(phi);
}

RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector& point) const
{
  if (!m_bValid || !point.valid) return RS_VectorSolutions();

  LC_Quadratic hyper = getQuadratic();
  if (!hyper.isValid()) return RS_VectorSolutions();

  std::vector<double> coef = hyper.getCoefficients();
  double A = coef[0], B = coef[1], C = coef[2];
  double D = coef[3], E = coef[4], F = coef[5];

  double px = point.x, py = point.y;

  double polarA = A * px + (B / 2.0) * py + D / 2.0;
  double polarB = (B / 2.0) * px + C * py + E / 2.0;
  double polarK = D / 2.0 * px + E / 2.0 * py + F;

  if (fabs(polarA) < RS_TOLERANCE && fabs(polarB) < RS_TOLERANCE) {
    return RS_VectorSolutions();
  }

  RS_Vector p1, p2;
  if (fabs(polarA) >= fabs(polarB)) {
    p1 = RS_Vector(0.0, -polarK / polarB);
    p2 = RS_Vector(1.0, (-polarK - polarA) / polarB);
  } else {
    p1 = RS_Vector(-polarK / polarA, 0.0);
    p2 = RS_Vector((-polarK - polarB) / polarA, 1.0);
  }

  RS_Line polar(nullptr, RS_LineData(p1, p2));

  RS_VectorSolutions sol = LC_Quadratic::getIntersection(hyper, polar.getQuadratic());

  RS_VectorSolutions tangents;
  for (size_t i = 0; i < sol.getNumber(); ++i) {
    RS_Vector tp = sol.get(i);
    if (!tp.valid) continue;

    RS_Vector radius = tp - point;
    RS_Vector tangentDir = getTangentDirection(tp);
    if (tangentDir.valid && fabs(RS_Vector::dotP(radius, tangentDir)) < RS_TOLERANCE * 10.0) {
      tangents.push_back(tp);
    }
  }

  return tangents;
}

//=====================================================================
// Point evaluation
//=====================================================================

RS_Vector LC_Hyperbola::getPoint(double phi, bool useReversed) const
{
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return RS_Vector(false);

  double ch = std::cosh(phi);
  double sh = std::sinh(phi);

  RS_Vector local(useReversed ? -a * ch : a * ch, b * sh);
  local.rotate(getAngle());
  return data.center + local;
}

double LC_Hyperbola::getParamFromPoint(const RS_Vector& p, bool branchReversed) const
{
  if (!p.valid) return NAN;

  RS_Vector d = p - data.center;
  if (d.squared() < RS_TOLERANCE2) return NAN;

  RS_Vector local = d.rotate(-getAngle());

  double x = local.x;
  double y = local.y;

  double a = getMajorRadius();
  double b = getMinorRadius();

  if (std::fabs(x) < a - RS_TOLERANCE) return NAN;

  double ya = y / b;

  double phi = std::asinh(ya);

  if (std::signbit(x) != branchReversed) return NAN;

  return phi;
}

bool LC_Hyperbola::isInClipRect(const RS_Vector& p,
                                double xmin, double xmax, double ymin, double ymax) const
{
  return p.valid &&
         p.x >= xmin - RS_TOLERANCE && p.x <= xmax + RS_TOLERANCE &&
         p.y >= ymin - RS_TOLERANCE && p.y <= ymax + RS_TOLERANCE;
}

//=====================================================================
// Rendering
//=====================================================================

void LC_Hyperbola::draw(RS_Painter* painter)
{
  if (!painter || !isValid()) return;

  const LC_Rect& clip = painter->getWcsBoundingRect();
  if (clip.isEmpty(RS_TOLERANCE)) return;

  double xmin = clip.minP().x, xmax = clip.maxP().x;
  double ymin = clip.minP().y, ymax = clip.maxP().y;

  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return;

  double guiPixel = std::min(painter->toGuiDX(1.0), painter->toGuiDY(1.0));
  double maxWorldError = 1.0 / guiPixel;

  std::vector<RS_Vector> pts;
  pts.reserve(300);

  bool isFull = (data.angle1 == 0.0 && data.angle2 == 0.0);

  auto processBranch = [&, painter](bool rev) {
    std::vector<double> params;

    RS_Line borders[4] = {
        RS_Line(nullptr, RS_LineData(RS_Vector(xmin, ymin), RS_Vector(xmax, ymin))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmax, ymin), RS_Vector(xmax, ymax))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmax, ymax), RS_Vector(xmin, ymax))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmin, ymax), RS_Vector(xmin, ymin)))
    };

    for (const auto& line : borders) {
      RS_VectorSolutions sol = LC_Quadratic::getIntersection(getQuadratic(), line.getQuadratic());
      for (const RS_Vector& intersection: sol) {
        double phiCur = getParamFromPoint(intersection);
        if (std::isnan(phiCur) || phiCur < data.angle1 || phiCur > data.angle2)
          continue;
        if (isInClipRect(intersection, xmin, xmax, ymin, ymax)) {
          params.push_back(phiCur);
        }
      }
    }

    if (params.empty()) {
      RS_Vector test = getPoint((data.angle1 + data.angle2) * 0.5, rev);
      if (test.valid && isInClipRect(test, xmin, xmax, ymin, ymax)) {
        params = {data.angle1, data.angle2};
      } else {
        return;
      }
    } else {
      params.push_back(data.angle1);
      params.push_back(data.angle2);
      std::sort(params.begin(), params.end());
      auto last = std::unique(params.begin(), params.end(),
                              [](double a, double b){ return std::abs(a-b) < RS_TOLERANCE_ANGLE; });
      params.erase(last, params.end());
    }

    for (size_t i = 0; i + 1 < params.size(); ++i) {
      double start = params[i];
      double end = params[i + 1];
      RS_Vector middle = getPoint((start + end) * 0.5, rev);
      pts.clear();
      if (isInClipRect(middle, xmin, xmax, ymin, ymax)) {
          adaptiveSample(pts, start, end, rev, maxWorldError);
          painter->drawSplinePointsWCS(pts, false);
      }
    }
  };

  if (isFull) {
    processBranch(false);
    processBranch(true);
  } else {
    processBranch(data.reversed);
  }
}

void LC_Hyperbola::adaptiveSample(std::vector<RS_Vector>& out,
                                  double phiStart, double phiEnd, bool rev,
                                  double maxError) const
{
  if (phiStart > phiEnd) std::swap(phiStart, phiEnd);

  std::vector<std::pair<double, RS_Vector>> points;
  points.reserve(256);

  std::function<void(double, double)> subdiv = [&](double pa, double pb) {
    RS_Vector A = getPoint(pa, rev);
    RS_Vector B = getPoint(pb, rev);
    if (!A.valid || !B.valid) return;

    double pm = (pa + pb) * 0.5;
    RS_Vector M = getPoint(pm, rev);
    if (!M.valid) return;

    double sagitta = (M - (A + B) * 0.5).magnitude();
    double estimatedMaxError = sagitta * 1.15;

    if (estimatedMaxError < maxError || (pb - pa) < 0.05) {
      points.emplace_back(pa, A);
      points.emplace_back(pb, B);
      return;
    }

    subdiv(pa, pm);
    subdiv(pm, pb);
  };

  RS_Vector first = getPoint(phiStart, rev);
  if (first.valid) points.emplace_back(phiStart, first);

  subdiv(phiStart, phiEnd);

  std::sort(points.begin(), points.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  out.reserve(out.size() + points.size());
  for (const auto& kv : points) {
    if (out.empty() || out.back().distanceTo(kv.second) > RS_TOLERANCE) {
      out.push_back(kv.second);
    }
  }
}

//=====================================================================
// Nearest methods
//=====================================================================

RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector& coord,
                                         double* dist,
                                         int middlePoints) const
{
  if (dist) *dist = RS_MAXDOUBLE;

  if (!m_bValid || middlePoints < 1 || !coord.valid) {
    return RS_Vector(false);
  }

  if (data.angle1 != 0.0 || data.angle2 != 0.0) {
    double phi1 = std::min(data.angle1, data.angle2);
    double phi2 = std::max(data.angle1, data.angle2);
    double phiRange = phi2 - phi1;

    if (phiRange < RS_TOLERANCE) {
      return RS_Vector(false);
    }

    double phiMid = phi1 + phiRange * 0.5;
    RS_Vector midPoint = getPoint(phiMid, data.reversed);

    if (midPoint.valid) {
      double d = midPoint.distanceTo(coord);
      if (dist) *dist = d;
      return midPoint;
    }
    return RS_Vector(false);
  }

  RS_Vector vertex = data.center;
  double d = vertex.distanceTo(coord);
  if (dist) *dist = d;
  return vertex;
}

RS_Vector LC_Hyperbola::getNearestOrthTan(const RS_Vector& coord,
                                          const RS_Line& normal,
                                          bool onEntity) const
{
  if (!m_bValid || !coord.valid ||
      !normal.getStartpoint().valid || !normal.getEndpoint().valid) {
    return RS_Vector(false);
  }

  RS_Vector normalDir = normal.getNormalVector();
  if (!normalDir.valid) return RS_Vector(false);

  RS_Vector tanDir(-normalDir.y, normalDir.x);

  const int samples = 120;
  RS_Vector best(false);
  double bestDist = RS_MAXDOUBLE;

  auto checkBranch = [&](bool rev) {
    for (int i = 0; i <= samples; ++i) {
      double phi = -M_PI + 2.0 * M_PI * i / samples + (rev ? M_PI : 0.0);
      RS_Vector p = getPoint(phi, rev);
      if (!p.valid) continue;

      RS_Vector tan = getTangentDirection(p);
      if (!tan.valid) continue;

      double angleDiff = fabs(tan.angleTo(tanDir));
      if (angleDiff > M_PI/2.0) angleDiff = M_PI - angleDiff;

      if (angleDiff < 0.1) {
        double d = p.distanceTo(coord);
        if (d < bestDist) {
          bestDist = d;
          best = p;
        }
      }
    }
  };

  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    checkBranch(false);
    checkBranch(true);
  } else {
    checkBranch(data.reversed);
  }

  return best;
}

RS_Vector LC_Hyperbola::getNearestDist(double distance,
                                       const RS_Vector& coord,
                                       double* dist) const
{
  if (dist) *dist = RS_MAXDOUBLE;

  if (!m_bValid || distance < RS_TOLERANCE) {
    return RS_Vector(false);
  }

  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    return RS_Vector(false);
  }

  double phi1 = std::min(data.angle1, data.angle2);
  double phi2 = std::max(data.angle1, data.angle2);
  double phiRange = phi2 - phi1;

  if (phiRange < RS_TOLERANCE) {
    return RS_Vector(false);
  }

  double fraction = distance / getLength();
  if (fraction > 1.0) {
    return RS_Vector(false);
  }

  double phi = phi1 + fraction * phiRange;
  RS_Vector p = getPoint(phi, data.reversed);

  if (p.valid && dist) {
    *dist = distance;
  }

  return p;
}

//=====================================================================
// Transformations
//=====================================================================

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

void LC_Hyperbola::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)
{
  data.center.mirror(axisPoint1, axisPoint2);
  data.majorP.mirror(RS_Vector(0,0), axisPoint2 - axisPoint1);
  data.reversed = !data.reversed;
  m_bValid = data.majorP.squared() >= RS_TOLERANCE2;
  calculateBorders();
}

//=====================================================================
// Minimal overrides
//=====================================================================

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector&, double*) const { return RS_Vector(false); }

RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector& coord, bool onEntity,
                                                double* dist, RS_Entity** entity) const
{
  if (dist) *dist = RS_MAXDOUBLE;
  if (entity) *entity = nullptr;

  if (!m_bValid || !coord.valid) return RS_Vector(false);

  auto coef = getQuadratic().getCoefficients();
  double A = coef[0], B = coef[1], C = coef[2];
  double D = coef[3], E = coef[4], F = coef[5];

  const int maxIter = 30;
  const double tol = RS_TOLERANCE * 0.1;

  std::vector<RS_Vector> starts{data.center, coord,
                                data.center + data.majorP.normalized() * getMajorRadius() * 2.0,
                                data.center - data.majorP.normalized() * getMajorRadius() * 2.0};

  RS_Vector best(false);
  double bestD = RS_MAXDOUBLE;

  auto refine = [&](RS_Vector p) -> RS_Vector {
    for (int i = 0; i < maxIter; ++i) {
      double Q = A*p.x*p.x + B*p.x*p.y + C*p.y*p.y + D*p.x + E*p.y + F;
      RS_Vector g(2*A*p.x + B*p.y + D, B*p.x + 2*C*p.y + E);
      if (!g.valid || g.squared() < RS_TOLERANCE2) break;
      double lambda = -2.0 * RS_Vector::dotP(p - coord, g) / g.squared();
      double damping = (i > 5) ? 0.5 : 1.0;
      RS_Vector up = g * lambda * damping;
      p += up;
      if (up.squared() < tol*tol) {
        if (fabs(Q) > tol) p -= g * (Q / g.squared());
        return p;
      }
    }
    return RS_Vector(false);
  };

  for (auto s : starts)
    if (auto cand = refine(s); cand.valid && cand.distanceTo(coord) < bestD)
      best = cand, bestD = cand.distanceTo(coord);

  if (!best.valid) {
    auto sample = [&](bool rev) {
      const int n = 300;
      double step = 30.0 / n;
      for (int i = 0; i <= n; ++i) {
        double phi = -15.0 + step * i + (rev ? M_PI : 0.0);
        if (auto sp = getPoint(phi, rev); sp.valid && sp.distanceTo(coord) < bestD)
          best = sp, bestD = sp.distanceTo(coord);
      }
    };
    if (data.angle1 == 0.0 && data.angle2 == 0.0) {
      sample(false);
      sample(true);
    } else sample(data.reversed);
  }

  if (best.valid && data.angle1 != 0.0 && data.angle2 != 0.0) {
    double phi = getParamFromPoint(best, data.reversed);
    double p1 = std::min(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    double p2 = std::max(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    if (phi < p1 || phi > p2) best = RS_Vector(false);
  }

  if (best.valid && dist) *dist = bestD;
  if (entity) *entity = const_cast<LC_Hyperbola*>(this);
  return best;
}

double LC_Hyperbola::getDistanceToPoint(const RS_Vector& coord,
                                        RS_Entity** entity,
                                        RS2::ResolveLevel /*level*/,
                                        double /*solidDist*/) const
{
  if (entity) *entity = nullptr;

  if (!m_bValid || !coord.valid) {
    return RS_MAXDOUBLE;
  }

  double dist = RS_MAXDOUBLE;
  getNearestPointOnEntity(coord, true, &dist, entity);

  if (entity && *entity == nullptr && dist < RS_MAXDOUBLE) {
    *entity = const_cast<LC_Hyperbola*>(this);
  }

  return dist;
}

bool LC_Hyperbola::isPointOnEntity(const RS_Vector& coord,
                                   double tolerance) const
{
  if (!m_bValid || !coord.valid) return false;

  auto coef = getQuadratic().getCoefficients();
  double value = coef[0]*coord.x*coord.x + coef[1]*coord.x*coord.y + coef[2]*coord.y*coord.y +
                 coef[3]*coord.x + coef[4]*coord.y + coef[5];

  bool onCurve = fabs(value) <= tolerance * tolerance;

  if (onCurve && data.angle1 != 0.0 && data.angle2 != 0.0) {
    double phi = getParamFromPoint(coord, data.reversed);
    double phiMin = std::min(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    double phiMax = std::max(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    onCurve = (phi >= phiMin - tolerance && phi <= phiMax + tolerance);
  }

  return onCurve;
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

// lc_hyperbola.cpp - fixed calculateBorders() for hyperbolic parameter range

void LC_Hyperbola::calculateBorders()
{
  minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
  maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);

  if (!m_bValid) return;

         // Full unbounded hyperbola → infinite bounds
  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    minV = RS_Vector(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
    maxV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    return;
  }

         // Limited arc on single branch
  double phiStart = data.angle1;
  double phiEnd   = data.angle2;

         // No normalization needed — hyperbolic φ is over all real numbers
         // Ensure start ≤ end for consistent processing
  if (phiStart > phiEnd) std::swap(phiStart, phiEnd);

         // Branch offset handled in getPoint() — use raw angles here

         // Analytical extrema along global X and Y axes
  double rot = getAngle();
  RS_Vector dirX(cos(rot), sin(rot));
  RS_Vector dirY(-sin(rot), cos(rot));

  auto addExtrema = [&](const RS_Vector& dir) {
    double dx = dir.x, dy = dir.y;
    if (fabs(dx) < RS_TOLERANCE && fabs(dy) < RS_TOLERANCE) return;

    double tanh_phi = - (getMinorRadius() * dy) / (getMajorRadius() * dx);
    if (fabs(tanh_phi) >= 1.0) return;  // no real solution

    double phi = std::atanh(tanh_phi);
    // Check both solutions (phi and phi + π) — but only one will be on the correct branch
    for (int sign = 0; sign < 2; ++sign) {
      double phi_cand = phi + sign * M_PI;
      if (phi_cand >= phiStart - RS_TOLERANCE && phi_cand <= phiEnd + RS_TOLERANCE) {
        RS_Vector p = getPoint(phi_cand, data.reversed);
        if (p.valid) {
          minV = RS_Vector::minimum(minV, p);
          maxV = RS_Vector::maximum(maxV, p);
        }
      }
    }
  };

  addExtrema(RS_Vector(1.0, 0.0));  // global X
  addExtrema(RS_Vector(0.0, 1.0));  // global Y

         // Endpoints
  RS_Vector start = getPoint(phiStart, data.reversed);
  RS_Vector end   = getPoint(phiEnd,   data.reversed);
  if (start.valid) {
    minV = RS_Vector::minimum(minV, start);
    maxV = RS_Vector::maximum(maxV, start);
  }
  if (end.valid) {
    minV = RS_Vector::minimum(minV, end);
    maxV = RS_Vector::maximum(maxV, end);
  }

         // Safety expansion
  double expand = RS_TOLERANCE * 100.0;
  minV -= RS_Vector(expand, expand);
  maxV += RS_Vector(expand, expand);
}

// lc_hyperbola.cpp - getLength() implementation
// In lc_hyperbola.cpp – highly optimized iterative adaptive Simpson with minimal stack usage

#include <array>
#include <cmath>
#include <limits>

namespace {
// Inline speed function (critical path)
inline double hyperbolaSpeed(double phi, double a2, double b2) noexcept {
  const double sh = std::sinh(phi);
  const double ch = std::cosh(phi);
  return std::sqrt(a2 * sh * sh + b2 * ch * ch);
}
}

double LC_Hyperbola::getLength() const
{
  if (!m_bValid) return 0.0;

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return 0.0;

  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    return std::numeric_limits<double>::infinity();
  }

  double phiL = data.angle1;
  double phiR = data.angle2;
  if (phiL > phiR) std::swap(phiL, phiR);

  const double a2 = a * a;
  const double b2 = b * b;

  constexpr double eps_base = 1e-10;
  constexpr int max_depth = 30;

         // Fixed-size array instead of std::stack — avoids dynamic allocation entirely
         // Worst-case: 2^30 intervals → impossible in practice; real usage < 1000
         // Safe upper bound: 1024 intervals (more than enough for double precision)
  static constexpr size_t MAX_INTERVALS = 1024;

  struct Interval {
    double left, right;
    double eps;
    double whole;
    int depth;
    double f_left, f_mid, f_right;
  };

  std::array<Interval, MAX_INTERVALS> intervals{};
  size_t stack_size = 0;

         // Initial interval
  const double mid_init = 0.5 * (phiL + phiR);
  const double fL = hyperbolaSpeed(phiL, a2, b2);
  const double fR = hyperbolaSpeed(phiR, a2, b2);
  const double fM = hyperbolaSpeed(mid_init, a2, b2);
  const double whole_init = (fL + 4.0 * fM + fR) * (phiR - phiL) / 6.0;

  intervals[stack_size++] = {phiL, phiR, eps_base, whole_init, max_depth, fL, fM, fR};

  double length = 0.0;

  while (stack_size > 0) {
    const Interval curr = intervals[--stack_size];

    if (curr.depth <= 0) {
      length += curr.whole;
      continue;
    }

    const double cm = 0.5 * (curr.left + curr.right);
    const double lm = 0.5 * (curr.left + cm);
    const double rm = 0.5 * (curr.right + cm);

    const double flm = hyperbolaSpeed(lm, a2, b2);
    const double frm = hyperbolaSpeed(rm, a2, b2);

    const double h_left  = (cm - curr.left);
    const double h_right = (curr.right - cm);

    const double left  = (curr.f_left + 4.0 * flm + curr.f_mid) * h_left / 6.0;
    const double right = (curr.f_mid + 4.0 * frm + curr.f_right) * h_right / 6.0;
    const double delta = left + right - curr.whole;

    if (std::abs(delta) <= 15.0 * curr.eps) {
      length += left + right + delta / 15.0;
    } else if (stack_size + 2 < MAX_INTERVALS) {
      // Push right first → left processed next (depth-first)
      intervals[stack_size++] = {cm, curr.right, curr.eps * 0.5, right, curr.depth - 1,
                                 curr.f_mid, frm, curr.f_right};
      intervals[stack_size++] = {curr.left, cm, curr.eps * 0.5, left, curr.depth - 1,
                                 curr.f_left, flm, curr.f_mid};
    } else {
      // Fallback: accept current approximation if stack exhausted (extremely rare)
      length += curr.whole;
    }
  }

  return length;
}
void LC_Hyperbola::updateLength()
{
    cachedLength = getLength();
}

// lc_hyperbola.cpp - add implementations at the end of the file

void LC_Hyperbola::setFocus1(const RS_Vector& f1)
{
  if (!f1.valid || !m_bValid) return;

  RS_Vector f2 = data.getFocus2();
  // Use a point on the current curve (vertex approximation at phi=0)
  RS_Vector currentPoint = getPoint(0.0, data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !data.reversed); // try opposite branch
  }
  if (!currentPoint.valid) return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    data = newData;
    m_bValid = true;
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setFocus2(const RS_Vector& f2)
{
  if (!f2.valid || !m_bValid) return;

  RS_Vector f1 = data.getFocus1();
  RS_Vector currentPoint = getPoint(0.0, data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !data.reversed);
  }
  if (!currentPoint.valid) return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    data = newData;
    m_bValid = true;
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setPointOnCurve(const RS_Vector& p)
{
  if (!p.valid || !m_bValid) return;

  RS_Vector f1 = data.getFocus1();
  RS_Vector f2 = data.getFocus2();

  LC_HyperbolaData newData(f1, f2, p);
  if (newData.isValid()) {
    data = newData;
    m_bValid = true;
    calculateBorders();
    updateLength();
  }
}

// lc_hyperbola.cpp - add these implementations (place with other property editing methods)

// Direct ratio setter (b/a)
void LC_Hyperbola::setRatio(double r)
{
  if (r <= 0.0 || !m_bValid) return;
  data.ratio = r;
  calculateBorders();
  updateLength();
}

// Minor radius setter (b = a * ratio)
void LC_Hyperbola::setMinorRadius(double b)
{
  if (b <= 0.0 || !m_bValid) return;
  double a = getMajorRadius();
  if (a > 0.0) {
    data.ratio = b / a;
    calculateBorders();
    updateLength();
  }
}

// Set the primary vertex (closest vertex on the selected branch)
// This adjusts the major radius 'a' while keeping center and direction fixed
void LC_Hyperbola::setPrimaryVertex(const RS_Vector& v)
{
  if (!v.valid || !m_bValid) return;

  RS_Vector dir = data.majorP;
  if (dir.squared() < RS_TOLERANCE2) return;
  dir.normalize();

  RS_Vector expectedVertex = data.reversed
                                 ? data.center - dir * getMajorRadius()
                                 : data.center + dir * getMajorRadius();

  RS_Vector offset = v - expectedVertex;
  double distanceAlongAxis = offset.dotP(dir);

  double newA = std::abs(getMajorRadius() + distanceAlongAxis);
  if (newA < RS_TOLERANCE) return;

         // Adjust majorP magnitude
  data.majorP = dir * newA;
  if (data.reversed) data.majorP = -data.majorP;  // preserve direction for left branch

  calculateBorders();
  updateLength();
}

// In lc_hyperbola.cpp – current moveRef() implementation (latest version)

void LC_Hyperbola::moveRef(const RS_Vector& ref, const RS_Vector& offset)
{
  if (!m_bValid || !ref.valid || !offset.valid) {
    return;
  }

         // 1. Center movement – translate entire hyperbola
  if ((data.center - ref).squared() < RS_TOLERANCE2) {
    data.center += offset;
    calculateBorders();
    updateLength();
    return;
  }

         // 2. Primary vertex movement – constrained to major axis
  RS_Vector primaryVertex = getPrimaryVertex();
  if (primaryVertex.valid && (primaryVertex - ref).squared() < RS_TOLERANCE2) {
    RS_Vector axisDir = data.majorP;
    axisDir.normalize();
    if (data.reversed) {
      axisDir = -axisDir;
    }

    RS_Vector startPt = getStartpoint();
    RS_Vector endPt = getEndpoint();
    RS_Vector newVertexPos = primaryVertex + offset;
    RS_Vector vecFromCenter = newVertexPos - data.center;
    double projLength = vecFromCenter.dotP(axisDir);

    double originalDistAlongAxis = (primaryVertex - data.center).dotP(axisDir);
    if (originalDistAlongAxis * projLength < 0.0) {
      projLength = RS_TOLERANCE;
    }
    if (projLength < RS_TOLERANCE) {
      projLength = RS_TOLERANCE;
    }

    data.majorP = axisDir * projLength;

    data.angle1 = getParamFromPoint(startPt);
    data.angle2 = getParamFromPoint(endPt);
    calculateBorders();
    updateLength();
    return;
  }

         // 3. Focus movement
  RS_Vector f1 = data.getFocus1();
  RS_Vector f2 = data.getFocus2();

  if (f1.valid && (f1 - ref).squared() < RS_TOLERANCE2) {
    setFocus1(f1 + offset);
    return;
  }
  if (f2.valid && (f2 - ref).squared() < RS_TOLERANCE2) {
    setFocus2(f2 + offset);
    return;
  }

         // 4. Start/End point movement – only adjust parametric range (angle1/angle2)
         //    Do NOT modify center, majorP, ratio, or branch
  if (std::abs(data.angle1) >= RS_TOLERANCE || std::abs(data.angle2) >= RS_TOLERANCE) {
    RS_Vector start = getStartpoint();
    RS_Vector end   = getEndpoint();

    if (start.valid && (start - ref).squared() < RS_TOLERANCE2) {
      // Project new position onto the hyperbola and update angle1 only
      RS_Vector newPos = start + offset;
      RS_Vector projected = getNearestPointOnEntity(newPos, true);
      if (!projected.valid) {
        projected = newPos; // fallback
      }
      double newPhi = getParamFromPoint(projected, data.reversed);
      if (!std::isnan(newPhi)) {
        data.angle1 = newPhi;
        calculateBorders();
        updateLength();
      }
      return;
    }

    if (end.valid && (end - ref).squared() < RS_TOLERANCE2) {
      // Project new position onto the hyperbola and update angle2 only
      RS_Vector newPos = end + offset;
      RS_Vector projected = getNearestPointOnEntity(newPos, true);
      if (!projected.valid) {
        projected = newPos;
      }
      double newPhi = getParamFromPoint(projected, data.reversed);
      if (!std::isnan(newPhi)) {
        data.angle2 = newPhi;
        calculateBorders();
        updateLength();
      }
      return;
    }
  }

         // Fallback: translate entire entity
  move(offset);
}

// lc_hyperbola.cpp - implementation

RS_Vector LC_Hyperbola::getPrimaryVertex() const
{
  if (!m_bValid) {
    return RS_Vector(false);
  }

  double a = getMajorRadius();
  if (a < RS_TOLERANCE) {
    return RS_Vector(false);
  }

         // majorP already contains the vector from center to the right-branch vertex
         // with magnitude = a and correct direction
  RS_Vector vertex = data.center + data.majorP;

  if (data.reversed) {
    // For left branch, the primary vertex is on the opposite side
    vertex = data.center - data.majorP;
  }

  return vertex;
}

// lc_hyperbola.cpp – ADDED METHODS (place near other override implementations)

//=====================================================================
// Grip editing: move start/end points
//=====================================================================

void LC_Hyperbola::moveStartpoint(const RS_Vector& pos)
{
  if (!m_bValid || !pos.valid) return;

         // Unbounded hyperbolas have no defined endpoints
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LC_Hyperbola::moveStartpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newStart = getNearestPointOnEntity(pos, true);
  if (!newStart.valid) return;

  double newPhi1 = getParamFromPoint(newStart, data.reversed);
  double delta = data.angle2 - data.angle1;

  if (data.angle1 > data.angle2) {
    // Reversed angular order
    data.angle1 = newPhi1 + delta;
    data.angle2 = newPhi1;
  } else {
    data.angle1 = newPhi1;
    data.angle2 = newPhi1 + delta;
  }

  calculateBorders();
  updateLength();
}

void LC_Hyperbola::moveEndpoint(const RS_Vector& pos)
{
  if (!m_bValid || !pos.valid) return;

  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LC_Hyperbola::moveEndpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newEnd = getNearestPointOnEntity(pos, true);
  if (!newEnd.valid) return;

  double newPhi2 = getParamFromPoint(newEnd, data.reversed);
  double delta = data.angle2 - data.angle1;

  if (data.angle1 > data.angle2) {
    data.angle1 = newPhi2;
    data.angle2 = newPhi2 - delta;
  } else {
    data.angle2 = newPhi2;
  }

  calculateBorders();
  updateLength();
}

//=====================================================================
// Area calculation support (Green's theorem)
//=====================================================================

/**
 * @brief areaLineIntegral
 * Computes ∮ x dy along the hyperbola arc.
 * Used with Green's theorem for closed contour area: Area = ½ (∮ x dy - ∮ y dx)
 * @return ∮ x dy (twice the signed area contribution when part of a closed path)
 */
double LC_Hyperbola::areaLineIntegral() const
{
  if (!m_bValid) return 0.0;

         // Unbounded hyperbola → integral diverges
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    return 0.0;
  }

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return 0.0;

  double phi1 = data.angle1;
  double phi2 = data.angle2;
  bool reverse = false;
  if (phi1 > phi2) {
    std::swap(phi1, phi2);
    reverse = true;
  }

  const double cx = data.center.x;
  const double theta = data.majorP.angle();
  const double ct = std::cos(theta);
  const double st = std::sin(theta);

         // Coefficients for parametric form
  const double A = a * ct;    // cosh term in x
  const double B = -b * st;   // sinh term in x
  const double C = a * st;    // cosh term in y
  const double D = b * ct;    // sinh term in y

         // Antiderivative of x(φ) * y'(φ)
  auto F = [&](double phi) -> double {
    const double ch = std::cosh(phi);
    const double sh = std::sinh(phi);
    const double ch2 = std::cosh(2.0 * phi);
    const double sh2 = std::sinh(2.0 * phi);

    return cx * (D * ch + C * sh) +
           (A * D + B * C) * 0.5 * (ch2 + 1.0) +
           (A * C + B * D) * 0.5 * sh2;
  };

  double integral = F(phi2) - F(phi1);

  if (data.reversed) integral = -integral;
  if (reverse) integral = -integral;

  return integral;
}

// In lc_hyperbola.cpp – UPDATED prepareTrim implementation (replaces previous version)

RS_Vector LC_Hyperbola::prepareTrim(const RS_Vector& trimCoord,
                                    const RS_VectorSolutions& trimSol)
{
  if (!m_bValid || trimSol.empty()) {
    return RS_Vector(false);
  }

         // Unbounded hyperbolas are not trimmable in the usual sense
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    return RS_Vector(false);
  }

  const RS_Vector start = getStartpoint();
  const RS_Vector end   = getEndpoint();

  if (!start.valid || !end.valid) {
    return RS_Vector(false);
  }

         // Determine which endpoint the user clicked closer to
  RS_Vector nearestOnEntity = getNearestPointOnEntity(trimCoord, true);
  if (!nearestOnEntity.valid) {
    nearestOnEntity = trimCoord; // fallback
  }

  RS2::Ending clickedSide = getTrimPoint(trimCoord, nearestOnEntity);

         // Among valid intersection solutions, choose the one that:
         // 1. Lies on the side the user clicked (start or end direction)
         // 2. Is farthest in the direction away from the opposite endpoint
         // This ensures the segment containing the clicked point is preserved.
  RS_Vector bestSol(false);
  double bestScore = -RS_MAXDOUBLE;

  for (const RS_Vector& sol : trimSol) {
    if (!sol.valid) continue;

           // Project solution onto the hyperbola for robustness
    RS_Vector proj = getNearestPointOnEntity(sol, true);
    if (!proj.valid) proj = sol;

    double phi = getParamFromPoint(proj, data.reversed);
    if (std::isnan(phi)) continue;

    bool onStartSide = (clickedSide == RS2::EndingStart)
                           ? (phi <= data.angle1)
                           : (phi >= data.angle2);

           // Primary criterion: must be on the clicked side (extend or trim away from click)
    if (!onStartSide) continue;

           // Secondary: prefer the solution farthest from the kept endpoint
    double score = (clickedSide == RS2::EndingStart)
                       ? (data.angle1 - phi)   // larger difference = farther left
                       : (phi - data.angle2);  // larger difference = farther right

    if (score > bestScore) {
      bestScore = score;
      bestSol = proj;
    }
  }

         // Fallback: if no solution on the clicked side, use closest valid intersection
  if (!bestSol.valid) {
    double minDist = RS_MAXDOUBLE;
    for (const RS_Vector& sol : trimSol) {
      if (!sol.valid) continue;
      RS_Vector proj = getNearestPointOnEntity(sol, true);
      if (!proj.valid) proj = sol;
      double d = proj.distanceTo(nearestOnEntity);
      if (d < minDist) {
        minDist = d;
        bestSol = proj;
      }
    }
  }

  if (!bestSol.valid) {
    return RS_Vector(false);
  }

  double newPhi = getParamFromPoint(bestSol, data.reversed);

         // Update only the endpoint on the clicked side
  if (clickedSide == RS2::EndingStart) {
    data.angle1 = newPhi;
  } else {
    data.angle2 = newPhi;
  }

  calculateBorders();
  updateLength();

  return bestSol;
}

RS2::Ending LC_Hyperbola::getTrimPoint(const RS_Vector& trimCoord,
                                       const RS_Vector& trimPoint)
{
  if (!m_bValid || !trimPoint.valid) {
    return RS2::EndingNone;
  }

         // For unbounded hyperbolas, treat as no defined ends
  if (std::abs(data.angle1) < RS_TOLERANCE && std::abs(data.angle2) < RS_TOLERANCE) {
    return RS2::EndingNone;
  }

  const RS_Vector start = getStartpoint();
  const RS_Vector end   = getEndpoint();

  if (!start.valid || !end.valid) {
    return RS2::EndingNone;
  }

  const double distToStart = trimPoint.distanceTo(start);
  const double distToEnd   = trimPoint.distanceTo(end);

         // Use a small tolerance to avoid flickering when exactly midway
  if (distToStart + RS_TOLERANCE < distToEnd) {
    return RS2::EndingStart;
  } else {
    return RS2::EndingEnd;
  }
}
