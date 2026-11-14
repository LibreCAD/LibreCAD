/*******************************************************************************
 * This file is part of LibreCAD, distributed under GPL version 2 or later.
 * Copyright (C) 2026 LibreCAD.org
 ******************************************************************************/
#include "lc_offsetcachetrim.h"

#include "lc_ellipseoffsetgeometry.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_math.h"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace LC_OffsetCacheTrim {
namespace {
namespace Geo = LC_EllipseOffsetGeometry;
using I = LC_Interval;
using Q = boost::multiprecision::cpp_rational;
using Net = std::array<Geo::CacheBezierPoint, 4>;
using Spans = std::vector<Geo::CacheSpan>;

struct Failure { Refusal reason; };
[[noreturn]] void fail(Refusal reason) { throw Failure{reason}; }
bool finite(const I& a) { return a.isValid() && std::isfinite(a.lo()) && std::isfinite(a.hi()); }
double mid(const I& a) {
    // Halving a positive subnormal singleton can round both terms to zero.
    return a.lo() == a.hi() ? a.lo() : a.lo() * .5 + a.hi() * .5;
}
bool same(const RS_Vector& a, const RS_Vector& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool singleton(const I& a) { return finite(a) && a.lo() == a.hi(); }
bool exactParameter(const I& a, double value) { return singleton(a) && a.lo() == value; }
I intersect(const I& a, const I& b) {
    return finite(a) && finite(b) && std::max(a.lo(), b.lo()) <= std::min(a.hi(), b.hi())
        ? I::hull(std::max(a.lo(), b.lo()), std::min(a.hi(), b.hi())) : I{};
}
I enclose(const Q& q) {
    const double v = q.convert_to<double>();
    if (!std::isfinite(v)) { return {}; }
    const Q rounded{v};
    return I::hull(rounded > q ? std::nextafter(v, -INFINITY) : v,
                   rounded < q ? std::nextafter(v, INFINITY) : v);
}

struct ExactPoint { Q x, y; };
using ExactNet = std::array<ExactPoint, 4>;

Q signedDistance(const ExactPoint& point, const ExactPoint& origin,
                 const Q& dx, const Q& dy) {
    return dx * (point.y - origin.y) - dy * (point.x - origin.x);
}

std::pair<ExactNet, ExactNet> splitHalf(const ExactNet& net, std::size_t degree) {
    ExactNet work = net, left{}, right{};
    left[0] = work[0]; right[degree] = work[degree];
    for (std::size_t level = 1; level <= degree; ++level) {
        for (std::size_t i = 0; i <= degree - level; ++i) {
            work[i] = {(work[i].x + work[i + 1].x) / 2,
                       (work[i].y + work[i + 1].y) / 2};
        }
        left[level] = work[0]; right[degree - level] = work[degree - level];
    }
    return {left, right};
}

std::optional<MonotoneLineRootCertificate> certifyMonotoneLineRootExact(
        const std::vector<RS_Vector>& controls,
        const RS_Vector& lineStart, const RS_Vector& lineEnd) {
    if (controls.size() < 2 || controls.size() > 4 ||
        !std::isfinite(lineStart.x) || !std::isfinite(lineStart.y) ||
        !std::isfinite(lineEnd.x) || !std::isfinite(lineEnd.y)) { return {}; }

    const std::size_t degree = controls.size() - 1;
    const ExactPoint origin{Q{lineStart.x}, Q{lineStart.y}};
    const Q dx = Q{lineEnd.x} - origin.x, dy = Q{lineEnd.y} - origin.y;
    const Q lengthSquared = dx * dx + dy * dy;
    if (lengthSquared == 0) { return {}; }

    ExactNet net{};
    std::array<Q, 4> distances{};
    for (std::size_t i = 0; i <= degree; ++i) {
        if (!std::isfinite(controls[i].x) || !std::isfinite(controls[i].y)) { return {}; }
        net[i] = {Q{controls[i].x}, Q{controls[i].y}};
        distances[i] = signedDistance(net[i], origin, dx, dy);
    }

    const bool increasing = distances[1] > distances[0];
    if (distances[1] == distances[0]) { return {}; }
    for (std::size_t i = 1; i < degree; ++i) {
        if (increasing ? distances[i + 1] <= distances[i]
                       : distances[i + 1] >= distances[i]) { return {}; }
    }
    if ((distances[0] < 0 && distances[degree] < 0) ||
        (distances[0] > 0 && distances[degree] > 0)) { return {}; }

    const auto makeCertificate = [&](const Q& parameterLo, const Q& parameterHi,
                                     const ExactNet& restricted,
                                     bool exactRoot, const ExactPoint& rootPoint)
            -> std::optional<MonotoneLineRootCertificate> {
        if (!exactRoot && parameterHi - parameterLo > Q{1} / Q{std::uint64_t{1} << 48}) {
            return {};
        }
        Q projectionLo, projectionHi;
        if (exactRoot) {
            const Q projection = dx * (rootPoint.x - origin.x) + dy * (rootPoint.y - origin.y);
            if (projection < 0 || projection > lengthSquared) { return {}; }
            projectionLo = projectionHi = projection;
        } else {
            bool first = true;
            for (std::size_t i = 0; i <= degree; ++i) {
                const Q projection = dx * (restricted[i].x - origin.x) +
                                     dy * (restricted[i].y - origin.y);
                if (first) { projectionLo = projectionHi = projection; first = false; }
                else { projectionLo = std::min(projectionLo, projection);
                       projectionHi = std::max(projectionHi, projection); }
            }
            if (projectionLo < 0 || projectionHi > lengthSquared) { return {}; }
        }

        const I parameter = exactRoot ? enclose(parameterLo)
            : I::hull(enclose(parameterLo).lo(), enclose(parameterHi).hi());
        const I lineParameter = I::hull(enclose(projectionLo / lengthSquared).lo(),
                                        enclose(projectionHi / lengthSquared).hi());
        if (!finite(parameter) || !finite(lineParameter)) { return {}; }
        return MonotoneLineRootCertificate{parameter, lineParameter};
    };

    if (distances[0] == 0) {
        return makeCertificate(Q{0}, Q{0}, net, true, net[0]);
    }
    if (distances[degree] == 0) {
        return makeCertificate(Q{1}, Q{1}, net, true, net[degree]);
    }

    Q parameterLo = 0, parameterHi = 1;
    for (unsigned iteration = 0; iteration < 96; ++iteration) {
        auto halves = splitHalf(net, degree);
        const Q middleParameter = (parameterLo + parameterHi) / 2;
        const Q middleDistance = signedDistance(halves.first[degree], origin, dx, dy);
        if (middleDistance == 0) {
            return makeCertificate(middleParameter, middleParameter,
                                   halves.first, true, halves.first[degree]);
        }
        const bool lowerHasRoot = (distances[0] < 0) != (middleDistance < 0);
        if (lowerHasRoot) {
            net = halves.first;
            parameterHi = middleParameter;
            distances[degree] = middleDistance;
        } else {
            net = halves.second;
            parameterLo = middleParameter;
            distances[0] = middleDistance;
        }
        if (auto certificate = makeCertificate(parameterLo, parameterHi, net, false, {})) {
            return certificate;
        }
    }
    return {};
}

Limits clamp(Limits l) {
    const Limits hard;
#define LC_TRIM_CLAMP(field) l.field = std::min(l.field, hard.field)
    LC_TRIM_CLAMP(maxSpansPerCurve); LC_TRIM_CLAMP(maxInputSpans); LC_TRIM_CLAMP(maxInputRecords);
    LC_TRIM_CLAMP(maxBoxPairs); LC_TRIM_CLAMP(maxLeaves); LC_TRIM_CLAMP(maxGeneratedItems);
    LC_TRIM_CLAMP(maxOrderingComparisons); LC_TRIM_CLAMP(maxRoots);
    LC_TRIM_CLAMP(maxDepth); LC_TRIM_CLAMP(maxLeafDepth); LC_TRIM_CLAMP(maxFragments);
    LC_TRIM_CLAMP(maxOutputSpans); LC_TRIM_CLAMP(maxOutputControls); LC_TRIM_CLAMP(maxOutputKnots);
#undef LC_TRIM_CLAMP
    return l;
}

void cancelled(const Cancellation& c) {
    if (c.requested && c.requested->load(std::memory_order_relaxed)) { fail(Refusal::Cancelled); }
}

struct Context {
    Limits limits;
    Cancellation cancellation;
    Work work;
    void check() const { cancelled(cancellation); }
    std::size_t itemsLeft() const {
        return limits.maxGeneratedItems - work.kernelItems - work.intersections.generatedItems;
    }
    std::size_t comparisonsLeft() const {
        return limits.maxOrderingComparisons - work.kernelComparisons - work.intersections.orderingComparisons;
    }
    void items(std::size_t n) {
        check();
        if (n > itemsLeft()) { fail(Refusal::LimitExceeded); }
        work.kernelItems += n;
    }
    void compare() {
        check();
        if (!comparisonsLeft()) { fail(Refusal::LimitExceeded); }
        ++work.kernelComparisons;
    }
    void roots(std::size_t n) {
        check();
        if (n > limits.maxRoots - work.rootRecords) { fail(Refusal::LimitExceeded); }
        work.rootRecords += n;
    }
    void adopt(const LC_IntersectionWorkStats& delta) {
        if (delta.generatedItems > itemsLeft() ||
            delta.totalBoxPairs > limits.maxBoxPairs - work.intersections.totalBoxPairs ||
            delta.generatedLeaves > limits.maxLeaves - work.intersections.generatedLeaves ||
            delta.orderingComparisons > comparisonsLeft()) { fail(Refusal::LimitExceeded); }
        auto& s = work.intersections;
#define LC_TRIM_ADD(field) s.field += delta.field
        LC_TRIM_ADD(candidateBoxPairs); LC_TRIM_ADD(refinementBoxPairs); LC_TRIM_ADD(certificateBoxPairs);
        LC_TRIM_ADD(totalBoxPairs); LC_TRIM_ADD(generatedLeaves); LC_TRIM_ADD(generatedItems);
        LC_TRIM_ADD(orderingComparisons); LC_TRIM_ADD(validationSegmentVisits); LC_TRIM_ADD(validationNextCalls);
#undef LC_TRIM_ADD
        s.deepestPreparation = std::max(s.deepestPreparation, delta.deepestPreparation);
        s.deepestPairRefinement = std::max(s.deepestPairRefinement, delta.deepestPairRefinement);
        s.deepestOrderingRefinement = std::max(s.deepestOrderingRefinement, delta.deepestOrderingRefinement);
        s.deepestRootRefinement = std::max(s.deepestRootRefinement, delta.deepestRootRefinement);
    }
    LC_IntersectionOptions options(std::size_t anchors) const {
        LC_IntersectionOptions o;
        o.strict = true;
        o.tolerance = 1e-12; // positive seed policy, NEVER root identity or clipping error
        o.anchoredBranches = anchors;
        o.analyticOccurrenceIdentity = true;
        o.strictProofDomainPadding = 1.0 / 16.0;
        o.maxDepth = limits.maxDepth; o.maxLeafDepth = limits.maxLeafDepth;
        o.maxLeaves = limits.maxLeaves - work.intersections.generatedLeaves;
        o.maxGeneratedItems = itemsLeft();
        o.maxBoxPairs = limits.maxBoxPairs - work.intersections.totalBoxPairs;
        o.maxOrderingComparisons = comparisonsLeft();
        return o;
    }
};

Refusal coreFailure(LC_IntersectionStatus s, Refusal ambiguous) {
    return s == LC_IntersectionStatus::LimitExceeded ? Refusal::LimitExceeded :
           s == LC_IntersectionStatus::InvalidInput ? Refusal::Arithmetic : ambiguous;
}

std::unique_ptr<LC_CertifiedIntersectionSession> query(
        Context& c, std::unique_ptr<const LC_ParametricCurves> provider, Refusal ambiguous) {
    c.check();
    const auto options = c.options(1);
    if (options.maxGeneratedItems < 2 || !options.maxLeaves || !options.maxBoxPairs) {
        fail(Refusal::LimitExceeded);
    }
    LC_IntersectionWorkStats entered;
    LC_CertifiedQueryResult result;
    try { result = findCertifiedIntersections(std::move(provider), options, &entered); }
    catch (...) { c.adopt(entered); throw; }
    c.adopt(entered);
    if (result.status != LC_IntersectionStatus::Ok) { fail(coreFailure(result.status, ambiguous)); }
    if (!result.session || !result.session->hasValidProofs()) { fail(Refusal::AmbiguousRoots); }
    return std::move(result.session);
}

LC_CertifiedRootSnapshot contract(Context& c, LC_CertifiedIntersectionSession& session,
        const LC_CertifiedRootHandle& handle, Refusal ambiguous) {
    c.check();
    LC_IntersectionWorkStats entered;
    LC_CertifiedContractionResult result;
    try {
        result = session.contract(handle, {c.limits.maxBoxPairs - c.work.intersections.totalBoxPairs,
            c.itemsLeft(), c.limits.maxDepth, c.comparisonsLeft()}, &entered);
    } catch (...) { c.adopt(entered); throw; }
    c.adopt(entered);
    if (result.status != LC_IntersectionStatus::Ok || !result.snapshot || !session.hasValidProofs()) {
        fail(coreFailure(result.status, ambiguous));
    }
    return *result.snapshot;
}

I hullNet(const Net& net, std::size_t degree, bool x) {
    I value = x ? net[0].x : net[0].y;
    for (std::size_t i = 1; i <= degree; ++i) { value = I::hull(value, x ? net[i].x : net[i].y); }
    return value;
}

bool rawBound(const Geo::CacheSpan& span, const Geo::ReferenceFrame& frame,
              double a, double b, I& x, I& y, I& dx, I& dy) {
    Geo::CurveChordBounds chord;
    Net net; std::size_t degree = 0;
    if (!Geo::boundCacheChord(span, frame, a, b, chord) ||
        !Geo::boundCacheDerivativeControls(span, frame, a, b, net, degree)) { return false; }
    x = chord.x; y = chord.y; dx = hullNet(net, degree, true); dy = hullNet(net, degree, false);
    return finite(x) && finite(y) && finite(dx) && finite(dy);
}

I cacheCutRootError(const Geo::CacheSpan& span, const Geo::ReferenceFrame& frame,
                    const I& rootParameter, const double cutParameter) {
    I u = intersect(rootParameter, I::hull(0, 1));
    if (!finite(u) || !std::isfinite(cutParameter) || !u.contains(cutParameter)) { return {}; }
    Net derivative; std::size_t degree = 0;
    if (!Geo::boundCacheDerivativeControls(span, frame, u.lo(), u.hi(), derivative, degree)) { return {}; }
    const I dx = hullNet(derivative, degree, true), dy = hullNet(derivative, degree, false);
    if (!finite(dx) || !finite(dy)) { return {}; }
    const double delta = std::max(std::abs(cutParameter - u.lo()), std::abs(u.hi() - cutParameter));
    const double speedX = std::max(std::abs(dx.lo()), std::abs(dx.hi()));
    const double speedY = std::max(std::abs(dy.lo()), std::abs(dy.hi()));
    const I x = I::point(speedX) * I::point(delta);
    const I y = I::point(speedY) * I::point(delta);
    if (!finite(x) || !finite(y)) { return {}; }
    std::array<std::array<Q, 2>, 4> point;
    for (std::size_t i = 0; i <= span.degree; ++i) {
        point[i] = {Q(span.controls[i].x), Q(span.controls[i].y)};
    }
    const Q t{cutParameter}, oneMinus = Q{1} - t;
    for (std::size_t level = span.degree; level > 0; --level) {
        for (std::size_t i = 0; i < level; ++i) {
            point[i] = {oneMinus * point[i][0] + t * point[i + 1][0],
                        oneMinus * point[i][1] + t * point[i + 1][1]};
        }
    }
    const double roundedX = point[0][0].convert_to<double>();
    const double roundedY = point[0][1].convert_to<double>();
    if (!std::isfinite(roundedX) || !std::isfinite(roundedY)) { return {}; }
    const auto ulp = [](const double value) {
        const double lower = std::nextafter(value, -INFINITY);
        const double upper = std::nextafter(value, INFINITY);
        return std::isfinite(upper) ? std::max(value - lower, upper - value) : value - lower;
    };
    const I scale = I::point(frame.scale());
    const I roundingX = I::point(4) * I::point(ulp(roundedX)) / scale;
    const I roundingY = I::point(4) * I::point(ulp(roundedY)) / scale;
    if (!finite(roundingX) || !finite(roundingY)) { return {}; }
    const I error = sqrt(sqr(x + roundingX) + sqr(y + roundingY));
    return finite(error) ? error : I{};
}

I contractCacheRootAgainstLine(const Geo::CacheSpan& source, const Geo::CacheSpan& line,
                               const Geo::ReferenceFrame& frame, const I& parameter) {
    if (line.degree != 1 || !finite(parameter) || parameter.lo() < 0 || parameter.hi() > 1) { return {}; }
    std::array<std::array<Q, 2>, 4> point;
    for (std::size_t i = 0; i <= source.degree; ++i) {
        point[i] = {Q(source.controls[i].x), Q(source.controls[i].y)};
    }
    const double middle = mid(parameter);
    const Q middleExact{middle}, oneMinus = Q{1} - middleExact;
    for (std::size_t level = source.degree; level > 0; --level) {
        for (std::size_t i = 0; i < level; ++i) {
            point[i] = {oneMinus * point[i][0] + middleExact * point[i + 1][0],
                        oneMinus * point[i][1] + middleExact * point[i + 1][1]};
        }
    }
    const Q lineXExact = Q(line.controls[1].x) - Q(line.controls[0].x);
    const Q lineYExact = Q(line.controls[1].y) - Q(line.controls[0].y);
    const I lineX = enclose(lineXExact), lineY = enclose(lineYExact);
    const I value = enclose(-lineYExact * (point[0][0] - Q(line.controls[0].x)) +
                            lineXExact * (point[0][1] - Q(line.controls[0].y)));
    if (!finite(lineX) || !finite(lineY) || !finite(value)) { return {}; }
    Net derivative; std::size_t degree = 0;
    if (!Geo::boundCacheDerivativeControls(source, frame, parameter.lo(), parameter.hi(), derivative, degree)) { return {}; }
    const I scale = I::point(frame.scale());
    const I dx = hullNet(derivative, degree, true) * scale;
    const I dy = hullNet(derivative, degree, false) * scale;
    const I projectedDerivative = -lineY * dx + lineX * dy;
    if (!finite(projectedDerivative) || !projectedDerivative.excludesZero()) { return {}; }
    const I newton = I::point(middle) - value / projectedDerivative;
    return finite(newton) ? intersect(parameter, newton) : I{};
}

using Axis = std::array<double, 2>;
I project(const Geo::CacheBezierPoint& p, const Axis& a) {
    if (singleton(p.x) && p.x.lo() == 0 && singleton(p.y) && p.y.lo() == 0) { return I::point(0); }
    if (a[0] == 0) { return I::point(a[1]) * p.y; }
    if (a[1] == 0) { return I::point(a[0]) * p.x; }
    return I::point(a[0]) * p.x + I::point(a[1]) * p.y;
}
bool forward(const Net& net, std::size_t degree, const Axis& a) {
    if (!std::isfinite(a[0]) || !std::isfinite(a[1]) || (a[0] == 0 && a[1] == 0)) { return false; }
    bool positive = false;
    for (std::size_t i = 0; i <= degree; ++i) {
        const auto v = project(net[i], a);
        if (!finite(v) || v.lo() < 0) { return false; }
        positive = positive || v.lo() > 0;
    }
    return positive;
}
Axis direction(const Geo::CacheBezierPoint& p) {
    const double x = mid(p.x), y = mid(p.y), d = std::hypot(x, y);
    return d > 0 && std::isfinite(d) ? Axis{x / d, y / d} : Axis{};
}
bool commonProjection(const Net& a, std::size_t da, const Net& b, std::size_t db) {
    const auto end = direction(a[da]), start = direction(b[0]);
    const std::array<Axis, 11> axes{{{1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}, end, start, {end[0] + start[0], end[1] + start[1]}}};
    for (const auto& axis : axes) {
        if (forward(a, da, axis) && forward(b, db, axis)) { return true; }
    }
    return false;
}

struct Conic {
    RS_EllipseData data;
    Geo::ReferenceFrame unit;
};

bool conicJet(const Conic& conic, const Geo::ReferenceFrame& frame,
              double a, double b, LC_CurveJetBounds& out) {
    LC_CurveJetBounds unit;
    I cx, cy;
    if (!conic.unit.boundSourceJet(a, b, unit) || !frame.pointToFrame(conic.data.center, cx, cy)) return false;
    const I scale = I::point(frame.scale());
    const I ax = I::point(conic.data.majorP.x) / scale, ay = I::point(conic.data.majorP.y) / scale;
    const I bx = -ay * I::point(conic.data.ratio), by = ax * I::point(conic.data.ratio);
    out.x = cx + ax * unit.x + bx * unit.y; out.y = cy + ay * unit.x + by * unit.y;
    out.dx = ax * unit.dx + bx * unit.dy; out.dy = ay * unit.dx + by * unit.dy;
    out.ddx = ax * unit.ddx + bx * unit.ddy; out.ddy = ay * unit.ddx + by * unit.ddy;
    return finite(out.x) && finite(out.y) && finite(out.dx) && finite(out.dy) && finite(out.ddx) && finite(out.ddy);
}

double conicAllowance(const Conic& conic, const Geo::ReferenceFrame& frame) {
    const double axis = std::max({std::abs(conic.data.majorP.x), std::abs(conic.data.majorP.y),
        std::abs(conic.data.majorP.x * conic.data.ratio), std::abs(conic.data.majorP.y * conic.data.ratio)}) / frame.scale();
    const double error = 4096 * std::numeric_limits<double>::epsilon() * axis;
    if (!std::isfinite(error) || !(error > 0)) fail(Refusal::Arithmetic);
    return error;
}

// The existing strict stationary-query pattern, now with actual analytic
// conic jets. x=t proves global injectivity of each scalar graph; no sampled
// point or atan2 result becomes a closest-occurrence certificate.
class ConicStationary final : public LC_ParametricCurves {
public:
    ConicStationary(Conic conic, Geo::ReferenceFrame frame, RS_Vector pick, Cancellation cancel)
        : m_conic(std::move(conic)), m_frame(std::move(frame)), m_pick(pick), m_cancel(std::move(cancel)),
          m_segments{{0, 0, m_conic.unit.parameterEnd()}, {1, 0, m_conic.unit.parameterEnd()}} {}
    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return 2; }
    std::ptrdiff_t next(std::size_t) const override { return -1; }
    bool bound(std::size_t s, double a, double b, I& x, I& y, I& dx, I& dy) const override {
        cancelled(m_cancel);
        if (s > 1 || !(0 <= a && a <= b && b <= m_conic.unit.parameterEnd())) return false;
        x = I::hull(a, b); dx = I::point(1);
        if (s == 1) { y = dy = I::point(0); return true; }
        LC_CurveJetBounds jet; I px, py;
        if (!conicJet(m_conic, m_frame, a, b, jet) || !m_frame.pointToFrame(m_pick, px, py)) return false;
        y = (jet.x - px) * jet.dx + (jet.y - py) * jet.dy;
        dy = sqr(jet.dx) + sqr(jet.dy) + (jet.x - px) * jet.ddx + (jet.y - py) * jet.ddy;
        return finite(y) && finite(dy);
    }
    bool evaluate(std::size_t s, double t, RS_Vector& p, RS_Vector& d) const override {
        I x, y, dx, dy;
        if (!bound(s, t, t, x, y, dx, dy)) return false;
        p = {mid(x), mid(y)}; d = {mid(dx), mid(dy)};
        return Geo::isFinite2D(p) && Geo::isFinite2D(d);
    }
    LC_AnalyticLeafCertificate certifyInjective(std::size_t s, const I& t) const override {
        cancelled(m_cancel);
        return s < 2 && finite(t) && 0 <= t.lo() && t.hi() <= m_conic.unit.parameterEnd()
            ? LC_AnalyticLeafCertificate{true, s, t} : LC_AnalyticLeafCertificate{};
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        cancelled(m_cancel);
        if (box.segmentA == box.segmentB) {
            if (!certifyInjective(box.segmentA, I::hull(box.parameterA, box.parameterB)).available) return {};
            LC_AnalyticIntersectionCertificate result;
            result.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            result.proof = LC_IntersectionProof::AnalyticExclusion; result.proofDomain = box;
            return result;
        }
        if (box.segmentA != 0 || box.segmentB != 1) return {};
        I x, y, dx, dy;
        if (!bound(0, box.parameterA.lo(), box.parameterA.hi(), x, y, dx, dy) || !dy.excludesZero()) return {};
        for (const double t : {0., m_conic.unit.parameterEnd()}) {
            if (!box.parameterA.contains(t) || !box.parameterB.contains(t)) continue;
            // Certify the actual provider chart endpoint, NOT a raw storage
            // angle. Whole ReferenceFrames start at pi/4, not raw angle1=0.
            // Only an actual theta=0 endpoint has this rational trig proof.
            const double theta = t == 0 || m_conic.unit.sourceSweep().whole
                ? m_conic.unit.sourceSweep().start : m_conic.data.angle2;
            if (theta != 0) continue;
            const auto& d = m_conic.data;
            const Q ex = Q(d.center.x) + Q(d.majorP.x) - Q(m_pick.x);
            const Q ey = Q(d.center.y) + Q(d.majorP.y) - Q(m_pick.y);
            if (ex * -Q(d.majorP.y) + ey * Q(d.majorP.x) != 0) continue;
            LC_AnalyticIntersectionCertificate result;
            result.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            result.proof = LC_IntersectionProof::AnalyticEndpoint; result.proofDomain = box;
            result.isolationA = result.isolationB = I::point(t);
            result.kind = LC_ParametricIntersection::Kind::Endpoint;
            return result;
        }
        return {};
    }
private:
    Conic m_conic;
    Geo::ReferenceFrame m_frame;
    RS_Vector m_pick;
    Cancellation m_cancel;
    std::vector<LC_ParametricSegment> m_segments;
};

class Curves final : public LC_ParametricCurves {
public:
    Curves(Geo::ReferenceFrame frame, std::shared_ptr<const Spans> source,
           std::shared_ptr<const Spans> cutter, std::optional<Conic> conic,
           bool closed, bool cutterClosed, Cancellation cancel)
        : m_frame(std::move(frame)), m_source(std::move(source)), m_cutter(std::move(cutter)),
          m_conic(std::move(conic)), m_closed(closed), m_cutterClosed(cutterClosed), m_cancel(std::move(cancel)) {
        m_segments.reserve(m_source->size() + (m_conic ? 8 : m_cutter->size()));
        for (std::size_t k = 0; k < m_source->size(); ++k) {
            m_segments.push_back({0, double(k), double(k + 1)});
        }
        if (m_conic) {
            const double end = m_conic->unit.parameterEnd();
            for (std::size_t k = 0; k < 8; ++k) {
                // Avoid octant-aligned internal cuts without changing the
                // eight-patch bound. Cuts themselves carry no root proof.
                m_segments.push_back({1, end * double(k * (24 + k)) / 256,
                    end * double((k + 1) * (25 + k)) / 256});
            }
        } else {
            for (std::size_t k = 0; k < m_cutter->size(); ++k) {
                m_segments.push_back({1, double(k), double(k + 1)});
            }
        }
    }
    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return 2; }
    std::ptrdiff_t next(std::size_t branch) const override {
        return branch == 0 ? (m_closed ? 0 : -1) : (branch == 1 && m_cutterClosed ? 1 : -1);
    }
    bool contains(std::size_t s, const I& t) const {
        return s < m_segments.size() && finite(t) && m_segments[s].t0 <= t.lo() && t.hi() <= m_segments[s].t1;
    }
    const Geo::CacheSpan* raw(std::size_t s) const {
        if (s < m_source->size()) { return &(*m_source)[s]; }
        return !m_conic && s < m_segments.size() ? &(*m_cutter)[s - m_source->size()] : nullptr;
    }
    bool derivative(std::size_t s, double a, double b, Net& net, std::size_t& degree) const {
        const auto* span = raw(s);
        return span && contains(s, I::hull(a, b)) &&
            Geo::boundCacheDerivativeControls(*span, m_frame, a - m_segments[s].t0,
                b - m_segments[s].t0, net, degree);
    }
    bool bound(std::size_t s, double a, double b, I& x, I& y, I& dx, I& dy) const override {
        cancelled(m_cancel);
        if (a > b || !contains(s, I::hull(a, b))) { return false; }
        if (const auto* span = raw(s)) {
            return rawBound(*span, m_frame, a - m_segments[s].t0, b - m_segments[s].t0, x, y, dx, dy);
        }
        LC_CurveJetBounds j;
        if (!m_conic || !m_conic->unit.boundSourceJet(a, b, j)) { return false; }
        const auto& d = m_conic->data;
        I cx, cy;
        if (!m_frame.pointToFrame(d.center, cx, cy)) { return false; }
        const I scale = I::point(m_frame.scale());
        const I ax = I::point(d.majorP.x) / scale, ay = I::point(d.majorP.y) / scale;
        const I bx = -ay * I::point(d.ratio), by = ax * I::point(d.ratio);
        x = cx + ax * j.x + bx * j.y; y = cy + ay * j.x + by * j.y;
        dx = ax * j.dx + bx * j.dy; dy = ay * j.dx + by * j.dy;
        return finite(x) && finite(y) && finite(dx) && finite(dy);
    }
    bool evaluate(std::size_t s, double t, RS_Vector& p, RS_Vector& d) const override {
        I x, y, dx, dy;
        if (!bound(s, t, t, x, y, dx, dy)) { return false; }
        p = RS_Vector{mid(x), mid(y)}; d = RS_Vector{mid(dx), mid(dy)};
        return Geo::isFinite2D(p) && Geo::isFinite2D(d);
    }
    LC_AnalyticLeafCertificate certifyInjective(std::size_t s, const I& t) const override {
        cancelled(m_cancel);
        Net net; std::size_t degree = 0;
        if (!contains(s, t) || !derivative(s, t.lo(), t.hi(), net, degree) ||
            !commonProjection(net, degree, net, degree)) { return {}; }
        return {true, s, t};
    }
    bool join(std::size_t first, double from, std::size_t last, double to) const {
        const auto* a = raw(first); const auto* b = raw(last);
        Net left, right; std::size_t da = 0, db = 0;
        return a && b && same(a->controls[a->degree], b->controls[0]) &&
            derivative(first, from, m_segments[first].t1, left, da) &&
            derivative(last, m_segments[last].t0, to, right, db) && commonProjection(left, da, right, db);
    }
    bool freeEnd(std::size_t s, const I& t) const {
        if (!singleton(t)) { return false; }
        const bool source = s < m_source->size();
        if (source ? m_closed : m_cutterClosed) { return false; }
        const auto first = source ? 0 : m_source->size();
        const auto last = source ? m_source->size() - 1 : m_segments.size() - 1;
        return t.lo() == m_segments[first].t0 || t.lo() == m_segments[last].t1;
    }
    bool incidentDirections(std::size_t s, const I& t,
                            std::array<Geo::CacheBezierPoint, 2>& directions, std::size_t& count) const {
        count = 0;
        if (!contains(s, t)) { return false; }
        const auto add = [&](std::size_t segment, const I& parameter) {
            I x, y, dx, dy;
            if (!bound(segment, parameter.lo(), parameter.hi(), x, y, dx, dy)) { return false; }
            directions[count++] = {dx, dy}; return true;
        };
        if (!add(s, t) || !singleton(t)) { return count == 1; }
        const bool source = s < m_source->size(), closed = source ? m_closed : m_cutterClosed;
        const auto first = source ? 0 : m_source->size();
        const auto last = source ? m_source->size() - 1 : m_segments.size() - 1;
        if (t.lo() == m_segments[s].t0 && (s > first || closed)) {
            const auto previous = s > first ? s - 1 : last;
            return add(previous, I::point(m_segments[previous].t1));
        }
        if (t.lo() == m_segments[s].t1 && (s < last || closed)) {
            const auto next = s < last ? s + 1 : first;
            return add(next, I::point(m_segments[next].t0));
        }
        return true;
    }

    // Exact stored endpoint on an exact stored line, plus whole-box strict
    // monotonicity of its signed distance, proves ONE boundary contact.
    LC_AnalyticIntersectionCertificate endpoint(const LC_IntersectionDomain& box,
                                                std::array<Q, 2>* exactParameters = nullptr) const {
        if (!contains(box.segmentA, box.parameterA) || !contains(box.segmentB, box.parameterB)) { return {}; }
        for (bool swapped : {false, true}) {
            const auto s = swapped ? box.segmentB : box.segmentA, l = swapped ? box.segmentA : box.segmentB;
            const auto a = swapped ? box.parameterB : box.parameterA, b = swapped ? box.parameterA : box.parameterB;
            const auto* span = raw(s); const auto* line = raw(l);
            if (!span || !line || line->degree != 1) { continue; }
            const Q vx = Q(line->controls[1].x) - Q(line->controls[0].x);
            const Q vy = Q(line->controls[1].y) - Q(line->controls[0].y);
            if (vx == 0 && vy == 0) { continue; }
            for (std::size_t end : {std::size_t{0}, span->degree}) {
                const double t = end == 0 ? m_segments[s].t0 : m_segments[s].t1;
                if (!a.contains(t)) { continue; }
                const Q px = Q(span->controls[end].x) - Q(line->controls[0].x);
                const Q py = Q(span->controls[end].y) - Q(line->controls[0].y);
                if (px * vy - py * vx != 0) { continue; }
                const Q u = vx != 0 ? px / vx : py / vy;
                if (u < 0 || u > 1) { continue; }
                const Q lineParameter = u + Q(m_segments[l].t0);
                const auto lineT = enclose(lineParameter);
                if (!finite(lineT) || lineT.lo() < b.lo() || lineT.hi() > b.hi()) { continue; }
                I x, y, dx, dy, lx, ly, ldx, ldy;
                if (!bound(s, a.lo(), a.hi(), x, y, dx, dy) ||
                    !bound(l, b.lo(), b.hi(), lx, ly, ldx, ldy) || !(dx * ldy - dy * ldx).excludesZero()) { continue; }
                LC_AnalyticIntersectionCertificate out;
                out.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
                out.proof = LC_IntersectionProof::AnalyticEndpoint; out.proofDomain = box;
                out.isolationA = swapped ? lineT : I::point(t);
                out.isolationB = swapped ? I::point(t) : lineT;
                if (exactParameters) {
                    (*exactParameters)[swapped ? 1 : 0] = Q(t);
                    (*exactParameters)[swapped ? 0 : 1] = lineParameter;
                }
                out.kind = freeEnd(s, I::point(t)) || freeEnd(l, lineT)
                    ? LC_ParametricIntersection::Kind::Endpoint : LC_ParametricIntersection::Kind::Transverse;
                return out;
            }
        }
        return {};
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        cancelled(m_cancel);
        if (!contains(box.segmentA, box.parameterA) || !contains(box.segmentB, box.parameterB)) { return {}; }
        auto first = box.segmentA, last = box.segmentB;
        auto a = box.parameterA, b = box.parameterB;
        if (first > last || (first == last && a.lo() > b.lo())) { std::swap(first, last); std::swap(a, b); }
        bool excluded = false;
        if (m_segments[first].branch == m_segments[last].branch) {
            if (first == last && a.hi() <= b.lo()) {
                excluded = certifyInjective(first, I::hull(a.lo(), b.hi())).available;
            } else if (last == first + 1) { excluded = join(first, a.lo(), last, b.hi()); }
            if (!excluded && m_closed && first == 0 && last == m_source->size() - 1) {
                excluded = join(last, b.lo(), first, a.hi());
            }
        }
        if (!excluded) { return endpoint(box); }
        LC_AnalyticIntersectionCertificate out;
        out.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
        out.proof = LC_IntersectionProof::AnalyticExclusion; out.proofDomain = box;
        return out;
    }
    LC_AnalyticOccurrenceIdentityCertificate certifyOccurrenceIdentity(
            const LC_ParametricIntersection& a, bool sideA,
            const LC_ParametricIntersection& b, bool sideB) const override {
        cancelled(m_cancel);
        std::array<Q, 2> exactA, exactB;
        const auto ca = endpoint(a.proofDomain, &exactA), cb = endpoint(b.proofDomain, &exactB);
        if (ca.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique ||
            cb.outcome != LC_AnalyticIntersectionCertificate::Outcome::Unique) { return {}; }
        // Exact stored endpoints can meet a finite line at a non-dyadic
        // parameter. Compare the proved rational occurrences, never equal or
        // overlapping floating enclosures (which can contain distinct roots).
        const Q& pa = exactA[sideA ? 0 : 1]; const Q& pb = exactB[sideB ? 0 : 1];
        if (pa != pb) { return {}; }
        const auto parameter = enclose(pa);
        if (!finite(parameter)) { return {}; }
        return {true, a.proofDomain, b.proofDomain, parameter};
    }

private:
    Geo::ReferenceFrame m_frame;
    std::shared_ptr<const Spans> m_source, m_cutter;
    std::optional<Conic> m_conic;
    bool m_closed, m_cutterClosed;
    Cancellation m_cancel;
    std::vector<LC_ParametricSegment> m_segments;
};

// A scalar stationary equation is an ordinary strict intersection query of
// (u,(B-P).B') against (u,0). Its roots are exhaustive, not sampled candidates.
class Stationary final : public LC_ParametricCurves {
public:
    Stationary(Geo::CacheSpan span, Geo::ReferenceFrame frame, RS_Vector pick, Cancellation c)
        : m_span(std::move(span)), m_frame(std::move(frame)), m_pick(pick), m_cancel(std::move(c)),
          m_segments{{0, 0, 1}, {1, 0, 1}} {}
    const std::vector<LC_ParametricSegment>& segments() const override { return m_segments; }
    std::size_t branchCount() const override { return 2; }
    std::ptrdiff_t next(std::size_t) const override { return -1; }
    bool bound(std::size_t s, double a, double b, I& x, I& y, I& dx, I& dy) const override {
        cancelled(m_cancel);
        if (s > 1 || !(0 <= a && a <= b && b <= 1)) { return false; }
        x = I::hull(a, b); dx = I::point(1);
        if (s == 1) { y = dy = I::point(0); return true; }
        I px, py, bx, by, vx, vy;
        if (!m_frame.pointToFrame(m_pick, px, py) || !rawBound(m_span, m_frame, a, b, bx, by, vx, vy)) { return false; }
        Net net; std::size_t degree = 0;
        if (!Geo::boundCacheDerivativeControls(m_span, m_frame, 0, 1, net, degree)) { return false; }
        Net second;
        if (degree == 0) { second[0] = {I::point(0), I::point(0)}; }
        else {
            for (std::size_t i = 0; i < degree; ++i) {
                second[i] = {I::point(double(degree)) * (net[i + 1].x - net[i].x),
                             I::point(double(degree)) * (net[i + 1].y - net[i].y)};
            }
            // Restrict the SECOND-derivative net with interval de Casteljau.
            const auto split = [](Net input, std::size_t d, I t, Net& left, Net& right) {
                left[0] = input[0]; right[d] = input[d];
                for (std::size_t level = 1; level <= d; ++level) {
                    for (std::size_t i = 0; i <= d - level; ++i) {
                        input[i] = {(I::point(1) - t) * input[i].x + t * input[i + 1].x,
                                    (I::point(1) - t) * input[i].y + t * input[i + 1].y};
                    }
                    left[level] = input[0]; right[d - level] = input[d - level];
                }
            };
            Net left, right;
            if (b < 1) { split(second, degree - 1, I::point(b), left, right); second = left; }
            if (a > 0) {
                split(second, degree - 1, I::point(a) / I::point(b), left, right); second = right;
            }
        }
        const auto dd = degree == 0 ? 0 : degree - 1;
        y = (bx - px) * vx + (by - py) * vy;
        dy = sqr(vx) + sqr(vy) + (bx - px) * hullNet(second, dd, true) + (by - py) * hullNet(second, dd, false);
        return finite(y) && finite(dy);
    }
    bool evaluate(std::size_t s, double t, RS_Vector& p, RS_Vector& d) const override {
        I x, y, dx, dy;
        if (!bound(s, t, t, x, y, dx, dy)) { return false; }
        p = RS_Vector{mid(x), mid(y)}; d = RS_Vector{mid(dx), mid(dy)};
        return Geo::isFinite2D(p) && Geo::isFinite2D(d);
    }
    LC_AnalyticLeafCertificate certifyInjective(std::size_t s, const I& t) const override {
        cancelled(m_cancel);
        return s < 2 && finite(t) && 0 <= t.lo() && t.hi() <= 1 ? LC_AnalyticLeafCertificate{true, s, t}
                                                                            : LC_AnalyticLeafCertificate{};
    }
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override {
        cancelled(m_cancel);
        if (box.segmentA == box.segmentB) {
            if (!certifyInjective(box.segmentA, I::hull(box.parameterA, box.parameterB)).available) { return {}; }
            LC_AnalyticIntersectionCertificate out;
            out.outcome = LC_AnalyticIntersectionCertificate::Outcome::Excluded;
            out.proof = LC_IntersectionProof::AnalyticExclusion; out.proofDomain = box;
            return out;
        }
        if (box.segmentA != 0 || box.segmentB != 1) { return {}; }
        I x, y, dx, dy;
        if (!bound(0, box.parameterA.lo(), box.parameterA.hi(), x, y, dx, dy) || !dy.excludesZero()) { return {}; }
        for (const double t : {0., 1.}) {
            if (!box.parameterA.contains(t) || !box.parameterB.contains(t)) { continue; }
            const auto end = t == 0 ? 0 : m_span.degree;
            const auto neighbor = t == 0 ? 1 : m_span.degree - 1;
            const Q vx = Q(m_span.controls[neighbor].x) - Q(m_span.controls[end].x);
            const Q vy = Q(m_span.controls[neighbor].y) - Q(m_span.controls[end].y);
            if ((Q(m_span.controls[end].x) - Q(m_pick.x)) * vx +
                (Q(m_span.controls[end].y) - Q(m_pick.y)) * vy != 0) { continue; }
            LC_AnalyticIntersectionCertificate out;
            out.outcome = LC_AnalyticIntersectionCertificate::Outcome::Unique;
            out.proof = LC_IntersectionProof::AnalyticEndpoint; out.proofDomain = box;
            out.isolationA = out.isolationB = I::point(t);
            out.kind = LC_ParametricIntersection::Kind::Endpoint;
            return out;
        }
        return {};
    }
private:
    Geo::CacheSpan m_span;
    Geo::ReferenceFrame m_frame;
    RS_Vector m_pick;
    Cancellation m_cancel;
    std::vector<LC_ParametricSegment> m_segments;
};

std::size_t preflight(Context& c, const RS_SplineData& data) {
    c.check();
    const auto degree = std::size_t(data.degree);
    if ((degree != 1 && degree != 3) || data.controlPoints.size() < degree + 1 ||
        data.knotslist.size() != data.controlPoints.size() + degree + 1 ||
        data.weights.size() != data.controlPoints.size()) fail(Refusal::UnsupportedLayout);
    // Charge every input array, including forbidden payload, before scanning knots.
    std::size_t n = 0;
    const auto add = [&](std::size_t count) {
        if (count > c.limits.maxInputRecords - c.work.inputRecords - n) { fail(Refusal::LimitExceeded); }
        n += count;
    };
    add(data.controlPoints.size()); add(data.weights.size()); add(data.knotslist.size());
    add(data.fitPoints.size()); add(data.savedOpenKnots.size());
    std::size_t spans = 0;
    for (std::size_t i = 0; i < data.knotslist.size();) {
        if (!std::isfinite(data.knotslist[i]) || (i && !(data.knotslist[i - 1] <= data.knotslist[i])))
            fail(Refusal::UnsupportedLayout);
        const double value = data.knotslist[i];
        do { ++i; } while (i < data.knotslist.size() && data.knotslist[i] == value);
        ++spans;
    }
    if (spans < 2) fail(Refusal::UnsupportedLayout);
    --spans;
    if (spans > c.limits.maxSpansPerCurve || spans > c.limits.maxInputSpans - c.work.inputSpans) {
        fail(Refusal::LimitExceeded);
    }
    add(spans); add(spans);
    c.work.inputSpans += spans; c.work.inputRecords += n;
    return spans;
}

Spans inspect(Context& c, const RS_SplineData& data) {
    c.check();
    LC_OffsetCheckLimits limits;
    limits.maxCacheSpans = c.limits.maxSpansPerCurve;
    limits.maxVertices = c.limits.maxInputRecords;
    Geo::WorkBudget budget(limits);
    auto result = Geo::inspectCacheLayout(data, budget);
    if (result.status != Geo::CacheLayoutStatus::Ok) {
        fail(result.status == Geo::CacheLayoutStatus::LimitExceeded ? Refusal::LimitExceeded : Refusal::UnsupportedLayout);
    }
    return std::move(result.spans);
}

bool closure(const RS_SplineData& data, const Spans& spans) {
    const bool closed = same(spans.front().controls[0], spans.back().controls[spans.back().degree]);
    // The actual offset producer emits clamped controls with coincident ends
    // without setting the DXF closed flag. That is still one closed cache path.
    if ((data.m_closedFlag || data.m_periodicFlag) && !closed) { fail(Refusal::UnsupportedLayout); }
    return closed;
}

struct Component { std::size_t begin = 0, end = 0; bool closed = false; };

bool isCuspBranchBreak(const Geo::CacheSpan& before, const Geo::CacheSpan& after) {
    if (before.degree != after.degree || after.controlOffset < before.controlOffset ||
        after.controlOffset - before.controlOffset != before.degree + 1 ||
        !same(before.controls[before.degree], after.controls[0])) return false;
    return same(before.controls[before.degree - 1], before.controls[before.degree]) &&
           same(after.controls[0], after.controls[1]);
}

std::vector<Component> components(const Spans& spans) {
    std::vector<Component> result;
    if (spans.empty()) return result;
    std::size_t begin = 0;
    for (std::size_t i = 1; i <= spans.size(); ++i) {
        if (i != spans.size() &&
            (isCuspBranchBreak(spans[i - 1], spans[i]) ||
             !same(spans[i - 1].controls[spans[i - 1].degree], spans[i].controls[0]))) {
            result.push_back({begin, i,
                same(spans[begin].controls[0], spans[i - 1].controls[spans[i - 1].degree])});
            begin = i;
            continue;
        }
        if (i != spans.size()) continue;
        result.push_back({begin, i,
            same(spans[begin].controls[0], spans[i - 1].controls[spans[i - 1].degree])});
        begin = i;
    }
    return result;
}

struct Input {
    std::shared_ptr<const Spans> source, cutter;
    std::optional<Conic> conic;
    bool closed = false, cutterClosed = false;
    Geo::ReferenceFrame frame;
};

Input admit(Context& c, const RS_SplineData& source, const RS_Entity& entity, const Pick& pick) {
    c.check();
    if (!Geo::isFinite2D(pick.worldPoint)) { fail(Refusal::InvalidInput); }
    const auto sourceCount = preflight(c, source);
    if ((pick.rawSpanHint && *pick.rawSpanHint >= sourceCount) ||
        (pick.localParameterSeed && (!std::isfinite(*pick.localParameterSeed) ||
         *pick.localParameterSeed < 0 || *pick.localParameterSeed > 1))) { fail(Refusal::InvalidInput); }
    const auto* spline = dynamic_cast<const RS_Spline*>(&entity);
    const auto* line = dynamic_cast<const RS_Line*>(&entity);
    const auto* circle = dynamic_cast<const RS_Circle*>(&entity);
    const auto* arc = dynamic_cast<const RS_Arc*>(&entity);
    const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity);
    RS_EllipseData conicData;
    if (spline) { preflight(c, spline->getData()); }
    else if (line) {
        if (c.work.inputSpans == c.limits.maxInputSpans || !c.limits.maxSpansPerCurve ||
            4 > c.limits.maxInputRecords - c.work.inputRecords) { fail(Refusal::LimitExceeded); }
        ++c.work.inputSpans; c.work.inputRecords += 4;
    } else if (circle || arc || ellipse) {
        if (8 > c.limits.maxInputSpans - c.work.inputSpans || 8 > c.limits.maxSpansPerCurve ||
            9 > c.limits.maxInputRecords - c.work.inputRecords) { fail(Refusal::LimitExceeded); }
        c.work.inputSpans += 8; c.work.inputRecords += 9;
        if (ellipse) { conicData = ellipse->getData(); }
        else if (circle) {
            conicData.center = circle->getCenter(); conicData.majorP = RS_Vector{circle->getRadius(), 0}; conicData.ratio = 1;
        } else {
            const auto& a = arc->getData();
            conicData.center = a.center; conicData.majorP = RS_Vector{a.radius, 0}; conicData.ratio = 1;
            conicData.angle1 = a.angle1; conicData.angle2 = a.angle2; conicData.reversed = a.reversed; conicData.isArc = true;
            if (a.angle1 == a.angle2) { fail(Refusal::UnsupportedCutter); }
        }
        if (!Geo::isFinite2D(conicData.center) || !Geo::isFinite2D(conicData.majorP) ||
            (conicData.majorP.x == 0 && conicData.majorP.y == 0) ||
            !std::isfinite(conicData.ratio) || !(conicData.ratio > 0)) { fail(Refusal::UnsupportedCutter); }
        if ((circle && !(circle->getRadius() > 0)) || (arc && !(arc->getRadius() > 0))) { fail(Refusal::UnsupportedCutter); }
    } else { fail(Refusal::UnsupportedCutter); }
    // All aggregate counts precede the first copied span, adapter or callback.
    c.items(4 + sourceCount + (spline ? c.work.inputSpans - sourceCount : line ? 1 : 8));
    auto sourceSpans = std::make_shared<const Spans>(inspect(c, source));
    c.items(sourceSpans->size());
    const auto sourceComponents = components(*sourceSpans);
    if (sourceComponents.empty() || ((source.m_closedFlag || source.m_periodicFlag) && sourceComponents.size() != 1))
        fail(Refusal::UnsupportedLayout);
    const bool closed = sourceComponents.size() == 1 ? closure(source, *sourceSpans) : false;
    std::shared_ptr<const Spans> cutterSpans;
    bool cutterClosed = false;
    std::optional<Conic> conic;
    if (spline) {
        cutterSpans = std::make_shared<const Spans>(inspect(c, spline->getData()));
        cutterClosed = closure(spline->getData(), *cutterSpans);
    } else if (line) {
        const auto data = line->getData();
        if (!Geo::isFinite2D(data.startpoint) || !Geo::isFinite2D(data.endpoint) || same(data.startpoint, data.endpoint)) {
            fail(Refusal::UnsupportedCutter);
        }
        Geo::CacheSpan span; span.degree = 1; span.parameter = I::hull(0, 1);
        span.controls[0] = data.startpoint; span.controls[1] = data.endpoint;
        cutterSpans = std::make_shared<const Spans>(Spans{span});
    } else {
        RS_EllipseData unit;
        unit.center = RS_Vector{0, 0}; unit.majorP = RS_Vector{1, 0}; unit.ratio = 1;
        unit.angle1 = conicData.angle1; unit.angle2 = conicData.angle2; unit.reversed = conicData.reversed;
        unit.isArc = conicData.isArc;
        auto frame = Geo::ReferenceFrame::create(unit, 0);
        if (!frame) { fail(Refusal::UnsupportedCutter); }
        // The native reference utility recognizes near-full turns using its
        // admission policy. A trim cutter must NEVER acquire that missing arc.
        // Only the primitive's canonical whole representation is accepted here.
        if (frame->sourceSweep().whole && !(unit.angle1 == 0 && unit.angle2 == 0)) {
            fail(Refusal::UnsupportedCutter);
        }
        if (frame->sourceSweep().whole) {
            // Trim alone uses a theta=0 seam; native frame defaults stay unchanged.
            frame = frame->withWholeStart(0.);
            if (!frame) fail(Refusal::Arithmetic);
        }
        cutterClosed = frame->sourceSweep().whole;
        conic = Conic{conicData, *frame};
    }
    const auto origin = sourceSpans->front().controls[0];
    long double extent = 0;
    const auto include = [&](const RS_Vector& point) {
        if (!Geo::isFinite2D(point)) { fail(Refusal::InvalidInput); }
        extent = std::max({extent, std::abs(static_cast<long double>(point.x) - origin.x),
                          std::abs(static_cast<long double>(point.y) - origin.y)});
    };
    for (const auto& spans : {sourceSpans, cutterSpans}) {
        if (!spans) { continue; }
        for (const auto& span : *spans) { c.check(); for (std::size_t i = 0; i <= span.degree; ++i) { include(span.controls[i]); } }
    }
    include(pick.worldPoint);
    if (conic) {
        include(conicData.center);
        extent = std::max(extent, std::hypot(static_cast<long double>(conicData.majorP.x),
            static_cast<long double>(conicData.majorP.y)) * std::max(1.L, static_cast<long double>(conicData.ratio)));
    }
    if (!(extent > 0) || extent > std::numeric_limits<double>::max()) { fail(Refusal::Arithmetic); }
    int exponent = 0; std::frexp(static_cast<double>(extent), &exponent);
    const double scale = std::scalbn(1., exponent - 1);
    RS_EllipseData basis; basis.center = origin; basis.majorP = RS_Vector{scale, 0}; basis.ratio = 1;
    auto frame = Geo::ReferenceFrame::create(basis, 0);
    if (!frame) { fail(Refusal::Arithmetic); }
    return {std::move(sourceSpans), std::move(cutterSpans), std::move(conic), closed, cutterClosed, *frame};
}

Location location(const I& t, std::size_t count) {
    if (!finite(t) || t.lo() < 0 || t.hi() > double(count)) { fail(Refusal::AmbiguousRoots); }
    std::size_t span = std::min(count - 1, static_cast<std::size_t>(t.lo()));
    if (t.hi() > double(span + 1)) { fail(Refusal::AmbiguousRoots); }
    if (singleton(t) && (t.lo() == double(span) || t.lo() == double(span + 1))) {
        return {span, I::point(t.lo() - double(span))};
    }
    return {span, intersect(t - I::point(double(span)), I::hull(0, 1))};
}

I global(std::size_t span, const I& u) {
    if (singleton(u) && (u.lo() == 0 || u.lo() == 1)) { return I::point(double(span) + u.lo()); }
    return u + I::point(double(span));
}

// Arithmetic allowance in the common LOCAL frame, independent of world origin
// and the native C tolerance. World endpoint bounds additionally include ULPs.
double arithmeticAllowance(const Geo::CacheSpan& span, const Geo::ReferenceFrame& frame) {
    I x, y, dx, dy;
    if (!rawBound(span, frame, 0, 1, x, y, dx, dy)) { fail(Refusal::Arithmetic); }
    const double value = 512 * std::numeric_limits<double>::epsilon() *
        std::max({x.width(), y.width(), std::abs(dx.lo()), std::abs(dx.hi()), std::abs(dy.lo()), std::abs(dy.hi())});
    if (!std::isfinite(value) || !(value > 0)) { fail(Refusal::Arithmetic); }
    return value;
}

I distance(const Geo::CacheSpan& span, const Geo::ReferenceFrame& frame, const RS_Vector& pick, const I& t) {
    I x, y, dx, dy, px, py;
    if (!finite(t) || !rawBound(span, frame, t.lo(), t.hi(), x, y, dx, dy) || !frame.pointToFrame(pick, px, py)) {
        fail(Refusal::Arithmetic);
    }
    const auto result = sqr(x - px) + sqr(y - py);
    if (!finite(result)) { fail(Refusal::Arithmetic); }
    return result;
}

struct Projection { I parameter; std::size_t span = 0; };

Projection closest(Context& c, const Input& input, const Pick& pick) {
    struct Candidate {
        I parameter, distance;
        std::optional<Q> exactDistance;
        std::optional<RS_Vector> exactEndpoint;
        std::size_t span = 0;
    };
    std::vector<Candidate> candidates;
    const auto& spans = *input.source;
    // Degree five has at most five isolated stationary roots per cubic, plus
    // two ends. Admit the entire candidate store before its first allocation.
    const auto capacity = spans.size() * (spans.front().degree == 1 ? 1 : 7);
    c.items(capacity); candidates.reserve(capacity);
    const auto add = [&](std::size_t span, I u, I d, std::optional<Q> exact) {
        I t = global(span, u);
        if (input.closed && singleton(t) && t.lo() == double(spans.size())) { t = I::point(0); }
        std::optional<RS_Vector> endpoint;
        if (singleton(u) && (u.lo() == 0 || u.lo() == 1))
            endpoint = spans[span].controls[u.lo() == 0 ? 0 : spans[span].degree];
        for (auto& old : candidates) {
            c.compare();
            if (singleton(t) && singleton(old.parameter) && t.lo() == old.parameter.lo() &&
                endpoint && old.exactEndpoint && same(*endpoint, *old.exactEndpoint)) return;
        }
        if (candidates.size() == capacity) { fail(Refusal::LimitExceeded); }
        candidates.push_back({t, d, std::move(exact), endpoint, span});
    };
    for (std::size_t k = 0; k < spans.size(); ++k) {
        c.check(); const auto& span = spans[k];
        if (span.degree == 1) {
            const Q x0{span.controls[0].x}, y0{span.controls[0].y};
            const Q dx = Q(span.controls[1].x) - x0, dy = Q(span.controls[1].y) - y0;
            const Q denom = dx * dx + dy * dy;
            if (denom == 0) { fail(Refusal::AmbiguousPick); }
            Q t = ((Q(pick.worldPoint.x) - x0) * dx + (Q(pick.worldPoint.y) - y0) * dy) / denom;
            if (t < 0) { t = 0; } else if (t > 1) { t = 1; }
            const Q ex = x0 + t * dx - Q(pick.worldPoint.x), ey = y0 + t * dy - Q(pick.worldPoint.y);
            const auto u = enclose(t);
            add(k, u, distance(span, input.frame, pick.worldPoint, u), ex * ex + ey * ey);
            continue;
        }
        for (double t : {0., 1.}) {
            const auto& p = span.controls[t == 0 ? 0 : span.degree];
            const Q dx = Q(p.x) - Q(pick.worldPoint.x), dy = Q(p.y) - Q(pick.worldPoint.y);
            add(k, I::point(t), distance(span, input.frame, pick.worldPoint, I::point(t)), dx * dx + dy * dy);
        }
        c.items(3);
        auto session = query(c, std::make_unique<Stationary>(span, input.frame, pick.worldPoint, c.cancellation), Refusal::AmbiguousPick);
        c.roots(session->rootCount());
        for (std::size_t j = 0; j < session->rootCount(); ++j) {
            c.check(); const auto handle = session->rootHandle(j);
            auto snapshot = session->snapshot(handle);
            if (!snapshot) { fail(Refusal::AmbiguousPick); }
            auto t = snapshot->event.isolation.parameterA;
            for (unsigned round = 0; !singleton(t); ++round) {
                I x, y, dx, dy;
                if (!rawBound(span, input.frame, t.lo(), t.hi(), x, y, dx, dy)) { fail(Refusal::Arithmetic); }
                if (x.width() <= arithmeticAllowance(span, input.frame) && y.width() <= arithmeticAllowance(span, input.frame)) { break; }
                if (round == c.limits.maxDepth) { fail(Refusal::AmbiguousPick); }
                try {
                    snapshot = contract(c, *session, handle, Refusal::AmbiguousPick);
                } catch (const Failure& failure) {
                    if (failure.reason != Refusal::AmbiguousPick || !session->hasValidProofs()) throw;
                    // Keep the last issued isolation. The final interval-distance ordering
                    // must still prove this candidate uniquely closest, or refuse the pick.
                    break;
                }
                t = snapshot->event.isolation.parameterA;
            }
            add(k, t, distance(span, input.frame, pick.worldPoint, t), {});
        }
    }
    if (candidates.empty()) { fail(Refusal::AmbiguousPick); }
    std::size_t winner = 0;
    for (std::size_t k = 1; k < candidates.size(); ++k) {
        c.compare();
        const auto& a = candidates[k]; const auto& b = candidates[winner];
        if (a.exactDistance && b.exactDistance ? *a.exactDistance < *b.exactDistance : a.distance.hi() < b.distance.lo()) { winner = k; }
    }
    for (std::size_t k = 0; k < candidates.size(); ++k) {
        if (k == winner) { continue; }
        c.compare(); const auto& a = candidates[winner]; const auto& b = candidates[k];
        if (!(a.exactDistance && b.exactDistance ? *a.exactDistance < *b.exactDistance : a.distance.hi() < b.distance.lo())) {
            fail(Refusal::AmbiguousPick);
        }
    }
    return {candidates[winner].parameter, candidates[winner].span};
}

struct Root {
    LC_CertifiedRootHandle handle;
    LC_CertifiedRootSnapshot snapshot;
    I parameter;
    double cut = 0;
    std::size_t provenanceId = 0;
    bool lineContracted = false;
};

std::vector<Root> roots(Context& c, const Input& input, const Curves& curves,
                        LC_CertifiedIntersectionSession& session) {
    c.roots(session.rootCount());
    c.items(session.rootCount());
    std::vector<Root> out; out.reserve(session.rootCount());
    for (std::size_t k = 0; k < session.rootCount(); ++k) {
        c.check(); const auto handle = session.rootHandle(k); auto snapshot = session.snapshot(handle);
        if (!snapshot || snapshot->event.branchA != 0 || snapshot->event.branchB != 1) { fail(Refusal::AmbiguousRoots); }
        if (snapshot->event.kind == LC_ParametricIntersection::Kind::Tangent ||
            (snapshot->event.proof != LC_IntersectionProof::IntervalUniqueTransverse &&
             snapshot->event.proof != LC_IntersectionProof::AnalyticEndpoint)) { fail(Refusal::TangencyOrOverlap); }
        auto t = snapshot->event.isolation.parameterA;
        bool lineContracted = false;
        bool scalarContracted = false;
        for (unsigned round = 0;; ++round) {
            const auto id = snapshot->event.isolation.segmentA;
            if (id >= input.source->size()) { fail(Refusal::AmbiguousRoots); }
            const auto domain = I::hull(double(id), double(id + 1));
            const auto local = location(intersect(t, domain), input.source->size());
            if (local.rawSpan != id && !(singleton(t) && t.lo() == double(id + 1))) { fail(Refusal::AmbiguousRoots); }
            const auto u = intersect(t - I::point(double(id)), I::hull(0, 1));
            const auto error = arithmeticAllowance((*input.source)[id], input.frame);
            const auto cutterId = snapshot->event.isolation.segmentB;
            const double localMiddle = mid(u), globalMiddle = double(id) + localMiddle;
            const auto ulp = [](const double value) {
                const double lower = std::nextafter(value, -INFINITY);
                const double upper = std::nextafter(value, INFINITY);
                return std::isfinite(upper) ? std::max(value - lower, upper - value) : value - lower;
            };
            const bool globalParameterLosesLocalPrecision = std::isfinite(globalMiddle) &&
                ulp(globalMiddle) > 128 * ulp(localMiddle);
            const bool lineCutter = globalParameterLosesLocalPrecision && input.cutter &&
                cutterId >= input.source->size() &&
                cutterId - input.source->size() < input.cutter->size() &&
                (*input.cutter)[cutterId - input.source->size()].degree == 1;
            if (singleton(t)) { break; }
            bool lineProgress = false;
            if (lineCutter) {
                const double cut = mid(t);
                const I cutError = cacheCutRootError((*input.source)[id], input.frame, u, cut - double(id));
                if (round < c.limits.maxDepth) {
                    const auto& line = (*input.cutter)[cutterId - input.source->size()];
                    c.items(1);
                    c.compare();
                    const I narrowed = contractCacheRootAgainstLine((*input.source)[id], line, input.frame, u);
                    if (finite(narrowed) && narrowed.width() < u.width()) {
                        t = global(id, narrowed);
                        scalarContracted = lineProgress = true;
                    }
                }
                if (lineProgress) { continue; }
                if (finite(cutError) && cutError.hi() <= error) {
                    lineContracted = scalarContracted;
                    break;
                }
                if (!lineContracted) {
                    I x, y, dx, dy;
                    if (!rawBound((*input.source)[id], input.frame, u.lo(), u.hi(), x, y, dx, dy)) {
                        fail(Refusal::Arithmetic);
                    }
                    if (x.width() <= error && y.width() <= error) { break; }
                }
            } else {
                I x, y, dx, dy;
                if (!rawBound((*input.source)[id], input.frame, u.lo(), u.hi(), x, y, dx, dy)) { fail(Refusal::Arithmetic); }
                if (x.width() <= error && y.width() <= error) { break; }
            }
            if (round == c.limits.maxDepth) { fail(Refusal::AmbiguousRoots); }
            snapshot = contract(c, session, handle, Refusal::AmbiguousRoots);
            t = intersect(snapshot->event.isolation.parameterA, t);
            if (!finite(t)) { fail(Refusal::AmbiguousRoots); }
        }
        if (scalarContracted && !lineContracted) { t = snapshot->event.isolation.parameterA; }
        // A segment endpoint certificate alone does not prove that a CORNER
        // crosses the cutter. All actual one-sided tangent combinations must
        // agree; a V touching a line is not a trimming boundary.
        std::array<Geo::CacheBezierPoint, 2> a, b; std::size_t na = 0, nb = 0;
        const auto& isolation = snapshot->event.isolation;
        if (!curves.incidentDirections(isolation.segmentA, isolation.parameterA, a, na) ||
            !curves.incidentDirections(isolation.segmentB, isolation.parameterB, b, nb)) { fail(Refusal::Arithmetic); }
        int orientation = 0;
        for (std::size_t i = 0; i < na; ++i) for (std::size_t j = 0; j < nb; ++j) {
            c.compare(); const auto determinant = a[i].x * b[j].y - a[i].y * b[j].x;
            if (!finite(determinant) || !determinant.excludesZero()) { fail(Refusal::TangencyOrOverlap); }
            const int sign = determinant.isPositive() ? 1 : -1;
            if (orientation && sign != orientation) { fail(Refusal::TangencyOrOverlap); }
            orientation = sign;
        }
        double cut = mid(t);
        if (!std::isfinite(cut) || !t.contains(cut) ||
            (!singleton(t) && cut == std::floor(cut))) { fail(Refusal::AmbiguousRoots); }
        // Only exact closure transports the certified end occurrence to zero.
        if (input.closed && exactParameter(t, double(input.source->size()))) { t = I::point(0); cut = 0; }
        // Only equal singleton parameter isolations identify the same occurrence.
        bool duplicate = false;
        for (const auto& previous : out) {
            c.compare();
            if (singleton(t) && singleton(previous.parameter) && t.lo() == previous.parameter.lo()) { duplicate = true; break; }
        }
        if (!duplicate) { out.push_back({handle, *snapshot, t, cut, 0, lineContracted}); }
    }
    // Insertion sort is bounded by the shared comparison allowance; never use
    // numeric seed order when actual certified isolations overlap.
    for (std::size_t i = 1; i < out.size(); ++i) {
        std::size_t j = i;
        while (j) {
            c.compare();
            if (out[j - 1].parameter.hi() < out[j].parameter.lo()) { break; }
            if (!(out[j].parameter.hi() < out[j - 1].parameter.lo())) { fail(Refusal::AmbiguousRoots); }
            std::swap(out[j], out[j - 1]); --j;
        }
    }
    if (!session.hasValidProofs()) { fail(Refusal::AmbiguousRoots); }
    return out;
}

struct Boundary { I parameter; double cut; std::optional<std::size_t> root; };
struct Range { Boundary from, to; bool wrap = false; };

std::vector<Range> retain(Context& c, double end, bool closed, const std::vector<Root>& roots,
                          const I& pick, Range& removed) {
    c.items(roots.size() + 4);
    std::vector<Boundary> boundaries; boundaries.reserve(roots.size() + 2);
    if (!closed) { boundaries.push_back({I::point(0), 0, {}}); }
    for (std::size_t k = 0; k < roots.size(); ++k) {
        const auto& root = roots[k];
        // Shared by cache and conic preparation: an uncertain rounded cut
        // cannot acquire a source endpoint identity or erase its complement.
        if (!finite(root.parameter) || root.parameter.lo() < 0 || root.parameter.hi() > end ||
            !std::isfinite(root.cut) || !root.parameter.contains(root.cut) ||
            ((root.cut == 0 || root.cut == end) && !exactParameter(root.parameter, root.cut))) {
            fail(Refusal::AmbiguousRoots);
        }
        if (!closed && exactParameter(root.parameter, 0)) { boundaries.front().root = k; }
        else if (!closed && exactParameter(root.parameter, end)) { continue; }
        else { boundaries.push_back({root.parameter, root.cut, k}); }
    }
    if (!closed) {
        std::optional<std::size_t> last;
        for (std::size_t k = 0; k < roots.size(); ++k) {
            if (exactParameter(roots[k].parameter, end)) { last = k; }
        }
        boundaries.push_back({I::point(end), end, last});
        if (boundaries.size() == 2) { fail(Refusal::NoUsableRoots); }
    } else if (boundaries.size() < 2) { fail(Refusal::NoUsableRoots); }
    std::optional<std::size_t> selected;
    const std::size_t intervals = closed ? boundaries.size() : boundaries.size() - 1;
    for (std::size_t k = 0; k < intervals; ++k) {
        c.compare();
        const auto& a = boundaries[k]; const auto& b = boundaries[(k + 1) % boundaries.size()];
        const bool wrap = closed && k + 1 == boundaries.size();
        const bool inside = wrap ? (pick.lo() > a.parameter.hi() || pick.hi() < b.parameter.lo())
                                 : (pick.lo() > a.parameter.hi() && pick.hi() < b.parameter.lo());
        if (inside) { if (selected) { fail(Refusal::AmbiguousPick); } selected = k; }
    }
    if (!selected) { fail(Refusal::AmbiguousPick); }
    const auto k = *selected;
    removed = {boundaries[k], boundaries[(k + 1) % boundaries.size()], closed && k + 1 == boundaries.size()};
    std::vector<Range> ranges; ranges.reserve(2);
    if (closed) { ranges.push_back({removed.to, removed.from, !removed.wrap}); }
    else {
        if (removed.from.cut > 0) { ranges.push_back({boundaries.front(), removed.from, false}); }
        if (removed.to.cut < end) { ranges.push_back({removed.to, boundaries.back(), false}); }
    }
    if (ranges.empty()) { fail(Refusal::EmptyOutput); }
    if (ranges.size() > c.limits.maxFragments) { fail(Refusal::LimitExceeded); }
    return ranges;
}

I conicDistance(const Input& input, const Pick& pick, const I& t) {
    LC_CurveJetBounds jet; I px, py;
    if (!input.conic || !finite(t) || !conicJet(*input.conic, input.frame, t.lo(), t.hi(), jet) ||
        !input.frame.pointToFrame(pick.worldPoint, px, py)) fail(Refusal::Arithmetic);
    const auto distance = sqr(jet.x - px) + sqr(jet.y - py);
    if (!finite(distance)) fail(Refusal::Arithmetic);
    return distance;
}

I closestConic(Context& c, const Input& input, const Pick& pick) {
    if (!Geo::isFinite2D(pick.worldPoint)) fail(Refusal::InvalidInput);
    c.items(3);
    auto session = query(c, std::make_unique<ConicStationary>(*input.conic, input.frame, pick.worldPoint, c.cancellation), Refusal::AmbiguousPick);
    c.roots(session->rootCount());
    c.items(session->rootCount() + 2);
    struct Candidate { I parameter, distance; };
    std::vector<Candidate> candidates; candidates.reserve(session->rootCount() + 2);
    const double end = input.conic->unit.parameterEnd();
    const auto add = [&](I t) {
        if (input.cutterClosed && singleton(t) && t.lo() == end) t = I::point(0);
        for (const auto& old : candidates) {
            c.compare(); if (singleton(t) && singleton(old.parameter) && t.lo() == old.parameter.lo()) return;
        }
        candidates.push_back({t, conicDistance(input, pick, t)});
    };
    add(I::point(0)); add(I::point(end));
    const double error = conicAllowance(*input.conic, input.frame);
    for (std::size_t i = 0; i < session->rootCount(); ++i) {
        c.check(); const auto handle = session->rootHandle(i); auto snapshot = session->snapshot(handle);
        if (!snapshot || snapshot->event.branchA != 0 || snapshot->event.branchB != 1 ||
            (snapshot->event.proof != LC_IntersectionProof::IntervalUniqueTransverse &&
             snapshot->event.proof != LC_IntersectionProof::AnalyticEndpoint)) fail(Refusal::AmbiguousPick);
        auto t = snapshot->event.isolation.parameterA;
        for (unsigned round = 0;; ++round) {
            LC_CurveJetBounds jet;
            if (!finite(t) || !conicJet(*input.conic, input.frame, t.lo(), t.hi(), jet)) fail(Refusal::Arithmetic);
            if (jet.x.width() <= error && jet.y.width() <= error) break;
            if (round == c.limits.maxDepth) fail(Refusal::AmbiguousPick);
            snapshot = contract(c, *session, handle, Refusal::AmbiguousPick); t = snapshot->event.isolation.parameterA;
        }
        add(t);
    }
    std::size_t winner = 0;
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        c.compare(); if (candidates[i].distance.hi() < candidates[winner].distance.lo()) winner = i;
    }
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (i == winner) continue;
        c.compare(); if (!(candidates[winner].distance.hi() < candidates[i].distance.lo())) fail(Refusal::AmbiguousPick);
    }
    if (!session->hasValidProofs()) fail(Refusal::AmbiguousPick);
    return candidates[winner].parameter;
}

std::vector<Root> conicContacts(Context& c, const Input& input, const std::vector<Root>& contacts,
                                LC_CertifiedIntersectionSession& session) {
    c.items(contacts.size()); std::vector<Root> out; out.reserve(contacts.size());
    const double end = input.conic->unit.parameterEnd(), error = conicAllowance(*input.conic, input.frame);
    for (const auto& contact : contacts) {
        c.check(); auto snapshot = contact.snapshot; I t = snapshot.event.isolation.parameterB;
        for (unsigned round = 0;; ++round) {
            LC_CurveJetBounds jet; I angle;
            if (!finite(t) || t.lo() < 0 || t.hi() > end ||
                !conicJet(*input.conic, input.frame, t.lo(), t.hi(), jet) ||
                !input.conic->unit.boundUnwrappedAngle(t.lo(), t.hi(), angle)) fail(Refusal::Arithmetic);
            // Root isolation already proves existence/uniqueness. Contract to
            // the geometric endpoint allowance; the angle enclosure can reach
            // its outward-rounding floor before that independent bound.
            if (jet.x.width() <= error && jet.y.width() <= error) break;
            if (round == c.limits.maxDepth) fail(Refusal::AmbiguousRoots);
            snapshot = contract(c, session, contact.handle, Refusal::AmbiguousRoots); t = snapshot.event.isolation.parameterB;
        }
        double cut = mid(t);
        if (!std::isfinite(cut) || !t.contains(cut)) fail(Refusal::AmbiguousRoots);
        if (input.cutterClosed && exactParameter(t, end)) { t = I::point(0); cut = 0; }
        bool duplicate = false;
        for (const auto& old : out) {
            c.compare(); if (singleton(t) && singleton(old.parameter) && t.lo() == old.parameter.lo()) duplicate = true;
        }
        if (!duplicate) out.push_back({contact.handle, snapshot, t, cut, contact.provenanceId});
    }
    for (std::size_t i = 1; i < out.size(); ++i) {
        std::size_t j = i;
        while (j) {
            c.compare(); if (out[j - 1].parameter.hi() < out[j].parameter.lo()) break;
            if (!(out[j].parameter.hi() < out[j - 1].parameter.lo())) fail(Refusal::AmbiguousRoots);
            std::swap(out[j], out[j - 1]); --j;
        }
    }
    if (!session.hasValidProofs()) fail(Refusal::AmbiguousRoots);
    return out;
}

bool conicEndpoint(const Conic& original, const Geo::ReferenceFrame& frame, double angle, LC_CurveJetBounds& out) {
    RS_EllipseData unit{{0, 0}, {1, 0}, 1., angle, angle < std::acos(-1.) ? angle + .5 : angle - .5, false};
    const auto reference = Geo::ReferenceFrame::create(unit, 0);
    return reference && conicJet(Conic{original.data, *reference}, frame, 0, 0, out);
}

std::vector<Result::ConicFragment> assembleConic(Context& c, const Input& input,
    const std::vector<Root>& contacts, const std::vector<Range>& ranges) {
    if (ranges.size() > c.limits.maxOutputSpans - c.work.outputSpans) fail(Refusal::LimitExceeded);
    c.items(ranges.size()); c.work.outputSpans += ranges.size();
    std::vector<Result::ConicFragment> output; output.reserve(ranges.size());
    const auto& conic = *input.conic; const auto& unit = conic.unit;
    const double end = unit.parameterEnd(), error = conicAllowance(conic, input.frame);
    const I turn = unit.traversalRate() * I::point(end);
    const auto angle = [&](const Boundary& boundary) {
        if (!input.cutterClosed && exactParameter(boundary.parameter, 0)) return conic.data.angle1;
        if (!input.cutterClosed && exactParameter(boundary.parameter, end)) return conic.data.angle2;
        I theta; if (!unit.boundUnwrappedAngle(boundary.cut, boundary.cut, theta)) fail(Refusal::Arithmetic);
        return RS_Math::correctAngle(mid(theta));
    };
    const auto endpoint = [&](const Boundary& boundary, double theta) {
        LC_CurveJetBounds root, point;
        if (!conicJet(conic, input.frame, boundary.parameter.lo(), boundary.parameter.hi(), root) ||
            !conicEndpoint(conic, input.frame, theta, point)) fail(Refusal::Arithmetic);
        const I dx = point.x - root.x, dy = point.y - root.y;
        if (!finite(dx) || !finite(dy) || dx.lo() < -error || dx.hi() > error || dy.lo() < -error || dy.hi() > error)
            fail(Refusal::Arithmetic);
    };
    for (const auto& range : ranges) {
        c.check(); auto data = conic.data;
        data.setAngle1(angle(range.from)); data.setAngle2(angle(range.to));
        const auto domain = Geo::sweep(data);
        if (!domain || domain->whole || (std::abs(data.angle1) < RS_TOLERANCE_ANGLE &&
            std::abs(data.angle2) < RS_TOLERANCE_ANGLE)) fail(Refusal::Arithmetic);
        RS_EllipseData represented{{0, 0}, {1, 0}, 1., data.angle1, data.angle2, data.reversed};
        const auto check = Geo::ReferenceFrame::create(represented, 0);
        if (!check) fail(Refusal::Arithmetic);
        const I wanted = unit.traversalRate() * (I::point(range.to.cut) - I::point(range.from.cut)) +
            (range.wrap ? turn : I::point(0));
        const I actual = check->traversalRate() * I::point(check->parameterEnd());
        const I difference = actual - wanted;
        const double angleError = 128 * std::numeric_limits<double>::epsilon();
        if (!finite(wanted) || !wanted.isPositive() || !finite(difference) ||
            difference.lo() < -angleError || difference.hi() > angleError) fail(Refusal::Arithmetic);
        endpoint(range.from, data.angle1); endpoint(range.to, data.angle2);
        const auto rootId = [&](const std::optional<std::size_t>& sortedIndex) -> std::optional<std::size_t> {
            if (!sortedIndex) return {};
            if (*sortedIndex >= contacts.size()) fail(Refusal::Arithmetic);
            return contacts[*sortedIndex].provenanceId;
        };
        // Public root IDs remain indices into Result::roots, not conic sort order.
        output.push_back({data, range.from.parameter, range.to.parameter, rootId(range.from.root), rootId(range.to.root), range.wrap});
    }
    return output;
}

std::array<RS_Vector, 4> restrictSpan(const Geo::CacheSpan& span, double a, double b) {
    if (!(0 <= a && a < b && b <= 1)) { fail(Refusal::Arithmetic); }
    if (a == 0 && b == 1) { return span.controls; }
    // Exact rational de Casteljau, rounded ONCE per output control. This is
    // restriction of stored doubles, not a new curve fit or endpoint snap.
    struct Point { Q x, y; };
    std::array<Point, 4> net;
    for (std::size_t i = 0; i <= span.degree; ++i) { net[i] = {Q(span.controls[i].x), Q(span.controls[i].y)}; }
    const auto split = [](std::array<Point, 4> in, std::size_t degree, const Q& t,
                          std::array<Point, 4>& left, std::array<Point, 4>& right) {
        left[0] = in[0]; right[degree] = in[degree];
        for (std::size_t level = 1; level <= degree; ++level) {
            for (std::size_t i = 0; i <= degree - level; ++i) {
                in[i] = {(1 - t) * in[i].x + t * in[i + 1].x, (1 - t) * in[i].y + t * in[i + 1].y};
            }
            left[level] = in[0]; right[degree - level] = in[degree - level];
        }
    };
    std::array<Point, 4> left, right;
    if (b != 1) { split(net, span.degree, Q(b), left, right); net = std::move(left); }
    if (a != 0) { split(net, span.degree, Q(a) / Q(b), left, right); net = std::move(right); }
    std::array<RS_Vector, 4> result;
    for (std::size_t i = 0; i <= span.degree; ++i) {
        result[i] = RS_Vector{net[i].x.convert_to<double>(), net[i].y.convert_to<double>()};
        if (!Geo::isFinite2D(result[i])) { fail(Refusal::Arithmetic); }
    }
    return result;
}

std::size_t sectionCount(double from, double to) {
    return from < to ? static_cast<std::size_t>(std::ceil(to) - std::floor(from)) : 0;
}

Endpoint endpoint(const Input& input, const std::vector<Root>& roots, const Boundary& boundary,
                  const RS_Vector& actual) {
    Endpoint out;
    const double end = double(input.source->size());
    out.source = location(boundary.parameter, input.source->size());
    if (!input.closed && exactParameter(boundary.parameter, 0)) { out.labels |= unsigned(EndLabel::SourceStart); }
    if (!input.closed && exactParameter(boundary.parameter, end)) { out.labels |= unsigned(EndLabel::SourceEnd); }
    I x, y;
    if (!input.frame.pointToFrame(actual, x, y)) { fail(Refusal::Arithmetic); }
    if (boundary.root) {
        const auto& root = roots[*boundary.root];
        out.labels |= unsigned(EndLabel::CutterRoot); out.rootId = boundary.root;
        // Cover root-location uncertainty AND one-rounding subdivision error.
        const auto id = root.snapshot.event.isolation.segmentA;
        const auto t = root.lineContracted ? root.parameter : root.snapshot.event.isolation.parameterA;
        const auto u = intersect(t - I::point(double(id)), I::hull(0, 1));
        I rx, ry, dx, dy;
        const auto& span = (*input.source)[id];
        if (exactParameter(t, double(id)) || exactParameter(t, double(id + 1))) {
            // A certified singleton span endpoint is the stored control itself.
            // Do not widen its exact local 0/1 by interval subtraction/evaluation.
            const auto& control = span.controls[exactParameter(t, double(id)) ? 0 : span.degree];
            if (!input.frame.pointToFrame(control, rx, ry)) { fail(Refusal::Arithmetic); }
        } else if (!rawBound(span, input.frame, u.lo(), u.hi(), rx, ry, dx, dy)) {
            fail(Refusal::Arithmetic);
        }
        const auto allowance = arithmeticAllowance((*input.source)[id], input.frame);
        const auto ulp = [](double value) {
            const double next = std::nextafter(value, INFINITY);
            return std::isfinite(next) ? next - value : value - std::nextafter(value, -INFINITY);
        };
        if (root.lineContracted) {
            const I cutError = cacheCutRootError(span, input.frame, u, boundary.cut - double(id));
            if (!finite(cutError) || cutError.hi() > allowance) {
                fail(Refusal::Arithmetic);
            }
        } else {
            const auto ulp = [](double value) {
                const double lower = std::nextafter(value, -INFINITY);
                const double upper = std::nextafter(value, INFINITY);
                return std::isfinite(upper) ? std::max(value - lower, upper - value) : value - lower;
            };
            const double ulpX = ulp(actual.x) / input.frame.scale(), ulpY = ulp(actual.y) / input.frame.scale();
            if (!std::isfinite(ulpX) || !std::isfinite(ulpY) ||
                I::hull(x, rx).width() > allowance + 4 * ulpX ||
                I::hull(y, ry).width() > allowance + 4 * ulpY) { fail(Refusal::Arithmetic); }
        }
        x = I::hull(x, rx); y = I::hull(y, ry);
    }
    if (!input.frame.pointToWorld(x, y, out.imageX, out.imageY)) { fail(Refusal::Arithmetic); }
    return out;
}

std::vector<Fragment> assemble(Context& c, const Input& input, const std::vector<Root>& roots,
                                const std::vector<Range>& ranges) {
    const double end = double(input.source->size());
    const auto degree = input.source->front().degree;
    std::array<std::size_t, 2> counts{};
    for (std::size_t k = 0; k < ranges.size(); ++k) {
        const auto& r = ranges[k];
        counts[k] = r.wrap ? sectionCount(r.from.cut, end) + sectionCount(0, r.to.cut) : sectionCount(r.from.cut, r.to.cut);
        const auto layout = Geo::cacheLayoutCounts(degree, counts[k]);
        if (!layout) { fail(Refusal::EmptyOutput); }
        if (counts[k] > c.limits.maxOutputSpans - c.work.outputSpans ||
            layout->controls > c.limits.maxOutputControls - c.work.outputControls ||
            layout->knots > c.limits.maxOutputKnots - c.work.outputKnots) { fail(Refusal::LimitExceeded); }
        c.work.outputSpans += counts[k]; c.work.outputControls += layout->controls; c.work.outputKnots += layout->knots;
    }
    c.items(c.work.outputSpans + ranges.size());
    std::vector<Fragment> staged; staged.reserve(ranges.size());
    for (std::size_t k = 0; k < ranges.size(); ++k) {
        c.check(); const auto& range = ranges[k];
        Fragment f; f.data.degree = degree;
        const auto layout = *Geo::cacheLayoutCounts(degree, counts[k]);
        f.data.controlPoints.reserve(layout.controls); f.data.weights.reserve(layout.controls);
        f.data.knotslist.reserve(layout.knots); f.sourceSections.reserve(counts[k]);
        const auto append = [&](double from, double to) {
            while (from < to) {
                c.check(); const auto id = static_cast<std::size_t>(std::floor(from));
                if (id >= input.source->size()) { fail(Refusal::Arithmetic); }
                const double next = std::min(to, double(id + 1));
                const double a = from - double(id), b = next - double(id);
                const auto controls = restrictSpan((*input.source)[id], a, b);
                // Positive source parameter length is not enough: rounding at
                // a large origin can collapse every control to the same point.
                // Refuse even a constant span inside a nonconstant fragment;
                // never silently discard a retained interval or publish a prefix.
                bool nonconstant = false;
                for (std::size_t i = 1; i <= degree; ++i) {
                    c.compare();
                    if (!same(controls[0], controls[i])) { nonconstant = true; }
                }
                if (!nonconstant) { fail(Refusal::Arithmetic); }
                if (!f.data.controlPoints.empty() && !same(f.data.controlPoints.back(), controls[0])) { fail(Refusal::Arithmetic); }
                for (std::size_t i = f.data.controlPoints.empty() ? 0 : 1; i <= degree; ++i) {
                    f.data.controlPoints.push_back(controls[i]); f.data.weights.push_back(1);
                }
                f.sourceSections.push_back({id, a, b, (*input.source)[id].parameter});
                from = next;
            }
        };
        if (range.wrap) { append(range.from.cut, end); append(0, range.to.cut); }
        else { append(range.from.cut, range.to.cut); }
        for (std::size_t j = 0; j <= counts[k]; ++j) {
            const auto multiplicity = j == 0 || j == counts[k] ? degree + 1 : degree;
            for (std::size_t n = 0; n < multiplicity; ++n) { f.data.knotslist.push_back(double(j)); }
        }
        f.data.type = f.data.savedOpenType = RS_SplineData::SplineType::ClampedOpen;
        f.data.m_closedFlag = f.data.m_periodicFlag = false;
        if (f.data.controlPoints.size() != layout.controls || f.data.weights.size() != layout.controls ||
            f.data.knotslist.size() != layout.knots || f.sourceSections.size() != counts[k]) { fail(Refusal::Arithmetic); }
        f.start = endpoint(input, roots, range.from, f.data.controlPoints.front());
        f.end = endpoint(input, roots, range.to, f.data.controlPoints.back());
        staged.push_back(std::move(f));
    }
    return staged;
}

RS_SplineData componentData(const Spans& spans, const Component& component) {
    RS_SplineData data;
    data.degree = spans[component.begin].degree;
    data.type = data.savedOpenType = RS_SplineData::SplineType::ClampedOpen;
    data.m_closedFlag = component.closed;
    for (std::size_t k = component.begin; k < component.end; ++k) {
        const auto& span = spans[k];
        const std::size_t multiplicity = k == component.begin ? span.degree + 1 :
            span.controlOffset - spans[k - 1].controlOffset;
        if (multiplicity != span.degree && multiplicity != span.degree + 1)
            fail(Refusal::UnsupportedLayout);
        const std::size_t first = k == component.begin || multiplicity == span.degree + 1 ? 0 : 1;
        for (std::size_t i = first; i <= span.degree; ++i) {
            data.controlPoints.push_back(span.controls[i]);
            data.weights.push_back(1.0);
        }
        const std::size_t local = k - component.begin;
        for (std::size_t i = 0; i < multiplicity; ++i) data.knotslist.push_back(double(local));
    }
    for (std::size_t i = 0; i <= data.degree; ++i)
        data.knotslist.push_back(double(component.end - component.begin));
    return data;
}

std::vector<Root> rootsAcrossCutterComponents(Context& c, const Input& input,
                                               const std::vector<Component>& parts) {
    std::vector<Root> combined;
    for (const auto& part : parts) {
        c.check();
        Input local = input;
        const auto data = componentData(*input.cutter, part);
        local.cutter = std::make_shared<const Spans>(inspect(c, data));
        local.cutterClosed = part.closed;
        c.items(1);
        auto provider = std::make_unique<Curves>(local.frame, local.source, local.cutter,
            local.conic, local.closed, local.cutterClosed, c.cancellation);
        const auto* view = provider.get();
        auto session = query(c, std::move(provider), Refusal::TangencyOrOverlap);
        auto partRoots = roots(c, local, *view, *session);
        if (!session->hasValidProofs()) fail(Refusal::AmbiguousRoots);
        c.items(partRoots.size());
        for (auto& root : partRoots) {
            const auto shiftCutter = [&](LC_IntersectionDomain& domain) {
                domain.segmentB += part.begin;
                domain.parameterB = domain.parameterB + I::point(double(part.begin));
            };
            shiftCutter(root.snapshot.event.isolation);
            shiftCutter(root.snapshot.event.proofDomain);
            root.snapshot.event.parameterB += double(part.begin);
            combined.push_back(std::move(root));
        }
    }
    for (std::size_t i = 1; i < combined.size(); ++i) {
        std::size_t j = i;
        while (j) {
            c.compare();
            if (combined[j - 1].parameter.hi() < combined[j].parameter.lo()) break;
            if (!(combined[j].parameter.hi() < combined[j - 1].parameter.lo()))
                fail(Refusal::AmbiguousRoots);
            std::swap(combined[j], combined[j - 1]);
            --j;
        }
    }
    return combined;
}

Limits remainingLimits(const Context& c) {
    Limits out = c.limits;
    const auto subtract = [](std::size_t limit, std::size_t used) {
        return used <= limit ? limit - used : std::size_t{0};
    };
    out.maxInputSpans = subtract(out.maxInputSpans, c.work.inputSpans);
    out.maxInputRecords = subtract(out.maxInputRecords, c.work.inputRecords);
    out.maxGeneratedItems = subtract(out.maxGeneratedItems,
        c.work.kernelItems + c.work.intersections.generatedItems);
    out.maxOrderingComparisons = subtract(out.maxOrderingComparisons,
        c.work.kernelComparisons + c.work.intersections.orderingComparisons);
    out.maxRoots = subtract(out.maxRoots, c.work.rootRecords);
    out.maxBoxPairs = subtract(out.maxBoxPairs, c.work.intersections.totalBoxPairs);
    out.maxLeaves = subtract(out.maxLeaves, c.work.intersections.generatedLeaves);
    out.maxOutputSpans = subtract(out.maxOutputSpans, c.work.outputSpans);
    out.maxOutputControls = subtract(out.maxOutputControls, c.work.outputControls);
    out.maxOutputKnots = subtract(out.maxOutputKnots, c.work.outputKnots);
    return out;
}

void addWork(Work& to, const Work& from) {
#define LC_TRIM_WORK_ADD(field) to.field += from.field
    LC_TRIM_WORK_ADD(inputSpans); LC_TRIM_WORK_ADD(inputRecords); LC_TRIM_WORK_ADD(rootRecords);
    LC_TRIM_WORK_ADD(kernelItems); LC_TRIM_WORK_ADD(kernelComparisons);
    LC_TRIM_WORK_ADD(outputSpans); LC_TRIM_WORK_ADD(outputControls); LC_TRIM_WORK_ADD(outputKnots);
#undef LC_TRIM_WORK_ADD
    auto& a = to.intersections; const auto& b = from.intersections;
#define LC_TRIM_INTERSECTION_ADD(field) a.field += b.field
    LC_TRIM_INTERSECTION_ADD(candidateBoxPairs); LC_TRIM_INTERSECTION_ADD(refinementBoxPairs);
    LC_TRIM_INTERSECTION_ADD(certificateBoxPairs); LC_TRIM_INTERSECTION_ADD(totalBoxPairs);
    LC_TRIM_INTERSECTION_ADD(generatedLeaves); LC_TRIM_INTERSECTION_ADD(generatedItems);
    LC_TRIM_INTERSECTION_ADD(orderingComparisons); LC_TRIM_INTERSECTION_ADD(validationSegmentVisits);
    LC_TRIM_INTERSECTION_ADD(validationNextCalls);
#undef LC_TRIM_INTERSECTION_ADD
    a.deepestPreparation = std::max(a.deepestPreparation, b.deepestPreparation);
    a.deepestPairRefinement = std::max(a.deepestPairRefinement, b.deepestPairRefinement);
    a.deepestOrderingRefinement = std::max(a.deepestOrderingRefinement, b.deepestOrderingRefinement);
    a.deepestRootRefinement = std::max(a.deepestRootRefinement, b.deepestRootRefinement);
}

void shift(Location& location, std::size_t offset) { location.rawSpan += offset; }
void constrainSourceLabels(Endpoint& endpoint, std::size_t spanCount) {
    if (endpoint.source.rawSpan != 0 || !exactParameter(endpoint.source.localParameter, 0))
        endpoint.labels &= ~unsigned(EndLabel::SourceStart);
    if (endpoint.source.rawSpan + 1 != spanCount || !exactParameter(endpoint.source.localParameter, 1))
        endpoint.labels &= ~unsigned(EndLabel::SourceEnd);
    if (!(endpoint.labels & (unsigned(EndLabel::SourceStart) | unsigned(EndLabel::SourceEnd) |
                             unsigned(EndLabel::CutterRoot))))
        endpoint.labels |= unsigned(EndLabel::ComponentBoundary);
}
void shift(LC_IntersectionDomain& domain, std::size_t sourceOffset, std::size_t cutterOffset) {
    domain.segmentA += sourceOffset;
    domain.segmentB += cutterOffset;
    domain.parameterA = domain.parameterA + I::point(double(sourceOffset));
}

static Result prepareImpl(const RS_SplineData&, const RS_Entity&, const Pick&,
    const std::optional<Pick>&, bool, const Limits&, Cancellation, Work*);

Result prepareComponents(Context& c, const Input& input, const std::vector<Component>& parts,
                         const RS_Entity& cutter, const Pick& pick,
                         const std::optional<Pick>& conicPick, bool emitCache) {
    if (!emitCache) fail(Refusal::UnsupportedLayout);
    const auto picked = closest(c, input, pick);
    const auto pickedSpan = picked.span;
    const auto selected = std::find_if(parts.begin(), parts.end(), [pickedSpan](const Component& part) {
        return part.begin <= pickedSpan && pickedSpan < part.end;
    });
    if (selected == parts.end()) fail(Refusal::AmbiguousPick);

    const auto selectedCount = selected->end - selected->begin;
    const auto cutterOffset = input.source->size() - selectedCount;
    Pick localPick = pick;
    if (localPick.rawSpanHint) {
        if (*localPick.rawSpanHint < selected->begin || *localPick.rawSpanHint >= selected->end)
            localPick.rawSpanHint.reset();
        else *localPick.rawSpanHint -= selected->begin;
    }
    c.items(selectedCount * (input.source->front().degree + 1) + selectedCount + 2);

    auto prepared = prepareImpl(componentData(*input.source, *selected), cutter, localPick,
        conicPick, true, remainingLimits(c), c.cancellation, nullptr);
    addWork(c.work, prepared.work);
    if (prepared.refusal != Refusal::None) fail(prepared.refusal);

    Result result = std::move(prepared);
    for (auto& fragment : result.fragments) {
        for (auto& section : fragment.sourceSections) {
            section.rawSpan += selected->begin;
            section.originalDomain = (*input.source)[section.rawSpan].parameter;
        }
        shift(fragment.start.source, selected->begin); shift(fragment.end.source, selected->begin);
        constrainSourceLabels(fragment.start, input.source->size());
        constrainSourceLabels(fragment.end, input.source->size());
    }
    if (result.removed) {
        shift(result.removed->from, selected->begin);
        shift(result.removed->to, selected->begin);
    }
    for (auto& root : result.roots) {
        shift(root.target, selected->begin);
        shift(root.isolation, selected->begin, cutterOffset);
        shift(root.proofDomain, selected->begin, cutterOffset);
    }
    for (const auto& part : parts) {
        c.check();
        if (part.begin == selected->begin) continue;
        Input untouched = input;
        untouched.source = std::make_shared<const Spans>(input.source->begin() + part.begin,
                                                          input.source->begin() + part.end);
        untouched.closed = part.closed;
        const auto count = part.end - part.begin;
        const Range all{{I::point(0), 0, {}}, {I::point(double(count)), double(count), {}}, false};
        auto preserved = assemble(c, untouched, {}, {all});
        auto& fragment = preserved.front();
        for (auto& section : fragment.sourceSections) section.rawSpan += part.begin;
        shift(fragment.start.source, part.begin); shift(fragment.end.source, part.begin);
        constrainSourceLabels(fragment.start, input.source->size());
        constrainSourceLabels(fragment.end, input.source->size());
        result.fragments.push_back(std::move(fragment));
    }
    if (result.fragments.size() > c.limits.maxFragments) fail(Refusal::LimitExceeded);
    result.fragmentCount = result.fragments.size();
    result.work = c.work;
    return result;
}

} // namespace

std::optional<MonotoneLineRootCertificate> certifyMonotoneLineRoot(
        const std::vector<RS_Vector>& bezierControlPoints,
        const RS_Vector& lineStart, const RS_Vector& lineEnd) {
    try { return certifyMonotoneLineRootExact(bezierControlPoints, lineStart, lineEnd); }
    catch (...) { return {}; }
}

namespace {
static Result prepareImpl(const RS_SplineData& target, const RS_Entity& cutter, const Pick& pick,
               const std::optional<Pick>& conicPick, bool emitCache,
               const Limits& limits, Cancellation cancellation, Work* onExit) {
    Context c{clamp(limits), std::move(cancellation), {}};
    Result result;
    try {
        c.check();
        if (!c.limits.maxDepth || !c.limits.maxLeafDepth) { fail(Refusal::LimitExceeded); }
        if (conicPick && (!Geo::isFinite2D(conicPick->worldPoint) || conicPick->rawSpanHint || conicPick->localParameterSeed))
            fail(Refusal::InvalidInput);
        if (conicPick && (!c.limits.maxOutputControls || !c.limits.maxOutputKnots)) fail(Refusal::LimitExceeded);
        const auto input = admit(c, target, cutter, pick);
        if (conicPick && !input.conic) fail(Refusal::UnsupportedCutter);
        c.items(input.source->size());
        const auto parts = components(*input.source);
        if (parts.size() > 1 && emitCache) {
            result = prepareComponents(c, input, parts, cutter, pick, conicPick, emitCache);
            result.work = c.work;
            if (onExit) *onExit = result.work;
            return result;
        }
        Range removed;
        std::vector<Fragment> fragments;
        std::vector<Result::ConicFragment> conics;
        std::vector<RootProvenance> provenance;
        const auto cutterParts = input.conic ? std::vector<Component>{} : components(*input.cutter);
        c.items(cutterParts.size());
        if (conicPick && !emitCache && parts.size() > 1) {
            std::vector<Root> ordered;
            for (const auto& part : parts) {
                c.check();
                const auto count = part.end - part.begin;
                c.items(count);
                Input local = input;
                local.source = std::make_shared<const Spans>(input.source->begin() + part.begin,
                                                              input.source->begin() + part.end);
                local.closed = part.closed;
                c.items(1);
                auto provider = std::make_unique<Curves>(local.frame, local.source, local.cutter,
                    local.conic, local.closed, local.cutterClosed, c.cancellation);
                const auto* view = provider.get();
                auto session = query(c, std::move(provider), Refusal::TangencyOrOverlap);
                auto contacts = roots(c, local, *view, *session);
                c.items(contacts.size());
                for (auto& root : contacts) {
                    root.provenanceId = provenance.size();
                    I x, y;
                    if (!local.frame.pointToWorld(root.snapshot.imageX, root.snapshot.imageY, x, y))
                        fail(Refusal::Arithmetic);
                    auto targetLocation = location(root.parameter, count);
                    shift(targetLocation, part.begin);
                    auto isolation = root.snapshot.event.isolation;
                    auto proofDomain = root.snapshot.event.proofDomain;
                    const auto cutterOffset = input.source->size() - count;
                    shift(isolation, part.begin, cutterOffset);
                    shift(proofDomain, part.begin, cutterOffset);
                    provenance.push_back({root.provenanceId, targetLocation, isolation, proofDomain,
                        root.snapshot.event.proof, x, y});
                }
                auto partOrdered = conicContacts(c, local, contacts, *session);
                c.items(partOrdered.size());
                ordered.insert(ordered.end(), partOrdered.begin(), partOrdered.end());
                if (!session->hasValidProofs()) fail(Refusal::AmbiguousRoots);
                result.rootCount += contacts.size();
            }
            if (!result.rootCount) fail(Refusal::NoUsableRoots);
            for (std::size_t i = 1; i < ordered.size(); ++i) {
                std::size_t j = i;
                while (j) {
                    c.compare();
                    if (!(ordered[j].parameter.lo() < ordered[j - 1].parameter.lo())) break;
                    std::swap(ordered[j], ordered[j - 1]); --j;
                }
            }
            std::size_t unique = 0;
            for (std::size_t i = 0; i < ordered.size(); ++i) {
                if (unique) {
                    c.compare();
                    const auto& previous = ordered[unique - 1].parameter;
                    const auto& current = ordered[i].parameter;
                    if (!(previous.hi() < current.lo())) {
                        if (singleton(previous) && singleton(current) && previous.lo() == current.lo()) continue;
                        fail(Refusal::AmbiguousRoots);
                    }
                }
                if (unique != i) ordered[unique] = std::move(ordered[i]);
                ++unique;
            }
            ordered.resize(unique);
            const auto projected = closestConic(c, input, *conicPick);
            Range conicRemoved;
            const auto ranges = retain(c, input.conic->unit.parameterEnd(), input.cutterClosed, ordered, projected, conicRemoved);
            conics = assembleConic(c, input, ordered, ranges);
        } else {
            std::vector<Root> contacts;
            std::unique_ptr<LC_CertifiedIntersectionSession> session;
            if (!conicPick && cutterParts.size() > 1) {
                contacts = rootsAcrossCutterComponents(c, input, cutterParts);
            } else {
                c.items(1);
                auto provider = std::make_unique<Curves>(input.frame, input.source, input.cutter, input.conic,
                    input.closed, input.cutterClosed, c.cancellation);
                const auto* view = provider.get(); // Owned by the session throughout root preparation.
                session = query(c, std::move(provider), Refusal::TangencyOrOverlap);
                contacts = roots(c, input, *view, *session);
            }
            for (std::size_t k = 0; k < contacts.size(); ++k) contacts[k].provenanceId = k;
            result.rootCount = contacts.size();
            if (contacts.empty()) { fail(Refusal::NoUsableRoots); }
            if (emitCache) {
                const auto projected = closest(c, input, pick);
                const auto ranges = retain(c, double(input.source->size()), input.closed, contacts,
                    projected.parameter, removed);
                fragments = assemble(c, input, contacts, ranges);
            }
            if (conicPick) {
                auto ordered = conicContacts(c, input, contacts, *session);
                const auto projected = closestConic(c, input, *conicPick);
                Range conicRemoved;
                const auto ranges = retain(c, input.conic->unit.parameterEnd(), input.cutterClosed,
                    ordered, projected, conicRemoved);
                conics = assembleConic(c, input, ordered, ranges);
            }
            c.items(contacts.size()); provenance.reserve(contacts.size());
            for (std::size_t k = 0; k < contacts.size(); ++k) {
                c.check(); const auto& root = contacts[k];
                I x, y;
                if (!input.frame.pointToWorld(root.snapshot.imageX, root.snapshot.imageY, x, y))
                    fail(Refusal::Arithmetic);
                provenance.push_back({k, location(root.parameter, input.source->size()),
                    root.snapshot.event.isolation, root.snapshot.event.proofDomain,
                    root.snapshot.event.proof, x, y});
            }
            if (session && !session->hasValidProofs()) { fail(Refusal::AmbiguousRoots); }
        }
        c.check();
        if (emitCache) result.removed = RemovedInterval{location(removed.from.parameter, input.source->size()),
            location(removed.to.parameter, input.source->size()), removed.wrap};
        result.conicFragments = std::move(conics);
        result.roots = std::move(provenance); result.fragments = std::move(fragments);
        result.fragmentCount = result.fragments.size(); result.refusal = Refusal::None;
    } catch (const Failure& failure) { result.refusal = failure.reason; }
      catch (const std::bad_alloc&) { result.refusal = Refusal::AllocationFailure; }
      catch (...) { if (onExit) { *onExit = c.work; } throw; }
    if (result.refusal != Refusal::None) { result.fragments.clear(); result.conicFragments.clear(); result.fragmentCount = 0; result.removed.reset(); }
    result.work = c.work;
    if (onExit) { *onExit = c.work; }
    return result;
}
} // namespace

Result prepare(const RS_SplineData& target, const RS_Entity& cutter, const Pick& pick,
               const Limits& limits, Cancellation cancellation, Work* onExit) {
    return prepareImpl(target, cutter, pick, {}, true, limits, std::move(cancellation), onExit);
}

Result prepareConic(const RS_SplineData& cache, const RS_Entity& conic, const Pick& conicPick,
    const std::optional<Pick>& cachePick, const Limits& limits, Cancellation cancellation, Work* onExit) {
    return prepareImpl(cache, conic, cachePick.value_or(conicPick), conicPick, cachePick.has_value(),
        limits, std::move(cancellation), onExit);
}

} // namespace LC_OffsetCacheTrim
