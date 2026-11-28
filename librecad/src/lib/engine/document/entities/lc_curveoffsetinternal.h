/*******************************************************************************
 *
 * This file is part of the LibreCAD project, a 2D CAD program.
 *
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 ******************************************************************************/

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "lc_parametriccurveintersection.h"

namespace LC_CurveOffsetInternal {

using BezierBranches = std::vector<std::vector<std::array<RS_Vector, 4>>>;

/** Internal adapter for the topology check on fitted Bezier pieces. */
std::unique_ptr<LC_ParametricCurves> makePieceCurves(const BezierBranches& branches,
                                                     std::vector<std::ptrdiff_t> next);

} // namespace LC_CurveOffsetInternal
