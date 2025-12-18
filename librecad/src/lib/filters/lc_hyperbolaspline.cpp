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

#include "lc_hyperbolaspline.h"
#include "lc_hyperbola.h"
#include "lc_linemath.h"
#include "lc_quadratic.h"
#include "rs_math.h"
#include "drw_entities.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr double g_kTolerance = 1e-10;
constexpr double g_kDefaultPhiRange = 4.0;  // ±4 covers asymptotic behavior well
}

bool LC_HyperbolaSpline::isHyperbolaSpline(const DRW_Spline& s) {
  return (s.degree == 2 &&
          s.controllist.size() == 3 &&
          s.weightlist.size() == 3 &&
          std::fabs(s.weightlist.at(0) - 1.0) <= g_kTolerance &&
          std::fabs(s.weightlist.at(2) - 1.0) <= g_kTolerance &&
          s.weightlist.at(1) > 1.0 + g_kTolerance);
}

LC_HyperbolaData LC_HyperbolaSpline::splineToHyperbola(const DRW_Spline& s) {
  LC_HyperbolaData hyperbolaData;

  if (!isHyperbolaSpline(s)) {
    return hyperbolaData;
  }

  const auto& c0 = s.controllist[0];
  const auto& c1 = s.controllist[1];
  const auto& c2 = s.controllist[2];

  RS_Vector pStart(c0->x, c0->y, c0->z);
  RS_Vector pShoulder(c1->x, c1->y, c1->z);
  RS_Vector pEnd(c2->x, c2->y, c2->z);

  double k = s.weightlist[1];

  double dx0 = pStart.x - pShoulder.x;
  double dy0 = pStart.y - pShoulder.y;
  double dx2 = pShoulder.x - pEnd.x;
  double dy2 = pShoulder.y - pEnd.y;

  double coeffXX = dy0*dy0 + k*dy2*dy2 - (dy0 + dy2)*(dy0 + dy2);
  double coeffXY = -2.0 * (dx0*dy0 + k*dx2*dy2 - (dx0 + dx2)*(dy0 + dy2));
  double coeffYY = dx0*dx0 + k*dx2*dx2 - (dx0 + dx2)*(dx0 + dx2);
  double coeffX  = 2.0 * (pShoulder.x*(dy0 + dy2) - pStart.x*dy0 - pEnd.x*dy2);
  double coeffY  = -2.0 * (pShoulder.y*(dx0 + dx2) - pStart.y*dx0 - pEnd.y*dx2);
  double coeffConst = pStart.x*pEnd.x - pStart.x*pShoulder.x - pEnd.x*pShoulder.x +
                      k*pShoulder.x*pShoulder.x;

  double maxCoeff = std::max({std::fabs(coeffXX), std::fabs(coeffXY), std::fabs(coeffYY),
                              std::fabs(coeffX),  std::fabs(coeffY),  std::fabs(coeffConst), g_kTolerance});

  if (maxCoeff < g_kTolerance) {
    return hyperbolaData;  // Degenerate case
  }

  std::vector<double> coeffs = {
      coeffXX / maxCoeff,
      coeffXY / maxCoeff,
      coeffYY / maxCoeff,
      coeffX  / maxCoeff,
      coeffY  / maxCoeff,
      coeffConst / maxCoeff
  };

  LC_Quadratic quadratic(coeffs);
  LC_Hyperbola tempEntity(nullptr, hyperbolaData);

  if (tempEntity.createFromQuadratic(quadratic)) {
    hyperbolaData = tempEntity.getData();

    RS_Vector chordMid = (pStart + pEnd) * 0.5;
    RS_Vector vecShoulder = pShoulder - chordMid;
    RS_Vector vecCenter = hyperbolaData.center - chordMid;

    if (vecShoulder.squared() > g_kTolerance * g_kTolerance) {
      hyperbolaData.reversed = (vecCenter.dotP(vecShoulder) < 0.0);
    }
  }

  return hyperbolaData;
}

bool LC_HyperbolaSpline::hyperbolaToRationalQuadratic(const LC_HyperbolaData& hd,
                                                      std::vector<RS_Vector>& ctrlPts,
                                                      std::vector<double>& weights) {
  if (hd.ratio <= 0.0) return false;

  double phiStart = hd.angle1;
  double phiEnd   = hd.angle2;
  if (std::abs(phiStart) < g_kTolerance && std::abs(phiEnd) < g_kTolerance) {
    phiStart = -g_kDefaultPhiRange;
    phiEnd   =  g_kDefaultPhiRange;
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

  RS_Vector shoulder = LC_LineMath::getIntersectionLineLine(pStart, pEnd, tStart, tEnd);
  if (! shoulder.valid)
    return false;


  RS_Vector vStart = pStart - shoulder;
  RS_Vector vEnd   = pEnd   - shoulder;

  double lenStart = vStart.magnitude();
  double lenEnd   = vEnd.magnitude();
  if (lenStart < g_kTolerance || lenEnd < g_kTolerance) return false;

  double cosTheta = (vStart / lenStart).dotP(vEnd / lenEnd);
  double wMiddle = std::fabs(cosTheta);

  if (wMiddle < g_kTolerance) return false;
  if (cosTheta < 0.0) wMiddle = 1.0 / wMiddle;

  ctrlPts = {pStart, shoulder, pEnd};
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
