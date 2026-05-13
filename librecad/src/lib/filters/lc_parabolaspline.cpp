/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <array>
#include <cmath>
#include <memory>

#include "lc_parabola.h"
#include "lc_parabolaspline.h"
#include "rs_math.h"

namespace {
// Tighter than the entity-construction tolerance: the dispatcher only accepts
// a parabola if the input shape is unambiguous.
constexpr double kWeightTolerance = 1e-9;
constexpr double kKnotTolerance = 1e-8;
} // namespace

bool LC_ParabolaSpline::isParabolaSpline(const DRW_Spline &s) {
  if (s.degree != 2 || s.controllist.size() != 3) {
    return false;
  }

  // Open-uniform knot vector {0,0,0,1,1,1} is the standard form for a
  // single quadratic Bézier segment. Allow absent knots (some writers
  // omit them for the canonical case).
  if (!s.knotslist.empty()) {
    if (s.knotslist.size() != 6)
      return false;
    const auto &k = s.knotslist;
    if (std::abs(k[0]) > kKnotTolerance || std::abs(k[1]) > kKnotTolerance ||
        std::abs(k[2]) > kKnotTolerance ||
        std::abs(k[3] - 1.0) > kKnotTolerance ||
        std::abs(k[4] - 1.0) > kKnotTolerance ||
        std::abs(k[5] - 1.0) > kKnotTolerance) {
      return false;
    }
  }

  // Weights: absent (non-rational) → parabola; or all 1.0 (rational
  // weight=1) → still parabola. Reject ellipse-arc weights (w<1) and
  // hyperbola weights (w>1) so the dispatcher can route them elsewhere.
  if (!s.weightlist.empty()) {
    if (s.weightlist.size() != 3)
      return false;
    for (double w : s.weightlist) {
      if (std::abs(w - 1.0) > kWeightTolerance)
        return false;
    }
  }

  // Reject collinear / coincident control points — those describe a line
  // segment, not a parabola.
  const auto &c0 = *s.controllist[0];
  const auto &c1 = *s.controllist[1];
  const auto &c2 = *s.controllist[2];
  const double cross =
      (c1.x - c0.x) * (c2.y - c0.y) - (c1.y - c0.y) * (c2.x - c0.x);
  if (std::abs(cross) <= RS_TOLERANCE) {
    return false;
  }

  return true;
}

std::unique_ptr<LC_Parabola>
LC_ParabolaSpline::splineToParabola(const DRW_Spline &s,
                                    RS_EntityContainer *parent) {
  if (!isParabolaSpline(s)) {
    return nullptr;
  }

  LC_ParabolaData pd{{
      RS_Vector{s.controllist[0]->x, s.controllist[0]->y},
      RS_Vector{s.controllist[1]->x, s.controllist[1]->y},
      RS_Vector{s.controllist[2]->x, s.controllist[2]->y},
  }};

  // CalculatePrimitives sets m_valid=false if the 3 points don't admit
  // a non-degenerate parabola (axis ≈ 0). Belt-and-suspenders with the
  // collinearity check in isParabolaSpline.
  if (!pd.isValid()) {
    return nullptr;
  }

  return std::make_unique<LC_Parabola>(parent, pd);
}

bool LC_ParabolaSpline::parabolaToSpline(const LC_ParabolaData &pd,
                                         DRW_Spline &spl) {
  if (!pd.isValid()) {
    return false;
  }

  spl.degree = 2;
  // DRW_Spline::flags bit 0x08 = "planar"; bit 0x04 = "rational"; clear
  // rational since we emit non-rational quadratic Bézier (weights omitted,
  // mathematically equivalent to weights {1,1,1} for the parabola case).
  spl.flags = 8;

  spl.controllist.clear();
  for (const auto &p : pd.m_controlPoints) {
    spl.controllist.push_back(std::make_shared<DRW_Coord>(p.x, p.y, 0.0));
  }

  spl.weightlist.clear();
  spl.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  spl.fitlist.clear();

  spl.nknots = 6;
  spl.ncontrol = 3;
  spl.nfit = 0;

  return true;
}
