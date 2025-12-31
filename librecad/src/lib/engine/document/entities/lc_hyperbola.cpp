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
#include <limits>
#include <vector>

#include <boost/math/quadrature/gauss_kronrod.hpp>

#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"


//=====================================================================
// Construction
//=====================================================================

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector &c, const RS_Vector &m,
                                   double r, double a1, double a2, bool rev)
    : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev) {}
// In lc_hyperbola.cpp – updated LC_HyperbolaData constructor (foci + point)

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector &f0, const RS_Vector &f1,
                                   const RS_Vector &p)
    : center((f0 + f1) * 0.5) {
  if (!p.valid || !f0.valid || !f1.valid) {
    majorP = RS_Vector(0, 0);
    return;
  }

  double d0 = f0.distanceTo(p); // distance to focus1 (f0)
  double d1 = f1.distanceTo(p); // distance to focus2 (f1)

  double dc = f0.distanceTo(f1);
  double diff = std::abs(d0 - d1); // |d_far - d_near|

  if (dc < RS_TOLERANCE || diff < RS_TOLERANCE) {
    majorP = RS_Vector(0, 0);
    return;
  }

  // Always use right branch (reversed = false)
  // Choose majorP direction toward the closer focus
  // This ensures the selected branch is always the "right" branch
  // mathematically
  RS_Vector closerFocus = (d0 < d1) ? f0 : f1;

  // Vector from center to closer focus (standard form has vertex toward closer
  // focus) But we want vertex toward farther focus for right branch Standard
  // hyperbola: (x/a)^2 - (y/b)^2 = 1 opens right/left We orient majorP toward
  // the branch containing the point (closer focus side)
  majorP = closerFocus - center;

  // Compute ratio = b/a
  // |d1 - d2| = 2a
  double a = diff * 0.5;
  double c = dc * 0.5; // distance from center to each focus
  double b = std::sqrt(c * c - a * a);

  if (b < RS_TOLERANCE) {
    majorP = {};
    return;
  }

  ratio = b / a;
  majorP = majorP.normalized() * a;
}

bool LC_HyperbolaData::isValid() const {
  LC_Hyperbola tempHb{nullptr, *this};
  return tempHb.isValid();
}

RS_Vector LC_HyperbolaData::getFocus1() const {
  RS_Vector df = majorP * std::sqrt(1. + ratio * ratio);
  return center + df;
}
RS_Vector LC_HyperbolaData::getFocus2() const {
  RS_Vector df = majorP * std::sqrt(1. + ratio * ratio);
  return center - df;
}

/**
 * Stream output operator for LC_HyperbolaData.
 *
 * Provides human-readable formatted output for debugging and logging.
 * Example output:
 *   HyperbolaData{center=(0,0), majorP=(5,0), ratio=1.5, angle1=0, angle2=0,
 * reversed=false}
 */
std::ostream &operator<<(std::ostream &os, const LC_HyperbolaData &d) {
  os << "HyperbolaData{"
     << "center=" << d.center << ", majorP=" << d.majorP
     << ", ratio=" << d.ratio << ", angle1=" << d.angle1
     << ", angle2=" << d.angle2
     << ", reversed=" << (d.reversed ? "true" : "false") << "}";
  return os;
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent,
                           const LC_HyperbolaData &d)
    : LC_CachedLengthEntity(parent), data(d),
      m_bValid(d.majorP.squared() >= RS_TOLERANCE2) {
  LC_Hyperbola::calculateBorders();
}

LC_Hyperbola::LC_Hyperbola(const RS_Vector &f0, const RS_Vector &f1,
                           const RS_Vector &p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p)) {}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent,
                           const std::vector<double> &coeffs)
    : LC_CachedLengthEntity(parent), m_bValid(false) {
  createFromQuadratic(coeffs);
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent, const LC_Quadratic &q)
    : LC_CachedLengthEntity(parent), m_bValid(false) {
  createFromQuadratic(q);
}

//=====================================================================
// Factory methods from quadratic
//=====================================================================

// In lc_hyperbola.cpp - improved createFromQuadratic() with robustness fixes

// File: lc_hyperbola.cpp - improved createFromQuadratic() with robust center
// handling

// In lc_hyperbola.cpp - improved createFromQuadratic() with robust center
// handling and degeneracy checks

bool LC_Hyperbola::createFromQuadratic(const LC_Quadratic &q) {
  std::vector<double> ce = q.getCoefficients();
  if (ce.size() < 6)
    return false;

  double A = ce[0], B = ce[1], C = ce[2];
  double D = ce[3], E = ce[4], F = ce[5];

  // === Step 1: Classify conic type using discriminant ===
  double disc = B * B - 4.0 * A * C;
  if (disc <= 0.0)
    return false; // Not a hyperbola (ellipse or parabola)

  // === Step 2: Degeneracy check using 3x3 determinant ===
  double det = A * (C * F - E * E / 4.0) -
               B / 2.0 * (B / 2.0 * F - D * E / 2.0) +
               D / 2.0 * (B / 2.0 * E - D * C / 2.0);

  if (std::abs(det) < RS_TOLERANCE)
    return false; // Degenerate (e.g., two lines)

  // === Step 3: Find rotation angle to eliminate xy term ===
  double theta = 0.0;
  if (std::abs(B) > RS_TOLERANCE) {
    theta = 0.5 * std::atan2(B, A - C);
  }

  double ct = std::cos(theta);
  double st = std::sin(theta);

  // Rotate quadratic terms
  double Ap = A * ct * ct + B * ct * st + C * st * st;
  double Cp = A * st * st - B * ct * st + C * ct * ct;
  //double Bp = 2.0 * (A - C) * ct * st + B * (ct * ct - st * st); // Should be ~0

  // Rotate linear terms
  double Dp = D * ct + E * st;
  double Ep = -D * st + E * ct;

  // === Step 4: Find center by solving partial derivatives ===
  // 2 Ap x + Dp = 0
  // 2 Cp y + Ep = 0
  RS_Vector center{0., 0.};
  if (std::abs(Ap) > RS_TOLERANCE) {
    center.x = -Dp / (2.0 * Ap);
  } else if (std::abs(Dp) > RS_TOLERANCE) {
    return false; // Unbounded in x → invalid for hyperbola
  }

  if (std::abs(Cp) > RS_TOLERANCE) {
    center.y = -Ep / (2.0 * Cp);
  } else if (std::abs(Ep) > RS_TOLERANCE) {
    return false; // Unbounded in y → invalid
  }

  // === Step 5: Translate to center and evaluate constant term ===
  double Fp = LC_Quadratic{{A, B, C, D, E, F}}.evaluateAt(center);

  // === Step 6: Normalize to standard form ===
  // Ap (x')² + Cp (y')² + Fp = 0
  double denom = -Fp;
  if (std::abs(denom) < RS_TOLERANCE)
    return false;

  double coeff_x = Ap / denom;
  double coeff_y = Cp / denom;

  double a2 = 0., b2 = 0.;
  bool transverse_x = (coeff_x > 0.0);

  if (transverse_x) {
    if (coeff_y >= 0.0)
      return false; // Both positive → ellipse-like
    a2 = 1.0 / coeff_x;
    b2 = -1.0 / coeff_y; // Make positive
  } else {
    if (coeff_x >= 0.0)
      return false;
    a2 = 1.0 / coeff_y;
    b2 = -1.0 / coeff_x;
  }

  if (a2 <= RS_TOLERANCE || b2 <= RS_TOLERANCE)
    return false;

  double a = std::sqrt(a2);
  double ratio = std::sqrt(b2 / a2);

  // === Step 7: Determine major axis direction and branch ===
  // Along rotated x' or y'
  RS_Vector major_dir = transverse_x ? RS_Vector(ct, st) : RS_Vector(-st, ct);

  // Determine branch: evaluate sign at vertex
  RS_Vector vertex = center + major_dir * a;
  double sign_at_vertex = q.evaluateAt(vertex);
  bool reversed = (sign_at_vertex < 0.0);

  // For left branch, flip direction
  if (reversed) {
    major_dir = -major_dir;
  }

  // === Step 8: Set data ===
  data.center = center;
  data.majorP = major_dir * a;
  data.ratio = ratio;
  data.reversed = reversed;
  data.angle1 = 0.0;
  data.angle2 = 0.0; // Unbounded by default

  m_bValid = true;
  LC_Hyperbola::calculateBorders();
  LC_Hyperbola::updateLength();

  return true;
}

bool LC_Hyperbola::createFromQuadratic(const std::vector<double> &coeffs) {
  if (coeffs.size() < 6)
    return false;
  LC_Quadratic q(coeffs);
  return createFromQuadratic(q);
}

//=====================================================================
// Entity interface
//=====================================================================

RS_Entity *LC_Hyperbola::clone() const { return new LC_Hyperbola(*this); }

RS_VectorSolutions LC_Hyperbola::getFoci() const {
  double e = std::sqrt(1.0 + data.ratio * data.ratio);
  RS_Vector vp = data.majorP * e;
  RS_VectorSolutions sol;
  sol.push_back(data.center + vp);
  sol.push_back(data.center - vp);
  return sol;
}

// In lc_hyperbola.cpp – current getRefPoints() implementation

RS_VectorSolutions LC_Hyperbola::getRefPoints() const {
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
  if (f1.valid)
    sol.push_back(f1);
  if (f2.valid)
    sol.push_back(f2);

  // Start and end points (only for bounded arcs)
  if (std::abs(data.angle1) >= RS_TOLERANCE ||
      std::abs(data.angle2) >= RS_TOLERANCE) {
    RS_Vector start = getStartpoint();
    RS_Vector end = getEndpoint();

    if (start.valid)
      sol.push_back(start);
    if (end.valid)
      sol.push_back(end);
  }

  return sol;
}

RS_Vector LC_Hyperbola::getStartpoint() const {
  if (data.angle1 == 0.0 && data.angle2 == 0.0)
    return RS_Vector(false);
  return getPoint(data.angle1, data.reversed);
}

RS_Vector LC_Hyperbola::getEndpoint() const {
  if (data.angle1 == 0.0 && data.angle2 == 0.0)
    return RS_Vector(false);
  return getPoint(data.angle2, data.reversed);
}

RS_Vector LC_Hyperbola::getMiddlePoint() const { return RS_Vector(false); }

//=====================================================================
// Tangent methods
//=====================================================================

double LC_Hyperbola::getDirection1() const {
  RS_Vector p = getStartpoint();
  if (!p.valid)
    return 0.0;
  return getTangentDirection(p).angle();
}

double LC_Hyperbola::getDirection2() const {
  RS_Vector p = getEndpoint();
  if (!p.valid)
    return 0.0;
  return getTangentDirection(p).angle();
}
RS_Vector LC_Hyperbola::getTangentDirectionParam(double parameter) const {
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

RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector &point) const {
  double phi = getParamFromPoint(point, data.reversed);
  return getTangentDirectionParam(phi);
}

RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector &point) const {
  if (!m_bValid || !point.valid)
    return RS_VectorSolutions();

  LC_Quadratic hyper = getQuadratic();
  if (!hyper.isValid())
    return RS_VectorSolutions();

  std::vector<double> coef = hyper.getCoefficients();
  double A = coef[0], B = coef[1], C = coef[2];
  double D = coef[3], E = coef[4], F = coef[5];

  double px = point.x, py = point.y;

  double polarA = A * px + (B / 2.0) * py + D / 2.0;
  double polarB = (B / 2.0) * px + C * py + E / 2.0;
  double polarK = D / 2.0 * px + E / 2.0 * py + F;

  if (std::abs(polarA) < RS_TOLERANCE && std::abs(polarB) < RS_TOLERANCE) {
    return RS_VectorSolutions();
  }

  RS_Vector p1, p2;
  if (std::abs(polarA) >= std::abs(polarB)) {
    p1 = RS_Vector(0.0, -polarK / polarB);
    p2 = RS_Vector(1.0, (-polarK - polarA) / polarB);
  } else {
    p1 = RS_Vector(-polarK / polarA, 0.0);
    p2 = RS_Vector((-polarK - polarB) / polarA, 1.0);
  }

  RS_Line polar(nullptr, RS_LineData(p1, p2));

  RS_VectorSolutions sol =
      LC_Quadratic::getIntersection(hyper, polar.getQuadratic());

  RS_VectorSolutions tangents;
  for (size_t i = 0; i < sol.getNumber(); ++i) {
    RS_Vector tp = sol.get(i);
    if (!tp.valid)
      continue;

    RS_Vector radius = tp - point;
    RS_Vector tangentDir = getTangentDirection(tp);
    if (tangentDir.valid &&
        std::abs(RS_Vector::dotP(radius, tangentDir)) < RS_TOLERANCE * 10.0) {
      tangents.push_back(tp);
    }
  }

  return tangents;
}

//=====================================================================
// Point evaluation
//=====================================================================

RS_Vector LC_Hyperbola::getPoint(double phi, bool useReversed) const {
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE)
    return RS_Vector(false);

  double ch = std::cosh(phi);
  double sh = std::sinh(phi);

  RS_Vector local(useReversed ? -a * ch : a * ch, b * sh);
  local.rotate(getAngle());
  return data.center + local;
}
// In lc_hyperbola.cpp – updated getParamFromPoint() to respect majorP
// orientation

double LC_Hyperbola::getParamFromPoint(const RS_Vector &p,
                                       bool /*branchReversed*/) const {
  if (!m_bValid || !p.valid) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const double a = getMajorRadius();
  const double b = getMinorRadius();

  if (a < RS_TOLERANCE || b < RS_TOLERANCE) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  // Transform point to local coordinate system:
  // - Translate so center is at origin
  // - Rotate so majorP aligns with positive x-axis
  RS_Vector local = p - data.center;
  double rotationAngle = data.majorP.angle();
  local.rotate(-rotationAngle); // inverse rotation

  double x_local = local.x;
  double y_local = local.y;

  // Standard right-branch hyperbola: x = a cosh(phi), y = b sinh(phi)
  // Therefore: phi = asinh(y_local / b)
  double phi = std::asinh(y_local / b);

  // Verify the point lies on the hyperbola (within tolerance)
  // Reconstruct x from phi and compare
  double x_calc = a * std::cosh(phi);

  // Since we oriented majorP toward the branch, x_local should be >= a (vertex)
  // Allow small negative tolerance for numerical robustness
  if (x_local < x_calc - RS_TOLERANCE * a) {
    // Point not on this branch — return NaN
    return std::numeric_limits<double>::quiet_NaN();
  }

  return phi;
}

bool LC_Hyperbola::isInClipRect(const RS_Vector &p, double xmin, double xmax,
                                double ymin, double ymax) const {
  return p.valid && p.x >= xmin - RS_TOLERANCE && p.x <= xmax + RS_TOLERANCE &&
         p.y >= ymin - RS_TOLERANCE && p.y <= ymax + RS_TOLERANCE;
}

//=====================================================================
// Rendering
//=====================================================================

void LC_Hyperbola::draw(RS_Painter *painter) {
  if (!painter || !isValid())
    return;

  const LC_Rect &clip = painter->getWcsBoundingRect();
  if (clip.isEmpty(RS_TOLERANCE))
    return;

  double xmin = clip.minP().x, xmax = clip.maxP().x;
  double ymin = clip.minP().y, ymax = clip.maxP().y;

  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE)
    return;

  double guiPixel = std::min(painter->toGuiDX(1.0), painter->toGuiDY(1.0));
  double maxWorldError = 1.0 / guiPixel;

  std::vector<RS_Vector> pts;
  pts.reserve(300);

  bool isFull = (data.angle1 == 0.0 && data.angle2 == 0.0);

  auto processBranch = [&, painter](bool rev) {
    std::vector<double> params;

    RS_Line borders[4] = {
        RS_Line(nullptr,
                RS_LineData(RS_Vector(xmin, ymin), RS_Vector(xmax, ymin))),
        RS_Line(nullptr,
                RS_LineData(RS_Vector(xmax, ymin), RS_Vector(xmax, ymax))),
        RS_Line(nullptr,
                RS_LineData(RS_Vector(xmax, ymax), RS_Vector(xmin, ymax))),
        RS_Line(nullptr,
                RS_LineData(RS_Vector(xmin, ymax), RS_Vector(xmin, ymin)))};

    for (const auto &line : borders) {
      RS_VectorSolutions sol =
          LC_Quadratic::getIntersection(getQuadratic(), line.getQuadratic());
      for (const RS_Vector &intersection : sol) {
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
      auto last =
          std::unique(params.begin(), params.end(), [](double a, double b) {
            return std::abs(a - b) < RS_TOLERANCE_ANGLE;
          });
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

void LC_Hyperbola::adaptiveSample(std::vector<RS_Vector> &out, double phiStart,
                                  double phiEnd, bool rev,
                                  double maxError) const {
  if (phiStart > phiEnd)
    std::swap(phiStart, phiEnd);

  std::vector<std::pair<double, RS_Vector>> points;
  points.reserve(256);

  std::function<void(double, double)> subdiv = [&](double pa, double pb) {
    RS_Vector A = getPoint(pa, rev);
    RS_Vector B = getPoint(pb, rev);
    if (!A.valid || !B.valid)
      return;

    double pm = (pa + pb) * 0.5;
    RS_Vector M = getPoint(pm, rev);
    if (!M.valid)
      return;

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
  if (first.valid)
    points.emplace_back(phiStart, first);

  subdiv(phiStart, phiEnd);

  std::sort(points.begin(), points.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });

  out.reserve(out.size() + points.size());
  for (const auto &kv : points) {
    if (out.empty() || out.back().distanceTo(kv.second) > RS_TOLERANCE) {
      out.push_back(kv.second);
    }
  }
}

//=====================================================================
// Nearest methods
//=====================================================================

RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector &coord, double *dist,
                                         int middlePoints) const {
  if (dist)
    *dist = RS_MAXDOUBLE;

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
      if (dist)
        *dist = d;
      return midPoint;
    }
    return RS_Vector(false);
  }

  RS_Vector vertex = data.center;
  double d = vertex.distanceTo(coord);
  if (dist)
    *dist = d;
  return vertex;
}

RS_Vector
LC_Hyperbola::getNearestOrthTan(const RS_Vector &coord, const RS_Line &normal,
                                [[maybe_unused]] bool onEntity) const {
  if (!m_bValid || !coord.valid || !normal.getStartpoint().valid ||
      !normal.getEndpoint().valid) {
    return RS_Vector(false);
  }

  RS_Vector normalDir = normal.getNormalVector();
  if (!normalDir.valid)
    return RS_Vector(false);

  RS_Vector tanDir(-normalDir.y, normalDir.x);

  const int samples = 120;
  RS_Vector best(false);
  double bestDist = RS_MAXDOUBLE;

  auto checkBranch = [&](bool rev) {
    for (int i = 0; i <= samples; ++i) {
      double phi = -M_PI + 2.0 * M_PI * i / samples + (rev ? M_PI : 0.0);
      RS_Vector p = getPoint(phi, rev);
      if (!p.valid)
        continue;

      RS_Vector tan = getTangentDirection(p);
      if (!tan.valid)
        continue;

      double angleDiff = std::abs(tan.angleTo(tanDir));
      if (angleDiff > M_PI / 2.0)
        angleDiff = M_PI - angleDiff;

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

RS_Vector LC_Hyperbola::getNearestDist(double distance, const RS_Vector &coord,
                                       double *dist) const {
  if (dist)
    *dist = RS_MAXDOUBLE;

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

void LC_Hyperbola::move(const RS_Vector &offset) { data.center += offset; }

void LC_Hyperbola::rotate(const RS_Vector &center, double angle) {
  data.center.rotate(center, angle);
  data.majorP.rotate(angle);
}

void LC_Hyperbola::rotate(const RS_Vector &center,
                          const RS_Vector &angleVector) {
  rotate(center, angleVector.angle());
}

void LC_Hyperbola::scale(const RS_Vector &center, const RS_Vector &factor) {
  data.center.scale(center, factor);
  data.majorP.scale(factor);
  data.ratio *= std::abs(factor.y / factor.x);
}

void LC_Hyperbola::mirror(const RS_Vector &axisPoint1,
                          const RS_Vector &axisPoint2) {
  data.center.mirror(axisPoint1, axisPoint2);
  data.majorP.mirror(RS_Vector(0, 0), axisPoint2 - axisPoint1);
  // data.reversed = !data.reversed;
  m_bValid = data.majorP.squared() >= RS_TOLERANCE2;
  RS_Vector vp = getStartpoint().mirror(axisPoint1, axisPoint2);
  data.angle1 = getParamFromPoint(vp);
  vp = getEndpoint().mirror(axisPoint1, axisPoint2);
  data.angle2 = getParamFromPoint(vp);
  if (data.angle1 > data.angle2)
    std::swap(data.angle1, data.angle2);

  LC_Hyperbola::calculateBorders();
}

//=====================================================================
// Minimal overrides
//=====================================================================

// In lc_hyperbola.cpp – implemented getNearestEndpoint

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector &coord,
                                           double *dist) const {
  if (dist)
    *dist = RS_MAXDOUBLE;
  if (!m_bValid || !coord.valid) {
    return RS_Vector(false);
  }

  // For unbounded hyperbolas (full branch), there are no defined endpoints
  if (!std::isnormal(data.angle1) && !std::isnormal(data.angle2)) {
    return RS_Vector(false);
  }

  double distance = RS_MAXDOUBLE;
  RS_Vector ret{false};
  for (const RS_Vector &vp : {getStartpoint(), getEndpoint()}) {
    if (vp.valid) {
      double dvp = vp.distanceTo(coord);
      if (dvp <= distance - RS_TOLERANCE) {
        distance = dvp;
        ret = vp;
      }
    }
  }
  if (dist != nullptr)
    *dist = distance;
  return ret;
}
// In lc_hyperbola.cpp – improved getNearestPointOnEntity() with quartic solving
// and onEntity support

RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector &coord,
                                                bool onEntity, double *dist,
                                                RS_Entity **entity) const {
  if (!m_bValid || !coord.valid) {
    if (dist)
      *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

  if (entity)
    *entity = const_cast<LC_Hyperbola *>(this);

  // Special case: unbounded hyperbola (full branch)
  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    // For unbounded case, use asymptotic behavior for far points
    // But for most practical cases, the vertex is often the nearest
    RS_Vector vertex = data.center + data.majorP;
    double dVertex = coord.distanceTo(vertex);

    // Simple heuristic: if point is far along the major axis direction, project
    // to asymptote
    RS_Vector dir = (coord - data.center).normalized();
    double dot = dir.angleTo(data.majorP.normalized());

    if (std::abs(dot) < RS_TOLERANCE_ANGLE ||
        std::abs(dot - M_PI) < RS_TOLERANCE_ANGLE) {
      // Along major axis – nearest is vertex
      if (dist)
        *dist = dVertex;
      return vertex;
    } else {
      // Otherwise, vertex is reasonable approximation for unbounded
      if (dist)
        *dist = dVertex;
      return vertex;
    }
  }

  // Bounded or semi-bounded case – use parametric search + quartic for accuracy

  // First, get initial guess by sampling the arc
  double phiGuess = getParamFromPoint(coord, data.reversed);
  if (std::isnan(phiGuess)) {
    phiGuess = (data.angle1 + data.angle2) * 0.5; // fallback to middle
  }

  // Clamp initial guess to arc range for bounded case
  double phiMin = std::min(data.angle1, data.angle2);
  double phiMax = std::max(data.angle1, data.angle2);
  phiGuess = std::max(phiMin, std::min(phiMax, phiGuess));

  // Evaluate distance squared at endpoints and initial guess
  RS_Vector pStart = getPoint(data.angle1, data.reversed);
  RS_Vector pEnd = getPoint(data.angle2, data.reversed);
  RS_Vector pGuess = getPoint(phiGuess, data.reversed);

  double d2Start = coord.squaredTo(pStart);
  double d2End = coord.squaredTo(pEnd);
  double d2Guess = coord.squaredTo(pGuess);

  double minD2 = std::min({d2Start, d2End, d2Guess});
  RS_Vector nearest =
      (minD2 == d2Start) ? pStart : (minD2 == d2End ? pEnd : pGuess);

  // Now solve the exact quartic equation for critical points
  // Distance squared: d²(phi) = (x(phi) - px)² + (y(phi) - py)²
  // d(d²)/dphi = 0 ⇒ (x - px) x' + (y - py) y' = 0

  double px = coord.x, py = coord.y;
  double cx = data.center.x, cy = data.center.y;
  double aa = data.majorP.magnitude(); // semi-major a
  double bb = aa * data.ratio;         // semi-minor b
  double ct = std::cos(data.majorP.angle());
  double st = std::sin(data.majorP.angle());

  double A = aa * ct;
  double B = -bb * st;
  double C = aa * st;
  double D = bb * ct;

  // Coefficients of the quartic: tanh⁴ + p tanh³ + q tanh² + r tanh + s = 0
  double dx = cx + A - px;
  double dy = cy + C - py;

  double p = 4.0 * (A * dx + C * dy) / (B * dx + D * dy);
  double q = (dx * dx + dy * dy - aa * aa + bb * bb) / (B * dx + D * dy) * 2.0 -
             p * p / 2.0 - 3.0;
  double r = -p * (q + 5.0);
  double s = -(dx * dx + dy * dy - aa * aa - bb * bb) / (B * dx + D * dy) - q;

  std::vector<double> ce = {s, r, q, p,
                            1.0}; // t^4 + p t^3 + q t^2 + r t + s = 0

  std::vector<double> roots = RS_Math::quarticSolverFull(ce);

  // Evaluate all valid real roots
  for (double t : roots) {
    if (std::abs(B * dx + D * dy) < RS_TOLERANCE)
      continue; // degenerate case skipped

    double phi = std::atanh(t);
    if (std::isnan(phi) || std::isinf(phi))
      continue;

    // Check if phi is within the arc range
    bool inRange = (phi >= phiMin - RS_TOLERANCE_ANGLE &&
                    phi <= phiMax + RS_TOLERANCE_ANGLE);

    if (onEntity && !inRange)
      continue;

    RS_Vector cand = getPoint(phi, data.reversed);
    if (!cand.valid)
      continue;

    double d2Cand = coord.squaredTo(cand);

    if (onEntity) {
      // For onEntity=true, clamp to arc endpoints if outside
      if (!inRange) {
        double d2StartNew = coord.squaredTo(pStart);
        double d2EndNew = coord.squaredTo(pEnd);
        if (d2StartNew < minD2) {
          minD2 = d2StartNew;
          nearest = pStart;
        }
        if (d2EndNew < minD2) {
          minD2 = d2EndNew;
          nearest = pEnd;
        }
        continue;
      }
    }

    if (d2Cand < minD2 - RS_TOLERANCE) {
      minD2 = d2Cand;
      nearest = cand;
    }
  }

  // Final fallback to endpoints if onEntity
  if (onEntity) {
    if (coord.squaredTo(pStart) < minD2) {
      minD2 = coord.squaredTo(pStart);
      nearest = pStart;
    }
    if (coord.squaredTo(pEnd) < minD2) {
      minD2 = coord.squaredTo(pEnd);
      nearest = pEnd;
    }
  }

  if (dist)
    *dist = std::sqrt(minD2);
  return nearest;
}

double LC_Hyperbola::getDistanceToPoint(const RS_Vector &coord,
                                        RS_Entity **entity,
                                        RS2::ResolveLevel /*level*/,
                                        double /*solidDist*/) const {
  if (entity)
    *entity = nullptr;

  if (!m_bValid || !coord.valid) {
    return RS_MAXDOUBLE;
  }

  double dist = RS_MAXDOUBLE;
  getNearestPointOnEntity(coord, true, &dist, entity);

  if (entity && *entity == nullptr && dist < RS_MAXDOUBLE) {
    *entity = const_cast<LC_Hyperbola *>(this);
  }

  return dist;
}

bool LC_Hyperbola::isPointOnEntity(const RS_Vector &coord,
                                   double tolerance) const {
  if (!m_bValid || !coord.valid)
    return false;

  auto coef = getQuadratic().getCoefficients();
  double value = coef[0] * coord.x * coord.x + coef[1] * coord.x * coord.y +
                 coef[2] * coord.y * coord.y + coef[3] * coord.x +
                 coef[4] * coord.y + coef[5];

  bool onCurve = std::abs(value) <= tolerance * tolerance;

  if (onCurve && data.angle1 != 0.0 && data.angle2 != 0.0) {
    double phi = getParamFromPoint(coord, data.reversed);
    double phiMin =
        std::min(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    double phiMax =
        std::max(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    onCurve = (phi >= phiMin - tolerance && phi <= phiMax + tolerance);
  }

  return onCurve;
}

LC_Quadratic LC_Hyperbola::getQuadratic() const {
  std::vector<double> ce(6, 0.);
  ce[0] = data.majorP.squared();
  ce[2] = -data.ratio * data.ratio * ce[0];
  if (ce[0] < RS_TOLERANCE2 && std::abs(ce[2]) < RS_TOLERANCE2) {
    return LC_Quadratic();
  }
  ce[0] = 1. / ce[0];
  ce[2] = 1. / ce[2];
  ce[5] = -1.;
  LC_Quadratic ret(ce);
  ret.rotate(getAngle());
  ret.move(data.center);
  return ret;
}

// lc_hyperbola.cpp - fixed calculateBorders() for hyperbolic parameter range

void LC_Hyperbola::calculateBorders() {
  minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
  maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);

  if (!m_bValid)
    return;

  // Full unbounded hyperbola → infinite bounds
  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    minV = RS_Vector(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
    maxV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    return;
  }

  // Limited arc on single branch
  double phiStart = data.angle1;
  double phiEnd = data.angle2;

  // No normalization needed — hyperbolic φ is over all real numbers
  // Ensure start ≤ end for consistent processing
  if (phiStart > phiEnd)
    std::swap(phiStart, phiEnd);

  // Branch offset handled in getPoint() — use raw angles here

  // Analytical extrema along global X and Y axes
  double rot = getAngle();
  RS_Vector dirX(cos(rot), sin(rot));
  RS_Vector dirY(-sin(rot), cos(rot));

  auto addExtrema = [&](const RS_Vector &dir) {
    double dx = dir.x, dy = dir.y;
    if (std::abs(dx) < RS_TOLERANCE && std::abs(dy) < RS_TOLERANCE)
      return;

    double tanh_phi = -(getMinorRadius() * dy) / (getMajorRadius() * dx);
    if (std::abs(tanh_phi) >= 1.0)
      return; // no real solution

    double phi = std::atanh(tanh_phi);
    // Check both solutions (phi and phi + π) — but only one will be on the
    // correct branch
    for (int sign = 0; sign < 2; ++sign) {
      double phi_cand = phi + sign * M_PI;
      if (phi_cand >= phiStart - RS_TOLERANCE &&
          phi_cand <= phiEnd + RS_TOLERANCE) {
        RS_Vector p = getPoint(phi_cand, data.reversed);
        if (p.valid) {
          minV = RS_Vector::minimum(minV, p);
          maxV = RS_Vector::maximum(maxV, p);
        }
      }
    }
  };

  addExtrema(RS_Vector(1.0, 0.0)); // global X
  addExtrema(RS_Vector(0.0, 1.0)); // global Y

  // Endpoints
  RS_Vector start = getPoint(phiStart, data.reversed);
  RS_Vector end = getPoint(phiEnd, data.reversed);
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

double LC_Hyperbola::getLength() const {
  if (!m_bValid)
    return 0.0;

  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    return RS_MAXDOUBLE; // unbounded
  }

  double phi1 = std::min(data.angle1, data.angle2);
  double phi2 = std::max(data.angle1, data.angle2);

  double a = data.majorP.magnitude();
  double e2 = 1.0 + data.ratio * data.ratio;

  auto integrand = [a, e2](double phi) -> double {
    double ch = std::cosh(phi);
    double inner = std::max(0., e2 * ch * ch - 1.0);
    return a * std::sqrt(inner);
  };

  boost::math::quadrature::gauss_kronrod<double, 61> integrator;
  return integrator.integrate(integrand, phi1, phi2);
}

void LC_Hyperbola::updateLength() { cachedLength = LC_Hyperbola::getLength(); }

// lc_hyperbola.cpp - add implementations at the end of the file

void LC_Hyperbola::setFocus1(const RS_Vector &f1) {
  if (!f1.valid || !m_bValid)
    return;

  RS_Vector f2 = data.getFocus2();
  // Use a point on the current curve (vertex approximation at phi=0)
  RS_Vector currentPoint = getPoint(0.0, data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !data.reversed); // try opposite branch
  }
  if (!currentPoint.valid)
    return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    data = newData;
    m_bValid = true;
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setFocus2(const RS_Vector &f2) {
  if (!f2.valid || !m_bValid)
    return;

  RS_Vector f1 = data.getFocus1();
  RS_Vector currentPoint = getPoint(0.0, data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !data.reversed);
  }
  if (!currentPoint.valid)
    return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    data = newData;
    m_bValid = true;
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setPointOnCurve(const RS_Vector &p) {
  if (!p.valid || !m_bValid)
    return;

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

// lc_hyperbola.cpp - add these implementations (place with other property
// editing methods)

// Direct ratio setter (b/a)
void LC_Hyperbola::setRatio(double r) {
  if (r <= 0.0 || !m_bValid)
    return;
  data.ratio = r;
  calculateBorders();
  updateLength();
}

// Minor radius setter (b = a * ratio)
void LC_Hyperbola::setMinorRadius(double b) {
  if (b <= 0.0 || !m_bValid)
    return;
  double a = getMajorRadius();
  if (a > 0.0) {
    data.ratio = b / a;
    calculateBorders();
    updateLength();
  }
}

// Set the primary vertex (closest vertex on the selected branch)
// This adjusts the major radius 'a' while keeping center and direction fixed
void LC_Hyperbola::setPrimaryVertex(const RS_Vector &v) {
  if (!v.valid || !m_bValid)
    return;

  RS_Vector dir = data.majorP;
  if (dir.squared() < RS_TOLERANCE2)
    return;
  dir.normalize();

  RS_Vector expectedVertex = data.reversed
                                 ? data.center - dir * getMajorRadius()
                                 : data.center + dir * getMajorRadius();

  RS_Vector offset = v - expectedVertex;
  double distanceAlongAxis = offset.dotP(dir);

  double newA = std::abs(getMajorRadius() + distanceAlongAxis);
  if (newA < RS_TOLERANCE)
    return;

  // Adjust majorP magnitude
  data.majorP = dir * newA;
  if (data.reversed)
    data.majorP = -data.majorP; // preserve direction for left branch

  calculateBorders();
  updateLength();
}

// In lc_hyperbola.cpp – current moveRef() implementation (latest version)

void LC_Hyperbola::moveRef(const RS_Vector &ref, const RS_Vector &offset) {
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
  if (std::abs(data.angle1) >= RS_TOLERANCE ||
      std::abs(data.angle2) >= RS_TOLERANCE) {
    RS_Vector start = getStartpoint();
    RS_Vector end = getEndpoint();

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

RS_Vector LC_Hyperbola::getPrimaryVertex() const {
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

void LC_Hyperbola::moveStartpoint(const RS_Vector &pos) {
  if (!m_bValid || !pos.valid)
    return;

  // Unbounded hyperbolas have no defined endpoints
  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "LC_Hyperbola::moveStartpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newStart = getNearestPointOnEntity(pos, true);
  if (!newStart.valid)
    return;

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

void LC_Hyperbola::moveEndpoint(const RS_Vector &pos) {
  if (!m_bValid || !pos.valid)
    return;

  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "LC_Hyperbola::moveEndpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newEnd = getNearestPointOnEntity(pos, true);
  if (!newEnd.valid)
    return;

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
 * @return ∮ x dy (twice the signed area contribution when part of a closed
 * path)
 */
double LC_Hyperbola::areaLineIntegral() const {
  if (!m_bValid)
    return 0.0;

  // Unbounded hyperbola → integral diverges
  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    return 0.0;
  }

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE)
    return 0.0;

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
  const double A = a * ct;  // cosh term in x
  const double B = -b * st; // sinh term in x
  const double C = a * st;  // cosh term in y
  const double D = b * ct;  // sinh term in y

  // Antiderivative of x(φ) * y'(φ)
  auto F = [&](double phi) -> double {
    const double ch = std::cosh(phi);
    const double sh = std::sinh(phi);
    const double ch2 = std::cosh(2.0 * phi);
    const double sh2 = std::sinh(2.0 * phi);

    return cx * (D * ch + C * sh) + (A * D + B * C) * 0.5 * (ch2 + 1.0) +
           (A * C + B * D) * 0.5 * sh2;
  };

  double integral = F(phi2) - F(phi1);

  if (data.reversed)
    integral = -integral;
  if (reverse)
    integral = -integral;

  return integral;
}

// In lc_hyperbola.cpp – UPDATED prepareTrim implementation (replaces previous
// version)

RS_Vector LC_Hyperbola::prepareTrim(const RS_Vector &trimCoord,
                                    const RS_VectorSolutions &trimSol) {
  if (!m_bValid || trimSol.empty()) {
    return RS_Vector(false);
  }

  // Unbounded hyperbolas are not trimmable in the usual sense
  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    return RS_Vector(false);
  }

  const RS_Vector start = getStartpoint();
  const RS_Vector end = getEndpoint();

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

  for (const RS_Vector &sol : trimSol) {
    if (!sol.valid)
      continue;

    // Project solution onto the hyperbola for robustness
    RS_Vector proj = getNearestPointOnEntity(sol, true);
    if (!proj.valid)
      proj = sol;

    double phi = getParamFromPoint(proj, data.reversed);
    if (std::isnan(phi))
      continue;

    bool onStartSide = (clickedSide == RS2::EndingStart) ? (phi <= data.angle1)
                                                         : (phi >= data.angle2);

    // Primary criterion: must be on the clicked side (extend or trim away from
    // click)
    if (!onStartSide)
      continue;

    // Secondary: prefer the solution farthest from the kept endpoint
    double score =
        (clickedSide == RS2::EndingStart)
            ? (data.angle1 - phi)  // larger difference = farther left
            : (phi - data.angle2); // larger difference = farther right

    if (score > bestScore) {
      bestScore = score;
      bestSol = proj;
    }
  }

  // Fallback: if no solution on the clicked side, use closest valid
  // intersection
  if (!bestSol.valid) {
    double minDist = RS_MAXDOUBLE;
    for (const RS_Vector &sol : trimSol) {
      if (!sol.valid)
        continue;
      RS_Vector proj = getNearestPointOnEntity(sol, true);
      if (!proj.valid)
        proj = sol;
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

RS2::Ending LC_Hyperbola::getTrimPoint(const RS_Vector & /*trimCoord*/,
                                       const RS_Vector &trimPoint) {
  if (!m_bValid || !trimPoint.valid) {
    return RS2::EndingNone;
  }

  // For unbounded hyperbolas, treat as no defined ends
  if (std::abs(data.angle1) < RS_TOLERANCE &&
      std::abs(data.angle2) < RS_TOLERANCE) {
    return RS2::EndingNone;
  }

  const RS_Vector start = getStartpoint();
  const RS_Vector end = getEndpoint();

  if (!start.valid || !end.valid) {
    return RS2::EndingNone;
  }

  const double distToStart = trimPoint.distanceTo(start);
  const double distToEnd = trimPoint.distanceTo(end);

  // Use a small tolerance to avoid flickering when exactly midway
  if (distToStart + RS_TOLERANCE < distToEnd) {
    return RS2::EndingStart;
  } else {
    return RS2::EndingEnd;
  }
}

// In lc_hyperbola.cpp – fixed and improved dualLineTangentPoint() (analogous to
// ellipse)

RS_Vector LC_Hyperbola::dualLineTangentPoint(const RS_Vector &line) const {
  if (!m_bValid || !line.valid) {
    return RS_Vector(false);
  }
  // u x + v y + 1 = 0
  // coordinates : dual
  // real coordinates is rotated from canonical
  // (u; v)^T (M X) + 1 =0
  // Equivalent to rotation in dual coordinates, but opposite angle
  // ( M^T (u; v)^T) X + 1 = 0
  RS_Vector uv = RS_Vector{line}.rotate(-data.majorP.angle());
  // slope = (a sinh, b cosh)
  // u a sinh + v b cosh = 0,
  // phi = atanh(- (vb)/(ua))

  // No horizontal tangent lines for canonical form
  if (std::abs(uv.x) < RS_TOLERANCE_ANGLE)
    return RS_Vector{false};
  double r = -getRatio() * uv.y / uv.x;
  if (std::abs(r) > 1. - RS_TOLERANCE)
    return RS_Vector{false};

  return getPoint(std::atanh(r), false);
}
