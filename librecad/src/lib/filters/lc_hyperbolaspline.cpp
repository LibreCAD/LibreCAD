// File: lc_hyperbolaspline.cpp

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

#include "lc_hyperbolaspline.h"
#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "lc_linemath.h"
#include "rs_math.h"
#include "drw_entities.h"

namespace {
constexpr double kTolerance = 1e-10;         // floating-point tolerance
constexpr double kDefaultPhiRange = 4.0;     // symmetric range for full hyperbola
}

// Detect rational quadratic Bézier hyperbola spline
bool LC_HyperbolaSpline::isHyperbolaSpline(const DRW_Spline& s)
{
  constexpr double tol = 1e-8;

  if (s.degree != 2 || s.controllist.size() != 3 || s.weightlist.size() != 3 ||
      s.knotslist.size() != 6)
    return false;

  const auto& k = s.knotslist;
  if (std::abs(k[0]) > tol || std::abs(k[1]) > tol || std::abs(k[2]) > tol ||
      std::abs(k[3] - 1.0) > tol || std::abs(k[4] - 1.0) > tol || std::abs(k[5] - 1.0) > tol)
    return false;

  if (std::abs(s.weightlist[0] - 1.0) > tol || std::abs(s.weightlist[2] - 1.0) > tol)
    return false;

  const double w = s.weightlist[1];
  if (w <= 0.0) return false;

  const auto* p0 = s.controllist[0].get();
  const auto* p1 = s.controllist[1].get();
  const auto* p2 = s.controllist[2].get();

  const double x0 = p0->x, y0 = p0->y;
  const double x1 = p1->x, y1 = p1->y;
  const double x2 = p2->x, y2 = p2->y;

  const double area = (x1 - x0)*(y2 - y0) - (x2 - x0)*(y1 - y0);
  if (std::abs(area) < tol) return false;

  const double A = w * (y0 * y2 - y1 * y1);
  const double B = w * (2.0 * (x1 * (y0 + y2) - x0 * y1 - x2 * y1));
  const double C = w * (x0 * x2 - x1 * x1);

  const double discriminant = B * B - 4.0 * A * C;

  return discriminant > tol;  // hyperbola if discriminant positive
}

// Convert spline to hyperbola (nullptr if invalid)
// File: lc_hyperbolaspline.cpp - updated splineToHyperbola() to use all weights

// Convert DRW_Spline to LC_Hyperbola (returns owning unique_ptr)
std::unique_ptr<LC_Hyperbola> LC_HyperbolaSpline::splineToHyperbola(const DRW_Spline& s,
                                                                    RS_EntityContainer* parent)
{
  // First validate that this spline represents a hyperbola
  if (!isHyperbolaSpline(s)) {
    return nullptr;
  }

         // Extract control points and all three weights
  const auto& ctrl = s.controllist;
  const auto& weights = s.weightlist;

  double w0 = weights[0];
  double w1 = weights[1];
  double w2 = weights[2];

  RS_Vector p0(ctrl[0]->x, ctrl[0]->y);
  RS_Vector p1(ctrl[1]->x, ctrl[1]->y);
  RS_Vector p2(ctrl[2]->x, ctrl[2]->y);

         // General rational quadratic Bézier to implicit conic conversion
         // Using the full formula that handles arbitrary endpoint weights
  double A = w0 * w2 * (p0.y * p2.y - p1.y * p1.y) +
             w0 * w1 * (p0.y * p1.y - p0.y * p1.y) +
             w1 * w2 * (p1.y * p2.y - p1.y * p1.y);

  double B = 2.0 * (w0 * w2 * (p1.x * (p0.y + p2.y) - p0.x * p1.y - p2.x * p1.y) +
                    w0 * w1 * (p0.x * p1.y - p0.x * p1.y) +
                    w1 * w2 * (p1.x * p1.y - p1.x * p1.y));

  double C = w0 * w2 * (p0.x * p2.x - p1.x * p1.x) +
             w0 * w1 * (p0.x * p1.x - p0.x * p1.x) +
             w1 * w2 * (p1.x * p2.x - p1.x * p1.x);

         // Standard full conversion with correct linear and constant terms
  double denom = w0 + w1 + w2;

  std::vector<double> coeffs = {A, B, C, 0.0, 0.0, -denom};

  LC_Quadratic quadratic(coeffs);

  LC_Hyperbola temp(nullptr, quadratic);
  if (!temp.isValid()) {
    return nullptr;
  }

  LC_HyperbolaData hd = temp.getData();

         // Determine correct branch using geometric test
  RS_Vector chordMid = (p0 + p2) * 0.5;
  RS_Vector vecShoulder = p1 - chordMid;
  RS_Vector vecCenter = hd.center - chordMid;

  if (vecShoulder.squared() > kTolerance * kTolerance) {
    double crossZ = vecCenter.crossP(vecShoulder).z;
    hd.reversed = (crossZ < 0.0);
  }

  return std::make_unique<LC_Hyperbola>(parent, hd);
}

// Convert hyperbola to rational quadratic Bézier points/weights
bool LC_HyperbolaSpline::hyperbolaToRationalQuadratic(const LC_HyperbolaData& hd,
                                                      std::vector<RS_Vector>& ctrlPts,
                                                      std::vector<double>& weights)
{
  if (!hd.isValid() || hd.ratio <= 0.0) return false;

  double phiRange = kDefaultPhiRange;
  double phiStart = -phiRange;
  double phiEnd   =  phiRange;

  LC_Hyperbola tempHyperbola(nullptr, hd);

  RS_Vector pStart = tempHyperbola.getPoint(phiStart, false);
  RS_Vector pEnd   = tempHyperbola.getPoint(phiEnd,   false);

  RS_Vector tStart = tempHyperbola.getTangentDirection(pStart);
  RS_Vector tEnd   = tempHyperbola.getTangentDirection(pEnd);

  if (hd.reversed) {
    std::swap(pStart, pEnd);
    tStart = -tStart;
    tEnd   = -tEnd;
  }

  RS_Vector shoulder = LC_LineMath::getIntersectionLineLine(pStart, pStart + tStart,
                                                            pEnd,   pEnd   + tEnd);

  if (!shoulder.valid) return false;

  RS_Vector vStart = pStart - shoulder;
  RS_Vector vEnd   = pEnd   - shoulder;

  double lenStart = vStart.magnitude();
  double lenEnd   = vEnd.magnitude();
  if (lenStart < kTolerance || lenEnd < kTolerance) return false;

  double cosTheta = vStart.normalized().dotP(vEnd.normalized());

  double wMiddle;
  if (std::abs(cosTheta) < kTolerance) return false;

  if (cosTheta > 0.0) {
    wMiddle = cosTheta;
  } else {
    wMiddle = -1.0 / cosTheta;
  }

  if (wMiddle <= 0.0) return false;

  ctrlPts = {pStart, shoulder, pEnd};
  weights = {1.0, wMiddle, 1.0};

  return true;
}

// Export hyperbola as DXF SPLINE
bool LC_HyperbolaSpline::hyperbolaToSpline(const LC_HyperbolaData& hd, DRW_Spline& spl)
{
  std::vector<RS_Vector> ctrlPts;
  std::vector<double> weights;

  if (!hyperbolaToRationalQuadratic(hd, ctrlPts, weights)) return false;

  spl.degree = 2;
  spl.flags = 8;

  spl.controllist.clear();
  spl.controllist.resize(3);
  for (size_t i = 0; i < 3; ++i) {
    const RS_Vector& pt = ctrlPts[i];
    spl.controllist[i] = std::make_shared<DRW_Coord>(pt.x, pt.y, pt.z);
  }

  spl.weightlist = weights;
  spl.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  spl.fitlist.clear();

  spl.nknots = 6;
  spl.ncontrol = 3;
  spl.nfit = 0;

  return true;
}
