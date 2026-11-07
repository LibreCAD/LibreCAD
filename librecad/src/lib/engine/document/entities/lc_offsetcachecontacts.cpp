/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include "lc_offsetcachecontacts.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace LC_EllipseOffsetGeometry {
namespace {

bool finite(const LC_Interval& value) {
    return value.isValid() && std::isfinite(value.lo()) && std::isfinite(value.hi());
}

bool samePoint(const RS_Vector& a, const RS_Vector& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

double middle(const LC_Interval& value) {
    return 0.5 * value.lo() + 0.5 * value.hi();
}

bool adoptCoreWork(WorkBudget& budget, const LC_IntersectionWorkStats& before,
                   const LC_IntersectionWorkStats& after) {
    if (after.generatedItems < before.generatedItems || after.totalBoxPairs < before.totalBoxPairs ||
        after.orderingComparisons < before.orderingComparisons || after.deepest() < before.deepest()) {
        budget.latchInvalidInput();
        return false;
    }
    return budget.adoptWork({0, after.generatedItems - before.generatedItems,
        after.totalBoxPairs - before.totalBoxPairs, after.deepest(),
        after.orderingComparisons - before.orderingComparisons});
}

LC_IntersectionStatus budgetStatus(const WorkBudget& budget) {
    return budget.status() == BudgetStatus::LimitExceeded ? LC_IntersectionStatus::LimitExceeded
                                                         : LC_IntersectionStatus::InvalidInput;
}

struct ContactSpan {
    CacheSpan raw;
    CacheSpan directed;
    std::size_t rawId = 0;
};

using DerivativeNet = std::array<CacheBezierPoint, 4>;
using Direction = std::array<double, 2>;

Direction direction(const CacheBezierPoint& derivative) {
    const double x = middle(derivative.x), y = middle(derivative.y);
    const double scale = std::hypot(x, y);
    return scale > 0 && std::isfinite(scale) ? Direction{x / scale, y / scale} : Direction{};
}

LC_Interval project(const CacheBezierPoint& point, const Direction& axis) {
    // Preserve known zero coefficients, including stationary endpoints and axis-aligned curves.
    if ((point.x.lo() == 0 && point.x.hi() == 0) && (point.y.lo() == 0 && point.y.hi() == 0)) {
        return LC_Interval::point(0);
    }
    if (axis[0] == 0) { return LC_Interval::point(axis[1]) * point.y; }
    if (axis[1] == 0) { return LC_Interval::point(axis[0]) * point.x; }
    return LC_Interval::point(axis[0]) * point.x + LC_Interval::point(axis[1]) * point.y;
}

bool forward(const DerivativeNet& net, const std::size_t degree, const Direction& axis) {
    if (!std::isfinite(axis[0]) || !std::isfinite(axis[1]) || (axis[0] == 0 && axis[1] == 0)) { return false; }
    bool positive = false;
    for (std::size_t i = 0; i <= degree; ++i) {
        const auto coefficient = project(net[i], axis);
        if (!finite(coefficient) || coefficient.lo() < 0) { return false; }
        positive = positive || coefficient.lo() > 0;
    }
    // A nonzero nonnegative Bernstein polynomial is positive on the open interval.
    return positive;
}

/** Fixed candidates only seed axes; every coefficient of BOTH paths must prove the same direction. */
bool commonProjection(const DerivativeNet& a, const std::size_t degreeA,
                      const DerivativeNet& b, const std::size_t degreeB) {
    const auto endA = direction(a[degreeA]), startB = direction(b[0]);
    const std::array<Direction, 11> axes{{{1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}, endA, startB,
        {endA[0] + startB[0], endA[1] + startB[1]}}};
    for (const auto& axis : axes) {
        if (forward(a, degreeA, axis) && forward(b, degreeB, axis)) { return true; }
    }
    return false;
}

class CacheContactProvider final : public LC_ParametricCurves {
public:
    CacheContactProvider(const ReferenceFrame& frame, const bool reversed, const bool closed)
        : m_frame{frame}, m_reversed{reversed}, m_closed{closed} {}

    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return 1; }
    std::ptrdiff_t next(const std::size_t branch) const override { return branch == 0 && m_closed ? 0 : -1; }

    bool contains(const std::size_t segment, const LC_Interval& parameter) const {
        return segment < m_segments.size() && finite(parameter) &&
            m_segments[segment].t0 <= parameter.lo() && parameter.hi() <= m_segments[segment].t1;
    }

    bool derivativeNet(const std::size_t segment, const double a, const double b,
                       DerivativeNet& net, std::size_t& degree) const {
        return contains(segment, LC_Interval::hull(a, b)) && a <= b &&
            boundCacheDerivativeControls(m_spans[segment].directed, m_frame,
                a - m_segments[segment].t0, b - m_segments[segment].t0, net, degree);
    }

    bool bound(const std::size_t segment, const double a, const double b,
               LC_Interval& x, LC_Interval& y, LC_Interval& dx, LC_Interval& dy) const override {
        x = y = dx = dy = {};
        if (!(a <= b) || !contains(segment, LC_Interval::hull(a, b))) { return false; }
        CurveChordBounds chord;
        DerivativeNet derivative;
        std::size_t degree = 0;
        if (!boundCacheChord(m_spans[segment].directed, m_frame, a - m_segments[segment].t0,
                             b - m_segments[segment].t0, chord) ||
            !derivativeNet(segment, a, b, derivative, degree)) { return false; }
        x = chord.x; y = chord.y; dx = derivative[0].x; dy = derivative[0].y;
        for (std::size_t i = 1; i <= degree; ++i) {
            dx = LC_Interval::hull(dx, derivative[i].x);
            dy = LC_Interval::hull(dy, derivative[i].y);
        }
        return finite(x) && finite(y) && finite(dx) && finite(dy);
    }

    bool evaluate(const std::size_t segment, const double t, RS_Vector& point, RS_Vector& derivative) const override {
        point = derivative = RS_Vector{false};
        LC_Interval x, y, dx, dy;
        if (!bound(segment, t, t, x, y, dx, dy)) { return false; }
        point = RS_Vector{middle(x), middle(y)};
        derivative = RS_Vector{middle(dx), middle(dy)};
        return isFinite2D(point) && isFinite2D(derivative);
    }

    LC_AnalyticLeafCertificate certifyInjective(const std::size_t segment,
                                                const LC_Interval& parameter) const override {
        if (!contains(segment, parameter)) { return {}; }
        DerivativeNet net;
        std::size_t degree = 0;
        if (!derivativeNet(segment, parameter.lo(), parameter.hi(), net, degree) ||
            !commonProjection(net, degree, net, degree)) { return {}; }
        return {true, segment, parameter};
    }

    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        if (!contains(box.segmentA, box.parameterA) || !contains(box.segmentB, box.parameterB)) { return {}; }
        auto first = box.segmentA, last = box.segmentB;
        auto a = box.parameterA, b = box.parameterB;
        if (first > last || (first == last && a.lo() > b.lo())) {
            std::swap(first, last); std::swap(a, b);
        }
        bool excluded = false;
        if (first == last && a.hi() <= b.lo()) {
            const auto proof = certifyInjective(first, LC_Interval::hull(a.lo(), b.hi()));
            excluded = proof.available;
        } else if (last - first == 1) {
            excluded = joinProjection(first, a.lo(), last, b.hi());
        }
        if (!excluded && m_closed && first == 0 && last == m_segments.size() - 1) {
            excluded = joinProjection(last, b.lo(), first, a.hi());
        }
        if (!excluded) { return {}; }
        LC_AnalyticIntersectionCertificate out;
        out.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
        out.proof = LC_IntersectionProof::AnalyticExclusion;
        out.proofDomain = box;
        return out;
    }

    bool joinProjection(const std::size_t first, const double from,
                        const std::size_t last, const double to) const {
        // Include both COMPLETE tails to their exact shared endpoint, not each box's chord axis.
        const auto& a = m_spans[first].directed;
        const auto& b = m_spans[last].directed;
        if (!samePoint(a.controls[a.degree], b.controls[0])) { return false; }
        DerivativeNet left, right;
        std::size_t leftDegree = 0, rightDegree = 0;
        return derivativeNet(first, from, m_segments[first].t1, left, leftDegree) &&
            derivativeNet(last, m_segments[last].t0, to, right, rightDegree) &&
            commonProjection(left, leftDegree, right, rightDegree);
    }

    ReferenceFrame m_frame;
    bool m_reversed;
    bool m_closed;
    std::vector<ContactSpan> m_spans;
    std::vector<LC_ParametricSegment> m_segments;
};

} // namespace

struct CacheContactAnalysis::Impl {
    std::unique_ptr<LC_CertifiedIntersectionSession> session;
    const CacheContactProvider* provider = nullptr; // Owned by session.
};

CacheContactAnalysis::CacheContactAnalysis(std::unique_ptr<Impl> impl) : m_impl{std::move(impl)} {}
CacheContactAnalysis::~CacheContactAnalysis() = default;
CacheContactAnalysis::CacheContactAnalysis(CacheContactAnalysis&&) noexcept = default;
CacheContactAnalysis& CacheContactAnalysis::operator=(CacheContactAnalysis&&) noexcept = default;
const LC_ParametricCurves& CacheContactAnalysis::curves() const noexcept { return *m_impl->provider; }
std::size_t CacheContactAnalysis::spanCount() const noexcept { return m_impl->provider->m_spans.size(); }
std::optional<std::size_t> CacheContactAnalysis::rawSpanId(const std::size_t segment) const noexcept {
    return segment < spanCount() ? std::optional<std::size_t>{m_impl->provider->m_spans[segment].rawId} : std::nullopt;
}
const CacheSpan* CacheContactAnalysis::rawSpan(const std::size_t segment) const noexcept {
    return segment < spanCount() ? &m_impl->provider->m_spans[segment].raw : nullptr;
}
bool CacheContactAnalysis::reversed() const noexcept { return m_impl->provider->m_reversed; }
bool CacheContactAnalysis::closed() const noexcept { return m_impl->provider->m_closed; }
std::size_t CacheContactAnalysis::contactCount() const noexcept { return m_impl->session->rootCount(); }
bool CacheContactAnalysis::hasValidProofs() const noexcept {
    return m_impl && m_impl->session && m_impl->session->hasValidProofs();
}
LC_CertifiedRootHandle CacheContactAnalysis::contactHandle(const std::size_t index) const {
    return m_impl->session->rootHandle(index);
}
std::optional<LC_CertifiedRootSnapshot> CacheContactAnalysis::snapshot(const LC_CertifiedRootHandle& handle) const {
    return m_impl->session->snapshot(handle);
}

LC_CertifiedContractionResult CacheContactAnalysis::contract(const LC_CertifiedRootHandle& handle,
                                                            WorkBudget& budget) {
    LC_CertifiedContractionResult out;
    if (budget.status() != BudgetStatus::Ok) { out.status = budgetStatus(budget); return out; }
    LC_IntersectionWorkStats entered;
    try {
        out = m_impl->session->contract(handle, {budget.remainingPairs(), budget.remainingVertices(),
            budget.depthLimit(), budget.remainingOrderingComparisons()}, &entered);
    } catch (...) {
        adoptCoreWork(budget, {}, entered);
        throw;
    }
    if (!adoptCoreWork(budget, {}, entered) ||
        out.status == LC_IntersectionStatus::LimitExceeded) {
        if (budget.status() == BudgetStatus::Ok) { budget.latchLimitExceeded(); }
        out.status = budgetStatus(budget);
        out.snapshot.reset();
    }
    return out;
}

CacheContactResult analyseCacheContacts(const CacheLayoutResult& layout, const ReferenceFrame& frame,
                                       const bool cacheReversed, const bool closed, WorkBudget& budget) {
    CacheContactResult out;
    const auto finish = [&]() { out.work = budget.consumed(); return std::move(out); };
    if (budget.status() != BudgetStatus::Ok) { out.status = budgetStatus(budget); return finish(); }
    if (layout.status != CacheLayoutStatus::Ok || layout.spans.empty() ||
        !std::isfinite(frame.scale()) || !(frame.scale() > 0)) { return finish(); }
    const auto count = layout.spans.size();
    // One provider, two result-owner objects, and two copied spans plus one segment per input.
    // Preflight counts before allocation, then prepare storage before the atomic record debit.
    if (count > (std::numeric_limits<std::size_t>::max() - 3) / 3 ||
        count > std::vector<ContactSpan>{}.max_size() || count > std::vector<LC_ParametricSegment>{}.max_size() ||
        static_cast<long double>(count) > 0x1p52L || 3 + 3 * count > budget.remainingVertices()) {
        if (budget.status() == BudgetStatus::Ok) { budget.latchLimitExceeded(); }
        out.status = budgetStatus(budget);
        return finish();
    }
    auto provider = std::make_unique<CacheContactProvider>(frame, cacheReversed, closed);
    auto data = std::make_unique<CacheContactAnalysis::Impl>();
    auto* state = data.get();
    std::unique_ptr<CacheContactAnalysis> owner{new CacheContactAnalysis{std::move(data)}};
    provider->m_spans.reserve(count);
    provider->m_segments.reserve(count);
    if (!budget.adoptWork({0, 3 + 3 * count, 0, 0, 0})) { out.status = budgetStatus(budget); return finish(); }
    std::size_t degree = 0;
    for (std::size_t k = 0; k < count; ++k) {
        if (!budget.consumePairs(1)) { out.status = budgetStatus(budget); return finish(); }
        const auto rawId = cacheReversed ? count - 1 - k : k;
        const auto& raw = layout.spans[rawId];
        if (k == 0) { degree = raw.degree; }
        if ((degree != 1 && degree != 3) || raw.degree != degree || !finite(raw.parameter) ||
            !(raw.parameter.lo() < raw.parameter.hi())) { return finish(); }
        for (std::size_t i = 0; i <= degree; ++i) {
            if (!isFinite2D(raw.controls[i])) { return finish(); }
        }
        ContactSpan copied{raw, raw, rawId};
        copied.directed.parameter = LC_Interval::hull(0, 1);
        for (std::size_t i = 0; i <= degree; ++i) {
            copied.directed.controls[i] = raw.controls[cacheReversed ? degree - i : i];
        }
        if (k != 0) {
            const auto& previous = provider->m_spans.back().directed;
            if (!samePoint(previous.controls[degree], copied.directed.controls[0])) { return finish(); }
        }
        provider->m_spans.push_back(std::move(copied));
        provider->m_segments.push_back({0, static_cast<double>(k), static_cast<double>(k + 1)});
    }
    if (closed) {
        if (!budget.consumePairs(1)) { out.status = budgetStatus(budget); return finish(); }
        if (!samePoint(provider->m_spans.back().directed.controls[degree],
                       provider->m_spans.front().directed.controls[0])) { return finish(); }
    }
    LC_IntersectionOptions options;
    options.strict = true;
    options.strictProofDomainPadding = 1.0 / 16.0;
    // Strict decisions are proof-only; this positive seed-policy field is not a distance allowance.
    options.tolerance = 1e-12;
    options.maxDepth = options.maxLeafDepth = budget.depthLimit();
    options.maxBoxPairs = budget.remainingPairs();
    options.maxLeaves = options.maxGeneratedItems = budget.remainingVertices();
    options.maxOrderingComparisons = budget.remainingOrderingComparisons();
    if (options.maxGeneratedItems < 2) {
        budget.latchLimitExceeded(); out.status = budgetStatus(budget); return finish();
    }
    const auto* view = provider.get();
    LC_IntersectionWorkStats entered;
    LC_CertifiedQueryResult query;
    try {
        query = findCertifiedIntersections(std::move(provider), options, &entered);
    } catch (...) {
        adoptCoreWork(budget, {}, entered);
        throw;
    }
    out.queryWork = entered;
    if (!adoptCoreWork(budget, {}, entered)) { out.status = budgetStatus(budget); return finish(); }
    out.status = query.status;
    if (query.status == LC_IntersectionStatus::LimitExceeded) { budget.latchLimitExceeded(); }
    if (query.status != LC_IntersectionStatus::Ok) { return finish(); }
    if (!query.session) { out.status = LC_IntersectionStatus::InvalidInput; return finish(); }
    state->session = std::move(query.session);
    state->provider = view;
    out.analysis = std::move(owner);
    return finish();
}

} // namespace LC_EllipseOffsetGeometry
