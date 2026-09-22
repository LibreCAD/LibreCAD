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
#include <tuple>
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
    /** Conservative bounds over [a, b], inside one span; see boundJetWithProducts(). */
    virtual bool boundJet(double a, double b, LC_CurveJetBounds& out) const = 0;
    /**
     * boundJet() with the curve's enclosures of |C'|^2 and C' x C'' from the
     * products' Bezier coefficients, where it has them; the entities'
     * adapters leave those out of boundJet(). Only next to a vanishing tangent
     * do the components fail to resolve the curvature's sign; elsewhere the
     * products change which near-tangent contacts the intersection query
     * resolves, and so how a regular source is trimmed, without proving
     * anything the components cannot.
     */
    virtual bool boundJetWithProducts(const double a, const double b, LC_CurveJetBounds& out) const {
        return boundJet(a, b, out);
    }
    /** Points whose convex hull contains the curve. */
    virtual const std::vector<RS_Vector>& hull() const = 0;
    /** True, with its ends, when the curve is exactly one straight segment. */
    virtual bool straightSegment(RS_Vector& start, RS_Vector& end) const = 0;
    /**
     * True when every span is a polynomial of degree at most 3, so that its
     * cubic Bezier net follows from the jets at its ends.
     */
    virtual bool cubicSpans() const {
        return false;
    }
};

/** @p bounded, with @p out's product enclosures dropped (see OffsetSource::boundJetWithProducts()). */
bool withoutProducts(const bool bounded, LC_CurveJetBounds& out) {
    out.speedSquaredProduct = LC_Interval{};
    out.crossProduct = LC_Interval{};
    return bounded;
}

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
        return withoutProducts(m_spline.tryBoundJet(a, b, out), out);
    }

    bool boundJetWithProducts(const double a, const double b, LC_CurveJetBounds& out) const override {
        return m_spline.tryBoundJet(a, b, out);
    }

    const std::vector<RS_Vector>& hull() const override {
        return m_hull;
    }

    bool straightSegment(RS_Vector& start, RS_Vector& end) const override {
        // equal weights: unequal ones keep the line but not its linear parameter,
        // which the exact straight branch's provenance assumes
        const RS_SplineData& data = m_spline.getData();
        if (m_spline.getDegree() != 1 || m_spline.isClosed() || data.controlPoints.size() != 2 ||
            data.weights.size() != 2 || data.weights[0] != data.weights[1]) {
            return false;
        }
        start = data.controlPoints[0];
        end = data.controlPoints[1];
        return true;
    }

    bool cubicSpans() const override {
        const std::vector<double>& weights = m_spline.getData().weights;
        return m_spline.getDegree() <= 3 &&
               std::all_of(weights.begin(), weights.end(), [&](const double w) { return w == weights.front(); });
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
        return withoutProducts(m_spline.tryBoundJet(a, b, out), out);
    }

    bool boundJetWithProducts(const double a, const double b, LC_CurveJetBounds& out) const override {
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

    bool cubicSpans() const override {
        return true; // quadratic segments
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
        // A closed curve's first and last samples are the same point, the seam:
        // its neighbours wrap, and it is refined on both sides, after the seam
        // from the first sample and before it from the last.
        const bool first = (k == 0);
        const bool last = (k + 1 == n);
        const double before = first ? (closed ? ds[n - 2] : RS_MAXDOUBLE) : ds[k - 1];
        const double after = last ? (closed ? ds[1] : RS_MAXDOUBLE) : ds[k + 1];
        if (ds[k] > before || ds[k] > after) {
            continue;
        }
        const double lo = first ? ts.front() : ts[k - 1];
        const double hi = last ? ts.back() : ts[k + 1];
        const double t = refineNearest(source, p, ts[k], lo, hi, tolerance.parameter);
        // Refined from one side, a closed seam stops exactly at the seam when the
        // minimum lies across it, which the other side's refinement finds; a seam
        // point that is no minimum can carry the tangent of a sharp turn there.
        if (closed && (first || last) && t == (first ? ts.front() : ts.back())) {
            LC_CurveJet across;
            if (source.jet(first ? ts.back() : ts.front(), LC_CurveEvaluationSide::Interior, across)) {
                const double g = dot(across.point - p, across.first);
                if (first ? g > 0.0 : g < 0.0) {
                    continue;
                }
            }
        }
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
        // Unit tangents: at a break, where the curve may turn a corner, those
        // either side of it, whose normals together point into the wedge of
        // points that have the corner as their nearest.
        RS_Vector before = jet.first / jet.first.magnitude();
        RS_Vector after = before;
        // refined towards a corner, the nearest parameter stops a hair short of it
        const auto at = std::lower_bound(breaks.begin(), breaks.end(), c.t - tolerance.parameter);
        const bool atBreak = at != breaks.end() && std::abs(*at - c.t) <= tolerance.parameter;
        const double corner = atBreak ? *at : c.t;
        if (atBreak && (closed || (corner != breaks.front() && corner != breaks.back()))) {
            LC_CurveJet l;
            LC_CurveJet r;
            const double left = (closed && corner == breaks.front()) ? breaks.back() : corner;
            const double right = (closed && corner == breaks.back()) ? breaks.front() : corner;
            if (source.jet(left, LC_CurveEvaluationSide::Left, l) && source.jet(right, LC_CurveEvaluationSide::Right, r) &&
                dot(l.first, l.first) > 0.0 && dot(r.first, r.first) > 0.0) {
                before = l.first / l.first.magnitude();
                after = r.first / r.first.magnitude();
            }
        }
        const double side = 0.5 * (cross(before, toPoint) + cross(after, toPoint));
        // the side test is as uncertain as the point's position
        if (std::abs(side) <= tolerance.classification) {
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
    result.distance = best;
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

/** How the source goes on at a break: smoothly, or turning a corner. */
struct SourceJoin {
    enum class Kind {
        Smooth,
        /** A corner whose offsets leave a gap or overlap by more than twice the merge tolerance. */
        Kink,
        /** A corner whose offsets meet within twice the merge tolerance: closed at their middle. */
        Snap
    };
    Kind kind{Kind::Smooth};
    RS_Vector point{false};
    /** Unit tangents before and after it. */
    RS_Vector before{false};
    RS_Vector after{false};
};

/**
 * Removes reversed branches too small to keep: a swallowtail between two
 * cusps within twice the merge tolerance, whose neighbours then meet at its
 * middle and run on as one branch; or a tail at an open end within the
 * merge tolerance, whose neighbour then ends where the offset does. Done
 * before noding, so that a crossing of the neighbours inside it is their
 * join, not a node.
 */
void collapseTinyReversals(std::vector<LC_OffsetBranch>& branches, const double merge) {
    const auto extent = [](const LC_OffsetBranch& branch) {
        double e = 0.0;
        const RS_Vector& origin = branch.cubicPieces.front().bezier[0];
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            for (const RS_Vector& v : piece.bezier) {
                e = std::max(e, v.distanceTo(origin));
            }
        }
        return e;
    };
    for (size_t k = 0; k < branches.size() && branches.size() > 1; ++k) {
        const LC_OffsetBranch& r = branches[k];
        if (!r.reversed || r.closed) {
            continue;
        }
        const size_t n = branches.size();
        const bool cuspBefore = r.startEnd == LC_OffsetBranchEnd::Cusp;
        const bool cuspAfter = r.endEnd == LC_OffsetBranchEnd::Cusp;
        const double size = extent(r);
        if (cuspBefore && cuspAfter && size <= 2.0 * merge) {
            const size_t before = (k + n - 1) % n;
            const size_t after = (k + 1) % n;
            const RS_Vector middle = (r.cubicPieces.front().bezier[0] + r.cubicPieces.back().bezier[3]) * 0.5;
            LC_OffsetBranch& a = branches[before];
            if (before == after) {
                // the only other branch closes on itself
                a.cubicPieces.back().bezier[3] = middle;
                a.cubicPieces.front().bezier[0] = middle;
                a.closed = true;
                a.startEnd = LC_OffsetBranchEnd::Free;
                a.endEnd = LC_OffsetBranchEnd::Free;
                branches.erase(branches.begin() + static_cast<std::ptrdiff_t>(k));
                return;
            }
            LC_OffsetBranch& b = branches[after];
            a.cubicPieces.back().bezier[3] = middle;
            b.cubicPieces.front().bezier[0] = middle;
            a.cubicPieces.insert(a.cubicPieces.end(), b.cubicPieces.begin(), b.cubicPieces.end());
            a.endEnd = b.endEnd;
            for (const size_t gone : {std::max(k, after), std::min(k, after)}) {
                branches.erase(branches.begin() + static_cast<std::ptrdiff_t>(gone));
            }
            k = static_cast<size_t>(-1); // indices moved: start over
        }
        else if (!cuspBefore && cuspAfter && size <= merge) {
            branches[k + 1].cubicPieces.front().bezier[0] = r.cubicPieces.front().bezier[0];
            branches[k + 1].startEnd = LC_OffsetBranchEnd::Free;
            branches.erase(branches.begin() + static_cast<std::ptrdiff_t>(k));
            k = static_cast<size_t>(-1);
        }
        else if (cuspBefore && !cuspAfter && size <= merge) {
            branches[k - 1].cubicPieces.back().bezier[3] = r.cubicPieces.back().bezier[3];
            branches[k - 1].endEnd = LC_OffsetBranchEnd::Free;
            branches.erase(branches.begin() + static_cast<std::ptrdiff_t>(k));
            k = static_cast<size_t>(-1);
        }
    }
}

/** How the joins are handled: a kink fails the request, or its corner is rounded. */
enum class KinkPolicy {
    Refuse,
    Round
};

/**
 * The arc of radius |d| about a corner of the source from the offset's @p end
 * before it to its @p start after it, as pieces of at most a quarter turn; or
 * false, and no pieces, where the corner turns towards the offset, whose two
 * sides then overlap. A reversal turns away from either side.
 *
 * The pieces are close enough to the circle that offsetting the result again,
 * back by d, shrinks each of them to a stall at its corner: a coarser arc's
 * curvature swings about 1/|d|, and its offset by |d| would be a tangle, not a
 * point.
 */
bool cornerArc(const SourceJoin& join, const RS_Vector& end, const RS_Vector& start, const double d,
               const LC_CurveOffsetOptions& options, std::vector<LC_OffsetCubicPiece>& arc) {
    arc.clear();
    double turn = std::atan2(cross(join.before, join.after), dot(join.before, join.after));
    if (std::abs(std::abs(turn) - M_PI) <= options.angleTolerance) {
        turn = (d > 0.0) ? -M_PI : M_PI;
    }
    if (turn * d >= 0.0) {
        return false;
    }
    const double a0 = std::atan2(end.y - join.point.y, end.x - join.point.x);
    // A cubic arc of sweep s lies within e = 2/27 r sin^6(s/4) / cos^2(s/4) of
    // its circle, and its own offset by r within 7.45 e / s of the centre:
    // a sixteenth of what BranchBuilder::arcStall() allows.
    const auto strays = [&](const size_t pieces) {
        const double sweep = std::abs(turn) / static_cast<double>(pieces);
        const double quarter = 0.25 * sweep;
        return 8.0 * (2.0 / 27.0) * std::abs(d) * std::pow(std::sin(quarter), 6) / std::pow(std::cos(quarter), 2) /
               sweep;
    };
    constexpr size_t maxPieces = 64;
    size_t count = std::max<size_t>(1, static_cast<size_t>(std::ceil(std::abs(turn) / (0.5 * M_PI) - 1e-9)));
    while (count < maxPieces && strays(count) > options.tolerance.nodeMerge / 32.0) {
        ++count;
    }
    for (size_t k = 0; k < count; ++k) {
        const double from = a0 + turn * static_cast<double>(k) / static_cast<double>(count);
        const double to = (k + 1 == count) ? a0 + turn : a0 + turn * static_cast<double>(k + 1) / static_cast<double>(count);
        LC_OffsetCubicPiece piece;
        piece.provenance = {0, from, to, d, true};
        piece.provenance.arcCentre = join.point;
        piece.bezier = arcPiece(join.point, std::abs(d), from, to);
        if (!arc.empty()) {
            piece.bezier[0] = arc.back().bezier[3];
        }
        arc.push_back(piece);
    }
    arc.front().bezier[0] = end;
    arc.back().bezier[3] = start;
    return true;
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
     * seam runs from the last cusp to the first. Where the offset stalls
     * without turning back, it is not fitted: the piece after starts where the
     * piece before ends.
     */
    LC_CurveOffsetStatus build(std::vector<bool> joinCusps, const std::vector<SourceJoin>& joins,
                               std::vector<LC_OffsetBranch>& branches) {
        const std::vector<double>& breaks = m_source.breaks();
        std::vector<double> cusps;
        std::vector<std::pair<double, double>> stalls;
        const std::vector<bool> arcStalls = findArcStalls(joinCusps);
        // no unproved run continues across a cusp or a corner at a join
        std::vector<bool> barrier = joinCusps;
        for (size_t i = 0; i < barrier.size(); ++i) {
            barrier[i] = barrier[i] || joins[i].kind != SourceJoin::Kind::Smooth;
        }
        LC_CurveOffsetStatus status = isolateCusps(barrier, arcStalls, cusps, stalls);
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
        // what lies between each branch and the next: a cusp, or the join at a break
        std::vector<size_t> after; // a join index, or npos for a cusp
        constexpr size_t cusp = std::numeric_limits<size_t>::max();
        auto next = cusps.begin();
        auto stall = stalls.begin();
        double resume = breaks.front(); // where fitting continues after a stall, which may cross a join
        for (size_t span = 0; span + 1 < breaks.size(); ++span) {
            const double a = breaks[span];
            const double b = breaks[span + 1];
            if (span > 0 && (breakCusps[span] || joins[span].kind != SourceJoin::Kind::Smooth)) {
                after.push_back(joins[span].kind != SourceJoin::Kind::Smooth ? span : cusp);
                branches.emplace_back();
            }
            double from = std::max(a, resume);
            while (true) {
                const bool cuspHere = next != cusps.end() && *next < b;
                const bool stallHere = stall != stalls.end() && stall->first < b;
                if (!cuspHere && !stallHere) {
                    break;
                }
                if (cuspHere && (!stallHere || *next <= stall->first)) {
                    if (*next > from) {
                        status = fitSpan(span, from, *next, branches.back());
                        if (status != LC_CurveOffsetStatus::Ok) {
                            return status;
                        }
                        after.push_back(cusp);
                        branches.emplace_back();
                        from = *next;
                    }
                    ++next;
                    continue;
                }
                if (stall->first > from) {
                    status = fitSpan(span, from, stall->first, branches.back());
                    if (status != LC_CurveOffsetStatus::Ok) {
                        return status;
                    }
                }
                resume = stall->second;
                from = std::max(from, resume);
                ++stall;
            }
            if (from < b) {
                status = fitSpan(span, from, b, branches.back());
                if (status != LC_CurveOffsetStatus::Ok) {
                    return status;
                }
            }
        }
        for (size_t i = 0; i < branches.size(); ++i) {
            if (branches[i].cubicPieces.empty()) {
                return LC_CurveOffsetStatus::InvalidSource;
            }
            if (i > 0 && after[i - 1] == cusp) {
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
            if (joins[0].kind != SourceJoin::Kind::Smooth) {
                after.push_back(0); // a corner at the seam
            }
            else {
                auto& last = branches.back().cubicPieces.back().bezier[3];
                const RS_Vector& first = branches.front().cubicPieces.front().bezier[0];
                if (last.distanceTo(first) > m_options.tolerance.nodeMerge) {
                    return LC_CurveOffsetStatus::DiscontinuousNormal;
                }
                last = first;
                if (breakCusps[0]) {
                    after.push_back(cusp);
                }
                else if (branches.size() == 1) {
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
        // 1 - d kappa keeps its sign between cusps: its widest piece decides,
        // away from the cusps and stalls where it is all but zero
        for (LC_OffsetBranch& branch : branches) {
            const LC_OffsetBranchProvenance& middle =
                std::max_element(branch.cubicPieces.begin(), branch.cubicPieces.end(),
                                 [](const LC_OffsetCubicPiece& l, const LC_OffsetCubicPiece& r) {
                                     return l.provenance.sourceT1 - l.provenance.sourceT0 <
                                            r.provenance.sourceT1 - r.provenance.sourceT0;
                                 })
                    ->provenance;
            const double g =
                factorNumeratorAt(0.5 * (middle.sourceT0 + middle.sourceT1), LC_CurveEvaluationSide::Interior);
            if (!std::isfinite(g) || g == 0.0) {
                return LC_CurveOffsetStatus::AmbiguousRegularity;
            }
            branch.reversed = g < 0.0;
        }
        std::vector<Link> links;
        for (const size_t join : after) {
            links.push_back(join == cusp ? Link{Link::Kind::Cusp, nullptr} : Link{Link::Kind::Join, &joins[join]});
        }
        joinCorners(branches, links);
        // trimming first cuts every swallowtail it can where its neighbours cross
        if (m_options.mode != LC_CurveOffsetMode::Trimmed) {
            collapseTinyReversals(branches, m_options.tolerance.nodeMerge);
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

    /** What lies between a branch and the next: a cusp, or a join of the source. */
    struct Link {
        enum class Kind {
            Cusp,
            Join,
            /** Resolved: consecutive parts that run on as one branch. */
            Through,
            /** Resolved: the overlapping ends of a corner that turns towards the offset. */
            Overlap
        };
        Kind kind;
        const SourceJoin* join;
    };

    /**
     * Resolves the joins between consecutive branches (@p links[i] follows
     * branch i; one more than the branches' gaps for a closed source, the last
     * from the last branch to the first) and sets how each branch ends. A snap
     * closes at its middle; a corner that turns away from the offset gets an
     * arc of radius |d| about it; one that turns towards it leaves its two
     * sides overlapping. Parts that run on in the same direction become one
     * branch: an arc never joins a reversed branch, which turns back into it.
     */
    void joinCorners(std::vector<LC_OffsetBranch>& branches, std::vector<Link>& links) const {
        const bool cyclic = links.size() == branches.size();
        std::vector<LC_OffsetBranch> parts;
        std::vector<Link::Kind> between; // resolved links, between[i] after parts[i]
        const auto through = [](const bool a, const bool b) {
            return a == b ? Link::Kind::Through : Link::Kind::Cusp;
        };
        for (size_t i = 0; i < branches.size(); ++i) {
            parts.push_back(branches[i]);
            if (i >= links.size()) {
                break;
            }
            const Link& link = links[i];
            // a closed source's last link runs into the first branch, already a part
            LC_OffsetBranch& nextBranch = (i + 1 < branches.size()) ? branches[i + 1] : parts.front();
            if (link.kind == Link::Kind::Cusp) {
                // the offset turns back at a cusp: one between two branches running
                // the same way was rounding, and they run on as one
                between.push_back(through(parts.back().reversed, nextBranch.reversed));
                continue;
            }
            const SourceJoin& join = *link.join;
            RS_Vector& end = parts.back().cubicPieces.back().bezier[3];
            RS_Vector& start = nextBranch.cubicPieces.front().bezier[0];
            if (join.kind == SourceJoin::Kind::Snap) {
                const RS_Vector middle = (end + start) * 0.5;
                end = middle;
                start = middle;
                between.push_back(through(parts.back().reversed, nextBranch.reversed));
                continue;
            }
            LC_OffsetBranch arc;
            if (!cornerArc(join, end, start, m_d, m_options, arc.cubicPieces)) {
                between.push_back(Link::Kind::Overlap);
                continue;
            }
            between.push_back(through(parts.back().reversed, false));
            parts.push_back(std::move(arc));
            between.push_back(through(false, nextBranch.reversed));
        }
        if (cyclic && !parts.empty() && std::all_of(between.begin(), between.end(),
                                                    [](const Link::Kind k) { return k == Link::Kind::Through; })) {
            LC_OffsetBranch ring;
            ring.reversed = parts.front().reversed;
            for (LC_OffsetBranch& part : parts) {
                ring.cubicPieces.insert(ring.cubicPieces.end(), part.cubicPieces.begin(), part.cubicPieces.end());
            }
            ring.closed = true;
            branches.assign(1, std::move(ring));
            return;
        }
        // start after a link that parts do not run on through
        const size_t n = parts.size();
        size_t first = 0;
        if (cyclic) {
            while (between[(first + n - 1) % n] == Link::Kind::Through) {
                ++first;
            }
        }
        const auto endOf = [](const Link::Kind k) {
            return k == Link::Kind::Cusp ? LC_OffsetBranchEnd::Cusp : LC_OffsetBranchEnd::Kink;
        };
        branches.clear();
        for (size_t step = 0; step < n; ++step) {
            const size_t i = (first + step) % n;
            const bool continues = step > 0 && between[(i + n - 1) % n] == Link::Kind::Through;
            if (!continues) {
                branches.push_back(std::move(parts[i]));
                LC_OffsetBranch& b = branches.back();
                b.startEnd = (step == 0 && !cyclic) ? LC_OffsetBranchEnd::Free : endOf(between[(i + n - 1) % n]);
            }
            else {
                std::vector<LC_OffsetCubicPiece>& pieces = branches.back().cubicPieces;
                pieces.insert(pieces.end(), parts[i].cubicPieces.begin(), parts[i].cubicPieces.end());
            }
            const bool last = step + 1 == n;
            branches.back().endEnd = (last && !cyclic) ? LC_OffsetBranchEnd::Free : endOf(between[i]);
        }
    }

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
     * True if the offset stalls over the whole span [a, b] because the span is
     * a circular arc of radius |d| on the offset's side, within a distance that
     * keeps its offset within half the merge tolerance of the arc's centre.
     * Proving that box by box would take millions of boxes: 1 - d kappa is all
     * but zero throughout. Instead the span's cubic net, from the jets at its
     * ends, is compared with the cubic that approximates the arc. Their nets
     * eta apart bound the points eta apart and the tangents 6 eta / h apart,
     * against an arc tangent of at least 0.97 r |sweep| / h; the approximating
     * cubic's own offset strays from the centre by 7.45 e / |sweep| at most
     * (measured for sweeps up to a quarter turn; 8 is used), where e is its
     * distance from the circle.
     */
    bool arcStall(const double a, const double b) const {
        if (!m_source.cubicSpans()) {
            return false;
        }
        LC_CurveJet start;
        LC_CurveJet end;
        if (!m_source.jet(a, LC_CurveEvaluationSide::Right, start) ||
            !m_source.jet(b, LC_CurveEvaluationSide::Left, end)) {
            return false;
        }
        const double startSpeed = start.first.magnitude();
        const double endSpeed = end.first.magnitude();
        if (!(startSpeed > m_speedFloor) || !(endSpeed > m_speedFloor)) {
            return false;
        }
        const double merge = m_options.tolerance.nodeMerge;
        const RS_Vector c0 = start.point + RS_Vector{-start.first.y, start.first.x} * (m_d / startSpeed);
        const RS_Vector c1 = end.point + RS_Vector{-end.first.y, end.first.x} * (m_d / endSpeed);
        if (!(c0.distanceTo(c1) <= 0.25 * merge)) {
            return false;
        }
        const RS_Vector centre = (c0 + c1) * 0.5;
        const double r = std::abs(m_d);
        const double a0 = std::atan2(start.point.y - centre.y, start.point.x - centre.x);
        // around the centre the way the curve turns: anticlockwise for an offset to the left
        double sweep = std::remainder(std::atan2(end.point.y - centre.y, end.point.x - centre.x) - a0, 2.0 * M_PI);
        if ((m_d > 0.0) != (sweep > 0.0) || std::abs(sweep) > 0.5 * M_PI) {
            return false;
        }
        const double h = b - a;
        const std::array<RS_Vector, 4> net{start.point, start.point + start.first * (h / 3.0),
                                           end.point - end.first * (h / 3.0), end.point};
        const std::array<RS_Vector, 4> ideal = arcPiece(centre, r, a0, a0 + sweep);
        double eta = 0.0;
        for (size_t k = 0; k < net.size(); ++k) {
            eta = std::max(eta, net[k].distanceTo(ideal[k]));
        }
        const double slowest = 0.97 * r * std::abs(sweep);
        if (!(6.0 * eta < slowest)) {
            return false;
        }
        const double normals = 0.5 * M_PI * 6.0 * eta / slowest; // the largest angle between them
        const double quarter = 0.25 * std::abs(sweep);
        const double own = 8.0 * (2.0 / 27.0) * r * std::pow(std::sin(quarter), 6) / std::pow(std::cos(quarter), 2) /
                           std::abs(sweep);
        return eta + r * normals + own <= 0.5 * merge;
    }

    /**
     * The spans over which the offset stalls on an arc (arcStall()), by span
     * index. The join cusps inside a run of them are dropped: the offset turns
     * back within it, if at all, by what the signs of 1 - d kappa on either
     * side say, and then at the run's start. A run at the seam of a closed
     * source, or one that is all of it, is left to the boxes.
     */
    std::vector<bool> findArcStalls(std::vector<bool>& joinCusps) const {
        const std::vector<double>& breaks = m_source.breaks();
        const size_t spans = breaks.size() - 1;
        std::vector<bool> stalled(spans, false);
        for (size_t span = 0; span < spans; ++span) {
            stalled[span] = arcStall(breaks[span], breaks[span + 1]);
        }
        for (size_t first = 0; first < spans;) {
            if (!stalled[first]) {
                ++first;
                continue;
            }
            size_t last = first;
            while (last + 1 < spans && stalled[last + 1]) {
                ++last;
            }
            const bool atSeam = m_source.closed() && (first == 0 || last + 1 == spans);
            if (atSeam || (first == 0 && last + 1 == spans)) {
                std::fill(stalled.begin() + static_cast<std::ptrdiff_t>(first),
                          stalled.begin() + static_cast<std::ptrdiff_t>(last + 1), false);
                first = last + 1;
                continue;
            }
            for (size_t join = first + 1; join <= last; ++join) {
                joinCusps[join] = false;
            }
            bool turnsBack = false;
            if (first > 0 && last + 1 < spans) {
                const double before = factorNumeratorAt(breaks[first], LC_CurveEvaluationSide::Left);
                const double after = factorNumeratorAt(breaks[last + 1], LC_CurveEvaluationSide::Right);
                turnsBack = std::isfinite(before) && std::isfinite(after) && before != 0.0 && after != 0.0 &&
                            std::signbit(before) != std::signbit(after);
            }
            if (first > 0) {
                joinCusps[first] = turnsBack;
            }
            if (last + 1 < spans) {
                joinCusps[last + 1] = false;
            }
            first = last + 1;
        }
        return stalled;
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
     * The cusps of the offset away from the joins, in source order, and the
     * stalls: parameter intervals over which the offset moves less than the
     * merge tolerance and does not turn back. Every span is proved box by box
     * to have a nonvanishing tangent and 1 - d kappa away from zero, except for
     * runs of boxes too small to split. A run continues across a join where
     * 1 - d kappa keeps its sign, and across the seam of a closed source, since
     * a root next to either leaves unproved boxes on both sides. Over a run
     * the offset must move less than the fit tolerance; each sign change of
     * 1 - d kappa found by sampling the run is a cusp, found by bisection. A
     * run with no sign change, or an even number of them, is a stall if the
     * offset moves less than the merge tolerance over it: 1 - d kappa touches
     * zero there (the distance equals a radius of curvature), or two cusps lie
     * closer than the tolerance. Any other run is a singularity: a vanishing
     * tangent, or roots the samples cannot tell apart. The spans marked in
     * @p arcStalls are stalls as they are.
     */
    LC_CurveOffsetStatus isolateCusps(const std::vector<bool>& joinCusps, const std::vector<bool>& arcStalls,
                                      std::vector<double>& cusps,
                                      std::vector<std::pair<double, double>>& stalls) {
        // Interval bounds of 1 - d kappa widen with the box, so near a double
        // root, where the distance equals a radius of curvature, the unproved
        // run grows like the inverse square root of the finest box: about a
        // thousand boxes at the default depth, depending on the curve's
        // orientation. The cap only stops a span that is unproved throughout.
        constexpr unsigned maxRunBoxes = 16384;
        const std::vector<double>& breaks = m_source.breaks();
        std::vector<Run> runs;
        for (size_t span = 0; span + 1 < breaks.size(); ++span) {
            const double a = breaks[span];
            const double b = breaks[span + 1];
            if (arcStalls[span]) {
                stalls.emplace_back(a, b);
                continue;
            }
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
            // a run across the seam: from the start of the last run to the end of the first
            const bool acrossSeam = wraps && i + 1 == runs.size();
            const std::vector<std::pair<double, double>> parts =
                acrossSeam ? std::vector<std::pair<double, double>>{{run.t0, breaks.back()},
                                                                    {runs.front().t0, runs.front().t1}}
                           : std::vector<std::pair<double, double>>{{run.t0, run.t1}};
            const double extent = acrossSeam ? run.extent + runs.front().extent : run.extent;
            const LC_CurveOffsetStatus status = resolveRun(parts, run, extent, cusps, stalls);
            if (status != LC_CurveOffsetStatus::Ok) {
                return status;
            }
        }
        std::sort(cusps.begin(), cusps.end());
        std::sort(stalls.begin(), stalls.end());
        return LC_CurveOffsetStatus::Ok;
    }

    /**
     * The cusps in a run, given as one interval or, across a closed source's
     * seam, as two, from samples of 1 - d kappa at a few points per box; or the
     * run as a stall, if the samples change sign an even number of times and
     * the offset moves less than the merge tolerance over it (@p extent bounds
     * that motion).
     */
    LC_CurveOffsetStatus resolveRun(const std::vector<std::pair<double, double>>& parts, const Run& run,
                                    const double extent, std::vector<double>& cusps,
                                    std::vector<std::pair<double, double>>& stalls) {
        const std::vector<double>& breaks = m_source.breaks();
        const int count = static_cast<int>(std::min(256u, 4u * run.boxes + 4u));
        std::vector<double> found;
        std::size_t changes = 0;
        for (const auto& [a, b] : parts) {
            // the last sample with a nonzero value, and the first zero after it
            double lastT = a;
            double lastG = 0.0;
            double zeroT = std::numeric_limits<double>::quiet_NaN();
            for (int k = 0; k <= count; ++k) {
                const double t = (k == count) ? b : a + (b - a) * k / count;
                const LC_CurveEvaluationSide side = (k == 0)       ? LC_CurveEvaluationSide::Right
                                                    : (k == count) ? LC_CurveEvaluationSide::Left
                                                                   : LC_CurveEvaluationSide::Interior;
                const double g = factorNumeratorAt(t, side);
                if (!std::isfinite(g)) {
                    return LC_CurveOffsetStatus::AmbiguousRegularity;
                }
                if (g == 0.0) {
                    if (std::isnan(zeroT)) {
                        zeroT = t;
                    }
                    continue;
                }
                if (lastG != 0.0 && std::signbit(g) != std::signbit(lastG)) {
                    ++changes;
                    if (!std::isnan(zeroT)) {
                        found.push_back(zeroT); // an exact root among the samples
                    }
                    else {
                        const LC_CurveOffsetStatus status = bisectCusp(lastT, t, run, found);
                        if (status != LC_CurveOffsetStatus::Ok) {
                            return status;
                        }
                    }
                }
                lastT = t;
                lastG = g;
                zeroT = std::numeric_limits<double>::quiet_NaN();
            }
        }
        const bool stalled = extent <= m_options.tolerance.nodeMerge;
        // an odd count turns the offset back; an even one within the merge
        // tolerance is a swallowtail too small to keep, and none a touch
        if (changes % 2 == 1 || (changes > 0 && !stalled)) {
            cusps.insert(cusps.end(), found.begin(), found.end());
            return LC_CurveOffsetStatus::Ok;
        }
        if (!stalled) {
            return irregularity(run.last, true, breaks[run.span], breaks[run.span + 1]);
        }
        stalls.insert(stalls.end(), parts.begin(), parts.end());
        return LC_CurveOffsetStatus::Ok;
    }

    /** The root of 1 - d kappa in [lo, hi], a part of a run; it must change sign between them. */
    LC_CurveOffsetStatus bisectCusp(double lo, double hi, const Run& run, std::vector<double>& cusps) {
        const std::vector<double>& breaks = m_source.breaks();
        const double g0 = factorNumeratorAt(lo, LC_CurveEvaluationSide::Right);
        const double g1 = factorNumeratorAt(hi, LC_CurveEvaluationSide::Left);
        if (!(std::isfinite(g0) && std::isfinite(g1) && g0 != 0.0 && g1 != 0.0) ||
            std::signbit(g0) == std::signbit(g1)) {
            return irregularity(run.last, true, breaks[run.span], breaks[run.span + 1]);
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
     * large world coordinates. Its tangent is compared too, unless
     * @p positionOnly, weighted by how far the offset moves over the piece at
     * that rate against the fit tolerance: where the offset all but stops, near
     * a cusp or a stall, the exact tangent is mostly cancellation, and a piece
     * shorter than the fit tolerance has a tangent no subdivision can resolve.
     */
    Check check(const Leaf& leaf, const std::array<RS_Vector, 4>& local, const RS_Vector& origin,
                const double* nodes, const size_t count, const bool positionOnly) {
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
            const double motion = exact.first.magnitude() * h / m_options.tolerance.fit;
            const double angle = positionOnly ? 0.0
                                              : angleBetweenVectors(bezierDerivative(local, s), exact.first) *
                                                    std::min(1.0, motion);
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

            const bool tiny = local[0].distanceTo(local[1]) + local[1].distanceTo(local[2]) +
                                  local[2].distanceTo(local[3]) <= m_options.tolerance.fit;
            Check fit = check(leaf, local, origin, splitNodes, std::size(splitNodes), tiny);
            if (fit.status != LC_CurveOffsetStatus::Ok) {
                return fit.status;
            }
            if (!fit.failed) {
                fit = check(leaf, local, origin, validationNodes, std::size(validationNodes), tiny);
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
 * The source's joins, and for a closed source its seam. The curve must be
 * continuous there. Where its tangent direction is continuous within the angle
 * tolerance, and the offset gap that angle opens within the merge budget, the
 * join is smooth; otherwise it is a corner, which fails the request unless
 * @p policy rounds it (@p joins, by break index, the seam at index 0). The
 * curvature may jump at a smooth join (quadratic segments meet only C1):
 * where 1 - d kappa changes sign across it, the offset turns back there, a
 * cusp no box inside either span can see. @p cusps marks those joins.
 */
LC_CurveOffsetStatus checkJoins(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                                const KinkPolicy policy, std::vector<bool>& cusps, std::vector<SourceJoin>& joins) {
    const std::vector<double>& breaks = source.breaks();
    cusps.assign(breaks.size(), false);
    joins.assign(breaks.size(), SourceJoin{});
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
            if (policy == KinkPolicy::Refuse) {
                return LC_CurveOffsetStatus::DiscontinuousNormal;
            }
            SourceJoin& join = joins[index];
            join.point = r.point;
            join.before = l.first / l.first.magnitude();
            join.after = r.first / r.first.magnitude();
            // the two offsets' ends, d N apart on either side, 2 |d| sin(angle / 2) apart
            join.kind = (2.0 * std::abs(d) * std::sin(0.5 * angle) <= 2.0 * options.tolerance.nodeMerge)
                            ? SourceJoin::Kind::Snap
                            : SourceJoin::Kind::Kink;
            return LC_CurveOffsetStatus::Ok;
        }
        // 1 - d kappa's numerator, and a bound on its rounding error: within
        // it the sign is noise, as at a curvature maximum on a knot at d = rho
        auto factorNumerator = [d](const LC_CurveJet& jet, double& noise) {
            const double s2 = dot(jet.first, jet.first);
            const double s = std::sqrt(s2);
            noise = 64.0 * g_eps * (s2 * s + std::abs(d) * s * jet.second.magnitude());
            return s2 * s - d * cross(jet.first, jet.second);
        };
        double noiseBefore = 0.0;
        double noiseAfter = 0.0;
        const double before = factorNumerator(l, noiseBefore);
        const double after = factorNumerator(r, noiseAfter);
        if (!(std::isfinite(before) && std::isfinite(after))) {
            return LC_CurveOffsetStatus::AmbiguousRegularity;
        }
        // A root at the join itself, a value within its noise on one side, is
        // left to the boxes next to it, resolved as a cusp or a stall there.
        cusps[index] = std::abs(before) > noiseBefore && std::abs(after) > noiseAfter &&
                       std::signbit(before) != std::signbit(after);
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
 * the next one, which starts where it ends, and for a closed source the last
 * into the first.
 */
std::vector<std::ptrdiff_t> branchSuccessors(const std::vector<LC_OffsetBranch>& branches) {
    std::vector<std::ptrdiff_t> next(branches.size(), -1);
    for (size_t b = 0; b < branches.size(); ++b) {
        if (branches[b].closed) {
            next[b] = static_cast<std::ptrdiff_t>(b);
            continue;
        }
        // across a cusp the next branch starts where this one ends; once a
        // reversed branch is dropped, its neighbours' cusp ends are free
        const size_t n = (b + 1 < branches.size()) ? b + 1 : 0;
        if (branches[b].endEnd == LC_OffsetBranchEnd::Cusp && branches[n].startEnd == LC_OffsetBranchEnd::Cusp &&
            n != b && branches[b].cubicPieces.back().bezier[3] == branches[n].cubicPieces.front().bezier[0]) {
            next[b] = static_cast<std::ptrdiff_t>(n);
        }
    }
    return next;
}

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

/**
 * The exact offset of the branches, Q = C + d N with Q' = (1 - d kappa) C',
 * over the source parameter intervals of their pieces, and the arcs of their
 * round corners: one segment for each run of pieces within a source span or
 * on one arc. A branch runs over a parameter of its own, which increases along
 * it: the source parameter, continued past a closed source's seam by the
 * domain's length and past a round corner by the angle it turns.
 */
class OffsetCurves final : public LC_ParametricCurves {
public:
    /** What a branch parameter stands for on its segment. */
    struct Place {
        bool arc{false};
        /** The arc's centre, for a corner. */
        RS_Vector centre{false};
        /** The source parameter, or the angle on the arc. */
        double own{0.0};
        /** The source parameter there; at a corner, the corner's. */
        double sourceParameter{0.0};
    };

    OffsetCurves(const OffsetSource& source, const double d, const double speedFloor,
                 const std::vector<LC_OffsetBranch>& branches)
        : m_source{source},
          m_d{d},
          m_speedFloor{speedFloor},
          m_next{branchSuccessors(branches)} {
        const double period = source.breaks().back() - source.breaks().front();
        for (size_t b = 0; b < branches.size(); ++b) {
            double shift = 0.0;
            double reached = 0.0;
            bool started = false;
            for (const LC_OffsetCubicPiece& piece : branches[b].cubicPieces) {
                const LC_OffsetBranchProvenance& p = piece.provenance;
                const bool extends = started && m_segments.back().branch == b;
                if (p.arcCentre.valid) {
                    const double width = std::abs(p.sourceT1 - p.sourceT0);
                    if (extends && m_parts.back().arc && m_parts.back().centre == p.arcCentre &&
                        m_parts.back().a1 == p.sourceT0) {
                        m_segments.back().t1 += width;
                        m_parts.back().a1 = p.sourceT1;
                    }
                    else {
                        m_segments.push_back(LC_ParametricSegment{b, reached, reached + width});
                        Part part;
                        part.arc = true;
                        part.centre = p.arcCentre;
                        part.a0 = p.sourceT0;
                        part.a1 = p.sourceT1;
                        part.corner = started ? reached - shift : std::numeric_limits<double>::quiet_NaN();
                        m_parts.push_back(part);
                    }
                    reached += width;
                    shift += width;
                    started = true;
                    continue;
                }
                double s0 = p.sourceT0 + shift;
                if (started && s0 < reached) {
                    shift += period; // past a closed source's seam
                    s0 += period;
                }
                if (extends && m_parts.back().arc && std::isnan(m_parts.back().corner)) {
                    m_parts.back().corner = p.sourceT0;
                }
                if (extends && !m_parts.back().arc && m_parts.back().span == p.sourceSpan &&
                    m_parts.back().t1 == p.sourceT0) {
                    m_segments.back().t1 = p.sourceT1 + shift;
                    m_parts.back().t1 = p.sourceT1;
                }
                else {
                    m_segments.push_back(LC_ParametricSegment{b, s0, p.sourceT1 + shift});
                    Part part;
                    part.span = p.sourceSpan;
                    part.t0 = p.sourceT0;
                    part.t1 = p.sourceT1;
                    m_parts.push_back(part);
                }
                reached = p.sourceT1 + shift;
                started = true;
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

    /** What the branch parameter @p s stands for; false if no segment of the branch holds it. */
    bool locate(const std::size_t branch, const double s, Place& place) const {
        for (size_t k = 0; k < m_segments.size(); ++k) {
            const LC_ParametricSegment& segment = m_segments[k];
            if (segment.branch != branch || s < segment.t0 || s > segment.t1) {
                continue;
            }
            const Part& part = m_parts[k];
            place.arc = part.arc;
            if (part.arc) {
                place.centre = part.centre;
                place.own = angleAt(k, s);
                place.sourceParameter = part.corner;
            }
            else {
                place.centre = RS_Vector{false};
                place.own = sourceAt(k, s);
                place.sourceParameter = place.own;
            }
            return true;
        }
        return false;
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const LC_ParametricSegment& s = m_segments[segment];
        const Part& part = m_parts[segment];
        if (part.arc) {
            const double angle = angleAt(segment, t);
            const double r = std::abs(m_d);
            const double rate = (part.a1 < part.a0) ? -r : r;
            point = part.centre + RS_Vector{std::cos(angle), std::sin(angle)} * r;
            derivative = RS_Vector{-std::sin(angle), std::cos(angle)} * rate;
            return true;
        }
        const LC_CurveEvaluationSide side = (t == s.t0)   ? LC_CurveEvaluationSide::Right
                                            : (t == s.t1) ? LC_CurveEvaluationSide::Left
                                                          : LC_CurveEvaluationSide::Interior;
        OffsetJet q;
        if (computeOffsetJet(m_source, m_d, m_speedFloor, sourceAt(segment, t), side, q) !=
            LC_CurveOffsetStatus::Ok) {
            return false;
        }
        point = q.point;
        derivative = q.first;
        return true;
    }

    bool bound(const std::size_t segment, const double sa, const double sb, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const Part& part = m_parts[segment];
        if (part.arc) {
            const double alpha = angleAt(segment, sa);
            const double beta = angleAt(segment, sb);
            const LC_Interval r = LC_Interval::point(std::abs(m_d));
            const LC_Interval c = boundCos(std::min(alpha, beta), std::max(alpha, beta));
            const LC_Interval s = boundCos(std::min(alpha, beta) - 0.5 * M_PI, std::max(alpha, beta) - 0.5 * M_PI);
            const LC_Interval pad = LC_Interval::hull(-8.0 * g_eps, 8.0 * g_eps);
            x = LC_Interval::point(part.centre.x) + r * (c + pad);
            y = LC_Interval::point(part.centre.y) + r * (s + pad);
            dx = -(r * (s + pad));
            dy = r * (c + pad);
            if (part.a1 < part.a0) {
                dx = -dx;
                dy = -dy;
            }
            return true;
        }
        const double a = sourceAt(segment, sa);
        const double b = sourceAt(segment, sb);
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
    /** What a segment follows: the offset over source parameters t0 to t1, or an arc. */
    struct Part {
        bool arc{false};
        size_t span{0};
        double t0{0.0};
        double t1{0.0};
        RS_Vector centre{false};
        /** The arc's angles at the segment's start and end. */
        double a0{0.0};
        double a1{0.0};
        /** The source parameter of the corner an arc rounds. */
        double corner{0.0};
    };

    /**
     * The source parameter at branch parameter @p s: from the segment's start,
     * exact at both ends and never past them, so that a box never crosses the
     * knot at a span's end by rounding.
     */
    double sourceAt(const std::size_t segment, const double s) const {
        const LC_ParametricSegment& seg = m_segments[segment];
        const Part& part = m_parts[segment];
        if (s == seg.t0) {
            return part.t0;
        }
        if (s == seg.t1) {
            return part.t1;
        }
        return std::clamp(part.t0 + (s - seg.t0), part.t0, part.t1);
    }

    double angleAt(const std::size_t segment, const double s) const {
        const LC_ParametricSegment& seg = m_segments[segment];
        const Part& part = m_parts[segment];
        if (s == seg.t1) {
            return part.a1;
        }
        return part.a0 + (s - seg.t0) * (part.a1 < part.a0 ? -1.0 : 1.0);
    }

    /** The common part of two enclosures of one value; either one if rounding left them apart. */
    static LC_Interval narrowed(const LC_Interval& a, const LC_Interval& b) {
        if (!a.isValid() || !b.isValid() || a.hi() < b.lo() || b.hi() < a.lo()) {
            return a.isValid() ? a : b;
        }
        return LC_Interval::hull(std::max(a.lo(), b.lo()), std::min(a.hi(), b.hi()));
    }

    const OffsetSource& m_source;
    const double m_d;
    const double m_speedFloor;
    std::vector<std::ptrdiff_t> m_next;
    std::vector<LC_ParametricSegment> m_segments;
    std::vector<Part> m_parts;
};

/**
 * Makes every intersection occurrence a shared end of the pieces of its branch:
 * a piece is refitted as two pieces meeting at the node, or an end that
 * already lies there is moved onto it. Both occurrences of a node share one
 * point. An occurrence's parameter is its branch's in @p curves, which were
 * built from @p branches before any of them was split.
 */
struct Occurrence {
    size_t branch;
    double t;
    RS_Vector point;
};

LC_CurveOffsetStatus insertOccurrences(const OffsetSource& source, const OffsetCurves& curves, const double d,
                                       const double speedFloor, const LC_CurveOffsetOptions& options,
                                       const std::vector<Occurrence>& occurrences,
                                       std::vector<LC_OffsetBranch>& branches) {
    auto insert = [&](const size_t b, const double s, const RS_Vector& node) {
        OffsetCurves::Place place;
        if (!curves.locate(b, s, place)) {
            return LC_CurveOffsetStatus::FitFailed;
        }
        const double t = place.own;
        std::vector<LC_OffsetCubicPiece>& pieces = branches[b].cubicPieces;
        for (size_t k = 0; k < pieces.size(); ++k) {
            LC_OffsetCubicPiece& piece = pieces[k];
            const LC_OffsetBranchProvenance p = piece.provenance;
            const bool arc = p.arcCentre.valid;
            // an arc's angles may run down, from sourceT0 to a smaller sourceT1
            const double lo = std::min(p.sourceT0, p.sourceT1);
            const double hi = std::max(p.sourceT0, p.sourceT1);
            if (arc != place.arc || (arc && p.arcCentre != place.centre) || t < lo || t > hi) {
                continue;
            }
            // at an end of the piece, or so close that splitting would leave nothing
            const bool atStart = piece.bezier[0].distanceTo(node) <= options.tolerance.nodeMerge;
            const bool atEnd = piece.bezier[3].distanceTo(node) <= options.tolerance.nodeMerge;
            if (atStart || atEnd || !(t > lo && t < hi)) {
                const bool start = atStart || (!atEnd && std::abs(t - p.sourceT0) <= std::abs(p.sourceT1 - t));
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
            LC_OffsetCubicPiece left = piece;
            LC_OffsetCubicPiece right = piece;
            left.provenance.sourceT1 = t;
            right.provenance.sourceT0 = t;
            if (arc) {
                const double radius = std::abs(p.signedDistance);
                left.bezier = arcPiece(p.arcCentre, radius, p.sourceT0, t);
                right.bezier = arcPiece(p.arcCentre, radius, t, p.sourceT1);
                left.bezier[0] = piece.bezier[0];
                left.bezier[3] = node;
                right.bezier[0] = node;
                right.bezier[3] = piece.bezier[3];
            }
            else {
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
                left.bezier = hermitePiece(piece.bezier[0], q0.first, node, qt.first, t - p.sourceT0);
                right.bezier = hermitePiece(node, qt.first, piece.bezier[3], q1.first, p.sourceT1 - t);
            }
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

/**
 * Validates the pieces of a branch against the exact offset, refitting those
 * that stray: each piece is compared through its own Bezier, at parameters the
 * fitter never used, from both sides.
 */
class Materializer {
public:
    Materializer(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                 const std::size_t maxPieces, const double speedFloor, const std::size_t samplesUsed)
        : m_source{source},
          m_d{d},
          m_options{options},
          m_maxPieces{maxPieces},
          m_speedFloor{speedFloor},
          m_samples{samplesUsed} {
    }

    /** Exact evaluations so far, those it started from included. */
    std::size_t samples() const {
        return m_samples;
    }

    /** The branch's pieces, validated and refitted, into @p pieces. */
    LC_CurveOffsetStatus run(const LC_OffsetBranch& branch, std::vector<LC_OffsetCubicPiece>& pieces,
                             double& maxError) {
        if (branch.cubicPieces.empty()) {
            return LC_CurveOffsetStatus::FitFailed;
        }
        std::vector<Work> work;
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            work.push_back(Work{piece, false, false, 0.0, 0.5});
        }
        constexpr int maxRounds = 4;
        for (int round = 0;; ++round) {
            if (work.size() > m_maxPieces) {
                return LC_CurveOffsetStatus::LimitExceeded;
            }
            bool anyFailed = false;
            for (Work& w : work) {
                if (w.checked) {
                    continue;
                }
                const LC_CurveOffsetStatus status = validate(w);
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
        pieces.clear();
        maxError = 0.0;
        for (const Work& w : work) {
            maxError = std::max(maxError, w.error);
            pieces.push_back(w.piece);
        }
        return LC_CurveOffsetStatus::Ok;
    }

private:
    struct Work {
        LC_OffsetCubicPiece piece;
        bool checked;
        bool passed;
        double error;
        /** Local parameter of the worst sample: where a failed piece is split. */
        double worstU;
    };

    /** The exact offset at t, at the piece's own signed distance. */
    LC_CurveOffsetStatus jet(const double t, const LC_CurveEvaluationSide side, const double d, OffsetJet& out) {
        if (++m_samples > m_options.maxSamples) {
            return LC_CurveOffsetStatus::LimitExceeded;
        }
        return computeOffsetJet(m_source, d, m_speedFloor, t, side, out);
    }

    static RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
        const double r = 1.0 - s;
        return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
    }

    /**
     * The piece against the exact offset at fresh parameters: paired at the
     * same source parameter, which catches a wrong parameter map, and from the
     * piece to the nearest offset point on its interval, which catches an
     * overshoot.
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
            const RS_Vector candidate = bezierAt(w.piece.bezier, u);
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

    /**
     * An arc piece against its circle: radially, and within the arc's angles.
     * Its parameter maps to no source parameter, and a cubic arc's parameter is
     * not proportional to its angle, so nothing is paired.
     */
    LC_CurveOffsetStatus validateArc(Work& w) {
        const LC_OffsetBranchProvenance& p = w.piece.provenance;
        const double radius = std::abs(p.signedDistance);
        const double half = 0.5 * std::abs(p.sourceT1 - p.sourceT0);
        const double middle = 0.5 * (p.sourceT0 + p.sourceT1);
        const auto onCircle = [&](const double angle) {
            return p.arcCentre + RS_Vector{std::cos(angle), std::sin(angle)} * radius;
        };
        w.error = 0.0;
        w.worstU = 0.5;
        for (int k = 0; k <= 16; ++k) {
            const double u = k / 16.0;
            const RS_Vector candidate = bezierAt(w.piece.bezier, u);
            const RS_Vector r = candidate - p.arcCentre;
            double error = std::abs(r.magnitude() - radius);
            // past either end of the arc: as far as from that end
            const double off = std::remainder(std::atan2(r.y, r.x) - middle, 2.0 * M_PI);
            if (std::abs(off) > half) {
                error = std::max(error, std::min(candidate.distanceTo(onCircle(p.sourceT0)),
                                                 candidate.distanceTo(onCircle(p.sourceT1))));
            }
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
        Work left{w.piece, false, false, 0.0, 0.5};
        Work right{w.piece, false, false, 0.0, 0.5};
        left.piece.provenance.sourceT1 = tm;
        right.piece.provenance.sourceT0 = tm;
        if (p.arcCentre.valid) {
            left.piece.bezier = arcPiece(p.arcCentre, std::abs(p.signedDistance), p.sourceT0, tm);
            right.piece.bezier = arcPiece(p.arcCentre, std::abs(p.signedDistance), tm, p.sourceT1);
            left.piece.bezier[0] = w.piece.bezier[0];
            right.piece.bezier[0] = left.piece.bezier[3];
            right.piece.bezier[3] = w.piece.bezier[3];
        }
        else {
            OffsetJet q0;
            OffsetJet qm;
            OffsetJet q1;
            const bool forwards = p.sourceT0 < p.sourceT1;
            const LC_CurveEvaluationSide inward0 =
                forwards ? LC_CurveEvaluationSide::Right : LC_CurveEvaluationSide::Left;
            const LC_CurveEvaluationSide inward1 =
                forwards ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Right;
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
            left.piece.bezier = hermitePiece(w.piece.bezier[0], q0.first, qm.point, qm.first, tm - p.sourceT0);
            right.piece.bezier = hermitePiece(qm.point, qm.first, w.piece.bezier[3], q1.first, p.sourceT1 - tm);
        }
        work.erase(work.begin() + static_cast<std::ptrdiff_t>(index));
        work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(right));
        work.insert(work.begin() + static_cast<std::ptrdiff_t>(index), std::move(left));
        return LC_CurveOffsetStatus::Ok;
    }

    const OffsetSource& m_source;
    const double m_d;
    const LC_CurveOffsetOptions& m_options;
    const std::size_t m_maxPieces;
    const double m_speedFloor;
    std::size_t m_samples;
};

/** The piece split at s by de Casteljau, exactly: its two halves. */
std::pair<std::array<RS_Vector, 4>, std::array<RS_Vector, 4>> splitBezier(const std::array<RS_Vector, 4>& b,
                                                                           const double s) {
    const RS_Vector p01 = b[0] + (b[1] - b[0]) * s;
    const RS_Vector p12 = b[1] + (b[2] - b[1]) * s;
    const RS_Vector p23 = b[2] + (b[3] - b[2]) * s;
    const RS_Vector p012 = p01 + (p12 - p01) * s;
    const RS_Vector p123 = p12 + (p23 - p12) * s;
    const RS_Vector mid = p012 + (p123 - p012) * s;
    return {{b[0], p01, p012, mid}, {mid, p123, p23, b[3]}};
}

/**
 * One entity for a validated chain of pieces: an RS_Line for one straight
 * piece; a degree-1 RS_Spline through the corners when every piece is
 * straight; otherwise a clamped cubic RS_Spline whose knots are the integers,
 * each of multiplicity 3 at a joint, so each span's control points are its
 * piece's Bezier net. Pieces within the merge tolerance of a point are folded
 * into a neighbour; at a joint that is no corner the handles are made exactly
 * collinear; a closed chain starts in the middle of its longest piece, never
 * at a corner. Null if the result does not validate or draw.
 */
std::unique_ptr<RS_Entity> chainEntity(std::vector<LC_OffsetCubicPiece> chain, const bool closed,
                                       const LC_CurveOffsetOptions& options, const double numericFloor) {
    const double merge = options.tolerance.nodeMerge;
    const auto extent = [](const std::array<RS_Vector, 4>& b) {
        double e = 0.0;
        for (const RS_Vector& v : b) {
            e = std::max(e, v.distanceTo(b[0]));
        }
        return e;
    };
    for (size_t k = 0; k < chain.size() && chain.size() > 1;) {
        if (extent(chain[k].bezier) > merge) {
            ++k;
            continue;
        }
        if (k > 0) {
            chain[k - 1].bezier[3] = chain[k].bezier[3];
        }
        else {
            chain[1].bezier[0] = chain[0].bezier[0];
        }
        chain.erase(chain.begin() + static_cast<std::ptrdiff_t>(k));
    }
    const auto straight = [numericFloor](const std::array<RS_Vector, 4>& b) {
        const RS_Vector axis = b[3] - b[0];
        const double length = axis.magnitude();
        if (!(length > numericFloor)) {
            return false;
        }
        const auto off = [&](const RS_Vector& v) { return std::abs(cross(axis, v - b[0])) / length; };
        return off(b[1]) <= 16.0 * numericFloor && off(b[2]) <= 16.0 * numericFloor;
    };
    if (closed && !chain.empty()) {
        size_t longest = 0;
        for (size_t k = 1; k < chain.size(); ++k) {
            if (chain[k].bezier[0].distanceTo(chain[k].bezier[3]) >
                chain[longest].bezier[0].distanceTo(chain[longest].bezier[3])) {
                longest = k;
            }
        }
        const auto [first, second] = splitBezier(chain[longest].bezier, 0.5);
        LC_OffsetCubicPiece head = chain[longest];
        LC_OffsetCubicPiece tail = chain[longest];
        head.bezier = second;
        tail.bezier = first;
        std::vector<LC_OffsetCubicPiece> rotated{head};
        for (size_t k = 1; k < chain.size(); ++k) {
            rotated.push_back(chain[(longest + k) % chain.size()]);
        }
        rotated.push_back(tail);
        chain = std::move(rotated);
    }
    // exact collinear handles where the joint is no corner
    for (size_t k = 0; k + 1 < chain.size(); ++k) {
        RS_Vector& before = chain[k].bezier[2];
        RS_Vector& after = chain[k + 1].bezier[1];
        const RS_Vector& joint = chain[k].bezier[3];
        const RS_Vector in = joint - before;
        const RS_Vector out = after - joint;
        const double lengthIn = in.magnitude();
        const double lengthOut = out.magnitude();
        if (!(lengthIn > 0.0) || !(lengthOut > 0.0) || angleBetweenVectors(in, out) > options.angleTolerance) {
            continue;
        }
        const RS_Vector direction = (in / lengthIn + out / lengthOut).normalized();
        const RS_Vector movedBefore = joint - direction * lengthIn;
        const RS_Vector movedAfter = joint + direction * lengthOut;
        // a handle moved by h moves its piece by at most 4/9 h: within the pinning share
        if (4.0 / 9.0 * std::max(movedBefore.distanceTo(before), movedAfter.distanceTo(after)) > merge) {
            continue;
        }
        before = movedBefore;
        after = movedAfter;
    }
    if (chain.empty()) {
        return nullptr;
    }
    const bool allStraight = std::all_of(chain.begin(), chain.end(),
                                         [&](const LC_OffsetCubicPiece& piece) { return straight(piece.bezier); });
    std::unique_ptr<RS_Entity> entity;
    if (allStraight && chain.size() == 1 && !closed) {
        entity = std::make_unique<RS_Line>(nullptr, RS_LineData{chain.front().bezier[0], chain.front().bezier[3]});
    }
    else {
        RS_SplineData data(allStraight ? 1 : 3, false);
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.controlPoints.push_back(chain.front().bezier[0]);
        if (allStraight) {
            for (size_t k = 0; k < chain.size(); ++k) {
                // a corner only where the direction changes: a vertex within the
                // pinning share of the line past it, going on the same way, is dropped
                bool collinear = false;
                if (k + 1 < chain.size()) {
                    const RS_Vector& from = data.controlPoints.back();
                    const RS_Vector& vertex = chain[k].bezier[3];
                    const RS_Vector& to = chain[k + 1].bezier[3];
                    const RS_Vector axis = to - from;
                    const double length = axis.magnitude();
                    collinear = length > numericFloor &&
                                angleBetweenVectors(vertex - from, to - vertex) <= options.angleTolerance &&
                                std::abs(cross(axis, vertex - from)) / length <= merge;
                }
                if (!collinear) {
                    data.controlPoints.push_back(chain[k].bezier[3]);
                }
            }
            const size_t spans = data.controlPoints.size() - 1;
            data.knotslist = {0.0};
            for (size_t k = 0; k <= spans; ++k) {
                data.knotslist.push_back(static_cast<double>(k));
            }
            data.knotslist.push_back(static_cast<double>(spans));
        }
        else {
            data.knotslist.assign(4, 0.0);
            for (size_t k = 0; k < chain.size(); ++k) {
                data.controlPoints.push_back(chain[k].bezier[1]);
                data.controlPoints.push_back(chain[k].bezier[2]);
                data.controlPoints.push_back(chain[k].bezier[3]);
                const double knot = static_cast<double>(k + 1);
                data.knotslist.insert(data.knotslist.end(), (k + 1 == chain.size()) ? 4 : 3, knot);
            }
        }
        for (RS_Vector& v : data.controlPoints) {
            v.z = 0.0;
        }
        data.weights.assign(data.controlPoints.size(), 1.0);
        auto spline = std::make_unique<RS_Spline>(nullptr, data);
        // a spline without the segments it is drawn by would be an invisible success
        if (!spline->validate() || spline->count() == 0) {
            return nullptr;
        }
        // each span is its piece: checked through the spline's own evaluator
        if (!allStraight) {
            for (size_t k = 0; k < chain.size(); ++k) {
                LC_CurveJet jet;
                const auto [first, second] = splitBezier(chain[k].bezier, 0.5);
                if (!spline->tryEvaluateJet(static_cast<double>(k) + 0.5, LC_CurveEvaluationSide::Interior, jet) ||
                    jet.point.distanceTo(first[3]) > options.tolerance.evaluation) {
                    return nullptr;
                }
                (void)second;
            }
        }
        entity = std::move(spline);
    }
    entity->calculateBorders();
    if (!isFinite(entity->getMin()) || !isFinite(entity->getMax())) {
        return nullptr;
    }
    return entity;
}

// ---------------------------------------------------------------------------
// Direct branches and their nodes
// ---------------------------------------------------------------------------

/** The Direct branches at signed distance @p d, before any noding. */
LC_CurveOffsetStatus directBranches(const OffsetSource& source, const SourceScale& scale, const double d,
                                    const LC_CurveOffsetOptions& options, const LC_OffsetSourceBudget& budget,
                                    const KinkPolicy policy, std::vector<LC_OffsetBranch>& branches,
                                    double& maxError, std::size_t& samples) {
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
    std::vector<SourceJoin> joins;
    const LC_CurveOffsetStatus joined = checkJoins(source, d, options, policy, joinCusps, joins);
    if (joined != LC_CurveOffsetStatus::Ok) {
        return joined;
    }
    BranchBuilder builder{source, d, options, budget, scale.numericFloor / domain};
    const LC_CurveOffsetStatus status = builder.build(joinCusps, joins, branches);
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
    const OffsetCurves curves{source, d, speedFloor, branches};
    const LC_IntersectionResult found = findIntersections(curves, query);
    LC_CurveOffsetStatus status = fromIntersectionStatus(found.status);
    std::vector<Occurrence> occurrences;
    for (const LC_ParametricIntersection& x : found.intersections) {
        occurrences.push_back({x.branchA, x.parameterA, x.point});
        occurrences.push_back({x.branchB, x.parameterB, x.point});
    }
    if (status == LC_CurveOffsetStatus::Ok) {
        status = insertOccurrences(source, curves, d, speedFloor, options, occurrences, branches);
    }
    if (status != LC_CurveOffsetStatus::Ok) {
        return status;
    }
    // reported by source parameter, a round corner's at its arc
    result.intersections = found.intersections;
    for (LC_ParametricIntersection& x : result.intersections) {
        OffsetCurves::Place a;
        OffsetCurves::Place b;
        if (curves.locate(x.branchA, x.parameterA, a) && curves.locate(x.branchB, x.parameterB, b)) {
            x.parameterA = a.sourceParameter;
            x.parameterB = b.sourceParameter;
        }
    }
    result.offsetIntersections = found.intersections.size();

    if (options.mode == LC_CurveOffsetMode::Trimmed) {
        return LC_CurveOffsetStatus::Ok; // the count below is for noding alone
    }
    const std::vector<double>& breaks = source.breaks();
    LC_OffsetBranch whole;
    whole.closed = source.closed();
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        LC_OffsetCubicPiece piece;
        piece.provenance = {span, breaks[span], breaks[span + 1], 0.0, true};
        whole.cubicPieces.push_back(piece);
    }
    // a count only: a source that retraces itself leaves it at zero
    const LC_IntersectionResult crossings = findIntersections(OffsetCurves{source, 0.0, speedFloor, {whole}}, query);
    if (crossings.status == LC_IntersectionStatus::Ok) {
        result.sourceIntersections = crossings.intersections.size();
    }
    return LC_CurveOffsetStatus::Ok;
}

// ---------------------------------------------------------------------------
// Distance-map trimming (plan 5.12)
// ---------------------------------------------------------------------------

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

/** Circular arcs of one radius, one open branch each over its own angles. */
class ArcCurves final : public LC_ParametricCurves {
public:
    struct Arc {
        RS_Vector centre;
        double a0;
        double a1;
        /** Runs from a1 back to a0: its parameter t is the angle a0 + a1 - t. */
        bool reversed{false};
    };

    ArcCurves(std::vector<Arc> arcs, const double radius)
        : m_arcs{std::move(arcs)},
          m_radius{radius} {
        for (size_t b = 0; b < m_arcs.size(); ++b) {
            m_segments.push_back({b, m_arcs[b].a0, m_arcs[b].a1});
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_arcs.size();
    }

    std::ptrdiff_t next(const std::size_t) const override {
        return -1;
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const Arc& arc = m_arcs[segment];
        const double angle = arc.reversed ? arc.a0 + arc.a1 - t : t;
        point = arc.centre + RS_Vector{std::cos(angle), std::sin(angle)} * m_radius;
        derivative = RS_Vector{-std::sin(angle), std::cos(angle)} * (arc.reversed ? -m_radius : m_radius);
        return true;
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const Arc& arc = m_arcs[segment];
        const double from = arc.reversed ? arc.a0 + arc.a1 - b : a;
        const double to = arc.reversed ? arc.a0 + arc.a1 - a : b;
        const LC_Interval r = LC_Interval::point(m_radius);
        const LC_Interval c = boundCos(from, to);
        const LC_Interval s = boundCos(from - 0.5 * M_PI, to - 0.5 * M_PI); // sin t = cos(t - pi/2)
        const LC_Interval pad = LC_Interval::hull(-8.0 * g_eps, 8.0 * g_eps);
        x = LC_Interval::point(arc.centre.x) + r * (c + pad);
        y = LC_Interval::point(arc.centre.y) + r * (s + pad);
        dx = -(r * (s + pad));
        dy = r * (c + pad);
        if (arc.reversed) {
            dx = -dx;
            dy = -dy;
        }
        return true;
    }

private:
    std::vector<Arc> m_arcs;
    double m_radius;
    std::vector<LC_ParametricSegment> m_segments;
};

/**
 * Several curve sets as one, their branches numbered in the given order, with
 * joins across sets: each link runs the end of one branch into another's
 * start.
 */
class CompositeCurves final : public LC_ParametricCurves {
public:
    explicit CompositeCurves(std::vector<const LC_ParametricCurves*> parts,
                             std::vector<std::pair<std::size_t, std::size_t>> links = {})
        : m_parts{std::move(parts)},
          m_links{std::move(links)} {
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
        for (const auto& [from, to] : m_links) {
            if (from == branch) {
                return static_cast<std::ptrdiff_t>(to);
            }
        }
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
    std::vector<std::pair<std::size_t, std::size_t>> m_links;
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
 * Removes the offset's local loops, for trimming: each reversed branch between
 * two cusps, with the stretches of its neighbours from where they cross to the
 * cusps, the swallowtail's wings. The crossing is found by walking along the
 * neighbour after it and tracking which side of the neighbour before it that
 * point lies on: the side changes sign where they cross, however shallow the
 * crossing, where intersection boxes could not tell the two apart. Only the
 * neighbours' offset pieces next to the cusps are walked; one that ends first,
 * or turns a corner, leaves the loop to noding and trimming.
 */
LC_CurveOffsetStatus removeLocalLoops(const OffsetSource& source, const double d, const double speedFloor,
                                      const LC_CurveOffsetOptions& options, std::vector<LC_OffsetBranch>& branches,
                                      std::size_t& removed) {
    // the source parameters of a branch's offset pieces next to one end, as long
    // as they run on without a gap or a corner
    const auto stretch = [](const LC_OffsetBranch& branch, const bool atEnd, double& from, double& to) {
        const std::vector<LC_OffsetCubicPiece>& pieces = branch.cubicPieces;
        const size_t n = pieces.size();
        const LC_OffsetCubicPiece& first = atEnd ? pieces[n - 1] : pieces[0];
        if (first.provenance.arcCentre.valid) {
            return false;
        }
        from = first.provenance.sourceT0;
        to = first.provenance.sourceT1;
        for (size_t k = 1; k < n; ++k) {
            const LC_OffsetBranchProvenance& p = pieces[atEnd ? n - 1 - k : k].provenance;
            if (p.arcCentre.valid || (atEnd ? p.sourceT1 != from : p.sourceT0 != to)) {
                break;
            }
            (atEnd ? from : to) = atEnd ? p.sourceT0 : p.sourceT1;
        }
        return true;
    };
    const auto offsetAt = [&](const double t, OffsetJet& q) {
        return computeOffsetJet(source, d, speedFloor, t, LC_CurveEvaluationSide::Interior, q) ==
               LC_CurveOffsetStatus::Ok;
    };
    // the nearest point of the stretch [lo, hi] to p: the nearest of samples
    // from its end back, in doubling steps, and of t, then projection steps;
    // next to the cusp at hi the offset all but stops, too slow to step from
    const auto project = [&](const RS_Vector& p, double t, const double lo, const double hi, OffsetJet& at) {
        double best = std::numeric_limits<double>::infinity();
        for (double back = std::ldexp(hi - lo, -40); back <= 2.0 * (hi - lo); back *= 2.0) {
            const double candidate = std::max(lo, hi - back);
            if (offsetAt(candidate, at) && at.point.distanceTo(p) < best) {
                best = at.point.distanceTo(p);
                t = candidate;
            }
        }
        for (int iteration = 0; iteration < 40; ++iteration) {
            if (!offsetAt(t, at)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            const double speed2 = dot(at.first, at.first);
            if (!(speed2 > 0.0)) {
                break;
            }
            const double next = std::clamp(t + dot(p - at.point, at.first) / speed2, lo, hi);
            if (std::abs(next - t) <= options.tolerance.parameter) {
                t = next;
                break;
            }
            t = next;
        }
        return offsetAt(t, at) ? t : std::numeric_limits<double>::quiet_NaN();
    };

    for (size_t k = 0; k < branches.size() && branches.size() > 1; ++k) {
        const size_t n = branches.size();
        const LC_OffsetBranch& loop = branches[k];
        if (!loop.reversed || loop.closed || loop.startEnd != LC_OffsetBranchEnd::Cusp ||
            loop.endEnd != LC_OffsetBranchEnd::Cusp) {
            continue;
        }
        const size_t before = (k + n - 1) % n;
        const size_t after = (k + 1) % n;
        LC_OffsetBranch& a = branches[before];
        LC_OffsetBranch& b = branches[after];
        double a0 = 0.0;
        double a1 = 0.0;
        double b0 = 0.0;
        double b1 = 0.0;
        if (a.reversed || b.reversed || !stretch(a, true, a0, a1) || !stretch(b, false, b0, b1)) {
            continue;
        }
        // walk along b away from its cusp; the side is unreliable where a all but stops
        const auto sideAt = [&](const double u, double& t, RS_Vector& point) {
            OffsetJet qb;
            OffsetJet qa;
            if (!offsetAt(u, qb) || std::isnan(t = project(qb.point, t, a0, a1, qa))) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            point = qb.point;
            const double speed = qa.first.magnitude();
            if (!(speed > 0.0) || t <= a0) {
                return std::numeric_limits<double>::quiet_NaN(); // a ended before b crossed it
            }
            return cross(qa.first, qb.point - qa.point) / speed;
        };
        double t = a1;
        double sign = 0.0;
        double lo = b0;
        double hi = b0;
        double loT = a1;
        bool found = false;
        // the last point before any reliable side, still within the evaluation
        // tolerance of a: a crossing there is as good as anywhere nearer
        double nearU = std::numeric_limits<double>::quiet_NaN();
        double nearT = a1;
        // from the cusp outwards, in doubling steps: a small loop's crossing is near it
        const double first = std::max(std::ldexp(b1 - b0, -40), 4.0 * options.tolerance.parameter);
        for (double reach = first; !found && reach < 2.0 * (b1 - b0); reach *= 2.0) {
            const double u = std::min(b1, b0 + reach);
            RS_Vector point;
            const double side = sideAt(u, t, point);
            if (std::isnan(side)) {
                break;
            }
            if (std::abs(side) <= options.tolerance.evaluation) {
                if (sign == 0.0) {
                    nearU = u;
                    nearT = t;
                }
                continue; // too near a to tell
            }
            if (sign == 0.0) {
                sign = side;
                lo = u;
                loT = t;
                continue;
            }
            if (std::signbit(side) != std::signbit(sign)) {
                hi = u;
                found = true;
            }
            else {
                lo = u;
                loT = t;
            }
        }
        const bool below = !found && !std::isnan(nearU) && sign != 0.0;
        if (!found && !below) {
            continue;
        }
        if (below) {
            // the loop is below the evaluation tolerance: cut where b last was
            lo = nearU;
            hi = nearU;
        }
        t = below ? nearT : loT;
        RS_Vector point;
        for (int iteration = 0; iteration < 80 && hi - lo > options.tolerance.parameter; ++iteration) {
            const double mid = lo + 0.5 * (hi - lo);
            double tm = t;
            const double side = sideAt(mid, tm, point);
            if (std::isnan(side)) {
                break;
            }
            if (side != 0.0 && std::signbit(side) == std::signbit(sign)) {
                lo = mid;
                t = tm;
            }
            else {
                hi = mid;
            }
        }
        OffsetJet qb;
        OffsetJet qa;
        const double u = hi;
        if (!offsetAt(u, qb) || std::isnan(t = project(qb.point, t, a0, a1, qa)) ||
            qa.point.distanceTo(qb.point) > (below ? options.tolerance.evaluation : options.tolerance.nodeMerge)) {
            continue;
        }
        const RS_Vector meet = (qa.point + qb.point) * 0.5;
        // a branch up to the crossing (keepFront), or from it
        const auto cut = [&](LC_OffsetBranch& branch, const double at, const OffsetJet& q, const bool keepFront) {
            std::vector<LC_OffsetCubicPiece>& pieces = branch.cubicPieces;
            for (size_t j = 0; j < pieces.size(); ++j) {
                LC_OffsetCubicPiece& piece = pieces[j];
                const LC_OffsetBranchProvenance p = piece.provenance;
                if (p.arcCentre.valid || at < p.sourceT0 || at > p.sourceT1) {
                    continue;
                }
                OffsetJet end;
                if (keepFront) {
                    if (at > p.sourceT0 && computeOffsetJet(source, d, speedFloor, p.sourceT0,
                                                            LC_CurveEvaluationSide::Right, end) ==
                                               LC_CurveOffsetStatus::Ok) {
                        piece.bezier = hermitePiece(piece.bezier[0], end.first, meet, q.first, at - p.sourceT0);
                        piece.provenance.sourceT1 = at;
                        pieces.resize(j + 1);
                    }
                    else {
                        pieces.resize(j); // the crossing is this piece's start
                        if (pieces.empty()) {
                            return false;
                        }
                    }
                    pieces.back().bezier[3] = meet;
                }
                else {
                    if (at < p.sourceT1 && computeOffsetJet(source, d, speedFloor, p.sourceT1,
                                                            LC_CurveEvaluationSide::Left, end) ==
                                               LC_CurveOffsetStatus::Ok) {
                        piece.bezier = hermitePiece(meet, q.first, piece.bezier[3], end.first, p.sourceT1 - at);
                        piece.provenance.sourceT0 = at;
                        pieces.erase(pieces.begin(), pieces.begin() + static_cast<std::ptrdiff_t>(j));
                    }
                    else {
                        pieces.erase(pieces.begin(), pieces.begin() + static_cast<std::ptrdiff_t>(j + 1));
                        if (pieces.empty()) {
                            return false;
                        }
                    }
                    pieces.front().bezier[0] = meet;
                }
                return true;
            }
            return false;
        };
        if (before == after) {
            // the only other branch runs out of the loop and back into it: a ring
            LC_OffsetBranch ring = a;
            if (!cut(ring, t, qa, true) || !cut(ring, u, qb, false)) {
                continue;
            }
            ring.closed = true;
            ring.startEnd = LC_OffsetBranchEnd::Free;
            ring.endEnd = LC_OffsetBranchEnd::Free;
            removed += 3; // the loop and its two wings
            branches.assign(1, std::move(ring));
            return LC_CurveOffsetStatus::Ok;
        }
        LC_OffsetBranch front = a;
        LC_OffsetBranch back = b;
        if (!cut(front, t, qa, true) || !cut(back, u, qb, false)) {
            continue;
        }
        removed += 3; // the loop and its two wings
        front.cubicPieces.insert(front.cubicPieces.end(), back.cubicPieces.begin(), back.cubicPieces.end());
        front.endEnd = back.endEnd;
        branches[before] = std::move(front);
        for (const size_t gone : {std::max(k, after), std::min(k, after)}) {
            branches.erase(branches.begin() + static_cast<std::ptrdiff_t>(gone));
        }
        k = static_cast<size_t>(-1); // indices moved: start over
    }
    return LC_CurveOffsetStatus::Ok;
}

/**
 * The offset at @p d as a cutter for trimming: only where it runs matters,
 * which OffsetCurves evaluates exactly, so each span is one piece, not fitted,
 * and cusps and stalls are no concern, since a cutter's pieces are never noded
 * with each other. A corner that turns away from it is rounded as a branch's
 * would be; where one turns towards it, a new branch starts.
 */
LC_CurveOffsetStatus cutterBranches(const OffsetSource& source, const double d, const LC_CurveOffsetOptions& options,
                                    const double speedFloor, std::vector<LC_OffsetBranch>& branches) {
    const std::vector<double>& breaks = source.breaks();
    std::vector<bool> cusps;
    std::vector<SourceJoin> joins;
    const LC_CurveOffsetStatus joined = checkJoins(source, d, options, KinkPolicy::Round, cusps, joins);
    if (joined != LC_CurveOffsetStatus::Ok) {
        return joined;
    }
    std::vector<LC_OffsetCubicPiece> spans;
    for (size_t span = 0; span + 1 < breaks.size(); ++span) {
        OffsetJet q0;
        OffsetJet q1;
        if (computeOffsetJet(source, d, speedFloor, breaks[span], LC_CurveEvaluationSide::Right, q0) !=
                LC_CurveOffsetStatus::Ok ||
            computeOffsetJet(source, d, speedFloor, breaks[span + 1], LC_CurveEvaluationSide::Left, q1) !=
                LC_CurveOffsetStatus::Ok) {
            return LC_CurveOffsetStatus::UndefinedTangent;
        }
        LC_OffsetCubicPiece piece;
        piece.provenance = {span, breaks[span], breaks[span + 1], d, true};
        piece.bezier = {q0.point, q0.point, q1.point, q1.point};
        spans.push_back(piece);
    }
    // the corner before span k, the seam's for a closed source at 0
    const auto corner = [&](const size_t k, LC_OffsetBranch& into, const LC_OffsetCubicPiece& next) {
        std::vector<LC_OffsetCubicPiece> arc;
        const SourceJoin& join = joins[k];
        if (join.kind != SourceJoin::Kind::Kink) {
            return true; // smooth, or closed within the merge tolerance
        }
        if (!cornerArc(join, into.cubicPieces.back().bezier[3], next.bezier[0], d, options, arc)) {
            return false;
        }
        into.cubicPieces.insert(into.cubicPieces.end(), arc.begin(), arc.end());
        return true;
    };
    branches.assign(1, LC_OffsetBranch{});
    branches.back().cubicPieces.push_back(spans.front());
    for (size_t k = 1; k < spans.size(); ++k) {
        if (!corner(k, branches.back(), spans[k])) {
            branches.back().endEnd = LC_OffsetBranchEnd::Kink;
            branches.emplace_back();
            branches.back().startEnd = LC_OffsetBranchEnd::Kink;
        }
        branches.back().cubicPieces.push_back(spans[k]);
    }
    if (source.closed()) {
        if (!corner(0, branches.back(), spans.front())) {
            branches.back().endEnd = LC_OffsetBranchEnd::Kink;
            branches.front().startEnd = LC_OffsetBranchEnd::Kink;
        }
        else if (branches.size() == 1) {
            branches.front().closed = true;
        }
        else {
            // the last branch runs on across the seam into the first
            std::vector<LC_OffsetCubicPiece>& wrapped = branches.back().cubicPieces;
            wrapped.insert(wrapped.end(), branches.front().cubicPieces.begin(), branches.front().cubicPieces.end());
            branches.front().cubicPieces = std::move(wrapped);
            branches.front().startEnd = branches.back().startEnd;
            branches.pop_back();
        }
    }
    return LC_CurveOffsetStatus::Ok;
}

/**
 * Removes the parts of the noded Direct offset nearer to the source than
 * rho = |d| minus the approximation and classification budget, and chains
 * what remains through its nodes. Visibility can change only at the offset's
 * nodes and cusps, where it meets the offset on the other side, and, for an
 * open source, where it crosses the half circles of radius |d| ahead of its
 * ends; the branches are split there. A reversed branch is hidden throughout.
 * Every other fragment is classified by the distance of an interior point,
 * certified by visibility(), or the request fails; one that ends at a cusp or
 * at the overlapping end of a corner, unless on a node or cut, is hidden,
 * since that end is and visibility cannot change on the way to it.
 */
LC_CurveOffsetStatus trimBranches(const OffsetSource& source, const SourceScale& scale, const double d,
                                  const LC_CurveOffsetOptions& options, std::vector<LC_OffsetBranch>& branches,
                                  LC_CurveOffsetGeometryResult& result) {
    const double rho =
        std::abs(d) - (options.tolerance.requestedGeometry + options.tolerance.classification);
    if (!(rho > scale.numericFloor) || branches.front().straight) {
        return LC_CurveOffsetStatus::Ok; // nothing can be resolved as hidden: the Direct offset stands
    }
    const std::vector<double>& breaks = source.breaks();
    const double speedFloor = scale.numericFloor / (breaks.back() - breaks.front());

    // where the offset meets the other side's offset and the half circles ahead of the ends
    std::vector<LC_OffsetBranch> opposite;
    const LC_CurveOffsetStatus other = cutterBranches(source, -d, options, speedFloor, opposite);
    if (other != LC_CurveOffsetStatus::Ok) {
        return other;
    }
    // Past an open end E only the half plane ahead of it has E as its nearest
    // source point, so only the half circle there bounds what E hides. Its ends
    // are the two offsets' own ends, and it touches the offset there, where
    // the tangent contact is no crossing: each cap is joined to the offset's end.
    std::vector<ArcCurves::Arc> caps;
    if (!source.closed()) {
        LC_CurveJet first;
        LC_CurveJet last;
        if (!source.jet(breaks.front(), LC_CurveEvaluationSide::Right, first) ||
            !source.jet(breaks.back(), LC_CurveEvaluationSide::Left, last) ||
            !(dot(first.first, first.first) > 0.0) || !(dot(last.first, last.first) > 0.0)) {
            return LC_CurveOffsetStatus::InvalidSource;
        }
        // Q = E + d N lies at the start of the half circle ahead of the start for
        // d > 0, and at the end of the one ahead of the end: the start cap runs
        // into the offset and the end cap out of it, reversed for d > 0.
        for (const auto& [end, outward] : {std::pair{first.point, -first.first}, std::pair{last.point, last.first}}) {
            const double ahead = std::atan2(outward.y, outward.x);
            caps.push_back({end, ahead - 0.5 * M_PI, ahead + 0.5 * M_PI, d > 0.0});
        }
    }
    const OffsetCurves self{source, d, speedFloor, branches};
    const OffsetCurves across{source, -d, speedFloor, opposite};
    const ArcCurves capArcs{caps, std::abs(d)};
    std::vector<std::pair<std::size_t, std::size_t>> links;
    if (!caps.empty()) {
        const size_t firstCap = branches.size() + opposite.size();
        if (branches.front().startEnd == LC_OffsetBranchEnd::Free) {
            links.emplace_back(firstCap, 0);
        }
        if (branches.back().endEnd == LC_OffsetBranchEnd::Free) {
            links.emplace_back(branches.size() - 1, firstCap + 1);
        }
    }
    const CompositeCurves all{{&self, &across, &capArcs}, links};
    const size_t own = branches.size();
    LC_IntersectionOptions query;
    query.tolerance = options.tolerance.nodeMerge;
    query.maxBoxPairs = options.maxIntersectionPairs;
    // how the other side's offset and the circles meet each other is not asked:
    // two arms' offsets on one line, or an open source whose ends coincide,
    // would make that ambiguous without bearing on this offset
    query.anchoredBranches = own;
    const LC_IntersectionResult found = findIntersections(all, query);
    if (found.status != LC_IntersectionStatus::Ok) {
        return fromIntersectionStatus(found.status);
    }
    std::vector<Occurrence> cuts;
    for (const LC_ParametricIntersection& x : found.intersections) {
        // one occurrence on this offset, the other elsewhere (its own crossings are nodes already)
        if (x.branchA < own && x.branchB >= own) {
            cuts.push_back({x.branchA, x.parameterA, x.point});
        }
    }
    LC_CurveOffsetStatus status = insertOccurrences(source, self, d, speedFloor, options, cuts, branches);
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
        bool reversed{false};
        /** Ends at a cusp or a corner's overlapping end, on no boundary. */
        bool dangling{false};
        /** Within the merge tolerance of a point: its neighbours meet in its middle. */
        bool tiny{false};
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
        for (size_t f = first; f < fragments.size(); ++f) {
            fragments[f].reversed = branch.reversed;
        }
        if (!branch.closed) {
            // a cusp or corner end snapped onto a node or cut is a proper join
            const auto turnsBack = [](const LC_OffsetBranchEnd end) { return end != LC_OffsetBranchEnd::Free; };
            fragments[first].dangling =
                turnsBack(branch.startEnd) && !isBoundary(branch.cubicPieces.front().bezier[0]);
            fragments.back().dangling =
                fragments.back().dangling ||
                (turnsBack(branch.endEnd) && !isBoundary(branch.cubicPieces.back().bezier[3]));
        }
    }

    // each fragment by the distance of interior points
    const double marginal =
        16.0 * std::sqrt(2.0 * std::abs(d) * (options.tolerance.requestedGeometry + options.tolerance.classification));
    std::size_t boxes = 0;
    for (Fragment& fragment : fragments) {
        if (fragment.reversed) {
            continue; // nearer than the distance throughout
        }
        double size = 0.0;
        for (const LC_OffsetCubicPiece& piece : fragment.pieces) {
            for (const RS_Vector& v : piece.bezier) {
                size = std::max(size, v.distanceTo(fragment.pieces.front().bezier[0]));
            }
        }
        if (size <= options.tolerance.nodeMerge) {
            fragment.tiny = true; // below the resolution: dropped either way
            continue;
        }
        Visibility seen = Visibility::Undecided;
        RS_Vector probe;
        for (const double where : {0.5, 0.25, 0.75}) {
            const LC_OffsetCubicPiece& piece =
                fragment.pieces[std::min(fragment.pieces.size() - 1,
                                         static_cast<size_t>(where * static_cast<double>(fragment.pieces.size())))];
            const LC_OffsetBranchProvenance& p = piece.provenance;
            const double t = 0.5 * (p.sourceT0 + p.sourceT1);
            if (p.arcCentre.valid) {
                probe = p.arcCentre + RS_Vector{std::cos(t), std::sin(t)} * std::abs(d);
            }
            else {
                OffsetJet q;
                if (computeOffsetJet(source, d, speedFloor, t, LC_CurveEvaluationSide::Interior, q) !=
                    LC_CurveOffsetStatus::Ok) {
                    return LC_CurveOffsetStatus::FitFailed;
                }
                probe = q.point;
            }
            seen = visibility(source, {}, probe, rho, boxes, options.maxDistanceMapBoxes);
            if (seen != Visibility::Undecided) {
                break;
            }
            if (boxes > options.maxDistanceMapBoxes) {
                return LC_CurveOffsetStatus::LimitExceeded;
            }
        }
        if (seen == Visibility::Undecided && !fragment.dangling) {
            return LC_CurveOffsetStatus::AmbiguousTopology;
        }
        if (seen != Visibility::Hidden && fragment.dangling) {
            // A cusp is nearer than the distance, and visibility changes only
            // at boundaries: this fragment is hidden, though by less than the
            // tolerance can show. A stretch hidden by less than tolerance h
            // spans about sqrt(2 |d| h); one much longer means a missed boundary.
            if (size > marginal) {
                return LC_CurveOffsetStatus::AmbiguousTopology;
            }
            seen = Visibility::Hidden;
        }
        fragment.keep = seen == Visibility::Visible;
    }

    // a fragment too small to keep closes up: its neighbours meet in its middle
    for (size_t f = 0; f < fragments.size(); ++f) {
        if (!fragments[f].tiny || f == 0 || f + 1 == fragments.size()) {
            continue;
        }
        RS_Vector& end = fragments[f - 1].pieces.back().bezier[3];
        RS_Vector& start = fragments[f + 1].pieces.front().bezier[0];
        if (end == fragments[f].pieces.front().bezier[0] && start == fragments[f].pieces.back().bezier[3]) {
            const RS_Vector middle = (end + start) * 0.5;
            end = middle;
            start = middle;
        }
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
    result.removedIntervals += removed;
    // a loop that trimming closed, or a closed offset trimming left whole
    if (kept.size() == 1 && kept.front().cubicPieces.back().bezier[3] == kept.front().cubicPieces.front().bezier[0] &&
        (result.removedIntervals > 0 || branches.front().closed)) {
        kept.front().closed = true;
    }
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
                   const std::size_t maxBoxes, const double clearance = 0.0) {
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
            if (gap > clearance && gap > 0.0 && std::hypot(c.x.width(), c.y.width()) < gap) {
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
            // a piece wholly within the clearance: p is on the curve, to that tolerance
            const double fx = std::max(std::abs(c.x.lo() - p.x), std::abs(c.x.hi() - p.x));
            const double fy = std::max(std::abs(c.y.lo() - p.y), std::abs(c.y.hi() - p.y));
            const double mid = a + 0.5 * (b - a);
            if (!(mid > a && mid < b) || (clearance > 0.0 && std::hypot(fx, fy) <= clearance)) {
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

/**
 * A part of a source as a source of its own, with the same parameter. For a
 * part across a closed source's seam (t0 > t1) the parameter runs on past the
 * domain end by the domain's length, so the part is one continuous stretch;
 * sourceParameter() maps it back. The whole domain of a closed source stays
 * closed.
 */
class RestrictedSource final : public OffsetSource {
public:
    RestrictedSource(const OffsetSource& base, const double t0, const double t1) : m_base{base} {
        const std::vector<double>& breaks = base.breaks();
        m_start = breaks.front();
        m_end = breaks.back();
        m_period = m_end - m_start;
        m_closed = base.closed() && t0 == m_start && t1 == m_end;
        m_wraps = t1 < t0;
        add(t0, t0);
        if (!m_wraps) {
            for (const double t : breaks) {
                if (t > t0 && t < t1) {
                    add(t, t);
                }
            }
            add(t1, t1);
            return;
        }
        for (const double t : breaks) {
            if (t > t0 && t < m_end) {
                add(t, t);
            }
        }
        add(m_end, m_end); // the seam, a break inside the part
        for (const double t : breaks) {
            if (t > m_start && t < t1) {
                add(t + m_period, t);
            }
        }
        add(t1 + m_period, t1);
    }

    /**
     * The source's parameter for one of this part. Past the seam the part's
     * parameter is shifted by the domain's length, which rounding can move off
     * a knot: breaks map to the source's exact values, and parameters between
     * them stay within their source span.
     */
    double sourceParameter(const double t) const {
        if (!m_wraps || t <= m_end) {
            return t;
        }
        size_t j = static_cast<size_t>(std::upper_bound(m_breaks.begin(), m_breaks.end(), t) - m_breaks.begin());
        j = std::min(std::max<size_t>(j, 1), m_breaks.size() - 1) - 1;
        if (t == m_breaks[j] || t == m_breaks[j + 1]) {
            return m_sourceBreaks[t == m_breaks[j] ? j : j + 1];
        }
        const double lo = (m_breaks[j] == m_end) ? m_start : m_sourceBreaks[j];
        return std::clamp(t - m_period, lo, m_sourceBreaks[j + 1]);
    }

    /** The source's parameters for the ends of a piece or box of this part: the seam starts one past it. */
    std::pair<double, double> sourceInterval(const double a, const double b) const {
        const bool past = std::max(a, b) > m_end;
        const auto map = [&](const double t) { return (past && t == m_end) ? m_start : sourceParameter(t); };
        return {map(a), map(b)};
    }

    bool closed() const override {
        return m_closed;
    }

    const std::vector<double>& breaks() const override {
        return m_breaks;
    }

    bool jet(const double t, const LC_CurveEvaluationSide side, LC_CurveJet& out) const override {
        if (m_wraps && t == m_end && side == LC_CurveEvaluationSide::Right) {
            return m_base.jet(m_start, side, out); // just past the seam
        }
        return m_base.jet(sourceParameter(t), side, out);
    }

    bool boundJet(const double a, const double b, LC_CurveJetBounds& out) const override {
        const auto [from, to] = sourceInterval(a, b);
        return m_base.boundJet(from, to, out);
    }

    const std::vector<RS_Vector>& hull() const override {
        return m_base.hull();
    }

    bool straightSegment(RS_Vector&, RS_Vector&) const override {
        return false;
    }

    bool cubicSpans() const override {
        return m_base.cubicSpans(); // its spans are parts of the base's, at the same speed
    }

private:
    void add(const double part, const double own) {
        m_breaks.push_back(part);
        m_sourceBreaks.push_back(own);
    }

    const OffsetSource& m_base;
    std::vector<double> m_breaks;
    /** The source's exact parameter for each of m_breaks. */
    std::vector<double> m_sourceBreaks;
    double m_start{0.0};
    double m_end{0.0};
    double m_period{0.0};
    bool m_closed{false};
    bool m_wraps{false};
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
    for (const LC_ParametricIntersection& x : crossings.intersections) {
        cuts.push_back(x.parameterA);
        cuts.push_back(x.parameterB);
    }
    std::sort(cuts.begin(), cuts.end());

    // 2. source fragments between nodes, as parameter intervals (two when one wraps the seam)
    struct SourceFragment {
        std::vector<std::pair<double, double>> intervals;
        bool regionOnLeft{false};
        bool boundary{false};
    };
    std::vector<SourceFragment> fragments;
    size_t wrapFragment = std::numeric_limits<size_t>::max();
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
        wrapFragment = wrap.intervals.empty() ? fragments.size() : fragments.size() - 1;
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
    // Corners of the region are nodes a boundary fragment ends at; a node with
    // every sector on one side is inside or outside the region and needs no arc.
    // Fragment ends are the crossing parameters themselves, compared exactly.
    std::vector<RS_Vector> nodes;
    for (const LC_ParametricIntersection& x : crossings.intersections) {
        const bool corner = std::any_of(boundaryIntervals.begin(), boundaryIntervals.end(), [&](const auto& iv) {
            return iv.first == x.parameterA || iv.second == x.parameterA || iv.first == x.parameterB ||
                   iv.second == x.parameterB;
        });
        const bool known = std::any_of(nodes.begin(), nodes.end(), [&](const RS_Vector& n) {
            return n.distanceTo(x.point) <= options.tolerance.nodeMerge;
        });
        if (corner && !known) {
            nodes.push_back(x.point);
        }
    }

    // 4. candidates: the boundary fragments' offsets outwards or inwards, and circles about the nodes
    struct Candidate {
        std::unique_ptr<RestrictedSource> part;
        double signedDistance;
        bool reverse; // runs with the region on its right
        std::vector<LC_OffsetBranch> branches;
    };
    std::vector<Candidate> candidates;
    for (size_t f = 0; f < fragments.size(); ++f) {
        const SourceFragment& fragment = fragments[f];
        if (!fragment.boundary) {
            continue;
        }
        // one stretch: a fragment across the seam runs on through it
        std::vector<std::pair<double, double>> stretches = fragment.intervals;
        if (f == wrapFragment && stretches.size() == 2) {
            stretches = {{stretches[0].first, stretches[1].second}};
        }
        for (const auto& [a, b] : stretches) {
            Candidate candidate;
            candidate.part = std::make_unique<RestrictedSource>(source, a, b);
            // outward is right of a boundary with the region on its left
            candidate.signedDistance = (dilate == fragment.regionOnLeft) ? -magnitude : magnitude;
            candidate.reverse = !fragment.regionOnLeft;
            double ignoredError = 0.0;
            std::size_t ignoredSamples = 0;
            // the region's corners come from its circles, and a kink is refused
            const LC_CurveOffsetStatus status = directBranches(*candidate.part, scale, candidate.signedDistance,
                                                               options, budget, KinkPolicy::Refuse,
                                                               candidate.branches, ignoredError, ignoredSamples);
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
        const LC_CurveOffsetStatus status = insertOccurrences(*candidates[c].part, *offsetCurves[c],
                                                              candidates[c].signedDistance, speedFloor, options,
                                                              offsetCuts[c], candidates[c].branches);
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
                // back to the source's own parameters and spans
                for (LC_OffsetCubicPiece& p : run) {
                    LC_OffsetBranchProvenance& v = p.provenance;
                    std::tie(v.sourceT0, v.sourceT1) = candidate.part->sourceInterval(v.sourceT0, v.sourceT1);
                    const double middle = 0.5 * (v.sourceT0 + v.sourceT1);
                    v.sourceSpan = static_cast<size_t>(std::upper_bound(breaks.begin(), breaks.end(), middle) -
                                                       breaks.begin()) - 1;
                }
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
            size_t next = pieces.size();
            for (const size_t k : kept) {
                if (!used[k] && pieces[k].pieces.front().bezier[0] == end) {
                    if (next != pieces.size()) {
                        return LC_CurveOffsetStatus::AmbiguousTopology; // cycles touching at a point
                    }
                    next = k;
                }
            }
            const bool closes = end == cycle.cubicPieces.front().bezier[0];
            if (closes && next != pieces.size()) {
                return LC_CurveOffsetStatus::AmbiguousTopology; // another cycle touches it at its start
            }
            if (closes) {
                break;
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
    options.maxIntersectionPairs = kDefaultMaxIntersectionPairs;
    options.maxDistanceMapBoxes = 65536;
    options.maxArrangementEdges = 65536;
    options.maxArrangementFaces = 65536;
    return options;
}

LC_CurveOffsetOptions makeOffsetOptions(const RS_Entity& source, const double distanceMagnitude,
                                        const double requestedTolerance) {
    LC_CurveOffsetOptions options = makeDirectOptions(source, distanceMagnitude, requestedTolerance);
    options.mode = LC_CurveOffsetMode::Trimmed;
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
            // as for the other modes, a point on the curve, to the tolerance, has no side
            if (!windingNumber(*adapter, request.directionPoint, winding, boxes, options.maxDistanceMapBoxes,
                               options.tolerance.classification)) {
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
        directBranches(*adapter, scale, d, options, budget, KinkPolicy::Round, branches, maxError,
                       result.exactSamples);
    const bool trimmed = options.mode == LC_CurveOffsetMode::Trimmed;
    if (status == LC_CurveOffsetStatus::Ok && trimmed) {
        std::size_t removed = 0;
        status = removeLocalLoops(*adapter, d, speedFloor, options, branches, removed);
        collapseTinyReversals(branches, options.tolerance.nodeMerge);
        // any reversed branch left is hidden throughout, and dropped before
        // noding: next to a cusp the offset's two arms are too close to tell
        // apart, far from the origin in particular
        const std::size_t before = branches.size();
        branches.erase(std::remove_if(branches.begin(), branches.end(),
                                      [](const LC_OffsetBranch& b) { return b.reversed; }),
                       branches.end());
        result.removedIntervals = removed + before - branches.size();
        if (branches.empty()) {
            result.status = LC_CurveOffsetStatus::Ok;
            return result;
        }
    }
    if (status == LC_CurveOffsetStatus::Ok && (options.nodeIntersections || trimmed) && !branches.front().straight) {
        status = nodeBranches(*adapter, d, speedFloor, options, branches, result);
    }
    if (status == LC_CurveOffsetStatus::Ok && trimmed) {
        status = trimBranches(*adapter, scale, d, options, branches, result);
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
    std::vector<std::vector<LC_OffsetCubicPiece>> validated;
    double maxError = 0.0;
    std::size_t pieceCount = 0;
    std::size_t samples = geometry.exactSamples; // the limit is per source, over all branches
    for (const LC_OffsetBranch& branch : geometry.branches) {
        const std::size_t left = budget.maxCubicPieces - std::min(budget.maxCubicPieces, pieceCount);
        Materializer materializer{*adapter, geometry.signedDistance, options, left, scale.numericFloor / domain,
                                  samples};
        std::vector<LC_OffsetCubicPiece> pieces;
        double branchError = 0.0;
        const LC_CurveOffsetStatus status = materializer.run(branch, pieces, branchError);
        if (status != LC_CurveOffsetStatus::Ok) {
            result.status = status;
            return result;
        }
        samples = materializer.samples();
        pieceCount += pieces.size();
        maxError = std::max(maxError, branchError);
        validated.push_back(std::move(pieces));
    }

    if (options.nodeIntersections || options.mode != LC_CurveOffsetMode::Direct) {
        // The fitted pieces, not only the exact offset, may cross only at its
        // nodes: a fit within tolerance can still add a crossing.
        std::vector<std::vector<std::array<RS_Vector, 4>>> nets(validated.size());
        for (size_t b = 0; b < validated.size(); ++b) {
            for (const LC_OffsetCubicPiece& piece : validated[b]) {
                nets[b].push_back(piece.bezier);
            }
        }
        const PieceCurves curves{nets, branchSuccessors(geometry.branches)};
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

    // one entity for each branch
    std::vector<std::unique_ptr<RS_Entity>> entities;
    LC_OffsetOutputUsage usage;
    usage.cubicPieces = pieceCount;
    for (size_t b = 0; b < validated.size(); ++b) {
        if (usage.outputEntities >= budget.maxOutputEntities) {
            result.status = LC_CurveOffsetStatus::LimitExceeded;
            return result;
        }
        std::unique_ptr<RS_Entity> entity =
            chainEntity(validated[b], geometry.branches[b].closed, options, scale.numericFloor);
        if (entity == nullptr) {
            result.status = LC_CurveOffsetStatus::FitFailed;
            return result;
        }
        const LC_OffsetTreeCost cost =
            measureOffsetOutput({entity.get()}, budget.maxDeepEntities - std::min(budget.maxDeepEntities,
                                                                                usage.deepEntities));
        if (cost.status != LC_OffsetTreeStatus::Ok) {
            result.status = cost.status == LC_OffsetTreeStatus::LimitExceeded ? LC_CurveOffsetStatus::LimitExceeded
                                                                              : LC_CurveOffsetStatus::FitFailed;
            return result;
        }
        usage.deepEntities += cost.deepEntities;
        ++usage.outputEntities;
        entities.push_back(std::move(entity));
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
        createEntities(source, makeDirectionRequest(coord, magnitude), makeOffsetOptions(source, magnitude),
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
