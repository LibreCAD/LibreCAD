// File: lc_hyperbolaspline.h

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

#ifndef LC_HYPERBOLASPLINE_H
#define LC_HYPERBOLASPLINE_H

#include <memory>

#include "drw_entities.h"

// Forward declarations to avoid full includes
class LC_Hyperbola;
class RS_EntityContainer;
struct LC_HyperbolaData;   // Forward declaration

/**
 * Utility for hyperbola ↔ DXF SPLINE conversion.
 * Hyperbolas are stored as rational quadratic Bézier splines in DXF.
 */
namespace LC_HyperbolaSpline
{
  /** Detect if spline is a hyperbola segment */
  bool isHyperbolaSpline(const DRW_Spline& s);

  /** Convert spline to hyperbola (nullptr if invalid) */
  std::unique_ptr<LC_Hyperbola> splineToHyperbola(const DRW_Spline& s,
                                                         RS_EntityContainer* parent = nullptr);

  /** Convert hyperbola to DXF spline */
  bool hyperbolaToSpline(const LC_HyperbolaData& hd, DRW_Spline& spl);

};

#endif // LC_HYPERBOLASPLINE_H
