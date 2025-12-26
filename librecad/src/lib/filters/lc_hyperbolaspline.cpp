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
// Tolerance for floating-point comparisons
constexpr double kTolerance = 1e-10;
}

// Check if a DRW_Spline represents a rational quadratic Bézier hyperbola segment
bool LC_HyperbolaSpline::isHyperbolaSpline(const DRW_Spline& s)
{
  constexpr double tol = 1e-8;

         // Must be exactly a degree-2 spline with 3 control points and 3 weights
  if (s.degree != 2 || s.controllist.size() != 3 || s.weightlist.size() != 3 ||
      s.knotslist.size() != 6)
    return false;

         // Knot vector must be the standard open uniform [0,0,0,1,1,1]
  const auto& k = s.knotslist;
  if (std::abs(k[0]) > tol || std::abs(k[1]) > tol || std::abs(k[2]) > tol ||
      std::abs(k[3] - 1.0) > tol || std::abs(k[4] - 1.0) > tol || std::abs(k[5] - 1.0) > tol)
    return false;

         // Endpoint weights must be exactly 1.0
  if (std::abs(s.weightlist[0] - 1.0) > tol || std::abs(s.weightlist[2] - 1.0) > tol)
    return false;

         // Middle weight must be positive
  const double w = s.weightlist[1];
  if (w <= 0.0) return false;

         // Extract control points
  const auto* p0 = s.controllist[0].get();
  const auto* p1 = s.controllist[1].get();
  const auto* p2 = s.controllist[2].get();

  const double x0 = p0->x, y0 = p0->y;
  const double x1 = p1->x, y1 = p1->y;
  const double x2 = p2->x, y2 = p2->y;

         // Collinear control points produce a line, not a proper conic
  const double area = (x1 - x0)*(y2 - y0) - (x2 - x0)*(y1 - y0);
  if (std::abs(area) < tol) return false;

         // Compute quadratic terms of implicit conic equation from rational Bézier
  const double A = w * (y0 * y2 - y1 * y1);
  const double B = w * (2.0 * (x1 * (y0 + y2) - x0 * y1 - x2 * y1));
  const double C = w * (x0 * x2 - x1 * x1);

         // Conic type discriminant: B² - 4AC > 0 → hyperbola
  const double discriminant = B * B - 4.0 * A * C;

  return discriminant > tol;  // Positive discriminant indicates hyperbola
}

// Convert DRW_Spline to LC_Hyperbola (returns owning unique_ptr)
// File: lc_hyperbolaspline.cpp - updated splineToHyperbola() with full weight support

// Convert DRW_Spline to LC_Hyperbola (returns owning unique_ptr)
// File: lc_hyperbolaspline.cpp - updated splineToHyperbola() with full general weight support

// Convert DRW_Spline to LC_Hyperbola (returns owning unique_ptr)
// File: lc_hyperbolaspline.cpp - updated splineToHyperbola() with analytical reconstruction

// File: lc_hyperbolaspline.cpp - improved splineToHyperbola() with analytical reconstruction

// File: lc_hyperbolaspline.cpp - improved splineToHyperbola() with analytical reconstruction

std::unique_ptr<LC_Hyperbola> LC_HyperbolaSpline::splineToHyperbola(const DRW_Spline& s,
                                                                    RS_EntityContainer* parent)
{
  if (!isHyperbolaSpline(s)) {
    return nullptr;
  }

         // Extract control points and weights
  const auto& ctrl = s.controllist;
  const auto& weights = s.weightlist;

  RS_Vector p0(ctrl[0]->x, ctrl[0]->y);  // start point
  RS_Vector p1(ctrl[1]->x, ctrl[1]->y);  // shoulder point
  RS_Vector p2(ctrl[2]->x, ctrl[2]->y);  // end point

  double w0 = weights[0];
  double w1 = weights[1];  // middle weight
  if (w1 - RS_TOLERANCE <= 1.)
    return nullptr;
  double w2 = weights[2];

  // chord middle point
  RS_Vector M = (p0 + p2) * 0.5;

  // the bezier point for bezier parameter 0.5
  RS_Vector S = (p0 + p1 * (2. * w1) + p2) / (2 * ( 1. + w1));

  double w12 = w1 * w1;

  RS_Vector Vd = (p2 - p0) * 0.5;


  // center
  RS_Vector C = (p1 * (w1*w1) - M) / (w12 - 1.);
  RS_Vector p1c = p1 - C;

  double l2 = p1c.squared();
  double j2 = Vd.squared();
  double D = p1c.dotP(Vd);
  double s2 = w12  - 1.;

  double invs2 = 1./s2;
  // Q

  double w1l2 = w12 * l2;
  double j2s = j2 * invs2;
  double w1l2j2 = w12 * l2 + j2 * invs2;
  double q = std::sqrt( w1l2j2 * w1l2j2 - 4. * D * D * w12 * invs2);

  // major radius
  double a = std::sqrt(0.5 * (w1l2 - j2s + q));
  if (! (a >= RS_TOLERANCE))
    return nullptr;

  // minor radius
  double b = std::sqrt(0.5 * (j2s - w1l2 + q));
  if (! (b >= RS_TOLERANCE))
    return nullptr;

  LC_HyperbolaData hd;
  hd.center = C;

  // majorP
  double d1 = (p1 - C).dotP(Vd);
  RS_Vector Va = (p1 - C) * (a*a + j2s) - Vd * (d1*invs2);
  hd.majorP = Va.normalized() * a;
  hd.ratio = b / a;

  auto hyperbola = std::make_unique<LC_Hyperbola>(nullptr, hd);
  if (!hyperbola->isValid()) {
    return nullptr;
  }

  hyperbola->setAngle1(hyperbola->getParamFromPoint(p0));
  hyperbola->setAngle2(hyperbola->getParamFromPoint(p2));

  return hyperbola;
}

// Updated hyperbolaToRationalQuadratic() with analytical shoulder and weight
// Updated hyperbolaToRationalQuadratic() with full center support

// Convert a hyperbola to a DRW_Spline (rational quadratic Bézier form)

// This is the standard way to export a hyperbola to DXF
// File: lc_hyperbolaspline.cpp - improved hyperbolaToSpline() with analytical shoulder and weight

// Convert a hyperbola to a DRW_Spline (rational quadratic Bézier form)
// Uses analytical shoulder position and weight for perfect fidelity
bool LC_HyperbolaSpline::hyperbolaToSpline(const LC_HyperbolaData& hd, DRW_Spline& spl)
{
  if (!hd.isValid() || hd.ratio <= 0.0) return false;

  double a = hd.majorP.magnitude();  // semi-transverse axis
  double b = a * hd.ratio;           // semi-conjugate axis

  double t0 = hd.angle1;
  double t2 = hd.angle2;

         // For full branch, use symmetric range to ensure stable w > 1
  if (std::abs(t0) < kTolerance && std::abs(t2) < kTolerance) {
    t0 = -2.0;
    t2 =  2.0;
  }

         // Ensure phi1 <= phi2
  //if (t0 > t2) std::swap(t0, t2);

         // Analytical shoulder point
         // x = a
         // y = -b * tanh((phi1 + phi2)/2)
  double tm = (t2 + t0) * 0.5;
  double td = (t2 - t0) * 0.5;
  using std::cosh, std::sinh, std::tanh, std::hypot;
  auto getPoint = [a, b](double t) -> RS_Vector {
    return {a * cosh(t), b * sinh(t)};
  };
  RS_Vector shoulder = getPoint(tm) / cosh(td);

  // Weight from geometry
  RS_Vector pStart = getPoint(t0);
  RS_Vector pEnd = getPoint(t2);
  double w_middle = cosh(td); //( cosh(phi1) + cosh(phi2) - 2 * cosh(phiAvg)) /
                    // (2 * cosh(phiAvg) - cosh(phiDiff));
                    /*cosh(phiDiff)/cosh(phiAvg)
                    * hypot(a * sinh(phi1), b * cosh(phi1))
                    / hypot(a * sinh(phi2), b * cosh(phi2));*/

         // Handle left branch: mirror over y-axis
  if (hd.reversed) {
    pStart.x = -pStart.x;
    pEnd.x   = -pEnd.x;
    shoulder.x = -shoulder.x;
  }

         // Apply rotation by major axis angle
  RS_Vector angle {hd.majorP.angle()};

  if (w_middle <= 1.0 + kTolerance)
    return false;

         // Assemble output
  std::vector<RS_Vector> controlPoints = {pStart, shoulder, pEnd};
  std::vector<double> weights = {1.0, w_middle, 1.0};

         // Build DRW_Spline
  spl.degree = 2;
  spl.flags = 8;
  spl.controllist.clear();
  for(RS_Vector& vp: controlPoints) {
    vp.rotate(angle);
    vp.move(hd.center);
    spl.controllist.push_back(std::make_shared<DRW_Coord>(vp.x, vp.y, vp.z));
  }

  spl.weightlist = weights;
  spl.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  spl.fitlist.clear();

  spl.nknots = 6;
  spl.ncontrol = 3;
  spl.nfit = 0;

  return true;
}
