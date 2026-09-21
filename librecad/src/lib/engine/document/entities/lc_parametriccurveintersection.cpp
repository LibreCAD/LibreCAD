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

struct Leaf {
    std::size_t segment;
    std::size_t branch;
    /** Position among the leaves of its branch. */
    std::size_t order;
    double a;
    double b;
    bool monotone;
    LC_Interval x;
    LC_Interval y;
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
            for (const auto& [i, j] : candidatePairs()) {
                result.status = solvePair(m_leaves[i], m_leaves[j]);
                if (result.status != LC_IntersectionStatus::Ok) {
                    break;
                }
            }
        }
        if (result.status == LC_IntersectionStatus::Ok) {
            result.intersections = deduplicated();
        }
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
                    m_leaves.push_back(
                        Leaf{s, branch, m_branchLeaves[branch]++, box.a, box.b, monotone, bounds.x, bounds.y});
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

    bool hasPredecessor(const std::size_t branch) const {
        return std::any_of(m_joins.begin(), m_joins.end(), [branch](const Join& j) { return j.to == branch; });
    }

    /** Consecutive leaves of a branch, or across a join, share an end and nothing else. */
    bool adjacent(const Leaf& p, const Leaf& q) const {
        if (p.branch == q.branch && (p.order + 1 == q.order || q.order + 1 == p.order)) {
            return true;
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

    /**
     * Pairs of leaves whose bounds come within the tolerance, other than a
     * leaf with itself or its neighbours, from a sweep over x; in a fixed order.
     */
    std::vector<std::pair<std::size_t, std::size_t>> candidatePairs() const {
        std::vector<std::size_t> byX(m_leaves.size());
        for (std::size_t i = 0; i < byX.size(); ++i) {
            byX[i] = i;
        }
        std::sort(byX.begin(), byX.end(), [this](const std::size_t i, const std::size_t j) {
            return std::make_pair(m_leaves[i].x.lo(), i) < std::make_pair(m_leaves[j].x.lo(), j);
        });
        const double reach = m_options.tolerance;
        std::vector<std::pair<std::size_t, std::size_t>> pairs;
        for (std::size_t k = 0; k < byX.size(); ++k) {
            const Leaf& p = m_leaves[byX[k]];
            for (std::size_t n = k + 1; n < byX.size(); ++n) {
                const Leaf& q = m_leaves[byX[n]];
                if (q.x.lo() > p.x.hi() + reach) {
                    break;
                }
                if (gap(p.x, p.y, q.x, q.y) <= reach && !adjacent(p, q)) {
                    pairs.emplace_back(std::min(byX[k], byX[n]), std::max(byX[k], byX[n]));
                }
            }
        }
        std::sort(pairs.begin(), pairs.end());
        return pairs;
    }

    /** Both boxes lie within twice the tolerance of a join between their branches. */
    bool insideJoin(const Leaf& p, const Bounds& bp, const Leaf& q, const Bounds& bq) const {
        const double radius = 2.0 * m_options.tolerance;
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
        std::vector<Pair> stack{{p.a, p.b, q.a, q.b, 0}};
        while (!stack.empty()) {
            const Pair box = stack.back();
            stack.pop_back();
            if (++m_boxPairs > m_options.maxBoxPairs) {
                return LC_IntersectionStatus::LimitExceeded;
            }
            Bounds bp;
            Bounds bq;
            if (!bound(p.segment, box.a0, box.a1, bp) || !bound(q.segment, box.b0, box.b1, bq)) {
                return LC_IntersectionStatus::AmbiguousTopology;
            }
            if (gap(bp.x, bp.y, bq.x, bq.y) > 0.0 || insideJoin(p, bp, q, bq) || bridged(p, q, box)) {
                continue;
            }
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
                const LC_IntersectionStatus status = classifyContact(p, q, box);
                if (status != LC_IntersectionStatus::Ok) {
                    return status;
                }
                continue;
            }
            // split the side whose image is larger
            const double sizeP = std::hypot(bp.x.width(), bp.y.width());
            const double sizeQ = std::hypot(bq.x.width(), bq.y.width());
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
     * Two boxes of one segment with the piece between them monotone, or within
     * the tolerance: nothing there crosses, beyond a loop too small to resolve.
     */
    bool bridged(const Leaf& p, const Leaf& q, const Pair& box) const {
        if (p.segment != q.segment) {
            return false;
        }
        Bounds b;
        if (!bound(p.segment, std::min(box.a0, box.b0), std::max(box.a1, box.b1), b)) {
            return false;
        }
        return isMonotone(b.dx, b.dy) || std::hypot(b.x.width(), b.y.width()) <= m_options.tolerance;
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
     * branches do not come that close.
     */
    LC_IntersectionStatus classifyContact(const Leaf& p, const Leaf& q, const Pair& box) {
        const LC_ParametricSegment& sq = m_segments[q.segment];
        double t = 0.5 * (box.a0 + box.a1);
        double u = 0.5 * (box.b0 + box.b1);
        RS_Vector a;
        RS_Vector da;
        RS_Vector b;
        RS_Vector db;
        // Gauss-Newton towards the nearest pair within the two leaves
        for (int iteration = 0; iteration < 40; ++iteration) {
            if (!m_curves.evaluate(p.segment, t, a, da) || !m_curves.evaluate(q.segment, u, b, db)) {
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
            const double nextT = std::clamp(t - (h11 * g0 - h01 * g1) / det, p.a, p.b);
            const double nextU = std::clamp(u - (h00 * g1 - h01 * g0) / det, q.a, q.b);
            if (nextT == t && nextU == u) {
                break;
            }
            t = nextT;
            u = nextU;
        }
        if (!m_curves.evaluate(p.segment, t, a, da) || !m_curves.evaluate(q.segment, u, b, db)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        const double tolerance = m_options.tolerance;
        if (a.distanceTo(b) > tolerance || !(da.magnitude() > 0.0) || !(db.magnitude() > 0.0)) {
            return LC_IntersectionStatus::AmbiguousTopology;
        }
        // Which side of the first branch the second is on, on either side of
        // the contact: stepping away along it until they are more than the
        // tolerance apart. Staying that close over the whole leaf is retracing.
        int sides[2] = {0, 0};
        // how far along the second branch the contact reaches, on each side
        double reach[2] = {q.a, q.b};
        for (int k = 0; k < 2; ++k) {
            const double direction = (k == 0) ? -1.0 : 1.0;
            for (double step = 8.0 * tolerance / db.magnitude();; step *= 2.0) {
                const double s = u + direction * step;
                if (s < q.a || s > q.b) {
                    break;
                }
                reach[k] = s;
                double side = 0.0;
                if (!sideOf(p, t, q.segment, s, side)) {
                    return LC_IntersectionStatus::AmbiguousTopology;
                }
                if (std::abs(side) > tolerance) {
                    sides[k] = side > 0.0 ? 1 : -1;
                    break;
                }
            }
        }
        if (sides[0] == 0 && sides[1] == 0) {
            return LC_IntersectionStatus::AmbiguousTopology; // retraced
        }
        const bool crosses = sides[0] != 0 && sides[1] != 0 && sides[0] != sides[1];
        record(p, t, q, u, (a + b) * 0.5,
               crosses ? LC_ParametricIntersection::Kind::Transverse : LC_ParametricIntersection::Kind::Tangent,
               reach[0], reach[1], a.distanceTo(b));
        return LC_IntersectionStatus::Ok;
    }

    /**
     * The signed distance of the second branch's point at @p s from the first
     * branch, found by Newton steps from @p t within leaf @p p: positive on the
     * first branch's left.
     */
    bool sideOf(const Leaf& p, double t, const std::size_t segment, const double s, double& side) const {
        RS_Vector c;
        RS_Vector dc;
        if (!m_curves.evaluate(segment, s, c, dc)) {
            return false;
        }
        RS_Vector a;
        RS_Vector da;
        for (int iteration = 0; iteration < 30; ++iteration) {
            if (!m_curves.evaluate(p.segment, t, a, da) || !(da.squared() > 0.0)) {
                return false;
            }
            const double next = std::clamp(t + RS_Vector::dotP(c - a, da) / da.squared(), p.a, p.b);
            if (next == t) {
                break;
            }
            t = next;
        }
        if (!m_curves.evaluate(p.segment, t, a, da) || !(da.magnitude() > 0.0)) {
            return false;
        }
        side = (da.x * (c.y - a.y) - da.y * (c.x - a.x)) / da.magnitude();
        return std::isfinite(side);
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
    struct Found {
        LC_ParametricIntersection x;
        std::size_t reachBranch;
        double reachLo;
        double reachHi;
        double distance;
    };
    std::vector<Found> m_found;
    std::size_t m_boxPairs{0};
};

} // namespace

LC_IntersectionResult findIntersections(const LC_ParametricCurves& curves, const LC_IntersectionOptions& options) {
    return Solver{curves, options}.run();
}
