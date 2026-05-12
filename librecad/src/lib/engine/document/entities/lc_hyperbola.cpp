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
  // The LC_Hyperbola constructor's only validity test is `majorP.squared() >=
  // RS_TOLERANCE2`. Inline that check directly so callers (e.g.,
  // splineToHyperbola, focus setters, dialogs) don't pay for a temporary
  // entity construction with calculateBorders + numerical integration.
  return majorP.squared() >= RS_TOLERANCE2;
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
    : LC_CachedLengthEntity(parent), m_data(d),
      m_valid(d.majorP.squared() >= RS_TOLERANCE2) {
  LC_Hyperbola::calculateBorders();
}

LC_Hyperbola::LC_Hyperbola(const RS_Vector &f0, const RS_Vector &f1,
                           const RS_Vector &p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p)) {}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent,
                           const std::vector<double> &coeffs)
    : LC_CachedLengthEntity(parent), m_valid(false) {
  createFromQuadratic(coeffs);
}

LC_Hyperbola::LC_Hyperbola(RS_EntityContainer *parent, const LC_Quadratic &q)
    : LC_CachedLengthEntity(parent), m_valid(false) {
  createFromQuadratic(q);
}

//=====================================================================
// Factory methods from quadratic
//=====================================================================
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
  // The rotation by θ = ½·atan2(B, A−C) is supposed to eliminate the cross
  // term. Verify it actually did — if not (e.g. ill-conditioned input), the
  // following center/classification reasoning is invalid.
  double Bp = 2.0 * (A - C) * ct * st + B * (ct * ct - st * st);
  if (std::abs(Bp) > RS_TOLERANCE * (std::abs(A) + std::abs(C) + 1.0))
    return false;

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
  m_data.center = center;
  m_data.majorP = major_dir * a;
  m_data.ratio = ratio;
  m_data.reversed = reversed;
  m_data.angle1 = 0.0;
  m_data.angle2 = 0.0; // Unbounded by default

  m_valid = true;
  LC_Hyperbola::calculateBorders();
  LC_Hyperbola::updateLength();

  return true;
}

//=====================================================================
bool LC_Hyperbola::createFromQuadratic(const std::vector<double> &coeffs) {
  if (coeffs.size() < 6)
    return false;
  LC_Quadratic q(coeffs);
  return createFromQuadratic(q);
}

//=====================================================================
// Entity interface
//=====================================================================
RS_Entity *LC_Hyperbola::clone() const {
  return new LC_Hyperbola(*this);
}

RS_VectorSolutions LC_Hyperbola::getFoci() const {
  double e = std::sqrt(1.0 + m_data.ratio * m_data.ratio);
  RS_Vector vp = m_data.majorP * e;
  RS_VectorSolutions sol;
  sol.push_back(m_data.center + vp);
  sol.push_back(m_data.center - vp);
  return sol;
}

RS_VectorSolutions LC_Hyperbola::getRefPoints() const {
  RS_VectorSolutions sol;

  if (!m_valid) {
    return sol;
  }

  // Center (always included)
  sol.push_back(m_data.center);

  // Primary vertex (on the selected branch)
  RS_Vector primaryVertex = getPrimaryVertex();
  if (primaryVertex.valid) {
    sol.push_back(primaryVertex);
  }
  // Secondary vertex (opposite branch). For unbounded full hyperbolas the
  // user expects to snap to either vertex (analogous to the two endpoints
  // of an ellipse's major axis). For a bounded arc this is still meaningful
  // as an algebraic landmark.
  RS_Vector secondaryVertex = m_data.reversed
                                  ? m_data.center + m_data.majorP
                                  : m_data.center - m_data.majorP;
  if (secondaryVertex.valid && secondaryVertex.distanceTo(primaryVertex) > RS_TOLERANCE) {
    sol.push_back(secondaryVertex);
  }

  // Foci
  RS_Vector f1 = m_data.getFocus1();
  RS_Vector f2 = m_data.getFocus2();
  if (f1.valid)
    sol.push_back(f1);
  if (f2.valid)
    sol.push_back(f2);

  // Start and end points (only for bounded arcs)
  if (std::abs(m_data.angle1) >= RS_TOLERANCE ||
      std::abs(m_data.angle2) >= RS_TOLERANCE) {
    RS_Vector start = getStartpoint();
    RS_Vector end = getEndpoint();

    if (start.valid)
      sol.push_back(start);
    if (end.valid)
      sol.push_back(end);
  }

  return sol;
}

//=====================================================================
RS_Vector LC_Hyperbola::getStartpoint() const {
  if (isInfinite())
    return RS_Vector(false);
  return getPoint(m_data.angle1, m_data.reversed);
}

//=====================================================================
RS_Vector LC_Hyperbola::getEndpoint() const {
  if (isInfinite())
    return RS_Vector(false);
  return getPoint(m_data.angle2, m_data.reversed);
}

/**
 * @brief getMiddlePoint
 * Returns the true midpoint of the bounded hyperbola arc measured by arc length.
 *
 * This method computes the point exactly halfway along the curve (arc length L/2),
 * not the Euclidean midpoint of the chord between endpoints.
 *
 * Use cases:
 * - Placing dimension text/arrows at the center of the arc
 * - Providing a symmetric grip point for stretching or modifying the hyperbola
 * - Visual indicators (e.g., selection highlight) at the curve's middle
 *
 * Behavior:
 * - For bounded arcs: returns the point at arc distance total_length / 2 from either endpoint
 *   (uses getNearestDist() internally for high-precision location)
 * - For unbounded hyperbolas (angle1 ≈ angle2 ≈ 0): returns RS_Vector(false)
 *   because an infinite branch has no defined midpoint
 * - Dummy coordinate (center) is passed to getNearestDist() because side selection
 *   is irrelevant for the true midpoint
 *
 * @return Point at the arc-length midpoint, or RS_Vector(false) if unbounded or invalid
 */
RS_Vector LC_Hyperbola::getMiddlePoint() const
{
  if (!m_valid) {
    return RS_Vector(false);
  }

         // Unbounded hyperbola has infinite length → no meaningful midpoint
  if (isInfinite()) {
    return RS_Vector(false);
  }

  double totalLength = getLength();
  if (std::isinf(totalLength) || totalLength <= 0.0) {
    return RS_Vector(false);
  }

         // Midpoint is at half the total arc length.
         // Use the center as a dummy coordinate — side detection is not needed for the true midpoint.
  return getNearestDist(totalLength * 0.5, m_data.center);
}

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

//=====================================================================
RS_Vector LC_Hyperbola::getTangentDirectionParam(double parameter) const {
  double a = getMajorRadius();
  double b = getMinorRadius();

  double dx = a * std::sinh(parameter);
  double dy = b * std::cosh(parameter);
  if (m_data.reversed)
    dx = -dx;

  RS_Vector tangent{dx, dy};
  tangent.rotate(m_data.majorP.angle());
  return tangent.normalized();
}

RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector &point) const {
  double phi = getParamFromPoint(point, m_data.reversed);
  return getTangentDirectionParam(phi);
}

//=====================================================================
RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector &point) const {
  if (!m_valid || !point.valid)
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
//=====================================================================
RS_Vector LC_Hyperbola::getPoint(double phi, bool useReversed) const {
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE)
    return RS_Vector(false);

  double ch = std::cosh(phi);
  double sh = std::sinh(phi);

  RS_Vector local(useReversed ? -a * ch : a * ch, b * sh);

  return localToWorld(local);
}

//=====================================================================
RS_Vector LC_Hyperbola::worldToLocal(const RS_Vector& world) const
{
  RS_Vector local = (world - getCenter()).rotated(- getAngle());
  return local;
}

//=====================================================================
RS_Vector LC_Hyperbola::localToWorld(const RS_Vector& local) const
{
  return local.rotated(getAngle()) + getCenter();
}

/**
 * @brief getParamFromPoint
 * Returns the hyperbolic parameter φ corresponding to a point lying on the hyperbola.
 *
 * This method recovers the parametric angle φ from a point p that lies on the hyperbola.
 * It handles both branches correctly using the sign of the x-coordinate in local space.
 *
 * The hyperbola is defined as:
 *   x = a * cosh(φ)
 *   y = b * sinh(φ)     (right branch, reversed = false)
 *   x = -a * cosh(φ)
 *   y = b * sinh(φ)     (left branch, reversed = true)
 *
 * The implementation uses the stable and exact formula:
 *   φ = asinh(y_local / b)
 *
 * Then verifies consistency with x_local using cosh(φ), with proper handling of the branch.
 *
 * This approach avoids quartics, tanh substitution, and logarithmic forms that can
 * suffer from cancellation or overflow. It is numerically robust for all eccentricities,
 * including rectangular (b/a ≈ 1) and highly eccentric cases.
 *
 * @param p               Point on the hyperbola
 * @param branchReversed  Ignored — branch is automatically detected from geometry
 * @return                Hyperbolic parameter φ, or NaN if point is not on the hyperbola
 */
double LC_Hyperbola::getParamFromPoint(const RS_Vector& p,
                                       bool /*branchReversed*/) const
{
  if (!m_valid || !p.valid) {
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
  RS_Vector local = p - m_data.center;
  local.rotate(-m_data.majorP.angle());  // inverse rotation

  // Branch sign: getPoint(phi, true) emits local x = -a·cosh(phi), so the
  // left branch lives in local.x < 0. Recover phi from |local.x| / a (cosh)
  // rather than rejecting negative x outright. y-recovery is branch-agnostic
  // since sinh is odd. Returning a non-NaN φ for either branch is what every
  // caller (moveRef, moveStartpoint/moveEndpoint, scale, mirror, prepareTrim,
  // getNearestPointOnEntity initial guess) actually needs.

         // Primary recovery: φ from y-coordinate (sinh is odd and strictly increasing)
  double y_local = local.y;
  double sinh_phi = y_local / b;
  double phi = std::asinh(sinh_phi);

  //        // Reconstruct expected x from φ
  // double cosh_phi = std::cosh(phi);
  // double x_expected = a * cosh_phi;

  //        // Determine which branch the point belongs to by sign of x_local
  //        // m_data.reversed == true means left branch (x negative in local coords)
  // //bool pointOnLeftBranch = (x_local < 0.0);

  //        // Expected x sign based on m_data.reversed
  // double expectedSign = m_data.reversed ? -1.0 : 1.0;

  //        // Check consistency: reconstructed |x| should match, and sign should align with branch
  // double x_expected_signed = expectedSign * x_expected;

  // if (std::abs(x_local - x_expected_signed) > RS_TOLERANCE * a) {
  //   // Point does not lie on this hyperbola branch
  //   return std::numeric_limits<double>::quiet_NaN();
  // }

  //        // For left branch (reversed=true), φ is defined such that cosh(φ) is still positive,
  //        // so the same φ works for both branches — no sign flip needed
  return phi;
}

//=====================================================================
bool LC_Hyperbola::isInClipRect(const RS_Vector &p, const LC_Rect& rect) const {
  return p.valid && rect.inArea(p);
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

  painter->save();
  std::shared_ptr<double> painterRestore{&a, [painter](void*) {
      painter->restore(); }};

  double guiPixel = std::min(painter->toGuiDX(1.0), painter->toGuiDY(1.0));
  double maxWorldError = 1.0 / guiPixel;

  std::vector<RS_Vector> pts;
  pts.reserve(300);

  bool isFull = isInfinite();

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
        if (std::isnan(phiCur) || phiCur < m_data.angle1 || phiCur > m_data.angle2)
          continue;
        if (isInClipRect(intersection, clip)) {
          params.push_back(phiCur);
        }
      }
    }

    if (params.empty()) {
      RS_Vector test = getPoint((m_data.angle1 + m_data.angle2) * 0.5, rev);
      if (test.valid && isInClipRect(test, clip)) {
        params = {m_data.angle1, m_data.angle2};
      } else {
        return;
      }
    } else {
      params.push_back(m_data.angle1);
      params.push_back(m_data.angle2);
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
      if (isInClipRect(middle, clip)) {
        adaptiveSample(pts, start, end, rev, maxWorldError);
        painter->drawSplinePointsWCS(pts, false);
      }
    }
  };

  if (isFull) {
    processBranch(false);
    processBranch(true);
  } else {
    processBranch(m_data.reversed);
  }
}

//=====================================================================
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
/**
 * @brief getNearestMiddle
 * Returns the point on the hyperbola arc that is closest to the given coordinate
 * when considering only the middle portion of the arc (by arc length).
 *
 * This method is used by the CAD engine to provide a "middle grip" or snap point
 * that is biased toward the central part of the curve, rather than the endpoints.
 * It prevents accidental snapping to endpoints when the user intends to select
 * or modify the middle of a long hyperbola arc.
 *
 * Behavior:
 * - Computes the total arc length L.
 * - Defines the "middle zone" as the central 50% of the arc length
 *   (i.e., from L*0.25 to L*0.75 measured from the startpoint).
 * - Finds the point on the hyperbola closest to `coord`.
 * - If that nearest point lies within the middle zone → returns it directly.
 * - Otherwise, clamps to the nearest boundary of the middle zone
 *   (L*0.25 or L*0.75 from start).
 *
 * This ensures the returned point is always in the true middle half of the arc,
 * providing stable and predictable behavior for selection, stretching, and snapping.
 *
 * @param coord          Coordinate (usually mouse position) to measure closeness from
 * @param dist           Optional: receives the Euclidean distance to the returned point
 * @param middlePoints   Number of middle points requested (currently only 1 is supported)
 * @return               Point in the middle 50% of the arc closest to `coord`,
 *                       or RS_Vector(false) if hyperbola is invalid/unbounded
 */
RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector& coord,
                                         double* dist,
                                         int middlePoints) const
{
  Q_UNUSED(middlePoints);  // Only one middle point is provided

  if (!m_valid) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

         // Unbounded hyperbola has no defined middle
  if (isInfinite()) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

  double totalLength = getLength();
  if (std::isinf(totalLength) || totalLength <= 0.0) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

         // Define middle zone: central 50% of arc length
  double middleStart = totalLength * 0.25;
  double middleEnd   = totalLength * 0.75;

         // Find geometrically closest point on the entire arc
  RS_Vector nearest = getNearestPointOnEntity(coord, true);
  if (!nearest.valid) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

         // Compute arc distance from startpoint to the nearest point
  double phi_nearest = getParamFromPoint(nearest, m_data.reversed);
  if (std::isnan(phi_nearest)) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

  double arcToNearest = std::abs(getArcLength(m_data.angle1, phi_nearest));

  double targetArcFromStart;
  if (arcToNearest >= middleStart && arcToNearest <= middleEnd) {
    // Nearest point is already in middle zone → use it
    targetArcFromStart = arcToNearest;
  } else if (arcToNearest < middleStart) {
    // Too close to start → clamp to beginning of middle zone
    targetArcFromStart = middleStart;
  } else {
    // Too close to end → clamp to end of middle zone
    targetArcFromStart = middleEnd;
  }

         // Use existing high-precision method to get point at target arc distance
         // Dummy coordinate (center) — side selection irrelevant since we specify exact distance
  RS_Vector middlePoint = getNearestDist(targetArcFromStart, m_data.center);

  if (dist) {
    *dist = coord.distanceTo(middlePoint);
  }

  return middlePoint;
}

/**
 * @brief getNearestOrthTan
 * Returns the point on the hyperbola where the tangent is orthogonal to the given normal line.
 *
 * This implements orthogonal tangent snapping using conic pole-polar duality:
 * - The normal line through coord is interpreted as a point in dual space.
 * - The polar line of this point with respect to the hyperbola is computed using the dual conic.
 * - The polar line is tangent to the hyperbola.
 * - The point of tangency is returned.
 *
 * The dual conic is obtained via LC_Quadratic::getDualCurve() (normalized to constant +1).
 *
 * @param coord   Coordinate (usually mouse position) defining the normal direction
 * @param normal  Normal line direction (interpreted as line through origin in dual space)
 * @param onEntity Restrict to bounded arc if true
 * @return Point of tangency on hyperbola, or invalid if no real tangent
 */
/**
 * @brief getNearestOrthTan
 * Returns the point on the hyperbola where the tangent is orthogonal to the given normal line.
 *
 * Uses conic pole-polar duality:
 * - The normal line is interpreted as a point in dual space.
 * - The polar line of this point w.r.t. the hyperbola is computed using the dual conic.
 * - The polar line is tangent to the hyperbola.
 * - The point of tangency is found using the existing dualLineTangentPoint() method.
 *
 * The dual conic is normalized to constant term +1 to match the line form u x + v y + 1 = 0
 * used in dualLineTangentPoint().
 *
 * @param coord   Coordinate (usually mouse position) — not used directly
 * @param normal  Normal line (direction defines the required tangent orientation)
 * @param onEntity Restrict to bounded arc if true (handled by dualLineTangentPoint)
 * @return Point of tangency on hyperbola, or invalid if no real tangent
 */
/**
 * @brief getNearestOrthTan
 * Returns the point on the hyperbola where the tangent is orthogonal to the given normal line.

* Uses analytical parametric solution:
* - The tangent direction at φ is (dx/dφ, dy/dφ)
* - Solve for φ where tangent direction ⊥ normal direction, i.e., dx/dφ * nx + dy/dφ * ny = 0
* - Leads to tanh φ = - M / K, with M, K derived from rotation and a/b
* - Exact and efficient (no iteration)

 * Restricts to bounded arc if onEntity=true.
 *
 * @param coord   Unused (compatibility)
 * @param normal  Line whose direction is the desired normal at the point
 * @param onEntity Restrict to bounded arc
 * @return Tangent point, or invalid if no real solution or outside bounds
 */
RS_Vector LC_Hyperbola::getNearestOrthTan(const RS_Vector& /*coord*/,
                                          const RS_Line& normal,
                                          bool onEntity) const
{
  if (!m_valid)
    return RS_Vector(false);

  RS_Vector n{normal.getDirection1()};

  n = n.normalized();

  double cos_th = std::cos(m_data.majorP.angle());
  double sin_th = std::sin(m_data.majorP.angle());
  double a = getMajorRadius();
  double b = getMinorRadius();
  int sign_x = m_data.reversed ? -1 : 1;

  double K = sign_x * a * (cos_th * n.x + sin_th * n.y);
  double M = b * (-sin_th * n.x + cos_th * n.y);

  if (std::abs(K) < RS_TOLERANCE) return RS_Vector(false);  // no solution

  double tanh_phi = - M / K;

  if (std::abs(tanh_phi) >= 1.0 - RS_TOLERANCE) return RS_Vector(false);

  double phi = std::atanh(tanh_phi);

  if (onEntity && !isInfinite()) {
    double phi_min = std::min(m_data.angle1, m_data.angle2);
    double phi_max = std::max(m_data.angle1, m_data.angle2);
    if (phi < phi_min - RS_TOLERANCE || phi > phi_max + RS_TOLERANCE) return RS_Vector(false);
  }

  return getPoint(phi, m_data.reversed);
}

bool LC_Hyperbola::isInfinite() const
{
  return RS_Math::equal(m_data.angle1, 0.) &&
         RS_Math::equal(m_data.angle2, 0.);
}


// Directed arc length from phi1 to phi2 (signed based on order)
double LC_Hyperbola::getArcLength(double phi1, double phi2) const {
  if (!m_valid)
    return 0.0;

  if (isInfinite())
    return RS_MAXDOUBLE;

  bool forward = phi2 > phi1;
  double p_min = std::min(phi1, phi2);
  double p_max = std::max(phi1, phi2);

  double a = getMajorRadius();
  double ecc = getEccentricity();
  double ecc2 = ecc * ecc;

  auto integrand = [a, ecc2](double phi) -> double {
    double ch = std::cosh(phi);
    double inner = std::max(0., ecc2 * ch * ch - 1.0);
    return a * std::sqrt(inner);
  };

  double result = 0.0;
  double abs_error = 0.0;

  // Split at zero if interval contains the vertex (singularity point)
  if (p_min < 0.0 && p_max > 0.0) {
    double part1 =
        boost::math::quadrature::gauss_kronrod<double, 61>::integrate(
            integrand, p_min, 0.0, 0, 1e-12, &abs_error);
    double part2 =
        boost::math::quadrature::gauss_kronrod<double, 61>::integrate(
            integrand, 0.0, p_max, 0, 1e-12, &abs_error);
    result = part1 + part2;
  } else {
    result = boost::math::quadrature::gauss_kronrod<double, 61>::integrate(
        integrand, p_min, p_max, 0, 1e-12, &abs_error);
  }

  return forward ? result : -result;
}

/**
 * @brief getNearestDist
 * Returns the point on the bounded hyperbola arc at the specified arc-length
 * distance from the endpoint closest to the provided coordinate.
 *
 * Uses Newton-Raphson with initial guess from nearest point and direction-aware extrapolation.
 * Falls back to bisection if Newton does not converge.

 * @param distance   Desired arc-length distance from reference endpoint
 * @param coord      Coordinate to select reference side
 * @param dist       Optional: computed arc distance from start to returned point
 * @return Point at requested distance, or invalid on failure
 */
RS_Vector LC_Hyperbola::getNearestDist(double distance,
                                       const RS_Vector& coord,
                                       double* dist) const
{
  if (!m_valid || isInfinite()) return RS_Vector(false);

  double totalLength = getLength();
  if (totalLength <= std::abs(distance))
    return RS_Vector(false);

  double phi0 = getParamFromPoint(coord, m_data.reversed);
  const bool fromStart = std::abs(phi0 - m_data.angle1) <= std::abs(phi0 - m_data.angle2);

  double targetArcFromStart = fromStart ? distance : totalLength - distance;

  if (distance < 0.0 || targetArcFromStart < 0.0 || targetArcFromStart > totalLength + RS_TOLERANCE)
    return RS_Vector(false);

  if (dist)
    *dist = targetArcFromStart;

  double a = getMajorRadius();
  double ecc2 = getEccentricity() * getEccentricity();

         // Initial guess from nearest point on curve

  using std::asinh, std::cosh, std::sinh;
  double phi = asinh(sinh(m_data.angle1) + targetArcFromStart / totalLength * (sinh(m_data.angle2) - sinh(m_data.angle1)));
  if (std::isnan(phi))
    phi = m_data.angle1;

  constexpr int maxIter = 30;
  constexpr double tol = 1e-12;

  bool converged = false;
  for (int i = 0; i < maxIter; ++i) {
    double s = getArcLength(m_data.angle1, phi);
    double ds_dphi_current = a * std::sqrt(ecc2 * cosh(phi) * cosh(phi) - 1.0);
    if (ds_dphi_current < RS_TOLERANCE) break;

    double residual = targetArcFromStart - s;
    double delta = residual / ds_dphi_current;
    phi += delta;

    // LC_LOG<<__func__<<"(): "<<i<<": phi="<<phi<<", "<<s<<"("<<targetArcFromStart<<"): "<<residual;
    if (std::abs(delta) < tol) {
      converged = true;
      break;
    }
  }

         // Bisection fallback if Newton fails
  if (!converged) {
    double phiLow = std::min(m_data.angle1, m_data.angle2) - 30.0;
    double phiHigh = std::max(m_data.angle1, m_data.angle2) + 30.0;

    double sLow = getArcLength(m_data.angle1, phiLow);
    double sHigh = getArcLength(m_data.angle1, phiHigh);

    while (sLow > targetArcFromStart) { phiLow -= 30.0; sLow = getArcLength(m_data.angle1, phiLow); }
    while (sHigh < targetArcFromStart) { phiHigh += 30.0; sHigh = getArcLength(m_data.angle1, phiHigh); }

    for (int i = 0; i < 80; ++i) {
      phi = 0.5 * (phiLow + phiHigh);
      double s = getArcLength(m_data.angle1, phi);
      if (std::abs(s - targetArcFromStart) < 1e-9) break;

      if (s < targetArcFromStart) phiLow = phi;
      else phiHigh = phi;
    }
  }

  return getPoint(phi, m_data.reversed);
}

//=====================================================================
// Transformations
//=====================================================================

void LC_Hyperbola::move(const RS_Vector &offset) {
  m_data.center += offset;
  calculateBorders();
}

void LC_Hyperbola::rotate(const RS_Vector &center, double angle) {
  rotate(center, RS_Vector{angle});
}

void LC_Hyperbola::rotate(const RS_Vector &center,
                          const RS_Vector &angleVector) {
  m_data.center.rotate(center, angleVector);
  m_data.majorP.rotate(angleVector);
  calculateBorders();
  updateLength();
}

void LC_Hyperbola::scale(const RS_Vector &center, const RS_Vector &factor) {
  // NOTE: assigning `*this = LC_Hyperbola{...}` would clobber base-class
  // state (parent, layer, pen, selection). Mutate `m_data` in place instead
  // so the entity keeps its provenance.
  // NOTE: non-uniform scaling of a hyperbola does not generally yield a
  // hyperbola — same caveat as RS_Ellipse. We approximate by re-deriving the
  // hyperbola through the scaled foci + scaled start point.
  RS_VectorSolutions foci = getFoci();
  RS_Vector vpStart = isInfinite() ? getPrimaryVertex() : getStartpoint();
  RS_Vector vpEnd   = isInfinite() ? RS_Vector{false}    : getEndpoint();
  foci.scale(center, factor);
  vpStart.scale(center, factor);
  if (vpEnd.valid) vpEnd.scale(center, factor);

  LC_HyperbolaData newData{foci[0], foci[1], vpStart};
  if (newData.majorP.squared() < RS_TOLERANCE2)
    return;  // degenerate scaling result (e.g. collinear foci) — leave unchanged
  m_data = newData;
  m_valid = true;

  if (vpEnd.valid) {
    const double phiStart = getParamFromPoint(vpStart, m_data.reversed);
    const double phiEnd   = getParamFromPoint(vpEnd,   m_data.reversed);
    if (!std::isnan(phiStart)) m_data.angle1 = phiStart;
    if (!std::isnan(phiEnd))   m_data.angle2 = phiEnd;
    if (m_data.angle1 > m_data.angle2)
      std::swap(m_data.angle1, m_data.angle2);
  }

  calculateBorders();
  updateLength();
}

void LC_Hyperbola::mirror(const RS_Vector &axisPoint1,
                          const RS_Vector &axisPoint2) {
  if (axisPoint1 == axisPoint2)
    return;
  RS_Vector vpStart = getStartpoint();
  RS_Vector vpEnd = getEndpoint();
  auto mirrorFunc = [&axisPoint1, &axisPoint2](RS_Vector& vp) {
    return vp.mirror(axisPoint1, axisPoint2);
  };
  mirrorFunc(m_data.center);
  m_data.majorP.mirror(RS_Vector(0, 0), axisPoint2 - axisPoint1);
  // m_data.reversed = !m_data.reversed;
  m_data.angle2 = getParamFromPoint(mirrorFunc(vpStart));
  m_data.angle1 = getParamFromPoint(mirrorFunc(vpEnd));
  if (m_data.angle1 > m_data.angle2)
    std::swap(m_data.angle1, m_data.angle2);

  LC_Hyperbola::calculateBorders();
  LC_Hyperbola::updateLength();
}

RS_Entity &LC_Hyperbola::shear(double k) {
  // Shearing a hyperbola in general does not preserve the standard form
  // (the result can be a non-axis-aligned hyperbola or, for some k, a
  // different conic). We approximate by shearing the data members
  // analogously to the ellipse — center and majorP — and re-deriving via
  // calculateBorders. Same caveat as non-uniform scaling.
  m_data.center.shear(k);
  m_data.majorP.shear(k);
  calculateBorders();
  updateLength();
  return *this;
}

void LC_Hyperbola::revertDirection() {
  // Swap parameter bounds so traversal goes the other way along the branch.
  std::swap(m_data.angle1, m_data.angle2);
}

//=====================================================================
// Minimal overrides
//=====================================================================
RS_Vector LC_Hyperbola::getNearestCenter(const RS_Vector &coord,
                                         double *dist) const {
  if (!m_valid || !coord.valid) {
    if (dist) *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }
  if (dist) *dist = coord.distanceTo(m_data.center);
  return m_data.center;
}

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector &coord,
                                           double *dist) const {
  if (dist)
    *dist = RS_MAXDOUBLE;
  if (!m_valid || !coord.valid) {
    return RS_Vector(false);
  }

  // For unbounded hyperbolas (full branch), there are no defined endpoints
  if (isInfinite()) {
    return RS_Vector(false);
  }

  double distance = RS_MAXDOUBLE;
  RS_Vector ret{false};
  for (const RS_Vector &vp : {getStartpoint(), getEndpoint()}) {
    if (vp.valid) {
      double dvp = vp.distanceTo(coord);
      if (dvp < distance) {
        distance = dvp;
        ret = vp;
      }
    }
  }
  if (dist != nullptr)
    *dist = distance;
  return ret;
}

//=====================================================================
RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector &coord,
                                                bool onEntity, double *dist,
                                                RS_Entity **entity) const {
  if (!m_valid || !coord.valid) {
    if (dist)
      *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }

  if (entity)
    *entity = const_cast<LC_Hyperbola *>(this);

  // For unbounded (full-branch) hyperbolas, fall through to the same
  // parametric quartic search as bounded arcs but with very wide phi bounds
  // (cosh(30) already saturates double precision). The previous heuristic
  // returned the vertex regardless of `coord`, which is wrong for any point
  // not near the vertex.
  // TODO: full hyperbolas have two branches; this implementation searches the
  // active `m_data.reversed` branch. A future improvement should also
  // evaluate the opposite branch and return the closer candidate.
  const bool unbounded = isInfinite();

  // Initial parametric guess from coord projection
  double phiGuess = getParamFromPoint(coord, m_data.reversed);
  if (std::isnan(phiGuess)) {
    phiGuess = unbounded ? 0.0 : (m_data.angle1 + m_data.angle2) * 0.5;
  }

  // Effective arc range: real bounds for arcs, wide finite range for full curves.
  double phiMin = unbounded ? -30.0 : std::min(m_data.angle1, m_data.angle2);
  double phiMax = unbounded ?  30.0 : std::max(m_data.angle1, m_data.angle2);
  phiGuess = std::max(phiMin, std::min(phiMax, phiGuess));

  // Evaluate distance squared at endpoints and initial guess
  RS_Vector pStart = getPoint(m_data.angle1, m_data.reversed);
  RS_Vector pEnd = getPoint(m_data.angle2, m_data.reversed);
  RS_Vector pGuess = getPoint(phiGuess, m_data.reversed);

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
  double cx = m_data.center.x, cy = m_data.center.y;
  double aa = m_data.majorP.magnitude(); // semi-major a
  double bb = aa * m_data.ratio;         // semi-minor b
  double ct = std::cos(m_data.majorP.angle());
  double st = std::sin(m_data.majorP.angle());

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

    RS_Vector cand = getPoint(phi, m_data.reversed);
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

//=====================================================================
double LC_Hyperbola::getDistanceToPoint(const RS_Vector &coord,
                                        RS_Entity **entity,
                                        RS2::ResolveLevel /*level*/,
                                        double /*solidDist*/) const {
  if (entity)
    *entity = nullptr;

  if (!m_valid || !coord.valid) {
    return RS_MAXDOUBLE;
  }

  double dist = RS_MAXDOUBLE;
  getNearestPointOnEntity(coord, true, &dist, entity);

  if (entity && *entity == nullptr && dist < RS_MAXDOUBLE) {
    *entity = const_cast<LC_Hyperbola *>(this);
  }

  return dist;
}

//=====================================================================
bool LC_Hyperbola::isPointOnEntity(const RS_Vector &coord,
                                   double tolerance) const {
  if (!m_valid || !coord.valid)
    return false;

  double dist = RS_MAXDOUBLE;
  getNearestPointOnEntity(coord, true, &dist);
  return dist <= tolerance;
}

//=====================================================================
LC_Quadratic LC_Hyperbola::getQuadratic() const {
  std::vector<double> ce(6, 0.);
  ce[0] = m_data.majorP.squared();
  ce[2] = -m_data.ratio * m_data.ratio * ce[0];
  if (ce[0] < RS_TOLERANCE2 && std::abs(ce[2]) < RS_TOLERANCE2) {
    return LC_Quadratic();
  }
  ce[0] = 1. / ce[0];
  ce[2] = 1. / ce[2];
  ce[5] = -1.;
  LC_Quadratic ret(ce);
  ret.rotate(getAngle());
  ret.move(m_data.center);
  return ret;
}

//=====================================================================
void LC_Hyperbola::calculateBorders() {
  minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
  maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);

  if (!m_valid)
    return;

  // Full unbounded hyperbola → infinite bounds
  if (isInfinite()) {
    minV = RS_Vector(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
    maxV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    return;
  }

  // Limited arc on single branch
  double phiStart = m_data.angle1;
  double phiEnd = m_data.angle2;

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
        RS_Vector p = getPoint(phi_cand, m_data.reversed);
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
  RS_Vector start = getPoint(phiStart, m_data.reversed);
  RS_Vector end = getPoint(phiEnd, m_data.reversed);
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

//=====================================================================
double LC_Hyperbola::getLength() const {
  if (!m_valid)
    return 0.0;

  return getArcLength(m_data.angle1, m_data.angle2);
}

void LC_Hyperbola::updateLength() {
  cachedLength = LC_Hyperbola::getLength();
}

//=====================================================================
void LC_Hyperbola::setFocus1(const RS_Vector &f1) {
  if (!f1.valid || !m_valid)
    return;

  // Capture original arc bounds before rebuilding so we can re-project them
  // onto the new geometry (mirrors the moveRef pattern). The constructor
  // resets angle1/angle2 to 0, so without this the bounded arc would be
  // silently widened to the full unbounded hyperbola.
  const RS_Vector originalStart = getStartpoint();
  const RS_Vector originalEnd   = getEndpoint();
  const bool hadBounds = originalStart.valid && originalEnd.valid;

  RS_Vector f2 = m_data.getFocus2();
  RS_Vector currentPoint = getPoint(0.0, m_data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !m_data.reversed);
  }
  if (!currentPoint.valid)
    return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    m_data = newData;
    m_valid = true;
    if (hadBounds) {
      const double phiStart = getParamFromPoint(originalStart, m_data.reversed);
      const double phiEnd   = getParamFromPoint(originalEnd,   m_data.reversed);
      if (!std::isnan(phiStart)) m_data.angle1 = phiStart;
      if (!std::isnan(phiEnd))   m_data.angle2 = phiEnd;
      if (m_data.angle1 > m_data.angle2)
        std::swap(m_data.angle1, m_data.angle2);
    }
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setFocus2(const RS_Vector &f2) {
  if (!f2.valid || !m_valid)
    return;

  const RS_Vector originalStart = getStartpoint();
  const RS_Vector originalEnd   = getEndpoint();
  const bool hadBounds = originalStart.valid && originalEnd.valid;

  RS_Vector f1 = m_data.getFocus1();
  RS_Vector currentPoint = getPoint(0.0, m_data.reversed);
  if (!currentPoint.valid) {
    currentPoint = getPoint(0.0, !m_data.reversed);
  }
  if (!currentPoint.valid)
    return;

  LC_HyperbolaData newData(f1, f2, currentPoint);
  if (newData.isValid()) {
    m_data = newData;
    m_valid = true;
    if (hadBounds) {
      const double phiStart = getParamFromPoint(originalStart, m_data.reversed);
      const double phiEnd   = getParamFromPoint(originalEnd,   m_data.reversed);
      if (!std::isnan(phiStart)) m_data.angle1 = phiStart;
      if (!std::isnan(phiEnd))   m_data.angle2 = phiEnd;
      if (m_data.angle1 > m_data.angle2)
        std::swap(m_data.angle1, m_data.angle2);
    }
    calculateBorders();
    updateLength();
  }
}

void LC_Hyperbola::setPointOnCurve(const RS_Vector &p) {
  if (!p.valid || !m_valid)
    return;

  const RS_Vector originalStart = getStartpoint();
  const RS_Vector originalEnd   = getEndpoint();
  const bool hadBounds = originalStart.valid && originalEnd.valid;

  RS_Vector f1 = m_data.getFocus1();
  RS_Vector f2 = m_data.getFocus2();

  LC_HyperbolaData newData(f1, f2, p);
  if (newData.isValid()) {
    m_data = newData;
    m_valid = true;
    if (hadBounds) {
      const double phiStart = getParamFromPoint(originalStart, m_data.reversed);
      const double phiEnd   = getParamFromPoint(originalEnd,   m_data.reversed);
      if (!std::isnan(phiStart)) m_data.angle1 = phiStart;
      if (!std::isnan(phiEnd))   m_data.angle2 = phiEnd;
      if (m_data.angle1 > m_data.angle2)
        std::swap(m_data.angle1, m_data.angle2);
    }
    calculateBorders();
    updateLength();
  }
}

//=====================================================================
void LC_Hyperbola::setRatio(double r) {
  if (r <= 0.0 || !m_valid)
    return;
  m_data.ratio = r;
  calculateBorders();
  updateLength();
}

void LC_Hyperbola::setMinorRadius(double b) {
  if (b <= 0.0 || !m_valid)
    return;
  double a = getMajorRadius();
  if (a >= RS_TOLERANCE) {
    m_data.ratio = b / a;
    calculateBorders();
    updateLength();
  }
}

//=====================================================================
void LC_Hyperbola::setPrimaryVertex(const RS_Vector &v) {
  if (!v.valid || !m_valid)
    return;

  RS_Vector dir = m_data.majorP;
  if (dir.squared() < RS_TOLERANCE2)
    return;
  dir.normalize();

  RS_Vector expectedVertex = m_data.reversed
                                 ? m_data.center - dir * getMajorRadius()
                                 : m_data.center + dir * getMajorRadius();

  RS_Vector offset = v - expectedVertex;
  double distanceAlongAxis = offset.dotP(dir);

  double newA = std::abs(getMajorRadius() + distanceAlongAxis);
  if (newA < RS_TOLERANCE)
    return;

  // Adjust majorP magnitude
  m_data.majorP = dir * newA;
  if (m_data.reversed)
    m_data.majorP = -m_data.majorP; // preserve direction for left branch

  calculateBorders();
  updateLength();
}

// ==========================================================================
/**
 * @brief moveRef
 * Moves a reference point (center, vertex, focus, startpoint, or endpoint) by offset.
 *
 * Supported grips:
 * - Center: translation
 * - Primary vertex: updates major axis direction/length
 * - Foci: recomputes hyperbola preserving other focus + original start point
 * - Start/endpoint: directly updates angle1/angle2 via parameter recovery
 *
 * After any change, bounded arc is preserved by re-projecting original endpoints.
 */
void LC_Hyperbola::moveRef(const RS_Vector& ref, const RS_Vector& offset)
{
  // Store original start/end points BEFORE change
  RS_Vector originalStart = getStartpoint();
  RS_Vector originalEnd   = getEndpoint();
  bool hadBounds = originalStart.valid && originalEnd.valid;

  RS_Vector newRef = ref + offset;

  if (ref.distanceTo(m_data.center) < RS_TOLERANCE) {
    m_data.center = newRef;
  }
  else if (ref.distanceTo(getPrimaryVertex()) < RS_TOLERANCE) {
    RS_Vector dir = newRef - m_data.center;
    if (dir.magnitude() > RS_TOLERANCE) {
      m_data.majorP = dir.normalized() * getMajorRadius();
    }
  }
  else if (!isInfinite()) {
    // Start or end point movement
    if (ref.distanceTo(originalStart) < RS_TOLERANCE) {
      double phi = getParamFromPoint(newRef, m_data.reversed);
      if (!std::isnan(phi)) {
        m_data.angle1 = phi;
      }
    }
    else if (ref.distanceTo(originalEnd) < RS_TOLERANCE) {
      double phi = getParamFromPoint(newRef, m_data.reversed);
      if (!std::isnan(phi)) {
        m_data.angle2 = phi;
      }
    }
    else {
      // Focus movement (fallback)
      RS_Vector f1 = getFocus1();
      RS_Vector f2 = getFocus2();

      RS_Vector fixedPoint = originalStart.valid ? originalStart : getMiddlePoint();
      if (!fixedPoint.valid) fixedPoint = getPrimaryVertex();

      if (ref.distanceTo(f1) < RS_TOLERANCE) {
        LC_HyperbolaData newData(newRef, f2, fixedPoint);
        if (newData.majorP.squared() >= RS_TOLERANCE2) {
          m_data = newData;
        }
      }
      else if (ref.distanceTo(f2) < RS_TOLERANCE) {
        LC_HyperbolaData newData(f1, newRef, fixedPoint);
        if (newData.majorP.squared() >= RS_TOLERANCE2) {
          m_data = newData;
        }
      }
      else {
        return;  // Not recognized
      }

             // Re-project original endpoints after focus move
      if (hadBounds) {
        double phiStart = getParamFromPoint(originalStart, m_data.reversed);
        double phiEnd   = getParamFromPoint(originalEnd, m_data.reversed);

        if (!std::isnan(phiStart)) m_data.angle1 = phiStart;
        if (!std::isnan(phiEnd))   m_data.angle2 = phiEnd;

        if (m_data.angle1 > m_data.angle2) std::swap(m_data.angle1, m_data.angle2);
      }
    }
  }

  calculateBorders();
  updateLength();
}

// ============================================================================
RS_Vector LC_Hyperbola::getPrimaryVertex() const {
  if (!m_valid) {
    return RS_Vector(false);
  }

  double a = getMajorRadius();
  if (a < RS_TOLERANCE) {
    return RS_Vector(false);
  }

  // majorP already contains the vector from center to the right-branch vertex
  // with magnitude = a and correct direction
  RS_Vector vertex = m_data.center + m_data.majorP;

  if (m_data.reversed) {
    // For left branch, the primary vertex is on the opposite side
    vertex = m_data.center - m_data.majorP;
  }

  return vertex;
}

//=====================================================================
// Grip editing: move start/end points
//=====================================================================
void LC_Hyperbola::moveStartpoint(const RS_Vector &pos) {
  if (!m_valid || !pos.valid)
    return;

  // Unbounded hyperbolas have no defined endpoints
  if (isInfinite()) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "LC_Hyperbola::moveStartpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newStart = getNearestPointOnEntity(pos, true);
  if (!newStart.valid)
    return;

  // Trim semantics: set the start parameter to the new point and leave the
  // end parameter untouched. (Earlier code preserved the angular span,
  // which slid the entire arc instead of trimming one end.)
  const double newPhi1 = getParamFromPoint(newStart, m_data.reversed);
  if (std::isnan(newPhi1))
    return;
  m_data.angle1 = newPhi1;

  calculateBorders();
  updateLength();
}

//=====================================================================
void LC_Hyperbola::moveEndpoint(const RS_Vector &pos) {
  if (!m_valid || !pos.valid)
    return;

  if (isInfinite()) {
    RS_DEBUG->print(
        RS_Debug::D_WARNING,
        "LC_Hyperbola::moveEndpoint: ignored on unbounded hyperbola");
    return;
  }

  RS_Vector newEnd = getNearestPointOnEntity(pos, true);
  if (!newEnd.valid)
    return;

  const double newPhi2 = getParamFromPoint(newEnd, m_data.reversed);
  if (std::isnan(newPhi2))
    return;
  m_data.angle2 = newPhi2;

  calculateBorders();
  updateLength();
}

//=====================================================================
// Area calculation support (Green's theorem)
//=====================================================================
/**
 * @brief areaLineIntegral
 * Computes ∮ x dy along the hyperbola arc using exact analytical formula.
 *
 * @return Signed line integral ∮ x dy
 */
double LC_Hyperbola::areaLineIntegral() const
{
  if (!m_valid || isInfinite()) return 0.0;

  double phi1 = m_data.angle1;
  double phi2 = m_data.angle2;

  double a = getMajorRadius();
  double b = getMinorRadius();
  double a2 = a*a;
  double b2 = b*b;
  double cx = m_data.center.x;
  //double cy = m_data.center.y;
  double cos_th = std::cos(m_data.majorP.angle());
  double sin_th = std::sin(m_data.majorP.angle());
  double cos2_th = cos_th*cos_th - sin_th*sin_th;
  double sin2_th = 2. * cos_th*sin_th;

  double R = a * sin_th;
  double S = b * cos_th;

  double c1 = (a2 - b2)/8.;
  double c2 = a * b / 4.;
  double c3 = a * b /2.;

  // The undetermined integral function for \(\int x\,dy\) is
//  \(\mathbf{F(t)=}\frac{\mathbf{a}^{\mathbf{2}}\mathbf{-b}^{\mathbf{2}}}{\mathbf{8}}\sin \mathbf{(2\alpha )}\cosh \mathbf{(2t)+
// }\frac{\mathbf{ab}}{\mathbf{4}}\cos \mathbf{(2\alpha )}\sinh \mathbf{(2t)+
// }\frac{\mathbf{ab}}{\mathbf{2}}\mathbf{t+
  //c}_{\mathbf{x}}\mathbf{(a}\sin \mathbf{\alpha }\cosh \mathbf{t+b}\cos \mathbf{\alpha }\sinh \mathbf{t)+C}\)
  // Reversed branch: x_local = -a·cosh φ flips the sign of every term that
  // carries a single power of `a` from the local x coordinate. The cxTerm
  // also flips because R/S sit in front of the local-x and local-y
  // contributions to world x.
  const double sx = m_data.reversed ? -1.0 : 1.0;
  auto primitive = [&](double phi) -> double {
    double c1Term = c1 * sin2_th * std::cosh(2. * phi);
    double c2Term = c2 * cos2_th * std::sinh(2. * phi);
    double cxTerm = cx * sx * (R * std::cosh(phi) + S * std::sinh(phi));
    return sx * (c1Term + c2Term + c3 * phi) + cxTerm;
  };

  return primitive(phi2) - primitive(phi1);
}

double LC_Hyperbola::computeLocalArea(double phi1, double phi2) const {
  if (isInfinite()) return 0.0;
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  // areaLineIntegral = ∫ x dy ∝ ab cosh², so for the left branch (x_local =
  // -a·cosh) the entire integrand picks up a -1.
  const double sx = m_data.reversed ? -1.0 : 1.0;
  auto F = [&](double phi) {
    return sx * (a * b / 2.0) * (phi + 0.5 * std::sinh(2.0 * phi));
  };
  return F(phi2) - F(phi1);
}

LC_FirstMoment LC_Hyperbola::computeLocalFirstMoment(double phi1, double phi2) const {
  // First moments via Green's theorem with x = a·cosh φ, y = b·sinh φ:
  //   mx = ½ ∫ x² dy = (a²b/2) ∫ cosh³ φ dφ      = (a²b/2)·(sinh φ + sinh³ φ / 3)
  //   my = -½ ∫ y² dx = -(ab²/2) ∫ sinh³ φ dφ    = -(ab²/2)·(cosh³ φ / 3 − cosh φ)
  // For the reversed (left) branch x_local = -a·cosh, so x² is unchanged
  // (mx unaffected) and x doesn't appear in my (also unaffected).
  if (isInfinite()) return {};
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  auto F_mx = [&](double phi) {
    const double sh = std::sinh(phi);
    return (a * a * b / 2.0) * (sh + sh * sh * sh / 3.0);
  };
  auto F_my = [&](double phi) {
    const double ch = std::cosh(phi);
    return -(a * b * b / 2.0) * (ch * ch * ch / 3.0 - ch);
  };
  return {F_mx(phi2) - F_mx(phi1), F_my(phi2) - F_my(phi1)};
}

LC_SecondMoment LC_Hyperbola::computeLocalSecondMoment(double phi1, double phi2) const {
  // Second moments via Green's theorem with x = a·cosh φ, y = b·sinh φ:
  //   ixx =  ⅓ ∫ x³ dy   =  (a³b/24)·(3φ + 2 sinh 2φ + ¼ sinh 4φ)
  //   iyy = -⅓ ∫ y³ dx   = -(ab³/24)·(3φ − 2 sinh 2φ + ¼ sinh 4φ)
  //   ixy =  ½ ∫ x² y dy =  (a²b²/8)·cosh⁴ φ
  // For reversed (left) branch x_local = -a·cosh, so x³ flips sign (ixx) but
  // x² does not (iyy and ixy unaffected).
  if (isInfinite()) return {};
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  const double sx = m_data.reversed ? -1.0 : 1.0;
  auto F_ixx = [&](double phi) {
    const double s2 = std::sinh(2.0 * phi);
    const double s4 = std::sinh(4.0 * phi);
    return sx * (a * a * a * b / 24.0) * (3.0 * phi + 2.0 * s2 + 0.25 * s4);
  };
  auto F_iyy = [&](double phi) {
    const double s2 = std::sinh(2.0 * phi);
    const double s4 = std::sinh(4.0 * phi);
    return -(a * b * b * b / 24.0) * (3.0 * phi - 2.0 * s2 + 0.25 * s4);
  };
  auto F_ixy = [&](double phi) {
    const double ch = std::cosh(phi);
    return (a * a * b * b / 8.0) * (ch * ch * ch * ch);
  };

  LC_SecondMoment m;
  m.ixx = F_ixx(phi2) - F_ixx(phi1);
  m.iyy = F_iyy(phi2) - F_iyy(phi1);
  m.ixy = F_ixy(phi2) - F_ixy(phi1);
  return m;
}

// Parallel-axis shift used by firstMoment/secondMoment helpers below.
// We deliberately do NOT use `LC_SecondMoment::shifted` — that helper has
// `area*dy²` where `area*dx²` belongs (and vice versa for iyy), which is
// inconsistent with both the documented `ixx = ∬x²dA` convention and the
// sibling `getCentral` formula. Inlining the right transformation keeps the
// hyperbola pipeline correct without dragging the other entities (ellipse
// arcs, parabola) along.
namespace {
LC_FirstMoment shiftFirstMomentToOrigin(const LC_FirstMoment &local,
                                        double cx, double cy, double area) {
  // local moments are about a frame centered at world (cx, cy). To express
  // them about the world origin: ∫(x_local + cx) dA = ∫ x_local dA + cx·A.
  return {local.mx + cx * area, local.my + cy * area};
}
LC_SecondMoment shiftSecondMomentToOrigin(const LC_SecondMoment &local,
                                          double cx, double cy, double area) {
  return {
      local.ixx + area * cx * cx,
      local.iyy + area * cy * cy,
      local.ixy + area * cx * cy
  };
}
} // namespace

LC_FirstMoment LC_Hyperbola::firstMomentLineIntegral() const {
  if (!m_valid || isInfinite()) return {};
  const double phi = getAngle();
  const double cx  = m_data.center.x;
  const double cy  = m_data.center.y;

  // No t0/t1 swap for reversed branches: the helpers already apply the
  // x-sign factor (`effectiveSignX = -1`) so a single source of truth
  // handles the orientation. Swapping here on top of the sx flip would
  // double-correct (mx flipping sign even though x² is even, etc.).
  const double t0 = m_data.angle1;
  const double t1 = m_data.angle2;

  const auto local = computeLocalFirstMoment(t0, t1);
  const double area = computeLocalArea(t0, t1);

  return shiftFirstMomentToOrigin(local.rotated(phi), cx, cy, area);
}

LC_SecondMoment LC_Hyperbola::secondMomentLineIntegral() const {
  if (!m_valid || isInfinite()) return {};
  const double phi = getAngle();
  const double cx  = m_data.center.x;
  const double cy  = m_data.center.y;

  const double t0 = m_data.angle1;
  const double t1 = m_data.angle2;

  const auto local = computeLocalSecondMoment(t0, t1);
  const double area = computeLocalArea(t0, t1);

  return shiftSecondMomentToOrigin(local.rotated(phi), cx, cy, area);
}
//=====================================================================
RS_Vector LC_Hyperbola::dualLineTangentPoint(const RS_Vector &line) const {
  if (!m_valid || !line.valid) {
    return RS_Vector(false);
  }
  // u x + v y + 1 = 0
  // coordinates : dual
  // real coordinates is rotated from canonical
  // (u; v)^T (M X) + 1 =0
  // Equivalent to rotation in dual coordinates, but opposite angle
  // ( M^T (u; v)^T) X + 1 = 0
  RS_Vector uv = RS_Vector{line}.rotate(-m_data.majorP.angle());
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

//=====================================================================
// Trim support – updated to match LC_Parabola behavior
//=====================================================================
/**
 * @brief getTrimPoint
 * Determines which endpoint to move for trimming, based on the click position
 * relative to the chosen intersection point.
 *
 * Updated to match modern LibreCAD behavior (used by parabola, spline, etc.):
 * - The click point (trimCoord) and the chosen intersection (from prepareTrim())
 *   are used to decide whether to trim/extend the start or end.
 * - Keeps the portion containing the click point.
 *
 * @param trimCoord  Click coordinate (user's mouse position)
 * @param trimPoint  Chosen intersection point (returned by prepareTrim())
 * @return EndingStart if trimming/extending start point, EndingEnd for end point,
 *         EndingNone if invalid/unbounded
 */
RS2::Ending LC_Hyperbola::getTrimPoint(const RS_Vector& trimCoord,
                                       const RS_Vector& trimPoint)
{
  if (!m_valid || !trimPoint.valid || !trimCoord.valid || isInfinite()) {
    return RS2::EndingNone;
  }

         // Project click point onto current hyperbola arc
  RS_Vector nearest = getNearestPointOnEntity(trimCoord, true);
  if (!nearest.valid) {
    nearest = trimCoord;  // fallback
  }

  double phi_click = getParamFromPoint(nearest, m_data.reversed);
  double phi_inter = getParamFromPoint(trimPoint, m_data.reversed);

  if (std::isnan(phi_click) || std::isnan(phi_inter)) {
    // Fallback to geometric distance if param recovery fails
    RS_Vector start = getStartpoint();
    RS_Vector end   = getEndpoint();
    if (!start.valid || !end.valid) return RS2::EndingNone;

    return (nearest.distanceTo(start) < nearest.distanceTo(end))
               ? RS2::EndingStart : RS2::EndingEnd;
  }

         // Keep the side containing the click point
         // If intersection is on the "start" side of click → move startpoint
         // Otherwise → move endpoint
  return (phi_inter < phi_click) ? RS2::EndingStart : RS2::EndingEnd;
}

/**
 * @brief prepareTrim
 * Selects the intersection point closest along the branch to the click position.
 *
 * Returns the chosen intersection so getTrimPoint() can use it to decide direction.
 *
 * @param trimCoord  Click coordinate
 * @param trimSol    All intersection solutions
 * @return Chosen intersection point (closest along parametric branch to click)
 */
RS_Vector LC_Hyperbola::prepareTrim(const RS_Vector& trimCoord,
                                    const RS_VectorSolutions& trimSol)
{
  if (!m_valid || trimSol.empty() || isInfinite()) {
    return RS_Vector(false);
  }

         // Project click onto current arc to get reference parameter
  RS_Vector nearest = getNearestPointOnEntity(trimCoord, false);
  if (!nearest.valid) {
    nearest = trimCoord;
  }

  double phi_ref = getParamFromPoint(nearest, m_data.reversed);
  if (std::isnan(phi_ref)) {
    return RS_Vector(false);
  }

  RS_Vector bestSol(false);
  double minDeltaPhi = RS_MAXDOUBLE;

         // Choose intersection with smallest |Δφ| from click position
  for (const RS_Vector& intersect : trimSol) {
    if (!intersect.valid)
      continue;

    // RS_Vector proj = getNearestPointOnEntity(sol, false);
    // if (!proj.valid) proj = sol;

    double phi = getParamFromPoint(intersect, m_data.reversed);
    if (std::isnan(phi))
      continue;

    double deltaPhi = std::abs(phi - phi_ref);
    if (deltaPhi < minDeltaPhi) {
      minDeltaPhi = deltaPhi;
      bestSol = intersect;
    }
  }

  if (!bestSol.valid)
    return RS_Vector(false);

  double newPhi = getParamFromPoint(bestSol, m_data.reversed);

         // Use getTrimPoint() with the chosen intersection to decide which end to move
  RS2::Ending side = getTrimPoint(trimCoord, bestSol);

  if (side == RS2::EndingStart) {
    m_data.angle1 = newPhi;
  } else if (side == RS2::EndingEnd) {
    m_data.angle2 = newPhi;
  } else {
    return RS_Vector(false);
  }

  calculateBorders();
  updateLength();

  return bestSol;
}
