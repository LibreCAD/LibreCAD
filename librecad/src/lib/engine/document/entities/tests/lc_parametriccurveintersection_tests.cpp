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

// Parameter-pair intersections of parametric branches, and noded offsets (T1).

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_parametriccurveintersection.h"
#include "rs_spline.h"

using Catch::Approx;
using Kind = LC_ParametricIntersection::Kind;

namespace {

RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls, const std::vector<double>& knots) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights.assign(controls.size(), 1.0);
    return RS_Spline(nullptr, data);
}

RS_Spline line(const RS_Vector& a, const RS_Vector& b) {
    return makeSpline(1, {a, b}, {0, 0, 1, 1});
}

/**
 * How many piece ends of the output lie at @p p: an entity's own ends, and
 * twice every joint inside a spline that is one chain of pieces, where one
 * piece ends and the next starts (every control point of a degree-1 spline,
 * every third of a cubic one with triple knots).
 */
size_t endsAt(const std::vector<std::unique_ptr<RS_Entity>>& entities, const RS_Vector& p) {
    size_t ends = 0;
    for (const std::unique_ptr<RS_Entity>& e : entities) {
        ends += (e->getStartpoint() == p) ? 1 : 0;
        ends += (e->getEndpoint() == p) ? 1 : 0;
        if (const auto* spline = dynamic_cast<const RS_Spline*>(e.get())) {
            const std::vector<RS_Vector>& c = spline->getData().controlPoints;
            const size_t step = static_cast<size_t>(spline->getDegree());
            for (size_t k = step; k + step < c.size(); k += step) {
                ends += (c[k] == p) ? 2 : 0;
            }
        }
    }
    return ends;
}

/** Each spline one branch, each of its knot spans one segment. */
class SplineCurves final : public LC_ParametricCurves {
public:
    explicit SplineCurves(std::vector<const RS_Spline*> splines, const bool looseBounds = false)
        : m_splines{std::move(splines)}, m_looseBounds{looseBounds} {
        for (size_t b = 0; b < m_splines.size(); ++b) {
            const std::vector<double> breaks = m_splines[b]->getBreakParameters();
            for (size_t i = 0; i + 1 < breaks.size(); ++i) {
                m_segments.push_back({b, breaks[i], breaks[i + 1]});
            }
        }
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return m_splines.size();
    }

    std::ptrdiff_t next(const std::size_t branch) const override {
        return m_splines[branch]->isClosed() ? static_cast<std::ptrdiff_t>(branch) : -1;
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        const LC_ParametricSegment& s = m_segments[segment];
        const LC_CurveEvaluationSide side = (t == s.t0)   ? LC_CurveEvaluationSide::Right
                                            : (t == s.t1) ? LC_CurveEvaluationSide::Left
                                                          : LC_CurveEvaluationSide::Interior;
        LC_CurveJet jet;
        if (!m_splines[s.branch]->tryEvaluateJet(t, side, jet)) {
            return false;
        }
        point = jet.point;
        derivative = jet.first;
        return true;
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        LC_CurveJetBounds bounds;
        if (!m_splines[m_segments[segment].branch]->tryBoundJet(a, b, bounds)) {
            return false;
        }
        x = bounds.x;
        y = bounds.y;
        if (m_looseBounds) {
            x = LC_Interval::hull(x, LC_Interval::hull(-1e6, 1e6));
            y = LC_Interval::hull(y, LC_Interval::hull(-1e6, 1e6));
        }
        dx = bounds.dx;
        dy = bounds.dy;
        return true;
    }

private:
    std::vector<const RS_Spline*> m_splines;
    std::vector<LC_ParametricSegment> m_segments;
    bool m_looseBounds;
};

/** Two lines with exact bounds, independent of spline evaluation and normalization. */
class AnalyticLines final : public LC_ParametricCurves {
public:
    AnalyticLines(const RS_Vector& first, const RS_Vector& second, const double t0 = 0.0, const double t1 = 1.0)
        : m_directions{first, second}, m_segments{{0, t0, t1}, {1, t0, t1}}, m_middle{t0 + 0.5 * (t1 - t0)} {
    }

    const std::vector<LC_ParametricSegment>& segments() const override {
        return m_segments;
    }

    std::size_t branchCount() const override {
        return 2;
    }

    std::ptrdiff_t next(const std::size_t) const override {
        return -1;
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        derivative = m_directions[segment];
        point = {(t - m_middle) * derivative.x, (t - m_middle) * derivative.y};
        return true;
    }

    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        const RS_Vector& direction = m_directions[segment];
        x = LC_Interval::hull((a - m_middle) * direction.x, (b - m_middle) * direction.x);
        y = LC_Interval::hull((a - m_middle) * direction.y, (b - m_middle) * direction.y);
        dx = LC_Interval::point(direction.x);
        dy = LC_Interval::point(direction.y);
        return true;
    }

private:
    RS_Vector m_directions[2];
    std::vector<LC_ParametricSegment> m_segments;
    double m_middle;
};

struct PolynomialPiece {
    LC_ParametricSegment segment;
    std::array<double, 4> x;
    std::array<double, 4> y;
};

PolynomialPiece polynomialLine(const std::size_t branch, const RS_Vector& origin, const RS_Vector& direction,
                               const double from = 0.0, const double to = 1.0) {
    return {{branch, from, to}, {origin.x, direction.x, 0.0, 0.0}, {origin.y, direction.y, 0.0, 0.0}};
}

/** Exact polynomial coefficients, independently enclosed by interval Horner arithmetic. */
class CertifiedPolynomials final : public LC_ParametricCurves {
public:
    explicit CertifiedPolynomials(std::vector<PolynomialPiece> pieces, const std::size_t branches = 0)
        : m_pieces{std::move(pieces)} {
        for (const auto& piece : m_pieces) {
            m_segments.push_back(piece.segment);
        }
        m_branches = branches != 0 ? branches : m_segments.empty() ? 0 : m_segments.back().branch + 1;
    }
    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return m_branches; }
    std::ptrdiff_t next(const std::size_t branch) const override {
        return successor ? successor(branch) : closed ? static_cast<std::ptrdiff_t>(branch) : -1;
    }
    bool evaluate(const std::size_t segment, const double t, RS_Vector& p, RS_Vector& d) const override {
        const auto& piece = m_pieces[segment];
        const auto value = [t](const auto& c) { return c[0] + t * (c[1] + t * (c[2] + t * c[3])); };
        const auto derivative = [t](const auto& c) { return c[1] + t * (2.0 * c[2] + t * 3.0 * c[3]); };
        p = RS_Vector{value(piece.x) + seedShift, value(piece.y)};
        d = RS_Vector{derivative(piece.x), derivative(piece.y)};
        if (invalidSeed) {
            p.x = std::numeric_limits<double>::quiet_NaN();
        }
        if (outOfPlaneSeed) {
            p.z = 1.0;
        }
        return true;
    }
    bool bound(const std::size_t segment, const double from, const double to, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        if (missingPointBounds && from == to) {
            return false;
        }
        const auto& piece = m_pieces[segment];
        const LC_Interval t = LC_Interval::hull(from, to);
        const auto pt = [](const double value) { return LC_Interval::point(value); };
        const auto value = [&](const auto& c) { return pt(c[0]) + t * (pt(c[1]) + t * (pt(c[2]) + t * pt(c[3]))); };
        const auto derivative = [&](const auto& c) {
            return pt(c[1]) + t * (pt(2.0) * pt(c[2]) + t * pt(3.0) * pt(c[3]));
        };
        x = value(piece.x);
        y = value(piece.y);
        dx = derivative(piece.x);
        dy = derivative(piece.y);
        if (unbounded) {
            x = LC_Interval::entire();
        }
        return true;
    }
    LC_AnalyticLeafCertificate certifyInjective(const std::size_t segment, const LC_Interval& parameter) const override {
        return leafProof ? leafProof(segment, parameter) : LC_AnalyticLeafCertificate{};
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        return contactProof ? contactProof(box) : LC_AnalyticIntersectionCertificate{};
    }

    bool missingPointBounds{false};
    bool unbounded{false};
    bool invalidSeed{false};
    bool outOfPlaneSeed{false};
    bool closed{false};
    double seedShift{0.0};
    std::function<std::ptrdiff_t(std::size_t)> successor;
    std::function<LC_AnalyticLeafCertificate(std::size_t, const LC_Interval&)> leafProof;
    std::function<LC_AnalyticIntersectionCertificate(const LC_IntersectionDomain&)> contactProof;

private:
    std::vector<PolynomialPiece> m_pieces;
    std::vector<LC_ParametricSegment> m_segments;
    std::size_t m_branches{0};
};

/** Records actual bound requests; hooks may loosen valid bounds or refuse a query. */
class AccountingCurves final : public LC_ParametricCurves {
public:
    struct BoundCall {
        std::size_t segment;
        double from;
        double to;
    };

    explicit AccountingCurves(std::vector<PolynomialPiece> pieces) : curves{std::move(pieces)} {}
    const std::vector<LC_ParametricSegment>& segments() const override { return curves.segments(); }
    std::size_t branchCount() const override { return curves.branchCount(); }
    std::ptrdiff_t next(const std::size_t branch) const override { return curves.next(branch); }
    bool evaluate(const std::size_t segment, const double t, RS_Vector& p, RS_Vector& d) const override {
        return curves.evaluate(segment, t, p, d);
    }
    bool bound(const std::size_t segment, const double from, const double to, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        calls.push_back({segment, from, to});
        return curves.bound(segment, from, to, x, y, dx, dy) &&
               (!derivativeBounds || derivativeBounds(segment, from, to, dx, dy));
    }
    LC_AnalyticLeafCertificate certifyInjective(const std::size_t segment, const LC_Interval& parameter) const override {
        return curves.certifyInjective(segment, parameter);
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        return curves.certifyIntersection(box);
    }

    CertifiedPolynomials curves;
    mutable std::vector<BoundCall> calls;
    std::function<bool(std::size_t, double, double, LC_Interval&, LC_Interval&)> derivativeBounds;
};

// Recognize exact dyadic cells from the adapter's domain, not from solver depth/statistics.
bool examinedDepth(const AccountingCurves::BoundCall& call, const LC_ParametricSegment& domain, unsigned& depth) {
    if (!(call.from < call.to)) {
        return false;
    }
    double lo = domain.t0;
    double hi = domain.t1;
    for (depth = 0; depth <= 32; ++depth) {
        if (call.from == lo && call.to == hi) {
            return true;
        }
        const double middle = lo + 0.5 * (hi - lo);
        if (!(lo < middle && middle < hi)) {
            return false;
        }
        if (call.to <= middle) {
            hi = middle;
        } else if (call.from >= middle) {
            lo = middle;
        } else {
            return false;
        }
    }
    return false;
}

unsigned preparationTraceDepth(const AccountingCurves& curves) {
    unsigned deepest = 0;
    for (const auto& call : curves.calls) {
        unsigned depth = 0;
        if (examinedDepth(call, curves.segments()[call.segment], depth)) {
            deepest = std::max(deepest, depth);
        }
    }
    return deepest;
}

unsigned pairTraceDepth(const AccountingCurves& curves) {
    unsigned deepest = 0;
    for (std::size_t i = 1; i < curves.calls.size(); ++i) {
        const auto& a = curves.calls[i - 1];
        const auto& b = curves.calls[i];
        unsigned da = 0;
        unsigned db = 0;
        if (a.segment == 0 && b.segment == 1 && examinedDepth(a, curves.segments()[0], da) &&
            examinedDepth(b, curves.segments()[1], db)) {
            deepest = std::max(deepest, da + db);
        }
    }
    return deepest;
}

std::size_t orderingPointCalls(const AccountingCurves& curves) {
    // The endpoint-certificate fixture contracts at 3/8, never a preparation endpoint/midpoint.
    return static_cast<std::size_t>(std::count_if(curves.calls.begin(), curves.calls.end(), [](const auto& call) {
        return call.segment == 0 && call.from == 0.375 && call.to == call.from;
    }));
}

struct RefineProbe {
    std::size_t destroyed{0};
    std::size_t bounds{0};
    std::size_t evaluations{0};
    bool invalidBounds{false};
    bool throwBounds{false};
    bool translated{false};
    bool unavailable{false};
    bool wrongUniqueCategory{false};
    bool wrongExcludedCategory{false};
    std::size_t certificates{0};
    std::exception_ptr certificateException;
};

/** Owns all callback dependencies. Fault switches intentionally violate immutability to test fail-closed behavior. */
class RefineCurves final : public LC_ParametricCurves {
public:
    RefineCurves(std::vector<PolynomialPiece> pieces, std::shared_ptr<RefineProbe> probe)
        : curves{std::move(pieces)}, m_probe{std::move(probe)} {}
    ~RefineCurves() override { ++m_probe->destroyed; }
    const std::vector<LC_ParametricSegment>& segments() const override { return curves.segments(); }
    std::size_t branchCount() const override { return curves.branchCount(); }
    std::ptrdiff_t next(const std::size_t branch) const override { return curves.next(branch); }
    bool evaluate(const std::size_t segment, const double t, RS_Vector& p, RS_Vector& d) const override {
        ++m_probe->evaluations;
        if (!curves.evaluate(segment, t, p, d)) { return false; }
        if (m_probe->translated) { p.x += 100.0; }
        return true;
    }
    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        ++m_probe->bounds;
        if (m_probe->throwBounds) { throw std::runtime_error("test provider failure"); }
        if (!curves.bound(segment, a, b, x, y, dx, dy)) { return false; }
        if (m_probe->invalidBounds) { x = LC_Interval::entire(); }
        if (m_probe->translated) { x = x + LC_Interval::point(100.0); }
        return true;
    }
    LC_AnalyticLeafCertificate certifyInjective(const std::size_t segment, const LC_Interval& parameter) const override {
        return curves.certifyInjective(segment, parameter);
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        ++m_probe->certificates;
        if (m_probe->certificateException) { std::rethrow_exception(m_probe->certificateException); }
        if (m_probe->unavailable) { return {}; }
        auto result = curves.certifyIntersection(box);
        if (m_probe->wrongUniqueCategory) {
            result.proof = LC_IntersectionProof::Uncertified;
        }
        if (m_probe->wrongExcludedCategory) {
            result.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            result.proof = LC_IntersectionProof::AnalyticTangency;
        }
        return result;
    }
    CertifiedPolynomials curves;

private:
    std::shared_ptr<RefineProbe> m_probe;
};

std::unique_ptr<RefineCurves> endpointRefineCurves(
    const std::vector<double>& roots = {0.25}, std::shared_ptr<RefineProbe> probe = std::make_shared<RefineProbe>()) {
    std::vector<PolynomialPiece> pieces{polynomialLine(0, {0, 0}, {1, 0})};
    for (std::size_t i = 0; i < roots.size(); ++i) {
        pieces.push_back(polynomialLine(i + 1, {roots[i], 0}, {0, 1}));
    }
    auto curves = std::make_unique<RefineCurves>(std::move(pieces), std::move(probe));
    curves->curves.contactProof = [roots](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate proof;
        if (box.segmentA != 0 || box.segmentB == 0 || box.segmentB > roots.size()) { return proof; }
        proof.proofDomain = box;
        const double root = roots[box.segmentB - 1];
        if (!box.parameterA.contains(root) || !box.parameterB.contains(0.0)) {
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            proof.proof = LC_IntersectionProof::AnalyticExclusion;
        } else {
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            proof.proof = LC_IntersectionProof::AnalyticEndpoint;
            proof.kind = Kind::Endpoint;
            proof.isolationA = box.parameterA;
            proof.isolationB = LC_Interval::point(0.0);
        }
        return proof;
    };
    return curves;
}

std::unique_ptr<RefineCurves> singularRefineCurves(
    const bool cusp, std::shared_ptr<RefineProbe> probe = std::make_shared<RefineProbe>()) {
    PolynomialPiece other{{1, 0, 1}, cusp ? std::array<double, 4>{0.375, 0.75, -1.5, 1.0} :
                                                         std::array<double, 4>{0, 1, 0, 0},
                           {0.25, -1, 1, 0}};
    auto curves = std::make_unique<RefineCurves>(
        std::vector<PolynomialPiece>{polynomialLine(0, {0, 0}, {1, 0}), other}, std::move(probe));
    curves->curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
        // x=t or x=(t-.5)^3+.5 is strictly increasing, including the cusp's stationary point.
        return LC_AnalyticLeafCertificate{true, segment, parameter};
    };
    curves->curves.contactProof = [cusp](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate proof;
        if (box.segmentA != 0 || box.segmentB != 1) { return proof; }
        proof.proofDomain = box;
        if (!box.parameterA.contains(0.5) || !box.parameterB.contains(0.5)) {
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            proof.proof = LC_IntersectionProof::AnalyticExclusion;
        } else {
            // y=(u-.5)^2 proves the only root is t=u=.5; deliberately return a qualified enclosure, not its point.
            const auto inner = [](const LC_Interval& interval) {
                const double radius = 0.25 * interval.width();
                return LC_Interval::hull(std::max(interval.lo(), 0.5 - radius),
                                         std::min(interval.hi(), 0.5 + radius));
            };
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            proof.proof = cusp ? LC_IntersectionProof::AnalyticCusp : LC_IntersectionProof::AnalyticTangency;
            proof.kind = Kind::Tangent;
            proof.isolationA = inner(box.parameterA);
            proof.isolationB = inner(box.parameterB);
        }
        return proof;
    };
    return curves;
}

LC_CertifiedContractionAllowance refineAllowance(const unsigned depth = 48) {
    return {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), depth};
}

void checkSameInterval(const LC_Interval& a, const LC_Interval& b) {
    CHECK(a.lo() == b.lo());
    CHECK(a.hi() == b.hi());
}

void checkSameDomain(const LC_IntersectionDomain& a, const LC_IntersectionDomain& b) {
    CHECK(a.segmentA == b.segmentA);
    CHECK(a.segmentB == b.segmentB);
    checkSameInterval(a.parameterA, b.parameterA);
    checkSameInterval(a.parameterB, b.parameterB);
}

LC_IntersectionOptions strictOptions() {
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.strict = true;
    return options;
}

void checkStrictWork(const LC_IntersectionResult& result, const LC_IntersectionOptions& options) {
    CHECK(result.work.totalBoxPairs == result.work.candidateBoxPairs + result.work.refinementBoxPairs +
                                      result.work.certificateBoxPairs);
    CHECK(result.work.totalBoxPairs <= options.maxBoxPairs);
    CHECK(result.work.generatedItems <= options.maxGeneratedItems);
    CHECK(result.work.generatedLeaves <= options.maxLeaves);
    CHECK(result.work.orderingComparisons <= options.maxOrderingComparisons);
    CHECK(result.work.validationSegmentVisits <= options.maxGeneratedItems);
    CHECK(result.work.validationNextCalls <= result.work.validationSegmentVisits);
    if (result.status != LC_IntersectionStatus::Ok) {
        CHECK(result.intersections.empty());
    }
    for (const auto& event : result.intersections) {
        CHECK(event.proof != LC_IntersectionProof::Uncertified);
        CHECK(event.isolation.parameterA.isValid());
        CHECK(event.isolation.parameterB.isValid());
        CHECK(event.proofDomain.parameterA.contains(event.isolation.parameterA.lo()));
        CHECK(event.proofDomain.parameterA.contains(event.isolation.parameterA.hi()));
        CHECK(event.proofDomain.parameterB.contains(event.isolation.parameterB.lo()));
        CHECK(event.proofDomain.parameterB.contains(event.isolation.parameterB.hi()));
    }
}

struct StrictBudgetProbe {
    std::size_t segments{0}, branches{0}, next{0}, evaluations{0}, bounds{0};
    std::size_t leaves{0}, certificates{0}, identities{0};
    bool unavailable{false}, identityUnavailable{false};
    bool malformed{false}, oversizedBranches{false};
    std::exception_ptr exception;
    enum class ThrowAt { None, Segments, Branches, Next, Bound, Evaluate, Leaf, Certificate, Identity };
    ThrowAt throwAt{ThrowAt::None};
    void check(const ThrowAt at) const { if (throwAt == at) { std::rethrow_exception(exception); } }
};

/** Exact lines, with optional conservative derivative bounds; no spline or solver dependencies. */
class StrictBudgetCurves final : public LC_ParametricCurves {
public:
    enum class Shape { Straight, LeafSplit, PairSplit, Triple };
    StrictBudgetCurves(const Shape shape, std::shared_ptr<StrictBudgetProbe> probe, const std::size_t count = 2)
        : m_shape{shape}, m_probe{std::move(probe)} {
        const auto n = shape == Shape::Triple ? 3 : count;
        for (std::size_t i = 0; i < n; ++i) {
            const bool branch = shape == Shape::Triple || shape == Shape::PairSplit;
            m_segments.push_back({branch ? i : 0, shape == Shape::Triple ? 0.5 : branch ? 0.0 : double(i),
                                  branch ? 1.0 : double(i + 1)});
        }
        if (m_probe->malformed && m_segments.size() > 1) { m_segments[1].t1 = m_segments[1].t0; }
    }
    const std::vector<LC_ParametricSegment>& segments() const override {
        ++m_probe->segments; m_probe->check(StrictBudgetProbe::ThrowAt::Segments); return m_segments;
    }
    std::size_t branchCount() const override {
        ++m_probe->branches; m_probe->check(StrictBudgetProbe::ThrowAt::Branches);
        if (m_probe->oversizedBranches) { return std::numeric_limits<std::size_t>::max(); }
        return m_shape == Shape::Triple || m_shape == Shape::PairSplit ? m_segments.size() : 1;
    }
    std::ptrdiff_t next(std::size_t) const override {
        ++m_probe->next; m_probe->check(StrictBudgetProbe::ThrowAt::Next); return -1;
    }
    bool evaluate(const std::size_t segment, const double t, RS_Vector& p, RS_Vector& d) const override {
        ++m_probe->evaluations; m_probe->check(StrictBudgetProbe::ThrowAt::Evaluate);
        const auto slope = m_shape == Shape::Triple ? double(segment) - 1 : 0.0;
        p = {t, slope * (t - 0.5)}; d = {1, slope}; return true;
    }
    bool bound(const std::size_t segment, const double a, const double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override {
        ++m_probe->bounds; m_probe->check(StrictBudgetProbe::ThrowAt::Bound);
        const auto slope = m_shape == Shape::Triple ? double(segment) - 1 : 0.0;
        x = LC_Interval::hull(a, b); y = LC_Interval::hull(slope * (a - 0.5), slope * (b - 0.5));
        dx = LC_Interval::point(1); dy = LC_Interval::point(slope);
        if (m_shape == Shape::LeafSplit && b - a > 0.5) { dx = dy = LC_Interval::hull(-1, 1); }
        return true;
    }
    LC_AnalyticLeafCertificate certifyInjective(std::size_t, const LC_Interval&) const override {
        ++m_probe->leaves; m_probe->check(StrictBudgetProbe::ThrowAt::Leaf); return {};
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        ++m_probe->certificates; m_probe->check(StrictBudgetProbe::ThrowAt::Certificate);
        if (m_shape != Shape::Triple || m_probe->unavailable) { return {}; }
        LC_AnalyticIntersectionCertificate result;
        // Distinct slopes meet only at the three free starts t=u=.5, even in these wide isolations.
        result.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
        result.proof = LC_IntersectionProof::AnalyticEndpoint; result.kind = Kind::Endpoint;
        result.proofDomain = box;
        result.isolationA = result.isolationB = LC_Interval::hull(0.5, 0.75);
        return result;
    }
    LC_AnalyticOccurrenceIdentityCertificate certifyOccurrenceIdentity(
        const LC_ParametricIntersection& a, const bool firstA,
        const LC_ParametricIntersection& b, const bool firstB) const override {
        ++m_probe->identities; m_probe->check(StrictBudgetProbe::ThrowAt::Identity);
        // These two provider-internal proofs belong to the one solver-dispatched identity callback.
        const auto ca = certifyIntersection(a.isolation);
        const auto cb = certifyIntersection(b.isolation);
        if (m_probe->identityUnavailable || ca.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique ||
            cb.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique ||
            (firstA ? a.branchA : a.branchB) != (firstB ? b.branchA : b.branchB)) { return {}; }
        return {true, a.isolation, b.isolation, LC_Interval::point(0.5)};
    }
private:
    Shape m_shape;
    std::shared_ptr<StrictBudgetProbe> m_probe;
    std::vector<LC_ParametricSegment> m_segments;
};

void checkSameStrictWork(const LC_IntersectionWorkStats& a, const LC_IntersectionWorkStats& b) {
    CHECK(a.candidateBoxPairs == b.candidateBoxPairs);
    CHECK(a.refinementBoxPairs == b.refinementBoxPairs);
    CHECK(a.certificateBoxPairs == b.certificateBoxPairs);
    CHECK(a.totalBoxPairs == b.totalBoxPairs);
    CHECK(a.generatedLeaves == b.generatedLeaves);
    CHECK(a.generatedItems == b.generatedItems);
    CHECK(a.deepestPreparation == b.deepestPreparation);
    CHECK(a.deepestPairRefinement == b.deepestPairRefinement);
    CHECK(a.deepestOrderingRefinement == b.deepestOrderingRefinement);
    CHECK(a.deepestRootRefinement == b.deepestRootRefinement);
    CHECK(a.orderingComparisons == b.orderingComparisons);
    CHECK(a.validationSegmentVisits == b.validationSegmentVisits);
    CHECK(a.validationNextCalls == b.validationNextCalls);
}

LC_IntersectionResult intersect(std::vector<const RS_Spline*> splines, const double tolerance = 1e-9) {
    LC_IntersectionOptions options;
    options.tolerance = tolerance;
    return findIntersections(SplineCurves{std::move(splines)}, options);
}

void checkWork(const LC_IntersectionResult& result, const LC_IntersectionOptions& options) {
    REQUIRE(result.work.candidateBoxPairs <= options.maxBoxPairs);
    REQUIRE(result.work.refinementBoxPairs <= options.maxBoxPairs - result.work.candidateBoxPairs);
    CHECK(result.work.totalBoxPairs == result.work.candidateBoxPairs + result.work.refinementBoxPairs);
    CHECK(result.work.totalBoxPairs <= options.maxBoxPairs);
    CHECK(result.work.generatedLeaves <= options.maxLeaves);
}

/** Crossings of polylines through dense samples of each spline: an independent count. */
size_t sampledCrossings(const std::vector<const RS_Spline*>& splines) {
    std::vector<std::vector<RS_Vector>> polylines;
    for (const RS_Spline* s : splines) {
        double t0 = 0.0;
        double t1 = 0.0;
        REQUIRE(s->getParameterDomain(t0, t1));
        std::vector<RS_Vector> points;
        for (int i = 0; i <= 1000; ++i) {
            points.push_back(s->getPointAt(t0 + (t1 - t0) * i / 1000.0));
        }
        polylines.push_back(points);
    }
    auto cross = [](const RS_Vector& a, const RS_Vector& b) { return a.x * b.y - a.y * b.x; };
    size_t count = 0;
    for (size_t p = 0; p < polylines.size(); ++p) {
        for (size_t q = p; q < polylines.size(); ++q) {
            for (size_t i = 0; i + 1 < polylines[p].size(); ++i) {
                for (size_t j = (p == q) ? i + 2 : 0; j + 1 < polylines[q].size(); ++j) {
                    if (p == q && splines[p]->isClosed() && i == 0 && j + 2 == polylines[q].size()) {
                        continue; // the seam joins them
                    }
                    const RS_Vector a = polylines[p][i];
                    const RS_Vector r = polylines[p][i + 1] - a;
                    const RS_Vector c = polylines[q][j];
                    const RS_Vector s = polylines[q][j + 1] - c;
                    const double d = cross(r, s);
                    if (d == 0.0) {
                        continue;
                    }
                    const double u = cross(c - a, s) / d;
                    const double v = cross(c - a, r) / d;
                    count += (u >= 0.0 && u < 1.0 && v >= 0.0 && v < 1.0) ? 1 : 0;
                }
            }
        }
    }
    return count;
}

} // namespace

static_assert(!std::is_default_constructible_v<LC_CertifiedIntersectionSession>);
static_assert(!std::is_copy_constructible_v<LC_CertifiedIntersectionSession>);
static_assert(!std::is_copy_assignable_v<LC_CertifiedIntersectionSession>);
static_assert(std::is_nothrow_move_constructible_v<LC_CertifiedIntersectionSession>);
static_assert(std::is_nothrow_move_assignable_v<LC_CertifiedIntersectionSession>);
static_assert(!std::is_constructible_v<LC_CertifiedRootHandle, LC_ParametricIntersection>);
static_assert(!std::is_constructible_v<LC_CertifiedRootHandle, LC_CertifiedRootSnapshot>);
static_assert(!std::is_constructible_v<LC_CertifiedRootHandle, std::size_t>);

TEST_CASE("Certified sessions charge fixed and retained metadata without changing legacy work",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto options = strictOptions();
    const auto nullProvider = findCertifiedIntersections(nullptr, options);
    CHECK(nullProvider.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(nullProvider.session);
    CHECK(nullProvider.work.generatedItems == 0);
    options.tolerance = 0;
    const auto invalid = findCertifiedIntersections(endpointRefineCurves(), options);
    CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
    CHECK(invalid.work.generatedItems == 0);

    options = strictOptions();
    options.maxGeneratedItems = 4; // session + issuer + preparation root + accepted leaf, with no contacts
    const auto empty = findCertifiedIntersections(
        std::make_unique<CertifiedPolynomials>(std::vector<PolynomialPiece>{polynomialLine(0, {0, 0}, {1, 0})}), options);
    REQUIRE(empty.status == LC_IntersectionStatus::Ok);
    REQUIRE(empty.session);
    CHECK(empty.session->rootCount() == 0);
    CHECK(empty.work.generatedItems == 4);
    CHECK(empty.work.totalBoxPairs == 0);
    CHECK_FALSE(empty.session->snapshot(empty.session->rootHandle(0)));

    for (const std::size_t cap : {1u, 2u}) {
        options.maxGeneratedItems = cap;
        const auto shortMetadata = findCertifiedIntersections(
            std::make_unique<CertifiedPolynomials>(std::vector<PolynomialPiece>{polynomialLine(0, {0, 0}, {1, 0})}), options);
        CHECK(shortMetadata.status == LC_IntersectionStatus::LimitExceeded);
        CHECK_FALSE(shortMetadata.session);
        CHECK(shortMetadata.work.generatedItems == (cap == 1 ? 0 : 2));
    }

    options = strictOptions();
    auto legacyProvider = endpointRefineCurves();
    const auto legacy = findIntersections(*legacyProvider, options);
    REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
    options.strict = false; // the owned factory must still issue only strict certificates
    auto owned = findCertifiedIntersections(endpointRefineCurves(), options);
    REQUIRE(owned.status == LC_IntersectionStatus::Ok);
    REQUIRE(owned.session->rootCount() == 1);
    CHECK(owned.work.generatedItems == legacy.work.generatedItems + 3);
    CHECK(owned.work.candidateBoxPairs == legacy.work.candidateBoxPairs);
    CHECK(owned.work.refinementBoxPairs == legacy.work.refinementBoxPairs);
    CHECK(owned.work.certificateBoxPairs == legacy.work.certificateBoxPairs);
    CHECK(legacy.work.deepestRootRefinement == 0);
    CHECK(owned.work.deepestRootRefinement == 0);

    options.maxGeneratedItems = owned.work.generatedItems;
    const auto exact = findCertifiedIntersections(endpointRefineCurves(), options);
    REQUIRE(exact.status == LC_IntersectionStatus::Ok);
    --options.maxGeneratedItems;
    const auto shortRoot = findCertifiedIntersections(endpointRefineCurves(), options);
    CHECK(shortRoot.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(shortRoot.session);
    CHECK(shortRoot.work.generatedItems == options.maxGeneratedItems);
}

TEST_CASE("Certified roots own provider options and issuer lifetime through moves",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto sourceProbe = std::make_shared<RefineProbe>();
    auto query = [&] {
        auto localOptions = strictOptions();
        return findCertifiedIntersections(endpointRefineCurves({0.25}, sourceProbe), localOptions);
    }();
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    const auto sourceHandle = query.session->rootHandle(0);
    const auto boundCalls = sourceProbe->bounds;
    CHECK(query.session->snapshot(sourceHandle));
    CHECK(query.session->rootCount() == 1);
    CHECK(sourceProbe->bounds == boundCalls); // cached access is not another proof query
    CHECK(sourceProbe->destroyed == 0);

    LC_CertifiedIntersectionSession moved{std::move(*query.session)};
    CHECK(query.session->rootCount() == 0);
    CHECK_FALSE(query.session->snapshot(sourceHandle));
    CHECK(query.session->contract(sourceHandle, refineAllowance()).status == LC_IntersectionStatus::InvalidInput);
    CHECK(query.session->cumulativeWork().totalBoxPairs == 0);
    REQUIRE(moved.snapshot(sourceHandle));

    auto destinationProbe = std::make_shared<RefineProbe>();
    auto destination = findCertifiedIntersections(endpointRefineCurves({0.75}, destinationProbe), strictOptions());
    REQUIRE(destination.status == LC_IntersectionStatus::Ok);
    const auto overwritten = destination.session->rootHandle(0);
    CHECK_FALSE(destination.session->snapshot(sourceHandle));
    *destination.session = std::move(moved);
    CHECK(destinationProbe->destroyed == 1);
    CHECK(moved.rootCount() == 0);
    CHECK_FALSE(destination.session->snapshot(overwritten));
    REQUIRE(destination.session->snapshot(sourceHandle));
    const auto refined = destination.session->contract(sourceHandle, refineAllowance());
    REQUIRE(refined.status == LC_IntersectionStatus::Ok);
    CHECK(refined.snapshot->event.isolation.parameterA.contains(0.25));
    destination.session.reset();
    CHECK(sourceProbe->destroyed == 1);

    auto replacement = findCertifiedIntersections(endpointRefineCurves(), strictOptions());
    REQUIRE(replacement.status == LC_IntersectionStatus::Ok);
    const auto expired = replacement.session->contract(sourceHandle, refineAllowance());
    CHECK(expired.status == LC_IntersectionStatus::InvalidInput);
    CHECK(expired.work.totalBoxPairs == 0);
    CHECK(expired.work.generatedItems == 0);
}

TEST_CASE("Snapshot copies and invalid handles cannot alter an issued root",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto query = findCertifiedIntersections(endpointRefineCurves(), strictOptions());
    auto other = findCertifiedIntersections(endpointRefineCurves(), strictOptions());
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    REQUIRE(other.status == LC_IntersectionStatus::Ok);
    const auto handle = query.session->rootHandle(0);
    const auto original = *query.session->snapshot(handle);
    auto edited = original;
    edited.event.proof = LC_IntersectionProof::Uncertified;
    edited.event.isolation.parameterA = LC_Interval::point(0.9);
    edited.imageX = LC_Interval::entire();
    edited.admittedContractions = std::numeric_limits<unsigned>::max();
    CHECK(edited.event.proof != original.event.proof);
    const auto cumulative = query.session->cumulativeWork();
    for (const auto bad : {LC_CertifiedRootHandle{}, query.session->rootHandle(1), other.session->rootHandle(0)}) {
        const auto refused = query.session->contract(bad, refineAllowance());
        CHECK(refused.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(refused.snapshot);
        CHECK(refused.work.totalBoxPairs == 0);
        CHECK(refused.work.generatedItems == 0);
        CHECK(refused.work.deepest() == 0);
        CHECK(query.session->cumulativeWork().totalBoxPairs == cumulative.totalBoxPairs);
    }
    const auto unchanged = *query.session->snapshot(handle);
    checkSameDomain(unchanged.event.isolation, original.event.isolation);
    checkSameInterval(unchanged.imageX, original.imageX);
    CHECK(unchanged.event.proof == original.event.proof);
    CHECK(unchanged.admittedContractions == 0);
    const auto refined = query.session->contract(handle, {1, 1, 1});
    REQUIRE(refined.status == LC_IntersectionStatus::Ok);
    REQUIRE(refined.snapshot);
    CHECK(refined.snapshot->event.isolation.parameterA.contains(0.25));
    CHECK_FALSE(refined.snapshot->event.isolation.parameterA.contains(0.9));
}

TEST_CASE("Issued roots contract inside original proof and cached images despite cancellation seeds",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    for (const double shift : {0.0, 1e200}) {
        CAPTURE(shift);
        auto provider = endpointRefineCurves();
        provider->curves.seedShift = shift;
        auto query = findCertifiedIntersections(std::move(provider), strictOptions());
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        const auto before = *query.session->snapshot(handle);
        const auto step = query.session->contract(handle, {1, 1, 1});
        REQUIRE(step.status == LC_IntersectionStatus::Ok);
        const auto& after = *step.snapshot;
        CHECK(after.event.proof == before.event.proof);
        CHECK(after.event.kind == before.event.kind);
        checkSameDomain(after.event.proofDomain, before.event.proofDomain);
        CHECK(after.event.isolation.parameterA.contains(0.25));
        CHECK(after.event.isolation.parameterB.contains(0.0));
        CHECK(after.event.isolation.parameterA.width() < before.event.isolation.parameterA.width());
        CHECK(after.imageX.contains(0.25));
        CHECK(after.imageY.contains(0.0));
        CHECK(after.imageX.lo() >= before.imageX.lo());
        CHECK(after.imageX.hi() <= before.imageX.hi());
        CHECK(after.imageY.lo() >= before.imageY.lo());
        CHECK(after.imageY.hi() <= before.imageY.hi());
        CHECK(after.imageX.contains(after.event.point.x));
        CHECK(after.imageY.contains(after.event.point.y));
        CHECK(step.work.certificateBoxPairs == 1);
        CHECK(step.work.totalBoxPairs == 1);
        CHECK(step.work.generatedItems == 1);
        CHECK(step.work.deepestRootRefinement == 1);
        CHECK(step.work.deepestOrderingRefinement == 0);
        CHECK(after.admittedContractions == 1);
    }
}

TEST_CASE("Owned ordering tracks root history rather than resetting each partner comparison",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto options = strictOptions();
    options.maxDepth = 1;
    auto legacyProvider = endpointRefineCurves({0.25, 0.5, 0.75});
    const auto legacy = findIntersections(*legacyProvider, options);
    REQUIRE(legacy.status == LC_IntersectionStatus::Ok);
    CHECK(legacy.work.deepestRootRefinement == 0);
    const auto shallow = findCertifiedIntersections(endpointRefineCurves({0.25, 0.5, 0.75}), options);
    CHECK(shallow.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(shallow.session);
    CHECK(shallow.work.deepestRootRefinement == 1);
    options.maxDepth = 2;
    auto query = findCertifiedIntersections(endpointRefineCurves({0.25, 0.5, 0.75}), options);
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    REQUIRE(query.session->rootCount() == 3);
    const auto first = *query.session->snapshot(query.session->rootHandle(0));
    CHECK(first.event.isolation.parameterA.contains(0.25));
    CHECK(first.admittedContractions == 2);
    CHECK(query.work.deepestRootRefinement == 2);
    const auto exhausted = query.session->contract(query.session->rootHandle(0), refineAllowance());
    CHECK(exhausted.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(exhausted.work.totalBoxPairs == 0);
    CHECK(exhausted.work.deepestRootRefinement == 0);
}

TEST_CASE("Close roots retain identity and per-call depth maxima are not cumulative differences",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto options = strictOptions();
    options.tolerance = 1e-2;
    auto query = findCertifiedIntersections(endpointRefineCurves({0.25, 0.2501}), options);
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    REQUIRE(query.session->rootCount() == 2);
    const auto a = query.session->rootHandle(0);
    const auto b = query.session->rootHandle(1);
    const auto originalA = *query.session->snapshot(a);
    const auto originalB = *query.session->snapshot(b);
    REQUIRE(originalA.admittedContractions == 1);
    REQUIRE(originalB.admittedContractions == 1);
    for (unsigned depth : {2u, 3u}) {
        const auto step = query.session->contract(a, refineAllowance());
        CHECK((step.status == LC_IntersectionStatus::Ok || step.status == LC_IntersectionStatus::AmbiguousTopology));
        CHECK(step.work.deepestRootRefinement == depth);
        CHECK(query.session->snapshot(a)->event.isolation.parameterA.contains(0.25));
        CHECK(query.session->snapshot(b)->admittedContractions == 1);
    }
    const auto otherStep = query.session->contract(b, refineAllowance());
    CHECK((otherStep.status == LC_IntersectionStatus::Ok || otherStep.status == LC_IntersectionStatus::AmbiguousTopology));
    CHECK(otherStep.work.deepestRootRefinement == 2);
    CHECK(otherStep.work.deepest() == 2);
    CHECK(query.session->cumulativeWork().deepestRootRefinement == 3);
    CHECK(query.session->snapshot(b)->event.isolation.parameterA.contains(0.2501));
    checkSameDomain(query.session->snapshot(a)->event.proofDomain, originalA.event.proofDomain);
    checkSameDomain(query.session->snapshot(b)->event.proofDomain, originalB.event.proofDomain);
}

TEST_CASE("Exact identity merges inherit parent history through compaction and sorting",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    std::vector<PolynomialPiece> pieces{
        polynomialLine(0, {0, 0}, {1, 0}, 0, 0.5), polynomialLine(0, {0, 0}, {1, 0}, 0.5, 1),
        {{1, 0, 1}, {0.5, 0, 0, 0}, {0, 0, 1, 0}}, polynomialLine(2, {0.25, 0}, {0, 1})};
    auto provider = std::make_unique<RefineCurves>(std::move(pieces), std::make_shared<RefineProbe>());
    provider->curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
        return LC_AnalyticLeafCertificate{true, segment, parameter}; // linear x, or strictly increasing y=t^2 on [0,1]
    };
    provider->curves.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate proof;
        if (box.segmentA > 1 || box.segmentB < 2) { return proof; }
        proof.proofDomain = box;
        const double root = box.segmentB == 2 ? 0.5 : 0.25;
        if (!box.parameterA.contains(root) || !box.parameterB.contains(0.0)) {
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            proof.proof = LC_IntersectionProof::AnalyticExclusion;
        } else {
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            proof.proof = LC_IntersectionProof::AnalyticEndpoint;
            proof.kind = Kind::Endpoint;
            proof.isolationA = box.segmentB == 2 && box.parameterB.lo() == 0 && box.parameterB.hi() == 0 ?
                LC_Interval::point(0.5) : box.parameterA;
            proof.isolationB = LC_Interval::point(0.0);
        }
        return proof;
    };
    auto options = strictOptions();
    options.maxDepth = 4;
    auto query = findCertifiedIntersections(std::move(provider), options);
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    REQUIRE(query.session->rootCount() == 2);
    const auto quarter = *query.session->snapshot(query.session->rootHandle(0));
    const auto partition = *query.session->snapshot(query.session->rootHandle(1));
    CHECK(quarter.event.isolation.parameterA.contains(0.25));
    CHECK(quarter.admittedContractions == 1);
    checkSameInterval(partition.event.isolation.parameterA, LC_Interval::point(0.5));
    CHECK(partition.admittedContractions == 2);
    CHECK(query.work.deepestRootRefinement == 2);
    const auto retained = query.session->contract(query.session->rootHandle(1), refineAllowance());
    CHECK(retained.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK_FALSE(retained.snapshot);
    CHECK(retained.work.deepestRootRefinement == 3);
    CHECK(query.session->snapshot(query.session->rootHandle(1))->admittedContractions == 3);
}

TEST_CASE("Contraction admission reports exact local charges without refunding failed publication",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    for (const unsigned mode : {0u, 1u, 2u, 3u}) {
        CAPTURE(mode);
        auto query = findCertifiedIntersections(endpointRefineCurves(), strictOptions());
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        const auto original = *query.session->snapshot(handle);
        const auto before = query.session->cumulativeWork();
        auto allowance = LC_CertifiedContractionAllowance{1, 1, 1};
        if (mode == 0) { allowance.remainingPairs = 0; }
        if (mode == 1) { allowance.remainingItems = 0; }
        if (mode == 2) { allowance.absoluteRootDepth = 0; }
        const auto step = query.session->contract(handle, allowance);
        CHECK(step.status == (mode == 3 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
        CHECK(step.snapshot.has_value() == (mode == 3));
        CHECK(step.work.totalBoxPairs == ((mode == 1 || mode == 3) ? 1 : 0));
        CHECK(step.work.generatedItems == (mode == 3 ? 1 : 0));
        CHECK(step.work.deepestRootRefinement == ((mode == 1 || mode == 3) ? 1 : 0));
        CHECK(query.session->cumulativeWork().totalBoxPairs == before.totalBoxPairs + step.work.totalBoxPairs);
        CHECK(query.session->cumulativeWork().generatedItems == before.generatedItems + step.work.generatedItems);
        REQUIRE(query.session->snapshot(handle));
        if (mode != 3) {
            checkSameDomain(query.session->snapshot(handle)->event.isolation, original.event.isolation);
            checkSameInterval(query.session->snapshot(handle)->imageX, original.imageX);
            const auto bad = query.session->contract({}, refineAllowance());
            CHECK(bad.status == LC_IntersectionStatus::InvalidInput);
            CHECK(bad.work.totalBoxPairs == 0);
            LC_CertifiedIntersectionSession moved{std::move(*query.session)};
            const auto again = moved.contract(handle, refineAllowance());
            CHECK(again.status == LC_IntersectionStatus::LimitExceeded);
            CHECK(again.work.totalBoxPairs == 0);
            CHECK(again.work.generatedItems == 0);
            CHECK(moved.snapshot(handle));
        }
    }
}

TEST_CASE("Original cumulative session ceilings cannot be reset by fresh call allowances",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto options = strictOptions();
    const auto measured = findCertifiedIntersections(endpointRefineCurves(), options);
    REQUIRE(measured.status == LC_IntersectionStatus::Ok);
    for (const std::size_t extra : {0u, 1u}) {
        CAPTURE(extra);
        options.maxBoxPairs = measured.work.totalBoxPairs + extra;
        auto query = findCertifiedIntersections(endpointRefineCurves(), options);
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        if (extra == 1) {
            REQUIRE(query.session->contract(handle, refineAllowance()).status == LC_IntersectionStatus::Ok);
        }
        const auto refused = query.session->contract(handle, refineAllowance());
        CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(refused.work.totalBoxPairs == 0);
        CHECK(refused.work.deepestRootRefinement == 0);
        CHECK(query.session->cumulativeWork().totalBoxPairs == options.maxBoxPairs);
    }

    options = strictOptions();
    const auto singularMeasured = findCertifiedIntersections(singularRefineCurves(false), options);
    REQUIRE(singularMeasured.status == LC_IntersectionStatus::Ok);
    options.maxGeneratedItems = singularMeasured.work.generatedItems + 2; // one callback record + one narrowed event
    auto items = findCertifiedIntersections(singularRefineCurves(false), options);
    REQUIRE(items.status == LC_IntersectionStatus::Ok);
    const auto handle = items.session->rootHandle(0);
    REQUIRE(items.session->contract(handle, refineAllowance()).status == LC_IntersectionStatus::Ok);
    const auto before = *items.session->snapshot(handle);
    const auto refused = items.session->contract(handle, refineAllowance());
    CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK_FALSE(refused.snapshot);
    CHECK(refused.work.totalBoxPairs == 1); // contraction enters; paired callback admission refuses before debit
    CHECK(refused.work.generatedItems == 0);
    CHECK(refused.work.deepestRootRefinement == 2);
    CHECK(items.session->cumulativeWork().generatedItems == options.maxGeneratedItems);
    checkSameDomain(items.session->snapshot(handle)->event.isolation, before.event.isolation);
    CHECK(items.session->snapshot(handle)->admittedContractions == 2);

    options = strictOptions();
    options.maxDepth = 3;
    auto depth = findCertifiedIntersections(endpointRefineCurves(), options);
    REQUIRE(depth.status == LC_IntersectionStatus::Ok);
    const auto depthHandle = depth.session->rootHandle(0);
    for (unsigned h = 1; h <= 3; ++h) {
        const auto attempt = depth.session->contract(depthHandle, refineAllowance(std::numeric_limits<unsigned>::max()));
        CHECK((attempt.status == LC_IntersectionStatus::Ok || attempt.status == LC_IntersectionStatus::AmbiguousTopology));
        CHECK(attempt.work.totalBoxPairs == 1);
        CHECK(attempt.work.deepestRootRefinement == h);
        CHECK(depth.session->snapshot(depthHandle)->admittedContractions == h);
    }
    const auto exhausted = depth.session->contract(depthHandle, refineAllowance(std::numeric_limits<unsigned>::max()));
    CHECK(exhausted.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(exhausted.work.totalBoxPairs == 0);
    CHECK(exhausted.work.deepest() == 0);
    CHECK(depth.session->snapshot(depthHandle)->admittedContractions == 3);
}

TEST_CASE("Singular issued contacts use qualified analytic contraction or honest nonterminal refusal",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    for (const bool cusp : {false, true}) {
        CAPTURE(cusp);
        auto probe = std::make_shared<RefineProbe>();
        auto query = findCertifiedIntersections(singularRefineCurves(cusp, probe), strictOptions());
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        const auto original = *query.session->snapshot(handle);
        probe->unavailable = true;
        const auto unknown = query.session->contract(handle, refineAllowance());
        CHECK(unknown.status == LC_IntersectionStatus::AmbiguousTopology);
        CHECK_FALSE(unknown.snapshot);
        CHECK(unknown.work.totalBoxPairs == 2); // contraction + unavailable certificate callback
        CHECK(unknown.work.generatedItems == 1);
        CHECK(unknown.work.deepestRootRefinement == 1);
        REQUIRE(query.session->snapshot(handle));
        checkSameDomain(query.session->snapshot(handle)->event.isolation, original.event.isolation);
        probe->unavailable = false;
        const auto step = query.session->contract(handle, {2, 2, 2});
        REQUIRE(step.status == LC_IntersectionStatus::Ok);
        CHECK(step.work.totalBoxPairs == 2);
        CHECK(step.work.generatedItems == 2); // callback record + narrowed event
        CHECK(step.work.deepestRootRefinement == 2);
        CHECK(step.snapshot->event.proof == original.event.proof);
        checkSameDomain(step.snapshot->event.proofDomain, original.event.proofDomain);
        CHECK(step.snapshot->event.isolation.parameterA.contains(0.5));
        CHECK(step.snapshot->event.isolation.parameterB.contains(0.5));
        CHECK(step.snapshot->imageX.contains(0.5));
        CHECK(step.snapshot->imageY.contains(0.0));
        CHECK(step.snapshot->event.isolation.parameterA.width() < original.event.isolation.parameterA.width());
    }
    auto shortCertificate = findCertifiedIntersections(singularRefineCurves(false), strictOptions());
    REQUIRE(shortCertificate.status == LC_IntersectionStatus::Ok);
    const auto shortWork = shortCertificate.session->contract(shortCertificate.session->rootHandle(0), {1, 1, 1});
    CHECK(shortWork.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(shortWork.work.totalBoxPairs == 1);
    CHECK(shortWork.work.deepestRootRefinement == 1);
    CHECK(shortCertificate.session->snapshot(shortCertificate.session->rootHandle(0)));
}

TEST_CASE("Malformed analytic categories terminally invalidate retained roots but not legacy policy",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    for (const bool excluded : {false, true}) {
        CAPTURE(excluded);
        auto probe = std::make_shared<RefineProbe>();
        auto query = findCertifiedIntersections(singularRefineCurves(false, probe), strictOptions());
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        probe->wrongUniqueCategory = !excluded;
        probe->wrongExcludedCategory = excluded;
        const auto refused = query.session->contract(handle, refineAllowance());
        CHECK(refused.status == LC_IntersectionStatus::InvalidInput);
        CHECK_FALSE(refused.snapshot);
        CHECK(refused.work.totalBoxPairs == 2);
        CHECK(refused.work.deepestRootRefinement == 1);
        CHECK(query.session->rootCount() == 0);
        CHECK_FALSE(query.session->snapshot(handle));
        CHECK_FALSE(query.session->snapshot(query.session->rootHandle(0)));
        const auto spent = query.session->cumulativeWork();
        const auto bad = query.session->contract({}, refineAllowance());
        CHECK(bad.status == LC_IntersectionStatus::InvalidInput);
        CHECK(bad.work.totalBoxPairs == 0);
        LC_CertifiedIntersectionSession moved{std::move(*query.session)};
        const auto again = moved.contract(handle, refineAllowance());
        CHECK(again.status == LC_IntersectionStatus::InvalidInput);
        CHECK(again.work.totalBoxPairs == 0);
        CHECK(moved.cumulativeWork().totalBoxPairs == spent.totalBoxPairs);
        CHECK(moved.rootCount() == 0);

        auto legacy = singularRefineCurves(false, probe);
        const auto oldPolicy = findIntersections(*legacy, strictOptions());
        CHECK(oldPolicy.status == LC_IntersectionStatus::AmbiguousTopology);
    }
}

TEST_CASE("An exact singular root refuses unchanged contraction without losing its existing proof",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto provider = singularRefineCurves(true);
    provider->curves.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate proof;
        proof.proofDomain = box;
        proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
        proof.proof = LC_IntersectionProof::AnalyticCusp;
        proof.kind = Kind::Tangent;
        proof.isolationA = proof.isolationB = LC_Interval::point(0.5);
        return proof; // exact y=(u-.5)^2=0 and x_A=t proves the unique parameter pair
    };
    auto query = findCertifiedIntersections(std::move(provider), strictOptions());
    REQUIRE(query.status == LC_IntersectionStatus::Ok);
    const auto handle = query.session->rootHandle(0);
    const auto original = *query.session->snapshot(handle);
    for (unsigned h : {1u, 2u}) {
        const auto attempt = query.session->contract(handle, refineAllowance());
        CHECK(attempt.status == LC_IntersectionStatus::AmbiguousTopology);
        CHECK_FALSE(attempt.snapshot);
        CHECK(attempt.work.totalBoxPairs == 2);
        CHECK(attempt.work.generatedItems == 1); // unchanged contraction still admitted its analytic callback
        CHECK(attempt.work.deepestRootRefinement == h);
        REQUIRE(query.session->snapshot(handle));
        CHECK(query.session->snapshot(handle)->admittedContractions == h);
        checkSameDomain(query.session->snapshot(handle)->event.isolation, original.event.isolation);
        checkSameDomain(query.session->snapshot(handle)->event.proofDomain, original.event.proofDomain);
    }
}

TEST_CASE("Provider contradictions and exceptions cannot leave a reusable trusted session",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    for (const unsigned fault : {0u, 1u, 2u}) {
        CAPTURE(fault);
        auto probe = std::make_shared<RefineProbe>();
        auto query = findCertifiedIntersections(endpointRefineCurves({0.25}, probe), strictOptions());
        REQUIRE(query.status == LC_IntersectionStatus::Ok);
        const auto handle = query.session->rootHandle(0);
        const auto before = query.session->cumulativeWork();
        probe->invalidBounds = fault == 0;
        probe->translated = fault == 1;
        probe->throwBounds = fault == 2;
        if (fault == 2) {
            REQUIRE_THROWS_AS(query.session->contract(handle, refineAllowance()), std::runtime_error);
        } else {
            const auto failed = query.session->contract(handle, refineAllowance());
            CHECK(failed.status == LC_IntersectionStatus::InvalidInput);
            CHECK_FALSE(failed.snapshot);
            CHECK(failed.work.totalBoxPairs == 1);
            CHECK(failed.work.generatedItems == (fault == 1 ? 1 : 0));
        }
        CHECK(query.session->rootCount() == 0);
        CHECK_FALSE(query.session->snapshot(handle));
        CHECK(query.session->cumulativeWork().totalBoxPairs == before.totalBoxPairs + 1);
        CHECK(query.session->cumulativeWork().deepestRootRefinement == 1);
        const auto calls = probe->bounds;
        probe->invalidBounds = probe->translated = probe->throwBounds = false;
        const auto after = query.session->contract(handle, refineAllowance());
        CHECK(after.status == LC_IntersectionStatus::InvalidInput);
        CHECK(after.work.totalBoxPairs == 0);
        CHECK(after.work.generatedItems == 0);
        CHECK(probe->bounds == calls);
    }
}

TEST_CASE("Overlapping self occurrences cannot become retained unique-root credentials",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    const auto provider = [] {
        auto curves = std::make_unique<CertifiedPolynomials>(std::vector<PolynomialPiece>{
            {{0, 0, 1}, {0, 45, -105, 70}, {0, 30, -30, 0}}});
        curves->contactProof = [](const LC_IntersectionDomain&) {
            LC_AnalyticIntersectionCertificate proof;
            proof.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            proof.proof = LC_IntersectionProof::AnalyticCusp;
            proof.proofDomain = {0, 0, LC_Interval::hull(0, 1), LC_Interval::hull(0, 1)};
            proof.isolationA = proof.isolationB = LC_Interval::point(0.5);
            return proof; // intentionally impossible: the diagonal has nonisolated equal occurrences
        };
        return curves;
    };
    auto legacy = provider();
    CHECK(findIntersections(*legacy, strictOptions()).status == LC_IntersectionStatus::AmbiguousTopology);
    const auto retained = findCertifiedIntersections(provider(), strictOptions());
    CHECK(retained.status == LC_IntersectionStatus::InvalidInput);
    CHECK_FALSE(retained.session);
}

TEST_CASE("Observed intersection depths distinguish roots from unexamined work",
          "[lc-offsetcurve][geometry][intersection-accounting]") {
    LC_IntersectionWorkStats work;
    CHECK(work.deepest() == 0);
    work.deepestPreparation = 2;
    work.deepestPairRefinement = 4;
    work.deepestOrderingRefinement = 3;
    CHECK(work.deepest() == 4);
    work.deepestOrderingRefinement = std::numeric_limits<unsigned>::max();
    CHECK(work.deepest() == std::numeric_limits<unsigned>::max());

    for (const bool strict : {false, true}) {
        CAPTURE(strict);
        auto options = strictOptions();
        options.strict = strict;
        options.maxDepth = options.maxLeafDepth = 0;
        AnalyticLines crossing{{1.0, 0.0}, {0.0, 1.0}};
        const auto result = findIntersections(crossing, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 1);
        CHECK(result.work.generatedLeaves == 2);
        CHECK(result.work.refinementBoxPairs == 1);
        CHECK(result.work.deepestPreparation == 0);
        CHECK(result.work.deepestPairRefinement == 0);
        CHECK(result.work.deepestOrderingRefinement == 0);
        CHECK(result.work.deepest() == 0);

        CertifiedPolynomials empty{{}};
        const auto invalid = findIntersections(empty, options);
        CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
        CHECK(invalid.work.totalBoxPairs == 0);
        CHECK(invalid.work.generatedLeaves == 0);
        CHECK(invalid.work.deepest() == 0);

        options.maxBoxPairs = 0;
        const auto unexamined = findIntersections(crossing, options);
        CHECK(unexamined.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(unexamined.work.totalBoxPairs == 0);
        CHECK(unexamined.work.refinementBoxPairs == 0);
        CHECK(unexamined.work.deepest() == 0);
    }
}

TEST_CASE("Preparation depths follow independently traced dyadic bounds on every exit",
          "[lc-offsetcurve][geometry][intersection-accounting]") {
    for (const bool strict : {false, true}) {
        CAPTURE(strict);
        AccountingCurves curves{{polynomialLine(0, {0, 0}, {1, 0}),
                                 polynomialLine(1, {0, 4}, {1, 0})}};
        curves.derivativeBounds = [](const std::size_t segment, const double from, const double to,
                                     LC_Interval& dx, LC_Interval& dy) {
            if (to - from > (segment == 0 ? 0.125 : 0.5)) {
                dx = dy = LC_Interval::hull(-1.0, 1.0);
            }
            return true;
        };
        auto options = strictOptions();
        options.strict = strict;
        options.maxLeafDepth = 3;
        const auto exact = findIntersections(curves, options);
        REQUIRE(exact.status == LC_IntersectionStatus::Ok);
        CHECK(exact.work.generatedLeaves == 10);
        CHECK(preparationTraceDepth(curves) == 3);
        CHECK(exact.work.deepestPreparation == preparationTraceDepth(curves));
        CHECK(exact.work.deepestPairRefinement == 0);
        CHECK(exact.work.deepestOrderingRefinement == 0);
        CHECK(exact.work.deepest() == 3);

        curves.calls.clear();
        options.maxLeafDepth = 2;
        const auto shortDepth = findIntersections(curves, options);
        CHECK(shortDepth.status == (strict ? LC_IntersectionStatus::LimitExceeded :
                                            LC_IntersectionStatus::AmbiguousTopology));
        CHECK(shortDepth.intersections.empty());
        CHECK(preparationTraceDepth(curves) == 2);
        CHECK(shortDepth.work.deepestPreparation == preparationTraceDepth(curves));
        CHECK(shortDepth.work.generatedLeaves == 0);

        curves.calls.clear();
        options.maxLeafDepth = 8;
        const auto loose = curves.derivativeBounds;
        curves.derivativeBounds = [loose](const std::size_t segment, const double from, const double to,
                                         LC_Interval& dx, LC_Interval& dy) {
            return !(from < to && to - from <= 0.125) && loose(segment, from, to, dx, dy);
        };
        const auto invalid = findIntersections(curves, options);
        CHECK(invalid.status == (strict ? LC_IntersectionStatus::InvalidInput :
                                         LC_IntersectionStatus::AmbiguousTopology));
        CHECK(invalid.intersections.empty());
        // Ordinary preparation keeps subdividing a refused bound; strict preparation stops at that examination.
        const unsigned failedDepth = strict ? 3u : options.maxLeafDepth;
        CHECK(preparationTraceDepth(curves) == failedDepth);
        CHECK(invalid.work.deepestPreparation == preparationTraceDepth(curves));
        CHECK(invalid.work.deepest() == failedDepth);
    }
}

TEST_CASE("Refused preparation children and rounded midpoint stagnation do not invent depth",
          "[lc-offsetcurve][geometry][intersection-accounting]") {
    AccountingCurves children{{polynomialLine(0, {0, 0}, {1, 0})}};
    children.derivativeBounds = [](std::size_t, const double from, const double to,
                                   LC_Interval& dx, LC_Interval& dy) {
        if (to - from > 0.125) {
            dx = dy = LC_Interval::hull(-1.0, 1.0);
        }
        return true;
    };
    auto options = strictOptions();
    options.maxGeneratedItems = 8; // root + two (callback + two children) groups + final unavailable callback
    const auto refused = findIntersections(children, options);
    CHECK(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(refused.work.generatedItems == 8);
    CHECK(refused.work.generatedLeaves == 0);
    CHECK(preparationTraceDepth(children) == 2);
    CHECK(refused.work.deepestPreparation == preparationTraceDepth(children));

    for (const bool strict : {false, true}) {
        CAPTURE(strict);
        AccountingCurves rounded{{polynomialLine(0, {-1e16, 0}, {1, 0}, 1e16, 1e16 + 16)}};
        rounded.derivativeBounds = [](std::size_t, double, double, LC_Interval& dx, LC_Interval& dy) {
            dx = dy = LC_Interval::hull(-1.0, 1.0);
            return true;
        };
        options = strictOptions();
        options.strict = strict;
        options.maxLeafDepth = 8;
        const auto stagnant = findIntersections(rounded, options);
        CHECK(stagnant.status == LC_IntersectionStatus::AmbiguousTopology);
        CHECK(stagnant.intersections.empty());
        CHECK(preparationTraceDepth(rounded) == 3);
        CHECK(stagnant.work.deepestPreparation == preparationTraceDepth(rounded));
        CHECK(stagnant.work.deepest() == 3);
    }
}

TEST_CASE("Pair depths count charged examined cells rather than pending subdivisions",
          "[lc-offsetcurve][geometry][intersection-accounting]") {
    AccountingCurves curves{{polynomialLine(0, {-0.375, 0}, {1, 0}),
                             polynomialLine(1, {0, -0.625}, {0, 1})}};
    curves.derivativeBounds = [](std::size_t, const double from, const double to,
                                 LC_Interval& dx, LC_Interval& dy) {
        if (from < to) {
            dx = dy = LC_Interval::hull(-2.0, 2.0);
        }
        return true;
    };
    curves.curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
        return LC_AnalyticLeafCertificate{true, segment, parameter}; // both exact nonconstant lines are injective
    };
    auto options = strictOptions();
    options.maxDepth = 2;
    const auto baseline = findIntersections(curves, options);
    REQUIRE(baseline.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(baseline.intersections.empty());
    CHECK(baseline.work.deepestPreparation == 0);
    CHECK(pairTraceDepth(curves) == 2);
    CHECK(baseline.work.deepestPairRefinement == pairTraceDepth(curves));
    CHECK(baseline.work.deepestOrderingRefinement == 0);
    REQUIRE(baseline.work.certificateBoxPairs == 2 + 3); // two leaf callbacks + three entered solve-cell callbacks

    for (const unsigned admitted : {1u, 2u}) {
        CAPTURE(admitted);
        curves.calls.clear();
        options.maxBoxPairs = 2 + 1 + 2 * admitted; // leaf callbacks + hierarchy pair + N (cell + callback) groups
        const auto shortWork = findIntersections(curves, options);
        CHECK(shortWork.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(shortWork.intersections.empty());
        CHECK(shortWork.work.totalBoxPairs == options.maxBoxPairs);
        CHECK(shortWork.work.refinementBoxPairs == admitted);
        CHECK(pairTraceDepth(curves) == admitted - 1);
        CHECK(shortWork.work.deepestPairRefinement == pairTraceDepth(curves));
    }

    curves.calls.clear();
    curves.derivativeBounds = [](std::size_t, const double from, const double to,
                                 LC_Interval& dx, LC_Interval& dy) {
        if (to - from > 0.25) {
            dx = dy = LC_Interval::hull(-2.0, 2.0);
        }
        return true;
    };
    options = strictOptions();
    options.maxDepth = 16;
    const auto resolved = findIntersections(curves, options);
    REQUIRE(resolved.status == LC_IntersectionStatus::Ok);
    REQUIRE(resolved.intersections.size() == 1);
    const unsigned requiredDepth = pairTraceDepth(curves);
    REQUIRE(requiredDepth > 2);
    CHECK(resolved.work.deepestPairRefinement == requiredDepth);

    curves.calls.clear();
    options.maxDepth = requiredDepth;
    const auto exactDepth = findIntersections(curves, options);
    REQUIRE(exactDepth.status == LC_IntersectionStatus::Ok);
    CHECK(exactDepth.work.deepestPairRefinement == requiredDepth);
    CHECK(pairTraceDepth(curves) == requiredDepth);

    curves.calls.clear();
    options.maxDepth = requiredDepth - 1;
    const auto shortDepth = findIntersections(curves, options);
    CHECK(shortDepth.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(shortDepth.intersections.empty());
    CHECK(pairTraceDepth(curves) == requiredDepth - 1);
    CHECK(shortDepth.work.deepestPairRefinement == pairTraceDepth(curves));

    curves.calls.clear();
    curves.derivativeBounds = [](std::size_t, const double from, const double to,
                                 LC_Interval& dx, LC_Interval& dy) {
        if (to - from == 0.5) {
            return false;
        }
        if (from < to) {
            dx = dy = LC_Interval::hull(-2.0, 2.0);
        }
        return true;
    };
    options = strictOptions();
    const auto invalid = findIntersections(curves, options);
    CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
    CHECK(invalid.intersections.empty());
    REQUIRE_FALSE(curves.calls.empty());
    unsigned failedDepth = 0;
    REQUIRE(examinedDepth(curves.calls.back(), curves.segments()[curves.calls.back().segment], failedDepth));
    CHECK(failedDepth == 1);
    CHECK(invalid.work.deepestPairRefinement == failedDepth);

    AccountingCurves ordinary{{{{0, 0, 1}, {0, 1, 0, 0}, {3.0 / 64.0, -0.25, 0.25, 0}},
                               polynomialLine(1, {0, 0}, {1, 0})}};
    options = strictOptions();
    options.strict = false;
    options.maxDepth = 2;
    const auto ordinaryResult = findIntersections(ordinary, options);
    CHECK(ordinaryResult.work.deepestPreparation == 0);
    CHECK(pairTraceDepth(ordinary) == 2);
    CHECK(ordinaryResult.work.deepestPairRefinement == pairTraceDepth(ordinary));
    CHECK(ordinaryResult.work.deepestOrderingRefinement == 0);
}

TEST_CASE("Strict ordering depths survive contraction limits invalid bounds and ambiguity",
          "[lc-offsetcurve][geometry][intersection-accounting]") {
    AccountingCurves curves{{polynomialLine(0, {0, 0}, {1, 0}),
                             polynomialLine(1, {0.25, 0}, {0, 1}),
                             polynomialLine(2, {0.5, 0}, {0, 1})}};
    curves.curves.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate certificate;
        if (box.segmentA != 0 || (box.segmentB != 1 && box.segmentB != 2)) {
            return certificate;
        }
        certificate.proofDomain = box;
        const double root = box.segmentB == 1 ? 0.25 : 0.5;
        if (!box.parameterA.contains(root) || !box.parameterB.contains(0.0)) {
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            certificate.proof = LC_IntersectionProof::AnalyticExclusion;
        } else {
            // Exact horizontal/vertical lines have just this one contact, at the vertical branch's free start.
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proof = LC_IntersectionProof::AnalyticEndpoint;
            certificate.kind = Kind::Endpoint;
            certificate.isolationA = LC_Interval::hull(std::max(0.125, box.parameterA.lo()),
                                                       std::min(0.625, box.parameterA.hi()));
            certificate.isolationB = LC_Interval::point(0.0);
        }
        return certificate;
    };
    auto options = strictOptions();
    options.maxDepth = 1;
    const auto baseline = findIntersections(curves, options);
    REQUIRE(baseline.status == LC_IntersectionStatus::Ok);
    REQUIRE(baseline.intersections.size() == 2);
    CHECK(orderingPointCalls(curves) == 2);
    CHECK(baseline.work.deepestPreparation == 0);
    CHECK(baseline.work.deepestPairRefinement == 0);
    CHECK(baseline.work.deepestOrderingRefinement == 1);
    CHECK(baseline.work.deepest() == 1);

    curves.calls.clear();
    options.maxDepth = 0;
    const auto shortDepth = findIntersections(curves, options);
    CHECK(shortDepth.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(shortDepth.intersections.empty());
    CHECK(orderingPointCalls(curves) == 0);
    CHECK(shortDepth.work.deepestOrderingRefinement == 0);

    for (const std::size_t admitted : {0u, 1u}) {
        CAPTURE(admitted);
        curves.calls.clear();
        options.maxDepth = 1;
        // The final comparison follows two contractions. Admit only the preceding comparison and N contractions.
        options.maxBoxPairs = baseline.work.totalBoxPairs - 3 + admitted;
        const auto shortWork = findIntersections(curves, options);
        CHECK(shortWork.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(shortWork.intersections.empty());
        CHECK(shortWork.work.totalBoxPairs == options.maxBoxPairs);
        CHECK(orderingPointCalls(curves) == admitted);
        CHECK(shortWork.work.deepestOrderingRefinement == admitted);
    }

    curves.calls.clear();
    options = strictOptions();
    curves.derivativeBounds = [](const std::size_t segment, const double from, const double to,
                                 LC_Interval&, LC_Interval&) {
        return !(segment == 0 && from == 0.375 && to == from);
    };
    const auto invalid = findIntersections(curves, options);
    CHECK(invalid.status == LC_IntersectionStatus::InvalidInput);
    CHECK(invalid.intersections.empty());
    CHECK(orderingPointCalls(curves) == 1);
    CHECK(invalid.work.deepestOrderingRefinement == 1);

    curves.calls.clear();
    curves.derivativeBounds = [](std::size_t, const double from, const double to,
                                 LC_Interval& dx, LC_Interval& dy) {
        if (from < to) {
            dx = dy = LC_Interval::hull(-2.0, 2.0);
        }
        return true;
    };
    curves.curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
        return LC_AnalyticLeafCertificate{true, segment, parameter}; // all three nonconstant lines are injective
    };
    const auto ambiguous = findIntersections(curves, options);
    CHECK(ambiguous.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(ambiguous.intersections.empty());
    CHECK(orderingPointCalls(curves) == 2);
    CHECK(ambiguous.work.deepestOrderingRefinement == 1);
}

TEST_CASE("Strict transverse certificates use outward points rather than cancellation seeds",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{polynomialLine(0, {-0.5, 0}, {1, 0}),
                                  polynomialLine(1, {0, -0.5}, {0, 1})}};
    auto options = strictOptions();
    for (const double shift : {0.0, 1e12}) {
        CAPTURE(shift);
        curves.seedShift = shift; // finite estimates are deliberately poor; the exact residual still has its own bounds
        const auto result = findIntersections(curves, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 1);
        checkStrictWork(result, options);
        const auto& root = result.intersections.front();
        CHECK(root.proof == LC_IntersectionProof::IntervalUniqueTransverse);
        CHECK(root.kind == Kind::Transverse);
        CHECK(root.isolation.segmentA == 0);
        CHECK(root.isolation.segmentB == 1);
        CHECK(root.isolation.parameterA.contains(0.5));
        CHECK(root.isolation.parameterB.contains(0.5));
        CHECK(root.point.distanceTo({0, 0}) < 1e-12);
    }
    curves.seedShift = 0.0;
    options.strict = false;
    options.maxGeneratedItems = 0; // strict-only limits/hooks cannot affect ordinary callers
    curves.contactProof = [](const auto&) {
        LC_AnalyticIntersectionCertificate invalid;
        invalid.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
        return invalid;
    };
    const auto ordinary = findIntersections(curves, options);
    REQUIRE(ordinary.status == LC_IntersectionStatus::Ok);
    REQUIRE(ordinary.intersections.size() == 1);
    CHECK(ordinary.intersections.front().proof == LC_IntersectionProof::Uncertified);
    CHECK(ordinary.work.generatedItems == 0);
    CHECK(ordinary.work.certificateBoxPairs == 0);
    checkWork(ordinary, options);
}

TEST_CASE("Strict isolation keeps nearby distinct roots below the numerical merge tolerance",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    const double lo = 0.5 - 1.0 / 1024.0;
    const double hi = 0.5 + 1.0 / 1024.0;
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}),
        {{1, 0, 1}, {0, 1, 0, 0}, {lo * hi, -1, 1, 0}}}};
    auto options = strictOptions();
    options.tolerance = 0.1;
    const auto result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 2);
    checkStrictWork(result, options);
    CHECK(result.intersections[0].isolation.parameterA.contains(lo));
    CHECK(result.intersections[1].isolation.parameterA.contains(hi));
    CHECK(result.intersections[0].isolation.parameterA.hi() < result.intersections[1].isolation.parameterA.lo());
    CHECK(result.intersections[0].isolation.parameterB.hi() < result.intersections[1].isolation.parameterB.lo());
    CHECK(result.work.certificateBoxPairs > 0);
}

TEST_CASE("Strict global ordering considers shared branches with different partners",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    const double first = 0.5 - 1.0 / 1024.0;
    const double second = 0.5 + 1.0 / 1024.0;
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}),
        polynomialLine(1, {first, -0.5}, {0, 1}), polynomialLine(2, {second, -0.5}, {0, 1})}};
    auto options = strictOptions();
    options.tolerance = 1.0;
    const auto ordered = findIntersections(curves, options);
    REQUIRE(ordered.status == LC_IntersectionStatus::Ok);
    REQUIRE(ordered.intersections.size() == 2);
    checkStrictWork(ordered, options);
    CHECK(ordered.intersections[0].branchB == 1);
    CHECK(ordered.intersections[1].branchB == 2);
    CHECK(ordered.intersections[0].isolation.parameterA.hi() < ordered.intersections[1].isolation.parameterA.lo());

    CertifiedPolynomials triple{{polynomialLine(0, {-0.5, 0}, {1, 0}),
        polynomialLine(1, {0, -0.5}, {0, 1}), polynomialLine(2, {-0.5, -0.5}, {1, 1})}};
    const auto unresolved = findIntersections(triple, options);
    CHECK(unresolved.status == LC_IntersectionStatus::AmbiguousTopology);
    checkStrictWork(unresolved, options);
    CHECK(unresolved.work.certificateBoxPairs > 0);
    options.strict = false;
    options.tolerance = 1e-9;
    const auto ordinary = findIntersections(triple, options);
    REQUIRE(ordinary.status == LC_IntersectionStatus::Ok);
    CHECK(ordinary.intersections.size() == 3);
}

TEST_CASE("Qualified partition endpoints merge by exact occurrence not spatial proximity",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}, 0.0, 0.5),
        polynomialLine(0, {0, 0}, {1, 0}, 0.5, 1.0), polynomialLine(1, {0.5, -0.5}, {0, 1})}};
    curves.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate certificate;
        certificate.proofDomain = box;
        if (box.parameterA.contains(0.5) && box.parameterB.contains(0.5)) {
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proof = LC_IntersectionProof::AnalyticEndpoint;
            certificate.isolationA = certificate.isolationB = LC_Interval::point(0.5);
            certificate.kind = Kind::Transverse;
        } else {
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            certificate.proof = LC_IntersectionProof::AnalyticExclusion;
        }
        return certificate;
    };
    auto options = strictOptions();
    options.maxDepth = 0;
    const auto result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    checkStrictWork(result, options);
    CHECK(result.intersections.front().isolation.parameterA.lo() == 0.5);
    CHECK(result.intersections.front().isolation.parameterA.hi() == 0.5);
    CHECK(result.intersections.front().kind == Kind::Transverse);
    CHECK(result.work.generatedItems >= 6); // three initial boxes, two event records, and their derived merge
}

TEST_CASE("Strict tiny self loops cannot disappear through small-image preparation or pruning",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    const double scale = 1e-9;
    CertifiedPolynomials loop{{{{0, 0, 1}, {0, 45 * scale, -105 * scale, 70 * scale},
                                                      {0, 30 * scale, -30 * scale, 0}}}};
    auto options = strictOptions();
    options.tolerance = 1e-3;
    const auto result = findIntersections(loop, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    checkStrictWork(result, options);
    const auto& root = result.intersections.front();
    CHECK(root.branchA == 0);
    CHECK(root.branchB == 0);
    CHECK(root.isolation.parameterA.contains(0.5 * (1.0 - std::sqrt(3.0 / 7.0))));
    CHECK(root.isolation.parameterB.contains(0.5 * (1.0 + std::sqrt(3.0 / 7.0))));
    CHECK(root.isolation.parameterA.contains(root.parameterA));
    CHECK(root.isolation.parameterB.contains(root.parameterB));
    CHECK(root.isolation.parameterA.hi() < root.isolation.parameterB.lo());
    CHECK(root.proof == LC_IntersectionProof::IntervalUniqueTransverse);
}

TEST_CASE("Strict joined-end exclusions do not suppress an adjacent segment re-crossing",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}),
                                  {{0, 1, 2}, {2, -1, 0, 0}, {1.5, -2.5, 1, 0}}}};
    curves.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate certificate;
        // The only non-construction intersection is (t,u)=(0.5,1.5).
        // Exclude the joined end only after this WHOLE box excludes that second root.
        if (!box.parameterA.contains(0.5) || !box.parameterB.contains(1.5)) {
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            certificate.proof = LC_IntersectionProof::AnalyticExclusion;
            certificate.proofDomain = box;
        }
        return certificate;
    };
    auto options = strictOptions();
    options.tolerance = 1.0;
    const auto result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    checkStrictWork(result, options);
    CHECK(result.intersections.front().isolation.parameterA.contains(0.5));
    CHECK(result.intersections.front().isolation.parameterB.contains(1.5));
    CHECK(result.intersections.front().proof == LC_IntersectionProof::IntervalUniqueTransverse);
    CHECK(result.work.certificateBoxPairs > 0);
}

TEST_CASE("Strict analytic tangency and cusp certificates carry explicit classifications",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    for (const bool cusp : {false, true}) {
        CertifiedPolynomials curves{cusp ? std::vector<PolynomialPiece>{
            {{0, -1, 1}, {0, 0, 1, 0}, {0, 0, 0, 1}}, polynomialLine(1, {0, 0}, {0, 1}, -1, 1)} :
            std::vector<PolynomialPiece>{polynomialLine(0, {0, 0}, {1, 0}),
                                        {{1, 0, 1}, {0, 1, 0, 0}, {0.25, -1, 1, 0}}}};
        if (cusp) {
            // (t^2,t^3) is injective on the WHOLE interval despite its stationary interior point.
            curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
                return LC_AnalyticLeafCertificate{true, segment, parameter};
            };
        }
        curves.contactProof = [cusp](const LC_IntersectionDomain& box) {
            LC_AnalyticIntersectionCertificate certificate;
            certificate.proofDomain = box;
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proof = cusp ? LC_IntersectionProof::AnalyticCusp : LC_IntersectionProof::AnalyticTangency;
            certificate.isolationA = certificate.isolationB = LC_Interval::point(cusp ? 0.0 : 0.5);
            certificate.kind = Kind::Tangent;
            return certificate;
        };
        auto options = strictOptions();
        options.maxDepth = options.maxLeafDepth = 0;
        const auto qualified = findIntersections(curves, options);
        REQUIRE(qualified.status == LC_IntersectionStatus::Ok);
        REQUIRE(qualified.intersections.size() == 1);
        CHECK(qualified.intersections.front().proof ==
              (cusp ? LC_IntersectionProof::AnalyticCusp : LC_IntersectionProof::AnalyticTangency));
        CHECK(qualified.intersections.front().kind == Kind::Tangent);
        checkStrictWork(qualified, options);
        curves.contactProof = {};
        const auto unresolved = findIntersections(curves, options);
        CHECK(unresolved.status == LC_IntersectionStatus::LimitExceeded);
        checkStrictWork(unresolved, options);
    }
    CertifiedPolynomials retrace{{polynomialLine(0, {0, 0}, {1, 0}), polynomialLine(1, {0, 0}, {1, 0})}};
    auto options = strictOptions();
    options.maxDepth = 0;
    const auto unresolved = findIntersections(retrace, options);
    CHECK(unresolved.status == LC_IntersectionStatus::LimitExceeded);
    checkStrictWork(unresolved, options);
}

TEST_CASE("Strict certificates preserve distinct end occurrences and proven shared triple incidences",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    SECTION("Coincident source ends stay two occurrences against another branch") {
        CertifiedPolynomials curves{{{{0, 0, 1}, {0, 1, -1, 0}, {0, -0.5, 1.5, -1}},
                                      polynomialLine(1, {0, -1}, {0, 2})}};
        curves.successor = [](const std::size_t branch) -> std::ptrdiff_t { return branch == 0 ? 0 : -1; };
        curves.contactProof = [](const LC_IntersectionDomain& box) {
            LC_AnalyticIntersectionCertificate certificate;
            certificate.proofDomain = box;
            if (box.segmentA == box.segmentB) {
                // This simple closed polynomial has only the construction-shared seam, not another self-root.
                certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
                certificate.proof = LC_IntersectionProof::AnalyticExclusion;
            } else if ((box.parameterA.contains(0.0) || box.parameterA.contains(1.0)) && box.parameterB.contains(0.5)) {
                certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
                certificate.proof = LC_IntersectionProof::AnalyticEndpoint;
                certificate.isolationA = LC_Interval::point(box.parameterA.contains(0.0) ? 0.0 : 1.0);
                certificate.isolationB = LC_Interval::point(0.5);
                certificate.kind = Kind::Endpoint;
            } else {
                certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
                certificate.proof = LC_IntersectionProof::AnalyticExclusion;
            }
            return certificate;
        };
        auto options = strictOptions();
        const auto result = findIntersections(curves, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 2);
        checkStrictWork(result, options);
        CHECK(result.intersections[0].parameterA == 0.0);
        CHECK(result.intersections[1].parameterA == 1.0);
        CHECK(result.intersections[0].parameterB == 0.5);
        CHECK(result.intersections[1].parameterB == 0.5);
    }
    SECTION("Analytically exact common occurrences keep all branch-pair incidences") {
        CertifiedPolynomials curves{{polynomialLine(0, {-0.5, 0}, {1, 0}, 0.5, 1),
            polynomialLine(1, {0, -0.5}, {0, 1}, 0.5, 1), polynomialLine(2, {-0.5, -0.5}, {1, 1}, 0.5, 1)}};
        curves.contactProof = [](const LC_IntersectionDomain& box) {
            LC_AnalyticIntersectionCertificate certificate;
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proof = LC_IntersectionProof::AnalyticEndpoint;
            certificate.proofDomain = box;
            certificate.isolationA = certificate.isolationB = LC_Interval::point(0.5);
            return certificate;
        };
        const auto options = strictOptions();
        const auto result = findIntersections(curves, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 3);
        checkStrictWork(result, options);
        CHECK(result.intersections[0].branchA == 0);
        CHECK(result.intersections[0].branchB == 1);
        CHECK(result.intersections[1].branchA == 0);
        CHECK(result.intersections[1].branchB == 2);
        CHECK(result.intersections[2].branchA == 1);
        CHECK(result.intersections[2].branchB == 2);
    }
}

TEST_CASE("Strict stationary leaves require whole-interval analytic injectivity",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{{{0, 0, 1}, {0, 0, 1, 0}, {0, 0, 0, 0}}}};
    auto options = strictOptions();
    options.maxLeafDepth = 0;
    const auto unknown = findIntersections(curves, options);
    CHECK(unknown.status == LC_IntersectionStatus::LimitExceeded);
    checkStrictWork(unknown, options);
    curves.leafProof = [](const std::size_t segment, const LC_Interval& parameter) {
        return LC_AnalyticLeafCertificate{true, segment, parameter}; // t^2 is injective on [0,1]
    };
    options.maxGeneratedItems = 3; // preparation root + analytic leaf certificate + accepted leaf
    const auto qualified = findIntersections(curves, options);
    REQUIRE(qualified.status == LC_IntersectionStatus::Ok);
    CHECK(qualified.intersections.empty());
    CHECK(qualified.work.generatedItems == 3);
    CHECK(qualified.work.generatedLeaves == 1);
    CHECK(qualified.work.certificateBoxPairs == 1);
    checkStrictWork(qualified, options);
    for (int invalid = 0; invalid < 3; ++invalid) {
        curves.leafProof = [invalid](const std::size_t segment, const LC_Interval& parameter) {
            return LC_AnalyticLeafCertificate{true, invalid == 0 ? segment + 1 : segment,
                invalid == 1 ? LC_Interval::point(parameter.lo()) :
                invalid == 2 ? LC_Interval::hull(-1.0, parameter.hi()) : parameter};
        };
        const auto rejected = findIntersections(curves, options);
        CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
        checkStrictWork(rejected, options);
    }
}

TEST_CASE("Strict malformed contacts and missing endpoint certificates never publish partial events",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}), polynomialLine(1, {1, 0}, {0, 1})}};
    auto options = strictOptions();
    options.maxDepth = 0;
    const auto missing = findIntersections(curves, options);
    CHECK(missing.status == LC_IntersectionStatus::LimitExceeded);
    checkStrictWork(missing, options);
    for (int invalid = -1; invalid < 7; ++invalid) {
        CAPTURE(invalid);
        curves.contactProof = [invalid](const LC_IntersectionDomain& box) {
            LC_AnalyticIntersectionCertificate certificate;
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proof = LC_IntersectionProof::AnalyticEndpoint;
            certificate.proofDomain = box;
            certificate.isolationA = LC_Interval::point(1.0);
            certificate.isolationB = LC_Interval::point(0.0);
            if (invalid == 0) { certificate.proofDomain.segmentA = 100; }
            if (invalid == 1) { certificate.proofDomain.parameterA = LC_Interval::hull(-1.0, 1.0); }
            if (invalid == 2) { certificate.proofDomain.parameterA = LC_Interval::point(1.0); }
            if (invalid == 3) { certificate.isolationA = LC_Interval::point(std::numeric_limits<double>::infinity()); }
            if (invalid == 4) { certificate.isolationB = LC_Interval::point(2.0); }
            if (invalid == 5) { certificate.proof = LC_IntersectionProof::Uncertified; }
            if (invalid == 6) {
                certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
                certificate.proof = LC_IntersectionProof::Uncertified;
            }
            return certificate;
        };
        const auto result = findIntersections(curves, options);
        CHECK(result.status == (invalid == -1 ? LC_IntersectionStatus::Ok : invalid >= 5 ?
                               LC_IntersectionStatus::AmbiguousTopology : LC_IntersectionStatus::InvalidInput));
        checkStrictWork(result, options);
        if (invalid == -1) {
            REQUIRE(result.intersections.size() == 1);
            CHECK(result.intersections.front().kind == Kind::Endpoint);
        }
    }
    CertifiedPolynomials partial{{polynomialLine(0, {0, 0}, {1, 0}),
        polynomialLine(1, {0.25, -0.5}, {0, 1}), polynomialLine(2, {0.75, -0.5}, {0, 1})}};
    partial.contactProof = [](const LC_IntersectionDomain& box) {
        LC_AnalyticIntersectionCertificate certificate;
        if (box.segmentB == 2) {
            certificate.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            certificate.proofDomain.segmentA = 100;
        }
        return certificate;
    };
    const auto rejected = findIntersections(partial, options);
    CHECK(rejected.status == LC_IntersectionStatus::InvalidInput);
    CHECK(rejected.work.generatedItems > 3); // a prior certified root existed internally
    checkStrictWork(rejected, options);
}

TEST_CASE("Strict preflight rejects invalid point bounds seeds and noncontiguous branch domains",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    for (const bool disjoint : {false, true}) {
        for (int invalid = 0; invalid < 4; ++invalid) {
            std::vector<PolynomialPiece> pieces{polynomialLine(0, {0, 0}, {1, 0})};
            if (disjoint) { pieces.push_back(polynomialLine(1, {0, 10}, {1, 0})); }
            CertifiedPolynomials curves{pieces};
            curves.missingPointBounds = invalid == 0;
            curves.unbounded = invalid == 1;
            curves.invalidSeed = invalid == 2;
            curves.outOfPlaneSeed = invalid == 3;
            const auto options = strictOptions();
            const auto result = findIntersections(curves, options);
            CHECK(result.status == LC_IntersectionStatus::InvalidInput);
            checkStrictWork(result, options);
        }
    }
    for (const double boundary : {0.4, 0.6}) {
        CertifiedPolynomials gap{{polynomialLine(0, {0, 0}, {1, 0}, 0.0, boundary),
                                  polynomialLine(0, {0, 0}, {1, 0}, 0.5, 1.0)}};
        const auto options = strictOptions();
        const auto result = findIntersections(gap, options);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput);
        CHECK(result.work.generatedItems == 0);
        checkStrictWork(result, options);
    }
    CertifiedPolynomials oversized{{polynomialLine(0, {0, 0}, {1, 0})}, std::numeric_limits<std::size_t>::max()};
    const auto options = strictOptions();
    const auto result = findIntersections(oversized, options);
    CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(result.work.generatedItems == 0);
    checkStrictWork(result, options);
}

TEST_CASE("Strict generated-item pair and depth allowances are checked without refunds or overflow",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    CertifiedPolynomials curves{{polynomialLine(0, {0, 0}, {1, 0}),
        polynomialLine(1, {0.25, -0.5}, {0, 1}), polynomialLine(2, {0.75, -0.5}, {0, 1})}};
    auto options = strictOptions();
    const auto baseline = findIntersections(curves, options);
    REQUIRE(baseline.status == LC_IntersectionStatus::Ok);
    REQUIRE(baseline.intersections.size() == 2);
    REQUIRE(baseline.work.certificateBoxPairs > 0);
    for (const bool generated : {false, true}) {
        options = strictOptions();
        if (generated) { options.maxGeneratedItems = baseline.work.generatedItems; }
        else { options.maxBoxPairs = baseline.work.totalBoxPairs; }
        const auto exact = findIntersections(curves, options);
        REQUIRE(exact.status == LC_IntersectionStatus::Ok);
        CHECK(exact.intersections.size() == 2);
        checkStrictWork(exact, options);
        if (generated) { --options.maxGeneratedItems; } else { --options.maxBoxPairs; }
        const auto shortBudget = findIntersections(curves, options);
        CHECK(shortBudget.status == LC_IntersectionStatus::LimitExceeded);
        checkStrictWork(shortBudget, options);
    }
    options = strictOptions();
    options.maxGeneratedItems = 0;
    const auto zeroGenerated = findIntersections(curves, options);
    CHECK(zeroGenerated.status == LC_IntersectionStatus::InvalidInput);
    checkStrictWork(zeroGenerated, options);
    options = strictOptions();
    options.maxBoxPairs = 0;
    const auto noPairs = findIntersections(curves, options);
    CHECK(noPairs.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(noPairs.work.totalBoxPairs == 0);
    checkStrictWork(noPairs, options);
    options = strictOptions();
    options.maxLeaves = 0;
    const auto noLeaves = findIntersections(curves, options);
    CHECK(noLeaves.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(noLeaves.work.generatedItems == 0); // mandatory leaf capacity is checked before preparation
    checkStrictWork(noLeaves, options);
    CertifiedPolynomials stationary{{{{0, 0, 1}, {0, 0, 1, 0}, {0, 0, 0, 0}}}};
    options = strictOptions();
    options.maxGeneratedItems = 1;
    const auto noChildren = findIntersections(stationary, options);
    CHECK(noChildren.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(noChildren.work.generatedItems == 1);
    checkStrictWork(noChildren, options);
    options = strictOptions();
    options.maxDepth = 0;
    const auto provedWithoutSubdivision = findIntersections(curves, options);
    CHECK(provedWithoutSubdivision.status == LC_IntersectionStatus::Ok);
    checkStrictWork(provedWithoutSubdivision, options);
    options.maxBoxPairs = options.maxGeneratedItems = options.maxLeaves = std::numeric_limits<std::size_t>::max();
    const auto enormousAllowances = findIntersections(curves, options);
    CHECK(enormousAllowances.status == LC_IntersectionStatus::Ok);
    checkStrictWork(enormousAllowances, options);
}

TEST_CASE("Strict hierarchy prunes separated two-axis and unanchored clouds within linear work",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    constexpr std::size_t count = 512;
    for (const bool separateY : {false, true}) {
        std::vector<PolynomialPiece> pieces;
        for (std::size_t i = 0; i < count; ++i) {
            const double offset = 4.0 * static_cast<double>(i);
            pieces.push_back(polynomialLine(i, separateY ? RS_Vector{0, offset} : RS_Vector{offset, 0}, {1, 0}));
        }
        auto options = strictOptions();
        options.maxBoxPairs = 4 * count;
        options.maxGeneratedItems = 2 * count; // one preparation root and one accepted leaf per source segment
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        CHECK(result.work.refinementBoxPairs == 0);
        checkStrictWork(result, options);
    }
    for (const std::size_t cloudCount : {128u, 256u, 512u, 1024u}) {
        CAPTURE(cloudCount);
        std::vector<PolynomialPiece> pieces{polynomialLine(0, {0, 1e6}, {1, 0})};
        for (std::size_t i = 1; i <= cloudCount; ++i) {
            pieces.push_back(polynomialLine(i, {0, 0}, {1, 0})); // all unanchored images overlap
        }
        auto options = strictOptions();
        options.maxBoxPairs = 8 * pieces.size();
        options.maxGeneratedItems = 2 * pieces.size(); // distinct mandatory roots and accepted leaves
        options.anchoredBranches = 1;
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        CHECK(result.work.refinementBoxPairs == 0);
        CHECK(result.work.totalBoxPairs < options.maxBoxPairs);
        checkStrictWork(result, options);
    }
    SECTION("bit-reversed y separation keeps the adaptive hierarchy within its linear pair envelope") {
        constexpr std::size_t n = 128;
        std::vector<PolynomialPiece> pieces;
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t reversed = 0;
            for (unsigned bit = 0; bit < 7; ++bit) { reversed = 2 * reversed + ((i >> bit) & 1u); }
            pieces.push_back(polynomialLine(i, {std::ldexp(double(i), -20), 4.0 * double(reversed)}, {1, 0}));
        }
        auto options = strictOptions();
        options.maxGeneratedItems = 2 * n;
        options.maxBoxPairs = 4 * n;
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        CAPTURE(result.work.candidateBoxPairs, result.work.generatedItems, result.work.orderingComparisons);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        // Every adaptive split separates y ranges, so only the n-1 internal sibling pairs are examined.
        CHECK(result.work.candidateBoxPairs == n - 1);
        CHECK(result.work.refinementBoxPairs == 0);
        // Seven levels scan 7*n-(n-1) additional centres: four reductions each, one axis and two index reductions/node.
        CHECK(result.work.orderingComparisons >= 4 * (7 * n - (n - 1)) + 3 * (n - 1));
        CHECK(result.work.orderingComparisons <= 4 * (7 * n - (n - 1)) + 3 * (n - 1) + 2 * n * 28);
        checkStrictWork(result, options);
    }
}

TEST_CASE("Strict cached full-path projection qualifies contiguous multi-segment branches",
          "[lc-offsetcurve][geometry][intersection-strict][curve-offset][topology]") {
    for (const std::size_t count : {255u, 256u, 257u}) {
        std::vector<PolynomialPiece> pieces;
        for (std::size_t i = 0; i < count; ++i) {
            pieces.push_back(polynomialLine(0, {0, 0}, {1, 0}, static_cast<double>(i), static_cast<double>(i + 1)));
        }
        auto options = strictOptions();
        options.maxBoxPairs = 2 * count;
        options.maxGeneratedItems = 2 * count; // distinct mandatory roots and accepted leaves
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        CHECK(result.work.generatedLeaves == count);
        CHECK(result.work.generatedItems == 2 * count);
        CHECK(result.work.refinementBoxPairs == 0);
        CHECK(result.work.certificateBoxPairs == 0);
        checkStrictWork(result, options);
    }
}

TEST_CASE("Dense overlapping leaves exhaust the shared budget during hierarchy traversal",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const RS_Spline segment = line({0, 0}, {1, 1});
    // More than 130000 possible pairs, but only the permitted prefix may be examined or retained.
    const SplineCurves curves{std::vector<const RS_Spline*>(512, &segment)};
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    for (const std::size_t cap : {0u, 1u, 7u}) {
        CAPTURE(cap);
        options.maxBoxPairs = cap;
        const LC_IntersectionResult result = findIntersections(curves, options);
        REQUIRE(result.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(result.intersections.empty());
        checkWork(result, options);
        CHECK(result.work.candidateBoxPairs == cap);
        CHECK(result.work.refinementBoxPairs == 0);
        CHECK(result.work.totalBoxPairs == cap);
        CHECK(result.work.generatedLeaves == curves.segments().size());
    }
}

TEST_CASE("An exact shared intersection budget succeeds and one less discards partial roots",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const RS_Spline a = line({0, 0}, {2, 2});
    const RS_Spline b = line({0, 2}, {2, 0});
    const RS_Spline c = line({1, -1}, {1, 3});
    const SplineCurves curves{{&a, &b, &c}};
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    const LC_IntersectionResult baseline = findIntersections(curves, options);
    REQUIRE(baseline.status == LC_IntersectionStatus::Ok);
    REQUIRE(baseline.intersections.size() == 3);
    checkWork(baseline, options);
    CHECK(baseline.work.candidateBoxPairs >= 3);
    REQUIRE(baseline.work.refinementBoxPairs >= 3);
    CHECK(baseline.work.generatedLeaves == 3);

    for (const std::size_t cap : {baseline.work.totalBoxPairs, std::numeric_limits<std::size_t>::max()}) {
        CAPTURE(cap);
        options.maxBoxPairs = cap;
        const LC_IntersectionResult exact = findIntersections(curves, options);
        REQUIRE(exact.status == LC_IntersectionStatus::Ok);
        checkWork(exact, options);
        CHECK(exact.work.candidateBoxPairs == baseline.work.candidateBoxPairs);
        CHECK(exact.work.refinementBoxPairs == baseline.work.refinementBoxPairs);
        CHECK(exact.work.totalBoxPairs == baseline.work.totalBoxPairs);
        CHECK(exact.work.generatedLeaves == baseline.work.generatedLeaves);
        REQUIRE(exact.intersections.size() == baseline.intersections.size());
        for (std::size_t i = 0; i < exact.intersections.size(); ++i) {
            const LC_ParametricIntersection& x = exact.intersections[i];
            const LC_ParametricIntersection& expected = baseline.intersections[i];
            CHECK(x.branchA == expected.branchA);
            CHECK(x.parameterA == expected.parameterA);
            CHECK(x.branchB == expected.branchB);
            CHECK(x.parameterB == expected.parameterB);
            CHECK(x.point == expected.point);
            CHECK(x.kind == expected.kind);
        }
    }

    options.maxBoxPairs = baseline.work.totalBoxPairs - 1;
    const LC_IntersectionResult refused = findIntersections(curves, options);
    REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
    CHECK(refused.intersections.empty());
    checkWork(refused, options);
    CHECK(refused.work.candidateBoxPairs == baseline.work.candidateBoxPairs);
    CHECK(refused.work.refinementBoxPairs == baseline.work.refinementBoxPairs - 1);
    CHECK(refused.work.totalBoxPairs == options.maxBoxPairs);
    CHECK(refused.work.generatedLeaves == 3);
}

TEST_CASE("Overlapping X bounds separated on Y are pruned with a linear pair budget",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    constexpr std::size_t count = 256;
    std::vector<RS_Spline> splines;
    splines.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double y = 4.0 * static_cast<double>(i);
        splines.push_back(line({0.0, y}, {1.0, y}));
    }
    std::vector<const RS_Spline*> branches;
    for (const RS_Spline& spline : splines) {
        branches.push_back(&spline);
    }
    const SplineCurves curves{branches};
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.maxBoxPairs = count - 1;
    const LC_IntersectionResult result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    checkWork(result, options);
    CHECK(result.intersections.empty());
    CHECK(result.work.candidateBoxPairs == count - 1);
    CHECK(result.work.refinementBoxPairs == 0);
    CHECK(result.work.generatedLeaves == count);

    --options.maxBoxPairs;
    const LC_IntersectionResult refused = findIntersections(curves, options);
    REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
    checkWork(refused, options);
    CHECK(refused.work.totalBoxPairs == options.maxBoxPairs);
    CHECK(refused.work.refinementBoxPairs == 0);
    CHECK(refused.intersections.empty());
}

TEST_CASE("An anchored cloud does not enumerate the unanchored cloud's internal pairs",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    constexpr std::size_t count = 256;
    const RS_Spline anchor = line({-1, 0}, {1, 0});
    const RS_Spline crossing = line({0, -1}, {0, 1});
    std::vector<const RS_Spline*> branches(count, &crossing);
    branches.front() = &anchor;
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.anchoredBranches = 1;
    options.maxBoxPairs = 8 * count;
    const LC_IntersectionResult result = findIntersections(SplineCurves{branches}, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    checkWork(result, options);
    REQUIRE(result.intersections.size() == count - 1);
    CHECK(result.work.generatedLeaves == count);
    CHECK(result.work.totalBoxPairs < options.maxBoxPairs);
    for (std::size_t i = 0; i < result.intersections.size(); ++i) {
        const LC_ParametricIntersection& x = result.intersections[i];
        CHECK(x.branchA == 0);
        CHECK(x.branchB == i + 1);
        CHECK(x.kind == Kind::Transverse);
        CHECK(x.point.distanceTo(RS_Vector{0, 0}) < options.tolerance);
    }
}

TEST_CASE("Hierarchy intersections match a brute-force walk over eligible branch pairs",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const std::vector<RS_Spline> splines{
        line({-3, 0}, {3, 0}), line({0, -3}, {0, 3}), line({-2, -2}, {2, 2}),
        line({-2, 2}, {2, -2}), line({5, 5}, {6, 6}), line({-3, 1}, {3, 1})};
    std::vector<const RS_Spline*> branches;
    for (const RS_Spline& spline : splines) {
        branches.push_back(&spline);
    }
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    for (const std::size_t anchored : {std::size_t{2}, std::numeric_limits<std::size_t>::max()}) {
        CAPTURE(anchored);
        options.anchoredBranches = anchored;
        const LC_IntersectionResult result = findIntersections(SplineCurves{branches}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        checkWork(result, options);
        std::vector<LC_ParametricIntersection> expected;
        for (std::size_t a = 0; a < branches.size(); ++a) {
            for (std::size_t b = a + 1; b < branches.size(); ++b) {
                if (a >= anchored && b >= anchored) {
                    continue;
                }
                const LC_IntersectionResult pair = intersect({branches[a], branches[b]}, options.tolerance);
                REQUIRE(pair.status == LC_IntersectionStatus::Ok);
                for (LC_ParametricIntersection x : pair.intersections) {
                    REQUIRE(x.branchA == 0);
                    REQUIRE(x.branchB == 1);
                    x.branchA = a;
                    x.branchB = b;
                    expected.push_back(x);
                }
            }
        }
        const auto key = [](const LC_ParametricIntersection& x) {
            return std::make_tuple(x.branchA, x.parameterA, x.branchB, x.parameterB);
        };
        std::sort(expected.begin(), expected.end(), [&](const auto& a, const auto& b) { return key(a) < key(b); });
        REQUIRE_FALSE(expected.empty());
        REQUIRE(result.intersections.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            const LC_ParametricIntersection& x = result.intersections[i];
            CHECK(x.branchA == expected[i].branchA);
            CHECK(x.parameterA == Approx(expected[i].parameterA).margin(1e-12));
            CHECK(x.branchB == expected[i].branchB);
            CHECK(x.parameterB == Approx(expected[i].parameterB).margin(1e-12));
            CHECK(x.kind == expected[i].kind);
            CHECK(x.point.distanceTo(expected[i].point) < options.tolerance);
        }
    }
}

TEST_CASE("Aggregate bridge pruning requires a complete single-branch proof",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;

    SECTION("Many monotone segments with overlapping loose boxes use a linear pair budget") {
        for (const std::size_t count : {255u, 256u, 257u}) {
            CAPTURE(count);
            std::vector<RS_Vector> controls;
            std::vector<double> knots{0.0};
            for (std::size_t i = 0; i <= count; ++i) {
                controls.emplace_back(static_cast<double>(i), 0.0);
                knots.push_back(static_cast<double>(i));
            }
            knots.push_back(static_cast<double>(count));
            const RS_Spline chain = makeSpline(1, controls, knots);
            REQUIRE(chain.validate());
            const SplineCurves curves{{&chain}, true};
            REQUIRE(curves.segments().size() == count);
            options.maxBoxPairs = count - 1;
            const LC_IntersectionResult result = findIntersections(curves, options);
            REQUIRE(result.status == LC_IntersectionStatus::Ok);
            checkWork(result, options);
            CHECK(result.intersections.empty());
            CHECK(result.work.candidateBoxPairs == count - 1);
            CHECK(result.work.refinementBoxPairs == 0);
            CHECK(result.work.generatedLeaves == count);

            --options.maxBoxPairs;
            const LC_IntersectionResult refused = findIntersections(curves, options);
            REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
            checkWork(refused, options);
            CHECK(refused.work.totalBoxPairs == options.maxBoxPairs);
            CHECK(refused.work.refinementBoxPairs == 0);
            CHECK(refused.intersections.empty());
        }
    }

    SECTION("Separate monotone branches still report their crossing") {
        const RS_Spline a = line({-1, 0}, {1, 0});
        const RS_Spline b = line({0, -1}, {0, 1});
        const LC_IntersectionResult result = findIntersections(SplineCurves{{&a, &b}, true}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        checkWork(result, options);
        REQUIRE(result.intersections.size() == 1);
        CHECK(result.intersections.front().kind == Kind::Transverse);
        CHECK(result.intersections.front().point.distanceTo(RS_Vector{0, 0}) < options.tolerance);
        CHECK(result.work.candidateBoxPairs == 1);
        CHECK(result.work.refinementBoxPairs == 1);
    }

    SECTION("A nonmonotone intervening span prevents pruning a self-crossing") {
        const RS_Spline chain = makeSpline(1, {{0, 0}, {2, 2}, {0, 2}, {2, 0}}, {0, 0, 1, 2, 3, 3});
        const RS_Spline first = line({0, 0}, {2, 2});
        const RS_Spline last = line({0, 2}, {2, 0});
        const LC_IntersectionResult pair = intersect({&first, &last}, options.tolerance);
        REQUIRE(pair.status == LC_IntersectionStatus::Ok);
        REQUIRE(pair.intersections.size() == 1);
        const LC_IntersectionResult result = findIntersections(SplineCurves{{&chain}, true}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        checkWork(result, options);
        REQUIRE(result.intersections.size() == 1);
        const LC_ParametricIntersection& x = result.intersections.front();
        CHECK(x.branchA == 0);
        CHECK(x.branchB == 0);
        CHECK(x.parameterA == Approx(pair.intersections.front().parameterA).margin(1e-12));
        CHECK(x.parameterB == Approx(2.0 + pair.intersections.front().parameterB).margin(1e-12));
        CHECK(x.kind == Kind::Transverse);
        CHECK(x.point.distanceTo(pair.intersections.front().point) < options.tolerance);
    }
}

TEST_CASE("Intersection broad-phase rejections consume work but absent pairs do not",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const RS_Spline a = line({0, 0}, {1, 0});
    const RS_Spline xSeparated = line({2, 0}, {3, 0});
    const RS_Spline ySeparated = line({0, 2}, {1, 2});
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;

    for (const RS_Spline* other : {&xSeparated, &ySeparated}) {
        const SplineCurves curves{{&a, other}};
        options.maxBoxPairs = 0;
        const LC_IntersectionResult refused = findIntersections(curves, options);
        REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
        checkWork(refused, options);
        CHECK(refused.work.totalBoxPairs == 0);
        CHECK(refused.work.generatedLeaves == 2);
        CHECK(refused.intersections.empty());

        options.maxBoxPairs = 1;
        const LC_IntersectionResult excluded = findIntersections(curves, options);
        REQUIRE(excluded.status == LC_IntersectionStatus::Ok);
        checkWork(excluded, options);
        CHECK(excluded.work.candidateBoxPairs == 1);
        CHECK(excluded.work.refinementBoxPairs == 0);
        CHECK(excluded.work.totalBoxPairs == 1);
        CHECK(excluded.work.generatedLeaves == 2);
        CHECK(excluded.intersections.empty());
    }

    options.maxBoxPairs = 0;
    const LC_IntersectionResult single = findIntersections(SplineCurves{{&a}}, options);
    REQUIRE(single.status == LC_IntersectionStatus::Ok);
    checkWork(single, options);
    CHECK(single.work.totalBoxPairs == 0);
    CHECK(single.work.generatedLeaves == 1);
    CHECK(single.intersections.empty());

    options.anchoredBranches = 0;
    const LC_IntersectionResult unanchored = findIntersections(SplineCurves{{&a, &a}}, options);
    REQUIRE(unanchored.status == LC_IntersectionStatus::Ok);
    checkWork(unanchored, options);
    CHECK(unanchored.work.totalBoxPairs == 0);
    CHECK(unanchored.work.generatedLeaves == 2);
    CHECK(unanchored.intersections.empty());
}

TEST_CASE("Intersection work is reported for invalid input and leaf or topology refusals",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const RS_Spline a = line({0, 0}, {4, 0});
    const RS_Spline overlapping = line({2, 0}, {6, 0});
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;

    SECTION("Invalid tolerance and empty input consume no work") {
        for (const double tolerance : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                                       std::numeric_limits<double>::quiet_NaN()}) {
            options.tolerance = tolerance;
            const LC_IntersectionResult invalid = findIntersections(SplineCurves{{&a}}, options);
            REQUIRE(invalid.status == LC_IntersectionStatus::InvalidInput);
            checkWork(invalid, options);
            CHECK(invalid.work.totalBoxPairs == 0);
            CHECK(invalid.work.generatedLeaves == 0);
            CHECK(invalid.intersections.empty());
        }
        options.tolerance = 1e-9;
        const LC_IntersectionResult empty = findIntersections(SplineCurves{std::vector<const RS_Spline*>{}}, options);
        REQUIRE(empty.status == LC_IntersectionStatus::InvalidInput);
        checkWork(empty, options);
        CHECK(empty.work.totalBoxPairs == 0);
        CHECK(empty.work.generatedLeaves == 0);
        CHECK(empty.intersections.empty());
    }

    SECTION("A leaf cap reports the generated prefix without any pair work") {
        for (const std::size_t cap : {0u, 1u}) {
            CAPTURE(cap);
            options.maxLeaves = cap;
            const LC_IntersectionResult refused = findIntersections(SplineCurves{{&a, &overlapping}}, options);
            REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
            checkWork(refused, options);
            CHECK(refused.work.generatedLeaves == cap);
            CHECK(refused.work.totalBoxPairs == 0);
            CHECK(refused.intersections.empty());
        }
    }

    SECTION("An unresolvable leaf reports no generated leaves or pair work") {
        const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}},
                                         {0, 0, 0, 0, 1, 1, 1, 1});
        options.maxLeafDepth = 0;
        const LC_IntersectionResult ambiguous = findIntersections(SplineCurves{{&loop}}, options);
        REQUIRE(ambiguous.status == LC_IntersectionStatus::AmbiguousTopology);
        checkWork(ambiguous, options);
        CHECK(ambiguous.work.generatedLeaves == 0);
        CHECK(ambiguous.work.totalBoxPairs == 0);
        CHECK(ambiguous.intersections.empty());
    }

    SECTION("An ambiguous contact reports both phases of pair work") {
        options.maxDepth = 0;
        const LC_IntersectionResult ambiguous = findIntersections(SplineCurves{{&a, &overlapping}}, options);
        REQUIRE(ambiguous.status == LC_IntersectionStatus::AmbiguousTopology);
        checkWork(ambiguous, options);
        CHECK(ambiguous.work.generatedLeaves == 2);
        CHECK(ambiguous.work.candidateBoxPairs == 1);
        CHECK(ambiguous.work.refinementBoxPairs == 1);
        CHECK(ambiguous.work.totalBoxPairs == 2);
        CHECK(ambiguous.intersections.empty());
    }
}

TEST_CASE("Contact classification refuses an underflowed initial probe step",
          "[lc-offsetcurve][geometry][intersection-progress][curve-offset][topology]") {
    const double scale = 1e200;
    const RS_Vector direction{scale, scale};
    const AnalyticLines curves{{scale, 0.0}, direction};
    LC_IntersectionOptions options;
    options.tolerance = 1e-200;
    options.maxDepth = 0;
    REQUIRE(std::isfinite(direction.magnitude()));
    REQUIRE(8.0 * options.tolerance / direction.magnitude() == 0.0);

    const LC_IntersectionResult result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.intersections.empty());
    CHECK(result.work.refinementBoxPairs == 1);
    checkWork(result, options);
}

TEST_CASE("Contact classification refuses a nonfinite speed with finite derivative components",
          "[lc-offsetcurve][geometry][intersection-progress][curve-offset][topology]") {
    const double scale = 1.4e308;
    const RS_Vector direction{scale, scale};
    const AnalyticLines curves{{scale, 0.0}, direction};
    LC_IntersectionOptions options;
    options.tolerance = 1.0;
    options.maxDepth = 0;
    REQUIRE(std::isfinite(direction.x));
    REQUIRE(std::isfinite(direction.y));
    REQUIRE_FALSE(std::isfinite(direction.magnitude()));

    const LC_IntersectionResult result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.intersections.empty());
    CHECK(result.work.refinementBoxPairs == 1);
    checkWork(result, options);
}

TEST_CASE("Contact classification refuses an overflowed initial probe step",
          "[lc-offsetcurve][geometry][intersection-progress][curve-offset][topology]") {
    const double scale = 1e308;
    const RS_Vector direction{0.0, scale};
    const AnalyticLines curves{{scale, 0.0}, direction};
    LC_IntersectionOptions options;
    options.tolerance = 3e307;
    options.maxDepth = 0;
    REQUIRE(std::isfinite(direction.magnitude()));
    REQUIRE_FALSE(std::isfinite(8.0 * options.tolerance / direction.magnitude()));
    REQUIRE(RS_Vector(-0.5 * scale, 0.0).distanceTo({0.0, -0.5 * scale}) > options.tolerance);

    const LC_IntersectionResult result = findIntersections(curves, options);
    REQUIRE(result.status == LC_IntersectionStatus::AmbiguousTopology);
    CHECK(result.intersections.empty());
    CHECK(result.work.refinementBoxPairs == 1);
    checkWork(result, options);
}

TEST_CASE("Positive contact probe steps grow through rounded parameters to a shallow crossing",
          "[lc-offsetcurve][geometry][intersection-progress][curve-offset][topology]") {
    const double t0 = 1e16;
    const double t1 = t0 + 16.0;
    const double middle = t0 + 8.0;
    const RS_Vector direction{1.0, 1e-14};
    const AnalyticLines curves{{1.0, 0.0}, direction, t0, t1};
    LC_IntersectionOptions options;
    options.maxDepth = 0;
    for (const double tolerance : {1e-16, std::numeric_limits<double>::denorm_min()}) {
        CAPTURE(tolerance);
        options.tolerance = tolerance;
        const double step = 8.0 * tolerance / direction.magnitude();
        REQUIRE(std::isfinite(step));
        REQUIRE(step > 0.0);
        REQUIRE(middle + step == middle);
        REQUIRE(middle - step == middle);
        REQUIRE(8.0 * direction.y > tolerance);

        const LC_IntersectionResult result = findIntersections(curves, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 1);
        checkWork(result, options);
        CHECK(result.work.refinementBoxPairs == 1);
        const LC_ParametricIntersection& contact = result.intersections.front();
        CHECK(contact.kind == Kind::Transverse);
        CHECK(contact.branchA == 0);
        CHECK(contact.branchB == 1);
        CHECK(contact.parameterA == middle);
        CHECK(contact.parameterB == middle);
        CHECK(contact.parameterA > t0);
        CHECK(contact.parameterA < t1);
        CHECK(contact.parameterB > t0);
        CHECK(contact.parameterB < t1);
        CHECK(contact.point.x == 0.0);
        CHECK(contact.point.y == 0.0);
    }
}

TEST_CASE("A loop crosses itself once, found as two occurrences on one branch", "[curve-offset][topology]") {
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    REQUIRE(sampledCrossings({&loop}) == 1);
    const LC_IntersectionResult result = intersect({&loop});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    const LC_ParametricIntersection& x = result.intersections.front();
    CHECK(x.branchA == 0);
    CHECK(x.branchB == 0);
    CHECK(x.parameterA < x.parameterB);
    CHECK(x.kind == Kind::Transverse);
    // both occurrences are the point, to the tolerance
    CHECK(loop.getPointAt(x.parameterA).distanceTo(x.point) < 1e-9);
    CHECK(loop.getPointAt(x.parameterB).distanceTo(x.point) < 1e-9);
    // by symmetry about x = 5
    CHECK(x.point.x == Approx(5.0).margin(1e-9));
    CHECK(x.parameterA + x.parameterB == Approx(1.0).margin(1e-9));
}

TEST_CASE("A closed figure eight crosses itself at its waist", "[curve-offset][topology]") {
    RS_Spline eight(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{20, 10}, RS_Vector{20, -10}, RS_Vector{-20, 10}, RS_Vector{-20, -10}}) {
        eight.addControlPoint(p);
    }
    eight.setClosed(true);
    REQUIRE(eight.validate());
    const size_t sampled = sampledCrossings({&eight});
    const LC_IntersectionResult result = intersect({&eight});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(result.intersections.size() == sampled);
    REQUIRE_FALSE(result.intersections.empty());
    CHECK(result.intersections.front().point.distanceTo(RS_Vector{0.0, 0.0}) < 1e-9);
}

TEST_CASE("A touch is a tangency; a near touch that crosses is two crossings", "[curve-offset][topology]") {
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1}); // y = x^2
    const RS_Spline axis = line({-3, 0}, {3, 0});
    const LC_IntersectionResult touch = intersect({&parabola, &axis});
    REQUIRE(touch.status == LC_IntersectionStatus::Ok);
    REQUIRE(touch.intersections.size() == 1);
    CHECK(touch.intersections.front().kind == Kind::Tangent);
    CHECK(touch.intersections.front().point.distanceTo(RS_Vector{0.0, 0.0}) < 1e-6);

    // y = x^2 against y = 1e-4: crossings at x = -0.01 and 0.01
    const RS_Spline raised = line({-3, 1e-4}, {3, 1e-4});
    const LC_IntersectionResult crossing = intersect({&parabola, &raised});
    REQUIRE(crossing.status == LC_IntersectionStatus::Ok);
    REQUIRE(crossing.intersections.size() == 2);
    CHECK(crossing.intersections[0].point.x == Approx(-0.01).margin(1e-9));
    CHECK(crossing.intersections[1].point.x == Approx(0.01).margin(1e-9));
    for (const LC_ParametricIntersection& x : crossing.intersections) {
        CHECK(x.kind == Kind::Transverse);
    }
}

TEST_CASE("A triple point keeps all three incident pairs", "[curve-offset][topology]") {
    const RS_Spline a = line({0, 0}, {2, 2});
    const RS_Spline b = line({0, 2}, {2, 0});
    const RS_Spline c = line({1, -1}, {1, 3});
    const LC_IntersectionResult result = intersect({&a, &b, &c});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 3);
    CHECK((result.intersections[0].branchA == 0 && result.intersections[0].branchB == 1));
    CHECK((result.intersections[1].branchA == 0 && result.intersections[1].branchB == 2));
    CHECK((result.intersections[2].branchA == 1 && result.intersections[2].branchB == 2));
    for (const LC_ParametricIntersection& x : result.intersections) {
        CHECK(x.point.distanceTo(RS_Vector{1.0, 1.0}) < 1e-12);
        CHECK(x.kind == Kind::Transverse);
    }
}

TEST_CASE("A branch that ends on another meets it at an endpoint", "[curve-offset][topology]") {
    const RS_Spline base = line({0, 0}, {4, 0});
    const RS_Spline post = line({2, 0}, {2, 3});
    const LC_IntersectionResult result = intersect({&base, &post});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1);
    CHECK(result.intersections.front().kind == Kind::Endpoint);
    CHECK(result.intersections.front().point.distanceTo(RS_Vector{2.0, 0.0}) < 1e-12);
}

TEST_CASE("Branches that retrace each other are ambiguous, not a crossing", "[curve-offset][topology]") {
    const RS_Spline curve = makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline copy = makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK(intersect({&curve, &copy}).status == LC_IntersectionStatus::AmbiguousTopology);
    const RS_Spline first = line({0, 0}, {4, 0});
    const RS_Spline overlapping = line({2, 0}, {6, 0});
    CHECK(intersect({&first, &overlapping}).status == LC_IntersectionStatus::AmbiguousTopology);
}

TEST_CASE("The two sides of a corner in a branch may cross again", "[curve-offset][topology]") {
    // As trimming joins the offsets on the two sides of a swallowtail where
    // they cross: one quadratic piece on each side of the corner (-10, 0),
    // each monotone, consecutive along the branch, and meeting again on the
    // axis at x = 7.6. Consecutive leaves were taken to share their end only.
    const RS_Spline lens = makeSpline(2, {{12, -2}, {1, 4}, {-10, 0}, {1, -4}, {12, 2}}, {0, 0, 0, 1, 1, 2, 2, 2});
    REQUIRE(sampledCrossings({&lens}) == 1);
    const LC_IntersectionResult result = intersect({&lens});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    REQUIRE(result.intersections.size() == 1); // not the corner itself
    CHECK(result.intersections.front().kind == Kind::Transverse);
    CHECK(result.intersections.front().point.distanceTo(RS_Vector{7.6, 0.0}) < 1e-9);
}

TEST_CASE("A branch that turns back along itself at a corner does not cross itself", "[curve-offset][topology]") {
    // within the tolerance of its other side all the way, the side after the
    // corner is the corner: no crossing, and no box-by-box search of it
    const RS_Spline spike = makeSpline(1, {{10, 0}, {0, 0}, {10, 1e-12}}, {0, 0, 1, 2, 2});
    const LC_IntersectionResult result = intersect({&spike});
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(result.intersections.empty());
    // turned back at a small angle, it leaves the tolerance and is a corner only
    const RS_Spline wedge = makeSpline(1, {{10, 0}, {0, 0}, {10, 1}}, {0, 0, 1, 2, 2});
    const LC_IntersectionResult open = intersect({&wedge});
    REQUIRE(open.status == LC_IntersectionStatus::Ok);
    CHECK(open.intersections.empty());
}

TEST_CASE("A corner exclusion does not hide later crossings on its branch", "[curve-offset][topology]") {
    const RS_Spline branch = makeSpline(2,
        {{8, 1}, {1, 0}, {0, 0}, {1, 0}, {8, -1}, {8, -2}, {4, -2}, {4, 0}, {4, 2}},
        {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4});
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.maxDepth = 80;
    const LC_IntersectionResult result = findIntersections(SplineCurves{{&branch}}, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(std::any_of(result.intersections.begin(), result.intersections.end(), [](const auto& crossing) {
        return crossing.parameterA < 1.0 && crossing.parameterB > 3.0 && crossing.kind == Kind::Transverse &&
               crossing.point.distanceTo(RS_Vector{4, 4.0 / 9.0}) < 1e-9;
    }));
}

TEST_CASE("Unresolved corner intersections respect the depth limit",
          "[lc-offsetcurve][geometry][intersection-budget][curve-offset][topology]") {
    const RS_Spline lens = makeSpline(2, {{12, -2}, {1, 4}, {-10, 0}, {1, -4}, {12, 2}},
                                     {0, 0, 0, 1, 1, 2, 2, 2});
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.maxDepth = 0;
    const LC_IntersectionResult refused = findIntersections(SplineCurves{{&lens}}, options);
    REQUIRE(refused.status == LC_IntersectionStatus::LimitExceeded);
    checkWork(refused, options);
    CHECK(refused.work.generatedLeaves > 0);
    CHECK(refused.work.candidateBoxPairs > 0);
    CHECK(refused.work.refinementBoxPairs > 0);
    CHECK(refused.intersections.empty());
}

TEST_CASE("Curved sides of a tangent corner can separate and cross again", "[curve-offset][topology]") {
    const RS_Spline branch = makeSpline(3,
        {{8, -0.8}, {5, 1.0 / 15.0}, {1, 0}, {0, 0}, {1, 0}, {5, -1.0 / 15.0}, {8, 0.8}},
        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    LC_IntersectionOptions options;
    options.tolerance = 1e-9;
    options.maxDepth = 80;
    const LC_IntersectionResult result = findIntersections(SplineCurves{{&branch}}, options);
    REQUIRE(result.status == LC_IntersectionStatus::Ok);
    CHECK(std::any_of(result.intersections.begin(), result.intersections.end(), [](const auto& crossing) {
        return crossing.kind == Kind::Transverse && crossing.point.distanceTo(RS_Vector{0.928, 0}) < 1e-9;
    }));
}

TEST_CASE("The intersections do not depend on the order of the branches", "[curve-offset][topology]") {
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const RS_Spline a = line({-1, 3}, {11, 3});
    const RS_Spline b = line({5, -2}, {5, 12});
    const LC_IntersectionResult forward = intersect({&loop, &a, &b});
    const LC_IntersectionResult backward = intersect({&b, &a, &loop});
    REQUIRE(forward.status == LC_IntersectionStatus::Ok);
    REQUIRE(backward.status == LC_IntersectionStatus::Ok);
    REQUIRE(forward.intersections.size() == backward.intersections.size());
    REQUIRE(forward.intersections.size() == sampledCrossings({&loop, &a, &b}));
    // the same incidences, with branch k of one order being branch 2 - k of the other
    for (const LC_ParametricIntersection& x : forward.intersections) {
        const bool found = std::any_of(backward.intersections.begin(), backward.intersections.end(),
                                       [&x](const LC_ParametricIntersection& y) {
                                           return y.point.distanceTo(x.point) < 1e-9 && y.kind == x.kind &&
                                                  ((2 - y.branchA == x.branchA && 2 - y.branchB == x.branchB) ||
                                                   (2 - y.branchA == x.branchB && 2 - y.branchB == x.branchA));
                                       });
        CHECK(found);
    }
}

TEST_CASE("A noded offset ends pieces at its crossing", "[curve-offset][topology]") {
    // Inside y = x^2 at distance 1 the offset has cusps at x = -0.383 and 0.383,
    // and its two outer branches cross on the axis where sqrt(1 + 4x^2) = 2:
    // at x = -+sqrt(3) / 2, in the point (0, 1.25).
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0);

    const LC_CurveOffsetGeometryResult plain =
        LC_CurveOffset::buildDirectBranches(parabola, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(plain.status == LC_CurveOffsetStatus::Ok);
    CHECK(plain.intersections.empty());

    options.nodeIntersections = true;
    const LC_CurveOffsetGeometryResult noded =
        LC_CurveOffset::buildDirectBranches(parabola, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(noded.status == LC_CurveOffsetStatus::Ok);
    CHECK(noded.sourceIntersections == 0);
    REQUIRE(noded.branches.size() == 3);
    REQUIRE(noded.intersections.size() == 1);
    CHECK(noded.offsetIntersections == 1);
    const LC_ParametricIntersection& x = noded.intersections.front();
    CHECK(x.branchA == 0);
    CHECK(x.branchB == 2);
    CHECK(x.point.distanceTo(RS_Vector{0.0, 1.25}) < options.tolerance.requestedGeometry);
    CHECK(x.parameterA == Approx((2.0 - std::sqrt(3.0) / 2.0) / 4.0).margin(1e-9));
    CHECK(x.parameterB == Approx((2.0 + std::sqrt(3.0) / 2.0) / 4.0).margin(1e-9));
    // each occurrence is where two of its branch's pieces meet
    for (const auto& [branch, t] : {std::pair{x.branchA, x.parameterA}, std::pair{x.branchB, x.parameterB}}) {
        const std::vector<LC_OffsetCubicPiece>& pieces = noded.branches[branch].cubicPieces;
        size_t ends = 0;
        for (size_t k = 0; k + 1 < pieces.size(); ++k) {
            if (pieces[k].provenance.sourceT1 == t) {
                CHECK(pieces[k].bezier[3] == x.point);
                CHECK(pieces[k + 1].bezier[0] == x.point);
                ++ends;
            }
        }
        CHECK(ends == 1);
    }

    // The entities pass the check that their fits add no crossing: four piece ends meet at the node.
    const LC_CurveOffsetMaterializationResult entities = LC_CurveOffset::materializeBranches(
        parabola, noded, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(entities.status == LC_CurveOffsetStatus::Ok);
    CHECK(endsAt(entities.entities, x.point) == 4);
}

TEST_CASE("A noded offset of a crossing source keeps every crossing", "[curve-offset][topology]") {
    // Offsets of a loop on either side cross themselves near the source's crossing.
    const RS_Spline loop = makeSpline(3, {{0, 0}, {15, 10}, {-5, 10}, {10, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(loop, 0.2);
        options.nodeIntersections = true;
        const LC_CurveOffsetMaterializationResult result = LC_CurveOffset::createEntities(
            loop, LC_CurveOffset::makeSideRequest(side, 0.2), options, LC_CurveOffset::makeDirectSourceBudget());
        const LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
            loop, LC_CurveOffset::makeSideRequest(side, 0.2), options, LC_CurveOffset::makeDirectSourceBudget());
        INFO("side " << static_cast<int>(side));
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
        CHECK(geometry.intersections.size() >= 1);
        CHECK(geometry.sourceIntersections == 1);
        for (const LC_ParametricIntersection& x : geometry.intersections) {
            CHECK(endsAt(result.entities, x.point) == 4);
        }
    }
}

TEST_CASE("A closed source's cusped offset is noded across its branch joins", "[curve-offset][topology]") {
    RS_Spline ring(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        ring.addControlPoint(p);
    }
    ring.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(ring.getParameterDomain(t0, t1));
    double tightest = RS_MAXDOUBLE;
    for (int i = 0; i < 2000; ++i) {
        LC_CurveJet j;
        REQUIRE(ring.tryEvaluateJet(t0 + (t1 - t0) * i / 2000.0, LC_CurveEvaluationSide::Interior, j));
        const double s = j.first.magnitude();
        tightest = std::min(tightest, s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x));
    }
    const double d = 1.5 * tightest;
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(ring, d);
    options.nodeIntersections = true;
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, d);
    const LC_CurveOffsetGeometryResult geometry =
        LC_CurveOffset::buildDirectBranches(ring, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() >= 2);
    const LC_CurveOffsetMaterializationResult entities =
        LC_CurveOffset::materializeBranches(ring, geometry, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(entities.status == LC_CurveOffsetStatus::Ok);
    for (const LC_ParametricIntersection& x : geometry.intersections) {
        CHECK(endsAt(entities.entities, x.point) == 4);
    }
    // the swallowtail between each pair of cusps crosses the rest of the offset
    CHECK(geometry.intersections.size() >= geometry.branches.size() / 2);
}

TEST_CASE("provider analytic scans are covered by query debit",
          "[lc-offsetcurve][geometry][cache-budget-accounting][strict-budget]") {
    using Shape = StrictBudgetCurves::Shape;
    auto options = strictOptions();
    options.analyticOccurrenceIdentity = true;
    options.maxDepth = 0;
    // Three roots + three leaves; three candidate records; three solve cells/certificates/events;
    // three order-scratch records; three identity records = 24 items. BVH visits four distinct pairs.
    // Three solves + three order attempts + three identity callbacks give certificate=9, total=16.
    // BVH nodes of sizes 3/2 use (8+1+3+2)/(4+1+1+2) scan/axis/heap/index predicates = 22.
    // Candidate/event canonicalization and pair/event heapsorts add 3 each: 34 total.
    options.maxGeneratedItems = 24; options.maxBoxPairs = 16; options.maxOrderingComparisons = 34;
    SECTION("wide analytic isolations share true parameter occurrences without duplicate helper debits") {
        auto probe = std::make_shared<StrictBudgetProbe>();
        const auto result = findIntersections(StrictBudgetCurves{Shape::Triple, probe}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        REQUIRE(result.intersections.size() == 3);
        CHECK(result.work.generatedItems == 24);
        CHECK(result.work.candidateBoxPairs == 4);
        CHECK(result.work.refinementBoxPairs == 3);
        CHECK(result.work.certificateBoxPairs == 9);
        CHECK(result.work.totalBoxPairs == 16);
        CHECK(result.work.orderingComparisons == 34);
        CHECK(probe->identities == 3);
        CHECK(probe->certificates == 3 + 2 * 3);
        for (const auto& event : result.intersections) {
            CHECK(event.isolation.parameterA.contains(0.5));
            CHECK(event.isolation.parameterB.contains(0.5));
            CHECK(event.isolation.parameterA.width() > 0);
            CHECK(event.isolation.parameterB.width() > 0);
            CHECK(event.point.x >= 0.5); CHECK(event.point.x <= 0.75); // qualified seed, not an exact root coordinate
            CHECK(event.point.y == Approx(0));
        }
        CHECK(result.intersections[0].branchA == 0); CHECK(result.intersections[0].branchB == 1);
        CHECK(result.intersections[1].branchA == 0); CHECK(result.intersections[1].branchB == 2);
        CHECK(result.intersections[2].branchA == 1); CHECK(result.intersections[2].branchB == 2);
        checkStrictWork(result, options);
    }
    SECTION("direct and identity pair/item preflight refuses before callback execution") {
        for (const bool identity : {false, true}) {
            for (const bool itemShort : {false, true}) {
                CAPTURE(identity, itemShort);
                auto limited = options;
                // Before first direct certificate: 6 preparation + 3 candidates + 1 solve cell;
                // before first identity: 18 solved records + 3 scratch, and its order-attempt pair.
                if (itemShort) { limited.maxGeneratedItems = identity ? 21 : 10; }
                else { limited.maxBoxPairs = identity ? 11 : 5; }
                auto probe = std::make_shared<StrictBudgetProbe>();
                const auto result = findIntersections(StrictBudgetCurves{Shape::Triple, probe}, limited);
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
                CHECK(probe->identities == 0);
                CHECK(probe->certificates == (identity ? 3 : 0));
                CHECK(result.work.generatedItems == (identity ? 21 : 10));
                CHECK(result.work.totalBoxPairs == (identity ? 11 : 5));
                CHECK(result.work.certificateBoxPairs == (identity ? 4 : 0));
                checkStrictWork(result, limited);
            }
        }
    }
    SECTION("unavailable certificates are admitted once including provider-internal helpers") {
        for (const bool identity : {false, true}) {
            auto probe = std::make_shared<StrictBudgetProbe>();
            probe->unavailable = !identity; probe->identityUnavailable = identity;
            const auto result = findIntersections(StrictBudgetCurves{Shape::Triple, probe}, options);
            CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
            CHECK(probe->identities == (identity ? 1 : 0));
            CHECK(probe->certificates == (identity ? 5 : 1));
            CHECK(result.work.generatedItems == (identity ? 22 : 11));
            CHECK(result.work.totalBoxPairs == (identity ? 12 : 6));
            CHECK(result.work.certificateBoxPairs == (identity ? 5 : 1));
            checkStrictWork(result, options);
        }
    }
    SECTION("throwing direct certificates report their pre-callback debit on factory exit") {
        for (const bool identity : {false, true}) {
            auto probe = std::make_shared<StrictBudgetProbe>();
            probe->exception = std::make_exception_ptr(std::runtime_error("analytic callback failure"));
            probe->throwAt = identity ? StrictBudgetProbe::ThrowAt::Identity : StrictBudgetProbe::ThrowAt::Certificate;
            LC_IntersectionWorkStats exit;
            auto retainedOptions = options; retainedOptions.maxGeneratedItems = 29; // 24 ordinary + session/issuer + 3 roots
            try {
                findCertifiedIntersections(std::make_unique<StrictBudgetCurves>(Shape::Triple, probe), retainedOptions, &exit);
                FAIL("certificate did not throw");
            } catch (...) { CHECK(std::current_exception() == probe->exception); }
            CHECK(probe->certificates == (identity ? 3 : 1)); CHECK(probe->identities == (identity ? 1 : 0));
            CHECK(exit.generatedItems == (identity ? 27 : 13));
            CHECK(exit.candidateBoxPairs == 4); CHECK(exit.refinementBoxPairs == (identity ? 3 : 1));
            CHECK(exit.certificateBoxPairs == (identity ? 5 : 1)); CHECK(exit.totalBoxPairs == (identity ? 12 : 6));
            CHECK(exit.generatedLeaves == 3);
        }
    }
}

TEST_CASE("strict-core seed and split admissions precede queue mutation",
          "[lc-offsetcurve][geometry][cache-budget-accounting][strict-budget]") {
    using Shape = StrictBudgetCurves::Shape;
    SECTION("preparation children are indivisible and unentered children do not raise depth") {
        for (const std::size_t cap : {3u, 4u, 6u}) {
            CAPTURE(cap);
            auto options = strictOptions(); options.maxGeneratedItems = cap; options.maxLeafDepth = 1;
            auto probe = std::make_shared<StrictBudgetProbe>();
            const auto result = findIntersections(StrictBudgetCurves{Shape::LeafSplit, probe, 1}, options);
            // Root + unavailable leaf certificate = 2; split adds 2; accepted leaves add 1 each.
            CHECK(result.status == (cap == 6 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
            CHECK(result.work.generatedItems == (cap == 3 ? 2 : cap));
            CHECK(result.work.generatedLeaves == (cap == 6 ? 2 : 0));
            CHECK(result.work.deepestPreparation == (cap == 3 ? 0 : 1));
            CHECK(probe->bounds == (cap == 3 ? 4 : cap == 4 ? 5 : 6));
            CHECK(probe->leaves == 1); CHECK(result.work.certificateBoxPairs == 1);
            checkStrictWork(result, options);
        }
    }
    SECTION("remaining mandatory seeds cannot be spent on an earlier segment split") {
        auto options = strictOptions(); options.maxGeneratedItems = 4;
        auto probe = std::make_shared<StrictBudgetProbe>();
        const auto result = findIntersections(StrictBudgetCurves{Shape::LeafSplit, probe, 2}, options);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
        CHECK(result.work.generatedItems == 2); // root + callback, with one untouched mandatory seed reserved
        CHECK(result.work.deepestPreparation == 0); CHECK(probe->bounds == 4);
        CHECK(result.work.validationSegmentVisits == 2); CHECK(probe->evaluations == 3);
    }
    SECTION("pair cells charge roots and atomic children before queue insertion") {
        for (const std::size_t cap : {8u, 9u}) {
            CAPTURE(cap);
            auto options = strictOptions(); options.maxGeneratedItems = cap; options.maxDepth = 1;
            auto probe = std::make_shared<StrictBudgetProbe>();
            const auto result = findIntersections(StrictBudgetCurves{Shape::PairSplit, probe}, options);
            CHECK(result.status == LC_IntersectionStatus::LimitExceeded);
            // Four preparation records + candidate + solve root + unavailable certificate = 7.
            CHECK(result.work.generatedItems == (cap == 8 ? 7 : 9));
            CHECK(result.work.deepestPairRefinement == (cap == 8 ? 0 : 1));
            CHECK(result.work.refinementBoxPairs == (cap == 8 ? 1 : 2));
            CHECK(result.work.certificateBoxPairs == 1); CHECK(probe->certificates == 1);
            CHECK(result.work.totalBoxPairs == (cap == 8 ? 3 : 4));
            checkStrictWork(result, options);
        }
    }
}

TEST_CASE("strict-core validation, pair work, ordering comparisons and output scratch are bounded",
          "[lc-offsetcurve][geometry][cache-budget-accounting][strict-budget]") {
    using Shape = StrictBudgetCurves::Shape;
    SECTION("two straight pieces have separate root and accepted-leaf records") {
        for (const bool retained : {false, true}) {
            for (const bool shortCap : {false, true}) {
                CAPTURE(retained, shortCap);
                auto options = strictOptions(); options.maxGeneratedItems = (retained ? 6 : 4) - shortCap;
                options.maxBoxPairs = 1; options.maxOrderingComparisons = 10;
                auto probe = std::make_shared<StrictBudgetProbe>();
                LC_IntersectionWorkStats work;
                LC_IntersectionStatus status;
                if (retained) {
                    const auto result = findCertifiedIntersections(
                        std::make_unique<StrictBudgetCurves>(Shape::Straight, probe), options, &work);
                    status = result.status; checkSameStrictWork(result.work, work);
                    CHECK(bool(result.session) == !shortCap);
                    if (result.session) { CHECK(result.session->rootCount() == 0); }
                } else {
                    const auto result = findIntersections(StrictBudgetCurves{Shape::Straight, probe}, options);
                    status = result.status; work = result.work; CHECK(result.intersections.empty());
                }
                CHECK(status == (shortCap ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                CHECK(work.generatedItems == options.maxGeneratedItems);
                CHECK(work.generatedLeaves == (shortCap ? 1 : 2));
                CHECK(work.validationSegmentVisits == 2); CHECK(work.validationNextCalls == 1);
                CHECK(probe->segments == 1); CHECK(probe->branches == 1); CHECK(probe->next == 1);
                CHECK(probe->evaluations == 6); CHECK(probe->bounds == 8);
                CHECK(probe->leaves == 0); CHECK(probe->certificates == 0);
                CHECK(work.totalBoxPairs == (shortCap ? 0 : 1));
                CHECK(work.orderingComparisons == (shortCap ? 0 : 10));
            }
        }
    }
    SECTION("root-cap refusal precedes scans and retained zero/one precedes all accessors") {
        for (const std::size_t cap : {0u, 1u, 3u}) {
            auto options = strictOptions(); options.maxGeneratedItems = cap;
            auto probe = std::make_shared<StrictBudgetProbe>();
            LC_IntersectionWorkStats work; work.generatedItems = 999;
            const auto result = findCertifiedIntersections(
                std::make_unique<StrictBudgetCurves>(Shape::Straight, probe), options, &work);
            CHECK(result.status == (cap == 0 ? LC_IntersectionStatus::InvalidInput : LC_IntersectionStatus::LimitExceeded));
            CHECK_FALSE(result.session); checkSameStrictWork(work, result.work);
            CHECK(work.generatedItems == (cap < 2 ? 0 : 2));
            CHECK(work.validationSegmentVisits == 0); CHECK(work.validationNextCalls == 0);
            CHECK(probe->segments == (cap < 2 ? 0 : 1)); CHECK(probe->branches == (cap < 2 ? 0 : 1));
            CHECK(probe->next == 0); CHECK(probe->evaluations == 0); CHECK(probe->bounds == 0);
        }
        auto options = strictOptions(); options.maxGeneratedItems = 1;
        auto probe = std::make_shared<StrictBudgetProbe>();
        const auto result = findIntersections(StrictBudgetCurves{Shape::Straight, probe}, options);
        CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK(result.work.generatedItems == 0);
        CHECK(result.work.validationSegmentVisits == 0); CHECK(probe->next == 0); CHECK(probe->bounds == 0);
    }
    SECTION("malformed and oversized metadata are distinct from exhausted geometry work") {
        for (const bool retained : {false, true}) {
            for (const bool oversized : {false, true}) {
                auto probe = std::make_shared<StrictBudgetProbe>();
                probe->malformed = !oversized; probe->oversizedBranches = oversized;
                auto options = strictOptions();
                LC_IntersectionWorkStats work; LC_IntersectionStatus status;
                if (retained) {
                    const auto result = findCertifiedIntersections(
                        std::make_unique<StrictBudgetCurves>(Shape::Straight, probe), options, &work);
                    status = result.status; CHECK_FALSE(result.session); checkSameStrictWork(result.work, work);
                } else {
                    const auto result = findIntersections(StrictBudgetCurves{Shape::Straight, probe}, options);
                    status = result.status; work = result.work;
                }
                CHECK(status == (oversized ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::InvalidInput));
                CHECK(work.generatedItems == (retained ? 2 : 0));
                CHECK(work.validationSegmentVisits == (oversized ? 0 : 2));
                CHECK(work.validationNextCalls == (oversized ? 0 : 1));
                CHECK(probe->bounds == 0); CHECK(probe->evaluations == 0);
            }
        }
    }
    SECTION("pair and ordering zero exact and one-short caps are independent") {
        for (const std::size_t ordering : {0u, 9u, 10u}) {
            for (const std::size_t pairs : {0u, 1u}) {
                auto options = strictOptions(); options.maxOrderingComparisons = ordering; options.maxBoxPairs = pairs;
                auto probe = std::make_shared<StrictBudgetProbe>();
                const auto result = findIntersections(StrictBudgetCurves{Shape::Straight, probe}, options);
                CHECK(result.status == (ordering == 10 && pairs == 1 ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
                // Four axis reductions + axis choice + heap predicate + two index reductions precede the pair;
                // two parameter-range reductions follow its admission.
                CHECK(result.work.orderingComparisons == (pairs == 0 ? std::min(ordering, std::size_t{8}) : ordering));
                CHECK(result.work.totalBoxPairs == (ordering == 0 ? 0 : pairs));
                CHECK(result.work.generatedItems == 4); CHECK(probe->evaluations == 6); CHECK(probe->bounds == 8);
                checkStrictWork(result, options);
            }
        }
        auto options = strictOptions(); options.maxOrderingComparisons = 0;
        auto probe = std::make_shared<StrictBudgetProbe>();
        const auto singleton = findIntersections(StrictBudgetCurves{Shape::Straight, probe, 1}, options);
        CHECK(singleton.status == LC_IntersectionStatus::Ok); CHECK(singleton.work.orderingComparisons == 0);
        options.strict = false; options.maxGeneratedItems = 0;
        const auto ordinary = findIntersections(StrictBudgetCurves{Shape::Straight, probe}, options);
        CHECK(ordinary.status == LC_IntersectionStatus::Ok);
        CHECK(ordinary.work.orderingComparisons == 0); CHECK(ordinary.work.validationSegmentVisits == 0);
        CHECK(ordinary.work.validationNextCalls == 0);
    }
    SECTION("order scratch is reserved and admitted as a complete event-sized batch") {
        for (const bool retained : {false, true}) {
            auto probe = std::make_shared<StrictBudgetProbe>();
            auto options = strictOptions(); options.analyticOccurrenceIdentity = true;
            // Before order: 6 roots/leaves + 3 candidates + 9 solve/certificate/event records.
            // Retention additionally owns 2 metadata and 3 root records. Three scratch tokens are required together.
            options.maxGeneratedItems = retained ? 25 : 20;
            LC_IntersectionWorkStats work;
            if (retained) {
                const auto result = findCertifiedIntersections(
                    std::make_unique<StrictBudgetCurves>(Shape::Triple, probe), options, &work);
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.session);
                checkSameStrictWork(result.work, work);
            } else {
                const auto result = findIntersections(StrictBudgetCurves{Shape::Triple, probe}, options);
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK(result.intersections.empty());
                work = result.work;
            }
            CHECK(work.generatedItems == (retained ? 23 : 18));
            CHECK(work.totalBoxPairs == 10); CHECK(work.certificateBoxPairs == 3);
            CHECK(probe->certificates == 3); CHECK(probe->identities == 0);
        }
    }
    SECTION("final output heapsort refuses one-short without publishing a partial result or owner") {
        for (const bool retained : {false, true}) {
            for (const bool oneShort : {false, true}) {
                auto options = strictOptions(); options.analyticOccurrenceIdentity = true;
                options.maxGeneratedItems = retained ? 29 : 24; options.maxBoxPairs = 16;
                options.maxOrderingComparisons = oneShort ? 33 : 34;
                auto probe = std::make_shared<StrictBudgetProbe>();
                LC_IntersectionWorkStats work;
                if (retained) {
                    const auto result = findCertifiedIntersections(
                        std::make_unique<StrictBudgetCurves>(Shape::Triple, probe), options, &work);
                    CHECK(result.status == (oneShort ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                    CHECK(bool(result.session) == !oneShort); checkSameStrictWork(work, result.work);
                    if (result.session) {
                        REQUIRE(result.session->rootCount() == 3);
                        for (std::size_t i = 0; i < 3; ++i) {
                            const auto event = result.session->snapshot(result.session->rootHandle(i))->event;
                            CHECK(event.branchA == (i == 2 ? 1 : 0)); CHECK(event.branchB == (i == 0 ? 1 : 2));
                        }
                    }
                } else {
                    const auto result = findIntersections(StrictBudgetCurves{Shape::Triple, probe}, options);
                    CHECK(result.status == (oneShort ? LC_IntersectionStatus::LimitExceeded : LC_IntersectionStatus::Ok));
                    CHECK(result.intersections.size() == (oneShort ? 0 : 3)); work = result.work;
                }
                CHECK(work.generatedItems == options.maxGeneratedItems);
                CHECK(work.orderingComparisons == options.maxOrderingComparisons);
                CHECK(work.totalBoxPairs == 16); CHECK(probe->identities == 3); CHECK(probe->certificates == 9);
            }
        }
    }
    SECTION("oversized segment inventories refuse before validation in either entry point") {
        for (const bool retained : {false, true}) {
            auto options = strictOptions(); options.maxGeneratedItems = retained ? 4 : 2;
            auto probe = std::make_shared<StrictBudgetProbe>();
            LC_IntersectionWorkStats work;
            if (retained) {
                const auto result = findCertifiedIntersections(
                    std::make_unique<StrictBudgetCurves>(Shape::Straight, probe, 3), options, &work);
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded); CHECK_FALSE(result.session);
            } else {
                const auto result = findIntersections(StrictBudgetCurves{Shape::Straight, probe, 3}, options);
                CHECK(result.status == LC_IntersectionStatus::LimitExceeded); work = result.work;
            }
            CHECK(work.generatedItems == (retained ? 2 : 0)); CHECK(work.validationSegmentVisits == 0);
            CHECK(work.validationNextCalls == 0); CHECK(probe->bounds == 0); CHECK(probe->evaluations == 0);
        }
    }
    SECTION("appended fields preserve legacy aggregates and unconstrained caps do not size scratch") {
        static_assert(std::is_aggregate_v<LC_IntersectionOptions>);
        static_assert(std::is_aggregate_v<LC_IntersectionWorkStats>);
        const LC_IntersectionOptions legacy{1e-9, 3, 4, 5, 6, 7, true, 8, true};
        CHECK(legacy.maxDepth == 3); CHECK(legacy.maxLeafDepth == 4); CHECK(legacy.maxLeaves == 5);
        CHECK(legacy.maxBoxPairs == 6); CHECK(legacy.anchoredBranches == 7);
        CHECK(legacy.strict); CHECK(legacy.maxGeneratedItems == 8); CHECK(legacy.analyticOccurrenceIdentity);
        CHECK(legacy.maxOrderingComparisons == (std::size_t{1} << 20));
        const LC_IntersectionWorkStats old{1, 2, 3, 6, 7, 8, 9, 10, 11, 12};
        CHECK(old.deepestRootRefinement == 12); CHECK(old.orderingComparisons == 0);
        CHECK(old.validationSegmentVisits == 0); CHECK(old.validationNextCalls == 0);
        const LC_CertifiedContractionAllowance oldAllowance{3, 4, 5};
        CHECK(oldAllowance.remainingPairs == 3); CHECK(oldAllowance.remainingItems == 4);
        CHECK(oldAllowance.absoluteRootDepth == 5);
        CHECK(oldAllowance.remainingOrderingComparisons == std::numeric_limits<std::size_t>::max());
        auto options = strictOptions();
        options.maxGeneratedItems = options.maxLeaves = options.maxBoxPairs =
            options.maxOrderingComparisons = std::numeric_limits<std::size_t>::max();
        auto probe = std::make_shared<StrictBudgetProbe>();
        const auto result = findCertifiedIntersections(
            std::make_unique<StrictBudgetCurves>(Shape::Straight, probe), options);
        CHECK(result.status == LC_IntersectionStatus::Ok); CHECK(result.work.generatedItems == 6);
        CHECK(result.work.orderingComparisons == 10); CHECK(result.work.totalBoxPairs == 1);
    }
    SECTION("retained ordering honors both current shared allowance and original cumulative ceiling") {
        const auto measured = findCertifiedIntersections(endpointRefineCurves(), strictOptions());
        REQUIRE(measured.status == LC_IntersectionStatus::Ok);
        for (const bool cumulativeCeiling : {false, true}) {
            for (const std::size_t extra : {0u, 1u}) {
                CAPTURE(cumulativeCeiling, extra);
                auto options = strictOptions();
                options.maxOrderingComparisons = measured.work.orderingComparisons + (cumulativeCeiling ? extra : 3);
                auto query = findCertifiedIntersections(endpointRefineCurves(), options);
                REQUIRE(query.status == LC_IntersectionStatus::Ok);
                const auto handle = query.session->rootHandle(0);
                const auto original = *query.session->snapshot(handle);
                auto allowance = refineAllowance();
                allowance.remainingOrderingComparisons = cumulativeCeiling ? 100 : extra;
                const auto step = query.session->contract(handle, allowance);
                CHECK(step.status == (extra ? LC_IntersectionStatus::Ok : LC_IntersectionStatus::LimitExceeded));
                CHECK(bool(step.snapshot) == bool(extra));
                CHECK(step.work.orderingComparisons == extra);
                // The contraction and staged event precede canonicalization; refusal cannot refund either.
                CHECK(step.work.totalBoxPairs == 1); CHECK(step.work.generatedItems == 1);
                CHECK(step.work.deepestRootRefinement == 1);
                CHECK(query.session->cumulativeWork().orderingComparisons == measured.work.orderingComparisons + extra);
                if (extra) {
                    CHECK(step.snapshot->event.isolation.parameterA.width() < original.event.isolation.parameterA.width());
                    allowance.remainingOrderingComparisons = cumulativeCeiling ? 100 : 0;
                    const auto next = query.session->contract(handle, allowance);
                    CHECK(next.status == LC_IntersectionStatus::LimitExceeded);
                    CHECK(next.work.orderingComparisons == 0); CHECK_FALSE(next.snapshot);
                } else {
                    checkSameDomain(query.session->snapshot(handle)->event.isolation, original.event.isolation);
                }
                const auto stopped = query.session->cumulativeWork();
                const auto retry = query.session->contract(handle, refineAllowance());
                CHECK(retry.status == LC_IntersectionStatus::LimitExceeded);
                checkSameStrictWork(retry.work, {});
                checkSameStrictWork(query.session->cumulativeWork(), stopped);
            }
        }
    }
    SECTION("factory exit snapshots preserve original setup and provider exceptions") {
        using At = StrictBudgetProbe::ThrowAt;
        for (const auto at : {At::Segments, At::Branches, At::Next, At::Bound, At::Evaluate, At::Leaf}) {
            auto probe = std::make_shared<StrictBudgetProbe>();
            probe->throwAt = at; probe->exception = std::make_exception_ptr(std::runtime_error("provider failure"));
            LC_IntersectionWorkStats exit;
            try {
                findCertifiedIntersections(std::make_unique<StrictBudgetCurves>(
                    at == At::Leaf ? Shape::LeafSplit : Shape::Straight, probe), strictOptions(), &exit);
                FAIL("provider did not throw");
            } catch (...) { CHECK(std::current_exception() == probe->exception); }
            CHECK(exit.generatedItems == (at == At::Leaf ? 4 : at == At::Bound || at == At::Evaluate ? 3 : 2));
            CHECK(exit.certificateBoxPairs == (at == At::Leaf ? 1 : 0));
            CHECK(exit.totalBoxPairs == exit.certificateBoxPairs); CHECK(exit.generatedLeaves == 0);
        }
        LC_IntersectionWorkStats exit; exit.generatedItems = 999;
        const auto invalid = findCertifiedIntersections(nullptr, strictOptions(), &exit);
        CHECK(invalid.status == LC_IntersectionStatus::InvalidInput); checkSameStrictWork(exit, {});
    }
}

TEST_CASE("retained certificate callback throw preserves admitted work",
          "[lc-offsetcurve][geometry][intersection-refine]") {
    auto probe = std::make_shared<RefineProbe>();
    auto provider = singularRefineCurves(false, probe); // A(t)=(t,0), B(u)=(u,(u-.5)^2)
    for (const std::size_t segment : {0u, 1u}) {
        RS_Vector p, d;
        REQUIRE(provider->evaluate(segment, 0.5, p, d));
        CHECK(p.x == 0.5); CHECK(p.y == 0); CHECK(d.x == 1); CHECK(d.y == 0);
    }
    LC_IntersectionWorkStats exit;
    auto query = findCertifiedIntersections(std::move(provider), strictOptions(), &exit);
    REQUIRE(query.status == LC_IntersectionStatus::Ok); REQUIRE(query.session);
    REQUIRE(query.session->rootCount() == 1); checkSameStrictWork(exit, query.work);
    CHECK(query.session->hasValidProofs());
    const auto handle = query.session->rootHandle(0);
    REQUIRE(query.session->snapshot(handle));
    const auto before = query.session->cumulativeWork();
    const auto callbacks = probe->certificates;
    probe->certificateException = std::make_exception_ptr(std::runtime_error("retained analytic failure"));
    LC_IntersectionWorkStats callExit;
    try {
        query.session->contract(handle, refineAllowance(), &callExit);
        FAIL("retained callback did not throw");
    } catch (...) { CHECK(std::current_exception() == probe->certificateException); }
    const auto after = query.session->cumulativeWork();
    CHECK(probe->certificates == callbacks + 1);
    CHECK(after.certificateBoxPairs == before.certificateBoxPairs + 2);
    CHECK(after.totalBoxPairs == before.totalBoxPairs + 2);
    CHECK(after.generatedItems == before.generatedItems + 1);
    CHECK(after.deepestRootRefinement == before.deepestRootRefinement + 1);
    CHECK(callExit.totalBoxPairs == 2); CHECK(callExit.certificateBoxPairs == 2);
    CHECK(callExit.generatedItems == 1);
    CHECK(callExit.deepestRootRefinement == after.deepestRootRefinement);
    CHECK(query.session->rootCount() == 0); CHECK_FALSE(query.session->snapshot(handle));
    CHECK_FALSE(query.session->hasValidProofs());
    const auto bounds = probe->bounds, evaluations = probe->evaluations;
    const auto retry = query.session->contract(handle, refineAllowance());
    CHECK(retry.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(retry.snapshot);
    checkSameStrictWork(retry.work, {}); checkSameStrictWork(query.session->cumulativeWork(), after);
    CHECK(probe->bounds == bounds); CHECK(probe->evaluations == evaluations); CHECK(probe->certificates == callbacks + 1);
}

TEST_CASE("Strict proof-domain padding is bounded and opt in", "[lc-offsetcurve][geometry][strict-proof-domain]") {
    LC_IntersectionOptions options; options.strict = true; options.tolerance = 1e-6;
    CHECK(options.strictProofDomainPadding == 0.001);
    const AnalyticLines lines{{1, 0}, {0, 1}};
    for (const double padding : {0.0, 0.001, 1.0 / 16.0, 0.25}) {
        CAPTURE(padding); options.strictProofDomainPadding = padding;
        const auto result = findIntersections(lines, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok); REQUIRE(result.intersections.size() == 1);
        const auto& root = result.intersections[0];
        CHECK(root.proof == LC_IntersectionProof::IntervalUniqueTransverse);
        CHECK(root.isolation.parameterA.contains(.5)); CHECK(root.isolation.parameterB.contains(.5));
        LC_IntersectionWorkStats entered;
        const auto owned = findCertifiedIntersections(std::make_unique<AnalyticLines>(RS_Vector{1, 0}, RS_Vector{0, 1}), options, &entered);
        REQUIRE(owned.status == LC_IntersectionStatus::Ok); REQUIRE(owned.session); CHECK(owned.session->rootCount() == 1);
        checkSameStrictWork(entered, owned.work);
    }
    for (const double padding : {-0.001, std::nextafter(.25, 1.0),
                                 std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        CAPTURE(padding); options.strictProofDomainPadding = padding;
        const auto result = findIntersections(lines, options);
        CHECK(result.status == LC_IntersectionStatus::InvalidInput); CHECK(result.intersections.empty());
        checkSameStrictWork(result.work, {});
        LC_IntersectionWorkStats entered;
        const auto owned = findCertifiedIntersections(std::make_unique<AnalyticLines>(RS_Vector{1, 0}, RS_Vector{0, 1}), options, &entered);
        CHECK(owned.status == LC_IntersectionStatus::InvalidInput); CHECK_FALSE(owned.session);
        checkSameStrictWork(entered, {}); checkSameStrictWork(owned.work, {});
    }
    options.strict = false;
    const auto ordinary = findIntersections(lines, options);
    CHECK(ordinary.status == LC_IntersectionStatus::Ok); CHECK(ordinary.intersections.size() == 1);
}
