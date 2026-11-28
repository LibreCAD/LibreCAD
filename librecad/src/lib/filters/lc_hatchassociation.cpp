/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_hatchassociation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <utility>
#include <vector>

#include "lc_containertraverser.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
using Point = std::pair<double, double>;
using Edge = std::array<double, 4>;

bool straightCycle(const RS_EntityContainer& container, std::vector<Edge>& edges) {
    constexpr std::size_t maxEdges = 4096;
    std::set<Point> vertices;
    Point first, previous;
    lc::LC_ContainerTraverser traverser{container, RS2::ResolveNone};
    for (const RS_Entity* entity = traverser.first(); entity; entity = traverser.next()) {
        if (entity->rtti() != RS2::EntityLine || edges.size() == maxEdges)
            return false;
        const auto* line = static_cast<const RS_Line*>(entity);
        const RS_Vector a = line->getStartpoint(), b = line->getEndpoint();
        if (!a.isValid() || !b.isValid() || !std::isfinite(a.x) || !std::isfinite(a.y)
            || !std::isfinite(b.x) || !std::isfinite(b.y) || a.z != 0.0 || b.z != 0.0)
            return false;
        Point start{a.x, a.y}, end{b.x, b.y};
        if (start == end || (!edges.empty() && start != previous) || !vertices.insert(start).second)
            return false;
        if (edges.empty()) first = start;
        previous = end;
        if (end < start) std::swap(start, end);
        edges.push_back({{start.first, start.second, end.first, end.second}});
    }
    return edges.size() >= 3u && previous == first;
}
}

bool LC_HatchAssociation::matchesClosedStraightBoundary(const RS_Hatch& hatch,
                                                         const RS_Polyline& boundary) {
    if (!hatch.isSolid() || !boundary.isClosed() || hatch.isDeleted() || boundary.isDeleted())
        return false;
    const RS_EntityContainer* source = nullptr;
    for (const RS_Entity* entity : hatch) {
        if (!entity || entity->getFlag(RS2::FlagTemp)) continue;
        if (!entity->isContainer() || source) return false;
        source = static_cast<const RS_EntityContainer*>(entity);
    }
    if (!source) return false;
    std::vector<Edge> sourceEdges, hatchEdges, boundaryEdges;
    if (!straightCycle(*source, sourceEdges) || !straightCycle(boundary, boundaryEdges))
        return false;
    const auto loops = hatch.exportLoops();
    if (loops.size() != 1u || loops.front().depth != 0 || !loops.front().edges)
        return false;
    if (!straightCycle(*loops.front().edges, hatchEdges)
        || hatchEdges.size() != boundaryEdges.size())
        return false;
    std::sort(sourceEdges.begin(), sourceEdges.end());
    std::sort(hatchEdges.begin(), hatchEdges.end());
    std::sort(boundaryEdges.begin(), boundaryEdges.end());
    return sourceEdges == hatchEdges && hatchEdges == boundaryEdges;
}
