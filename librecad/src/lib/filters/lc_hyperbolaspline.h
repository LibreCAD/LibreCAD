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

// Forward declarations to minimize dependencies
class RS_Vector;
struct DRW_Spline;
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
 * This provides exact geometry preservation and full compatibility with
 * standard DXF readers (AutoCAD, etc.).
 */

namespace LC_HyperbolaSpline {
    /**
     * Detect whether a SPLINE entity represents a rational quadratic hyperbola.
     *
     * The criteria are:
     *  - degree == 2
     *  - exactly 3 control points
     *  - exactly 3 weights
     *  - endpoint weights == 1.0
     *  - middle weight > 1.0 (characteristic of hyperbola branch)
     */
    bool isHyperbolaSpline(const DRW_Spline& s);

    /**
     * Convert a rational quadratic SPLINE (detected as hyperbola) back to LC_HyperbolaData.
     *
     * Uses exact implicitization into general conic form, then classifies via
     * LC_Hyperbola::createFromQuadratic().
     */
    LC_HyperbolaData splineToHyperbola(const DRW_Spline& s);

    /**
     * Convert a single hyperbola branch to exact rational quadratic control points/weights.
     *
     * Always exports the right branch geometry aligned with positive majorP direction.
     * Left branch (reversed=true) is flipped to match standard orientation.
     */
    bool hyperbolaToRationalQuadratic(const LC_HyperbolaData& hd,
                                             std::vector<RS_Vector>& ctrlPts,
                                             std::vector<double>& weights);

    /**
     * Create a DRW_Spline representing the hyperbola branch (exact rational quadratic).
     *
     * Returns true on success and fills the provided DRW_Spline.
     * The spline is ready for writing via dxfW->writeSpline().
     */
    bool createSplineFromHyperbola(const LC_HyperbolaData& hd, DRW_Spline& spl);

};

#endif // LC_HYPERBOLASPLINE_H
