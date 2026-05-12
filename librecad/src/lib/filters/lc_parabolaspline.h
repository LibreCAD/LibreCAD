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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_PARABOLASPLINE_H
#define LC_PARABOLASPLINE_H

#include <memory>

#include "drw_entities.h"

class LC_Parabola;
class RS_EntityContainer;
struct LC_ParabolaData;

/**
 * Utility for parabola ↔ DXF SPLINE conversion. Mirrors LC_HyperbolaSpline.
 *
 * A degree-2 NURBS with 3 control points {P0, P1, P2} is mathematically a
 * conic section: weights {1, w, 1} where w<1 → ellipse arc, w=1 → parabola,
 * w>1 → hyperbola. This module handles the parabola (w=1) case.
 *
 * Per project convention writes are emitted as non-rational quadratic Béziers
 * (weights omitted) — equivalent to weights {1,1,1} per NURBS definition.
 * The reader accepts both forms.
 */
namespace LC_ParabolaSpline
{
  /** True if the spline is a degree-2 / 3-control-point quadratic Bézier
   *  whose weights are absent or all 1 (within tolerance), with the standard
   *  open-uniform knot vector {0,0,0,1,1,1}. Returns false for ellipse arcs
   *  (w<1) or any conic that is not a parabola. */
  bool isParabolaSpline(const DRW_Spline& s);

  /** Decode the spline as a parabola. Returns nullptr if isParabolaSpline
   *  is false or if the 3 control points are collinear (degenerate). */
  std::unique_ptr<LC_Parabola> splineToParabola(const DRW_Spline& s,
                                                RS_EntityContainer* parent = nullptr);

  /** Encode a parabola as a non-rational degree-2 spline with 3 control
   *  points and the standard open-uniform knot vector. weightlist is left
   *  empty (the rational flag is cleared). */
  bool parabolaToSpline(const LC_ParabolaData& pd, DRW_Spline& spl);
}

#endif // LC_PARABOLASPLINE_H
