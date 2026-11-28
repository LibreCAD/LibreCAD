/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_HATCHASSOCIATION_H
#define LC_HATCHASSOCIATION_H

#include <array>
#include <vector>

class RS_Hatch;
class RS_Polyline;

namespace LC_HatchAssociation {
// Checks only live 2D geometry, not source identities, ownership, reactors,
// output handles, or polygon simplicity. Those require separate checks.
using Edge = std::array<double, 4>;
bool matchesClosedStraightBoundary(const RS_Hatch& hatch, const RS_Polyline& boundary,
                                   std::vector<Edge>* geometry = nullptr);
}

#endif
