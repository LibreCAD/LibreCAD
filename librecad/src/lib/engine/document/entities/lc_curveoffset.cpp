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

#include "lc_curveoffset.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "lc_curvejet.h"
#include "lc_splinepoints.h"
#include "rs_line.h"
#include "rs_spline.h"

namespace {

constexpr double g_eps = std::numeric_limits<double>::epsilon();

bool isFinite(const RS_Vector& v) {
    return v.valid && std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

double cross(const RS_Vector& a, const RS_Vector& b) {
    return a.x * b.y - a.y * b.x;
}

double dot(const RS_Vector& a, const RS_Vector& b) {
    return a.x * b.x + a.y * b.y;
}

/** The angle between two directions, in [0, pi]. */
double angleBetweenVectors(const RS_Vector& a, const RS_Vector& b) {
    return std::atan2(std::abs(cross(a, b)), dot(a, b));
}

// ---------------------------------------------------------------------------
// Source adapters: the one interface the engine reads a curve through. An
// adapter lives only for one request and holds the entity by reference; no
// result keeps it.
// ---------------------------------------------------------------------------
class OffsetSource {
public:
    virtual ~OffsetSource() = default;
    virtual bool closed() const = 0;
    /** The domain is [front, back]; the curve is smooth between consecutive values. */
    virtual const std::vector<double>& breaks() const = 0;
    virtual bool jet(double t, LC_CurveEvaluationSide side, LC_CurveJet& out) const = 0;
    virtual bool boundJet(double a, double b, LC_CurveJetBounds& out) const = 0;
    /** Points whose convex hull contains the curve. */
    virtual const std::vector<RS_Vector>& hull() const = 0;
    /** True, with its ends, when the curve is exactly one straight segment. */
    virtual bool straightSegment(RS_Vector& start, RS_Vector& end) const = 0;
};

class SplineSource final : public OffsetSource {
public:
    explicit SplineSource(const RS_Spline& spline)
        : m_spline{spline},
          m_breaks{spline.getBreakParameters()},
          m_hull{spline.getData().controlPoints} {
    }

    bool closed() const override {
        return m_spline.isClosed();
    }

    const std::vector<double>& breaks() const override {
        return m_breaks;
    }

    bool jet(const double t, const LC_CurveEvaluationSide side, LC_CurveJet& out) const override {
        return m_spline.tryEvaluateJet(t, side, out);
    }

    bool boundJet(const double a, const double b, LC_CurveJetBounds& out) const override {
        return m_spline.tryBoundJet(a, b, out);
    }

    const std::vector<RS_Vector>& hull() const override {
        return m_hull;
    }

    bool straightSegment(RS_Vector& start, RS_Vector& end) const override {
        if (m_spline.getDegree() != 1 || m_spline.isClosed() || m_spline.getData().controlPoints.size() != 2) {
            return false;
        }
        start = m_spline.getData().controlPoints[0];
        end = m_spline.getData().controlPoints[1];
        return true;
    }

private:
    const RS_Spline& m_spline;
    std::vector<double> m_breaks;
    std::vector<RS_Vector> m_hull;
};

class SplinePointsSource final : public OffsetSource {
public:
    explicit SplinePointsSource(const LC_SplinePoints& spline)
        : m_spline{spline},
          m_hull{spline.getControlPoints()} {
        const size_t count = spline.getSegmentCount();
        if (count > 0) {
            for (size_t i = 0; i <= count; ++i) {
                m_breaks.push_back(static_cast<double>(i));
            }
        }
    }

    bool closed() const override {
        return m_spline.isClosed();
    }

    const std::vector<double>& breaks() const override {
        return m_breaks;
    }

    bool jet(const double t, const LC_CurveEvaluationSide side, LC_CurveJet& out) const override {
        return m_spline.tryEvaluateJet(t, side, out);
    }

    bool boundJet(const double a, const double b, LC_CurveJetBounds& out) const override {
        return m_spline.tryBoundJet(a, b, out);
    }

    const std::vector<RS_Vector>& hull() const override {
        return m_hull;
    }

    bool straightSegment(RS_Vector& start, RS_Vector& end) const override {
        LC_SplinePointsSegment segment;
        if (m_spline.getSegmentCount() != 1 || !m_spline.tryGetSegment(0, segment) ||
            segment.kind != LC_SplinePointsSegment::Kind::Line) {
            return false;
        }
        start = segment.start;
        end = segment.end;
        return true;
    }

private:
    const LC_SplinePoints& m_spline;
    std::vector<double> m_breaks;
    std::vector<RS_Vector> m_hull;
};

std::unique_ptr<OffsetSource> makeSource(const RS_Entity& entity) {
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
        return std::make_unique<SplineSource>(*spline);
    }
    if (const auto* points = dynamic_cast<const LC_SplinePoints*>(&entity)) {
        return std::make_unique<SplinePointsSource>(*points);
    }
    return nullptr;
}

/** World-space scales the tolerances are derived from (plan 5.8). */
struct SourceScale {
    double feature{0.0};
    double coordinate{0.0};
    double numericFloor{0.0};
};

bool computeScale(const OffsetSource& source, const double distanceMagnitude, SourceScale& scale) {
    const std::vector<RS_Vector>& hull = source.hull();
    if (hull.empty() || !std::isfinite(distanceMagnitude)) {
        return false;
    }
    RS_Vector lo = hull.front();
    RS_Vector hi = hull.front();
    double coordinate = 0.0;
    for (const RS_Vector& v : hull) {
        if (!isFinite(v)) {
            return false;
        }
        lo = RS_Vector::minimum(lo, v);
        hi = RS_Vector::maximum(hi, v);
        coordinate = std::max({coordinate, std::abs(v.x), std::abs(v.y)});
    }
    const double diagonal = std::hypot(hi.x - lo.x, hi.y - lo.y);
    scale.feature = std::max(diagonal, std::abs(distanceMagnitude));
    scale.coordinate = std::max(coordinate, scale.feature);
    scale.numericFloor = 64.0 * g_eps * std::max(scale.coordinate, std::numeric_limits<double>::min());
    return std::isfinite(scale.feature) && std::isfinite(scale.coordinate) && scale.feature > 0.0;
}

bool isPlanar(const OffsetSource& source, const double numericFloor) {
    const std::vector<RS_Vector>& hull = source.hull();
    return std::all_of(hull.begin(), hull.end(),
                       [numericFloor](const RS_Vector& v) { return std::abs(v.z) <= numericFloor; });
}

bool isPositiveFinite(const double v) {
    return std::isfinite(v) && v > 0.0;
}

bool validDirectOptions(const LC_CurveOffsetOptions& o) {
    const LC_CurveOffsetTolerances& t = o.tolerance;
    return (o.mode == LC_CurveOffsetMode::Direct || o.mode == LC_CurveOffsetMode::Trimmed ||
            o.mode == LC_CurveOffsetMode::RegionBoundary) &&
           isPositiveFinite(t.requestedGeometry) &&
           isPositiveFinite(t.evaluation) && isPositiveFinite(t.fit) && isPositiveFinite(t.nodeMerge) &&
           isPositiveFinite(t.parameter) && isPositiveFinite(t.rootResidual) &&
           isPositiveFinite(t.classification) && isPositiveFinite(o.angleTolerance) && o.angleTolerance < M_PI &&
           o.maxSubdivisionDepth > 0 && o.maxSamples > 0 && o.maxOutputBranches > 0 && o.maxIntersectionPairs > 0 &&
           o.maxDistanceMapBoxes > 0 && o.maxArrangementEdges > 0 && o.maxArrangementFaces > 0;
}

// ---------------------------------------------------------------------------
// Side selection (plan 5.3)
// ---------------------------------------------------------------------------
struct NearestCandidate {
    double t;
    double distance;
};

/** Safeguarded Newton on g(t) = (C(t) - P) . C'(t) inside [lo, hi]. */
double refineNearest(const OffsetSource& source, const RS_Vector& p, double t, double lo, double hi,
                     const double parameterTolerance) {
    for (int iteration = 0; iteration < 60; ++iteration) {
        LC_CurveJet jet;
        if (!source.jet(t, LC_CurveEvaluationSide::Interior, jet)) {
            break;
        }
        const RS_Vector r = jet.point - p;
        const double g = dot(r, jet.first);
        const double gp = dot(jet.first, jet.first) + dot(r, jet.second);
        // keep the bracket around the minimum: g < 0 before it, g > 0 after it
        if (g < 0.0) {
            lo = t;
        }
        else if (g > 0.0) {
            hi = t;
        }
        else {
            return t;
        }
        double next = (gp > 0.0) ? t - g / gp : std::numeric_limits<double>::quiet_NaN();
        if (!(next > lo && next < hi)) {
            next = lo + 0.5 * (hi - lo);
        }
        if (std::abs(next - t) <= parameterTolerance) {
            return next;
        }
        t = next;
    }
    return t;
}

LC_OffsetSideResolution resolveSideImpl(const OffsetSource& source, const RS_Vector& p,
                                        const LC_CurveOffsetTolerances& tolerance) {
    LC_OffsetSideResolution result;
    result.status = LC_CurveOffsetStatus::AmbiguousSide;
    const std::vector<double>& breaks = source.breaks();
    if (breaks.size() < 2 || !isFinite(p)) {
        result.status = LC_CurveOffsetStatus::InvalidSource;
        return result;
    }

    // Seed every span at its ends and interior samples, keep the local minima of
    // the distance, and refine each one.
    constexpr int samplesPerSpan = 32;
    std::vector<double> ts;
    std::vector<double> ds;
    for (size_t i = 0; i + 1 < breaks.size(); ++i) {
        for (int k = 0; k < samplesPerSpan; ++k) {
            ts.push_back(breaks[i] + (breaks[i + 1] - breaks[i]) * k / samplesPerSpan);
        }
    }
    ts.push_back(breaks.back());
    for (const double t : ts) {
        LC_CurveJet jet;
        if (!source.jet(t, LC_CurveEvaluationSide::Interior, jet)) {
            result.status = LC_CurveOffsetStatus::InvalidSource;
            return result;
        }
        ds.push_back(jet.point.distanceTo(p));
    }

    const bool closed = source.closed();
    const size_t n = ts.size();
    std::vector<NearestCandidate> candidates;
    for (size_t k = 0; k < n; ++k) {
        // a closed curve's first and last samples are the same point: its neighbours wrap
        const bool first = (k == 0);
        const bool last = (k + 1 == n);
        const double before = first ? (closed ? ds[n - 2] : RS_MAXDOUBLE) : ds[k - 1];
        const double after = last ? (closed ? ds[1] : RS_MAXDOUBLE) : ds[k + 1];
        if (ds[k] > before || ds[k] > after || (closed && last)) {
            continue;
        }
        const double lo = first ? ts.front() : ts[k - 1];
        const double hi = last ? ts.back() : ts[k + 1];
        const double t = refineNearest(source, p, ts[k], lo, hi, tolerance.parameter);
        LC_CurveJet jet;
        if (source.jet(t, LC_CurveEvaluationSide::Interior, jet)) {
            candidates.push_back({t, jet.point.distanceTo(p)});
        }
        // a minimum at an open end need not have g = 0: keep the end itself too
        if ((first || last) && !closed) {
            candidates.push_back({ts[k], ds[k]});
        }
    }
    if (candidates.empty()) {
        return result;
    }
    double best = RS_MAXDOUBLE;
    for (const NearestCandidate& c : candidates) {
        best = std::min(best, c.distance);
    }
    if (best <= tolerance.classification) {
        return result; // the point is on the curve: no side
    }

    // Every occurrence as near as the nearest must agree on the side.
    int sign = 0;
    std::sort(candidates.begin(), candidates.end(),
              [](const NearestCandidate& a, const NearestCandidate& b) { return a.t < b.t; });
    for (const NearestCandidate& c : candidates) {
        if (c.distance - best > tolerance.classification) {
            continue;
        }
        if (!result.occurrences.empty() && std::abs(c.t - result.occurrences.back()) <= tolerance.parameter) {
            continue;
        }
        LC_CurveJet jet;
        if (!source.jet(c.t, LC_CurveEvaluationSide::Interior, jet) ||
            dot(jet.first, jet.first) <= 0.0) {
            return result;
        }
        const RS_Vector toPoint = p - jet.point;
        const double side = cross(jet.first, toPoint);
        // the side test is as uncertain as the point's position, scaled by the tangent
        if (std::abs(side) <= tolerance.classification * jet.first.magnitude()) {
            return result;
        }
        const int s = side > 0.0 ? 1 : -1;
        if (sign != 0 && s != sign) {
            return result;
        }
        sign = s;
        result.occurrences.push_back(c.t);
    }
    if (sign == 0) {
        return result;
    }
    result.status = LC_CurveOffsetStatus::Ok;
    result.side = sign > 0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right;
    return result;
}

// ---------------------------------------------------------------------------
// Adaptive Hermite fitting of one branch (plan 5.4, 5.6, 5.9)
// ---------------------------------------------------------------------------
struct OffsetJet {
    RS_Vector point;
    RS_Vector first;
};

/** The exact offset Q = C + d N and its derivative at t (plan 5.4). */
LC_CurveOffsetStatus computeOffsetJet(const OffsetSource& source, const double d, const double speedFloor,
                                      const double t, const LC_CurveEvaluationSide side, OffsetJet& out) {
    LC_CurveJet c;
    if (!source.jet(t, side, c)) {
        return LC_CurveOffsetStatus::InvalidSource;
    }
    const RS_Vector& v = c.first;
    const RS_Vector& a = c.second;
    const double s2 = dot(v, v);
    const double s = std::sqrt(s2);
    if (!(s > speedFloor)) {
        return LC_CurveOffsetStatus::UndefinedTangent;
    }
    // N = (-vy, vx) / s and its derivative
    const RS_Vector n{-v.y / s, v.x / s};
    const double va = dot(v, a);
    const double s3 = s * s2;
    const RS_Vector dn{-a.y / s + v.y * va / s3, a.x / s - v.x * va / s3};
    out.point = c.point + n * d;
    out.first = v + dn * d;
    out.point.z = 0.0;
    out.first.z = 0.0;
    if (!isFinite(out.point) || !isFinite(out.first)) {
        return LC_CurveOffsetStatus::FitFailed;
    }
    return LC_CurveOffsetStatus::Ok;
}

/** The Hermite cubic through a with derivative da and b with derivative db over a parameter width h. */
std::array<RS_Vector, 4> hermitePiece(const RS_Vector& a, const RS_Vector& da, const RS_Vector& b,
                                      const RS_Vector& db, const double h) {
    return {a, a + da * (h / 3.0), b - db * (h / 3.0), b};
}

/** The cubic Bezier arc of the circle about c of radius r from angle a0 to a1 (at most a quarter turn). */
std::array<RS_Vector, 4> arcPiece(const RS_Vector& c, const double r, const double a0, const double a1) {
    const double k = 4.0 / 3.0 * std::tan((a1 - a0) / 4.0) * r;
    const RS_Vector p0 = c + RS_Vector{std::cos(a0), std::sin(a0)} * r;
    const RS_Vector p3 = c + RS_Vector{std::cos(a1), std::sin(a1)} * r;
    return {p0, p0 + RS_Vector{-std::sin(a0), std::cos(a0)} * k, p3 - RS_Vector{-std::sin(a1), std::cos(a1)} * k, p3};
}

class BranchBuilder {
public:
    BranchBuilder(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                  const LC_OffsetSourceBudget& budget, const double speedFloor)
        : m_source{source},
          m_d{d},
          m_options{options},
          m_budget{budget},
          m_speedFloor{speedFloor} {
    }

    /**
     * The offset as branches in source order. A cusp, inside a span or at a
     * join marked in @p joinCusps (entry 0 for the seam of a closed source),
     * ends one branch and starts the next at the same point. A closed source
     * gives one closed branch without cusps; with them, the branch across its
     * seam runs from the last cusp to the first.
     */
    LC_CurveOffsetStatus build(const std::vector<bool>& joinCusps, std::vector<LC_OffsetBranch>& branches) {
        const std::vector<double>& breaks = m_source.breaks();
        std::vector<double> cusps;
        LC_CurveOffsetStatus status = isolateCusps(joinCusps, cusps);
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
        // a cusp that fell exactly on a break acts as a join cusp there
        std::vector<bool> breakCusps = joinCusps;
        for (size_t i = 0; i < breaks.size(); ++i) {
            if (std::binary_search(cusps.begin(), cusps.end(), breaks[i])) {
                breakCusps[(i + 1 == breaks.size()) ? 0 : i] = true;
            }
        }
        branches.assign(1, LC_OffsetBranch{});
        auto next = cusps.begin();
        for (size_t span = 0; span + 1 < breaks.size(); ++span) {
            const double a = breaks[span];
            const double b = breaks[span + 1];
            if (span > 0 && breakCusps[span]) {
                branches.emplace_back();
            }
            double from = a;
            for (; next != cusps.end() && *next < b; ++next) {
                if (*next <= a) {
                    continue;
                }
                status = fitSpan(span, from, *next, branches.back());
                if (status != LC_CurveOffsetStatus::Ok) {
                    return status;
                }
                branches.emplace_back();
                from = *next;
            }
            status = fitSpan(span, from, b, branches.back());
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
        }
        for (size_t i = 0; i < branches.size(); ++i) {
            if (branches[i].cubicPieces.empty()) {
                return LC_CurveOffsetStatus::InvalidSource;
            }
            if (i > 0) {
                // branches meet at their cusp exactly; across a join its two
                // evaluations differ by rounding
                const RS_Vector& end = branches[i - 1].cubicPieces.back().bezier[3];
                RS_Vector& start = branches[i].cubicPieces.front().bezier[0];
                if (start.distanceTo(end) > m_options.tolerance.nodeMerge) {
                    return LC_CurveOffsetStatus::DiscontinuousNormal;
                }
                start = end;
            }
        }
        if (m_source.closed()) {
            auto& last = branches.back().cubicPieces.back().bezier[3];
            const RS_Vector& first = branches.front().cubicPieces.front().bezier[0];
            if (last.distanceTo(first) > m_options.tolerance.nodeMerge) {
                return LC_CurveOffsetStatus::DiscontinuousNormal;
            }
            last = first;
            if (!breakCusps[0]) {
                if (branches.size() == 1) {
                    branches.front().closed = true;
                }
                else {
                    // the last branch continues across the seam into the first
                    std::vector<LC_OffsetCubicPiece>& wrapped = branches.back().cubicPieces;
                    wrapped.insert(wrapped.end(), branches.front().cubicPieces.begin(),
                                   branches.front().cubicPieces.end());
                    branches.front().cubicPieces = std::move(wrapped);
                    branches.pop_back();
                }
            }
        }
        return LC_CurveOffsetStatus::Ok;
    }

    double maxObservedError() const {
        return m_maxError;
    }

    std::size_t samples() const {
        return m_samples;
    }

private:
    struct Leaf {
        double t0;
        double t1;
        unsigned depth;
    };

    LC_CurveOffsetStatus offsetJet(const double t, const LC_CurveEvaluationSide side, OffsetJet& out) {
        if (++m_samples > m_options.maxSamples) {
            return LC_CurveOffsetStatus::LimitExceeded;
        }
        return computeOffsetJet(m_source, m_d, m_speedFloor, t, side, out);
    }

    /**
     * |C'|^3 - d (C' x C'') at t, which has the sign of 1 - d kappa where the
     * tangent exists and, unlike it, stays continuous where it does not.
     * NaN on failure.
     */
    double factorNumeratorAt(const double t, const LC_CurveEvaluationSide side) const {
        LC_CurveJet c;
        if (!m_source.jet(t, side, c)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        const double s2 = dot(c.first, c.first);
        return s2 * std::sqrt(s2) - m_d * cross(c.first, c.second);
    }

    double speedAt(const double t, const LC_CurveEvaluationSide side) const {
        LC_CurveJet c;
        return m_source.jet(t, side, c) ? c.first.magnitude() : 0.0;
    }

    /** Boxes too small to split in which regularity could not be proved, consecutive in parameter. */
    struct Run {
        double t0;
        double t1;
        /** A bound on how far the offset moves over the run. */
        double extent;
        unsigned boxes;
        /** The span of the run's last box, and that box, for a failure report. */
        size_t span;
        Leaf last;
    };

    /**
     * The cusps of the offset away from the joins, in source order. Every span
     * is proved box by box to have a nonvanishing tangent and 1 - d kappa away
     * from zero, except for runs of boxes too small to split. A run continues
     * across a join where 1 - d kappa keeps its sign, and across the seam of a
     * closed source, since a root next to either leaves unproved boxes on both
     * sides. A run across which 1 - d kappa changes sign, and over which the
     * offset moves less than the fit tolerance, holds one cusp, found by
     * bisection. Any other run is a singularity the offset is not split at: a
     * vanishing tangent, a zero that touches without crossing, or roots too
     * close to tell apart.
     */
    LC_CurveOffsetStatus isolateCusps(const std::vector<bool>& joinCusps, std::vector<double>& cusps) {
        constexpr unsigned maxRunBoxes = 64;
        const std::vector<double>& breaks = m_source.breaks();
        std::vector<Run> runs;
        for (size_t span = 0; span + 1 < breaks.size(); ++span) {
            const double a = breaks[span];
            const double b = breaks[span + 1];
            std::vector<Leaf> stack{{a, b, 0}};
            while (!stack.empty()) {
                const Leaf leaf = stack.back();
                stack.pop_back();
                LC_CurveJetBounds bounds;
                const bool bounded = m_source.boundJet(leaf.t0, leaf.t1, bounds);
                const bool speedProved = bounded && bounds.speedSquared().isPositive();
                const LC_Interval g = bounded ? bounds.offsetFactorNumerator(m_d) : LC_Interval{};
                if (speedProved && g.excludesZero()) {
                    continue;
                }
                const double mid = leaf.t0 + 0.5 * (leaf.t1 - leaf.t0);
                if (leaf.depth < m_options.maxSubdivisionDepth && mid > leaf.t0 && mid < leaf.t1) {
                    split(stack, leaf, 0.5);
                    continue;
                }
                if (!speedProved) {
                    return irregularity(leaf, false, a, b);
                }
                // the offset moves at |Q'| = |1 - d kappa| |C'| = |g| / |C'|^2
                const double extent =
                    std::max(std::abs(g.lo()), std::abs(g.hi())) / bounds.speedSquared().lo() * (leaf.t1 - leaf.t0);
                const bool continues = !runs.empty() && runs.back().t1 == leaf.t0 && !(leaf.t0 == a && joinCusps[span]);
                if (continues) {
                    Run& run = runs.back();
                    run.t1 = leaf.t1;
                    run.extent += extent;
                    ++run.boxes;
                    run.span = span;
                    run.last = leaf;
                }
                else {
                    runs.push_back(Run{leaf.t0, leaf.t1, extent, 1, span, leaf});
                }
                const Run& run = runs.back();
                if (run.boxes > maxRunBoxes || !(run.extent <= m_options.tolerance.fit)) {
                    return irregularity(leaf, true, a, b);
                }
            }
        }
        // a run across the seam: from the last one, at the end, into the first
        bool wraps = false;
        if (m_source.closed() && !joinCusps[0] && runs.size() > 1 && runs.front().t0 == breaks.front() &&
            runs.back().t1 == breaks.back()) {
            wraps = true;
            if (runs.front().boxes + runs.back().boxes > maxRunBoxes ||
                !(runs.front().extent + runs.back().extent <= m_options.tolerance.fit)) {
                const Run& run = runs.back();
                return irregularity(run.last, true, breaks[run.span], breaks[run.span + 1]);
            }
        }
        for (size_t i = wraps ? 1 : 0; i < runs.size(); ++i) {
            const Run& run = runs[i];
            if (wraps && i + 1 == runs.size()) {
                // from the start of the last run to the end of the first
                const LC_CurveOffsetStatus status =
                    bisectCusp(run.t0, breaks.back(), runs.front().t0, runs.front().t1, run, cusps);
                if (status != LC_CurveOffsetStatus::Ok) {
                    return status;
                }
                continue;
            }
            const LC_CurveOffsetStatus status = bisectCusp(run.t0, run.t1, run.t1, run.t1, run, cusps);
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
        }
        std::sort(cusps.begin(), cusps.end());
        return LC_CurveOffsetStatus::Ok;
    }

    /**
     * The root of 1 - d kappa in the run [t0, t1], continued over [u0, u1]
     * when the run wraps a closed source's seam (u0 is then the domain start;
     * otherwise u0 == u1 == t1). It must change sign across the run.
     */
    LC_CurveOffsetStatus bisectCusp(double lo, double hi, const double u0, const double u1, const Run& run,
                                    std::vector<double>& cusps) {
        const std::vector<double>& breaks = m_source.breaks();
        const bool wraps = u0 != u1;
        const double g0 = factorNumeratorAt(lo, LC_CurveEvaluationSide::Right);
        const double g1 = factorNumeratorAt(wraps ? u1 : hi, LC_CurveEvaluationSide::Left);
        if (!(std::isfinite(g0) && std::isfinite(g1) && g0 != 0.0 && g1 != 0.0) ||
            std::signbit(g0) == std::signbit(g1)) {
            return irregularity(run.last, true, breaks[run.span], breaks[run.span + 1]);
        }
        if (wraps) {
            // the sign changes before the seam or after it
            const double atEnd = factorNumeratorAt(hi, LC_CurveEvaluationSide::Left);
            if (!std::isfinite(atEnd) || atEnd == 0.0) {
                return LC_CurveOffsetStatus::SingularOffset;
            }
            if (std::signbit(atEnd) == std::signbit(g0)) {
                lo = u0;
                hi = u1;
            }
        }
        const double gLo = factorNumeratorAt(lo, LC_CurveEvaluationSide::Right);
        while (true) {
            const double mid = lo + 0.5 * (hi - lo);
            if (!(mid > lo && mid < hi)) {
                break;
            }
            // the right limit is the value inside a span and exists at a break too
            const double g = factorNumeratorAt(mid, LC_CurveEvaluationSide::Right);
            if (!std::isfinite(g)) {
                return LC_CurveOffsetStatus::AmbiguousRegularity;
            }
            if (g == 0.0) {
                lo = hi = mid;
                break;
            }
            (std::signbit(g) == std::signbit(gLo) ? lo : hi) = mid;
        }
        cusps.push_back(lo + 0.5 * (hi - lo));
        return LC_CurveOffsetStatus::Ok;
    }

    /**
     * Why a leaf could not be proved regular once it can no longer be split. A
     * sampled zero tangent, or a zero or sign change of 1 - d kappa, confirms a
     * singularity; otherwise the interval bounds were only too wide, as they are
     * next to a root, or at a zero that touches without changing sign. The
     * samples spread out from the leaf over its span, since the unproved leaf a
     * root leaves behind need not contain it.
     */
    LC_CurveOffsetStatus irregularity(const Leaf& leaf, const bool speedProved, const double spanStart,
                                      const double spanEnd) const {
        std::vector<double> ts{spanStart, spanEnd, leaf.t0, leaf.t1};
        const double width = leaf.t1 - leaf.t0;
        const double centre = leaf.t0 + 0.5 * width;
        for (double reach = width; reach < spanEnd - spanStart; reach *= 2.0) {
            ts.push_back(centre - reach);
            ts.push_back(centre + reach);
        }
        constexpr int grid = 64;
        for (int k = 1; k < grid; ++k) {
            ts.push_back(spanStart + (spanEnd - spanStart) * k / grid);
        }
        ts.push_back(centre);
        ts.erase(std::remove_if(ts.begin(), ts.end(), [&](const double t) { return t < spanStart || t > spanEnd; }),
                 ts.end());
        std::sort(ts.begin(), ts.end());
        ts.erase(std::unique(ts.begin(), ts.end()), ts.end());

        auto sideAt = [&](const double t) {
            return (t == spanStart) ? LC_CurveEvaluationSide::Right
                   : (t == spanEnd) ? LC_CurveEvaluationSide::Left
                                    : LC_CurveEvaluationSide::Interior;
        };
        // a vanishing source tangent is the root cause wherever it shows: next to
        // it the curvature, and so 1 - d kappa, swings through zero as well
        for (const double t : ts) {
            if (!(speedAt(t, sideAt(t)) > m_speedFloor)) {
                return LC_CurveOffsetStatus::UndefinedTangent;
            }
        }
        if (!speedProved) {
            return LC_CurveOffsetStatus::AmbiguousRegularity;
        }
        double previous = std::numeric_limits<double>::quiet_NaN();
        for (const double t : ts) {
            const double g = factorNumeratorAt(t, sideAt(t));
            if (!std::isfinite(g)) {
                return LC_CurveOffsetStatus::AmbiguousRegularity;
            }
            if (g == 0.0 || (std::isfinite(previous) && std::signbit(g) != std::signbit(previous))) {
                return LC_CurveOffsetStatus::SingularOffset;
            }
            previous = g;
        }
        return LC_CurveOffsetStatus::AmbiguousRegularity;
    }

    static RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
        const double r = 1.0 - s;
        return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
    }

    static RS_Vector bezierDerivative(const std::array<RS_Vector, 4>& b, const double s) {
        const double r = 1.0 - s;
        return ((b[1] - b[0]) * (r * r) + (b[2] - b[1]) * (2.0 * r * s) + (b[3] - b[2]) * (s * s)) * 3.0;
    }

    struct Check {
        LC_CurveOffsetStatus status{LC_CurveOffsetStatus::Ok};
        double worstError{0.0};
        /** Where the positional or angular excess is largest: the place to split. */
        double worstS{0.5};
        bool failed{false};
    };

    /**
     * Compares the piece with the exact offset at local parameters @p nodes, in
     * coordinates relative to @p origin to keep a small offset from drowning in
     * large world coordinates.
     */
    Check check(const Leaf& leaf, const std::array<RS_Vector, 4>& local, const RS_Vector& origin,
                const double* nodes, const size_t count) {
        Check result;
        const double h = leaf.t1 - leaf.t0;
        double worstExcess = 0.0;
        for (size_t i = 0; i < count; ++i) {
            const double s = nodes[i];
            OffsetJet exact;
            result.status = offsetJet(leaf.t0 + s * h, LC_CurveEvaluationSide::Interior, exact);
            if (result.status != LC_CurveOffsetStatus::Ok) {
                return result;
            }
            const double error = bezierAt(local, s).distanceTo(exact.point - origin);
            const double angle = angleBetweenVectors(bezierDerivative(local, s), exact.first);
            const double excess = std::max(error / m_options.tolerance.fit, angle / m_options.angleTolerance);
            result.worstError = std::max(result.worstError, error);
            if (excess > worstExcess) {
                worstExcess = excess;
                result.worstS = s;
            }
        }
        result.failed = worstExcess > 1.0;
        return result;
    }

    LC_CurveOffsetStatus fitSpan(const size_t span, const double a, const double b, LC_OffsetBranch& branch) {
        // Independent validation nodes: Chebyshev points plus a low-discrepancy
        // sequence seeded by the span, neither used to place the first split.
        static constexpr double splitNodes[] = {0.125, 0.25, 0.5, 0.75, 0.875};
        double validationNodes[11];
        for (int k = 1; k <= 7; ++k) {
            validationNodes[k - 1] = 0.5 * (1.0 - std::cos((2.0 * k - 1.0) * M_PI / 14.0));
        }
        const double golden = 0.6180339887498949;
        double seed = std::fmod(0.5 + golden * static_cast<double>(span + 1), 1.0);
        for (int k = 0; k < 4; ++k) {
            seed = std::fmod(seed + golden, 1.0);
            validationNodes[7 + k] = 0.02 + 0.96 * seed;
        }

        std::vector<Leaf> stack{{a, b, 0}};
        while (!stack.empty()) {
            const Leaf leaf = stack.back();
            stack.pop_back();

            OffsetJet q0;
            OffsetJet q1;
            LC_CurveOffsetStatus status = offsetJet(leaf.t0, LC_CurveEvaluationSide::Right, q0);
            if (status == LC_CurveOffsetStatus::Ok) {
                status = offsetJet(leaf.t1, LC_CurveEvaluationSide::Left, q1);
            }
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
            const double h = leaf.t1 - leaf.t0;
            const std::array<RS_Vector, 4> piece = hermitePiece(q0.point, q0.first, q1.point, q1.first, h);
            const RS_Vector origin = (q0.point + q1.point) * 0.5;
            const std::array<RS_Vector, 4> local{piece[0] - origin, piece[1] - origin, piece[2] - origin,
                                                 piece[3] - origin};

            Check fit = check(leaf, local, origin, splitNodes, std::size(splitNodes));
            if (fit.status != LC_CurveOffsetStatus::Ok) {
                return fit.status;
            }
            if (!fit.failed) {
                fit = check(leaf, local, origin, validationNodes, std::size(validationNodes));
                if (fit.status != LC_CurveOffsetStatus::Ok) {
                    return fit.status;
                }
            }
            if (fit.failed) {
                if (leaf.depth >= m_options.maxSubdivisionDepth) {
                    return LC_CurveOffsetStatus::ToleranceNotMet;
                }
                split(stack, leaf, fit.worstS);
                continue;
            }

            if (m_pieces >= m_budget.maxCubicPieces || m_pieces >= m_budget.maxOutputEntities) {
                return LC_CurveOffsetStatus::LimitExceeded;
            }
            LC_OffsetCubicPiece cubic;
            cubic.provenance = {span, leaf.t0, leaf.t1, m_d, true};
            cubic.bezier = piece;
            if (!branch.cubicPieces.empty()) {
                // neighbours share their end exactly; within a span the two
                // evaluations agree, across a join they differ by rounding
                const RS_Vector& previous = branch.cubicPieces.back().bezier[3];
                if (previous.distanceTo(cubic.bezier[0]) > m_options.tolerance.nodeMerge) {
                    return LC_CurveOffsetStatus::DiscontinuousNormal;
                }
                cubic.bezier[0] = previous;
            }
            branch.cubicPieces.push_back(cubic);
            ++m_pieces;
            m_maxError = std::max(m_maxError, fit.worstError);
        }
        return LC_CurveOffsetStatus::Ok;
    }

    /** Splits the leaf at local parameter s, kept away from its ends; the left
     *  half is taken first, so pieces come out in parameter order. */
    static void split(std::vector<Leaf>& stack, const Leaf& leaf, double s) {
        s = std::clamp(s, 0.125, 0.875);
        const double mid = leaf.t0 + s * (leaf.t1 - leaf.t0);
        stack.push_back({mid, leaf.t1, leaf.depth + 1});
        stack.push_back({leaf.t0, mid, leaf.depth + 1});
    }

    const OffsetSource& m_source;
    const double m_d;
    const LC_CurveOffsetOptions& m_options;
    const LC_OffsetSourceBudget& m_budget;
    const double m_speedFloor;
    double m_maxError{0.0};
    std::size_t m_samples{0};
    std::size_t m_pieces{0};
};

/**
 * The source's joins, and for a closed source its seam, must have a single
 * normal: the curve continuous, its tangent direction continuous within the
 * angle tolerance, and the offset gap that angle opens within the merge budget.
 * The curvature may jump at a join (quadratic segments meet only C1): where
 * 1 - d kappa changes sign across it, the offset turns back there, a cusp no
 * box inside either span can see. @p cusps marks those joins by break index,
 * and the seam of a closed source at index 0.
 */
LC_CurveOffsetStatus checkJoins(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                                std::vector<bool>& cusps) {
    const std::vector<double>& breaks = source.breaks();
    cusps.assign(breaks.size(), false);
    auto checkJoin = [&](const double left, const double right, const bool seam, const size_t index) {
        LC_CurveJet l;
        LC_CurveJet r;
        if (!source.jet(left, LC_CurveEvaluationSide::Left, l) || !source.jet(right, LC_CurveEvaluationSide::Right, r)) {
            return LC_CurveOffsetStatus::InvalidSource;
        }
        if (l.point.distanceTo(r.point) > options.tolerance.nodeMerge) {
            // only a closed flag can claim a join whose ends are apart
            return seam ? LC_CurveOffsetStatus::InvalidSource : LC_CurveOffsetStatus::DiscontinuousNormal;
        }
        if (!(dot(l.first, l.first) > 0.0) || !(dot(r.first, r.first) > 0.0)) {
            return LC_CurveOffsetStatus::UndefinedTangent;
        }
        const double angle = angleBetweenVectors(l.first, r.first);
        if (angle > options.angleTolerance || std::abs(d) * angle > options.tolerance.nodeMerge) {
            return LC_CurveOffsetStatus::DiscontinuousNormal;
        }
        auto factorNumerator = [d](const LC_CurveJet& jet) {
            const double s2 = dot(jet.first, jet.first);
            return s2 * std::sqrt(s2) - d * cross(jet.first, jet.second);
        };
        const double before = factorNumerator(l);
        const double after = factorNumerator(r);
        if (!(std::isfinite(before) && std::isfinite(after) && before != 0.0 && after != 0.0)) {
            return LC_CurveOffsetStatus::SingularOffset; // a root at the join itself
        }
        cusps[index] = std::signbit(before) != std::signbit(after);
        return LC_CurveOffsetStatus::Ok;
    };
    for (size_t i = 1; i + 1 < breaks.size(); ++i) {
        const LC_CurveOffsetStatus status = checkJoin(breaks[i], breaks[i], false, i);
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
    }
    if (source.closed()) {
        return checkJoin(breaks.back(), breaks.front(), true, 0);
    }
    return LC_CurveOffsetStatus::Ok;
}

// ---------------------------------------------------------------------------
// Noding (plan 5.10, 5.11)
// ---------------------------------------------------------------------------

/**
 * Where each branch runs on into another: itself when closed; across a cusp
 * the next one, and for a closed source the last into the first.
 */
std::vector<std::ptrdiff_t> branchSuccessors(const std::vector<LC_OffsetBranch>& branches, const bool closedSource) {
    std::vector<std::ptrdiff_t> next(branches.size(), -1);
    for (size_t b = 0; b < branches.size(); ++b) {
        if (branches[b].closed) {
            next[b] = static_cast<std::ptrdiff_t>(b);
        }
        else if (b + 1 < branches.size()) {
            next[b] = static_cast<std::ptrdiff_t>(b + 1);
        }
        else if (closedSource && branches.size() > 1) {
            next[b] = 0;
        }
    }
    return next;
}

/**
 * The exact offset of the branches, Q = C + d N with Q' = (1 - d kappa) C',
 * over the source parameter intervals of their pieces: one segment for each
 * run of pieces within a source span.
 */
class OffsetCurves final : public LC_ParametricCurves {
public:
    OffsetCurves(const OffsetSource& source, const double d, const double speedFloor,
                 const std::vector<LC_OffsetBranch>& branches)
        : m_source{source},
          m_d{d},
          m_speedFloor{speedFloor},
          m_next{branchSuccessors(branches, source.closed())} {
        for (size_t b = 0; b < branches.size(); ++b) {
            for (const LC_OffsetCubicPiece& piece : branches[b].cubicPieces) {
                const LC_OffsetBranchProvenance& p = piece.provenance;
                if (!m_segments.empty() && m_segments.back().branch == b && m_spans.back() == p.sourceSpan &&
                    m_segments.back().t1 == p.sourceT0) {
                    m_segments.back().t1 = p.sourceT1;
                    continue;
                }
                m_segments.push_back(LC_ParametricSegment{b, p.sourceT0, p.sourceT1});
                m_spans.push_back(p.sourceSpan);
            }
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_next.size();
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        return m_next[branch];
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const LC_ParametricSegment& s = m_segments[segment];
        const LC_CurveEvaluationSide side = (t == s.t0)   ? LC_CurveEvaluationSide::Right
                                            : (t == s.t1) ? LC_CurveEvaluationSide::Left
                                                          : LC_CurveEvaluationSide::Interior;
        OffsetJet q;
        if (computeOffsetJet(m_source, m_d, m_speedFloor, t, side, q) != LC_CurveOffsetStatus::Ok) {
            return false;
        }
        point = q.point;
        derivative = q.first;
        return true;
    }

    bool bound(const std::size_t, const double a, const double b, LC_Interval& x, LC_Interval& y, LC_Interval& dx,
               LC_Interval& dy) const override {
        LC_CurveJetBounds c;
        if (!m_source.boundJet(a, b, c)) {
            return false;
        }
        const LC_Interval speed2 = c.speedSquared();
        if (!speed2.isPositive()) {
            return false;
        }
        const LC_Interval speed = sqrt(speed2);
        const LC_Interval d = LC_Interval::point(m_d);
        const LC_Interval factor = c.offsetFactorNumerator(m_d) / (speed2 * speed);
        dx = factor * c.dx;
        dy = factor * c.dy;
        // C + d N bounds each term apart, which next to a cusp, where the two
        // nearly cancel, is far wider than the offset's own motion; the mean
        // value form Q(m) + Q'([a, b]) ([a, b] - m) is not. Both enclose Q.
        x = c.x - d * c.dy / speed;
        y = c.y + d * c.dx / speed;
        const double m = a + 0.5 * (b - a);
        OffsetJet q;
        if (computeOffsetJet(m_source, m_d, m_speedFloor, m, LC_CurveEvaluationSide::Interior, q) ==
            LC_CurveOffsetStatus::Ok) {
            const double margin = 64.0 * g_eps * (std::abs(q.point.x) + std::abs(q.point.y) + std::abs(m_d));
            const LC_Interval step = LC_Interval::hull(a - m, b - m);
            const LC_Interval mx = LC_Interval::hull(q.point.x - margin, q.point.x + margin) + dx * step;
            const LC_Interval my = LC_Interval::hull(q.point.y - margin, q.point.y + margin) + dy * step;
            x = narrowed(x, mx);
            y = narrowed(y, my);
        }
        return true;
    }

private:
    /** The common part of two enclosures of one value; either one if rounding left them apart. */
    static LC_Interval narrowed(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid() || a.hi() < b.lo() || b.hi() < a.lo()) {
            return a.isValid() ? a : b;
        }
        return LC_Interval::hull(std::max(a.lo(), b.lo()), std::min(a.hi(), b.hi()));
    }

public:

private:
    const OffsetSource& m_source;
    const double m_d;
    const double m_speedFloor;
    std::vector<std::ptrdiff_t> m_next;
    std::vector<LC_ParametricSegment> m_segments;
    std::vector<size_t> m_spans;
};

/**
 * Makes every intersection occurrence a shared end of the pieces of its branch:
 * a piece is refitted as two Hermite pieces meeting at the node, or an end that
 * already lies there is moved onto it. Both occurrences of a node share one
 * point.
 */
struct Occurrence {
    size_t branch;
    double t;
    RS_Vector point;
};

LC_CurveOffsetStatus insertOccurrences(const OffsetSource& source, const double d, const double speedFloor,
                                       const LC_CurveOffsetOptions& options, const std::vector<Occurrence>& occurrences,
                                       std::vector<LC_OffsetBranch>& branches) {
    auto insert = [&](const size_t b, const double t, const RS_Vector& node) {
        std::vector<LC_OffsetCubicPiece>& pieces = branches[b].cubicPieces;
        for (size_t k = 0; k < pieces.size(); ++k) {
            LC_OffsetCubicPiece& piece = pieces[k];
            const LC_OffsetBranchProvenance p = piece.provenance;
            if (t < p.sourceT0 || t > p.sourceT1) {
                continue;
            }
            // at an end of the piece, or so close that splitting would leave nothing
            const bool atStart = piece.bezier[0].distanceTo(node) <= options.tolerance.nodeMerge;
            const bool atEnd = piece.bezier[3].distanceTo(node) <= options.tolerance.nodeMerge;
            if (atStart || atEnd || !(t > p.sourceT0 && t < p.sourceT1)) {
                const bool start = atStart || (!atEnd && t - p.sourceT0 <= p.sourceT1 - t);
                // the neighbouring end moves with it, across the seam of a closed branch too
                const bool closed = branches[b].closed;
                if (start) {
                    piece.bezier[0] = node;
                    if (k > 0 || closed) {
                        pieces[(k + pieces.size() - 1) % pieces.size()].bezier[3] = node;
                    }
                }
                else {
                    piece.bezier[3] = node;
                    if (k + 1 < pieces.size() || closed) {
                        pieces[(k + 1) % pieces.size()].bezier[0] = node;
                    }
                }
                return LC_CurveOffsetStatus::Ok;
            }
            OffsetJet q0;
            OffsetJet qt;
            OffsetJet q1;
            LC_CurveOffsetStatus status =
                computeOffsetJet(source, d, speedFloor, p.sourceT0, LC_CurveEvaluationSide::Right, q0);
            if (status == LC_CurveOffsetStatus::Ok) {
                status = computeOffsetJet(source, d, speedFloor, t, LC_CurveEvaluationSide::Interior, qt);
            }
            if (status == LC_CurveOffsetStatus::Ok) {
                status = computeOffsetJet(source, d, speedFloor, p.sourceT1, LC_CurveEvaluationSide::Left, q1);
            }
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
            LC_OffsetCubicPiece left = piece;
            LC_OffsetCubicPiece right = piece;
            left.provenance.sourceT1 = t;
            right.provenance.sourceT0 = t;
            left.bezier = hermitePiece(piece.bezier[0], q0.first, node, qt.first, t - p.sourceT0);
            right.bezier = hermitePiece(node, qt.first, piece.bezier[3], q1.first, p.sourceT1 - t);
            pieces[k] = left;
            pieces.insert(pieces.begin() + static_cast<std::ptrdiff_t>(k) + 1, right);
            return LC_CurveOffsetStatus::Ok;
        }
        return LC_CurveOffsetStatus::FitFailed; // no piece holds the parameter
    };
    for (const Occurrence& o : occurrences) {
        if (o.branch >= branches.size()) {
            return LC_CurveOffsetStatus::FitFailed;
        }
        const LC_CurveOffsetStatus status = insert(o.branch, o.t, o.point);
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
    }
    return LC_CurveOffsetStatus::Ok;
}

LC_CurveOffsetStatus insertNodes(const OffsetSource& source, const double d, const double speedFloor,
                                 const LC_CurveOffsetOptions& options,
                                 const std::vector<LC_ParametricIntersection>& intersections,
                                 std::vector<LC_OffsetBranch>& branches) {
    std::vector<Occurrence> occurrences;
    for (const LC_ParametricIntersection& x : intersections) {
        occurrences.push_back({x.branchA, x.parameterA, x.point});
        occurrences.push_back({x.branchB, x.parameterB, x.point});
    }
    return insertOccurrences(source, d, speedFloor, options, occurrences, branches);
}

LC_CurveOffsetStatus fromIntersectionStatus(const LC_IntersectionStatus status) {
    switch (status) {
        case LC_IntersectionStatus::Ok:
            return LC_CurveOffsetStatus::Ok;
        case LC_IntersectionStatus::AmbiguousTopology:
            return LC_CurveOffsetStatus::AmbiguousTopology;
        case LC_IntersectionStatus::LimitExceeded:
            return LC_CurveOffsetStatus::LimitExceeded;
        case LC_IntersectionStatus::InvalidInput:
            break;
    }
    return LC_CurveOffsetStatus::FitFailed;
}

/** Cubic Bezier pieces as parametric curves, each piece one segment on [0, 1]. */
class PieceCurves final : public LC_ParametricCurves {
public:
    PieceCurves(const std::vector<std::vector<std::array<RS_Vector, 4>>>& branches, std::vector<std::ptrdiff_t> next)
        : m_next{std::move(next)} {
        for (size_t b = 0; b < branches.size(); ++b) {
            for (const std::array<RS_Vector, 4>& piece : branches[b]) {
                m_segments.push_back(LC_ParametricSegment{b, 0.0, 1.0});
                m_pieces.push_back(piece);
            }
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_next.size();
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        return m_next[branch];
    }

    bool evaluate(const std::size_t segment, const double s, RS_Vector& point, RS_Vector& derivative) const override {
        const std::array<RS_Vector, 4>& b = m_pieces[segment];
        const double r = 1.0 - s;
        point = b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
        derivative = ((b[1] - b[0]) * (r * r) + (b[2] - b[1]) * (2.0 * r * s) + (b[3] - b[2]) * (s * s)) * 3.0;
        return isFinite(point) && isFinite(derivative);
    }

    /** From the piece's Bezier nets over [a, b], by blossoming, and those of its derivative. */
    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const std::array<RS_Vector, 4>& p = m_pieces[segment];
        const auto pt = [](const double v) { return LC_Interval::point(v); };
        const LC_Interval one = pt(1.0);
        const LC_Interval ua = pt(a);
        const LC_Interval ub = pt(b);
        // blossom of the cubic, of its quadratic derivative (without the factor 3)
        const auto cubic = [&](const LC_Interval& u1, const LC_Interval& u2, const LC_Interval& u3, const double c0,
                               const double c1, const double c2, const double c3) {
            LC_Interval q[4] = {pt(c0), pt(c1), pt(c2), pt(c3)};
            const LC_Interval* u[3] = {&u1, &u2, &u3};
            for (int r = 0; r < 3; ++r) {
                for (int j = 0; j < 3 - r; ++j) {
                    q[j] = (one - *u[r]) * q[j] + *u[r] * q[j + 1];
                }
            }
            return q[0];
        };
        const auto quadratic = [&](const LC_Interval& u1, const LC_Interval& u2, const double c0, const double c1,
                                   const double c2) {
            LC_Interval q[3] = {pt(c0), pt(c1), pt(c2)};
            const LC_Interval* u[2] = {&u1, &u2};
            for (int r = 0; r < 2; ++r) {
                for (int j = 0; j < 2 - r; ++j) {
                    q[j] = (one - *u[r]) * q[j] + *u[r] * q[j + 1];
                }
            }
            return q[0];
        };
        const LC_Interval* args[4][3] = {{&ua, &ua, &ua}, {&ua, &ua, &ub}, {&ua, &ub, &ub}, {&ub, &ub, &ub}};
        x = LC_Interval{};
        y = LC_Interval{};
        for (const auto& u : args) {
            const LC_Interval vx = cubic(*u[0], *u[1], *u[2], p[0].x, p[1].x, p[2].x, p[3].x);
            const LC_Interval vy = cubic(*u[0], *u[1], *u[2], p[0].y, p[1].y, p[2].y, p[3].y);
            x = x.isValid() ? LC_Interval::hull(x, vx) : vx;
            y = y.isValid() ? LC_Interval::hull(y, vy) : vy;
        }
        const LC_Interval* args2[3][2] = {{&ua, &ua}, {&ua, &ub}, {&ub, &ub}};
        dx = LC_Interval{};
        dy = LC_Interval{};
        for (const auto& u : args2) {
            const LC_Interval vx = pt(3.0) * quadratic(*u[0], *u[1], p[1].x - p[0].x, p[2].x - p[1].x, p[3].x - p[2].x);
            const LC_Interval vy = pt(3.0) * quadratic(*u[0], *u[1], p[1].y - p[0].y, p[2].y - p[1].y, p[3].y - p[2].y);
            dx = dx.isValid() ? LC_Interval::hull(dx, vx) : vx;
            dy = dy.isValid() ? LC_Interval::hull(dy, vy) : vy;
        }
        return x.isValid() && y.isValid() && dx.isValid() && dy.isValid();
    }

private:
    std::vector<std::ptrdiff_t> m_next;
    std::vector<LC_ParametricSegment> m_segments;
    std::vector<std::array<RS_Vector, 4>> m_pieces;
};

// ---------------------------------------------------------------------------
// Materialization and independent validation (plan 5.7)
// ---------------------------------------------------------------------------

/** One clamped cubic spline holding exactly this Bezier piece, on the knot domain [0, 1]. */
RS_SplineData cubicPieceData(const std::array<RS_Vector, 4>& bezier) {
    RS_SplineData data(3, false);
    data.type = RS_SplineData::SplineType::ClampedOpen;
    data.controlPoints.assign(bezier.begin(), bezier.end());
    for (RS_Vector& v : data.controlPoints) {
        v.z = 0.0;
    }
    data.knotslist = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    data.weights.assign(4, 1.0);
    return data;
}

class Materializer {
public:
    Materializer(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                 const LC_OffsetSourceBudget& budget, const double speedFloor, const std::size_t samplesUsed)
        : m_source{source},
          m_d{d},
          m_options{options},
          m_budget{budget},
          m_speedFloor{speedFloor},
          m_samples{samplesUsed} {
    }

    LC_CurveOffsetStatus run(const LC_OffsetBranch& branch, std::vector<std::unique_ptr<RS_Entity>>& entities,
                             LC_OffsetOutputUsage& usage, double& maxError) {
        if (branch.cubicPieces.empty()) {
            return LC_CurveOffsetStatus::FitFailed;
        }
        std::vector<Work> work;
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            work.push_back(Work{piece, nullptr, false, false, 0.0, 0.5, 0});
        }
        constexpr int maxRounds = 4;
        for (int round = 0;; ++round) {
            bool anyFailed = false;
            for (Work& w : work) {
                if (w.checked) {
                    continue;
                }
                LC_CurveOffsetStatus status = makeEntity(w, branch.straight);
                if (status == LC_CurveOffsetStatus::Ok) {
                    status = validate(w);
                }
                if (status != LC_CurveOffsetStatus::Ok) {
                    return status;
                }
                anyFailed = anyFailed || !w.passed;
            }
            if (!anyFailed) {
                break;
            }
            if (round + 1 >= maxRounds || branch.straight) {
                return LC_CurveOffsetStatus::ToleranceNotMet;
            }
            for (size_t i = work.size(); i-- > 0;) {
                if (!work[i].passed) {
                    const LC_CurveOffsetStatus status = split(work, i);
                    if (status != LC_CurveOffsetStatus::Ok) {
                        return status;
                    }
                }
            }
        }

        // The whole output once more: no duplicate root or shared child, and
        // within the deep limit.
        std::vector<const RS_Entity*> roots;
        for (const Work& w : work) {
            roots.push_back(w.entity.get());
        }
        const LC_OffsetTreeCost cost = measureOffsetOutput(roots, m_budget.maxDeepEntities);
        if (cost.status != LC_OffsetTreeStatus::Ok) {
            return cost.status == LC_OffsetTreeStatus::LimitExceeded ? LC_CurveOffsetStatus::LimitExceeded
                                                                     : LC_CurveOffsetStatus::FitFailed;
        }
        usage.cubicPieces = work.size();
        usage.outputEntities = work.size();
        usage.deepEntities = cost.deepEntities;
        maxError = 0.0;
        for (Work& w : work) {
            maxError = std::max(maxError, w.error);
            entities.push_back(std::move(w.entity));
        }
        return LC_CurveOffsetStatus::Ok;
    }

private:
    struct Work {
        LC_OffsetCubicPiece piece;
        std::unique_ptr<RS_Entity> entity;
        bool checked;
        bool passed;
        double error;
        /** Local parameter of the worst sample: where a failed piece is split. */
        double worstU;
        std::size_t deepCost;
    };

    /** The exact offset at t, at the piece's own signed distance. */
    LC_CurveOffsetStatus jet(const double t, const LC_CurveEvaluationSide side, const double d, OffsetJet& out) {
        if (++m_samples > m_options.maxSamples) {
            return LC_CurveOffsetStatus::LimitExceeded;
        }
        return computeOffsetJet(m_source, d, m_speedFloor, t, side, out);
    }

    /**
     * The entity for a piece, counted against the output limits before it is
     * kept: every piece becomes one entity, whose display segments count as its
     * deep cost.
     */
    LC_CurveOffsetStatus makeEntity(Work& w, const bool straight) {
        if (m_pieces >= m_budget.maxCubicPieces || m_pieces >= m_budget.maxOutputEntities) {
            return LC_CurveOffsetStatus::LimitExceeded;
        }
        std::unique_ptr<RS_Entity> entity;
        const std::array<RS_Vector, 4>& b = w.piece.bezier;
        if (straight) {
            entity = std::make_unique<RS_Line>(nullptr, RS_LineData{b[0], b[3]});
        }
        else {
            auto spline = std::make_unique<RS_Spline>(nullptr, cubicPieceData(b));
            // a spline without the segments it is drawn by would be an
            // invisible success
            if (!spline->validate() || spline->count() == 0) {
                return LC_CurveOffsetStatus::FitFailed;
            }
            entity = std::move(spline);
        }
        entity->calculateBorders();
        if (!isFinite(entity->getMin()) || !isFinite(entity->getMax())) {
            return LC_CurveOffsetStatus::FitFailed;
        }
        const LC_OffsetTreeCost cost = measureOffsetOutput({entity.get()}, m_budget.maxDeepEntities - m_deep);
        if (cost.status != LC_OffsetTreeStatus::Ok) {
            return cost.status == LC_OffsetTreeStatus::LimitExceeded ? LC_CurveOffsetStatus::LimitExceeded
                                                                     : LC_CurveOffsetStatus::FitFailed;
        }
        m_deep += cost.deepEntities;
        ++m_pieces;
        w.deepCost = cost.deepEntities;
        w.entity = std::move(entity);
        return LC_CurveOffsetStatus::Ok;
    }

    static bool evaluateEntity(const RS_Entity& entity, const double u, const LC_CurveEvaluationSide side,
                               RS_Vector& point) {
        if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
            LC_CurveJet jet;
            if (!spline->tryEvaluateJet(u, side, jet)) {
                return false;
            }
            point = jet.point;
            return true;
        }
        if (const auto* line = dynamic_cast<const RS_Line*>(&entity)) {
            point = line->getStartpoint() + (line->getEndpoint() - line->getStartpoint()) * u;
            return true;
        }
        return false;
    }

    /**
     * The entity against the exact offset at fresh parameters, with a seed of
     * its own: paired at the same source parameter, which catches a wrong
     * parameter map, and from the entity to the nearest offset point on the
     * piece's interval, which catches an overshoot.
     */
    LC_CurveOffsetStatus validate(Work& w) {
        const LC_OffsetBranchProvenance& p = w.piece.provenance;
        if (p.arcCentre.valid) {
            return validateArc(w);
        }
        const double h = p.sourceT1 - p.sourceT0;
        double nodes[11] = {0.0, 1.0};
        for (int k = 1; k <= 9; ++k) {
            const double frac = std::fmod(0.3183098861837907 + k * 0.4142135623730951, 1.0);
            nodes[k + 1] = 0.02 + 0.96 * frac;
        }
        w.error = 0.0;
        w.worstU = 0.5;
        // a piece may run against the source parameter, from sourceT0 down to sourceT1
        const double lo = std::min(p.sourceT0, p.sourceT1);
        const double hi = std::max(p.sourceT0, p.sourceT1);
        for (const double u : nodes) {
            const LC_CurveEvaluationSide side = (u == 0.0)   ? LC_CurveEvaluationSide::Right
                                                : (u == 1.0) ? LC_CurveEvaluationSide::Left
                                                             : LC_CurveEvaluationSide::Interior;
            RS_Vector candidate;
            if (!evaluateEntity(*w.entity, u, side, candidate)) {
                return LC_CurveOffsetStatus::FitFailed;
            }
            const double t = (u == 0.0) ? p.sourceT0 : (u == 1.0) ? p.sourceT1 : p.sourceT0 + u * h;
            const LC_CurveEvaluationSide sourceSide = (t == lo)   ? LC_CurveEvaluationSide::Right
                                                      : (t == hi) ? LC_CurveEvaluationSide::Left
                                                                  : LC_CurveEvaluationSide::Interior;
            OffsetJet exact;
            LC_CurveOffsetStatus status = jet(t, sourceSide, p.signedDistance, exact);
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
            const double paired = candidate.distanceTo(exact.point);

            // Gauss-Newton towards the offset point nearest to the candidate
            double tr = t;
            OffsetJet q = exact;
            for (int iteration = 0; iteration < 8; ++iteration) {
                const double speed2 = dot(q.first, q.first);
                if (!(speed2 > 0.0)) {
                    break;
                }
                const double step = dot(q.point - candidate, q.first) / speed2;
                const double next = std::clamp(tr - step, lo, hi);
                if (std::abs(next - tr) <= m_options.tolerance.parameter) {
                    break;
                }
                tr = next;
                status = jet(tr, LC_CurveEvaluationSide::Interior, p.signedDistance, q);
                if (status != LC_CurveOffsetStatus::Ok) {
                    return status;
                }
            }
            const double reverse = candidate.distanceTo(q.point);
            const double error = std::max(paired, reverse);
            if (error > w.error) {
                w.error = error;
                w.worstU = u;
            }
        }
        w.checked = true;
        w.passed = w.error <= m_options.tolerance.requestedGeometry;
        return LC_CurveOffsetStatus::Ok;
    }

    /** A vertex arc against its circle: paired at the same angle, and radially. */
    LC_CurveOffsetStatus validateArc(Work& w) {
        const LC_OffsetBranchProvenance& p = w.piece.provenance;
        const double radius = std::abs(p.signedDistance);
        w.error = 0.0;
        w.worstU = 0.5;
        for (int k = 0; k <= 16; ++k) {
            const double u = k / 16.0;
            RS_Vector candidate;
            if (!evaluateEntity(*w.entity, u, (k == 0)    ? LC_CurveEvaluationSide::Right
                                              : (k == 16) ? LC_CurveEvaluationSide::Left
                                                          : LC_CurveEvaluationSide::Interior,
                                candidate)) {
                return LC_CurveOffsetStatus::FitFailed;
            }
            const double angle = p.sourceT0 + u * (p.sourceT1 - p.sourceT0);
            const RS_Vector exact = p.arcCentre + RS_Vector{std::cos(angle), std::sin(angle)} * radius;
            const double error =
                std::max(candidate.distanceTo(exact), std::abs(candidate.distanceTo(p.arcCentre) - radius));
            if (error > w.error) {
                w.error = error;
                w.worstU = u;
            }
        }
        w.checked = true;
        w.passed = w.error <= m_options.tolerance.requestedGeometry;
        return LC_CurveOffsetStatus::Ok;
    }

    /** Replaces a failed piece by two refitted halves split at its worst sample;
     *  its ends stay where the neighbours share them. */
    LC_CurveOffsetStatus split(std::vector<Work>& work, const size_t index) {
        const Work& w = work[index];
        const LC_OffsetBranchProvenance& p = w.piece.provenance;
        const double s = std::clamp(w.worstU, 0.125, 0.875);
        const double tm = p.sourceT0 + s * (p.sourceT1 - p.sourceT0);
        if (p.arcCentre.valid) {
            Work left{w.piece, nullptr, false, false, 0.0, 0.5, 0};
            Work right{w.piece, nullptr, false, false, 0.0, 0.5, 0};
            left.piece.provenance.sourceT1 = tm;
            right.piece.provenance.sourceT0 = tm;
            left.piece.bezier = arcPiece(p.arcCentre, std::abs(p.signedDistance), p.sourceT0, tm);
            right.piece.bezier = arcPiece(p.arcCentre, std::abs(p.signedDistance), tm, p.sourceT1);
            left.piece.bezier[0] = w.piece.bezier[0];
            right.piece.bezier[0] = left.piece.bezier[3];
            right.piece.bezier[3] = w.piece.bezier[3];
            m_deep -= w.deepCost;
            --m_pieces;
            work.erase(work.begin() + static_cast<std::ptrdiff_t>(index));
            work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(right));
            work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(left));
            return LC_CurveOffsetStatus::Ok;
        }
        OffsetJet q0;
        OffsetJet qm;
        OffsetJet q1;
        const bool forwards = p.sourceT0 < p.sourceT1;
        const LC_CurveEvaluationSide inward0 = forwards ? LC_CurveEvaluationSide::Right : LC_CurveEvaluationSide::Left;
        const LC_CurveEvaluationSide inward1 = forwards ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Right;
        LC_CurveOffsetStatus status = jet(p.sourceT0, inward0, p.signedDistance, q0);
        if (status == LC_CurveOffsetStatus::Ok) {
            status = jet(tm, LC_CurveEvaluationSide::Interior, p.signedDistance, qm);
        }
        if (status == LC_CurveOffsetStatus::Ok) {
            status = jet(p.sourceT1, inward1, p.signedDistance, q1);
        }
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
        Work left{w.piece, nullptr, false, false, 0.0, 0.5, 0};
        Work right{w.piece, nullptr, false, false, 0.0, 0.5, 0};
        left.piece.provenance.sourceT1 = tm;
        right.piece.provenance.sourceT0 = tm;
        left.piece.bezier = hermitePiece(w.piece.bezier[0], q0.first, qm.point, qm.first, tm - p.sourceT0);
        right.piece.bezier = hermitePiece(qm.point, qm.first, w.piece.bezier[3], q1.first, p.sourceT1 - tm);
        m_deep -= w.deepCost;
        --m_pieces;
        work.erase(work.begin() + static_cast<std::ptrdiff_t>(index));
        work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(right));
        work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(left));
        return LC_CurveOffsetStatus::Ok;
    }

    const OffsetSource& m_source;
    const double m_d;
    const LC_CurveOffsetOptions& m_options;
    const LC_OffsetSourceBudget& m_budget;
    const double m_speedFloor;
    std::size_t m_samples;
    std::size_t m_pieces{0};
    std::size_t m_deep{0};
};

// ---------------------------------------------------------------------------
// Direct branches and their nodes
// ---------------------------------------------------------------------------

/** The Direct branches at signed distance @p d, before any noding. */
LC_CurveOffsetStatus directBranches(const OffsetSource& source, const SourceScale& scale, const double d,
                                    const LC_CurveOffsetOptions& options, const LC_OffsetSourceBudget& budget,
                                    std::vector<LC_OffsetBranch>& branches, double& maxError, std::size_t& samples) {
    const std::vector<double>& breaks = source.breaks();
    const double domain = breaks.back() - breaks.front();
    branches.clear();
    RS_Vector start;
    RS_Vector end;
    if (source.straightSegment(start, end)) {
        // exact: a straight segment moved along its constant normal
        const RS_Vector direction = end - start;
        const double length = direction.magnitude();
        if (!(length > scale.numericFloor)) {
            return LC_CurveOffsetStatus::UndefinedTangent;
        }
        const RS_Vector shift = RS_Vector{-direction.y, direction.x} * (d / length);
        const RS_Vector a = start + shift;
        const RS_Vector b = end + shift;
        LC_OffsetBranch branch;
        LC_OffsetCubicPiece piece;
        piece.provenance = {0, breaks.front(), breaks.back(), d, true};
        piece.bezier = {a, a + (b - a) / 3.0, b - (b - a) / 3.0, b};
        for (RS_Vector& v : piece.bezier) {
            v.z = 0.0;
        }
        branch.cubicPieces.push_back(piece);
        branch.straight = true;
        branches.push_back(std::move(branch));
        maxError = 0.0;
        return LC_CurveOffsetStatus::Ok;
    }
    std::vector<bool> joinCusps;
    const LC_CurveOffsetStatus joins = checkJoins(source, d, options, joinCusps);
    if (joins != LC_CurveOffsetStatus::Ok) {
        return joins;
    }
    BranchBuilder builder{source, d, options, budget, scale.numericFloor / domain};
    const LC_CurveOffsetStatus status = builder.build(joinCusps, branches);
    samples = builder.samples();
    maxError = builder.maxObservedError();
    return status;
}

/**
 * Nodes the branches where the offset meets itself, and counts the source's
 * own crossings with the same engine at distance zero.
 */
LC_CurveOffsetStatus nodeBranches(const OffsetSource& source, const double d, const double speedFloor,
                                  const LC_CurveOffsetOptions& options, std::vector<LC_OffsetBranch>& branches,
                                  LC_CurveOffsetGeometryResult& result) {
    LC_IntersectionOptions query;
    query.tolerance = options.tolerance.nodeMerge;
    query.maxBoxPairs = options.maxIntersectionPairs;
    const LC_IntersectionResult found = findIntersections(OffsetCurves{source, d, speedFloor, branches}, query);
    LC_CurveOffsetStatus status = fromIntersectionStatus(found.status);
    if (status == LC_CurveOffsetStatus::Ok) {
        status = insertNodes(source, d, speedFloor, options, found.intersections, branches);
    }
    if (status != LC_CurveOffsetStatus::Ok) {
        return status;
    }
    result.intersections = found.intersections;
    result.offsetIntersections = found.intersections.size();

    const std::vector<double>& breaks = source.breaks();
    LC_OffsetBranch whole;
    whole.closed = source.closed();
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        LC_OffsetCubicPiece piece;
        piece.provenance = {span, breaks[span], breaks[span + 1], 0.0, true};
        whole.cubicPieces.push_back(piece);
    }
    const LC_IntersectionResult crossings = findIntersections(OffsetCurves{source, 0.0, speedFloor, {whole}}, query);
    if (crossings.status != LC_IntersectionStatus::Ok) {
        return fromIntersectionStatus(crossings.status);
    }
    result.sourceIntersections = crossings.intersections.size();
    return LC_CurveOffsetStatus::Ok;
}

// ---------------------------------------------------------------------------
// Distance-map trimming (plan 5.12)
// ---------------------------------------------------------------------------

/** cos over [a, b], rounded outward. */
LC_Interval boundCos(const double a, const double b) {
    double lo = std::min(std::cos(a), std::cos(b));
    double hi = std::max(std::cos(a), std::cos(b));
    // the extremes at the multiples of pi inside
    for (double k = std::ceil(a / M_PI); k * M_PI <= b; k += 1.0) {
        if (std::fmod(std::abs(k), 2.0) == 0.0) {
            hi = 1.0;
        }
        else {
            lo = -1.0;
        }
    }
    const double pad = 8.0 * g_eps;
    return LC_Interval::hull(std::max(-1.0, lo - pad), std::min(1.0, hi + pad));
}

/** Circles, one closed branch each on [0, 2 pi]. */
class CircleCurves final : public LC_ParametricCurves {
public:
    CircleCurves(std::vector<RS_Vector> centres, const double radius)
        : m_centres{std::move(centres)},
          m_radius{radius} {
        for (size_t b = 0; b < m_centres.size(); ++b) {
            m_segments.push_back({b, 0.0, 2.0 * M_PI});
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_centres.size();
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        return static_cast<std::ptrdiff_t>(branch);
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        point = m_centres[segment] + RS_Vector{std::cos(t), std::sin(t)} * m_radius;
        derivative = RS_Vector{-std::sin(t), std::cos(t)} * m_radius;
        return true;
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const LC_Interval r = LC_Interval::point(m_radius);
        const LC_Interval c = boundCos(a, b);
        const LC_Interval s = boundCos(a - 0.5 * M_PI, b - 0.5 * M_PI); // sin t = cos(t - pi/2)
        const LC_Interval pad = LC_Interval::hull(-8.0 * g_eps, 8.0 * g_eps);
        x = LC_Interval::point(m_centres[segment].x) + r * (c + pad);
        y = LC_Interval::point(m_centres[segment].y) + r * (s + pad);
        dx = -(r * (s + pad));
        dy = r * (c + pad);
        return true;
    }

private:
    std::vector<RS_Vector> m_centres;
    double m_radius;
    std::vector<LC_ParametricSegment> m_segments;
};

/** Several curve sets as one, their branches numbered in the given order. */
class CompositeCurves final : public LC_ParametricCurves {
public:
    explicit CompositeCurves(std::vector<const LC_ParametricCurves*> parts) : m_parts{std::move(parts)} {
        size_t offset = 0;
        for (size_t p = 0; p < m_parts.size(); ++p) {
            m_offsets.push_back(offset);
            const std::vector<LC_ParametricSegment>& segments = m_parts[p]->segments();
            for (size_t s = 0; s < segments.size(); ++s) {
                m_segments.push_back({segments[s].branch + offset, segments[s].t0, segments[s].t1});
                m_origin.emplace_back(p, s);
            }
            offset += m_parts[p]->branchCount();
        }
        m_branches = offset;
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_branches;
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        size_t p = m_parts.size() - 1;
        while (m_offsets[p] > branch) {
            --p;
        }
        const std::ptrdiff_t local = m_parts[p]->next(branch - m_offsets[p]);
        return local < 0 ? -1 : local + static_cast<std::ptrdiff_t>(m_offsets[p]);
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const auto& [p, s] = m_origin[segment];
        return m_parts[p]->evaluate(s, t, point, derivative);
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const auto& [p, s] = m_origin[segment];
        return m_parts[p]->bound(s, a, b, x, y, dx, dy);
    }

private:
    std::vector<const LC_ParametricCurves*> m_parts;
    std::vector<size_t> m_offsets;
    std::vector<LC_ParametricSegment> m_segments;
    std::vector<std::pair<size_t, size_t>> m_origin;
    size_t m_branches{0};
};

enum class Visibility {
    Visible,
    Hidden,
    Undecided
};

/**
 * Whether @p p is definitely farther than @p rho from the source, definitely
 * nearer, or neither to within the box budget: branch and bound over the
 * source's spans, with lower bounds from their boxes and upper bounds from
 * exact points refined by projection.
 */
Visibility visibility(const OffsetSource& source, const std::vector<std::pair<double, double>>& intervals,
                      const RS_Vector& p, const double rho, std::size_t& boxes, const std::size_t maxBoxes) {
    struct Box {
        double lower;
        size_t span;
        double a;
        double b;
    };
    const auto farther = [](const Box& l, const Box& r) {
        return std::tie(l.lower, l.span, l.a) > std::tie(r.lower, r.span, r.a);
    };
    const auto lowerOf = [&p](const LC_CurveJetBounds& c) {
        const double gx = std::max({0.0, c.x.lo() - p.x, p.x - c.x.hi()});
        const double gy = std::max({0.0, c.y.lo() - p.y, p.y - c.y.hi()});
        return std::hypot(gx, gy);
    };
    double upper = std::numeric_limits<double>::infinity();
    // the nearest point of a box, from its middle by Newton steps within it
    const auto project = [&](const double a, const double b) {
        double t = a + 0.5 * (b - a);
        for (int iteration = 0; iteration < 8; ++iteration) {
            LC_CurveJet c;
            if (!source.jet(t, LC_CurveEvaluationSide::Interior, c)) {
                return;
            }
            upper = std::min(upper, c.point.distanceTo(p));
            const RS_Vector r = c.point - p;
            const double h = dot(c.first, c.first) + dot(r, c.second);
            if (!(h > 0.0)) {
                return;
            }
            const double next = std::clamp(t - dot(r, c.first) / h, a, b);
            if (next == t) {
                return;
            }
            t = next;
        }
    };
    std::vector<Box> heap;
    const std::vector<double>& breaks = source.breaks();
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        // the parts of the span the intervals cover; all of it without intervals
        std::vector<std::pair<double, double>> parts;
        if (intervals.empty()) {
            parts.emplace_back(breaks[span], breaks[span + 1]);
        }
        for (const auto& [from, to] : intervals) {
            const double a = std::max(from, breaks[span]);
            const double b = std::min(to, breaks[span + 1]);
            if (a < b) {
                parts.emplace_back(a, b);
            }
        }
        for (const auto& [a, b] : parts) {
            LC_CurveJetBounds c;
            if (!source.boundJet(a, b, c)) {
                return Visibility::Undecided;
            }
            heap.push_back({lowerOf(c), span, a, b});
            project(a, b);
        }
    }
    std::make_heap(heap.begin(), heap.end(), farther);
    while (true) {
        if (upper < rho) {
            return Visibility::Hidden;
        }
        if (heap.empty() || heap.front().lower >= rho) {
            return Visibility::Visible;
        }
        std::pop_heap(heap.begin(), heap.end(), farther);
        const Box box = heap.back();
        heap.pop_back();
        const double mid = box.a + 0.5 * (box.b - box.a);
        if (++boxes > maxBoxes || !(mid > box.a && mid < box.b)) {
            return Visibility::Undecided;
        }
        for (const auto& [a, b] : {std::pair{box.a, mid}, std::pair{mid, box.b}}) {
            LC_CurveJetBounds c;
            if (!source.boundJet(a, b, c)) {
                return Visibility::Undecided;
            }
            project(a, b);
            const double lower = lowerOf(c);
            if (lower < upper) {
                heap.push_back({lower, box.span, a, b});
                std::push_heap(heap.begin(), heap.end(), farther);
            }
        }
    }
}

/**
 * Removes the parts of the noded Direct offset nearer to the source than
 * rho = |d| minus the approximation and classification budget, and chains
 * what remains through its nodes. Visibility can change only at the offset's
 * nodes and cusps, where it meets the offset on the other side, and, for an
 * open source, where it crosses the circles of radius |d| about its ends; the
 * branches are split there, and each fragment is classified by the distance
 * of an interior point, certified by visibility(), or the request fails.
 */
LC_CurveOffsetStatus trimBranches(const OffsetSource& source, const SourceScale& scale, const double d,
                                  const LC_CurveOffsetOptions& options, const LC_OffsetSourceBudget& budget,
                                  std::vector<LC_OffsetBranch>& branches, LC_CurveOffsetGeometryResult& result) {
    const double rho =
        std::abs(d) - (options.tolerance.requestedGeometry + options.tolerance.classification);
    if (!(rho > scale.numericFloor) || branches.front().straight) {
        return LC_CurveOffsetStatus::Ok; // nothing can be resolved as hidden: the Direct offset stands
    }
    const std::vector<double>& breaks = source.breaks();
    const double speedFloor = scale.numericFloor / (breaks.back() - breaks.front());

    // where the offset meets the other side's offset and the circles about the ends
    std::vector<LC_OffsetBranch> opposite;
    double ignoredError = 0.0;
    std::size_t ignoredSamples = 0;
    const LC_CurveOffsetStatus other =
        directBranches(source, scale, -d, options, budget, opposite, ignoredError, ignoredSamples);
    if (other != LC_CurveOffsetStatus::Ok) {
        return LC_CurveOffsetStatus::AmbiguousTopology; // transitions against it cannot be found
    }
    std::vector<RS_Vector> ends;
    if (!source.closed()) {
        LC_CurveJet first;
        LC_CurveJet last;
        if (!source.jet(breaks.front(), LC_CurveEvaluationSide::Right, first) ||
            !source.jet(breaks.back(), LC_CurveEvaluationSide::Left, last)) {
            return LC_CurveOffsetStatus::InvalidSource;
        }
        ends = {first.point, last.point};
    }
    const OffsetCurves self{source, d, speedFloor, branches};
    const OffsetCurves across{source, -d, speedFloor, opposite};
    const CircleCurves caps{ends, std::abs(d)};
    const CompositeCurves all{{&self, &across, &caps}};
    LC_IntersectionOptions query;
    query.tolerance = options.tolerance.nodeMerge;
    query.maxBoxPairs = options.maxIntersectionPairs;
    const LC_IntersectionResult found = findIntersections(all, query);
    if (found.status != LC_IntersectionStatus::Ok) {
        return fromIntersectionStatus(found.status);
    }
    const size_t own = branches.size();
    std::vector<Occurrence> cuts;
    for (const LC_ParametricIntersection& x : found.intersections) {
        // one occurrence on this offset, the other elsewhere (its own crossings are nodes already)
        if (x.branchA < own && x.branchB >= own) {
            cuts.push_back({x.branchA, x.parameterA, x.point});
        }
    }
    LC_CurveOffsetStatus status = insertOccurrences(source, d, speedFloor, options, cuts, branches);
    if (status != LC_CurveOffsetStatus::Ok) {
        return status;
    }
    std::vector<RS_Vector> boundaries;
    for (const LC_ParametricIntersection& x : result.intersections) {
        boundaries.push_back(x.point);
    }
    for (const Occurrence& c : cuts) {
        boundaries.push_back(c.point);
    }
    const auto isBoundary = [&boundaries](const RS_Vector& v) {
        return std::find(boundaries.begin(), boundaries.end(), v) != boundaries.end();
    };

    // fragments: runs of pieces between boundaries, and the ends of branches
    struct Fragment {
        std::vector<LC_OffsetCubicPiece> pieces;
        bool keep{false};
    };
    std::vector<Fragment> fragments;
    for (const LC_OffsetBranch& branch : branches) {
        const size_t first = fragments.size();
        fragments.emplace_back();
        for (size_t k = 0; k < branch.cubicPieces.size(); ++k) {
            fragments.back().pieces.push_back(branch.cubicPieces[k]);
            if (k + 1 < branch.cubicPieces.size() && isBoundary(branch.cubicPieces[k].bezier[3])) {
                fragments.emplace_back();
            }
        }
        // a closed branch's seam is not a boundary unless a node lies there
        if (branch.closed && fragments.size() - first > 1 && !isBoundary(branch.cubicPieces.front().bezier[0])) {
            std::vector<LC_OffsetCubicPiece>& wrapped = fragments.back().pieces;
            wrapped.insert(wrapped.end(), fragments[first].pieces.begin(), fragments[first].pieces.end());
            fragments[first].pieces = std::move(wrapped);
            fragments.pop_back();
        }
    }

    // each fragment by the distance of interior points
    std::size_t boxes = 0;
    for (Fragment& fragment : fragments) {
        double size = 0.0;
        for (const LC_OffsetCubicPiece& piece : fragment.pieces) {
            for (const RS_Vector& v : piece.bezier) {
                size = std::max(size, v.distanceTo(fragment.pieces.front().bezier[0]));
            }
        }
        if (size <= options.tolerance.nodeMerge) {
            continue; // below the resolution: dropped either way
        }
        Visibility seen = Visibility::Undecided;
        for (const double where : {0.5, 0.25, 0.75}) {
            const LC_OffsetCubicPiece& piece =
                fragment.pieces[std::min(fragment.pieces.size() - 1,
                                         static_cast<size_t>(where * static_cast<double>(fragment.pieces.size())))];
            const double t = 0.5 * (piece.provenance.sourceT0 + piece.provenance.sourceT1);
            OffsetJet q;
            if (computeOffsetJet(source, d, speedFloor, t, LC_CurveEvaluationSide::Interior, q) !=
                LC_CurveOffsetStatus::Ok) {
                return LC_CurveOffsetStatus::FitFailed;
            }
            seen = visibility(source, {}, q.point, rho, boxes, options.maxDistanceMapBoxes);
            if (seen != Visibility::Undecided) {
                break;
            }
            if (boxes > options.maxDistanceMapBoxes) {
                return LC_CurveOffsetStatus::LimitExceeded;
            }
        }
        if (seen == Visibility::Undecided) {
            return LC_CurveOffsetStatus::AmbiguousTopology;
        }
        fragment.keep = seen == Visibility::Visible;
    }

    // what remains, chained where one fragment ends on the next one's start
    std::vector<LC_OffsetBranch> kept;
    size_t removed = 0;
    for (Fragment& fragment : fragments) {
        if (!fragment.keep) {
            ++removed;
            continue;
        }
        if (!kept.empty() && kept.back().cubicPieces.back().bezier[3] == fragment.pieces.front().bezier[0]) {
            kept.back().cubicPieces.insert(kept.back().cubicPieces.end(), fragment.pieces.begin(),
                                           fragment.pieces.end());
            continue;
        }
        LC_OffsetBranch branch;
        branch.cubicPieces = std::move(fragment.pieces);
        kept.push_back(std::move(branch));
    }
    if (kept.size() > 1 && kept.back().cubicPieces.back().bezier[3] == kept.front().cubicPieces.front().bezier[0]) {
        std::vector<LC_OffsetCubicPiece>& wrapped = kept.back().cubicPieces;
        wrapped.insert(wrapped.end(), kept.front().cubicPieces.begin(), kept.front().cubicPieces.end());
        kept.front().cubicPieces = std::move(wrapped);
        kept.pop_back();
    }
    if (kept.size() == 1 && removed == 0 && branches.size() == 1 && branches.front().closed) {
        kept.front().closed = true;
    }
    result.removedIntervals = removed;
    branches = std::move(kept);
    return LC_CurveOffsetStatus::Ok;
}

// ---------------------------------------------------------------------------
// Region boundary (plan 5.13)
// ---------------------------------------------------------------------------

/**
 * How many times the closed source winds around @p p, counter-clockwise
 * positive: the angle it turns through as seen from p, summed over pieces each
 * farther from p than they are wide, so none can wind around p inside itself.
 * False where p lies on the curve, or past the box budget.
 */
bool windingNumber(const OffsetSource& source, const RS_Vector& p, int& winding, std::size_t& boxes,
                   const std::size_t maxBoxes) {
    const std::vector<double>& breaks = source.breaks();
    double turn = 0.0;
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        std::vector<std::pair<double, double>> stack{{breaks[span], breaks[span + 1]}};
        while (!stack.empty()) {
            const auto [a, b] = stack.back();
            stack.pop_back();
            LC_CurveJetBounds c;
            if (++boxes > maxBoxes || !source.boundJet(a, b, c)) {
                return false;
            }
            const double gx = std::max({0.0, c.x.lo() - p.x, p.x - c.x.hi()});
            const double gy = std::max({0.0, c.y.lo() - p.y, p.y - c.y.hi()});
            const double gap = std::hypot(gx, gy);
            if (gap > 0.0 && std::hypot(c.x.width(), c.y.width()) < gap) {
                LC_CurveJet ja;
                LC_CurveJet jb;
                if (!source.jet(a, LC_CurveEvaluationSide::Right, ja) ||
                    !source.jet(b, LC_CurveEvaluationSide::Left, jb)) {
                    return false;
                }
                const RS_Vector u = ja.point - p;
                const RS_Vector v = jb.point - p;
                turn += std::atan2(cross(u, v), dot(u, v));
                continue;
            }
            const double mid = a + 0.5 * (b - a);
            if (!(mid > a && mid < b)) {
                return false; // on the curve
            }
            stack.emplace_back(mid, b);
            stack.emplace_back(a, mid);
        }
    }
    const double turns = turn / (2.0 * M_PI);
    winding = static_cast<int>(std::lround(turns));
    return std::abs(turns - winding) < 0.25;
}

/** The part [t0, t1] of a source, as an open source of its own with the same parameter. */
class RestrictedSource final : public OffsetSource {
public:
    RestrictedSource(const OffsetSource& base, const double t0, const double t1) : m_base{base} {
        m_breaks.push_back(t0);
        for (const double t : base.breaks()) {
            if (t > t0 && t < t1) {
                m_breaks.push_back(t);
            }
        }
        m_breaks.push_back(t1);
    }

    bool closed() const override {
        return false;
    }

    const std::vector<double>& breaks() const override {
        return m_breaks;
    }

    bool jet(const double t, const LC_CurveEvaluationSide side, LC_CurveJet& out) const override {
        return m_base.jet(t, side, out);
    }

    bool boundJet(const double a, const double b, LC_CurveJetBounds& out) const override {
        return m_base.boundJet(a, b, out);
    }

    const std::vector<RS_Vector>& hull() const override {
        return m_base.hull();
    }

    bool straightSegment(RS_Vector&, RS_Vector&) const override {
        return false;
    }

private:
    const OffsetSource& m_base;
    std::vector<double> m_breaks;
};

bool included(const int winding, const LC_CurveFillRule rule) {
    return rule == LC_CurveFillRule::EvenOdd ? (winding % 2 != 0) : (winding != 0);
}

/**
 * The boundary of the region the closed source encloses under the fill rule,
 * grown (dilate) or shrunk by the disk of radius @p magnitude, as closed
 * cycles with the result on their left. The source is noded at its crossings;
 * a fragment between them is on the region's boundary when the faces on its
 * two sides differ under the fill rule, which a winding number either side
 * decides. The candidates are those fragments' offsets towards the outside
 * (dilate) or inside, and circles of radius |d| about the source's nodes, all
 * noded together. A candidate fragment is kept when an interior point lies
 * outside the source region (dilate) or inside it, and provably no nearer to
 * the region's boundary than |d|; what is kept is chained into cycles.
 */
LC_CurveOffsetStatus regionBoundary(const OffsetSource& source, const SourceScale& scale, const double magnitude,
                                    const bool dilate, const LC_CurveOffsetOptions& options,
                                    const LC_OffsetSourceBudget& budget, std::vector<LC_OffsetBranch>& cycles,
                                    LC_CurveOffsetGeometryResult& result) {
    if (!source.closed()) {
        return LC_CurveOffsetStatus::InvalidSource; // a region needs a closed boundary
    }
    const std::vector<double>& breaks = source.breaks();
    const double t0 = breaks.front();
    const double t1 = breaks.back();
    const double speedFloor = scale.numericFloor / (t1 - t0);
    LC_IntersectionOptions query;
    query.tolerance = options.tolerance.nodeMerge;
    query.maxBoxPairs = options.maxIntersectionPairs;
    std::size_t boxes = 0;

    // 1. the source's nodes
    LC_OffsetBranch whole;
    whole.closed = true;
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        LC_OffsetCubicPiece piece;
        piece.provenance = {span, breaks[span], breaks[span + 1], 0.0, true};
        whole.cubicPieces.push_back(piece);
    }
    const LC_IntersectionResult crossings = findIntersections(OffsetCurves{source, 0.0, speedFloor, {whole}}, query);
    if (crossings.status != LC_IntersectionStatus::Ok) {
        return fromIntersectionStatus(crossings.status);
    }
    result.sourceIntersections = crossings.intersections.size();
    std::vector<double> cuts;
    std::vector<RS_Vector> nodes;
    for (const LC_ParametricIntersection& x : crossings.intersections) {
        cuts.push_back(x.parameterA);
        cuts.push_back(x.parameterB);
        const bool known = std::any_of(nodes.begin(), nodes.end(), [&](const RS_Vector& n) {
            return n.distanceTo(x.point) <= options.tolerance.nodeMerge;
        });
        if (!known) {
            nodes.push_back(x.point);
        }
    }
    std::sort(cuts.begin(), cuts.end());

    // 2. source fragments between nodes, as parameter intervals (two when one wraps the seam)
    struct SourceFragment {
        std::vector<std::pair<double, double>> intervals;
        bool regionOnLeft{false};
        bool boundary{false};
    };
    std::vector<SourceFragment> fragments;
    if (cuts.empty()) {
        fragments.push_back({{{t0, t1}}});
    }
    else {
        for (size_t i = 0; i + 1 < cuts.size(); ++i) {
            if (cuts[i] < cuts[i + 1]) {
                fragments.push_back({{{cuts[i], cuts[i + 1]}}});
            }
        }
        SourceFragment wrap;
        if (cuts.back() < t1) {
            wrap.intervals.emplace_back(cuts.back(), t1);
        }
        if (t0 < cuts.front()) {
            wrap.intervals.emplace_back(t0, cuts.front());
        }
        if (!wrap.intervals.empty()) {
            fragments.push_back(wrap);
        }
    }

    // 3. which side of each fragment is in the region
    std::vector<std::pair<double, double>> boundaryIntervals;
    for (SourceFragment& fragment : fragments) {
        const auto& [a, b] = *std::max_element(
            fragment.intervals.begin(), fragment.intervals.end(),
            [](const auto& l, const auto& r) { return l.second - l.first < r.second - r.first; });
        LC_CurveJet c;
        if (!source.jet(a + 0.5 * (b - a), LC_CurveEvaluationSide::Interior, c)) {
            return LC_CurveOffsetStatus::InvalidSource;
        }
        const RS_Vector normal = RS_Vector{-c.first.y, c.first.x} / c.first.magnitude();
        bool decided = false;
        for (double step = 1e-3 * scale.feature; step >= 16.0 * options.tolerance.classification; step /= 8.0) {
            int left = 0;
            int right = 0;
            if (!windingNumber(source, c.point + normal * step, left, boxes, options.maxDistanceMapBoxes) ||
                !windingNumber(source, c.point - normal * step, right, boxes, options.maxDistanceMapBoxes)) {
                continue;
            }
            if (left - right == 1) { // one crossing between them, right to left
                fragment.regionOnLeft = included(left, options.fillRule);
                fragment.boundary = included(left, options.fillRule) != included(right, options.fillRule);
                decided = true;
                break;
            }
        }
        if (!decided) {
            return boxes > options.maxDistanceMapBoxes ? LC_CurveOffsetStatus::LimitExceeded
                                                       : LC_CurveOffsetStatus::AmbiguousTopology;
        }
        if (fragment.boundary) {
            boundaryIntervals.insert(boundaryIntervals.end(), fragment.intervals.begin(), fragment.intervals.end());
        }
    }
    cycles.clear();
    if (boundaryIntervals.empty()) {
        return LC_CurveOffsetStatus::Ok; // no region: nothing to grow or shrink
    }

    // 4. candidates: the boundary fragments' offsets outwards or inwards, and circles about the nodes
    struct Candidate {
        std::unique_ptr<RestrictedSource> part;
        double signedDistance;
        bool reverse; // runs with the region on its right
        std::vector<LC_OffsetBranch> branches;
    };
    std::vector<Candidate> candidates;
    for (const SourceFragment& fragment : fragments) {
        if (!fragment.boundary) {
            continue;
        }
        for (const auto& [a, b] : fragment.intervals) {
            Candidate candidate;
            candidate.part = std::make_unique<RestrictedSource>(source, a, b);
            // outward is right of a boundary with the region on its left
            candidate.signedDistance = (dilate == fragment.regionOnLeft) ? -magnitude : magnitude;
            candidate.reverse = !fragment.regionOnLeft;
            double ignoredError = 0.0;
            std::size_t ignoredSamples = 0;
            const LC_CurveOffsetStatus status = directBranches(*candidate.part, scale, candidate.signedDistance,
                                                               options, budget, candidate.branches, ignoredError,
                                                               ignoredSamples);
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
            candidates.push_back(std::move(candidate));
        }
    }
    std::vector<std::unique_ptr<OffsetCurves>> offsetCurves;
    std::vector<const LC_ParametricCurves*> parts;
    std::vector<size_t> firstBranch;
    size_t branchCount = 0;
    for (const Candidate& candidate : candidates) {
        offsetCurves.push_back(std::make_unique<OffsetCurves>(*candidate.part, candidate.signedDistance, speedFloor,
                                                              candidate.branches));
        parts.push_back(offsetCurves.back().get());
        firstBranch.push_back(branchCount);
        branchCount += candidate.branches.size();
    }
    const CircleCurves circles{nodes, magnitude};
    parts.push_back(&circles);
    const size_t firstCircle = branchCount;

    // 5. node all candidates together, and split them there
    const LC_IntersectionResult found = findIntersections(CompositeCurves{parts}, query);
    if (found.status != LC_IntersectionStatus::Ok) {
        return fromIntersectionStatus(found.status);
    }
    std::vector<std::vector<Occurrence>> offsetCuts(candidates.size());
    std::vector<std::vector<std::pair<double, RS_Vector>>> arcCuts(nodes.size());
    std::vector<RS_Vector> boundaries;
    auto place = [&](const size_t branch, const double t, const RS_Vector& point) {
        if (branch >= firstCircle) {
            arcCuts[branch - firstCircle].emplace_back(t, point);
            return;
        }
        size_t c = candidates.size() - 1;
        while (firstBranch[c] > branch) {
            --c;
        }
        offsetCuts[c].push_back({branch - firstBranch[c], t, point});
    };
    for (const LC_ParametricIntersection& x : found.intersections) {
        place(x.branchA, x.parameterA, x.point);
        place(x.branchB, x.parameterB, x.point);
        boundaries.push_back(x.point);
    }
    for (size_t c = 0; c < candidates.size(); ++c) {
        const LC_CurveOffsetStatus status = insertOccurrences(*candidates[c].part, candidates[c].signedDistance,
                                                              speedFloor, options, offsetCuts[c],
                                                              candidates[c].branches);
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
    }
    const auto isBoundary = [&boundaries](const RS_Vector& v) {
        return std::find(boundaries.begin(), boundaries.end(), v) != boundaries.end();
    };

    // 6. candidate fragments, each with the result on its left if kept
    struct Piece {
        std::vector<LC_OffsetCubicPiece> pieces;
        RS_Vector sample;
        RS_Vector alternative[2];
    };
    std::vector<Piece> pieces;
    for (const Candidate& candidate : candidates) {
        for (const LC_OffsetBranch& branch : candidate.branches) {
            std::vector<std::vector<LC_OffsetCubicPiece>> runs(1);
            for (size_t k = 0; k < branch.cubicPieces.size(); ++k) {
                runs.back().push_back(branch.cubicPieces[k]);
                if (k + 1 < branch.cubicPieces.size() && isBoundary(branch.cubicPieces[k].bezier[3])) {
                    runs.emplace_back();
                }
            }
            for (std::vector<LC_OffsetCubicPiece>& run : runs) {
                Piece piece;
                const double where[3] = {0.5, 0.25, 0.75};
                RS_Vector samples[3];
                for (int k = 0; k < 3; ++k) {
                    const LC_OffsetCubicPiece& at =
                        run[std::min(run.size() - 1, static_cast<size_t>(where[k] * static_cast<double>(run.size())))];
                    OffsetJet q;
                    if (computeOffsetJet(*candidate.part, candidate.signedDistance, speedFloor,
                                         0.5 * (at.provenance.sourceT0 + at.provenance.sourceT1),
                                         LC_CurveEvaluationSide::Interior, q) != LC_CurveOffsetStatus::Ok) {
                        return LC_CurveOffsetStatus::FitFailed;
                    }
                    samples[k] = q.point;
                }
                piece.sample = samples[0];
                piece.alternative[0] = samples[1];
                piece.alternative[1] = samples[2];
                if (candidate.reverse) {
                    std::reverse(run.begin(), run.end());
                    for (LC_OffsetCubicPiece& p : run) {
                        std::reverse(p.bezier.begin(), p.bezier.end());
                        std::swap(p.provenance.sourceT0, p.provenance.sourceT1);
                        p.provenance.forward = false;
                    }
                }
                piece.pieces = std::move(run);
                pieces.push_back(std::move(piece));
            }
        }
    }
    for (size_t n = 0; n < nodes.size(); ++n) {
        std::vector<std::pair<double, RS_Vector>>& at = arcCuts[n];
        std::sort(at.begin(), at.end(), [](const auto& l, const auto& r) { return l.first < r.first; });
        if (at.empty()) {
            at.emplace_back(0.0, nodes[n] + RS_Vector{magnitude, 0.0});
        }
        for (size_t k = 0; k < at.size(); ++k) {
            const double a0 = at[k].first;
            const double a1 = (k + 1 < at.size()) ? at[k + 1].first : at.front().first + 2.0 * M_PI;
            if (!(a1 > a0)) {
                continue;
            }
            // counter-clockwise the grown region is inside the circle, on the left
            const bool ccw = dilate;
            const double from = ccw ? a0 : a1;
            const double to = ccw ? a1 : a0;
            const int count = std::max(1, static_cast<int>(std::ceil(std::abs(to - from) / (M_PI / 8.0))));
            Piece piece;
            for (int j = 0; j < count; ++j) {
                LC_OffsetCubicPiece arc;
                const double s0 = from + (to - from) * j / count;
                const double s1 = from + (to - from) * (j + 1) / count;
                arc.provenance.sourceT0 = s0;
                arc.provenance.sourceT1 = s1;
                arc.provenance.signedDistance = magnitude;
                arc.provenance.arcCentre = nodes[n];
                arc.bezier = arcPiece(nodes[n], magnitude, s0, s1);
                piece.pieces.push_back(arc);
            }
            // its ends are the nodes it was split at, exactly
            piece.pieces.front().bezier[0] = ccw ? at[k].second : at[(k + 1) % at.size()].second;
            piece.pieces.back().bezier[3] = ccw ? at[(k + 1) % at.size()].second : at[k].second;
            for (size_t j = 1; j < piece.pieces.size(); ++j) {
                piece.pieces[j].bezier[0] = piece.pieces[j - 1].bezier[3];
            }
            const double mid = 0.5 * (a0 + a1);
            piece.sample = nodes[n] + RS_Vector{std::cos(mid), std::sin(mid)} * magnitude;
            const double q1 = a0 + 0.25 * (a1 - a0);
            const double q3 = a0 + 0.75 * (a1 - a0);
            piece.alternative[0] = nodes[n] + RS_Vector{std::cos(q1), std::sin(q1)} * magnitude;
            piece.alternative[1] = nodes[n] + RS_Vector{std::cos(q3), std::sin(q3)} * magnitude;
            pieces.push_back(std::move(piece));
        }
    }

    // 7. keep what bounds the result: outside the region (dilate) or inside, and no nearer than |d|
    const double rho = magnitude - (options.tolerance.requestedGeometry + options.tolerance.classification);
    std::vector<size_t> kept;
    size_t removed = 0;
    for (size_t i = 0; i < pieces.size(); ++i) {
        const Piece& piece = pieces[i];
        double size = 0.0;
        for (const LC_OffsetCubicPiece& p : piece.pieces) {
            for (const RS_Vector& v : p.bezier) {
                size = std::max(size, v.distanceTo(piece.pieces.front().bezier[0]));
            }
        }
        if (size <= options.tolerance.nodeMerge) {
            ++removed;
            continue;
        }
        int decision = -1;
        for (const RS_Vector& sample : {piece.sample, piece.alternative[0], piece.alternative[1]}) {
            int winding = 0;
            if (!windingNumber(source, sample, winding, boxes, options.maxDistanceMapBoxes)) {
                continue;
            }
            const bool inside = included(winding, options.fillRule);
            if (inside == dilate) {
                decision = 0; // inside a grown region, or outside a shrunk one
                break;
            }
            const Visibility seen =
                visibility(source, boundaryIntervals, sample, rho, boxes, options.maxDistanceMapBoxes);
            if (seen != Visibility::Undecided) {
                decision = seen == Visibility::Visible ? 1 : 0;
                break;
            }
        }
        if (decision < 0) {
            return boxes > options.maxDistanceMapBoxes ? LC_CurveOffsetStatus::LimitExceeded
                                                       : LC_CurveOffsetStatus::AmbiguousTopology;
        }
        if (decision == 1) {
            kept.push_back(i);
        }
        else {
            ++removed;
        }
    }

    // 8. chain into cycles
    std::vector<bool> used(pieces.size(), false);
    for (const size_t first : kept) {
        if (used[first]) {
            continue;
        }
        LC_OffsetBranch cycle;
        size_t current = first;
        while (true) {
            used[current] = true;
            cycle.cubicPieces.insert(cycle.cubicPieces.end(), pieces[current].pieces.begin(),
                                     pieces[current].pieces.end());
            const RS_Vector end = cycle.cubicPieces.back().bezier[3];
            if (end == cycle.cubicPieces.front().bezier[0]) {
                break;
            }
            size_t next = pieces.size();
            for (const size_t k : kept) {
                if (!used[k] && pieces[k].pieces.front().bezier[0] == end) {
                    if (next != pieces.size()) {
                        return LC_CurveOffsetStatus::AmbiguousTopology; // cycles touching at a point
                    }
                    next = k;
                }
            }
            if (next == pieces.size()) {
                return LC_CurveOffsetStatus::AmbiguousTopology; // a boundary that does not close
            }
            current = next;
        }
        cycle.closed = true;
        cycles.push_back(std::move(cycle));
    }
    result.intersections.clear();
    result.offsetIntersections = found.intersections.size();
    result.removedIntervals = removed;
    return LC_CurveOffsetStatus::Ok;
}

} // namespace

namespace LC_CurveOffset {

bool isSupportedSource(const RS_Entity& source) {
    return dynamic_cast<const RS_Spline*>(&source) != nullptr ||
           dynamic_cast<const LC_SplinePoints*>(&source) != nullptr;
}

LC_CurveOffsetOptions makeDirectOptions(const RS_Entity& source, const double distanceMagnitude,
                                        const double requestedTolerance) {
    const std::unique_ptr<OffsetSource> adapter = makeSource(source);
    SourceScale scale;
    if (adapter == nullptr || !isPositiveFinite(distanceMagnitude) ||
        !computeScale(*adapter, distanceMagnitude, scale) || adapter->breaks().size() < 2) {
        return {};
    }
    const double requested = (requestedTolerance > 0.0)
                                 ? requestedTolerance
                                 : std::max(kDefaultRelativeOffsetTolerance * scale.feature, scale.numericFloor);
    if (!std::isfinite(requested) || !(requested > scale.numericFloor)) {
        return {}; // no tolerance the arithmetic can honour at this scale
    }
    LC_CurveOffsetOptions options;
    LC_CurveOffsetTolerances& t = options.tolerance;
    t.requestedGeometry = requested;
    t.evaluation = std::max(8.0 * g_eps * scale.coordinate, std::min(requested / 8.0, requested * 0.05));
    t.nodeMerge = std::min(requested * 0.10, requested - t.evaluation);
    t.fit = requested - t.evaluation - t.nodeMerge;
    t.classification = std::max(t.evaluation + t.nodeMerge, requested * 0.25);
    t.rootResidual = t.evaluation;
    const std::vector<double>& breaks = adapter->breaks();
    const double t0 = breaks.front();
    const double t1 = breaks.back();
    t.parameter = 16.0 * g_eps * std::max({std::abs(t0), std::abs(t1), t1 - t0});
    if (!isPositiveFinite(t.fit) || !isPositiveFinite(t.nodeMerge) || !isPositiveFinite(t.parameter)) {
        return {};
    }
    options.angleTolerance = kDefaultOffsetAngleTolerance;
    options.maxSubdivisionDepth = kDefaultMaxSubdivisionDepth;
    options.maxSamples = kDefaultMaxSamples;
    options.maxOutputBranches = kDefaultMaxOutputBranches;
    // not used by Direct, but part of a complete options object
    options.maxIntersectionPairs = std::size_t{1} << 20;
    options.maxDistanceMapBoxes = 65536;
    options.maxArrangementEdges = 65536;
    options.maxArrangementFaces = 65536;
    return options;
}

LC_CurveOffsetRequest makeDirectionRequest(const RS_Vector& directionPoint, const double distanceMagnitude) {
    LC_CurveOffsetRequest request;
    request.side = LC_CurveOffsetSide::FromDirectionPoint;
    request.directionPoint = directionPoint;
    request.distanceMagnitude = distanceMagnitude;
    return request;
}

LC_CurveOffsetRequest makeSideRequest(const LC_CurveOffsetSide side, const double distanceMagnitude) {
    LC_CurveOffsetRequest request;
    request.side = side;
    request.distanceMagnitude = distanceMagnitude;
    return request;
}

LC_OffsetSideResolution resolveSide(const RS_Entity& source, const RS_Vector& directionPoint,
                                    const LC_CurveOffsetOptions& options) {
    LC_OffsetSideResolution result;
    if (!validDirectOptions(options)) {
        result.status = LC_CurveOffsetStatus::InvalidRequest;
        return result;
    }
    const std::unique_ptr<OffsetSource> adapter = makeSource(source);
    if (adapter == nullptr) {
        return result;
    }
    return resolveSideImpl(*adapter, directionPoint, options.tolerance);
}

LC_CurveOffsetGeometryResult buildDirectBranches(const RS_Entity& source, const LC_CurveOffsetRequest& request,
                                                 const LC_CurveOffsetOptions& options,
                                                 const LC_OffsetSourceBudget& budget) {
    LC_CurveOffsetGeometryResult result;
    if (!validDirectOptions(options) || !isValidOffsetBudget(budget)) {
        result.status = LC_CurveOffsetStatus::InvalidRequest;
        return result;
    }
    if (!isPositiveFinite(request.distanceMagnitude)) {
        result.status = LC_CurveOffsetStatus::InvalidDistance;
        return result;
    }
    if (request.side == LC_CurveOffsetSide::FromDirectionPoint && !isFinite(request.directionPoint)) {
        result.status = LC_CurveOffsetStatus::InvalidRequest;
        return result;
    }

    const std::unique_ptr<OffsetSource> adapter = makeSource(source);
    SourceScale scale;
    if (adapter == nullptr || adapter->breaks().size() < 2 ||
        !computeScale(*adapter, request.distanceMagnitude, scale)) {
        result.status = LC_CurveOffsetStatus::InvalidSource;
        return result;
    }
    if (!isPlanar(*adapter, scale.numericFloor)) {
        result.status = LC_CurveOffsetStatus::UnsupportedNonPlanar;
        return result;
    }

    if (options.mode == LC_CurveOffsetMode::RegionBoundary) {
        if (!adapter->closed()) {
            result.status = LC_CurveOffsetStatus::InvalidSource;
            return result;
        }
        // grow from a point outside the region, shrink from one inside
        bool dilate = request.side == LC_CurveOffsetSide::Right;
        if (request.side == LC_CurveOffsetSide::FromDirectionPoint) {
            int winding = 0;
            std::size_t boxes = 0;
            if (!windingNumber(*adapter, request.directionPoint, winding, boxes, options.maxDistanceMapBoxes)) {
                result.status = LC_CurveOffsetStatus::AmbiguousSide;
                return result;
            }
            dilate = !included(winding, options.fillRule);
        }
        result.signedDistance = dilate ? request.distanceMagnitude : -request.distanceMagnitude;
        std::vector<LC_OffsetBranch> cycles;
        const LC_CurveOffsetStatus status = regionBoundary(*adapter, scale, request.distanceMagnitude, dilate,
                                                           options, budget, cycles, result);
        size_t pieces = 0;
        for (const LC_OffsetBranch& cycle : cycles) {
            pieces += cycle.cubicPieces.size();
        }
        if (status != LC_CurveOffsetStatus::Ok || pieces > budget.maxCubicPieces ||
            cycles.size() > options.maxOutputBranches) {
            result.status = (status != LC_CurveOffsetStatus::Ok) ? status : LC_CurveOffsetStatus::LimitExceeded;
            return result;
        }
        result.branches = std::move(cycles);
        result.validationLevel = LC_OffsetValidationLevel::None;
        result.status = LC_CurveOffsetStatus::Ok;
        return result;
    }

    LC_CurveOffsetSide side = request.side;
    if (side == LC_CurveOffsetSide::FromDirectionPoint) {
        const LC_OffsetSideResolution resolution =
            resolveSideImpl(*adapter, request.directionPoint, options.tolerance);
        if (resolution.status != LC_CurveOffsetStatus::Ok) {
            result.status = resolution.status;
            return result;
        }
        side = resolution.side;
    }
    const double d = (side == LC_CurveOffsetSide::Left) ? request.distanceMagnitude : -request.distanceMagnitude;
    result.signedDistance = d;

    const std::vector<double>& breaks = adapter->breaks();
    const double speedFloor = scale.numericFloor / (breaks.back() - breaks.front());
    std::vector<LC_OffsetBranch> branches;
    double maxError = 0.0;
    LC_CurveOffsetStatus status =
        directBranches(*adapter, scale, d, options, budget, branches, maxError, result.exactSamples);
    const bool trimmed = options.mode == LC_CurveOffsetMode::Trimmed;
    if (status == LC_CurveOffsetStatus::Ok && (options.nodeIntersections || trimmed) && !branches.front().straight) {
        status = nodeBranches(*adapter, d, speedFloor, options, branches, result);
    }
    if (status == LC_CurveOffsetStatus::Ok && trimmed) {
        status = trimBranches(*adapter, scale, d, options, budget, branches, result);
    }
    if (status != LC_CurveOffsetStatus::Ok) {
        result.status = status;
        return result;
    }
    result.maxObservedError = maxError;
    size_t pieces = 0;
    for (const LC_OffsetBranch& branch : branches) {
        pieces += branch.cubicPieces.size();
    }
    if (pieces > budget.maxCubicPieces) {
        result.status = LC_CurveOffsetStatus::LimitExceeded;
        return result;
    }
    if (branches.size() > options.maxOutputBranches) {
        result.status = LC_CurveOffsetStatus::LimitExceeded;
        return result;
    }
    result.branches = std::move(branches);
    // the pieces were compared with the offset only at their own parameters;
    // materialization adds the independent, two-way check
    result.validationLevel = LC_OffsetValidationLevel::None;
    result.status = LC_CurveOffsetStatus::Ok;
    return result;
}


LC_CurveOffsetMaterializationResult materializeBranches(const RS_Entity& source,
                                                        const LC_CurveOffsetGeometryResult& geometry,
                                                        const LC_CurveOffsetOptions& options,
                                                        const LC_OffsetSourceBudget& budget) {
    LC_CurveOffsetMaterializationResult result;
    if (!validDirectOptions(options) || !isValidOffsetBudget(budget)) {
        result.status = LC_CurveOffsetStatus::InvalidRequest;
        return result;
    }
    if (geometry.status == LC_CurveOffsetStatus::Ok && geometry.branches.empty() &&
        options.mode != LC_CurveOffsetMode::Direct && std::isfinite(geometry.signedDistance)) {
        result.status = LC_CurveOffsetStatus::Ok; // trimmed or shrunk away entirely
        result.validationLevel = LC_OffsetValidationLevel::SampledBidirectional;
        result.maxObservedError = 0.0;
        return result;
    }
    if (geometry.status != LC_CurveOffsetStatus::Ok || geometry.branches.empty() ||
        !std::isfinite(geometry.signedDistance)) {
        result.status = (geometry.status != LC_CurveOffsetStatus::Ok) ? geometry.status
                                                                      : LC_CurveOffsetStatus::InvalidRequest;
        return result;
    }
    if (geometry.branches.size() > options.maxOutputBranches) {
        result.status = LC_CurveOffsetStatus::LimitExceeded;
        return result;
    }
    const std::unique_ptr<OffsetSource> adapter = makeSource(source);
    SourceScale scale;
    if (adapter == nullptr || adapter->breaks().size() < 2 ||
        !computeScale(*adapter, std::abs(geometry.signedDistance), scale)) {
        result.status = LC_CurveOffsetStatus::InvalidSource;
        return result;
    }
    const double domain = adapter->breaks().back() - adapter->breaks().front();

    // Everything is built in local ownership and handed over only complete.
    std::vector<std::unique_ptr<RS_Entity>> entities;
    std::vector<size_t> branchStarts;
    LC_OffsetOutputUsage usage;
    double maxError = 0.0;
    for (const LC_OffsetBranch& branch : geometry.branches) {
        branchStarts.push_back(entities.size());
        LC_OffsetSourceBudget remaining = budget;
        remaining.maxCubicPieces -= std::min(remaining.maxCubicPieces, usage.cubicPieces);
        remaining.maxOutputEntities -= std::min(remaining.maxOutputEntities, usage.outputEntities);
        remaining.maxDeepEntities -= std::min(remaining.maxDeepEntities, usage.deepEntities);
        if (!isValidOffsetBudget(remaining)) {
            result.status = LC_CurveOffsetStatus::LimitExceeded;
            return result;
        }
        Materializer materializer{*adapter, geometry.signedDistance, options, remaining,
                                  scale.numericFloor / domain, geometry.exactSamples};
        LC_OffsetOutputUsage branchUsage;
        double branchError = 0.0;
        const LC_CurveOffsetStatus status = materializer.run(branch, entities, branchUsage, branchError);
        if (status != LC_CurveOffsetStatus::Ok) {
            result.status = status;
            return result; // entities destroyed with this scope
        }
        usage.cubicPieces += branchUsage.cubicPieces;
        usage.outputEntities += branchUsage.outputEntities;
        usage.deepEntities += branchUsage.deepEntities;
        maxError = std::max(maxError, branchError);
    }

    if (options.nodeIntersections || options.mode != LC_CurveOffsetMode::Direct) {
        // The fitted pieces, not only the exact offset, may cross only at its
        // nodes: a fit within tolerance can still add a crossing.
        std::vector<std::vector<std::array<RS_Vector, 4>>> pieces(geometry.branches.size());
        for (size_t i = 0; i < entities.size(); ++i) {
            const size_t branch =
                static_cast<size_t>(std::upper_bound(branchStarts.begin(), branchStarts.end(), i) -
                                    branchStarts.begin()) - 1;
            if (const auto* spline = dynamic_cast<const RS_Spline*>(entities[i].get())) {
                const std::vector<RS_Vector>& c = spline->getData().controlPoints;
                pieces[branch].push_back({c[0], c[1], c[2], c[3]});
            }
            else {
                const RS_Vector a = entities[i]->getStartpoint();
                const RS_Vector b = entities[i]->getEndpoint();
                pieces[branch].push_back({a, a + (b - a) / 3.0, b - (b - a) / 3.0, b});
            }
        }
        const PieceCurves curves{pieces, branchSuccessors(geometry.branches, adapter->closed())};
        LC_IntersectionOptions query;
        query.tolerance = options.tolerance.nodeMerge;
        query.maxBoxPairs = options.maxIntersectionPairs;
        const LC_IntersectionResult found = findIntersections(curves, query);
        if (found.status != LC_IntersectionStatus::Ok) {
            result.status = fromIntersectionStatus(found.status);
            return result;
        }
        for (const LC_ParametricIntersection& x : found.intersections) {
            const bool atNode = std::any_of(
                geometry.intersections.begin(), geometry.intersections.end(),
                [&](const LC_ParametricIntersection& node) {
                    return node.point.distanceTo(x.point) <= options.tolerance.requestedGeometry;
                });
            if (!atNode) {
                result.status = LC_CurveOffsetStatus::AmbiguousTopology;
                return result;
            }
        }
    }

    // a new entity, not a clone: only the drawing attributes come from the source
    for (const std::unique_ptr<RS_Entity>& entity : entities) {
        entity->setPen(source.getPen(false));
        entity->setLayer(source.getLayer());
    }
    result.entities = std::move(entities);
    result.usage = usage;
    result.validationLevel = LC_OffsetValidationLevel::SampledBidirectional;
    result.maxObservedError = maxError;
    result.status = LC_CurveOffsetStatus::Ok;
    return result;
}

LC_CurveOffsetMaterializationResult createEntities(const RS_Entity& source, const LC_CurveOffsetRequest& request,
                                                   const LC_CurveOffsetOptions& options,
                                                   const LC_OffsetSourceBudget& budget) {
    const LC_CurveOffsetGeometryResult geometry = buildDirectBranches(source, request, options, budget);
    if (geometry.status != LC_CurveOffsetStatus::Ok) {
        LC_CurveOffsetMaterializationResult result;
        result.status = geometry.status;
        return result;
    }
    return materializeBranches(source, geometry, options, budget);
}

std::vector<RS_Entity*> createLegacyOffset(const RS_Entity& source, const RS_Vector& coord, const double distance) {
    const double magnitude = std::abs(distance);
    LC_CurveOffsetMaterializationResult result =
        createEntities(source, makeDirectionRequest(coord, magnitude), makeDirectOptions(source, magnitude),
                       makeDirectSourceBudget());
    std::vector<RS_Entity*> entities;
    if (result.status == LC_CurveOffsetStatus::Ok) {
        for (std::unique_ptr<RS_Entity>& entity : result.entities) {
            entities.push_back(entity.release());
        }
    }
    return entities;
}

} // namespace LC_CurveOffset
