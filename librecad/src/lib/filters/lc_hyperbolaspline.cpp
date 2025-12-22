/*******************************************************************************
 *
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
 ******************************************************************************/

#include <algorithm>
#include <cmath>

#include "lc_hyperbolaspline.h"
#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "lc_linemath.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "drw_entities.h"

namespace {
constexpr double kTolerance = 1e-10;
constexpr double kDefaultPhiRange = 4.0;
}

bool LC_HyperbolaSpline::isHyperbolaSpline(const DRW_Spline& s)
{
  constexpr double tol = 1e-8;

         // Structural checks: single rational quadratic Bézier segment
  if (s.degree != 2 || s.controllist.size() != 3 || s.weightlist.size() != 3 ||
      s.knotslist.size() != 6)
    return false;

         // Standard open knot vector [0,0,0,1,1,1]
  const auto& k = s.knotslist;
  if (std::abs(k[0]) > tol || std::abs(k[1]) > tol || std::abs(k[2]) > tol ||
      std::abs(k[3] - 1.0) > tol || std::abs(k[4] - 1.0) > tol || std::abs(k[5] - 1.0) > tol)
    return false;

         // Endpoint weights must be 1.0
  if (std::abs(s.weightlist[0] - 1.0) > tol || std::abs(s.weightlist[2] - 1.0) > tol)
    return false;

  const double w = s.weightlist[1];
  if (w <= 0.0) return false;

         // Extract control points
  const auto* p0 = s.controllist[0].get();
  const auto* p1 = s.controllist[1].get();
  const auto* p2 = s.controllist[2].get();

  const double x0 = p0->x, y0 = p0->y;
  const double x1 = p1->x, y1 = p1->y;
  const double x2 = p2->x, y2 = p2->y;

         // Degenerate check: collinear control points → line (not a proper conic)
  const double area = (x1 - x0)*(y2 - y0) - (x2 - x0)*(y1 - y0);
  if (std::abs(area) < tol) return false;

         // Implicit quadratic coefficients A, B, C (only quadratic terms needed for discriminant)
  const double A = w * (y0 * y2 - y1 * y1);
  const double B = w * (2.0 * (x1 * (y0 + y2) - x0 * y1 - x2 * y1));
  const double C = w * (x0 * x2 - x1 * x1);

         // Conic discriminant B² - 4AC
  const double discriminant = B * B - 4.0 * A * C;

         // Hyperbola if discriminant > 0 (with tolerance for floating-point errors)
  return discriminant > tol;
}

LC_Hyperbola* LC_HyperbolaSpline::splineToHyperbola(const DRW_Spline& s, RS_EntityContainer* parent) {
  if (!isHyperbolaSpline(s)) {
    return nullptr;
  }

  const auto& c0 = s.controllist[0];
  const auto& c1 = s.controllist[1];
  const auto& c2 = s.controllist[2];

  RS_Vector pStart(c0->x, c0->y, c0->z);
  RS_Vector pShoulder(c1->x, c1->y, c1->z);
  RS_Vector pEnd(c2->x, c2->y, c2->z);

  double k = s.weightlist[1];

  double xStart = pStart.x, yStart = pStart.y;
  double xShoulder = pShoulder.x, yShoulder = pShoulder.y;
  double xEnd = pEnd.x, yEnd = pEnd.y;

  double coeffXX = (yStart - yShoulder)*(yStart - yShoulder) +
                   k*(yShoulder - yEnd)*(yShoulder - yEnd) -
                   (yStart - yEnd)*(yStart - yEnd);

  double coeffXY = -2.0 * ((xStart - xShoulder)*(yStart - yShoulder) +
                           k*(xShoulder - xEnd)*(yShoulder - yEnd) -
                           (xStart - xEnd)*(yStart - yEnd));

  double coeffYY = (xStart - xShoulder)*(xStart - xShoulder) +
                   k*(xShoulder - xEnd)*(xShoulder - xEnd) -
                   (xStart - xEnd)*(xStart - xEnd);

  double coeffX = 2.0 * (xShoulder*(yStart - yEnd) - xStart*(yShoulder - yEnd) +
                         k*(xShoulder*(yEnd - yStart) + xEnd*(yShoulder - yStart)));

  double coeffY = 2.0 * (yShoulder*(xStart - xEnd) - yStart*(xShoulder - xEnd) +
                         k*(yShoulder*(xEnd - xStart) + yEnd*(xShoulder - xStart)));

  double coeffConst = xStart*(xEnd - xShoulder) + xShoulder*(xStart - xEnd) +
                      k*xShoulder*(xShoulder - xStart - xEnd) + xEnd*(xShoulder - xStart);

  double maxCoeff = std::max({std::fabs(coeffXX), std::fabs(coeffXY), std::fabs(coeffYY),
                              std::fabs(coeffX), std::fabs(coeffY), std::fabs(coeffConst), kTolerance});

  if (maxCoeff < kTolerance) {
    return nullptr;
  }

  coeffXX /= maxCoeff; coeffXY /= maxCoeff; coeffYY /= maxCoeff;
  coeffX  /= maxCoeff; coeffY  /= maxCoeff; coeffConst /= maxCoeff;

  std::vector<double> coeffs = {coeffXX, coeffXY, coeffYY, coeffX, coeffY, coeffConst};

  LC_Quadratic quadratic(coeffs);
  LC_HyperbolaData hd;
  LC_Hyperbola temp(nullptr, hd);

  if (!temp.createFromQuadratic(quadratic)) {
    return nullptr;
  }

  hd = temp.getData();

  RS_Vector chordMid = (pStart + pEnd) * 0.5;
  RS_Vector vecShoulder = pShoulder - chordMid;
  RS_Vector vecCenter = hd.center - chordMid;

  if (vecShoulder.squared() > kTolerance * kTolerance) {
    double crossZ = vecCenter.crossP(vecShoulder).z;
    hd.reversed = (crossZ < 0.0);
  }

  return new LC_Hyperbola(parent, hd);
}

bool LC_HyperbolaSpline::hyperbolaToRationalQuadratic(const LC_HyperbolaData& hd,
                                                      std::vector<RS_Vector>& ctrlPts,
                                                      std::vector<double>& weights) {
  if (!hd.isValid() || hd.ratio <= 0.0) return false;

  double phiStart = hd.angle1;
  double phiEnd   = hd.angle2;
  if (std::abs(phiStart) < kTolerance && std::abs(phiEnd) < kTolerance) {
    phiStart = -kDefaultPhiRange;
    phiEnd   =  kDefaultPhiRange;
  }

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

         // Use LC_LineMath for line-line intersection (existing utility in LibreCAD)
  RS_Vector shoulder = LC_LineMath::getIntersectionLineLine(pStart, pStart + tStart, pEnd, pEnd + tEnd);

  if (!shoulder.valid) {
    return false;  // No intersection → degenerate case
  }

  RS_Vector vStart = pStart - shoulder;
  RS_Vector vEnd   = pEnd - shoulder;

  double lenStart = vStart.magnitude();
  double lenEnd   = vEnd.magnitude();
  if (lenStart < kTolerance || lenEnd < kTolerance) return false;

  double cosTheta = vStart.normalized().dotP(vEnd.normalized());
  double wMiddle = std::abs(cosTheta);

  if (wMiddle < kTolerance) return false;
  if (cosTheta < 0.0) wMiddle = 1.0 / wMiddle;

  ctrlPts = {pStart, shoulder, pEnd};
  if (wMiddle <= 1. + RS_TOLERANCE) {
    LC_ERR<<wMiddle;
  }
  weights = {1.0, wMiddle, 1.0};

  return true;
}

bool LC_HyperbolaSpline::createSplineFromHyperbola(const LC_HyperbolaData& hd, DRW_Spline& spl) {
  std::vector<RS_Vector> ctrlPts;
  std::vector<double> weights;

  if (!hyperbolaToRationalQuadratic(hd, ctrlPts, weights)) {
    return false;
  }

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
