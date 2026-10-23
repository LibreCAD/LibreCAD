/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include "lc_offsetpathsearch.h"

#include <limits>

namespace LC_EllipseOffsetGeometry {
namespace {

bool sameTransition(const CachePathJoin& a, const CachePathJoin& b) {
    return a.kind == b.kind && a.incoming == b.incoming && a.outgoing == b.outgoing &&
        a.from.value == b.from.value && a.to.value == b.to.value && a.pairEvent == b.pairEvent;
}

class PathSearch {
public:
    PathSearch(const OffsetPathGraphView& graph, WorkBudget& budget, const OffsetPathVisitor& visitor)
        : m_graph{graph}, m_budget{budget}, m_visitor{visitor} {}

    OffsetPathSearchResult run() {
        if (m_budget.status() != BudgetStatus::Ok || !validate()) { return finish(); }
        if (m_graph.edgeCount == 0 || (!m_graph.closed && (!m_hasStart || !m_hasEnd))) { return finish(); }
        if (!prepare()) { return finish(); }
        for (std::size_t seed = 0; seed < m_graph.edgeCount; ++seed) {
            if (!pair()) { break; }
            if (!m_graph.closed && !m_graph.startEligible[seed]) { continue; }
            if (!walk(seed)) { break; }
        }
        return finish();
    }

private:
    struct Frame { std::size_t edge, successor; bool emitted = false; };

    OffsetPathSearchResult finish() {
        if (m_budget.status() == BudgetStatus::LimitExceeded) { m_result.status = OffsetPathSearchStatus::LimitExceeded; }
        else if (m_budget.status() == BudgetStatus::InvalidInput) { m_result.status = OffsetPathSearchStatus::InvalidInput; }
        m_result.work = m_budget.consumed();
        return m_result;
    }
    bool invalid() { m_budget.latchInvalidInput(); return false; }
    bool limit() { m_budget.latchLimitExceeded(); return false; }
    bool pair() { return m_budget.consumePairs(1); }
    bool orderedPair() { return m_budget.adoptWork({0, 0, 1, 0, 1}); }

    bool validate() {
        const auto n = m_graph.edgeCount;
        if (!m_visitor || n == std::numeric_limits<std::size_t>::max() || m_graph.offsets.size() != n + 1 ||
            (m_graph.closed && (!m_graph.startEligible.empty() || !m_graph.endEligible.empty())) ||
            (!m_graph.closed && (m_graph.startEligible.size() != n || m_graph.endEligible.size() != n))) {
            return invalid();
        }
        if (n > m_budget.remainingVertices() || n > m_used.max_size() || n > m_stack.max_size() ||
            n > m_edges.max_size() || n > m_joins.max_size()) { return limit(); }
        if (!pair()) { return false; }
        if (m_graph.offsets[0] != 0) { return invalid(); }
        std::size_t low = 0;
        for (std::size_t edge = 0; edge < n; ++edge) {
            if (!pair()) { return false; }
            const auto high = m_graph.offsets[edge + 1];
            if (high < low || high > m_graph.transitions.size()) { return invalid(); }
            if (!m_graph.closed) {
                if (m_graph.startEligible[edge] > 1 || m_graph.endEligible[edge] > 1) { return invalid(); }
                m_hasStart = m_hasStart || m_graph.startEligible[edge] != 0;
                m_hasEnd = m_hasEnd || m_graph.endEligible[edge] != 0;
            }
            for (std::size_t index = low; index < high; ++index) {
                if (!pair()) { return false; }
                const auto& join = m_graph.transitions[index];
                if (join.incoming != edge || join.outgoing >= n ||
                    (join.kind != CachePathJoinKind::SourceContinuation && join.kind != CachePathJoinKind::PairEvent &&
                     join.kind != CachePathJoinKind::Seam)) { return invalid(); }
                // Exact-key uniqueness is a producer contract, checked without sorting/copying input.
                for (std::size_t previous = low; previous < index; ++previous) {
                    if (!orderedPair()) { return false; }
                    if (sameTransition(m_graph.transitions[previous], join)) { return invalid(); }
                }
            }
            low = high;
        }
        return low == m_graph.transitions.size() || invalid();
    }

    bool prepare() {
        const auto n = m_graph.edgeCount;
        m_used.reserve(n); m_stack.reserve(n); m_edges.reserve(n); m_joins.reserve(n);
        if (!m_budget.consumeVertices(n)) { return false; }
        m_used.assign(n, 0);
        return true;
    }

    bool push(const std::size_t edge, const CachePathJoin* join) {
        if (m_stack.size() >= m_stack.capacity() || m_edges.size() >= m_edges.capacity() ||
            (join && m_joins.size() >= m_joins.capacity())) { return limit(); }
        if (!m_budget.consumeVertices(join ? 3 : 2)) { return false; }
        m_stack.push_back({edge, m_graph.offsets[edge], false});
        m_edges.push_back(edge); m_used[edge] = 1;
        if (join) { m_joins.push_back(*join); }
        return true;
    }
    void pop() {
        m_used[m_stack.back().edge] = 0; m_stack.pop_back(); m_edges.pop_back();
        if (!m_stack.empty()) { m_joins.pop_back(); }
    }
    bool visitRoute() {
        if (!pair()) { return false; }
        ++m_result.routesVisited;
        const auto decision = m_visitor({m_edges, m_joins, m_graph.closed});
        if (m_budget.status() != BudgetStatus::Ok) { return false; }
        if (decision == OffsetPathVisit::Stop) { m_result.status = OffsetPathSearchStatus::Stopped; return false; }
        return decision == OffsetPathVisit::Continue || invalid();
    }
    bool walk(const std::size_t seed) {
        if (!push(seed, nullptr)) { return false; }
        while (!m_stack.empty()) {
            if (!pair()) { return false; }
            auto& frame = m_stack.back();
            if (!frame.emitted) {
                frame.emitted = true;
                if (!m_graph.closed && m_graph.endEligible[frame.edge] && !visitRoute()) { return false; }
            }
            if (frame.successor == m_graph.offsets[frame.edge + 1]) { pop(); continue; }
            if (!orderedPair()) { return false; }
            const auto& join = m_graph.transitions[frame.successor++];
            if (m_graph.closed && join.outgoing == seed) {
                if (m_joins.size() >= m_joins.capacity()) { return limit(); }
                if (!m_budget.consumeVertices(1)) { return false; }
                m_joins.push_back(join);
                const bool keepGoing = visitRoute();
                m_joins.pop_back();
                if (!keepGoing) { return false; }
            } else if (!m_used[join.outgoing] && (!m_graph.closed || join.outgoing > seed)) {
                if (!push(join.outgoing, &join)) { return false; }
            }
        }
        return true;
    }

    const OffsetPathGraphView& m_graph;
    WorkBudget& m_budget;
    const OffsetPathVisitor& m_visitor;
    OffsetPathSearchResult m_result;
    std::vector<std::uint8_t> m_used;
    std::vector<Frame> m_stack;
    std::vector<std::size_t> m_edges;
    std::vector<CachePathJoin> m_joins;
    bool m_hasStart = false, m_hasEnd = false;
};

} // namespace

OffsetPathSearchResult searchOffsetPaths(const OffsetPathGraphView& graph, WorkBudget& budget,
                                        const OffsetPathVisitor& visitor) {
    return PathSearch{graph, budget, visitor}.run();
}

} // namespace LC_EllipseOffsetGeometry
