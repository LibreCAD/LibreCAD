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
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
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
    options.maxGeneratedItems = 2;
    const auto qualified = findIntersections(curves, options);
    REQUIRE(qualified.status == LC_IntersectionStatus::Ok);
    CHECK(qualified.intersections.empty());
    CHECK(qualified.work.generatedItems == 2);
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
    CHECK(noLeaves.work.generatedItems == 1);
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
        options.maxGeneratedItems = count;
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
        options.maxGeneratedItems = pieces.size();
        options.anchoredBranches = 1;
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        CHECK(result.work.refinementBoxPairs == 0);
        CHECK(result.work.totalBoxPairs < options.maxBoxPairs);
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
        options.maxGeneratedItems = count;
        const auto result = findIntersections(CertifiedPolynomials{pieces}, options);
        REQUIRE(result.status == LC_IntersectionStatus::Ok);
        CHECK(result.intersections.empty());
        CHECK(result.work.generatedLeaves == count);
        CHECK(result.work.generatedItems == count);
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
