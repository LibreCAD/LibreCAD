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

#ifndef LC_HYPERBOLASPLINE_H
#define LC_HYPERBOLASPLINE_H

#include <vector>
#include "rs_vector.h"

// Forward declarations
class RS_EntityContainer;
class DRW_Spline;
class LC_Hyperbola;
struct LC_HyperbolaData;

/**
 * Utility class for exact conversion between LC_Hyperbola (single branch)
 * and rational quadratic SPLINE entities in DXF/DWG via libdxfrw.
 *
 * Hyperbolas are stored as a single rational quadratic NURBS SPLINE:
 *   - degree = 2
 *   - 3 control points
 *   - weights: 1.0, w > 1.0, 1.0
 *   - clamped uniform knots {0,0,0,1,1,1}
 *
 * This provides exact geometry preservation and full compatibility with standard DXF readers.
 */
class LC_HyperbolaSpline {
public:
  /**
   * Detect whether a SPLINE entity represents a rational quadratic hyperbola.
   */
  static bool isHyperbolaSpline(const DRW_Spline& s);

  /**
   * Convert a rational quadratic SPLINE to LC_Hyperbola entity.
   * @param s      Input SPLINE
   * @param parent Parent container for the new entity
   * @return Pointer to new LC_Hyperbola on success, nullptr on failure
   */
  static LC_Hyperbola* splineToHyperbola(const DRW_Spline& s, RS_EntityContainer* parent);

  /**
   * Convert a single hyperbola branch to exact rational quadratic control points/weights.
   */
  static bool hyperbolaToRationalQuadratic(const LC_HyperbolaData& hd,
                                           std::vector<RS_Vector>& ctrlPts,
                                           std::vector<double>& weights);

  /**
   * Create a DRW_Spline representing the hyperbola branch (exact rational quadratic).
   * @return true on success
   */
  static bool createSplineFromHyperbola(const LC_HyperbolaData& hd, DRW_Spline& spl);
};

#endif // LC_HYPERBOLASPLINE_H
