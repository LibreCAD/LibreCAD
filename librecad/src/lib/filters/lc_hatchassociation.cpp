/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_hatchassociation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include "lc_containertraverser.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
using Point = std::pair<double, double>;
using Edge = LC_HatchAssociation::Edge;

bool straightCycle(const RS_EntityContainer& container, std::vector<Edge>& edges) {
    constexpr std::size_t maxEdges = 4096;
    std::set<Point> vertices;
    Point first, previous;
    lc::LC_ContainerTraverser traverser{container, RS2::ResolveNone};
    for (const RS_Entity* entity = traverser.first(); entity; entity = traverser.next()) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntityLine || edges.size() == maxEdges)
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
                                                         const RS_Polyline& boundary,
                                                         std::vector<Edge>* geometry) {
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
    if (sourceEdges != hatchEdges || hatchEdges != boundaryEdges) return false;
    if (geometry) *geometry = std::move(hatchEdges);
    return true;
}

// Exact integer predicates avoid rounding a near-intersection into a false
// simple-polygon witness. Other coordinates keep the deassociation path.
bool LC_HatchAssociation::isSimpleIntegerContour(const std::vector<Edge> &edges) {
  using Point = std::pair<std::int64_t, std::int64_t>;
  using Segment = std::pair<Point, Point>;
  if (edges.size() < 3u || edges.size() > 4096u) return false;
  std::vector<Segment> segments;
  for (const auto &edge : edges) {
    for (const double value : edge)
      if (!std::isfinite(value) || std::abs(value) > 1000000.0 || std::trunc(value) != value) return false;
    segments.push_back({{static_cast<std::int64_t>(edge[0]), static_cast<std::int64_t>(edge[1])},
                        {static_cast<std::int64_t>(edge[2]), static_cast<std::int64_t>(edge[3])}});
  }
  const auto cross = [](Point a, Point b, Point c) {
    return (b.first - a.first) * (c.second - a.second) - (b.second - a.second) * (c.first - a.first);
  };
  const auto on = [](Point a, Point b, Point p) {
    return p.first >= std::min(a.first, b.first) && p.first <= std::max(a.first, b.first) &&
           p.second >= std::min(a.second, b.second) && p.second <= std::max(a.second, b.second);
  };
  const auto opposite = [](std::int64_t a, std::int64_t b) { return (a < 0 && b > 0) || (a > 0 && b < 0); };
  for (std::size_t i = 0; i < segments.size(); ++i) {
    const auto [a, b] = segments[i];
    if (a == b) return false;
    for (std::size_t j = i + 1u; j < segments.size(); ++j) {
      const auto [c, d] = segments[j];
      if (std::max(a.first, b.first) < std::min(c.first, d.first) ||
          std::max(c.first, d.first) < std::min(a.first, b.first) ||
          std::max(a.second, b.second) < std::min(c.second, d.second) ||
          std::max(c.second, d.second) < std::min(a.second, b.second)) continue;
      if (a == c || a == d || b == c || b == d) {
        const Point shared = a == c || a == d ? a : b;
        const Point left = shared == a ? b : a, right = shared == c ? d : c;
        if (cross(shared, left, right) == 0 &&
            (left.first - shared.first) * (right.first - shared.first) +
            (left.second - shared.second) * (right.second - shared.second) > 0) return false;
        continue;
      }
      const auto ac = cross(a, b, c), ad = cross(a, b, d), ca = cross(c, d, a), cb = cross(c, d, b);
      if ((ac == 0 && on(a, b, c)) || (ad == 0 && on(a, b, d)) ||
          (ca == 0 && on(c, d, a)) || (cb == 0 && on(c, d, b)) ||
          (opposite(ac, ad) && opposite(ca, cb))) return false;
    }
  }
  return true;
}
