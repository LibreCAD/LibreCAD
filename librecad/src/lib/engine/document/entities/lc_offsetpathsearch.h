/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#ifndef LC_OFFSETPATHSEARCH_H
#define LC_OFFSETPATHSEARCH_H

#include <cstdint>
#include <functional>
#include <vector>

#include "lc_ellipseoffsetgeometry.h"

namespace LC_EllipseOffsetGeometry {

/** Immutable CSR graph whose vertices are directed reference EDGES, not physical nodes.
 * offsets has edgeCount+1 entries and partitions transitions by incoming edge.
 * Open graphs require edgeCount boolean bytes in each eligibility array; closed
 * graphs require both arrays empty. Exact duplicate transition keys are invalid.
 * Occurrence/event identities are opaque: their geometric validity is the caller's job.
 * All input storage must remain alive and unchanged through the visitor's return.
 */
struct OffsetPathGraphView {
    const std::vector<std::size_t>& offsets;
    const std::vector<CachePathJoin>& transitions;
    const std::vector<std::uint8_t>& startEligible;
    const std::vector<std::uint8_t>& endEligible;
    std::size_t edgeCount = 0;
    bool closed = false;
};

/** Borrowed only for the callback. Open routes have edges.size()-1 joins; closed
 * routes include the closing join and start at their least edge index. No edge
 * repeats. Different transition identities/orders are distinct semantic routes.
 */
struct OffsetPathRouteView {
    const std::vector<std::size_t>& edges;
    const std::vector<CachePathJoin>& joins;
    bool closed = false;
};

enum class OffsetPathVisit { Continue, Stop };
using OffsetPathVisitor = std::function<OffsetPathVisit(const OffsetPathRouteView&)>;

enum class OffsetPathSearchStatus { Exhausted, Stopped, InvalidInput, LimitExceeded };
struct OffsetPathSearchResult {
    OffsetPathSearchStatus status = OffsetPathSearchStatus::Exhausted;
    /** Complete callback invocations, including a visitor-requested stop. Not accepted geometry paths. */
    std::size_t routesVisited = 0;
    CheckWork work{};
    /** Structural suffix counts, NOT accepted geometry. Global only on Exhausted. */
    std::uint8_t structuralRoutes = 0;
    std::size_t memoStates = 0, memoHits = 0;
};

/** Enumerate complete open simple routes or closed simple cycles modulo rotation.
 * Traversal is deterministic in seed/CSR order, bounded by the shared work caps,
 * with O(edgeCount) traversal scratch and a budget-bounded suffix memo. No budget,
 * graph or callback is retained. A memo key is (seed, current edge, used-edge set),
 * not merely current edge. It owns a packed copy of the set and a complete 0/1/2
 * saturated continuation count. Equal suffixes may share this structural proof;
 * different ordered prefixes still reach the geometry visitor independently.
 * Zero-count suffixes are not expanded a second time. Memo insertion and traversal
 * scratch are
 * prepared before atomic record admission; popped records are never refunded.
 * Row/key/seed/stack scans consume pair work; duplicate-key and traversal-order
 * predicates also consume ordering work. Callback admission consumes one pair.
 * Stack length is bounded by edgeCount, NOT geometric refinement depth. Cache
 * spans and geometric depth are unchanged except by work the visitor admits.
 * The visitor owns acceptance, saturation and its own work charges. It can stop
 * after two accepted routes; Stopped does not assert validity or nonuniqueness.
 * Exceptions propagate unchanged, with admitted work remaining in the caller's
 * budget. Invalid input/resource refusal is sticky. No geometry certificate is
 * issued. Memo storage is never evicted or silently disabled on exhaustion.
 * Distinct-route output may be exponential; duplicate transitions are
 * refused and rotated cycles are pruned, never stored as duplicate assignments.
 */
OffsetPathSearchResult searchOffsetPaths(const OffsetPathGraphView& graph, WorkBudget& budget,
                                        const OffsetPathVisitor& visitor);

} // namespace LC_EllipseOffsetGeometry

#endif
