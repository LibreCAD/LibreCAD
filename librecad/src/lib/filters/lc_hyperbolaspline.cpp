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
#include "lc_linemath.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_vector.h"
#include "drw_entities.h"

namespace {
// Global tolerance for floating-point comparisons
constexpr double kTolerance = 1e-10;
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

         // Knot vector must be the standard open uniform [0,0,0,1,1,1]
  const auto& k = s.knotslist;
  if (std::abs(k[0]) > tol || std::abs(k[1]) > tol || std::abs(k[2]) > tol ||
      std::abs(k[3] - 1.0) > tol || std::abs(k[4] - 1.0) > tol || std::abs(k[5] - 1.0) > tol) {
    return false;
  }

         // Endpoint weights must be exactly 1.0
  if (!RS_Math::equal(s.weightlist[0], 1.0) || !RS_Math::equal(s.weightlist[2], 1.0)) {
    return false;
  }

 // Middle weight must be positive (and typically > 1 for hyperbolas)
  return s.weightlist[1] >= 1.0 + tol;
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

         // Extract control points and weights
  const RS_Vector p0(s.controllist[0]->x, s.controllist[0]->y); // start
  const RS_Vector p1(s.controllist[1]->x, s.controllist[1]->y); // shoulder (middle control point)
  const RS_Vector p2(s.controllist[2]->x, s.controllist[2]->y); // end

  //const double w0 = s.weightlist[0]; // always 1.0
  const double w1 = s.weightlist[1]; // middle weight (key parameter)
  //const double w2 = s.weightlist[2]; // always 1.0

         // Middle weight must be > 1 for a proper hyperbola segment
  if (w1 <= 1.0 + RS_TOLERANCE) {
    return nullptr;
  }

  const double w1_sq = w1 * w1;

         // Midpoint of chord (start to end)
  const RS_Vector chord_mid = (p0 + p2) * 0.5;

         // Center calculation derived from rational Bézier geometry
  const RS_Vector center = (p1 * w1_sq - chord_mid) / (w1_sq - 1.0);

         // Vectors relative to center
  const RS_Vector p1_rel = p1 - center;
  const RS_Vector chord_dir = (p2 - p0) * 0.5; // half chord vector

  const double l_sq = p1_rel.squared();          // ||p1 - center||²
  const double j_sq = chord_dir.squared();       // (half chord length)²
  const double dot   = p1_rel.dotP(chord_dir);   // alignment term
  const double s_sq  = w1_sq - 1.0;               // derived scale factor

  if (std::abs(s_sq) < kTolerance) {
    return nullptr;
  }
  const double inv_s_sq = 1.0 / s_sq;

         // Intermediate terms for radius calculations
  const double term1 = w1_sq * l_sq + j_sq * inv_s_sq;
  const double term2 = 4.0 * dot * dot * w1_sq * inv_s_sq;
  const double q = std::sqrt(term1 * term1 - term2);

         // Semi-transverse axis (a) and semi-conjugate axis (b)
  const double a = std::sqrt(0.5 * (w1_sq * l_sq - j_sq * inv_s_sq + q));
  const double b = std::sqrt(0.5 * (j_sq * inv_s_sq - w1_sq * l_sq + q));

  if (a < RS_TOLERANCE || b < RS_TOLERANCE) {
    return nullptr;
  }

         // Construct hyperbola data
  LC_HyperbolaData hd;
  hd.center = center;

         // Major axis vector: direction and magnitude a
         // Derived analytically to align with the control points
  const double proj_factor = (a * a + j_sq * inv_s_sq);
  const RS_Vector major_vec = p1_rel * proj_factor - chord_dir * (dot * inv_s_sq);
  hd.majorP = major_vec.normalized() * a;

  hd.ratio = b / a;

         // Create hyperbola entity
  auto hyperbola = std::make_unique<LC_Hyperbola>(parent, hd);

  if (!hyperbola->isValid()) {
    return nullptr;
  }

         // Set angular limits based on endpoints
  hyperbola->setAngle1(hyperbola->getParamFromPoint(p0));
  hyperbola->setAngle2(hyperbola->getParamFromPoint(p2));

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

         // For full (unbounded) hyperbola, use a large symmetric parameter range
         // to ensure stable middle weight > 1
  if (std::abs(phi1) < kTolerance && std::abs(phi2) < kTolerance) {
    phi1 = -2.0;
    phi2 =  2.0;
  }

         // Ensure phi1 ≤ phi2 for consistent calculations
  if (phi1 > phi2) {
    std::swap(phi1, phi2);
  }

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

         // Weight must be > 1 for a proper hyperbola segment
  if (weight_middle <= 1.0 + kTolerance) {
    return false;
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
  spl.flags = 8; // rational spline

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
