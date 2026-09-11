/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "lc_hyperbolaspline.h"
#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_math.h"
#include "rs_vector.h"
#include "drw_entities.h"

namespace {
// Global tolerance for floating-point comparisons
constexpr double kTolerance = 1e-10;

// How far above 1 the middle weight must be for a spline to be read as a
// hyperbola. Closer to 1 the arc cannot be told from a parabola, and the
// reconstruction divides by w^2 - 1.
constexpr double kHyperbolaWeightTolerance = 1e-8;
}

/**
 * @brief Detect whether a DRW_Spline represents a hyperbola segment stored as a
 *        rational quadratic Bézier curve.
 *
 * DXF stores conic sections (including hyperbolas) as degree-2 rational splines
 * with exactly 3 control points and specific knot/weights structure.
 *
 * @param s The spline to test
 * @return true if the spline is a valid hyperbola segment
 */
bool LC_HyperbolaSpline::isHyperbolaSpline(const DRW_Spline& s)
{
  constexpr double tol = 1e-8;

         // Must be a quadratic rational spline with 3 control points and matching weights/knots
  if (s.degree != 2 || s.controllist.size() != 3 || s.weightlist.size() != 3 ||
      s.knotslist.size() != 6) {
    return false;
  }
  for (const auto& p : s.controllist) {
    if (!p || !std::isfinite(p->x) || !std::isfinite(p->y)) {
      return false;
    }
  }

  // Only the form hyperbolaToSpline() writes is claimed: the knot vector
  // [0,0,0,1,1,1], end weights of 1 and a middle weight above 1. Any other
  // spline goes to LC_Parabola, as before.
  const auto& k = s.knotslist;
  for (const double knot : k) {
    if (!std::isfinite(knot)) {
      return false;
    }
  }
  if (std::abs(k[0]) > tol || std::abs(k[1]) > tol || std::abs(k[2]) > tol ||
      std::abs(k[3] - 1.0) > tol || std::abs(k[4] - 1.0) > tol || std::abs(k[5] - 1.0) > tol) {
    return false;
  }
  if (!RS_Math::equal(s.weightlist[0], 1.0) || !RS_Math::equal(s.weightlist[2], 1.0)) {
    return false;
  }
  const double w = s.weightlist[1];
  return std::isfinite(w) && w >= 1.0 + kHyperbolaWeightTolerance;
}

/**
 * @brief Convert a rational quadratic Bézier spline (DRW_Spline) that represents
 *        a hyperbola segment into an LC_Hyperbola entity.
 *
 * This performs an **analytical reconstruction** of the hyperbola parameters
 * (center, major axis vector, ratio) from the three control points and middle weight.
 *
 * @param s      The input spline (must pass isHyperbolaSpline)
 * @param parent Parent container (may be nullptr)
 * @return Unique pointer to LC_Hyperbola, or nullptr on failure
 */
std::unique_ptr<LC_Hyperbola> LC_HyperbolaSpline::splineToHyperbola(const DRW_Spline& s,
                                                                    RS_EntityContainer* parent)
{
  if (!isHyperbolaSpline(s)) {
    return nullptr;
  }

         // Extract control points and the middle weight of the standard form
  const RS_Vector p0(s.controllist[0]->x, s.controllist[0]->y); // start
  const RS_Vector p1(s.controllist[1]->x, s.controllist[1]->y); // shoulder (middle control point)
  const RS_Vector p2(s.controllist[2]->x, s.controllist[2]->y); // end
  const double w = s.weightlist[1];                   // cosh of half the parameter span
  const double wSquaredMinus1 = (w - 1.0) * (w + 1.0); // w^2 - 1 without cancellation

  // The arc from phi_m - d to phi_m + d has w = cosh d. With the chord midpoint
  // M = (p0 + p2)/2, M - C = w^2 (p1 - C), so p1 - C = (M - p1) / (w^2 - 1).
  // Taking that from the control points, rather than subtracting a centre that
  // can lie far away, keeps arcs far from the vertex exact.
  const RS_Vector shoulder = ((p0 + p2) * 0.5 - p1) / wSquaredMinus1;
  const RS_Vector center = p1 - shoulder;

  // Conjugate semi-diameters at phi_m: u from the centre to P(phi_m), and the
  // tangent v = P'(phi_m). For a hyperbola |u|^2 - |v|^2 = a^2 - b^2 and
  // |u x v| = a b.
  const RS_Vector u = shoulder * w;
  const RS_Vector v = (p2 - p0) / (2.0 * std::sqrt(wSquaredMinus1));
  const RS_Vector sum = u + v;
  const RS_Vector difference = u - v;
  const double d = difference.dotP(sum);
  const double c = std::abs(u.x * v.y - u.y * v.x);
  const double h = std::hypot(d, 2.0 * c);
  // a^2 solves a^4 - d a^2 - c^2 = 0; the second form avoids cancellation for d < 0
  const double aSquared = d >= 0.0 ? 0.5 * (h + d) : 2.0 * c * c / (h - d);
  // u + v = e^phi_m (a û + b v̂) and u - v = e^-phi_m (a û - b v̂), so
  // e^phi_m = sqrt(|u + v| / |u - v|) gives the major axis a û. It points at the
  // arc's branch, as u does.
  const double g = std::sqrt(sum.magnitude() / difference.magnitude());
  const RS_Vector major = (sum / g + difference * g) * 0.5;

  const double a = std::sqrt(aSquared);
  const double b = c / a;
  if (!std::isfinite(center.x) || !std::isfinite(center.y) ||
      !std::isfinite(major.x) || !std::isfinite(major.y) || !std::isfinite(b) ||
      a < RS_TOLERANCE || b < RS_TOLERANCE || major.squared() < RS_TOLERANCE2) {
    return nullptr;
  }

         // Construct hyperbola data
  LC_HyperbolaData hd;
  hd.center = center;
  hd.majorP = major.normalized() * a;
  hd.ratio = b / a;

         // Create hyperbola entity
  auto hyperbola = std::make_unique<LC_Hyperbola>(parent, hd);

  if (!hyperbola->isValid()) {
    return nullptr;
  }

  // The start and end stay where the spline has them, so the arc keeps its
  // direction.
  const double phi1 = hyperbola->getParamFromPoint(p0);
  const double phi2 = hyperbola->getParamFromPoint(p2);
  if (!std::isfinite(phi1) || !std::isfinite(phi2)) {
    return nullptr;
  }
  hyperbola->setAngle1(phi1);
  hyperbola->setAngle2(phi2);

  return hyperbola;
}

/**
 * @brief Convert an LC_Hyperbola (or arc thereof) to a DRW_Spline in rational
 *        quadratic Bézier form.
 *
 * This uses the **exact analytical representation** of a hyperbola arc as a
 * rational quadratic Bézier curve:
 *   - Control points: start point, shoulder point, end point
 *   - Weights: 1, cosh(Δφ/2), 1
 *   - Shoulder point = P(φ_mid) / cosh(Δφ/2)
 *
 * This guarantees perfect round-trip fidelity with DXF.
 *
 * @param hd  Hyperbola data (must be valid and have ratio > 0)
 * @param spl Output spline object to fill
 * @return true on success
 */
bool LC_HyperbolaSpline::hyperbolaToSpline(const LC_HyperbolaData& hd, DRW_Spline& spl)
{
  if (!hd.isValid() || hd.ratio <= 0.0) {
    return false;
  }

  const double a = hd.majorP.magnitude();           // semi-transverse axis
  const double b = a * hd.ratio;                     // semi-conjugate axis

  double phi1 = hd.angle1;
  double phi2 = hd.angle2;

         // An unbounded hyperbola has no parameter range to encode. A finite
         // range in its place would write a clipped arc that reads back as
         // the whole branch, so refuse and let the caller report it: a file
         // that is missing an entity is recoverable, a file that quietly
         // contains the wrong geometry is not.
  if (std::abs(phi1) < kTolerance && std::abs(phi2) < kTolerance) {
    return false;
  }
  // an arc of no length has no curve to encode
  if (phi1 == phi2) {
    return false;
  }

  // phi1 stays the start even when it is the larger parameter, so the arc keeps
  // its direction; the formulas below hold for a negative half span as well
  const double phi_mid = (phi1 + phi2) * 0.5;
  const double phi_delta = (phi2 - phi1) * 0.5;

         // Parametric point on standard hyperbola (before rotation/translation)
  auto standardPoint = [a, b](double phi) -> RS_Vector {
    return RS_Vector(a * std::cosh(phi), b * std::sinh(phi));
  };

         // Analytical shoulder point and weight for exact rational quadratic representation
  RS_Vector shoulder_standard = standardPoint(phi_mid) / std::cosh(phi_delta);
  const double weight_middle = std::cosh(phi_delta);

         // Endpoints on standard hyperbola
  RS_Vector start_standard = standardPoint(phi1);
  RS_Vector end_standard   = standardPoint(phi2);

         // Handle left branch (reversed = true) by mirroring over y-axis
  if (hd.reversed) {
    start_standard.x = -start_standard.x;
    end_standard.x   = -end_standard.x;
    shoulder_standard.x = -shoulder_standard.x;
  }

         // Apply rotation by major axis angle and translation by center
  const double rotation_angle = hd.majorP.angle();

  auto transform = [rotation_angle, &hd](RS_Vector v) {
    v.rotate(rotation_angle);
    v.move(hd.center);
    return v;
  };

  const RS_Vector start = transform(start_standard);
  const RS_Vector shoulder = transform(shoulder_standard);
  const RS_Vector end = transform(end_standard);

  // Build DRW_Spline
  spl.degree = 2;
  spl.flags = 0x08 | 0x04; // planar rational spline

  spl.controllist.clear();
  spl.controllist.push_back(std::make_shared<DRW_Coord>(start.x, start.y, 0.0));
  spl.controllist.push_back(std::make_shared<DRW_Coord>(shoulder.x, shoulder.y, 0.0));
  spl.controllist.push_back(std::make_shared<DRW_Coord>(end.x, end.y, 0.0));

  spl.weightlist = {1.0, weight_middle, 1.0};
  spl.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

  spl.fitlist.clear();

  spl.nknots = 6;
  spl.ncontrol = 3;
  spl.nfit = 0;

  return true;
}
