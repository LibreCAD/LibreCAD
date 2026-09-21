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
    return o.mode == LC_CurveOffsetMode::Direct && isPositiveFinite(t.requestedGeometry) &&
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

    LC_CurveOffsetStatus build(LC_OffsetBranch& branch) {
        const std::vector<double>& breaks = m_source.breaks();
        for (size_t span = 0; span + 1 < breaks.size(); ++span) {
            const LC_CurveOffsetStatus status = fitSpan(span, breaks[span], breaks[span + 1], branch);
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
        }
        if (branch.cubicPieces.empty()) {
            return LC_CurveOffsetStatus::InvalidSource;
        }
        if (m_source.closed()) {
            auto& last = branch.cubicPieces.back().bezier[3];
            const RS_Vector& first = branch.cubicPieces.front().bezier[0];
            if (last.distanceTo(first) > m_options.tolerance.nodeMerge) {
                return LC_CurveOffsetStatus::DiscontinuousNormal;
            }
            last = first;
            branch.closed = true;
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

            // regularity: the tangent and 1 - d kappa provably away from zero
            LC_CurveJetBounds bounds;
            const bool bounded = m_source.boundJet(leaf.t0, leaf.t1, bounds);
            const bool speedProved = bounded && bounds.speedSquared().isPositive();
            const bool factorProved = bounded && bounds.offsetFactorNumerator(m_d).excludesZero();
            if (!speedProved || !factorProved) {
                if (leaf.depth >= m_options.maxSubdivisionDepth) {
                    return irregularity(leaf, speedProved, a, b);
                }
                split(stack, leaf, 0.5);
                continue;
            }

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

            if (branch.cubicPieces.size() >= m_budget.maxCubicPieces ||
                branch.cubicPieces.size() >= m_budget.maxOutputEntities) {
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
};

/**
 * The source's joins, and for a closed source its seam, must have a single
 * normal: the curve continuous, its tangent direction continuous within the
 * angle tolerance, and the offset gap that angle opens within the merge budget.
 */
LC_CurveOffsetStatus checkJoins(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options) {
    const std::vector<double>& breaks = source.breaks();
    auto checkJoin = [&](const double left, const double right, const bool seam) {
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
        return LC_CurveOffsetStatus::Ok;
    };
    for (size_t i = 1; i + 1 < breaks.size(); ++i) {
        const LC_CurveOffsetStatus status = checkJoin(breaks[i], breaks[i], false);
        if (status != LC_CurveOffsetStatus::Ok) {
            return status;
        }
    }
    if (source.closed()) {
        return checkJoin(breaks.back(), breaks.front(), true);
    }
    return LC_CurveOffsetStatus::Ok;
}

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

    LC_CurveOffsetStatus jet(const double t, const LC_CurveEvaluationSide side, OffsetJet& out) {
        if (++m_samples > m_options.maxSamples) {
            return LC_CurveOffsetStatus::LimitExceeded;
        }
        return computeOffsetJet(m_source, m_d, m_speedFloor, t, side, out);
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
        const double h = p.sourceT1 - p.sourceT0;
        double nodes[11] = {0.0, 1.0};
        for (int k = 1; k <= 9; ++k) {
            const double frac = std::fmod(0.3183098861837907 + k * 0.4142135623730951, 1.0);
            nodes[k + 1] = 0.02 + 0.96 * frac;
        }
        w.error = 0.0;
        w.worstU = 0.5;
        for (const double u : nodes) {
            const LC_CurveEvaluationSide side = (u == 0.0)   ? LC_CurveEvaluationSide::Right
                                                : (u == 1.0) ? LC_CurveEvaluationSide::Left
                                                             : LC_CurveEvaluationSide::Interior;
            RS_Vector candidate;
            if (!evaluateEntity(*w.entity, u, side, candidate)) {
                return LC_CurveOffsetStatus::FitFailed;
            }
            const double t = (u == 0.0) ? p.sourceT0 : (u == 1.0) ? p.sourceT1 : p.sourceT0 + u * h;
            OffsetJet exact;
            LC_CurveOffsetStatus status = jet(t, side, exact);
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
                const double next = std::clamp(tr - step, p.sourceT0, p.sourceT1);
                if (std::abs(next - tr) <= m_options.tolerance.parameter) {
                    break;
                }
                tr = next;
                status = jet(tr, LC_CurveEvaluationSide::Interior, q);
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

    /** Replaces a failed piece by two refitted halves split at its worst sample;
     *  its ends stay where the neighbours share them. */
    LC_CurveOffsetStatus split(std::vector<Work>& work, const size_t index) {
        const Work& w = work[index];
        const LC_OffsetBranchProvenance& p = w.piece.provenance;
        const double s = std::clamp(w.worstU, 0.125, 0.875);
        const double tm = p.sourceT0 + s * (p.sourceT1 - p.sourceT0);
        OffsetJet q0;
        OffsetJet qm;
        OffsetJet q1;
        LC_CurveOffsetStatus status = jet(p.sourceT0, LC_CurveEvaluationSide::Right, q0);
        if (status == LC_CurveOffsetStatus::Ok) {
            status = jet(tm, LC_CurveEvaluationSide::Interior, qm);
        }
        if (status == LC_CurveOffsetStatus::Ok) {
            status = jet(p.sourceT1, LC_CurveEvaluationSide::Left, q1);
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
    options.maxIntersectionPairs = 65536;
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
    const double domain = breaks.back() - breaks.front();
    LC_OffsetBranch branch;

    RS_Vector start;
    RS_Vector end;
    if (adapter->straightSegment(start, end)) {
        // exact: a straight segment moved along its constant normal
        const RS_Vector direction = end - start;
        const double length = direction.magnitude();
        if (!(length > scale.numericFloor)) {
            result.status = LC_CurveOffsetStatus::UndefinedTangent;
            return result;
        }
        const RS_Vector shift = RS_Vector{-direction.y, direction.x} * (d / length);
        const RS_Vector a = start + shift;
        const RS_Vector b = end + shift;
        LC_OffsetCubicPiece piece;
        piece.provenance = {0, breaks.front(), breaks.back(), d, true};
        piece.bezier = {a, a + (b - a) / 3.0, b - (b - a) / 3.0, b};
        for (RS_Vector& v : piece.bezier) {
            v.z = 0.0;
        }
        branch.cubicPieces.push_back(piece);
        branch.straight = true;
        result.maxObservedError = 0.0;
    }
    else {
        const LC_CurveOffsetStatus joins = checkJoins(*adapter, d, options);
        if (joins != LC_CurveOffsetStatus::Ok) {
            result.status = joins;
            return result;
        }
        BranchBuilder builder{*adapter, d, options, budget, scale.numericFloor / domain};
        const LC_CurveOffsetStatus status = builder.build(branch);
        result.exactSamples = builder.samples();
        if (status != LC_CurveOffsetStatus::Ok) {
            result.status = status;
            return result;
        }
        result.maxObservedError = builder.maxObservedError();
    }
    if (branch.cubicPieces.size() > budget.maxCubicPieces) {
        result.status = LC_CurveOffsetStatus::LimitExceeded;
        return result;
    }
    result.branches.push_back(std::move(branch));
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
    LC_OffsetOutputUsage usage;
    double maxError = 0.0;
    for (const LC_OffsetBranch& branch : geometry.branches) {
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

} // namespace LC_CurveOffset
