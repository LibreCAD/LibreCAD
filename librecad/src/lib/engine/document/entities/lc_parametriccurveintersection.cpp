/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_parametriccurveintersection.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <tuple>
#include <utility>

namespace {

constexpr double g_eps = std::numeric_limits<double>::epsilon();

bool isFinite(const RS_Vector& v) {
    return v.valid && std::isfinite(v.x) && std::isfinite(v.y);
}

double mid(const LC_Interval& v) {
    return 0.5 * (v.lo() + v.hi());
}

LC_Interval intersect(const LC_Interval& a, const LC_Interval& b) {
    if (!a.isValid() || !b.isValid() || a.hi() < b.lo() || b.hi() < a.lo()) {
        return {};
    }
    return LC_Interval::hull(std::max(a.lo(), b.lo()), std::min(a.hi(), b.hi()));
}

/** The largest distance from @p p to a point of the box. */
double farthest(const LC_Interval& x, const LC_Interval& y, const RS_Vector& p) {
    const double dx = std::max(std::abs(x.lo() - p.x), std::abs(x.hi() - p.x));
    const double dy = std::max(std::abs(y.lo() - p.y), std::abs(y.hi() - p.y));
    return std::hypot(dx, dy);
}

/** The gap between two boxes; zero when they overlap. */
double gap(const LC_Interval& ax, const LC_Interval& ay, const LC_Interval& bx, const LC_Interval& by) {
    const double gx = std::max({0.0, ax.lo() - bx.hi(), bx.lo() - ax.hi()});
    const double gy = std::max({0.0, ay.lo() - by.hi(), by.lo() - ay.hi()});
    return std::hypot(gx, gy);
}

/**
 * The derivative box excludes zero and its directions span less than a right
 * angle: a curve piece with such tangents is monotone along their mean
 * direction, so it cannot cross itself, nor a neighbour whose tangents it shares
 * at their common end.
 */
bool isMonotone(const LC_Interval& dx, const LC_Interval& dy) {
    if (!dx.isValid() || !dy.isValid() || !std::isfinite(dx.width()) || !std::isfinite(dy.width()) ||
        (dx.containsZero() && dy.containsZero())) {
        return false;
    }
    const double centre = std::atan2(mid(dy), mid(dx));
    double lo = 0.0;
    double hi = 0.0;
    for (const double x : {dx.lo(), dx.hi()}) {
        for (const double y : {dy.lo(), dy.hi()}) {
            const double delta = std::remainder(std::atan2(y, x) - centre, 2.0 * M_PI);
            lo = std::min(lo, delta);
            hi = std::max(hi, delta);
        }
    }
    return hi - lo < 0.5 * M_PI;
}

/**
 * The directions a derivative box holds, as angles from @p reference in
 * [lo, hi]; false when the box holds zero, and so every direction.
 */
bool directions(const LC_Interval& dx, const LC_Interval& dy, const double reference, double& lo, double& hi) {
    if (!dx.isValid() || !dy.isValid() || !std::isfinite(dx.width()) || !std::isfinite(dy.width()) ||
        (dx.containsZero() && dy.containsZero())) {
        return false;
    }
    const double centre = std::atan2(mid(dy), mid(dx));
    lo = 0.0;
    hi = 0.0;
    for (const double x : {dx.lo(), dx.hi()}) {
        for (const double y : {dy.lo(), dy.hi()}) {
            const double delta = std::remainder(std::atan2(y, x) - centre, 2.0 * M_PI);
            lo = std::min(lo, delta);
            hi = std::max(hi, delta);
        }
    }
    const double shift = std::remainder(centre - reference, 2.0 * M_PI);
    lo += shift;
    hi += shift;
    return true;
}

/**
 * Two derivative boxes whose directions together lie in an open half plane:
 * pieces with such tangents that share an end are monotone along the middle
 * direction together, and meet nowhere else.
 */
bool jointlyMonotone(const LC_Interval& dx1, const LC_Interval& dy1, const LC_Interval& dx2, const LC_Interval& dy2) {
    const double reference = std::atan2(mid(dy1), mid(dx1));
    double lo1 = 0.0;
    double hi1 = 0.0;
    double lo2 = 0.0;
    double hi2 = 0.0;
    return directions(dx1, dy1, reference, lo1, hi1) && directions(dx2, dy2, reference, lo2, hi2) &&
           std::max(hi1, hi2) - std::min(lo1, lo2) < M_PI;
}

struct Leaf {
    std::size_t segment;
    std::size_t branch;
    /** Position among the leaves of its branch. */
    std::size_t order;
    double a;
    double b;
    bool monotone;
    bool cornerNext;
    LC_Interval x;
    LC_Interval y;
    LC_Interval dx;
    LC_Interval dy;
};

struct Bounds {
    LC_Interval x;
    LC_Interval y;
    LC_Interval dx;
    LC_Interval dy;
};

class Solver {
public:
    Solver(const LC_ParametricCurves& curves, const LC_IntersectionOptions& options)
        : m_curves{curves},
          m_options{options},
          m_segments{curves.segments()} {
    }

    LC_IntersectionResult run() {
        LC_IntersectionResult result;
        if (!(m_options.tolerance > 0.0) || !std::isfinite(m_options.tolerance) || !validSegments()) {
            return result;
        }
        result.status = buildLeaves();
        if (result.status == LC_IntersectionStatus::Ok) {
            result.status = findJoins();
        }
        if (result.status == LC_IntersectionStatus::Ok) {
            result.status = findCorners();
        }
        std::vector<std::pair<std::size_t, std::size_t>> pairs;
        if (result.status == LC_IntersectionStatus::Ok) {
            result.status = candidatePairs(pairs);
        }
        if (result.status == LC_IntersectionStatus::Ok) {
            for (const auto& [i, j] : pairs) {
                result.status = solvePair(m_leaves[i], m_leaves[j]);
                if (result.status != LC_IntersectionStatus::Ok) {
                    break;
                }
            }
        }
        if (result.status == LC_IntersectionStatus::Ok) {
            result.intersections = deduplicated();
        }
        result.work = m_work;
        result.work.generatedLeaves = m_leaves.size();
        return result;
    }

private:
    struct Join {
        std::size_t from;
        std::size_t to;
        RS_Vector point;
    };

    bool validSegments() const {
        const std::size_t branches = m_curves.branchCount();
        for (std::size_t i = 0; i < m_segments.size(); ++i) {
            const LC_ParametricSegment& s = m_segments[i];
            if (s.branch >= branches || !(s.t0 < s.t1) || !std::isfinite(s.t0) || !std::isfinite(s.t1) ||
                (i > 0 && s.branch < m_segments[i - 1].branch)) {
                return false;
            }
        }
        return !m_segments.empty();
    }

    bool bound(const std::size_t segment, const double a, const double b, Bounds& out) const {
        return m_curves.bound(segment, a, b, out.x, out.y, out.dx, out.dy) && out.x.isValid() && out.y.isValid() &&
               out.dx.isValid() && out.dy.isValid();
    }

    /** Leaves in branch order, each monotone or no wider than the tolerance. */
    LC_IntersectionStatus buildLeaves() {
        m_branchLeaves.assign(m_curves.branchCount(), 0);
        for (std::size_t s = 0; s < m_segments.size(); ++s) {
            struct Box {
                double a;
                double b;
                unsigned depth;
            };
            std::vector<Box> stack{{m_segments[s].t0, m_segments[s].t1, 0}};
            while (!stack.empty()) {
                const Box box = stack.back();
                stack.pop_back();
                Bounds bounds;
                const bool bounded = bound(s, box.a, box.b, bounds);
                const bool monotone = bounded && isMonotone(bounds.dx, bounds.dy);
                const bool small = bounded && std::hypot(bounds.x.width(), bounds.y.width()) <= m_options.tolerance;
                if (monotone || small) {
                    if (m_leaves.size() >= m_options.maxLeaves) {
                        return LC_IntersectionStatus::LimitExceeded;
                    }
                    const std::size_t branch = m_segments[s].branch;
                    m_leaves.push_back(Leaf{s, branch, m_branchLeaves[branch]++, box.a, box.b, monotone, false, bounds.x,
                                            bounds.y, bounds.dx, bounds.dy});
                    continue;
                }
                const double half = box.a + 0.5 * (box.b - box.a);
                if (box.depth >= m_options.maxLeafDepth || !(half > box.a && half < box.b)) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                stack.push_back({half, box.b, box.depth + 1});
                stack.push_back({box.a, half, box.depth + 1});
            }
        }
        return LC_IntersectionStatus::Ok;
    }

    /** Where a branch runs on into another (or itself): a place, not a crossing. */
    LC_IntersectionStatus findJoins() {
        for (std::size_t branch = 0; branch < m_curves.branchCount(); ++branch) {
            const std::ptrdiff_t next = m_curves.next(branch);
            if (next < 0) {
                continue;
            }
            if (static_cast<std::size_t>(next) >= m_curves.branchCount()) {
                return LC_IntersectionStatus::InvalidInput;
            }
            std::size_t last = m_segments.size();
            for (std::size_t s = 0; s < m_segments.size(); ++s) {
                last = (m_segments[s].branch == branch) ? s : last;
            }
            RS_Vector point;
            RS_Vector derivative;
            if (last == m_segments.size() || !m_curves.evaluate(last, m_segments[last].t1, point, derivative) ||
                !isFinite(point)) {
                return LC_IntersectionStatus::InvalidInput;
            }
            m_joins.push_back(Join{branch, static_cast<std::size_t>(next), point});
        }
        return LC_IntersectionStatus::Ok;
    }

    /** Adjacent segments not proved jointly monotone may intersect again. */
    LC_IntersectionStatus findCorners() {
        for (std::size_t i = 0; i + 1 < m_leaves.size(); ++i) {
            const Leaf& p = m_leaves[i];
            const Leaf& q = m_leaves[i + 1];
            if (p.branch != q.branch || p.segment == q.segment || jointlyMonotone(p.dx, p.dy, q.dx, q.dy)) {
                continue;
            }
            RS_Vector point;
            RS_Vector derivative;
            if (!m_curves.evaluate(p.segment, p.b, point, derivative) ||
                !isFinite(point)) {
                return LC_IntersectionStatus::InvalidInput;
            }
            m_leaves[i].cornerNext = true;
            m_corners.push_back(Corner{p.segment, q.segment, point});
        }
        return LC_IntersectionStatus::Ok;
    }

    bool hasPredecessor(const std::size_t branch) const {
        return std::any_of(m_joins.begin(), m_joins.end(), [branch](const Join& j) { return j.to == branch; });
    }

    /** Consecutive leaves of a branch, either side of a corner (findCorners()). */
    bool atCorner(const Leaf& p, const Leaf& q) const {
        if (p.branch != q.branch || (p.order + 1 != q.order && q.order + 1 != p.order)) {
            return false;
        }
        const Leaf& first = (p.order < q.order) ? p : q;
        return first.cornerNext;
    }

    /** Smooth neighbours, or leaves across a cusp join, share only an end. */
    bool adjacent(const Leaf& p, const Leaf& q) const {
        if (p.branch == q.branch && (p.order + 1 == q.order || q.order + 1 == p.order)) {
            return !atCorner(p, q);
        }
        for (const Join& j : m_joins) {
            const std::size_t lastOfFrom = m_branchLeaves[j.from] - 1;
            if ((p.branch == j.from && p.order == lastOfFrom && q.branch == j.to && q.order == 0) ||
                (q.branch == j.from && q.order == lastOfFrom && p.branch == j.to && p.order == 0)) {
                return true;
            }
        }
        return false;
    }

    bool consumePair(std::size_t& phaseCount) {
        if (m_work.totalBoxPairs >= m_options.maxBoxPairs) {
            return false;
        }
        // Each phase count is at most the total, so both increments are representable.
        ++m_work.totalBoxPairs;
        ++phaseCount;
        return true;
    }

    /**
     * Pairs of leaves whose bounds come within the tolerance, other than a
     * leaf with itself or its neighbours; a balanced box hierarchy prunes on
     * both axes before leaf pairs are retained in a fixed order.
     */
    LC_IntersectionStatus candidatePairs(std::vector<std::pair<std::size_t, std::size_t>>& pairs) {
        if (m_leaves.size() < 2 || m_options.anchoredBranches == 0) {
            return LC_IntersectionStatus::Ok;
        }
        struct Node {
            LC_Interval x;
            LC_Interval y;
            std::size_t count;
            std::size_t leaf;
            std::size_t firstLeaf;
            std::size_t lastLeaf;
            std::size_t branch;
            double a;
            double b;
            bool singleBranch;
            bool anchored;
            std::size_t left{0};
            std::size_t right{0};
        };
        const std::size_t count = m_leaves.size();
        std::vector<Node> nodes;
        std::vector<std::size_t> order;
        std::vector<std::array<double, 2>> keys;
        std::vector<Bounds> parameterBounds;
        if (count > nodes.max_size() || count - 1 > nodes.max_size() - count ||
            count > order.max_size() || count > keys.max_size() || count > parameterBounds.max_size() / 2) {
            return LC_IntersectionStatus::LimitExceeded;
        }
        nodes.reserve(count + (count - 1));
        order.resize(count);
        keys.resize(count);
        parameterBounds.resize(2 * count);
        const auto united = [](const Bounds& a, const Bounds& b) {
            return Bounds{LC_Interval::hull(a.x, b.x), LC_Interval::hull(a.y, b.y),
                          LC_Interval::hull(a.dx, b.dx), LC_Interval::hull(a.dy, b.dy)};
        };
        const auto centre = [](const LC_Interval& interval) {
            // Keys only partition the tree; an unbounded box must remain conservative, not yield a NaN key.
            return std::isfinite(interval.lo()) && std::isfinite(interval.hi())
                       ? 0.5 * interval.lo() + 0.5 * interval.hi() : 0.0;
        };
        for (std::size_t i = 0; i < count; ++i) {
            order[i] = i;
            keys[i] = {centre(m_leaves[i].x), centre(m_leaves[i].y)};
            parameterBounds[count + i] = Bounds{m_leaves[i].x, m_leaves[i].y, m_leaves[i].dx, m_leaves[i].dy};
        }
        for (std::size_t i = count - 1; i != 0; --i) {
            parameterBounds[i] = united(parameterBounds[2 * i], parameterBounds[2 * i + 1]);
        }
        const auto cachedBridge = [&](const std::size_t first, const std::size_t last) {
            // Original leaf order covers the complete intervening path, unlike spatial-tree descendants.
            Bounds all = parameterBounds[count + first];
            std::size_t left = count + first;
            std::size_t right = count + last + 1;
            while (left < right) {
                if ((left & 1u) != 0) {
                    all = united(all, parameterBounds[left++]);
                }
                if ((right & 1u) != 0) {
                    all = united(all, parameterBounds[--right]);
                }
                left /= 2;
                right /= 2;
            }
            return isMonotone(all.dx, all.dy) ||
                   std::hypot(all.x.width(), all.y.width()) <= m_options.tolerance;
        };
        const auto build = [&](auto&& self, const std::size_t first, const std::size_t last) -> std::size_t {
            const Leaf& leaf = m_leaves[order[first]];
            Node node{leaf.x, leaf.y, last - first, order[first], order[first], order[first],
                      leaf.branch, leaf.a, leaf.b, true, false};
            std::array<double, 2> low = keys[order[first]];
            std::array<double, 2> high = low;
            for (std::size_t i = first; i < last; ++i) {
                const std::size_t index = order[i];
                node.x = LC_Interval::hull(node.x, m_leaves[index].x);
                node.y = LC_Interval::hull(node.y, m_leaves[index].y);
                node.a = std::min(node.a, m_leaves[index].a);
                node.b = std::max(node.b, m_leaves[index].b);
                node.firstLeaf = std::min(node.firstLeaf, index);
                node.lastLeaf = std::max(node.lastLeaf, index);
                node.singleBranch = node.singleBranch && m_leaves[index].branch == node.branch;
                node.anchored = node.anchored || m_leaves[index].branch < m_options.anchoredBranches;
                for (std::size_t axis = 0; axis < 2; ++axis) {
                    low[axis] = std::min(low[axis], keys[index][axis]);
                    high[axis] = std::max(high[axis], keys[index][axis]);
                }
            }
            const std::size_t index = nodes.size();
            nodes.push_back(node);
            if (node.count > 1) {
                // Halved key extents cannot overflow, even for opposite-sign near-maximum coordinates.
                const std::size_t axis = (0.5 * high[1] - 0.5 * low[1] >
                                          0.5 * high[0] - 0.5 * low[0]) ? 1 : 0;
                const std::size_t half = first + node.count / 2;
                std::nth_element(order.begin() + first, order.begin() + half, order.begin() + last,
                                 [&](const std::size_t a, const std::size_t b) {
                    return std::make_pair(keys[a][axis], a) < std::make_pair(keys[b][axis], b);
                });
                const std::size_t left = self(self, first, half);
                const std::size_t right = self(self, half, last);
                nodes[index].left = left;
                nodes[index].right = right;
            }
            return index;
        };
        const std::size_t root = build(build, 0, count);

        // Balanced descent has at most twice the tree height's pending siblings, independent of pair density.
        std::array<std::pair<std::size_t, std::size_t>, 2 * std::numeric_limits<std::size_t>::digits + 1> pending;
        std::size_t pendingCount = 0;
        const auto push = [&](const std::size_t a, const std::size_t b) {
            if (pendingCount == pending.size()) {
                return false;
            }
            pending[pendingCount++] = {a, b};
            return true;
        };
        push(root, root);
        while (pendingCount != 0) {
            const auto [a, b] = pending[--pendingCount];
            const Node& p = nodes[a];
            const Node& q = nodes[b];
            if (a == b) {
                if (!p.anchored || p.count == 1) {
                    continue;
                }
                // These three disjoint pair sets enumerate each unordered leaf pair exactly once.
                if (!push(p.right, p.right) || !push(p.left, p.right) || !push(p.left, p.left)) {
                    return LC_IntersectionStatus::LimitExceeded;
                }
                continue;
            }
            if (!consumePair(m_work.candidateBoxPairs)) {
                return LC_IntersectionStatus::LimitExceeded;
            }
            if ((!p.anchored && !q.anchored) || gap(p.x, p.y, q.x, q.y) > m_options.tolerance) {
                continue;
            }
            // Prove the whole intervening branch range, not just these spatial subsets.
            if (p.singleBranch && q.singleBranch && p.branch == q.branch &&
                (cachedBridge(std::min(p.firstLeaf, q.firstLeaf), std::max(p.lastLeaf, q.lastLeaf)) ||
                 bridgedRange(p.branch, std::min(p.a, q.a), std::max(p.b, q.b)))) {
                continue;
            }
            if (p.count == 1 && q.count == 1) {
                const std::size_t first = std::min(p.leaf, q.leaf);
                const std::size_t second = std::max(p.leaf, q.leaf);
                if (!adjacent(m_leaves[first], m_leaves[second])) {
                    pairs.emplace_back(first, second);
                }
            }
            else if (q.count == 1 || (p.count > 1 && p.count >= q.count)) {
                if (!push(p.right, b) || !push(p.left, b)) {
                    return LC_IntersectionStatus::LimitExceeded;
                }
            }
            else if (!push(a, q.right) || !push(a, q.left)) {
                return LC_IntersectionStatus::LimitExceeded;
            }
        }
        std::sort(pairs.begin(), pairs.end());
        return LC_IntersectionStatus::Ok;
    }

    /** Exclude only the joined segments' shared endpoint, within twice the tolerance. */
    bool insideJoin(const Leaf& p, const Bounds& bp, const Leaf& q, const Bounds& bq) const {
        const double radius = 2.0 * m_options.tolerance;
        for (const Corner& c : m_corners) {
            const bool between = (p.segment == c.before && q.segment == c.after) ||
                                 (q.segment == c.before && p.segment == c.after);
            if (between && farthest(bp.x, bp.y, c.point) <= radius && farthest(bq.x, bq.y, c.point) <= radius) {
                return true;
            }
        }
        for (const Join& j : m_joins) {
            const bool between = (p.branch == j.from && q.branch == j.to) || (p.branch == j.to && q.branch == j.from);
            if (between && farthest(bp.x, bp.y, j.point) <= radius && farthest(bq.x, bq.y, j.point) <= radius) {
                return true;
            }
        }
        return false;
    }

    struct Pair {
        double a0;
        double a1;
        double b0;
        double b1;
        unsigned depth;
    };

    LC_IntersectionStatus solvePair(const Leaf& p, const Leaf& q) {
        const bool corner = atCorner(p, q);
        std::vector<Pair> stack{{p.a, p.b, q.a, q.b, 0}};
        while (!stack.empty()) {
            if (!consumePair(m_work.refinementBoxPairs)) {
                return LC_IntersectionStatus::LimitExceeded;
            }
            const Pair box = stack.back();
            stack.pop_back();
            Bounds bp;
            Bounds bq;
            if (!bound(p.segment, box.a0, box.a1, bp) || !bound(q.segment, box.b0, box.b1, bq)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            if (gap(bp.x, bp.y, bq.x, bq.y) > 0.0 || insideJoin(p, bp, q, bq) || bridged(p, q, box)) {
                continue;
            }
            // Pair indices follow branch order. Locally monotone sides can only share their endpoint.
            if (corner && box.a1 == p.b && box.b0 == q.a && jointlyMonotone(bp.dx, bp.dy, bq.dx, bq.dy)) {
                RS_Vector end;
                RS_Vector start;
                RS_Vector derivative;
                if (!m_curves.evaluate(p.segment, p.b, end, derivative) ||
                    !m_curves.evaluate(q.segment, q.a, start, derivative)) {
                    return LC_IntersectionStatus::InvalidInput;
                }
                // Fitted offset joins may only be tolerance-close, not exactly shared.
                if (end.x == start.x && end.y == start.y) {
                    continue;
                }
            }
            const double sizeP = std::hypot(bp.x.width(), bp.y.width());
            const double sizeQ = std::hypot(bq.x.width(), bq.y.width());
            {
                const Krawczyk k = krawczyk(p, q, box);
                if (k == Krawczyk::NoRoot) {
                    continue;
                }
                if (k == Krawczyk::Unique) {
                    const LC_IntersectionStatus status = refineAndRecord(p, q, box);
                    if (status != LC_IntersectionStatus::Ok) {
                        return status;
                    }
                    continue;
                }
            }
            if (box.depth >= m_options.maxDepth) {
                if (corner) {
                    return LC_IntersectionStatus::LimitExceeded;
                }
                const LC_IntersectionStatus status = classifyContact(p, q, box);
                if (status != LC_IntersectionStatus::Ok) {
                    return status;
                }
                continue;
            }
            // split the side whose image is larger
            if (sizeP >= sizeQ) {
                const double h = box.a0 + 0.5 * (box.a1 - box.a0);
                stack.push_back({box.a0, h, box.b0, box.b1, box.depth + 1});
                stack.push_back({h, box.a1, box.b0, box.b1, box.depth + 1});
            }
            else {
                const double h = box.b0 + 0.5 * (box.b1 - box.b0);
                stack.push_back({box.a0, box.a1, box.b0, h, box.depth + 1});
                stack.push_back({box.a0, box.a1, h, box.b1, box.depth + 1});
            }
        }
        return LC_IntersectionStatus::Ok;
    }

    /**
     * Two boxes of one branch with the stretch between them monotone, or within
     * the tolerance, across its segments too: nothing there crosses, beyond a
     * loop too small to resolve, as where the branch all but stops.
     */
    bool bridged(const Leaf& p, const Leaf& q, const Pair& box) const {
        if (p.branch != q.branch) {
            return false;
        }
        return bridgedRange(p.branch, std::min(box.a0, box.b0), std::max(box.a1, box.b1));
    }

    bool bridgedRange(const std::size_t branch, const double lo, const double hi) const {
        Bounds all;
        bool any = false;
        for (std::size_t k = 0; k < m_segments.size(); ++k) {
            const LC_ParametricSegment& seg = m_segments[k];
            // A span touching only the range boundary has no interval to bound.
            if (seg.branch != branch || seg.t1 <= lo || seg.t0 >= hi) {
                continue;
            }
            Bounds b;
            if (!bound(k, std::max(lo, seg.t0), std::min(hi, seg.t1), b)) {
                return false;
            }
            if (!any) {
                all = b;
                any = true;
                continue;
            }
            all.x = LC_Interval::hull(all.x, b.x);
            all.y = LC_Interval::hull(all.y, b.y);
            all.dx = LC_Interval::hull(all.dx, b.dx);
            all.dy = LC_Interval::hull(all.dy, b.dy);
        }
        return any &&
               (isMonotone(all.dx, all.dy) || std::hypot(all.x.width(), all.y.width()) <= m_options.tolerance);
    }

    enum class Krawczyk {
        NoRoot,
        Unique,
        Undecided
    };

    /**
     * The Krawczyk operator for F(t, u) = A(t) - B(u) over the box, slightly
     * inflated within the segments so a root on its edge is still proved: no
     * root when it misses the box, exactly one when it maps into its interior.
     */
    Krawczyk krawczyk(const Leaf& p, const Leaf& q, const Pair& box) const {
        const LC_ParametricSegment& sp = m_segments[p.segment];
        const LC_ParametricSegment& sq = m_segments[q.segment];
        const double ea = 1e-3 * (box.a1 - box.a0);
        const double eb = 1e-3 * (box.b1 - box.b0);
        const LC_Interval t = LC_Interval::hull(std::max(sp.t0, box.a0 - ea), std::min(sp.t1, box.a1 + ea));
        const LC_Interval u = LC_Interval::hull(std::max(sq.t0, box.b0 - eb), std::min(sq.t1, box.b1 + eb));
        Bounds bp;
        Bounds bq;
        if (!bound(p.segment, t.lo(), t.hi(), bp) || !bound(q.segment, u.lo(), u.hi(), bq)) {
            return Krawczyk::Undecided;
        }
        const double mt = mid(t);
        const double mu = mid(u);
        RS_Vector a;
        RS_Vector da;
        RS_Vector b;
        RS_Vector db;
        if (!m_curves.evaluate(p.segment, mt, a, da) || !m_curves.evaluate(q.segment, mu, b, db) || !isFinite(a) ||
            !isFinite(b) || !isFinite(da) || !isFinite(db)) {
            return Krawczyk::Undecided;
        }
        // Jm = [da, -db], and its inverse Y
        const double det = -da.x * db.y + db.x * da.y;
        if (!(std::abs(det) > 1e-12 * da.magnitude() * db.magnitude())) {
            return Krawczyk::Undecided;
        }
        const double y00 = -db.y / det;
        const double y01 = db.x / det;
        const double y10 = -da.y / det;
        const double y11 = da.x / det;
        // F(m) with a margin for evaluation error
        const double margin = 64.0 * g_eps * (std::abs(a.x) + std::abs(a.y) + std::abs(b.x) + std::abs(b.y));
        const LC_Interval fx = LC_Interval::hull(a.x - b.x - margin, a.x - b.x + margin);
        const LC_Interval fy = LC_Interval::hull(a.y - b.y - margin, a.y - b.y + margin);
        const auto pt = [](const double v) { return LC_Interval::point(v); };
        // M = I - Y J with J = [[A'x, -B'x], [A'y, -B'y]] over the box
        const LC_Interval m00 = pt(1.0) - (pt(y00) * bp.dx + pt(y01) * bp.dy);
        const LC_Interval m01 = pt(0.0) - (pt(y00) * (-bq.dx) + pt(y01) * (-bq.dy));
        const LC_Interval m10 = pt(0.0) - (pt(y10) * bp.dx + pt(y11) * bp.dy);
        const LC_Interval m11 = pt(1.0) - (pt(y10) * (-bq.dx) + pt(y11) * (-bq.dy));
        const LC_Interval dt = t - pt(mt);
        const LC_Interval du = u - pt(mu);
        const LC_Interval kt = pt(mt) - (pt(y00) * fx + pt(y01) * fy) + (m00 * dt + m01 * du);
        const LC_Interval ku = pt(mu) - (pt(y10) * fx + pt(y11) * fy) + (m10 * dt + m11 * du);
        if (!kt.isValid() || !ku.isValid()) {
            return Krawczyk::Undecided;
        }
        if (!intersect(kt, t).isValid() || !intersect(ku, u).isValid()) {
            return Krawczyk::NoRoot;
        }
        if (kt.lo() > t.lo() && kt.hi() < t.hi() && ku.lo() > u.lo() && ku.hi() < u.hi()) {
            return Krawczyk::Unique;
        }
        return Krawczyk::Undecided;
    }

    /** Newton steps from the box centre for a root proved unique there. */
    LC_IntersectionStatus refineAndRecord(const Leaf& p, const Leaf& q, const Pair& box) {
        const LC_ParametricSegment& sp = m_segments[p.segment];
        const LC_ParametricSegment& sq = m_segments[q.segment];
        const double ea = 1e-3 * (box.a1 - box.a0);
        const double eb = 1e-3 * (box.b1 - box.b0);
        const double t0 = std::max(sp.t0, box.a0 - ea);
        const double t1 = std::min(sp.t1, box.a1 + ea);
        const double u0 = std::max(sq.t0, box.b0 - eb);
        const double u1 = std::min(sq.t1, box.b1 + eb);
        double t = 0.5 * (t0 + t1);
        double u = 0.5 * (u0 + u1);
        RS_Vector a;
        RS_Vector da;
        RS_Vector b;
        RS_Vector db;
        for (int iteration = 0; iteration < 60; ++iteration) {
            if (!m_curves.evaluate(p.segment, t, a, da) || !m_curves.evaluate(q.segment, u, b, db)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            const RS_Vector f = a - b;
            const double det = -da.x * db.y + db.x * da.y;
            if (!(det != 0.0) || !std::isfinite(det)) {
                break;
            }
            const double stepT = (-db.y * f.x + db.x * f.y) / det;
            const double stepU = (-da.y * f.x + da.x * f.y) / det;
            const double nextT = std::clamp(t - stepT, t0, t1);
            const double nextU = std::clamp(u - stepU, u0, u1);
            if (nextT == t && nextU == u) {
                break;
            }
            t = nextT;
            u = nextU;
        }
        if (!m_curves.evaluate(p.segment, t, a, da) || !m_curves.evaluate(q.segment, u, b, db)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        if (a.distanceTo(b) > m_options.tolerance) {
            return LC_IntersectionStatus::AmbiguousTopology; // the proved root was not reached
        }
        record(p, t, q, u, (a + b) * 0.5, LC_ParametricIntersection::Kind::Transverse);
        return LC_IntersectionStatus::Ok;
    }

    /**
     * A box the Krawczyk test could not decide down to the finest size: the
     * branches are tangent there, or nearly so, or retrace each other. From the
     * nearest pair of points, a contact within the tolerance is a crossing when
     * the second branch changes sides of the first's tangent across it and a
     * tangency when it does not; a branch that stays within the tolerance on
     * both sides retraces the other, which is ambiguous, and so is a box whose
     * branches do not come that close. Two branch ends that meet are an end
     * contact.
     */
    LC_IntersectionStatus classifyContact(const Leaf& p, const Leaf& q, const Pair& box) {
        const LC_ParametricSegment& sq = m_segments[q.segment];
        RS_Vector a;
        RS_Vector da;
        RS_Vector b;
        RS_Vector db;
        // Two branch ends that meet, as the ends of an open curve that looks
        // closed, are an end contact however they meet: where they are
        // tangent the nearest pair below is not found, nor a side.
        for (const double endA : {box.a0, box.a1}) {
            for (const double endB : {box.b0, box.b1}) {
                if (m_curves.evaluate(p.segment, endA, a, da) && m_curves.evaluate(q.segment, endB, b, db) &&
                    isFinite(a) && isFinite(da) && isFinite(b) && isFinite(db) &&
                    a.distanceTo(b) <= m_options.tolerance && atBranchEnd(p.segment, endA) &&
                    atBranchEnd(q.segment, endB)) {
                    const RS_Vector contact = (a + b) * 0.5;
                    if (!isFinite(contact)) {
                        return LC_IntersectionStatus::AmbiguousTopology;
                    }
                    record(p, endA, q, endB, contact, LC_ParametricIntersection::Kind::Endpoint);
                    return LC_IntersectionStatus::Ok;
                }
            }
        }
        double t = 0.5 * (box.a0 + box.a1);
        double u = 0.5 * (box.b0 + box.b1);
        // Gauss-Newton towards the nearest pair within the two leaves
        for (int iteration = 0; iteration < 40; ++iteration) {
            if (!std::isfinite(t) || !std::isfinite(u) || !m_curves.evaluate(p.segment, t, a, da) ||
                !m_curves.evaluate(q.segment, u, b, db) || !isFinite(a) || !isFinite(da) || !isFinite(b) ||
                !isFinite(db)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            const RS_Vector f = a - b;
            const double h00 = da.squared();
            const double h01 = -RS_Vector::dotP(da, db);
            const double h11 = db.squared();
            const double g0 = RS_Vector::dotP(da, f);
            const double g1 = -RS_Vector::dotP(db, f);
            const double det = h00 * h11 - h01 * h01;
            if (!(det > 0.0) || !std::isfinite(det)) {
                break;
            }
            const double trialT = t - (h11 * g0 - h01 * g1) / det;
            const double trialU = u - (h00 * g1 - h01 * g0) / det;
            if (!std::isfinite(trialT) || !std::isfinite(trialU)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            const double nextT = std::clamp(trialT, p.a, p.b);
            const double nextU = std::clamp(trialU, q.a, q.b);
            if (nextT == t && nextU == u) {
                break;
            }
            t = nextT;
            u = nextU;
        }
        if (!std::isfinite(t) || !std::isfinite(u) || !m_curves.evaluate(p.segment, t, a, da) ||
            !m_curves.evaluate(q.segment, u, b, db) || !isFinite(a) || !isFinite(da) || !isFinite(b) || !isFinite(db)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        const double tolerance = m_options.tolerance;
        const double distance = a.distanceTo(b);
        const double speedA = da.magnitude();
        const double speedB = db.magnitude();
        const RS_Vector contact = (a + b) * 0.5;
        if (!std::isfinite(distance) || distance > tolerance || !(speedA > 0.0) || !std::isfinite(speedA) ||
            !(speedB > 0.0) || !std::isfinite(speedB) || !isFinite(contact)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        const double initialStep = 8.0 * tolerance / speedB;
        if (!(initialStep > 0.0) || !std::isfinite(initialStep)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        // Which side of the first branch the second is on, on either side of
        // the contact: stepping away along it until they are more than the
        // tolerance apart, beyond the leaf if need be. Branches that meet at a
        // shallow angle part slowly: if they stay within the tolerance over the
        // whole segment, the side furthest from the first above rounding noise
        // decides. Staying within the noise is retracing.
        const double noise = 1024.0 * g_eps * (std::abs(a.x) + std::abs(a.y) + std::abs(b.x) + std::abs(b.y));
        if (!std::isfinite(noise)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        // Enough doublings to span every positive double, including subnormals.
        constexpr int maxContactSteps = std::numeric_limits<double>::max_exponent -
                                        std::numeric_limits<double>::min_exponent +
                                        std::numeric_limits<double>::digits + 1;
        int sides[2] = {0, 0};
        double farthest[2] = {0.0, 0.0};
        // how far along the second branch the contact reaches, on each side
        double reach[2] = {q.a, q.b};
        for (int k = 0; k < 2; ++k) {
            const double direction = (k == 0) ? -1.0 : 1.0;
            double step = initialStep;
            bool finished = false;
            for (int iteration = 0; iteration < maxContactSteps; ++iteration) {
                const double s = u + direction * step;
                if (!std::isfinite(s)) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                if (s < sq.t0 || s > sq.t1) {
                    finished = true;
                    break;
                }
                reach[k] = s;
                double side = 0.0;
                if (!sideOf(p, t, q.segment, s, side)) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                if (std::abs(side) > std::abs(farthest[k])) {
                    farthest[k] = side;
                }
                if (std::abs(side) > tolerance) {
                    sides[k] = side > 0.0 ? 1 : -1;
                    finished = true;
                    break;
                }
                // A positive step may round s back to u; let it grow until representable.
                const double nextStep = 2.0 * step;
                if (!std::isfinite(nextStep) || !(nextStep > step)) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                step = nextStep;
            }
            if (!finished) {
                return LC_IntersectionStatus::LimitExceeded;
            }
            if (sides[k] == 0 && std::abs(farthest[k]) > noise) {
                sides[k] = farthest[k] > 0.0 ? 1 : -1;
            }
        }
        if (sides[0] == 0 && sides[1] == 0) {
            return LC_IntersectionStatus::AmbiguousTopology; // retraced
        }
        const bool crosses = sides[0] != 0 && sides[1] != 0 && sides[0] != sides[1];
        record(p, t, q, u, contact,
               crosses ? LC_ParametricIntersection::Kind::Transverse : LC_ParametricIntersection::Kind::Tangent,
               reach[0], reach[1], distance);
        return LC_IntersectionStatus::Ok;
    }

    /**
     * The signed distance of the second branch's point at @p s from the first
     * branch, found by Newton steps from @p t within the segment of leaf @p p:
     * positive on the first branch's left.
     */
    bool sideOf(const Leaf& p, double t, const std::size_t segment, const double s, double& side) const {
        const LC_ParametricSegment& sp = m_segments[p.segment];
        RS_Vector c;
        RS_Vector dc;
        if (!std::isfinite(t) || !std::isfinite(s) || !m_curves.evaluate(segment, s, c, dc) || !isFinite(c) ||
            !isFinite(dc)) {
            return false;
        }
        RS_Vector a;
        RS_Vector da;
        for (int iteration = 0; iteration < 30; ++iteration) {
            if (!m_curves.evaluate(p.segment, t, a, da) || !isFinite(a) || !isFinite(da)) {
                return false;
            }
            const double squaredSpeed = da.squared();
            if (!(squaredSpeed > 0.0) || !std::isfinite(squaredSpeed)) {
                return false;
            }
            const double trial = t + RS_Vector::dotP(c - a, da) / squaredSpeed;
            if (!std::isfinite(trial)) {
                return false;
            }
            const double next = std::clamp(trial, sp.t0, sp.t1);
            if (next == t) {
                break;
            }
            t = next;
        }
        if (!m_curves.evaluate(p.segment, t, a, da) || !isFinite(a) || !isFinite(da)) {
            return false;
        }
        const double speed = da.magnitude();
        if (!(speed > 0.0) || !std::isfinite(speed)) {
            return false;
        }
        side = (da.x * (c.y - a.y) - da.y * (c.x - a.x)) / speed;
        return std::isfinite(side);
    }

    /** The start of a branch's first segment, or the end of its last. */
    bool atBranchEnd(const std::size_t segment, const double t) const {
        const LC_ParametricSegment& s = m_segments[segment];
        const bool firstOfBranch = segment == 0 || m_segments[segment - 1].branch != s.branch;
        const bool lastOfBranch = segment + 1 == m_segments.size() || m_segments[segment + 1].branch != s.branch;
        return (firstOfBranch && t == s.t0) || (lastOfBranch && t == s.t1);
    }

    /** A free end of a branch: its start with no branch running into it, or its end running into none. */
    bool atFreeEnd(const std::size_t segment, const RS_Vector& point) const {
        const LC_ParametricSegment& s = m_segments[segment];
        const bool firstOfBranch = segment == 0 || m_segments[segment - 1].branch != s.branch;
        const bool lastOfBranch = segment + 1 == m_segments.size() || m_segments[segment + 1].branch != s.branch;
        RS_Vector end;
        RS_Vector derivative;
        if (firstOfBranch && !hasPredecessor(s.branch) && m_curves.evaluate(segment, s.t0, end, derivative) &&
            end.distanceTo(point) <= m_options.tolerance) {
            return true;
        }
        return lastOfBranch && m_curves.next(s.branch) < 0 && m_curves.evaluate(segment, s.t1, end, derivative) &&
               end.distanceTo(point) <= m_options.tolerance;
    }

    /**
     * A report, with the parameter range [reachLo, reachHi] of q's branch over
     * which a tangent contact stays within the tolerance, and the distance
     * between the branches there.
     */
    void record(const Leaf& p, const double t, const Leaf& q, const double u, const RS_Vector& point,
                LC_ParametricIntersection::Kind kind, const double reachLo, const double reachHi,
                const double distance) {
        if (atFreeEnd(p.segment, point) || atFreeEnd(q.segment, point)) {
            kind = LC_ParametricIntersection::Kind::Endpoint;
        }
        Found f{{p.branch, t, q.branch, u, point, kind}, q.branch, reachLo, reachHi, distance};
        LC_ParametricIntersection& x = f.x;
        if (std::make_pair(x.branchB, x.parameterB) < std::make_pair(x.branchA, x.parameterA)) {
            std::swap(x.branchA, x.branchB);
            std::swap(x.parameterA, x.parameterB);
        }
        x.point.z = 0.0;
        m_found.push_back(f);
    }

    void record(const Leaf& p, const double t, const Leaf& q, const double u, const RS_Vector& point,
                const LC_ParametricIntersection::Kind kind) {
        record(p, t, q, u, point, kind, u, u, 0.0);
    }

    /**
     * One entry per incidence: reports of the same pair of branches whose
     * points lie within the tolerance and whose parameters on each branch
     * agree to within what that distance allows are one, found from several
     * boxes or leaves. Distinct parameter occurrences at one point stay apart.
     */
    std::vector<LC_ParametricIntersection> deduplicated() const {
        std::vector<Found> sorted = m_found;
        const auto key = [](const Found& f) {
            return std::make_tuple(f.x.branchA, f.x.parameterA, f.x.branchB, f.x.parameterB);
        };
        std::sort(sorted.begin(), sorted.end(), [&key](const Found& l, const Found& r) { return key(l) < key(r); });
        const auto rank = [](const LC_ParametricIntersection::Kind kind) {
            return kind == LC_ParametricIntersection::Kind::Endpoint     ? 2
                   : kind == LC_ParametricIntersection::Kind::Transverse ? 1
                                                                         : 0;
        };
        std::vector<LC_ParametricIntersection> result;
        std::vector<bool> merged(sorted.size(), false);
        for (std::size_t i = 0; i < sorted.size(); ++i) {
            if (merged[i]) {
                continue;
            }
            Found x = sorted[i];
            for (bool grew = true; grew;) {
                grew = false;
                for (std::size_t j = i + 1; j < sorted.size(); ++j) {
                    const Found& y = sorted[j];
                    if (merged[j] || y.x.branchA != x.x.branchA || y.x.branchB != x.x.branchB) {
                        continue;
                    }
                    // one tangent contact, reached from several boxes
                    const bool sameContact = x.x.kind == LC_ParametricIntersection::Kind::Tangent &&
                                             y.x.kind == LC_ParametricIntersection::Kind::Tangent &&
                                             x.reachBranch == y.reachBranch && x.reachLo <= y.reachHi &&
                                             y.reachLo <= x.reachHi;
                    const bool samePoint =
                        y.x.point.distanceTo(x.x.point) <= m_options.tolerance && sameOccurrence(x.x, y.x);
                    if (!sameContact && !samePoint) {
                        continue;
                    }
                    merged[j] = true;
                    grew = true;
                    const LC_ParametricIntersection::Kind kind =
                        rank(y.x.kind) > rank(x.x.kind) ? y.x.kind : x.x.kind;
                    if (sameContact) {
                        // the nearest approach stands for the contact
                        if (y.distance < x.distance) {
                            x.x = y.x;
                            x.distance = y.distance;
                        }
                        x.reachLo = std::min(x.reachLo, y.reachLo);
                        x.reachHi = std::max(x.reachHi, y.reachHi);
                    }
                    x.x.kind = kind;
                }
            }
            result.push_back(x.x);
        }
        return result;
    }

    /** Whether each branch passes from one report's parameter to the other's within the tolerance. */
    bool sameOccurrence(const LC_ParametricIntersection& x, const LC_ParametricIntersection& y) const {
        const auto close = [this](const std::size_t branch, const double s, const double t) {
            if (s == t) {
                return true;
            }
            // the branch between the two parameters stays within the tolerance of both
            const double lo = std::min(s, t);
            const double hi = std::max(s, t);
            for (std::size_t k = 0; k < m_segments.size(); ++k) {
                const LC_ParametricSegment& seg = m_segments[k];
                if (seg.branch != branch || hi < seg.t0 || lo > seg.t1) {
                    continue;
                }
                Bounds b;
                if (!bound(k, std::max(lo, seg.t0), std::min(hi, seg.t1), b) ||
                    std::hypot(b.x.width(), b.y.width()) > 2.0 * m_options.tolerance) {
                    return false;
                }
            }
            return true;
        };
        return close(x.branchA, x.parameterA, y.parameterA) && close(x.branchB, x.parameterB, y.parameterB);
    }

    const LC_ParametricCurves& m_curves;
    const LC_IntersectionOptions& m_options;
    const std::vector<LC_ParametricSegment>& m_segments;
    std::vector<Leaf> m_leaves;
    std::vector<std::size_t> m_branchLeaves;
    std::vector<Join> m_joins;
    struct Corner {
        std::size_t before;
        std::size_t after;
        RS_Vector point;
    };
    std::vector<Corner> m_corners;
    struct Found {
        LC_ParametricIntersection x;
        std::size_t reachBranch;
        double reachLo;
        double reachHi;
        double distance;
    };
    std::vector<Found> m_found;
    LC_IntersectionWorkStats m_work;
};

/** Proof-only policy is separate so ordinary tolerance-based behavior is unchanged. */
class StrictSolver {
public:
    StrictSolver(const LC_ParametricCurves& curves, const LC_IntersectionOptions& options)
        : m_curves{curves}, m_options{options}, m_segments{curves.segments()} {
    }

    LC_IntersectionResult run() {
        if (validate() && prepare()) {
            std::vector<std::pair<std::size_t, std::size_t>> candidates;
            if (candidatePairs(candidates)) {
                for (const auto& [i, j] : candidates) {
                    const auto& a = m_leaves[i];
                    const auto& b = m_leaves[j];
                    if (!solve(domain(a.segment, a.parameter, b.segment, b.parameter), i, j)) {
                        break;
                    }
                }
            }
            if (good()) {
                order();
            }
        }
        LC_IntersectionResult result;
        result.status = m_status;
        result.work = m_work;
        result.work.generatedLeaves = m_leaves.size();
        if (good()) {
            result.intersections = std::move(m_events);
        }
        return result;
    }

private:
    enum class LeafProof { IntervalProjection, AnalyticInjectivity };
    struct Piece {
        std::size_t segment;
        LC_Interval parameter;
        Bounds bounds;
        LeafProof proof;
    };
    struct Cell {
        LC_IntersectionDomain box;
        unsigned depth;
    };
    struct Inclusion {
        enum class Kind { Undecided, Excluded, Unique };
        Kind kind{Kind::Undecided};
        LC_IntersectionDomain proofDomain;
        LC_Interval a;
        LC_Interval b;
    };

    bool good() const { return m_status == LC_IntersectionStatus::Ok; }
    bool fail(const LC_IntersectionStatus status) {
        m_status = status;
        return false;
    }
    static bool finite(const LC_Interval& value) {
        return value.isValid() && std::isfinite(value.lo()) && std::isfinite(value.hi());
    }
    static bool includes(const LC_Interval& outer, const LC_Interval& inner) {
        return finite(outer) && finite(inner) && outer.lo() <= inner.lo() && inner.hi() <= outer.hi();
    }
    static bool overlaps(const LC_Interval& a, const LC_Interval& b) {
        return a.lo() <= b.hi() && b.lo() <= a.hi();
    }
    static bool samePoint(const LC_Interval& a, const LC_Interval& b) {
        return a.lo() == a.hi() && b.lo() == b.hi() && a.lo() == b.lo();
    }
    static double centre(const LC_Interval& value) {
        const double width = value.hi() - value.lo();
        const double result = std::isfinite(width) ? value.lo() + 0.5 * width :
            0.5 * value.lo() + 0.5 * value.hi();
        return std::clamp(result, value.lo(), value.hi());
    }
    static LC_IntersectionDomain domain(const std::size_t a, const LC_Interval& ta,
                                        const std::size_t b, const LC_Interval& tb) {
        return {a, b, ta, tb};
    }
    bool generate(const std::size_t count) {
        if (count > m_options.maxGeneratedItems - m_work.generatedItems) {
            return fail(LC_IntersectionStatus::LimitExceeded);
        }
        m_work.generatedItems += count;
        return true;
    }
    bool consume(std::size_t& counter) {
        if (m_work.totalBoxPairs >= m_options.maxBoxPairs) {
            return fail(LC_IntersectionStatus::LimitExceeded);
        }
        ++counter;
        ++m_work.totalBoxPairs;
        return true;
    }
    bool validate() {
        const std::size_t branches = m_curves.branchCount();
        if (!(m_options.tolerance > 0.0) || !std::isfinite(m_options.tolerance) ||
            m_options.maxGeneratedItems == 0 || branches == 0 || m_segments.empty()) {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        // Bound branch/segment-dependent work before any preparation allocation.
        if (branches > m_options.maxGeneratedItems || m_segments.size() > m_options.maxGeneratedItems) {
            return fail(LC_IntersectionStatus::LimitExceeded);
        }
        for (std::size_t i = 0; i < m_segments.size(); ++i) {
            const auto& segment = m_segments[i];
            if (segment.branch >= branches || !std::isfinite(segment.t0) || !std::isfinite(segment.t1) ||
                !(segment.t0 < segment.t1) || (i == 0 && segment.branch != 0)) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
            if (i != 0) {
                const auto& previous = m_segments[i - 1];
                if ((segment.branch == previous.branch && segment.t0 != previous.t1) ||
                    (segment.branch != previous.branch && segment.branch != previous.branch + 1)) {
                    return fail(LC_IntersectionStatus::InvalidInput);
                }
            }
            if (i == 0 || segment.branch != m_segments[i - 1].branch) {
                const auto next = m_curves.next(segment.branch);
                if (next < -1 || (next >= 0 && static_cast<std::size_t>(next) >= branches)) {
                    return fail(LC_IntersectionStatus::InvalidInput);
                }
            }
        }
        return m_segments.back().branch == branches - 1 || fail(LC_IntersectionStatus::InvalidInput);
    }
    bool bound(const std::size_t segment, const LC_Interval& parameter, Bounds& out) {
        out = {};
        if (!m_curves.bound(segment, parameter.lo(), parameter.hi(), out.x, out.y, out.dx, out.dy) ||
            !finite(out.x) || !finite(out.y) || !finite(out.dx) || !finite(out.dy)) {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        return true;
    }
    static bool projection(const Bounds& bounds) {
        const double x = centre(bounds.dx);
        const double y = centre(bounds.dy);
        const double scale = std::max(std::abs(x), std::abs(y));
        if (!(scale > 0.0) || !std::isfinite(scale)) {
            return false;
        }
        // The direction is an arbitrary exact floating vector; positivity is proved outward.
        const LC_Interval derivative = LC_Interval::point(x / scale) * bounds.dx +
                                       LC_Interval::point(y / scale) * bounds.dy;
        return derivative.isPositive();
    }
    static bool disjoint(const Bounds& a, const Bounds& b) {
        return a.x.hi() < b.x.lo() || b.x.hi() < a.x.lo() || a.y.hi() < b.y.lo() || b.y.hi() < a.y.lo();
    }
    bool prepare() {
        for (std::size_t segment = 0; segment < m_segments.size(); ++segment) {
            struct Pending { LC_Interval parameter; unsigned depth; };
            const auto& source = m_segments[segment];
            if (!generate(1)) {
                return false;
            }
            std::vector<Pending> pending{{LC_Interval::hull(source.t0, source.t1), 0}};
            for (const double t : {source.t0, centre(LC_Interval::hull(source.t0, source.t1)), source.t1}) {
                Bounds point;
                RS_Vector p;
                RS_Vector d;
                if (!bound(segment, LC_Interval::point(t), point)) {
                    return false;
                }
                if (!m_curves.evaluate(segment, t, p, d) || !isFinite(p) || !isFinite(d) || p.z != 0.0 || d.z != 0.0) {
                    return fail(LC_IntersectionStatus::InvalidInput);
                }
            }
            while (!pending.empty()) {
                const Pending item = pending.back();
                pending.pop_back();
                Bounds bounds;
                if (!bound(segment, item.parameter, bounds)) {
                    return false;
                }
                bool injective = projection(bounds);
                LeafProof proof = LeafProof::IntervalProjection;
                if (!injective) {
                    const auto analytic = m_curves.certifyInjective(segment, item.parameter);
                    if (analytic.available) {
                        if (!consume(m_work.certificateBoxPairs)) {
                            return false;
                        }
                        if (analytic.segment != segment || !includes(analytic.parameter, item.parameter) ||
                            !includes(LC_Interval::hull(source.t0, source.t1), analytic.parameter)) {
                            return fail(LC_IntersectionStatus::InvalidInput);
                        }
                        if (!generate(1)) {
                            return false;
                        }
                        injective = true;
                        proof = LeafProof::AnalyticInjectivity;
                    }
                }
                if (injective) {
                    if (m_leaves.size() >= m_options.maxLeaves) {
                        return fail(LC_IntersectionStatus::LimitExceeded);
                    }
                    m_leaves.push_back({segment, item.parameter, bounds, proof});
                    continue;
                }
                if (item.depth >= m_options.maxLeafDepth) {
                    return fail(LC_IntersectionStatus::LimitExceeded);
                }
                const double middle = centre(item.parameter);
                if (!(item.parameter.lo() < middle && middle < item.parameter.hi())) {
                    return fail(LC_IntersectionStatus::AmbiguousTopology);
                }
                if (!generate(2)) {
                    return false;
                }
                pending.push_back({LC_Interval::hull(middle, item.parameter.hi()), item.depth + 1});
                pending.push_back({LC_Interval::hull(item.parameter.lo(), middle), item.depth + 1});
            }
        }
        return true;
    }

    static Bounds united(const Bounds& a, const Bounds& b) {
        return {LC_Interval::hull(a.x, b.x), LC_Interval::hull(a.y, b.y),
                LC_Interval::hull(a.dx, b.dx), LC_Interval::hull(a.dy, b.dy)};
    }
    Bounds range(const std::size_t first, const std::size_t last) const {
        Bounds all = m_leaves[first].bounds;
        std::size_t left = m_leaves.size() + first;
        std::size_t right = m_leaves.size() + last;
        while (left < right) {
            if ((left & 1u) != 0) {
                all = united(all, m_parameterBounds[left++]);
            }
            if ((right & 1u) != 0) {
                all = united(all, m_parameterBounds[--right]);
            }
            left /= 2;
            right /= 2;
        }
        return all;
    }
    bool candidatePairs(std::vector<std::pair<std::size_t, std::size_t>>& pairs) {
        const std::size_t count = m_leaves.size();
        if (count < 2) {
            return true;
        }
        struct Node {
            Bounds bounds;
            std::size_t count;
            std::size_t first;
            std::size_t last;
            std::size_t branch;
            std::size_t leaf;
            std::size_t left{0};
            std::size_t right{0};
            bool single{true};
            bool anchored{false};
        };
        std::vector<Node> nodes;
        std::vector<std::size_t> order;
        if (count > (nodes.max_size() - 1) / 2 || count > m_parameterBounds.max_size() / 2 ||
            count > order.max_size() || count > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
            return fail(LC_IntersectionStatus::LimitExceeded);
        }
        nodes.reserve(2 * count - 1);
        order.resize(count);
        m_parameterBounds.resize(2 * count);
        for (std::size_t i = 0; i < count; ++i) {
            order[i] = i;
            m_parameterBounds[count + i] = m_leaves[i].bounds;
        }
        for (std::size_t i = count - 1; i != 0; --i) {
            m_parameterBounds[i] = united(m_parameterBounds[2 * i], m_parameterBounds[2 * i + 1]);
        }
        const auto build = [&](auto&& self, const std::size_t first, const std::size_t last) -> std::size_t {
            const auto& leaf = m_leaves[order[first]];
            Node node{leaf.bounds, last - first, order[first], order[first],
                      m_segments[leaf.segment].branch, order[first]};
            double low[2] = {centre(leaf.bounds.x), centre(leaf.bounds.y)};
            double high[2] = {low[0], low[1]};
            for (std::size_t i = first; i < last; ++i) {
                const std::size_t index = order[i];
                const auto& piece = m_leaves[index];
                const auto branch = m_segments[piece.segment].branch;
                node.bounds = united(node.bounds, piece.bounds);
                node.first = std::min(node.first, index);
                node.last = std::max(node.last, index);
                node.single = node.single && branch == node.branch;
                node.anchored = node.anchored || branch < m_options.anchoredBranches;
                const double key[] = {centre(piece.bounds.x), centre(piece.bounds.y)};
                for (int axis = 0; axis < 2; ++axis) {
                    low[axis] = std::min(low[axis], key[axis]);
                    high[axis] = std::max(high[axis], key[axis]);
                }
            }
            const std::size_t index = nodes.size();
            nodes.push_back(node);
            if (node.count > 1) {
                const int axis = 0.5 * high[1] - 0.5 * low[1] > 0.5 * high[0] - 0.5 * low[0] ? 1 : 0;
                const std::size_t half = first + node.count / 2;
                std::nth_element(order.begin() + first, order.begin() + half, order.begin() + last,
                    [&](const auto a, const auto b) {
                        const auto& aa = m_leaves[a].bounds;
                        const auto& bb = m_leaves[b].bounds;
                        return std::make_pair(centre(axis == 0 ? aa.x : aa.y), a) <
                               std::make_pair(centre(axis == 0 ? bb.x : bb.y), b);
                    });
                nodes[index].left = self(self, first, half);
                nodes[index].right = self(self, half, last);
            }
            return index;
        };
        const std::size_t root = build(build, 0, count);
        std::array<std::pair<std::size_t, std::size_t>, 2 * std::numeric_limits<std::size_t>::digits + 1> pending;
        std::size_t used = 0;
        const auto push = [&](const std::size_t a, const std::size_t b) {
            if (used == pending.size()) {
                return fail(LC_IntersectionStatus::LimitExceeded);
            }
            pending[used++] = {a, b};
            return true;
        };
        push(root, root);
        while (used != 0) {
            const auto [a, b] = pending[--used];
            const auto& p = nodes[a];
            const auto& q = nodes[b];
            if (a == b) {
                if (!p.anchored || p.count == 1) {
                    continue;
                }
                if (!push(p.right, p.right) || !push(p.left, p.right) || !push(p.left, p.left)) {
                    return false;
                }
                continue;
            }
            if (!consume(m_work.candidateBoxPairs)) {
                return false;
            }
            if ((!p.anchored && !q.anchored) || disjoint(p.bounds, q.bounds)) {
                continue;
            }
            if (p.single && q.single && p.branch == q.branch &&
                projection(range(std::min(p.first, q.first), std::max(p.last, q.last) + 1))) {
                continue;
            }
            if (p.count == 1 && q.count == 1) {
                pairs.emplace_back(std::min(p.leaf, q.leaf), std::max(p.leaf, q.leaf));
            } else if (q.count == 1 || (p.count > 1 && p.count >= q.count)) {
                if (!push(p.right, b) || !push(p.left, b)) {
                    return false;
                }
            } else if (!push(a, q.right) || !push(a, q.left)) {
                return false;
            }
        }
        std::sort(pairs.begin(), pairs.end());
        return true;
    }

    bool pathProjection(const LC_IntersectionDomain& box, const std::size_t first, const std::size_t last,
                        bool& proved) {
        proved = false;
        if (m_segments[box.segmentA].branch != m_segments[box.segmentB].branch) {
            return true;
        }
        Bounds start;
        Bounds end;
        // Include the tails and every intervening leaf, not just the two refined cells.
        if (!bound(box.segmentA, LC_Interval::hull(box.parameterA.lo(), m_leaves[first].parameter.hi()), start) ||
            !bound(box.segmentB, LC_Interval::hull(m_leaves[last].parameter.lo(), box.parameterB.hi()), end)) {
            return false;
        }
        Bounds all = united(start, end);
        if (first + 1 < last) {
            all = united(all, range(first + 1, last));
        }
        proved = projection(all);
        return true;
    }
    bool validDomain(const LC_IntersectionDomain& box) const {
        return box.segmentA < m_segments.size() && box.segmentB < m_segments.size() &&
            includes(LC_Interval::hull(m_segments[box.segmentA].t0, m_segments[box.segmentA].t1), box.parameterA) &&
            includes(LC_Interval::hull(m_segments[box.segmentB].t0, m_segments[box.segmentB].t1), box.parameterB);
    }
    static bool covers(const LC_IntersectionDomain& outer, const LC_IntersectionDomain& inner) {
        return outer.segmentA == inner.segmentA && outer.segmentB == inner.segmentB &&
            includes(outer.parameterA, inner.parameterA) && includes(outer.parameterB, inner.parameterB);
    }
    bool analytic(const LC_IntersectionDomain& box, LC_AnalyticIntersectionCertificate& certificate) {
        certificate = m_curves.certifyIntersection(box);
        using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
        if (certificate.outcome == Outcome::Unavailable) {
            return true;
        }
        if (!consume(m_work.certificateBoxPairs)) {
            return false;
        }
        if (!validDomain(certificate.proofDomain) || !covers(certificate.proofDomain, box)) {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        if (certificate.outcome == Outcome::Excluded) {
            if (certificate.proof != LC_IntersectionProof::AnalyticExclusion) {
                return fail(LC_IntersectionStatus::AmbiguousTopology);
            }
        } else if (certificate.outcome == Outcome::Unique) {
            if (certificate.proof != LC_IntersectionProof::AnalyticEndpoint &&
                certificate.proof != LC_IntersectionProof::AnalyticCusp &&
                certificate.proof != LC_IntersectionProof::AnalyticTangency) {
                return fail(LC_IntersectionStatus::AmbiguousTopology);
            }
            if (!includes(certificate.proofDomain.parameterA, certificate.isolationA) ||
                !includes(certificate.proofDomain.parameterB, certificate.isolationB)) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
            using Kind = LC_ParametricIntersection::Kind;
            if (certificate.kind != Kind::Transverse && certificate.kind != Kind::Tangent && certificate.kind != Kind::Endpoint) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
        } else {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        return true;
    }
    bool krawczyk(const LC_IntersectionDomain& requested, const bool inflate, Inclusion& result) {
        result = {};
        result.proofDomain = requested;
        if (inflate) {
            const auto enlarged = [](const LC_Interval& input, const LC_ParametricSegment& segment) {
                const double margin = 0.001 * input.hi() - 0.001 * input.lo();
                return LC_Interval::hull(std::max(segment.t0, input.lo() - margin),
                                         std::min(segment.t1, input.hi() + margin));
            };
            result.proofDomain.parameterA = enlarged(requested.parameterA, m_segments[requested.segmentA]);
            result.proofDomain.parameterB = enlarged(requested.parameterB, m_segments[requested.segmentB]);
        }
        const auto& box = result.proofDomain;
        Bounds a;
        Bounds b;
        Bounds pointA;
        Bounds pointB;
        const double t = centre(box.parameterA);
        const double u = centre(box.parameterB);
        if (!bound(box.segmentA, box.parameterA, a) || !bound(box.segmentB, box.parameterB, b) ||
            !bound(box.segmentA, LC_Interval::point(t), pointA) ||
            !bound(box.segmentB, LC_Interval::point(u), pointB)) {
            return false;
        }
        RS_Vector pa;
        RS_Vector da;
        RS_Vector pb;
        RS_Vector db;
        if (!m_curves.evaluate(box.segmentA, t, pa, da) || !m_curves.evaluate(box.segmentB, u, pb, db) ||
            !isFinite(pa) || !isFinite(pb) || !isFinite(da) || !isFinite(db) ||
            pa.z != 0.0 || pb.z != 0.0 || da.z != 0.0 || db.z != 0.0) {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        const double sa = std::max(std::abs(da.x), std::abs(da.y));
        const double sb = std::max(std::abs(db.x), std::abs(db.y));
        if (!(sa > 0.0) || !(sb > 0.0)) {
            return true;
        }
        const double ax = da.x / sa;
        const double ay = da.y / sa;
        const double bx = db.x / sb;
        const double by = db.y / sb;
        const double det = -ax * by + bx * ay;
        if (det == 0.0 || !std::isfinite(det)) {
            return true;
        }
        const double r00 = (-by / det) / sa;
        const double r01 = (bx / det) / sa;
        const double r10 = (-ay / det) / sb;
        const double r11 = (ax / det) / sb;
        if (!std::isfinite(r00) || !std::isfinite(r01) || !std::isfinite(r10) || !std::isfinite(r11)) {
            return true;
        }
        const auto pt = [](const double value) { return LC_Interval::point(value); };
        const LC_Interval fx = pointA.x - pointB.x;
        const LC_Interval fy = pointA.y - pointB.y;
        const LC_Interval m00 = pt(1.0) - (pt(r00) * a.dx + pt(r01) * a.dy);
        const LC_Interval m01 = pt(r00) * b.dx + pt(r01) * b.dy;
        const LC_Interval m10 = -(pt(r10) * a.dx + pt(r11) * a.dy);
        const LC_Interval m11 = pt(1.0) + pt(r10) * b.dx + pt(r11) * b.dy;
        const LC_Interval dt = box.parameterA - pt(t);
        const LC_Interval du = box.parameterB - pt(u);
        const LC_Interval kt = pt(t) - (pt(r00) * fx + pt(r01) * fy) + m00 * dt + m01 * du;
        const LC_Interval ku = pt(u) - (pt(r10) * fx + pt(r11) * fy) + m10 * dt + m11 * du;
        if (!finite(kt) || !finite(ku)) {
            return true;
        }
        result.a = intersect(kt, box.parameterA);
        result.b = intersect(ku, box.parameterB);
        if (!result.a.isValid() || !result.b.isValid()) {
            result.kind = Inclusion::Kind::Excluded;
        } else if (kt.lo() > box.parameterA.lo() && kt.hi() < box.parameterA.hi() &&
                   ku.lo() > box.parameterB.lo() && ku.hi() < box.parameterB.hi()) {
            // Full-dimensional strict inclusion proves uniqueness and regularity:
            // Rump, Acta Numerica 2010, Theorem 13.3. R consists of exact finite doubles.
            result.kind = Inclusion::Kind::Unique;
        }
        return true;
    }
    bool makeEvent(const LC_IntersectionDomain& proof, const LC_Interval& a, const LC_Interval& b,
                   const LC_IntersectionProof category, LC_ParametricIntersection& out,
                   const LC_ParametricIntersection::Kind kind = LC_ParametricIntersection::Kind::Transverse) {
        if (proof.segmentA == proof.segmentB && overlaps(a, b)) {
            return fail(LC_IntersectionStatus::AmbiguousTopology);
        }
        Bounds pa;
        Bounds pb;
        if (!bound(proof.segmentA, a, pa) || !bound(proof.segmentB, b, pb)) {
            return false;
        }
        const LC_Interval x = intersect(pa.x, pb.x);
        const LC_Interval y = intersect(pa.y, pb.y);
        if (!finite(x) || !finite(y)) {
            return fail(LC_IntersectionStatus::InvalidInput);
        }
        if (!generate(1)) {
            return false;
        }
        out = {};
        out.branchA = m_segments[proof.segmentA].branch;
        out.branchB = m_segments[proof.segmentB].branch;
        out.parameterA = centre(a);
        out.parameterB = centre(b);
        out.point = RS_Vector{centre(x), centre(y)};
        out.proof = category;
        out.kind = kind;
        out.isolation = domain(proof.segmentA, a, proof.segmentB, b);
        out.proofDomain = proof;
        if (std::make_pair(out.branchB, out.parameterB) < std::make_pair(out.branchA, out.parameterA)) {
            std::swap(out.branchA, out.branchB);
            std::swap(out.parameterA, out.parameterB);
            std::swap(out.isolation.segmentA, out.isolation.segmentB);
            std::swap(out.isolation.parameterA, out.isolation.parameterB);
            std::swap(out.proofDomain.segmentA, out.proofDomain.segmentB);
            std::swap(out.proofDomain.parameterA, out.proofDomain.parameterB);
        }
        return true;
    }
    bool solve(const LC_IntersectionDomain& initial, const std::size_t first, const std::size_t last) {
        std::vector<Cell> pending{{initial, 0}};
        while (!pending.empty()) {
            if (!consume(m_work.refinementBoxPairs)) {
                return false;
            }
            const Cell cell = pending.back();
            pending.pop_back();
            Bounds a;
            Bounds b;
            if (!bound(cell.box.segmentA, cell.box.parameterA, a) ||
                !bound(cell.box.segmentB, cell.box.parameterB, b)) {
                return false;
            }
            if (disjoint(a, b)) {
                continue;
            }
            bool injectivePath = false;
            if (!pathProjection(cell.box, first, last, injectivePath)) {
                return false;
            }
            if (injectivePath) {
                continue;
            }
            LC_AnalyticIntersectionCertificate certificate;
            if (!analytic(cell.box, certificate)) {
                return false;
            }
            using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
            if (certificate.outcome == Outcome::Excluded) {
                if (!generate(1)) {
                    return false;
                }
                continue;
            }
            LC_ParametricIntersection event;
            if (certificate.outcome == Outcome::Unique) {
                if (!makeEvent(certificate.proofDomain, certificate.isolationA, certificate.isolationB,
                               certificate.proof, event, certificate.kind)) {
                    return false;
                }
                m_events.push_back(event);
                continue;
            }
            Inclusion inclusion;
            if (!krawczyk(cell.box, true, inclusion)) {
                return false;
            }
            if (inclusion.kind == Inclusion::Kind::Excluded) {
                continue;
            }
            if (inclusion.kind == Inclusion::Kind::Unique) {
                if (!overlaps(inclusion.a, cell.box.parameterA) || !overlaps(inclusion.b, cell.box.parameterB)) {
                    continue; // the sole root of the inflated proof domain is outside this original cell
                }
                if (!makeEvent(inclusion.proofDomain, inclusion.a, inclusion.b,
                               LC_IntersectionProof::IntervalUniqueTransverse, event)) {
                    return false;
                }
                m_events.push_back(event);
                continue;
            }
            if (cell.depth >= m_options.maxDepth) {
                return fail(LC_IntersectionStatus::LimitExceeded);
            }
            const bool first = std::hypot(a.x.width(), a.y.width()) >= std::hypot(b.x.width(), b.y.width());
            const LC_Interval parameter = first ? cell.box.parameterA : cell.box.parameterB;
            const double middle = centre(parameter);
            if (!(parameter.lo() < middle && middle < parameter.hi())) {
                return fail(LC_IntersectionStatus::AmbiguousTopology);
            }
            Cell left{cell.box, cell.depth + 1};
            Cell right{cell.box, cell.depth + 1};
            (first ? left.box.parameterA : left.box.parameterB) = LC_Interval::hull(parameter.lo(), middle);
            (first ? right.box.parameterA : right.box.parameterB) = LC_Interval::hull(middle, parameter.hi());
            pending.push_back(right);
            pending.push_back(left);
        }
        return true;
    }

    bool shrink(LC_ParametricIntersection& event, bool& changed) {
        changed = false;
        if (!consume(m_work.certificateBoxPairs)) {
            return false;
        }
        Inclusion inclusion;
        if (!krawczyk(event.isolation, false, inclusion)) {
            return false;
        }
        if (inclusion.kind == Inclusion::Kind::Excluded) {
            return fail(LC_IntersectionStatus::InvalidInput); // contradicts the existing existence proof
        }
        LC_Interval a = inclusion.a;
        LC_Interval b = inclusion.b;
        if (!finite(a) || !finite(b)) {
            LC_AnalyticIntersectionCertificate certificate;
            if (!analytic(event.isolation, certificate)) {
                return false;
            }
            using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
            if (certificate.outcome == Outcome::Excluded) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
            if (certificate.outcome == Outcome::Unavailable) {
                return true;
            }
            a = intersect(event.isolation.parameterA, certificate.isolationA);
            b = intersect(event.isolation.parameterB, certificate.isolationB);
            if (!finite(a) || !finite(b)) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
        }
        if (a.lo() == event.isolation.parameterA.lo() && a.hi() == event.isolation.parameterA.hi() &&
            b.lo() == event.isolation.parameterB.lo() && b.hi() == event.isolation.parameterB.hi()) {
            return true;
        }
        LC_ParametricIntersection narrowed;
        if (!makeEvent(event.proofDomain, a, b, event.proof, narrowed, event.kind)) {
            return false;
        }
        event = narrowed;
        changed = true;
        return true;
    }

    /** Compare every shared occurrence, not just events with the same partner branch. */
    static bool orderedOccurrences(const LC_ParametricIntersection& a, const LC_ParametricIntersection& b) {
        const std::pair<std::size_t, LC_Interval> aa[] = {
            {a.branchA, a.isolation.parameterA}, {a.branchB, a.isolation.parameterB}};
        const std::pair<std::size_t, LC_Interval> bb[] = {
            {b.branchA, b.isolation.parameterA}, {b.branchB, b.isolation.parameterB}};
        for (const auto& x : aa) {
            for (const auto& y : bb) {
                if (x.first == y.first && overlaps(x.second, y.second) && !samePoint(x.second, y.second)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool merge(LC_ParametricIntersection& a, const LC_ParametricIntersection& b, bool& merged) {
        merged = false;
        if (a.branchA != b.branchA || a.branchB != b.branchB ||
            !overlaps(a.isolation.parameterA, b.isolation.parameterA) ||
            !overlaps(a.isolation.parameterB, b.isolation.parameterB)) {
            return true;
        }
        LC_IntersectionDomain proof;
        LC_IntersectionProof category;
        LC_ParametricIntersection::Kind kind;
        if (covers(a.proofDomain, b.isolation) ||
            (samePoint(a.isolation.parameterA, b.isolation.parameterA) &&
             samePoint(a.isolation.parameterB, b.isolation.parameterB))) {
            proof = a.proofDomain;
            category = a.proof;
            kind = a.kind;
        } else if (covers(b.proofDomain, a.isolation)) {
            proof = b.proofDomain;
            category = b.proof;
            kind = b.kind;
        } else if (a.isolation.segmentA == b.isolation.segmentA && a.isolation.segmentB == b.isolation.segmentB) {
            if (!consume(m_work.certificateBoxPairs)) {
                return false;
            }
            const LC_IntersectionDomain common = domain(a.isolation.segmentA,
                LC_Interval::hull(a.isolation.parameterA, b.isolation.parameterA), a.isolation.segmentB,
                LC_Interval::hull(a.isolation.parameterB, b.isolation.parameterB));
            LC_AnalyticIntersectionCertificate certificate;
            if (!analytic(common, certificate)) {
                return false;
            }
            using Outcome = LC_AnalyticIntersectionCertificate::Outcome;
            if (certificate.outcome == Outcome::Excluded) {
                return fail(LC_IntersectionStatus::InvalidInput);
            }
            if (certificate.outcome == Outcome::Unique) {
                proof = certificate.proofDomain;
                category = certificate.proof;
                kind = certificate.kind;
            } else {
                Inclusion inclusion;
                if (!krawczyk(common, true, inclusion)) {
                    return false;
                }
                if (inclusion.kind == Inclusion::Kind::Excluded) {
                    return fail(LC_IntersectionStatus::InvalidInput);
                }
                if (inclusion.kind != Inclusion::Kind::Unique) {
                    return true;
                }
                proof = inclusion.proofDomain;
                category = LC_IntersectionProof::IntervalUniqueTransverse;
                kind = LC_ParametricIntersection::Kind::Transverse;
            }
        } else {
            return true;
        }
        const LC_Interval ta = intersect(a.isolation.parameterA, b.isolation.parameterA);
        const LC_Interval tb = intersect(a.isolation.parameterB, b.isolation.parameterB);
        LC_ParametricIntersection combined;
        if (!makeEvent(proof, ta, tb, category, combined, kind)) {
            return false;
        }
        a = combined;
        merged = true;
        return true;
    }

    bool order() {
        std::vector<bool> removed(m_events.size(), false);
        for (std::size_t i = 0; i < m_events.size(); ++i) {
            if (removed[i]) {
                continue;
            }
            for (std::size_t j = i + 1; j < m_events.size(); ++j) {
                if (removed[j]) {
                    continue;
                }
                for (unsigned attempt = 0;; ++attempt) {
                    if (!consume(m_work.certificateBoxPairs)) {
                        return false;
                    }
                    bool merged = false;
                    if (!merge(m_events[i], m_events[j], merged)) {
                        return false;
                    }
                    if (merged) {
                        removed[j] = true;
                        break;
                    }
                    if (orderedOccurrences(m_events[i], m_events[j])) {
                        break;
                    }
                    if (attempt >= m_options.maxDepth) {
                        return fail(LC_IntersectionStatus::LimitExceeded);
                    }
                    bool changedA = false;
                    bool changedB = false;
                    if (!shrink(m_events[i], changedA) || !shrink(m_events[j], changedB)) {
                        return false;
                    }
                    if (!changedA && !changedB) {
                        return fail(LC_IntersectionStatus::AmbiguousTopology);
                    }
                }
            }
        }
        std::size_t count = 0;
        for (std::size_t i = 0; i < m_events.size(); ++i) {
            if (!removed[i]) {
                m_events[count++] = m_events[i];
            }
        }
        m_events.resize(count);
        std::sort(m_events.begin(), m_events.end(), [](const auto& a, const auto& b) {
            return std::make_tuple(a.branchA, a.isolation.parameterA.lo(), a.branchB, a.isolation.parameterB.lo(),
                                   a.isolation.segmentA, a.isolation.segmentB) <
                   std::make_tuple(b.branchA, b.isolation.parameterA.lo(), b.branchB, b.isolation.parameterB.lo(),
                                   b.isolation.segmentA, b.isolation.segmentB);
        });
        return true;
    }

    const LC_ParametricCurves& m_curves;
    const LC_IntersectionOptions& m_options;
    const std::vector<LC_ParametricSegment>& m_segments;
    LC_IntersectionStatus m_status{LC_IntersectionStatus::Ok};
    LC_IntersectionWorkStats m_work;
    std::vector<Piece> m_leaves;
    std::vector<Bounds> m_parameterBounds;
    std::vector<LC_ParametricIntersection> m_events;
};

} // namespace

LC_IntersectionResult findIntersections(const LC_ParametricCurves& curves, const LC_IntersectionOptions& options) {
    return options.strict ? StrictSolver{curves, options}.run() : Solver{curves, options}.run();
}
