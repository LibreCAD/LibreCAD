/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include "lc_offsetpathsearch.h"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

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
    static constexpr std::size_t none = std::size_t(-1);
    static constexpr std::size_t bucketCount = 64;
    struct Frame { std::size_t edge, successor; bool emitted = false; std::size_t memo = none; };
    struct Memo {
        std::size_t seed = 0, edge = 0, next = 0;
        std::vector<std::uint64_t> used;
        std::uint8_t count = 0;
        bool complete = false;
    };
    struct MemoFrame { std::size_t memo, successor, child = none; };

    static std::uint8_t saturated(const unsigned a, const unsigned b) {
        return static_cast<std::uint8_t>(a + b < 2 ? a + b : 2);
    }

    OffsetPathSearchResult finish() {
        if (m_budget.status() == BudgetStatus::LimitExceeded) { m_result.status = OffsetPathSearchStatus::LimitExceeded; }
        else if (m_budget.status() == BudgetStatus::InvalidInput) { m_result.status = OffsetPathSearchStatus::InvalidInput; }
        m_result.work = m_budget.consumed();
        m_result.memoStates = m_memo.size();
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
        if (n > m_memoStack.max_size()) { return limit(); }
        m_used.reserve(n); m_stack.reserve(n); m_edges.reserve(n); m_joins.reserve(n); m_memoStack.reserve(n);
        if (!m_budget.consumeVertices(n)) { return false; }
        m_used.assign(n, 0);
        return true;
    }

    bool push(const std::size_t edge, const CachePathJoin* join, const std::size_t memo = none) {
        if (m_stack.size() >= m_stack.capacity() || m_edges.size() >= m_edges.capacity() ||
            (join && m_joins.size() >= m_joins.capacity())) { return limit(); }
        if (!m_budget.consumeVertices(join ? 3 : 2)) { return false; }
        m_stack.push_back({edge, m_graph.offsets[edge], false, memo});
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

    std::size_t wordCount() const { return m_graph.edgeCount / 64 + (m_graph.edgeCount % 64 != 0); }
    std::uint64_t word(const std::size_t parent, const std::size_t edge, const std::size_t i) const {
        return (parent == none ? 0 : m_memo[parent].used[i]) |
            (i == edge / 64 ? std::uint64_t{1} << (edge % 64) : 0);
    }
    bool findMemo(const std::size_t seed, const std::size_t edge, const std::size_t parent,
                  std::size_t& found, std::size_t& bucket) {
        // A fixed 64-head index is bounded scalar scratch, not 64 generated proofs.
        // Every data-dependent chain/key scan below has an owning debit.
        std::uint64_t hash = std::uint64_t(seed) * 0x9e3779b97f4a7c15ULL + edge;
        for (std::size_t i = 0; i < wordCount(); ++i) {
            if (!pair()) { return false; }
            hash = (hash ^ word(parent, edge, i)) * 0x100000001b3ULL;
        }
        bucket = hash % bucketCount; found = none;
        for (auto link = m_buckets[bucket]; link != 0; link = m_memo[link - 1].next) {
            if (!m_budget.adoptWork({0, 0, 1, 0, 2})) { return false; }
            const auto index = link - 1;
            if (m_memo[index].seed != seed || m_memo[index].edge != edge) { continue; }
            bool equal = true;
            for (std::size_t i = 0; i < wordCount(); ++i) {
                if (!orderedPair()) { return false; }
                if (m_memo[index].used[i] != word(parent, edge, i)) { equal = false; break; }
            }
            if (equal) { found = index; return true; }
        }
        return true;
    }
    bool insertMemo(const std::size_t seed, const std::size_t edge, const std::size_t parent,
                    const std::size_t bucket, std::size_t& index) {
        const auto words = wordCount();
        if (words > m_budget.remainingVertices() || m_budget.remainingVertices() - words < 2 ||
            m_memo.size() == m_memo.max_size() || m_memoStack.size() == m_memoStack.capacity()) { return limit(); }
        Memo entry; entry.seed = seed; entry.edge = edge; entry.next = m_buckets[bucket];
        if (words > entry.used.max_size()) { return limit(); }
        entry.used.reserve(words);
        // Do not reserve a caller-sized state space, and do not debit capacity.
        if (m_memo.size() == m_memo.capacity()) {
            const auto available = m_budget.remainingVertices() / (words + 2);
            const auto maximum = std::min(m_memo.max_size() - m_memo.size(), available);
            const auto growth = std::min(maximum, std::max<std::size_t>(1, m_memo.size()));
            if (growth == 0) { return limit(); }
            m_memo.reserve(m_memo.size() + growth);
        }
        static_assert(std::is_nothrow_move_constructible_v<Memo>);
        // One entered composite: key copy scan, immutable record and DFS frame.
        // Both counters are preflighted together; all following appends are reserved.
        if (!m_budget.adoptWork({0, words + 2, words, 0, 0})) { return false; }
        for (std::size_t i = 0; i < words; ++i) {
            entry.used.push_back(word(parent, edge, i));
        }
        if (!m_graph.closed && m_graph.endEligible[edge]) { entry.count = 1; }
        index = m_memo.size(); m_memo.push_back(std::move(entry));
        m_buckets[bucket] = index + 1;
        m_memoStack.push_back({index, m_graph.offsets[edge], none});
        return true;
    }
    bool countSuffixes(const std::size_t seed, std::size_t& root) {
        std::size_t bucket;
        if (!findMemo(seed, seed, none, root, bucket)) { return false; }
        if (root != none) { return m_memo[root].complete || invalid(); }
        if (!insertMemo(seed, seed, none, bucket, root)) { return false; }
        while (!m_memoStack.empty()) {
            if (!pair()) { return false; }
            auto& frame = m_memoStack.back();
            const auto index = frame.memo, edge = m_memo[index].edge;
            if (frame.child != none) {
                const auto child = frame.child; frame.child = none;
                if (!m_memo[child].complete) { return invalid(); }
                m_memo[index].count = saturated(m_memo[index].count, m_memo[child].count);
                continue;
            }
            if (frame.successor == m_graph.offsets[edge + 1]) {
                m_memo[index].complete = true; m_memoStack.pop_back(); continue;
            }
            if (!orderedPair()) { return false; }
            const auto outgoing = m_graph.transitions[frame.successor++].outgoing;
            if (m_graph.closed && outgoing == seed) {
                m_memo[index].count = saturated(m_memo[index].count, 1); continue;
            }
            if ((m_memo[index].used[outgoing / 64] & (std::uint64_t{1} << (outgoing % 64))) ||
                (m_graph.closed && outgoing < seed)) { continue; }
            std::size_t child;
            if (!findMemo(seed, outgoing, index, child, bucket)) { return false; }
            if (child != none) {
                if (!m_memo[child].complete) { return invalid(); }
                ++m_result.memoHits;
                m_memo[index].count = saturated(m_memo[index].count, m_memo[child].count);
            } else {
                // Store the parent continuation before insert can reallocate m_memo.
                frame.child = m_memo.size();
                if (!insertMemo(seed, outgoing, index, bucket, child)) { return false; }
            }
        }
        return true;
    }
    bool walk(const std::size_t seed) {
        if (!push(seed, nullptr)) { return false; }
        std::size_t root;
        if (!countSuffixes(seed, root)) { return false; }
        m_stack.back().memo = root;
        m_result.structuralRoutes = saturated(m_result.structuralRoutes, m_memo[root].count);
        if (m_memo[root].count == 0) { pop(); return true; }
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
                std::size_t child, bucket;
                if (!findMemo(seed, join.outgoing, frame.memo, child, bucket)) { return false; }
                if (child == none || !m_memo[child].complete) { return invalid(); }
                if (m_memo[child].count != 0 && !push(join.outgoing, &join, child)) { return false; }
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
    std::array<std::size_t, bucketCount> m_buckets{};
    std::vector<Memo> m_memo;
    std::vector<MemoFrame> m_memoStack;
    bool m_hasStart = false, m_hasEnd = false;
};

} // namespace

OffsetPathSearchResult searchOffsetPaths(const OffsetPathGraphView& graph, WorkBudget& budget,
                                        const OffsetPathVisitor& visitor) {
    return PathSearch{graph, budget, visitor}.run();
}

} // namespace LC_EllipseOffsetGeometry
