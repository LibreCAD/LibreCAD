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

#include <QPainterPath>

#include <boost/math/quadrature/gauss_kronrod.hpp>

#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_math.h"
#include "lc_rect.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

namespace {
// a copy of the vector, rotated by the angle
RS_Vector rotatedCopy(const RS_Vector &v, double angle) {
  RS_Vector r = v;
  r.rotate(angle);
  return r;
}
} // namespace

//=====================================================================
// Construction
//=====================================================================

LC_HyperbolaData::LC_HyperbolaData(const RS_Vector &c, const RS_Vector &m,
                                   double r, double a1, double a2, bool rev)
    : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev) {}

// the branch through p, with the given foci
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
  // For a real hyperbola we need c > a (focal distance > semi-major axis).
  // Guard before sqrt so c*c - a*a < 0 doesn't propagate as NaN; NaN escapes
  // the `b < RS_TOLERANCE` check below because NaN comparisons are false.
  double inside = c * c - a * a;
  if (inside < RS_TOLERANCE2 || a < RS_TOLERANCE) {
    majorP = {};
    return;
  }
  double b = std::sqrt(inside);

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
    : RS_AtomicEntity(parent), m_data(d),
      m_valid(d.majorP.squared() >= RS_TOLERANCE2) {
  LC_Hyperbola::calculateBorders();
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
  RS_Vector secondaryVertex = m_data.reversed ? m_data.center + m_data.majorP
                                              : m_data.center - m_data.majorP;
  if (secondaryVertex.valid &&
      secondaryVertex.distanceTo(primaryVertex) > RS_TOLERANCE) {
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
  return pointAtArcLength(totalLength * 0.5, totalLength);
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

  // Pick the larger-magnitude coefficient as the denominator to avoid
  // catastrophic cancellation / division-by-zero when one of polarA, polarB
  // is near zero but the other is not.
  RS_Vector p1, p2;
  if (std::abs(polarB) >= std::abs(polarA)) {
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

    const RS_Vector chord = tp - point;
    const RS_Vector tangentDir = getTangentDirection(tp);
    if (!tangentDir.valid)
      continue;
    const double lengths = chord.magnitude() * tangentDir.magnitude();
    if (lengths < RS_TOLERANCE)
      continue;
    // The line from the given point to a point of tangency runs *along* the
    // curve's tangent there, so the two are parallel and the cross product
    // vanishes. Testing the dot product asks for perpendicular instead, which
    // no genuine tangency satisfies - it discarded every solution. Normalising
    // keeps the comparison scale-free, and it also drops the mirrored point
    // the conic solver returns from the other branch.
    const double sine = std::abs(chord.x * tangentDir.y - chord.y * tangentDir.x) / lengths;
    if (sine < RS_TOLERANCE)
      tangents.push_back(tp);
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
  RS_Vector local = rotatedCopy(world - getCenter(), -getAngle());
  return local;
}

//=====================================================================
RS_Vector LC_Hyperbola::localToWorld(const RS_Vector& local) const
{
  return rotatedCopy(local, getAngle()) + getCenter();
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
 * This approach avoids quartics, tanh substitution, and logarithmic forms that can
 * suffer from cancellation or overflow. It is numerically robust for all eccentricities,
 * including rectangular (b/a ≈ 1) and highly eccentric cases.
 *
 * @param p               Point on the hyperbola
 * @param branchReversed  Ignored — branch is automatically detected from geometry
 * @return                Hyperbolic parameter φ, or NaN for an invalid hyperbola or point
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
  local.rotate(-m_data.majorP.angle()); // inverse rotation

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

  return phi;
}

//=====================================================================

//=====================================================================
// Rendering
//=====================================================================

void LC_Hyperbola::draw(RS_Painter *painter, RS_GraphicView *view,
                        double & /*patternOffset*/) {
  // The painter works in GUI coordinates through RS_GraphicView, so the arc is
  // sampled in world space and handed over as a path, as the other curve
  // entities render. An unbounded hyperbola has no parameter range to sample
  // and is not drawn.
  if (painter == nullptr || view == nullptr || !m_valid || isInfinite())
    return;

  // Sample by how large the arc actually is on screen, not by a fixed count:
  // a chord error that is invisible when the whole arc fits the window grows
  // in proportion to the zoom, and a CAD user zooms. Aiming at a few pixels
  // per segment keeps the polyline under half a pixel from the curve at any
  // magnification, and the clamp bounds the cost either way.
  const RS_Vector guiMin = view->toGui(getMin());
  const RS_Vector guiMax = view->toGui(getMax());
  const double screenSpan = std::max(std::abs(guiMax.x - guiMin.x),
                                     std::abs(guiMax.y - guiMin.y));
  const int segments = std::clamp(static_cast<int>(std::min(screenSpan / 3.0, 4096.0)), 32, 4096);

  const double phi1 = m_data.angle1;
  const double phi2 = m_data.angle2;
  const double step = (phi2 - phi1) / segments;

  const RS_Vector start = view->toGui(getPoint(phi1, m_data.reversed));
  if (!start.valid)
    return;
  QPainterPath path{QPointF{start.x, start.y}};
  for (int i = 1; i <= segments; ++i) {
    const RS_Vector p = view->toGui(getPoint(phi1 + step * i, m_data.reversed));
    if (!p.valid)
      return;
    path.lineTo(QPointF{p.x, p.y});
  }
  painter->drawPath(path);
}

//=====================================================================

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

         // The point at that arc length from the start, without measuring the arc again
  RS_Vector middlePoint = pointAtArcLength(targetArcFromStart, totalLength);

  if (dist) {
    *dist = coord.distanceTo(middlePoint);
  }

  return middlePoint;
}

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
  return RS_Math::equal(m_data.angle1, 0.) && RS_Math::equal(m_data.angle2, 0.);
}

bool LC_Hyperbola::hasLength(const double phi1, const double phi2) const
{
  return std::isfinite(phi1) && std::isfinite(phi2) &&
         getPoint(phi1, m_data.reversed).distanceTo(getPoint(phi2, m_data.reversed)) >= RS_TOLERANCE;
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
  if (!m_valid || isInfinite())
    return RS_Vector(false);

  const double totalLength = getLength();
  if (totalLength <= std::abs(distance))
    return RS_Vector(false);

  double phi0 = getParamFromPoint(coord, m_data.reversed);
  const bool fromStart =
      std::abs(phi0 - m_data.angle1) <= std::abs(phi0 - m_data.angle2);

  const double targetArcFromStart = fromStart ? distance : totalLength - distance;

  if (distance < 0.0 || targetArcFromStart < 0.0 || targetArcFromStart > totalLength + RS_TOLERANCE)
    return RS_Vector(false);

  if (dist)
    *dist = targetArcFromStart;

  return pointAtArcLength(targetArcFromStart, totalLength);
}

RS_Vector LC_Hyperbola::pointAtArcLength(const double fromStart,
                                         const double totalLength) const
{
  const double phi1 = m_data.angle1;
  const double phi2 = m_data.angle2;
  if (!(totalLength > 0.0))
    return getPoint(phi1, m_data.reversed);

  // The arc runs towards the smaller parameter after revertDirection() or a
  // start grip dragged past the end, so measure along it in either direction.
  const double direction = phi2 >= phi1 ? 1.0 : -1.0;
  const auto alongArc = [this, phi1, direction](const double phi) {
    return direction * getArcLength(phi1, phi);
  };
  const double a = getMajorRadius();
  const double ecc2 = getEccentricity() * getEccentricity();

  using std::asinh, std::cosh, std::sinh;
  double phi = asinh(sinh(phi1) + fromStart / totalLength * (sinh(phi2) - sinh(phi1)));
  if (std::isnan(phi))
    phi = phi1;

  constexpr int maxIter = 30;
  constexpr double tol = 1e-12;

  bool converged = false;
  for (int i = 0; i < maxIter; ++i) {
    const double speed = a * std::sqrt(std::max(0.0, ecc2 * cosh(phi) * cosh(phi) - 1.0));
    if (speed < RS_TOLERANCE)
      break;
    const double delta = (fromStart - alongArc(phi)) / speed;
    phi += direction * delta;
    if (std::abs(delta) < tol) {
      converged = true;
      break;
    }
  }

  // Bisection between the ends, where the arc length runs from 0 to totalLength
  if (!converged) {
    double low = phi1;
    double high = phi2;
    for (int i = 0; i < 80; ++i) {
      phi = 0.5 * (low + high);
      const double s = alongArc(phi);
      if (std::abs(s - fromStart) < 1e-9)
        break;
      if (s < fromStart)
        low = phi;
      else
        high = phi;
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

void LC_Hyperbola::rotate(const RS_Vector &center, const double &angle) {
  rotate(center, RS_Vector{angle});
}

void LC_Hyperbola::rotate(const RS_Vector &center,
                          const RS_Vector &angleVector) {
  m_data.center.rotate(center, angleVector);
  m_data.majorP.rotate(angleVector);
  calculateBorders();
}

// An affine map takes a hyperbola to a hyperbola, and conjugate semi-diameters
// to conjugate semi-diameters: with u and v the images of the semi-axes, the
// image is C' + u cosh(phi) + v sinh(phi). As in the DXF spline import,
// u + v = e^d (a' û + b' v̂) and u - v = e^-d (a' û - b' v̂) give the axes of the
// image; the parameter shifts by d, and changes sign where the map reverses
// orientation.
void LC_Hyperbola::transformLinear(const RS_Vector &newCenter,
                                   const RS_Vector &imageOfX,
                                   const RS_Vector &imageOfY) {
  if (!m_valid)
    return;
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  const RS_Vector unitMajor = m_data.majorP / a;
  const RS_Vector unitMinor{-unitMajor.y, unitMajor.x};
  const auto linear = [&imageOfX, &imageOfY](const RS_Vector &w) {
    return imageOfX * w.x + imageOfY * w.y;
  };
  const RS_Vector u = linear(unitMajor * (m_data.reversed ? -a : a));
  const RS_Vector v = linear(unitMinor * b);
  const RS_Vector sum = u + v;
  const RS_Vector difference = u - v;
  const double g = std::sqrt(sum.magnitude() / difference.magnitude());
  const RS_Vector major = (sum / g + difference * g) * 0.5;
  const RS_Vector minor = (sum / g - difference * g) * 0.5;
  const double newA = major.magnitude();
  const double newB = minor.magnitude();
  // a singular map collapses the curve: leave it as it is
  if (!std::isfinite(newA) || !std::isfinite(newB) || newA < RS_TOLERANCE ||
      newB < RS_TOLERANCE || !std::isfinite(newCenter.x) ||
      !std::isfinite(newCenter.y))
    return;
  const double orientation = u.x * v.y - u.y * v.x >= 0.0 ? 1.0 : -1.0;
  const double shift = std::log(g);
  const bool unbounded = isInfinite();
  m_data.center = newCenter;
  m_data.majorP = major;
  m_data.ratio = newB / newA;
  m_data.reversed = false;
  if (!unbounded) {
    m_data.angle1 = orientation * (m_data.angle1 + shift);
    m_data.angle2 = orientation * (m_data.angle2 + shift);
  }
  calculateBorders();
}

void LC_Hyperbola::scale(const RS_Vector &center, const RS_Vector &factor) {
  RS_Vector newCenter = m_data.center;
  newCenter.scale(center, factor);
  transformLinear(newCenter, RS_Vector(factor.x, 0.0), RS_Vector(0.0, factor.y));
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
}

RS_Entity &LC_Hyperbola::shear(double k) {
  RS_Vector newCenter = m_data.center;
  newCenter.shear(k);
  // RS_Vector::shear() maps (x, y) to (x + k y, y)
  transformLinear(newCenter, RS_Vector(1.0, 0.0), RS_Vector(k, 1.0));
  return *this;
}

void LC_Hyperbola::revertDirection() {
  // Swap parameter bounds so traversal goes the other way along the branch.
  std::swap(m_data.angle1, m_data.angle2);
}

//=====================================================================
// Minimal overrides
//=====================================================================
RS_Vector LC_Hyperbola::getNearestCenter(const RS_Vector& coord, double* dist) const {
  if (!m_valid || !coord.valid) {
    if (dist)
      *dist = RS_MAXDOUBLE;
    return RS_Vector(false);
  }
  if (dist)
    *dist = coord.distanceTo(m_data.center);
  return m_data.center;
}

RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector &coord, double *dist) const {
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

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  // Work on the branch as if it were the right one: x = a cosh(phi), y = b sinh(phi).
  const RS_Vector local = worldToLocal(coord);
  const double x = m_data.reversed ? -local.x : local.x;
  const double y = local.y;

  // A bounded arc restricts phi to its range. Otherwise the whole branch
  // counts; cosh(30) already exceeds any drawing.
  const bool bounded = onEntity && !isInfinite();
  const double phiMin = bounded ? std::min(m_data.angle1, m_data.angle2) : -30.0;
  const double phiMax = bounded ? std::max(m_data.angle1, m_data.angle2) : 30.0;

  const auto squaredDistance = [a, b, x, y](const double phi) {
    const double dx = a * std::cosh(phi) - x;
    const double dy = b * std::sinh(phi) - y;
    return dx * dx + dy * dy;
  };

  // Every candidate is a point of the arc, so none can undercut the true
  // minimum.
  double bestPhi = std::clamp(0.0, phiMin, phiMax);
  double best = squaredDistance(bestPhi);
  const auto consider = [&](const double phi) {
    if (!std::isfinite(phi) || phi < phiMin || phi > phiMax)
      return;
    const double d = squaredDistance(phi);
    if (d < best) {
      best = d;
      bestPhi = phi;
    }
  };
  if (bounded) {
    consider(phiMin);
    consider(phiMax);
  }

  // Half the derivative of the squared distance is
  // (a^2+b^2) sinh cosh - a x sinh - b y cosh. With u = e^phi, times 4u^2, it
  // is the quartic (a^2+b^2) u^4 - 2(ax+by) u^3 + 2(ax-by) u - (a^2+b^2) = 0,
  // whose positive roots are every stationary point on the branch.
  const double k = a * a + b * b;
  const std::vector<double> ce{-1.0, 2.0 * (a * x - b * y) / k, 0.0,
                               -2.0 * (a * x + b * y) / k, 1.0};
  // A coordinate far enough out overflows the coefficients, and a non-finite
  // coefficient reaches std::polar() inside the solver, which aborts when the
  // standard library is built with _GLIBCXX_ASSERTIONS.
  if (std::isfinite(ce[1]) && std::isfinite(ce[3])) {
    for (const double u : RS_Math::quarticSolverFull(ce)) {
      if (u > 0.0)
        consider(std::log(u));
    }
  }
  // On the major axis two of those roots coincide and the solver can miss
  // them; there the stationary points are the vertex and cosh(phi) = ax/k.
  const double coshOnAxis = a * x / k;
  if (coshOnAxis >= 1.0) {
    consider(std::acosh(coshOnAxis));
    consider(-std::acosh(coshOnAxis));
  }

  // Newton steps on phi, taken while they stay in range and do not move away
  for (int i = 0; i < 8; ++i) {
    const double ch = std::cosh(bestPhi);
    const double sh = std::sinh(bestPhi);
    const double d1 = k * sh * ch - a * x * sh - b * y * ch;
    const double d2 = k * (ch * ch + sh * sh) - a * x * ch - b * y * sh;
    if (!(d2 > 0.0))
      break;
    const double next = std::clamp(bestPhi - d1 / d2, phiMin, phiMax);
    const double d = squaredDistance(next);
    if (!(d <= best))
      break;
    best = d;
    bestPhi = next;
  }

  if (dist)
    *dist = std::sqrt(best);
  return getPoint(bestPhi, m_data.reversed);
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

  // Extrema along the world X and Y axes. A world axis u has the components
  // (ux, uy) in the hyperbola's own frame, where the local point
  // (±a cosh φ, b sinh φ) projects onto it as ±a·ux·cosh φ + b·uy·sinh φ.
  // That is stationary where tanh φ = ∓(b·uy)/(a·ux); trying both signs covers
  // either branch, and a parameter inside the arc always gives a point on it.
  const double rot = getAngle();
  const double a = getMajorRadius();
  const double b = getMinorRadius();

  auto addExtrema = [&](const double ux, const double uy) {
    if (std::abs(b * uy) >= std::abs(a * ux))
      return; // monotonic along this axis, so the endpoints bound it
    const double phi = std::atanh((b * uy) / (a * ux));
    for (const double candidate : {phi, -phi}) {
      if (candidate >= phiStart - RS_TOLERANCE &&
          candidate <= phiEnd + RS_TOLERANCE) {
        const RS_Vector p = getPoint(candidate, m_data.reversed);
        if (p.valid) {
          minV = RS_Vector::minimum(minV, p);
          maxV = RS_Vector::maximum(maxV, p);
        }
      }
    }
  };

  // the world axes seen in the hyperbola's own frame
  addExtrema(std::cos(rot), -std::sin(rot)); // world X
  addExtrema(std::sin(rot), std::cos(rot));  // world Y

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

  // getArcLength() is signed, and the arc runs towards the smaller parameter
  // after revertDirection() or a start grip dragged past the end
  return std::abs(getArcLength(m_data.angle1, m_data.angle2));
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
  const LC_HyperbolaData previous = m_data;

  RS_Vector newRef = ref + offset;

  if (ref.distanceTo(m_data.center) < RS_TOLERANCE) {
    m_data.center = newRef;
  } else if (ref.distanceTo(getPrimaryVertex()) < RS_TOLERANCE) {
    RS_Vector dir = newRef - m_data.center;
    if (dir.magnitude() > RS_TOLERANCE) {
      m_data.majorP = dir.normalized() * getMajorRadius();
    }
  } else if (!isInfinite()) {
    // Start or end point movement
    if (ref.distanceTo(originalStart) < RS_TOLERANCE) {
      double phi = getParamFromPoint(newRef, m_data.reversed);
      // a start dragged onto the end would leave no arc
      if (hasLength(phi, m_data.angle2)) {
        m_data.angle1 = phi;
      }
    }
    else if (ref.distanceTo(originalEnd) < RS_TOLERANCE) {
      double phi = getParamFromPoint(newRef, m_data.reversed);
      if (hasLength(m_data.angle1, phi)) {
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

      // Re-project the original end points where the new branch passes
      // nearest to them, and undo a change that would leave no arc
      if (hadBounds) {
        const double phiStart = getParamFromPoint(getNearestPointOnEntity(originalStart, false), m_data.reversed);
        const double phiEnd = getParamFromPoint(getNearestPointOnEntity(originalEnd, false), m_data.reversed);
        if (!hasLength(phiStart, phiEnd)) {
          m_data = previous;
          return;
        }
        m_data.angle1 = phiStart;
        m_data.angle2 = phiEnd;

        if (m_data.angle1 > m_data.angle2)
          std::swap(m_data.angle1, m_data.angle2);
      }
    }
  }

  calculateBorders();
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
  // a start moved onto the end would leave no arc
  if (!hasLength(newPhi1, m_data.angle2))
    return;
  m_data.angle1 = newPhi1;

  calculateBorders();
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
  if (!hasLength(m_data.angle1, newPhi2))
    return;
  m_data.angle2 = newPhi2;

  calculateBorders();
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
  if (!m_valid || isInfinite())
    return 0.0;

  double phi1 = m_data.angle1;
  double phi2 = m_data.angle2;

  double a = getMajorRadius();
  double b = getMinorRadius();
  double a2 = a*a;
  double b2 = b*b;
  double cx = m_data.center.x;
  // double cy = m_data.center.y;
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
  //  \(\mathbf{F(t)=}\frac{\mathbf{a}^{\mathbf{2}}\mathbf{-b}^{\mathbf{2}}}{\mathbf{8}}\sin
  //  \mathbf{(2\alpha )}\cosh \mathbf{(2t)+
  // }\frac{\mathbf{ab}}{\mathbf{4}}\cos \mathbf{(2\alpha )}\sinh \mathbf{(2t)+
  // }\frac{\mathbf{ab}}{\mathbf{2}}\mathbf{t+
  // c}_{\mathbf{x}}\mathbf{(a}\sin \mathbf{\alpha }\cosh \mathbf{t+b}\cos
  // \mathbf{\alpha }\sinh \mathbf{t)+C}\)
  // Reversed branch: x_local = -a·cosh φ flips the terms with a single power
  // of the local x, a·b (sinh 2φ and φ) and cx·a·cosh φ; the (a²-b²) term and
  // cx·b·sinh φ keep their sign.
  const double sx = m_data.reversed ? -1.0 : 1.0;
  auto primitive = [&](double phi) -> double {
    double c1Term = c1 * sin2_th * std::cosh(2. * phi);
    double c2Term = c2 * cos2_th * std::sinh(2. * phi);
    double cxTerm = cx * (sx * R * std::cosh(phi) + S * std::sinh(phi));
    return c1Term + sx * (c2Term + c3 * phi) + cxTerm;
  };

  return primitive(phi2) - primitive(phi1);
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

  // The slope fixes the tangent point up to its reflection through the centre,
  // which lies on the other branch. As in RS_Ellipse, the line equation
  // u x + v y + 1 = 0 picks the point the line actually touches.
  const double phi = std::atanh(r);
  const RS_Vector onRight = getPoint(phi, false);
  const RS_Vector onLeft = getPoint(-phi, true);
  const auto lineEqu = [&line](const RS_Vector& vp) {
    return std::abs(line.dotP(vp) + 1.);
  };
  const bool touchesLeft = lineEqu(onLeft) < lineEqu(onRight);
  // a line tangent to the other branch does not touch this entity
  if (touchesLeft != m_data.reversed)
    return RS_Vector{false};
  return touchesLeft ? onLeft : onRight;
}

//=====================================================================
// Trim support – updated to match LC_Parabola behavior
//=====================================================================
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

    double phi = getParamFromPoint(intersect, m_data.reversed);
    if (std::isnan(phi))
      continue;

    // LC_Quadratic describes the whole conic, so the solver hands back the
    // mirrored intersection on the other branch too, and getParamFromPoint()
    // recovers phi from y alone - both points yield the *same* phi. |dphi|
    // therefore cannot separate them and whichever came first would win. Round
    // the parameter back into a point: only the one on this branch returns.
    if (getPoint(phi, m_data.reversed).distanceTo(intersect) > RS_TOLERANCE)
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

  if (side == RS2::EndingStart && hasLength(newPhi, m_data.angle2)) {
    m_data.angle1 = newPhi;
  } else if (side == RS2::EndingEnd && hasLength(m_data.angle1, newPhi)) {
    m_data.angle2 = newPhi;
  } else {
    // no side to trim, or a trim that would leave no arc
    return RS_Vector(false);
  }

  calculateBorders();

  return bestSol;
}

/**
 * @brief getTrimPoint
 * Determines which endpoint to move for trimming, based on the click position
 * relative to the chosen intersection point.
 *
 * The click point (trimCoord) and the chosen intersection (from prepareTrim())
 * decide whether the start or the end is trimmed or extended; the portion
 * containing the click point is kept.
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
  // The start is at the smaller parameter unless the arc runs backwards, after
  // revertDirection() or a start grip dragged past the end
  const bool forward = m_data.angle2 >= m_data.angle1;
  return ((phi_inter < phi_click) == forward) ? RS2::EndingStart : RS2::EndingEnd;
}
