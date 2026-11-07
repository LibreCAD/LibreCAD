/*******************************************************************************
 * This file is part of LibreCAD, distributed under GPL version 2 or later.
 * Copyright (C) 2026 LibreCAD.org
 ******************************************************************************/
#include <catch2/catch_test_macros.hpp>

#include "lc_offsetcachetrim.h"
#include "lc_curveoffset.h"
#include "lc_offsetcurve.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

namespace {
namespace Trim = LC_OffsetCacheTrim;
constexpr const char* tags = "[lc-offsetcurve][trim-kernel]";
constexpr double pi = 3.14159265358979323846;

RS_SplineData cache(std::vector<RS_Vector> points, std::size_t degree = 1,
                    std::vector<double> breaks = {}) {
    RS_SplineData data{int(degree), false};
    data.controlPoints = std::move(points);
    data.weights.assign(data.controlPoints.size(), 1);
    const auto spans = (data.controlPoints.size() - 1) / degree;
    if (breaks.empty()) { for (std::size_t k = 0; k <= spans; ++k) { breaks.push_back(double(k)); } }
    REQUIRE(breaks.size() == spans + 1);
    for (std::size_t k = 0; k <= spans; ++k) {
        for (std::size_t n = 0; n < (k == 0 || k == spans ? degree + 1 : degree); ++n) {
            data.knotslist.push_back(breaks[k]);
        }
    }
    data.m_closedFlag = data.controlPoints.front().x == data.controlPoints.back().x &&
                        data.controlPoints.front().y == data.controlPoints.back().y;
    return data;
}

void reversed(RS_SplineData& data) { std::reverse(data.controlPoints.begin(), data.controlPoints.end()); }
bool label(const Trim::Endpoint& e, Trim::EndLabel bit) { return (e.labels & unsigned(bit)) != 0; }
void near(const RS_Vector& actual, const RS_Vector& expected, double error = 2e-10) {
    CHECK(std::abs(actual.x - expected.x) <= error);
    CHECK(std::abs(actual.y - expected.y) <= error);
    CHECK(actual.z == 0);
}
void refused(const Trim::Result& result) {
    CHECK(result.refusal != Trim::Refusal::None);
    CHECK(result.conicFragments.empty());
    CHECK(result.fragments.empty()); CHECK(result.fragmentCount == 0); CHECK_FALSE(result.removed);
}
void equalData(const RS_SplineData& actual, const RS_SplineData& expected) {
    REQUIRE(actual.controlPoints.size() == expected.controlPoints.size());
    for (std::size_t k = 0; k < actual.controlPoints.size(); ++k) {
        CHECK(actual.controlPoints[k].x == expected.controlPoints[k].x);
        CHECK(actual.controlPoints[k].y == expected.controlPoints[k].y);
        CHECK(actual.controlPoints[k].z == expected.controlPoints[k].z);
        CHECK(actual.controlPoints[k].valid == expected.controlPoints[k].valid);
    }
    CHECK(actual.knotslist == expected.knotslist); CHECK(actual.weights == expected.weights);
    CHECK(actual.degree == expected.degree); CHECK(actual.type == expected.type);
    CHECK(actual.savedOpenType == expected.savedOpenType); CHECK(actual.savedOpenKnots == expected.savedOpenKnots);
    CHECK(actual.fitPoints.size() == expected.fitPoints.size());
    CHECK(actual.m_closedFlag == expected.m_closedFlag); CHECK(actual.m_periodicFlag == expected.m_periodicFlag);
}

// Independent long-double evaluation, with no kernel bounds/restriction calls.
RS_Vector evaluate(const RS_SplineData& data, std::size_t span, long double t) {
    std::array<std::array<long double, 2>, 4> net{};
    for (std::size_t i = 0; i <= data.degree; ++i) {
        const auto& p = data.controlPoints[span * data.degree + i]; net[i] = {p.x, p.y};
    }
    for (std::size_t level = 1; level <= data.degree; ++level) {
        for (std::size_t i = 0; i <= data.degree - level; ++i) {
            for (std::size_t axis = 0; axis < 2; ++axis) { net[i][axis] = (1 - t) * net[i][axis] + t * net[i + 1][axis]; }
        }
    }
    return {double(net[0][0]), double(net[0][1])};
}

void geometry(const RS_SplineData& source, const Trim::Result& result) {
    REQUIRE(result.refusal == Trim::Refusal::None);
    CHECK(result.fragmentCount == result.fragments.size());
    for (const auto& f : result.fragments) {
        CHECK(f.data.type == RS_SplineData::SplineType::ClampedOpen);
        CHECK_FALSE(f.data.m_closedFlag); CHECK_FALSE(f.data.m_periodicFlag);
        CHECK(f.data.fitPoints.empty()); CHECK(f.data.savedOpenKnots.empty());
        REQUIRE(f.data.controlPoints.size() == f.data.degree * f.sourceSections.size() + 1);
        for (const double weight : f.data.weights) { CHECK(weight == 1); }
        for (std::size_t k = 0; k < f.sourceSections.size(); ++k) {
            const auto& section = f.sourceSections[k];
            REQUIRE(section.from < section.to);
            for (int n = 0; n <= 16; ++n) {
                const long double t = n / 16.L;
                near(evaluate(f.data, k, t), evaluate(source, section.rawSpan,
                    section.from + t * (static_cast<long double>(section.to) - section.from)));
            }
        }
    }
}

int coverage(const Trim::Result& result, double originalParameter) {
    int count = 0;
    for (const auto& f : result.fragments) for (const auto& section : f.sourceSections) {
        const double from = double(section.rawSpan) + section.from, to = double(section.rawSpan) + section.to;
        if (from < originalParameter && originalParameter < to) { ++count; }
    }
    return count;
}

void linearComplement(const RS_SplineData& data, const Trim::Result& result, double left, double right) {
    geometry(data, result);
    // The independent cutter equation fixes the removed interval, NOT returned provenance.
    for (int k = 0; k < 401; ++k) {
        const double t = (k + .375) / 401;
        CHECK(coverage(result, t) == int(t < left || t > right));
    }
}

RS_SplineData twoCuts(std::size_t degree = 1, double first = 3, double last = 7) {
    if (degree == 1) { return cache({{first, -2}, {first, 2}, {last, 2}, {last, -2}}); }
    return cache({{first, -2}, {first, -2. / 3}, {first, 2. / 3}, {first, 2},
                  {first + (last - first) / 3, 2}, {last - (last - first) / 3, 2}, {last, 2},
                  {last, 2. / 3}, {last, -2. / 3}, {last, -2}}, 3);
}

std::unique_ptr<LC_OffsetCurve> nativeRing(bool reverse) {
    std::vector<RS_Vector> controls;
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9. / 8}, {9. / 8, 2}, {0, 2}}};
    for (unsigned q = 0; q < 4; ++q) {
        for (unsigned i = q ? 1 : 0; i < 4; ++i) { controls.push_back(net[i]); }
        for (auto& p : net) { p = {-p.y, p.x}; }
    }
    auto data = cache(std::move(controls), 3);
    if (reverse) { reversed(data); }
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = 1;
    definition.relTolerance = .01; definition.cacheReversed = reverse;
    RS_Spline ordinary{nullptr, data};
    LC_OffsetCheckResult check;
    auto result = LC_OffsetCurve::adopt(ordinary, definition, .02, &check);
    REQUIRE(check); REQUIRE(result); REQUIRE(result->cacheFormsClosedChain());
    return result;
}
}

TEST_CASE("Cache trim removes an open end in either stored direction", tags) {
    for (bool reverse : {false, true}) {
        auto data = cache({{0, 0}, {10, 0}}); if (reverse) { reversed(data); }
        const auto before = data;
        RS_Line cutter{nullptr, {4, -2}, {4, 2}};
        const auto result = Trim::prepare(data, cutter, {{1, .25}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        linearComplement(data, result, reverse ? .6 : 0, reverse ? 1 : .4);
        const auto& f = result.fragments.front();
        near(f.data.controlPoints.front(), reverse ? RS_Vector{10, 0} : RS_Vector{4, 0});
        near(f.data.controlPoints.back(), reverse ? RS_Vector{4, 0} : RS_Vector{10, 0});
        CHECK(label(reverse ? f.start : f.end, reverse ? Trim::EndLabel::SourceStart : Trim::EndLabel::SourceEnd));
        CHECK(label(reverse ? f.end : f.start, Trim::EndLabel::CutterRoot));
        equalData(data, before);
    }
}

TEST_CASE("Cache trim retains both complements of a middle interval", tags) {
    for (bool reverse : {false, true}) for (bool cutterReverse : {false, true}) {
        auto data = cache({{0, 0}, {10, 0}}); if (reverse) { reversed(data); }
        auto limit = twoCuts(); if (cutterReverse) { reversed(limit); }
        RS_Spline cutter{nullptr, limit}; const auto before = cutter.getData();
        const auto result = Trim::prepare(data, cutter, {{5, .5}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
        CHECK(result.rootCount == 2); linearComplement(data, result, .3, .7);
        CHECK(label(result.fragments[0].start, Trim::EndLabel::SourceStart));
        CHECK(label(result.fragments[0].end, Trim::EndLabel::CutterRoot));
        CHECK(label(result.fragments[1].start, Trim::EndLabel::CutterRoot));
        CHECK(label(result.fragments[1].end, Trim::EndLabel::SourceEnd));
        equalData(cutter.getData(), before);
    }
}

TEST_CASE("Cache trim restricts an actual cubic without fitting", tags) {
    for (bool reverse : {false, true}) {
        auto data = cache({{0, 0}, {2, 1}, {4, 1}, {6, 0}}, 3); if (reverse) { reversed(data); }
        RS_Line cutter{nullptr, {3, -2}, {3, 2}};
        const auto result = Trim::prepare(data, cutter, {{1.5, .5625}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        linearComplement(data, result, reverse ? .5 : 0, reverse ? 1 : .5);
        const auto& f = result.fragments.front();
        near(reverse ? f.data.controlPoints.back() : f.data.controlPoints.front(), {3, .75});
    }
}

TEST_CASE("Cache trim keeps a certified construction knot and exact uncut controls", tags) {
    const auto data = cache({{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 1}, {5, 1}, {6, 0}}, 3);
    RS_Line cutter{nullptr, {3, -2}, {3, 2}};
    const auto result = Trim::prepare(data, cutter, {{1, 0}});
    REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
    CHECK(result.rootCount == 1); geometry(data, result);
    const auto& f = result.fragments.front(); REQUIRE(f.data.controlPoints.size() == 4);
    for (std::size_t i = 0; i < 4; ++i) { near(f.data.controlPoints[i], data.controlPoints[i + 3], 0); }
    CHECK(f.sourceSections.front().rawSpan == 1); CHECK(f.sourceSections.front().from == 0);
}

TEST_CASE("Cache trim ignores the magnitude of the original affine knot domain", tags) {
    for (const auto domain : {std::array<double, 2>{0, 1e-200}, std::array<double, 2>{-1e200, 1e200}}) {
        const auto data = cache({{0, 0}, {2, 1}, {4, 1}, {6, 0}}, 3, {domain[0], domain[1]});
        RS_Line cutter{nullptr, {3, -2}, {3, 2}};
        const auto result = Trim::prepare(data, cutter, {{1.5, .5625}});
        REQUIRE(result.refusal == Trim::Refusal::None); geometry(data, result);
        const auto& f = result.fragments.front(); CHECK(f.data.knotslist.front() == 0); CHECK(f.data.knotslist.back() == 1);
        CHECK(f.sourceSections.front().originalDomain.lo() == domain[0]);
        CHECK(f.sourceSections.front().originalDomain.hi() == domain[1]);
    }
}

TEST_CASE("Closed cache trim retains exactly one open complement through either seam direction", tags) {
    for (bool reverse : {false, true}) for (double side : {-1., 1.}) {
        auto data = cache({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}});
        if (reverse) { reversed(data); }
        RS_Line cutter{nullptr, {0, -3}, {0, 3}};
        const auto result = Trim::prepare(data, cutter, {{2 * side, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        CHECK(result.rootCount == 2); geometry(data, result);
        const auto& f = result.fragments.front(); CHECK(f.sourceSections.size() == 3);
        CHECK(f.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(f.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        for (int n = 0; n < 400; ++n) {
            const double t = (n + .375) / 100.;
            const auto p = evaluate(data, std::size_t(t), t - std::floor(t));
            CHECK(coverage(result, t) == int(p.x * side < 0));
        }
    }
}

TEST_CASE("Closed cache trim keeps many distinct circle roots without merging corners", tags) {
    const auto data = cache({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}});
    RS_Circle cutter{nullptr, {{0, 0}, 2.25}};
    const auto result = Trim::prepare(data, cutter, {{2, 0}});
    REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
    CHECK(result.rootCount == 8); geometry(data, result);
    const double h = std::sqrt(2.25 * 2.25 - 4);
    for (int n = 0; n < 400; ++n) {
        const double t = (n + .375) / 100.; const auto p = evaluate(data, std::size_t(t), t - std::floor(t));
        CHECK(coverage(result, t) == int(!(p.x == 2 && std::abs(p.y) < h)));
    }
}

TEST_CASE("Closed cache trim canonicalizes a proved root at the old storage seam", tags) {
    for (bool reverse : {false, true}) {
        auto data = cache({{0, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}, {0, -2}});
        if (reverse) { reversed(data); }
        RS_Line cutter{nullptr, {0, -3}, {0, 3}};
        const auto result = Trim::prepare(data, cutter, {{2, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        CHECK(result.rootCount == 2); geometry(data, result);
        const auto& f = result.fragments.front();
        CHECK(f.start.rootId != f.end.rootId); CHECK(f.sourceSections.size() == 3);
        near(reverse ? f.data.controlPoints.front() : f.data.controlPoints.back(), {0, -2});
    }
}

TEST_CASE("Closed seam identity accepts the same proved nondyadic line occurrence in both directions", tags) {
    for (bool reverse : {false, true}) for (bool cutterReverse : {false, true}) for (double extent : {3., 7.}) {
        CAPTURE(reverse, cutterReverse, extent);
        auto data = cache({{0, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}, {0, -2}});
        if (reverse) reversed(data);
        const auto before = data;
        RS_Line cutter{nullptr, {0, cutterReverse ? extent : -extent}, {0, cutterReverse ? -extent : extent}};
        const auto result = Trim::prepare(data, cutter, {{2, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        CHECK(result.rootCount == 2); geometry(data, result); equalData(data, before);
        const auto& f = result.fragments.front(); REQUIRE(f.sourceSections.size() == 3);
        CHECK(f.start.rootId != f.end.rootId);
        CHECK(f.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(f.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        near(reverse ? f.data.controlPoints.front() : f.data.controlPoints.back(), {0, -2}, 0);
        for (int n = 0; n < 500; ++n) {
            const double t = (n + .375) / 100.;
            const auto p = evaluate(data, std::size_t(t), t - std::floor(t));
            CHECK(coverage(result, t) == int(p.x < 0));
        }
    }
}

TEST_CASE("Closed seam nondyadic endpoint identity does not turn a touching corner into a crossing", tags) {
    for (bool reverse : {false, true}) for (bool cutterReverse : {false, true}) {
        CAPTURE(reverse, cutterReverse);
        auto data = cache({{0, -2}, {2, -1}, {3, -2}, {2, -3}, {0, -2}});
        if (reverse) reversed(data);
        const auto before = data;
        RS_Line cutter{nullptr, {0, cutterReverse ? 3. : -3.}, {0, cutterReverse ? -3. : 3.}};
        const auto result = Trim::prepare(data, cutter, {{3, -2}});
        refused(result); CHECK(result.refusal == Trim::Refusal::TangencyOrOverlap); equalData(data, before);
    }
}

TEST_CASE("Endpoint identity refuses distinct exact line parameters inside the same floating enclosure", tags) {
    using Rational = boost::multiprecision::cpp_rational;
    for (bool reverse : {false, true}) {
        const double nearby = std::nextafter(1., 2.);
        const Rational first = (Rational(1) + 3) / 6, second = (Rational(nearby) + 3) / 6;
        const double lo = 0x1.5555555555555p-1, hi = 0x1.5555555555556p-1;
        REQUIRE(first != second); REQUIRE(std::nextafter(lo, INFINITY) == hi);
        REQUIRE((Rational(lo) < first && first < Rational(hi)));
        REQUIRE((Rational(lo) < second && second < Rational(hi)));
        auto data = cache({{0, 1}, {2, 0}, {0, nearby}});
        if (reverse) reversed(data);
        const auto before = data;
        // The two exact cutter parameters are 2/3 and 2/3 + 2^-52/6.
        // Their binary64 outward enclosures coincide; the roots do not.
        RS_Line cutter{nullptr, {0, -3}, {0, 3}};
        const auto result = Trim::prepare(data, cutter, {{2, 0}});
        // The bounded query cannot separate these distinct occurrences under
        // the default depth/work caps. It must not merge their equal enclosures.
        refused(result); CHECK(result.refusal == Trim::Refusal::LimitExceeded);
        CHECK(result.roots.empty()); CHECK(result.rootCount == 0); equalData(data, before);
    }
}

TEST_CASE("Cache trim never extends a finite line cutter", tags) {
    const auto data = cache({{0, 0}, {10, 0}});
    RS_Line cutter{nullptr, {4, 1}, {4, 3}};
    const auto result = Trim::prepare(data, cutter, {{1, 0}});
    refused(result); CHECK(result.refusal == Trim::Refusal::NoUsableRoots);
}

TEST_CASE("Cache trim supports both directed arc cutters", tags) {
    const auto data = cache({{-3, 1}, {3, 1}});
    for (bool reverse : {false, true}) {
        RS_Arc cutter{nullptr, {{0, 0}, 2, reverse ? pi : 0, reverse ? 0 : pi, reverse}};
        const auto result = Trim::prepare(data, cutter, {{0, 1}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
        const double root = std::sqrt(3.);
        linearComplement(data, result, (3 - root) / 6, (3 + root) / 6);
    }
}

TEST_CASE("Cache trim supports an exact circle cutter rather than a display polygon", tags) {
    const auto data = cache({{-3, .5}, {3, .5}});
    RS_Circle cutter{nullptr, {{0, 0}, 2}};
    const auto result = Trim::prepare(data, cutter, {{0, .5}});
    REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
    const double root = std::sqrt(3.75);
    linearComplement(data, result, (3 - root) / 6, (3 + root) / 6);
    near(result.fragments.front().data.controlPoints.back(), {-root, .5});
}

TEST_CASE("Cache trim supports rotated ellipses including ratio greater than one", tags) {
    const auto data = cache({{-3, 0}, {3, 0}});
    for (double ratio : {.5, 2.}) {
        RS_EllipseData d; d.center = {0, 0}; d.majorP = {2, 1}; d.ratio = ratio;
        RS_Ellipse cutter{nullptr, d};
        const auto result = Trim::prepare(data, cutter, {{0, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
        const double root = ratio * 5 / std::sqrt(1 + 4 * ratio * ratio);
        linearComplement(data, result, (3 - root) / 6, (3 + root) / 6);
    }
}

TEST_CASE("Cache trim supports a normalized cubic spline cutter", tags) {
    const auto data = cache({{0, 0}, {10, 0}});
    for (bool reverse : {false, true}) {
        auto limit = twoCuts(3); if (reverse) { reversed(limit); }
        RS_Spline cutter{nullptr, limit};
        const auto result = Trim::prepare(data, cutter, {{5, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); CHECK(result.rootCount == 2);
        linearComplement(data, result, .3, .7);
    }
}

TEST_CASE("Cache trim reads an actually adopted native cutter without editing its cache or children", tags) {
    for (bool reverse : {false, true}) {
        const auto native = nativeRing(reverse); const auto before = native->getData();
        std::vector<const RS_Entity*> children;
        for (auto* child : *native) { children.push_back(child); }
        const auto data = cache({{-3, .3}, {3, .3}});
        const auto result = Trim::prepare(data, *native, {{0, .3}});
        REQUIRE(result.refusal == Trim::Refusal::None); CHECK(result.rootCount == 2);
        REQUIRE(result.fragments.size() == 2); geometry(data, result); equalData(native->getData(), before);
        std::size_t k = 0; for (auto* child : *native) { REQUIRE(k < children.size()); CHECK(child == children[k++]); }
        CHECK(k == children.size()); CHECK(native->definition().cacheReversed == reverse);
    }
}

TEST_CASE("Open endpoint-only roots are not unchanged trim successes", tags) {
    const auto data = cache({{0, 0}, {10, 0}});
    RS_Line cutter{nullptr, {0, -1}, {0, 1}};
    const auto result = Trim::prepare(data, cutter, {{5, 0}});
    refused(result); CHECK(result.refusal == Trim::Refusal::NoUsableRoots);
}

TEST_CASE("Preserved source ends can carry both source and cutter-root labels", tags) {
    const auto data = cache({{0, 0}, {10, 0}});
    RS_Spline cutter{nullptr, cache({{0, -2}, {0, 2}, {3, 2}, {3, -2}, {7, -2}, {7, 2}})};
    const auto result = Trim::prepare(data, cutter, {{5, 0}});
    REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
    const auto& start = result.fragments.front().start;
    CHECK(label(start, Trim::EndLabel::SourceStart)); CHECK(label(start, Trim::EndLabel::CutterRoot));
    CHECK(start.rootId); linearComplement(data, result, .3, .7);
    const auto& end = result.fragments.back().end;
    CHECK(label(end, Trim::EndLabel::SourceEnd)); CHECK_FALSE(label(end, Trim::EndLabel::CutterRoot));
    near(result.fragments.back().data.controlPoints.back(), {10, 0});
}

TEST_CASE("Cache trim refuses tangent and overlapping cutters without partial fragments", tags) {
    const auto data = cache({{0, 0}, {10, 0}});
    RS_Circle tangent{nullptr, {{5, 1}, 1}}; RS_Line overlap{nullptr, {3, 0}, {7, 0}};
    for (const RS_Entity* cutter : {static_cast<const RS_Entity*>(&tangent), static_cast<const RS_Entity*>(&overlap)}) {
        const auto result = Trim::prepare(data, *cutter, {{1, 0}}); refused(result);
        CHECK((result.refusal == Trim::Refusal::TangencyOrOverlap || result.refusal == Trim::Refusal::LimitExceeded));
    }
}

TEST_CASE("Cache trim refuses tied closest occurrences and picks on a cut boundary", tags) {
    RS_Line cutter{nullptr, {5, -1}, {5, 3}};
    for (bool reverse : {false, true}) {
        auto data = cache({{0, 0}, {10, 0}, {10, 2}, {0, 2}}); if (reverse) { reversed(data); }
        const auto result = Trim::prepare(data, cutter, {{2, 1}});
        refused(result); CHECK(result.refusal == Trim::Refusal::AmbiguousPick);
    }
    const auto result = Trim::prepare(cache({{0, 0}, {10, 0}}), cutter, {{5, 0}});
    refused(result); CHECK(result.refusal == Trim::Refusal::AmbiguousPick);
}

TEST_CASE("Cache trim preserves close distinct parameter roots", tags) {
    constexpr double a = .5, b = .5 + 0x1p-30;
    const auto data = cache({{0, 0}, {1, 0}});
    RS_Spline cutter{nullptr, twoCuts(1, a, b)};
    const auto result = Trim::prepare(data, cutter, {{(a + b) / 2, 0}});
    REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
    REQUIRE(result.roots.size() == 2); CHECK(result.rootCount == 2);
    CHECK(result.roots[0].target.localParameter.hi() < result.roots[1].target.localParameter.lo());
    near(result.fragments[0].data.controlPoints.back(), {a, 0}, 1e-12);
    near(result.fragments[1].data.controlPoints.front(), {b, 0}, 1e-12);
}

TEST_CASE("Subnormal certified roots never merge into source endpoints or lose complements", tags) {
    const double tiny = std::numeric_limits<double>::denorm_min();
    REQUIRE(tiny > 0);
    for (const bool reverse : {false, true}) for (const bool cutterReverse : {false, true}) {
        CAPTURE(reverse, cutterReverse);
        auto data = cache({{0, 0}, {1, 0}});
        if (reverse) { reversed(data); }
        const auto before = data;
        // The tiny crossing is a real stored cutter knot on the target line.
        // The provider can certify it exactly, without a numeric root seed.
        auto limit = cache({{tiny, -1}, {tiny, 0}, {tiny, 1}, {.5, 1}, {.5, 0}, {.5, -1}});
        if (cutterReverse) { reversed(limit); }
        RS_Spline cutter{nullptr, limit};
        const auto cutterBefore = cutter.getData();
        const auto result = Trim::prepare(data, cutter, {{.25, 0}});
        equalData(data, before); equalData(cutter.getData(), cutterBefore);
        if (result.refusal != Trim::Refusal::None) {
            refused(result); CHECK(result.roots.empty());
            CHECK((result.refusal == Trim::Refusal::Arithmetic || result.refusal == Trim::Refusal::AmbiguousRoots ||
                   result.refusal == Trim::Refusal::AmbiguousPick || result.refusal == Trim::Refusal::TangencyOrOverlap ||
                   result.refusal == Trim::Refusal::LimitExceeded));
            continue; // Unrepresentable/uncertain cuts must refuse the COMPLETE result.
        }
        REQUIRE(result.fragments.size() == 2); REQUIRE(result.roots.size() == 2);
        CHECK(result.fragmentCount == 2); CHECK(result.rootCount == 2);
        const auto& first = result.fragments.front(); const auto& last = result.fragments.back();
        CHECK(first.start.labels == unsigned(Trim::EndLabel::SourceStart));
        CHECK(first.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(last.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(last.end.labels == unsigned(Trim::EndLabel::SourceEnd));
        CHECK_FALSE(first.start.rootId); CHECK(first.end.rootId);
        CHECK(last.start.rootId); CHECK_FALSE(last.end.rootId);
        const auto& small = reverse ? last : first;
        REQUIRE(small.data.controlPoints.size() == 2); REQUIRE(small.sourceSections.size() == 1);
        CHECK(small.data.controlPoints.front().x == (reverse ? tiny : 0));
        CHECK(small.data.controlPoints.back().x == (reverse ? 0 : tiny));
        CHECK(small.data.controlPoints.front().y == 0); CHECK(small.data.controlPoints.back().y == 0);
        CHECK(small.data.controlPoints.front().x != small.data.controlPoints.back().x);
        CHECK(small.sourceSections.front().from < small.sourceSections.front().to);
        const auto& cut = reverse ? small.start : small.end;
        REQUIRE(cut.rootId);
        const auto& root = result.roots.at(*cut.rootId);
        CHECK(root.worldX.contains(tiny));
        const double parameter = reverse ? small.sourceSections.front().from : small.sourceSections.front().to;
        CHECK(root.isolation.parameterA.contains(parameter));
        if (!reverse) {
            CHECK(parameter == tiny);
            CHECK(root.isolation.parameterA.lo() == tiny);
            CHECK(root.isolation.parameterA.hi() == tiny);
        }
        for (const auto& fragment : result.fragments) {
            REQUIRE(fragment.data.controlPoints.size() == 2);
            CHECK(fragment.data.controlPoints.front().x != fragment.data.controlPoints.back().x);
        }
    }
}

TEST_CASE("Rounded constant retained spans refuse all cache-trim output", tags) {
    constexpr double origin = 0x1p52;
    for (unsigned fixture = 0; fixture < 3; ++fixture)
        for (const bool reverse : {false, true}) for (const bool cutterReverse : {false, true}) {
        CAPTURE(fixture, reverse, cutterReverse);
        auto data = fixture == 1
            ? cache({{origin - 2, origin}, {origin, origin}, {origin + 1, origin + 1}})
            : cache({{origin, origin}, {origin + (fixture == 2 ? 4 : 1), origin + (fixture == 2 ? 4 : 1)}});
        if (reverse) { reversed(data); }
        const auto before = data;
        std::unique_ptr<RS_Entity> cutter;
        if (fixture == 2) {
            auto limit = cache({{origin, origin + 1}, {origin + 1, origin - 2},
                {origin + 2, origin - 2}, {origin + 2, origin + 2}, {origin + 2, origin + 3}});
            if (cutterReverse) { reversed(limit); }
            cutter = std::make_unique<RS_Spline>(nullptr, limit);
        } else {
            RS_Vector from{origin, origin + 1}, to{origin + 1, origin - 2};
            if (cutterReverse) { std::swap(from, to); }
            cutter = std::make_unique<RS_Line>(nullptr, from, to);
        }
        const auto cutterStart = cutter->getStartpoint(), cutterEnd = cutter->getEndpoint();
        const auto* spline = dynamic_cast<const RS_Spline*>(cutter.get());
        RS_SplineData cutterBefore;
        if (spline) { cutterBefore = spline->getData(); }
        // The first intersection is (origin+.25, origin+.25). Its exact
        // retained subspan rounds to (origin,origin) at BOTH output controls.
        // fixture 1 embeds that constant span in a nonconstant fragment;
        // fixture 2/reverse stages the other, nonempty fragment before refusal.
        const RS_Vector pick = fixture == 2 ? RS_Vector{origin + 1, origin + 1} : RS_Vector{origin, origin + 1};
        const auto result = Trim::prepare(data, *cutter, {pick});
        refused(result); CHECK(result.refusal == Trim::Refusal::Arithmetic);
        CHECK(result.roots.empty()); CHECK(result.rootCount == (fixture == 2 ? 2 : 1));
        CHECK(result.work.outputSpans == (fixture == 0 ? 1 : 2));
        CHECK(result.work.outputControls == (fixture == 0 ? 2 : fixture == 1 ? 3 : 4));
        CHECK(result.work.outputKnots == (fixture == 0 ? 4 : fixture == 1 ? 5 : 8));
        equalData(data, before);
        if (spline) { equalData(spline->getData(), cutterBefore); }
        else { near(cutter->getStartpoint(), cutterStart, 0); near(cutter->getEndpoint(), cutterEnd, 0); }
    }
}

TEST_CASE("Exact singleton source and cutter endpoints keep their controls and labels", tags) {
    for (const bool reverse : {false, true}) for (const bool cutterReverse : {false, true}) {
        CAPTURE(reverse, cutterReverse);
        auto data = cache({{0, 0}, {1, 0}});
        if (reverse) { reversed(data); }
        const auto before = data;
        auto limit = cache({{0, -1}, {0, 0}, {0, 1}, {.25, 1}, {.25, 0}, {.25, -1},
                            {.75, -1}, {.75, 0}, {.75, 1}, {1, 1}, {1, 0}, {1, -1}});
        if (cutterReverse) { reversed(limit); }
        RS_Spline cutter{nullptr, limit}; const auto cutterBefore = cutter.getData();
        const auto result = Trim::prepare(data, cutter, {{.5, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
        REQUIRE(result.roots.size() == 4); CHECK(result.rootCount == 4);
        linearComplement(data, result, .25, .75);
        const std::array<double, 4> parameters{{0, .25, .75, 1}};
        for (std::size_t k = 0; k < parameters.size(); ++k) {
            CHECK(result.roots[k].isolation.parameterA.lo() == parameters[k]);
            CHECK(result.roots[k].isolation.parameterA.hi() == parameters[k]);
        }
        const auto& first = result.fragments.front(); const auto& last = result.fragments.back();
        CHECK(first.start.labels == (unsigned(Trim::EndLabel::SourceStart) | unsigned(Trim::EndLabel::CutterRoot)));
        CHECK(first.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(last.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(last.end.labels == (unsigned(Trim::EndLabel::SourceEnd) | unsigned(Trim::EndLabel::CutterRoot)));
        const std::array<const Trim::Endpoint*, 4> endpoints{{&first.start, &first.end, &last.start, &last.end}};
        const std::array<RS_Vector, 4> expected{{{reverse ? 1. : 0., 0}, {reverse ? .75 : .25, 0},
                                               {reverse ? .25 : .75, 0}, {reverse ? 0. : 1., 0}}};
        const std::array<const RS_Vector*, 4> controls{{&first.data.controlPoints.front(), &first.data.controlPoints.back(),
                                                       &last.data.controlPoints.front(), &last.data.controlPoints.back()}};
        for (std::size_t k = 0; k < endpoints.size(); ++k) {
            REQUIRE(endpoints[k]->rootId); CHECK(*endpoints[k]->rootId == k);
            CHECK(controls[k]->x == expected[k].x); CHECK(controls[k]->y == expected[k].y);
            CHECK(endpoints[k]->imageX.contains(expected[k].x)); CHECK(endpoints[k]->imageY.contains(expected[k].y));
        }
        equalData(data, before); equalData(cutter.getData(), cutterBefore);
    }
}

TEST_CASE("Paired conic preparation refuses collapsed polynomial complements in both cache orders", tags) {
    constexpr double origin = 0x1p52;
    for (const bool reverse : {false, true}) {
        CAPTURE(reverse);
        auto data = cache({{origin, origin}, {origin + 4, origin + 4}});
        if (reverse) { reversed(data); }
        const auto before = data;
        RS_Circle conic{nullptr, {{origin + 1, origin + 1}, 1}};
        // The two exact circle contacts are at coordinate offsets
        // 1 +/- 1/sqrt(2). The first retained complement rounds to one point.
        // Reverse storage stages the other nonempty fragment before refusal.
        const auto result = Trim::prepareConic(data, conic,
            {{origin + 1, origin + 2}}, Trim::Pick{{origin + 1, origin + 1}});
        refused(result); CHECK(result.refusal == Trim::Refusal::Arithmetic);
        CHECK(result.roots.empty()); CHECK(result.rootCount == 2);
        CHECK(result.work.outputSpans == 2); CHECK(result.work.outputControls == 4);
        CHECK(result.work.outputKnots == 8);
        equalData(data, before);
        CHECK(conic.getCenter().x == origin + 1); CHECK(conic.getCenter().y == origin + 1);
        CHECK(conic.getRadius() == 1);
    }
}

TEST_CASE("Conic source endpoints retain exact original angles and certified root identities", tags) {
    const auto source = cache({{-3, .5}, {3, .5}}); const auto before = source;
    for (const bool ellipse : {false, true}) for (const bool reverse : {false, true}) {
        CAPTURE(ellipse, reverse);
        const double ratio = ellipse ? .75 : 1.;
        const double start = reverse ? pi : 0, finish = reverse ? 0 : pi;
        std::unique_ptr<RS_Entity> conic;
        if (ellipse) conic = std::make_unique<RS_Ellipse>(nullptr,
            RS_EllipseData{{0, 0}, {2, 0}, ratio, start, finish, reverse});
        else conic = std::make_unique<RS_Arc>(nullptr, RS_ArcData{{0, 0}, 2, start, finish, reverse});
        const auto result = Trim::prepareConic(source, *conic, {{0, 2*ratio}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.conicFragments.size() == 2);
        CHECK(result.fragments.empty()); CHECK(result.fragmentCount == 0); CHECK_FALSE(result.removed);
        REQUIRE(result.roots.size() == 2);
        const auto& first = result.conicFragments.front(); const auto& last = result.conicFragments.back();
        CHECK(first.from.lo() == 0); CHECK(first.from.hi() == 0);
        CHECK(last.to.lo() == pi); CHECK(last.to.hi() == pi);
        CHECK(first.data.angle1 == start); CHECK(last.data.angle2 == finish);
        CHECK_FALSE(first.startRoot); CHECK_FALSE(last.endRoot);
        REQUIRE(first.endRoot); REQUIRE(last.startRoot);
        CHECK(*first.endRoot != *last.startRoot);
        for (const auto& fragment : result.conicFragments) {
            CHECK(fragment.data.center.x == 0); CHECK(fragment.data.center.y == 0);
            CHECK(fragment.data.majorP.x == 2); CHECK(fragment.data.majorP.y == 0);
            CHECK(fragment.data.ratio == ratio); CHECK(fragment.data.reversed == reverse);
            const auto insideProof = [&](const LC_Interval& parameter, const std::optional<std::size_t>& id) {
                if (!id) return;
                REQUIRE(*id < result.roots.size());
                const auto& isolation = result.roots[*id].isolation.parameterB;
                CHECK(parameter.lo() >= isolation.lo()); CHECK(parameter.hi() <= isolation.hi());
            };
            insideProof(fragment.from, fragment.startRoot); insideProof(fragment.to, fragment.endRoot);
        }
        equalData(source, before);
        if (ellipse) {
            const auto& original = static_cast<const RS_Ellipse&>(*conic).getData();
            CHECK(original.angle1 == start); CHECK(original.angle2 == finish); CHECK(original.reversed == reverse);
        } else {
            const auto& original = static_cast<const RS_Arc&>(*conic).getData();
            CHECK(original.angle1 == start); CHECK(original.angle2 == finish); CHECK(original.reversed == reverse);
        }
    }
}

TEST_CASE("Trim whole reference chart zero preserves periodic bounds and native defaults", tags) {
    for (const bool reverse : {false, true}) {
        RS_EllipseData data{{0, 0}, {1, 0}, 1., 0., 0., reverse};
        const auto original = LC_EllipseOffsetGeometry::ReferenceFrame::create(data, -.25);
        REQUIRE(original); REQUIRE(original->sourceSweep().whole);
        const double oldStart = original->sourceSweep().start;
        REQUIRE(oldStart == .25*pi);
        const auto shifted = original->withWholeStart(0.);
        REQUIRE(shifted);
        CHECK(shifted->sourceSweep().start == 0.);
        CHECK(original->sourceSweep().start == oldStart);
        CHECK(shifted->parameterEnd() == original->parameterEnd());
        CHECK(shifted->scale() == original->scale());
        CHECK(shifted->sourceSweep().signedSweep == original->sourceSweep().signedSweep);
        CHECK(shifted->traversalRate().lo() == original->traversalRate().lo());
        CHECK(shifted->traversalRate().hi() == original->traversalRate().hi());
        CHECK(shifted->engineSignedDistance() == original->engineSignedDistance());
        LC_Interval firstAngle;
        REQUIRE(shifted->boundUnwrappedAngle(0., 0., firstAngle));
        CHECK(firstAngle.lo() == 0.); CHECK(firstAngle.hi() == 0.);
        LC_CurveJetBounds first, last, offsetFirst, offsetLast;
        REQUIRE(shifted->boundSourceJet(0., 0., first));
        REQUIRE(shifted->boundSourceJet(shifted->parameterEnd(), shifted->parameterEnd(), last));
        REQUIRE(shifted->boundOffsetJet(0., 0., offsetFirst));
        REQUIRE(shifted->boundOffsetJet(shifted->parameterEnd(), shifted->parameterEnd(), offsetLast));
        CHECK(first.x.contains(1.)); CHECK(first.y.contains(0.));
        CHECK(first.dx.contains(0.)); CHECK(first.ddx.hi() < 0.);
        CHECK(offsetFirst.x.contains(.75)); CHECK(offsetFirst.y.contains(0.));
        if (reverse) CHECK(first.dy.hi() < 0.);
        else CHECK(first.dy.lo() > 0.);
        const auto periodic = [](const LC_CurveJetBounds& a, const LC_CurveJetBounds& b) {
            const std::array<LC_Interval, 8> left{{a.x, a.y, a.dx, a.dy, a.ddx, a.ddy,
                a.speedSquaredProduct, a.crossProduct}};
            const std::array<LC_Interval, 8> right{{b.x, b.y, b.dx, b.dy, b.ddx, b.ddy,
                b.speedSquaredProduct, b.crossProduct}};
            for (std::size_t i = 0; i < left.size(); ++i) {
                CHECK(left[i].lo() == right[i].lo()); CHECK(left[i].hi() == right[i].hi());
            }
        };
        periodic(first, last); periodic(offsetFirst, offsetLast);
        LC_CurveJetBounds oldFirst;
        REQUIRE(original->boundSourceJet(0., 0., oldFirst));
        CHECK(oldFirst.x.lo() > 0.); CHECK(oldFirst.y.lo() > 0.);
        CHECK(original->sourceSweep().start == oldStart);
        const auto recreated = LC_EllipseOffsetGeometry::ReferenceFrame::create(data, -.25);
        REQUIRE(recreated); CHECK(recreated->sourceSweep().start == oldStart);
    }
}

TEST_CASE("Trim whole reference chart copies refuse open and noncanonical phases", tags) {
    using Frame = LC_EllipseOffsetGeometry::ReferenceFrame;
    const auto open = Frame::create(RS_EllipseData{{0, 0}, {2, 0}, .5, .1, .8, false}, 0.);
    REQUIRE(open); CHECK_FALSE(open->withWholeStart(0.)); CHECK_FALSE(open->withWholeStart(.3));
    for (const bool reverse : {false, true}) {
        const auto whole = Frame::create(RS_EllipseData{{0, 0}, {2, 0}, .5, 0., 0., reverse}, 0.);
        REQUIRE(whole);
        const double turn = 2*pi, infinity = std::numeric_limits<double>::infinity();
        for (const double invalid : {-.1, std::nextafter(0., -infinity), turn,
                std::nextafter(turn, infinity), infinity, -infinity,
                std::numeric_limits<double>::quiet_NaN()}) {
            CHECK_FALSE(whole->withWholeStart(invalid));
            CHECK(whole->sourceSweep().start == .25*pi);
        }
        for (const double phase : {0., .3, pi, std::nextafter(turn, 0.)}) {
            const auto shifted = whole->withWholeStart(phase);
            REQUIRE(shifted); CHECK(shifted->sourceSweep().start == phase);
            LC_Interval angle;
            REQUIRE(shifted->boundUnwrappedAngle(0., 0., angle));
            CHECK(angle.lo() == phase); CHECK(angle.hi() == phase);
            LC_CurveJetBounds first, last;
            REQUIRE(shifted->boundSourceJet(0., 0., first));
            REQUIRE(shifted->boundSourceJet(shifted->parameterEnd(), shifted->parameterEnd(), last));
            CHECK(first.x.lo() == last.x.lo()); CHECK(first.x.hi() == last.x.hi());
            CHECK(first.y.lo() == last.y.lo()); CHECK(first.y.hi() == last.y.hi());
            CHECK(whole->sourceSweep().start == .25*pi);
        }
    }
}

TEST_CASE("Cache trim rejects unsupported raw layouts and inconsistent closure", tags) {
    RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    const auto valid = cache({{0, 0}, {10, 0}});
    for (int mutation = 0; mutation < 8; ++mutation) {
        auto data = valid;
        if (mutation == 0) { data.weights[0] = .5; }
        if (mutation == 1) { data.degree = 2; }
        if (mutation == 2) { data.fitPoints.push_back({1, 0}); }
        if (mutation == 3) { data.savedOpenKnots.push_back(0); }
        if (mutation == 4) { data.m_closedFlag = true; }
        if (mutation == 5) { data.m_periodicFlag = true; }
        if (mutation == 6) { data.controlPoints[0].z = 1; }
        if (mutation == 7) { data.knotslist[1] = .1; }
        refused(Trim::prepare(data, cutter, {{1, 0}}));
    }
    RS_Spline rational{nullptr, valid}; rational.getData().weights[0] = .5;
    const auto before = rational.getData(); refused(Trim::prepare(valid, rational, {{1, 0}})); equalData(rational.getData(), before);
}

TEST_CASE("Cache trim rejects nonfinite picks and invalid occurrence seeds", tags) {
    const auto data = cache({{0, 0}, {10, 0}}); RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    for (double value : {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        const auto result = Trim::prepare(data, cutter, {{value, 0}}); refused(result);
        CHECK(result.work.inputSpans == 0);
    }
    refused(Trim::prepare(data, cutter, {{1, 0}, 1, .5}));
    refused(Trim::prepare(data, cutter, {{1, 0}, 0, 1.5}));
    const auto plain = Trim::prepare(data, cutter, {{1, 0}});
    const auto hinted = Trim::prepare(data, cutter, {{1, 0}, 0, .9});
    REQUIRE(plain.refusal == Trim::Refusal::None); REQUIRE(hinted.refusal == Trim::Refusal::None);
    equalData(plain.fragments.front().data, hinted.fragments.front().data);
}

TEST_CASE("Every zero cache-trim allowance is exhausted rather than unlimited", tags) {
    using Field = std::size_t Trim::Limits::*;
    const std::array<Field, 12> fields{{&Trim::Limits::maxSpansPerCurve, &Trim::Limits::maxInputSpans,
        &Trim::Limits::maxInputRecords, &Trim::Limits::maxBoxPairs, &Trim::Limits::maxLeaves,
        &Trim::Limits::maxGeneratedItems, &Trim::Limits::maxOrderingComparisons, &Trim::Limits::maxRoots,
        &Trim::Limits::maxFragments, &Trim::Limits::maxOutputSpans, &Trim::Limits::maxOutputControls,
        &Trim::Limits::maxOutputKnots}};
    const auto data = cache({{0, 0}, {10, 0}}); RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    for (auto field : fields) {
        Trim::Limits limits; limits.*field = 0;
        const auto result = Trim::prepare(data, cutter, {{1, 0}}, limits); refused(result);
        CHECK(result.refusal == Trim::Refusal::LimitExceeded);
    }
    for (bool leaf : {false, true}) {
        Trim::Limits limits; if (leaf) { limits.maxLeafDepth = 0; } else { limits.maxDepth = 0; }
        const auto result = Trim::prepare(data, cutter, {{1, 0}}, limits); refused(result);
        CHECK(result.refusal == Trim::Refusal::LimitExceeded);
    }
}

TEST_CASE("Cache trim preflights aggregate input counts and clamps raised limits", tags) {
    std::vector<RS_Vector> points(4098);
    for (std::size_t k = 0; k < points.size(); ++k) { points[k] = {double(k), 0}; }
    const auto oversized = cache(std::move(points)); RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    Trim::Limits limits; limits.maxSpansPerCurve = limits.maxInputSpans = limits.maxInputRecords = std::numeric_limits<std::size_t>::max();
    const auto result = Trim::prepare(oversized, cutter, {{1, 0}}, limits); refused(result);
    CHECK(result.refusal == Trim::Refusal::LimitExceeded); CHECK(result.work.intersections.validationSegmentVisits == 0);
    CHECK(result.work.kernelItems == 0); CHECK(result.work.inputRecords == 0);
    const auto data = cache({{0, 0}, {10, 0}}); RS_Spline spline{nullptr, twoCuts()};
    limits = {}; limits.maxInputSpans = 3; // target 1 + cutter 3, neither copied yet
    const auto aggregate = Trim::prepare(data, spline, {{5, 0}}, limits); refused(aggregate);
    CHECK(aggregate.refusal == Trim::Refusal::LimitExceeded); CHECK(aggregate.work.kernelItems == 0);
    limits = {}; limits.maxGeneratedItems = 0;
    const auto noStore = Trim::prepare(data, cutter, {{1, 0}}, limits); refused(noStore);
    CHECK(noStore.work.intersections.validationSegmentVisits == 0); CHECK(noStore.work.kernelItems == 0);
}

TEST_CASE("Cache trim admits exact output counts before allocating either fragment", tags) {
    const auto data = cache({{0, 0}, {10, 0}}); RS_Spline cutter{nullptr, twoCuts()};
    Trim::Limits exact; exact.maxFragments = 2; exact.maxOutputSpans = 2;
    exact.maxOutputControls = 4; exact.maxOutputKnots = 8;
    const auto positive = Trim::prepare(data, cutter, {{5, 0}}, exact);
    REQUIRE(positive.refusal == Trim::Refusal::None); CHECK(positive.work.outputControls == 4);
    using Field = std::size_t Trim::Limits::*;
    for (auto field : {Field{&Trim::Limits::maxFragments}, Field{&Trim::Limits::maxOutputSpans},
                       Field{&Trim::Limits::maxOutputControls}, Field{&Trim::Limits::maxOutputKnots}}) {
        auto limits = exact; --(limits.*field);
        const auto result = Trim::prepare(data, cutter, {{5, 0}}, limits); refused(result);
        CHECK(result.refusal == Trim::Refusal::LimitExceeded);
    }
}

TEST_CASE("Cache trim shares pair item and comparison work across all roots and phases", tags) {
    const auto data = cache({{0, 0}, {10, 0}}); RS_Spline cutter{nullptr, twoCuts()};
    Trim::Work observed;
    const auto baseline = Trim::prepare(data, cutter, {{5, 0}}, {}, {}, &observed);
    REQUIRE(baseline.refusal == Trim::Refusal::None);
    CHECK(observed.intersections.totalBoxPairs == baseline.work.intersections.totalBoxPairs);
    for (int resource = 0; resource < 4; ++resource) {
        Trim::Limits exact;
        if (resource == 0) { exact.maxBoxPairs = observed.intersections.totalBoxPairs; }
        if (resource == 1) { exact.maxGeneratedItems = observed.kernelItems + observed.intersections.generatedItems; }
        if (resource == 2) { exact.maxOrderingComparisons = observed.kernelComparisons + observed.intersections.orderingComparisons; }
        if (resource == 3) { exact.maxRoots = observed.rootRecords; }
        REQUIRE(Trim::prepare(data, cutter, {{5, 0}}, exact).refusal == Trim::Refusal::None);
        if (resource == 0) { --exact.maxBoxPairs; }
        if (resource == 1) { --exact.maxGeneratedItems; }
        if (resource == 2) { --exact.maxOrderingComparisons; }
        if (resource == 3) { --exact.maxRoots; }
        const auto result = Trim::prepare(data, cutter, {{5, 0}}, exact); refused(result);
        CHECK(result.refusal == Trim::Refusal::LimitExceeded);
    }
}

TEST_CASE("Cache trim cancellation before preparation publishes nothing and spends no work", tags) {
    const auto data = cache({{0, 0}, {10, 0}}); const auto before = data;
    RS_Line cutter{nullptr, {4, -2}, {4, 2}};
    auto flag = std::make_shared<std::atomic_bool>(true); Trim::Work observed;
    const auto result = Trim::prepare(data, cutter, {{1, 0}}, {}, {flag}, &observed);
    refused(result); CHECK(result.refusal == Trim::Refusal::Cancelled);
    CHECK(observed.inputSpans == 0); CHECK(observed.kernelItems == 0); CHECK(observed.intersections.totalBoxPairs == 0);
    equalData(data, before);
    flag->store(false);
    REQUIRE(Trim::prepare(data, cutter, {{1, 0}}, {}, {flag}).refusal == Trim::Refusal::None);
}

TEST_CASE("Cache trim refuses unsupported containers and leaves aliasing inputs unchanged", tags) {
    const auto data = cache({{0, 0}, {10, 0}}); RS_EntityContainer container{nullptr};
    const auto unsupported = Trim::prepare(data, container, {{1, 0}}); refused(unsupported);
    CHECK(unsupported.refusal == Trim::Refusal::UnsupportedCutter);
    RS_Spline same{nullptr, data}; const auto before = same.getData();
    const auto result = Trim::prepare(same.getData(), same, {{1, 0}}); refused(result); equalData(same.getData(), before);
}

TEST_CASE("Cache trim prepares independent connected components separately", tags) {
    RS_Line cutter{nullptr, {4, -2}, {4, 4}};
    for (double y : {0., 2.}) {
        const auto data = cache({{0, y}, {10, y}});
        const auto result = Trim::prepare(data, cutter, {{1, y}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        linearComplement(data, result, 0, .4);
        for (const auto& p : result.fragments.front().data.controlPoints) { CHECK(p.y == y); }
    }
}

TEST_CASE("A V-shaped cache corner touching a line is not a crossing", tags) {
    for (bool reverse : {false, true}) {
        auto data = cache({{0, 1}, {1, 0}, {2, 1}}); if (reverse) { reversed(data); }
        RS_Line cutter{nullptr, {.5, 0}, {1.5, 0}};
        const auto result = Trim::prepare(data, cutter, {{.25, .75}}); refused(result);
        CHECK(result.refusal == Trim::Refusal::TangencyOrOverlap);
    }
}

TEST_CASE("Cache trim does not complete a near-full conic cutter or its missing half", tags) {
    const auto data = cache({{-3, -1}, {3, -1}});
    RS_Arc upper{nullptr, {{0, 0}, 2, 0, pi, false}};
    const auto missing = Trim::prepare(data, upper, {{0, -1}}); refused(missing);
    CHECK(missing.refusal == Trim::Refusal::NoUsableRoots);
    RS_EllipseData d; d.center = {0, 0}; d.majorP = {2, 0}; d.ratio = 1;
    d.angle2 = std::nextafter(2 * pi, 0.);
    RS_Ellipse nearFull{nullptr, d};
    const auto result = Trim::prepare(data, nearFull, {{0, -1}}); refused(result);
    CHECK(result.refusal == Trim::Refusal::UnsupportedCutter);
}

TEST_CASE("Raised cache-trim limits cannot raise any fixed ceiling", tags) {
    Trim::Limits limits;
    using Field = std::size_t Trim::Limits::*;
    for (auto field : {Field{&Trim::Limits::maxSpansPerCurve}, Field{&Trim::Limits::maxInputSpans},
                       Field{&Trim::Limits::maxInputRecords}, Field{&Trim::Limits::maxBoxPairs},
                       Field{&Trim::Limits::maxLeaves}, Field{&Trim::Limits::maxGeneratedItems},
                       Field{&Trim::Limits::maxOrderingComparisons}, Field{&Trim::Limits::maxRoots},
                       Field{&Trim::Limits::maxFragments}, Field{&Trim::Limits::maxOutputSpans},
                       Field{&Trim::Limits::maxOutputControls}, Field{&Trim::Limits::maxOutputKnots}}) {
        limits.*field = std::numeric_limits<std::size_t>::max();
    }
    limits.maxDepth = limits.maxLeafDepth = std::numeric_limits<unsigned>::max();
    const auto data = cache({{0, 0}, {10, 0}}); RS_Spline cutter{nullptr, twoCuts()};
    const auto baseline = Trim::prepare(data, cutter, {{5, 0}});
    const auto raised = Trim::prepare(data, cutter, {{5, 0}}, limits);
    REQUIRE(baseline.refusal == Trim::Refusal::None); REQUIRE(raised.refusal == Trim::Refusal::None);
    REQUIRE(raised.fragments.size() == baseline.fragments.size());
    for (std::size_t k = 0; k < raised.fragments.size(); ++k) { equalData(raised.fragments[k].data, baseline.fragments[k].data); }
    CHECK(raised.work.kernelItems == baseline.work.kernelItems);
    CHECK(raised.work.intersections.totalBoxPairs == baseline.work.intersections.totalBoxPairs);
    CHECK(raised.work.intersections.generatedItems == baseline.work.intersections.generatedItems);
    CHECK(raised.work.intersections.deepest() <= 48);
}

TEST_CASE("Cubic closest-occurrence ties and source self-crossings are explicit refusals", tags) {
    const auto data = cache({{0, 0}, {3, 0}, {7, 0}, {10, 0}, {10, .5}, {10, 1.5}, {10, 2},
                             {7, 2}, {3, 2}, {0, 2}}, 3);
    RS_Line cutter{nullptr, {5, -1}, {5, 3}};
    const auto tied = Trim::prepare(data, cutter, {{2, 1}}); refused(tied);
    CHECK(tied.refusal == Trim::Refusal::AmbiguousPick);
    const auto crossing = cache({{0, 0}, {2, 2}, {0, 2}, {2, 0}});
    refused(Trim::prepare(crossing, cutter, {{.5, .5}}));
}

TEST_CASE("Unproved conic boundary contacts and malformed conics never become trim roots", tags) {
    const auto data = cache({{0, 0}, {3, 0}});
    RS_Arc upper{nullptr, {{0, 0}, 2, 0, pi, false}};
    // (2,0) is exactly the finite arc's end. No approximate endpoint-on-line
    // certificate is supplied: strict full-dimensional inclusion cannot prove it.
    const auto boundary = Trim::prepare(data, upper, {{1, 0}}); refused(boundary);
    CHECK((boundary.refusal == Trim::Refusal::TangencyOrOverlap || boundary.refusal == Trim::Refusal::LimitExceeded));
    RS_Circle zero{nullptr, {{0, 0}, 0}};
    const auto degenerate = Trim::prepare(data, zero, {{1, 0}}); refused(degenerate);
    CHECK(degenerate.refusal == Trim::Refusal::UnsupportedCutter);
    RS_EllipseData d; d.center = {0, 0}; d.majorP = {2, 0, 1}; d.ratio = .5;
    RS_Ellipse spatial{nullptr, d};
    const auto nonplanar = Trim::prepare(data, spatial, {{1, 0}}); refused(nonplanar);
    CHECK(nonplanar.refusal == Trim::Refusal::UnsupportedCutter);
}

TEST_CASE("Near-tangent roots are either separated by proof or refused without output", tags) {
    constexpr double delta = 0x1p-40;
    const auto data = cache({{-2, 0}, {2, 0}});
    RS_Circle cutter{nullptr, {{0, 1 - delta}, 1}};
    const auto result = Trim::prepare(data, cutter, {{0, 0}});
    if (result.refusal == Trim::Refusal::None) {
        REQUIRE(result.roots.size() == 2); REQUIRE(result.fragments.size() == 2);
        CHECK(result.roots[0].target.localParameter.hi() < result.roots[1].target.localParameter.lo());
        const double root = std::sqrt(2 * delta - delta * delta);
        near(result.fragments[0].data.controlPoints.back(), {-root, 0});
        near(result.fragments[1].data.controlPoints.front(), {root, 0});
        geometry(data, result);
    } else {
        refused(result);
        CHECK((result.refusal == Trim::Refusal::TangencyOrOverlap || result.refusal == Trim::Refusal::AmbiguousRoots ||
               result.refusal == Trim::Refusal::LimitExceeded || result.refusal == Trim::Refusal::Arithmetic));
    }
}

TEST_CASE("An actually admitted native cache trims to its retained cubic cap in both orders", tags) {
    for (bool reverse : {false, true}) {
        const auto native = nativeRing(reverse); const auto before = native->getData();
        RS_Line cutter{nullptr, {-3, .3}, {3, .3}};
        const auto result = Trim::prepare(native->getData(), cutter, {{2, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        CHECK(result.rootCount == 2); geometry(before, result); equalData(native->getData(), before);
        const auto& f = result.fragments.front();
        CHECK(f.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(f.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        for (int n = 0; n < 400; ++n) {
            const double t = (n + .375) / 100.; const auto p = evaluate(before, std::size_t(t), t - std::floor(t));
            CHECK(coverage(result, t) == int(p.y > .3));
        }
    }
}

namespace {

// Same fixed producer input as C's "cache topology selects separate paths from
// two component producer". No fitted substitute, candidate search or C token.
std::array<std::unique_ptr<LC_OffsetCurve>, 2> producedNativeComponents(bool reverse) {
    const RS_EllipseData basis{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false};
    const RS_Ellipse source{nullptr, basis};
    auto options = LC_CurveOffset::makeDirectOptions(source, 4, 1e-4);
    options.mode = LC_CurveOffsetMode::Trimmed;
    auto made = LC_CurveOffset::createEntities(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 4),
        options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(made.entities.size() == 2);
    std::array<std::unique_ptr<LC_OffsetCurve>, 2> result;
    for (const auto& entity : made.entities) {
        const auto* spline = dynamic_cast<const RS_Spline*>(entity.get());
        REQUIRE(spline != nullptr);
        auto data = spline->getData();
        REQUIRE_FALSE(data.controlPoints.empty());
        const bool closed = data.controlPoints.front().x == data.controlPoints.back().x &&
                            data.controlPoints.front().y == data.controlPoints.back().y;
        const std::size_t component = closed ? 1 : 0;
        REQUIRE_FALSE(result[component]);
        // Preserve every produced array and flag; only reverse stored controls
        // for the opposite-order case, with the matching real C definition.
        if (reverse) { reversed(data); }
        LC_EllipseOffsetDefinition definition;
        definition.basis = basis; definition.distance = 4; definition.towardCentre = true;
        definition.relTolerance = .01; definition.cacheReversed = reverse;
        RS_Spline candidate{nullptr, data};
        LC_OffsetCheckResult check;
        result[component] = LC_OffsetCurve::adopt(candidate, definition, .001, &check);
        CAPTURE(component, reverse, check.reason);
        REQUIRE(check);
        REQUIRE(check.analysis);
        REQUIRE(check.analysis->reference().edges().size() == 3);
        REQUIRE(result[component]);
        REQUIRE(result[component]->cacheFormsClosedChain() == closed);
        equalData(result[component]->getData(), data);
    }
    REQUIRE(result[0]); REQUIRE(result[1]);
    // This is the real producer convention, not a test-side flag repair.
    REQUIRE_FALSE(result[1]->getData().m_closedFlag);
    return result;
}

long double componentCoordinate(const RS_SplineData& data, std::size_t span,
                                 long double t, unsigned axis) {
    std::array<long double, 4> net{};
    for (std::size_t i = 0; i <= data.degree; ++i) {
        const auto& p = data.controlPoints[span * data.degree + i];
        net[i] = axis == 0 ? p.x : p.y;
    }
    for (std::size_t n = data.degree; n != 0; --n) {
        for (std::size_t i = 0; i < n; ++i) { net[i] = (1 - t) * net[i] + t * net[i + 1]; }
    }
    return net[0];
}

struct ComponentCut {
    double x = 0, lowY = 0, highY = 0;
    std::vector<long double> parameters, ordinates;
};

// Independent one-dimensional Bezier oracle. A convex hull excludes a span,
// or monotone scalar controls and opposite endpoint signs prove one crossing.
// An uncertain fixture fails here; it is never skipped or delegated to T.
ComponentCut componentCut(const RS_SplineData& data) {
    REQUIRE((data.degree == 1 || data.degree == 3));
    const auto spans = (data.controlPoints.size() - 1) / data.degree;
    REQUIRE(spans > 0); REQUIRE(spans <= 4096);
    long double minX = data.controlPoints.front().x, maxX = minX;
    long double minY = data.controlPoints.front().y, maxY = minY;
    for (const auto& p : data.controlPoints) {
        minX = std::min(minX, static_cast<long double>(p.x));
        maxX = std::max(maxX, static_cast<long double>(p.x));
        minY = std::min(minY, static_cast<long double>(p.y));
        maxY = std::max(maxY, static_cast<long double>(p.y));
    }
    REQUIRE(minX < maxX);
    ComponentCut out;
    out.x = double(minX + (maxX - minX) * (95.L / 256));
    const long double margin = std::max(1.L, maxY - minY);
    out.lowY = double(minY - margin); out.highY = double(maxY + margin);
    REQUIRE(std::isfinite(out.x)); REQUIRE(std::isfinite(out.lowY)); REQUIRE(std::isfinite(out.highY));
    REQUIRE(out.lowY < minY); REQUIRE(out.highY > maxY);
    out.parameters.reserve(spans); out.ordinates.reserve(spans);
    for (std::size_t span = 0; span < spans; ++span) {
        const auto base = span * data.degree;
        long double lo = data.controlPoints[base].x, hi = lo;
        for (std::size_t i = 1; i <= data.degree; ++i) {
            lo = std::min(lo, static_cast<long double>(data.controlPoints[base + i].x));
            hi = std::max(hi, static_cast<long double>(data.controlPoints[base + i].x));
        }
        if (hi < out.x || lo > out.x) { continue; }
        const long double first = data.controlPoints[base].x, last = data.controlPoints[base + data.degree].x;
        CAPTURE(span, first, last, out.x);
        REQUIRE(((first < out.x && out.x < last) || (last < out.x && out.x < first)));
        const bool increasing = first < last;
        for (std::size_t i = 1; i <= data.degree; ++i) {
            const double previous = data.controlPoints[base + i - 1].x;
            const double current = data.controlPoints[base + i].x;
            REQUIRE((increasing ? previous <= current : current <= previous));
        }
        long double a = 0, b = 1;
        for (unsigned step = 0; step < 80; ++step) {
            const long double t = (a + b) / 2;
            const long double x = componentCoordinate(data, span, t, 0);
            if ((x < out.x) == increasing) { a = t; } else { b = t; }
        }
        const long double t = (a + b) / 2;
        out.parameters.push_back(span + t);
        out.ordinates.push_back(componentCoordinate(data, span, t, 1));
    }
    REQUIRE_FALSE(out.parameters.empty());
    return out;
}

RS_Vector componentPoint(const RS_SplineData& data, long double parameter) {
    const auto span = std::size_t(std::floor(parameter));
    REQUIRE(span * data.degree + data.degree < data.controlPoints.size());
    return evaluate(data, span, parameter - span);
}

void componentComplement(const RS_SplineData& source, const Trim::Result& result,
                           long double left, long double right, bool closed) {
    geometry(source, result);
    const auto spans = (source.controlPoints.size() - 1) / source.degree;
    REQUIRE(result.removed);
    CHECK_FALSE(result.removed->crossesStorageSeam);
    REQUIRE(result.fragments.size() == (closed || left == 0 ? 1u : 2u));
    // Every original span is sampled, independent of T's removed/retained ranges.
    for (std::size_t span = 0; span < spans; ++span) for (unsigned n = 0; n < 9; ++n) {
        const long double t = span + (n + .375L) / 9;
        CHECK(coverage(result, double(t)) == int(t < left || t > right));
    }
    const auto checkCut = [&](const Trim::Endpoint& endpoint, const RS_Vector& actual,
                               long double parameter) {
        REQUIRE(label(endpoint, Trim::EndLabel::CutterRoot)); REQUIRE(endpoint.rootId);
        const auto expected = componentPoint(source, parameter);
        near(actual, expected, 1e-7);
        const auto error = [](double value) { return 128 * std::numeric_limits<double>::epsilon() * std::max(1., std::abs(value)); };
        CHECK(endpoint.imageX.lo() <= expected.x + error(expected.x));
        CHECK(endpoint.imageX.hi() >= expected.x - error(expected.x));
        CHECK(endpoint.imageY.lo() <= expected.y + error(expected.y));
        CHECK(endpoint.imageY.hi() >= expected.y - error(expected.y));
    };
    const auto& first = result.fragments.front(); const auto& last = result.fragments.back();
    if (closed) {
        CHECK(first.start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(last.end.labels == unsigned(Trim::EndLabel::CutterRoot));
        checkCut(first.start, first.data.controlPoints.front(), right);
        checkCut(last.end, last.data.controlPoints.back(), left);
    } else {
        CHECK(last.end.labels == unsigned(Trim::EndLabel::SourceEnd));
        near(last.data.controlPoints.back(), source.controlPoints.back(), 0);
        if (left == 0) {
            CHECK(first.start.labels == unsigned(Trim::EndLabel::CutterRoot));
            checkCut(first.start, first.data.controlPoints.front(), right);
        } else {
            CHECK(first.start.labels == unsigned(Trim::EndLabel::SourceStart));
            near(first.data.controlPoints.front(), source.controlPoints.front(), 0);
            checkCut(first.end, first.data.controlPoints.back(), left);
            checkCut(last.start, last.data.controlPoints.front(), right);
        }
    }
}

} // namespace

TEST_CASE("Cache trim recognizes exact closed producer geometry without a DXF closed flag", tags) {
    for (bool reverse : {false, true}) {
        auto data = cache({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}, {-2, -2}});
        data.m_closedFlag = false;
        if (reverse) { reversed(data); }
        const auto before = data;
        RS_Line cutter{nullptr, {0, -3}, {0, 3}};
        const auto result = Trim::prepare(data, cutter, {{2, 0}});
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 1);
        REQUIRE(result.rootCount == 2); geometry(data, result); equalData(data, before);
        CHECK(result.fragments[0].start.labels == unsigned(Trim::EndLabel::CutterRoot));
        CHECK(result.fragments[0].end.labels == unsigned(Trim::EndLabel::CutterRoot));
        for (int n = 0; n < 400; ++n) {
            const double t = (n + .375) / 100.;
            const auto point = evaluate(data, std::size_t(t), t - std::floor(t));
            CHECK(coverage(result, t) == int(point.x < 0));
        }
    }
}

TEST_CASE("Actual C-admitted two-component producer natives trim independently in both cache orders", tags) {
    for (bool reverse : {false, true}) {
        auto native = producedNativeComponents(reverse);
        const std::array<RS_SplineData, 2> before{{native[0]->getData(), native[1]->getData()}};
        std::array<std::vector<const RS_Entity*>, 2> children;
        for (unsigned k = 0; k < 2; ++k) for (const auto* child : *native[k]) { children[k].push_back(child); }
        for (unsigned k = 0; k < 2; ++k) {
            CAPTURE(reverse, k);
            const auto oracle = componentCut(before[k]);
            const bool closed = k == 1;
            if (closed) { REQUIRE(oracle.parameters.size() == 2); }
            const long double left = oracle.parameters.size() > 1 ? oracle.parameters[0] : 0;
            const long double right = oracle.parameters.size() > 1 ? oracle.parameters[1] : oracle.parameters[0];
            REQUIRE(left < right);
            const auto pick = componentPoint(before[k], (left + right) / 2);
            RS_Line cutter{nullptr, {oracle.x, oracle.lowY}, {oracle.x, oracle.highY}};
            const auto result = Trim::prepare(native[k]->getData(), cutter, {pick});
            REQUIRE(result.refusal == Trim::Refusal::None);
            CHECK(result.rootCount == oracle.parameters.size());
            for (const auto& root : result.roots) {
                CHECK(root.proof != LC_IntersectionProof::Uncertified);
                CHECK(root.worldX.contains(oracle.x));
            }
            componentComplement(before[k], result, left, right, closed);
            for (unsigned j = 0; j < 2; ++j) {
                equalData(native[j]->getData(), before[j]);
                REQUIRE(native[j]->hasEditableDefinition());
                CHECK(native[j]->definition().cacheReversed == reverse);
                std::size_t i = 0;
                for (const auto* child : *native[j]) { REQUIRE(i < children[j].size()); CHECK(child == children[j][i++]); }
                CHECK(i == children[j].size());
            }
        }
    }
}

TEST_CASE("Both actual C-admitted producer components also act as native cache cutters", tags) {
    for (bool reverse : {false, true}) {
        auto native = producedNativeComponents(reverse);
        for (unsigned k = 0; k < 2; ++k) {
            CAPTURE(reverse, k);
            const auto before = native[k]->getData();
            auto oracle = componentCut(before);
            std::sort(oracle.ordinates.begin(), oracle.ordinates.end());
            for (std::size_t i = 1; i < oracle.ordinates.size(); ++i) { REQUIRE(oracle.ordinates[i - 1] < oracle.ordinates[i]); }
            const auto target = cache({{oracle.x, oracle.lowY}, {oracle.x, oracle.highY}});
            const auto parameter = [&](long double y) { return (y - oracle.lowY) / (oracle.highY - oracle.lowY); };
            const long double left = oracle.ordinates.size() > 1 ? parameter(oracle.ordinates[0]) : 0;
            const long double right = parameter(oracle.ordinates[oracle.ordinates.size() > 1 ? 1 : 0]);
            const auto result = Trim::prepare(target, *native[k], {componentPoint(target, (left + right) / 2)});
            REQUIRE(result.refusal == Trim::Refusal::None);
            CHECK(result.rootCount == oracle.ordinates.size());
            componentComplement(target, result, left, right, false);
            equalData(native[k]->getData(), before);
            REQUIRE(native[k]->hasEditableDefinition());
        }
    }
}

namespace {
double conicAlong(double from, double to, bool reverse) {
    double value = std::fmod((reverse ? from - to : to - from), 2*pi);
    return value < 0 ? value + 2*pi : value;
}
bool angularMember(const RS_EllipseData& d, double theta) {
    if (d.angle1 == 0 && d.angle2 == 0) return true;
    return conicAlong(d.angle1, theta, d.reversed) < conicAlong(d.angle1, d.angle2, d.reversed);
}
}

TEST_CASE("Conic paired preparation retains analytic complements in either traversal without refitting", tags) {
    const auto source = cache({{-3, .5}, {3, .5}}); const auto before = source;
    for (unsigned kind = 0; kind < 4; ++kind) for (bool reverse : {false, true}) {
        std::unique_ptr<RS_Entity> cutter;
        const bool ellipse = kind >= 2, open = kind == 1 || kind == 3;
        const double ratio = ellipse ? .75 : 1.;
        const double a = open && reverse ? pi : 0, b = open && !reverse ? pi : 0;
        if (kind == 0) cutter = std::make_unique<RS_Circle>(nullptr, RS_CircleData{{0, 0}, 2});
        if (kind == 1) cutter = std::make_unique<RS_Arc>(nullptr, RS_ArcData{{0, 0}, 2, a, b, reverse});
        if (ellipse) cutter = std::make_unique<RS_Ellipse>(nullptr, RS_EllipseData{{0, 0}, {2, 0}, ratio, a, b, reverse});
        const auto result = Trim::prepareConic(source, *cutter, {{0, 2*ratio}}, Trim::Pick{{0, .5}});
        CAPTURE(kind, reverse, result.refusal);
        REQUIRE(result.refusal == Trim::Refusal::None); REQUIRE(result.fragments.size() == 2);
        REQUIRE(result.conicFragments.size() == (open ? 2u : 1u));
        geometry(before, result); equalData(source, before);
        for (const auto& fragment : result.conicFragments) {
            CHECK(fragment.data.center == RS_Vector(0, 0)); CHECK(fragment.data.majorP == RS_Vector(2, 0));
            CHECK(fragment.data.ratio == ratio); CHECK(fragment.data.reversed == (kind == 0 ? false : reverse));
            for (const auto& id : {fragment.startRoot, fragment.endRoot}) if (id) REQUIRE(*id < result.roots.size());
        }
        // Independent membership oracle: the horizontal cutter removes the
        // portion ABOVE y=.5 from the original conic, not its clicked complement.
        // No selected ranges, root labels or helper-generated cut angles define expectation.
        for (unsigned i = 0; i < 256; ++i) {
            const double theta = (i + .375) * (2*pi/256), y = 2*ratio*std::sin(theta);
            if (std::abs(y - .5) < 1e-8) continue;
            const bool original = !open || (theta > 0 && theta < pi);
            unsigned actual = 0;
            for (const auto& fragment : result.conicFragments) actual += unsigned(angularMember(fragment.data, theta));
            CHECK(actual == unsigned(original && y < .5));
        }
    }
}

TEST_CASE("Conic only preparation has no polynomial output and preserves the read only cache", tags) {
    const auto source = cache({{-3, .5}, {3, .5}}); const auto before = source;
    RS_Circle cutter{nullptr, {{0, 0}, 2}};
    const auto result = Trim::prepareConic(source, cutter, {{0, 2}});
    REQUIRE(result.refusal == Trim::Refusal::None); CHECK(result.fragments.empty()); CHECK(result.fragmentCount == 0);
    CHECK_FALSE(result.removed); REQUIRE(result.conicFragments.size() == 1); equalData(source, before);
    CHECK(cutter.getCenter() == RS_Vector(0, 0)); CHECK(cutter.getRadius() == 2);
}

TEST_CASE("Conic endpoint certificates use the actual whole frame chart and refuse symmetric closest ties", tags) {
    const auto source = cache({{-6, .5}, {6, .5}}); const auto before = source;
    for (bool reverse : {false, true}) {
        RS_Ellipse ellipse{nullptr, {{0, 0}, {2, 0}, .5, 0, 0, reverse}};
        const auto frame = LC_EllipseOffsetGeometry::ReferenceFrame::create(ellipse.getData(), 0.);
        REQUIRE(frame); REQUIRE(frame->sourceSweep().whole);
        REQUIRE(frame->sourceSweep().start != ellipse.getAngle1());
        // At P=(1,0), x=4/3, y=+/-sqrt(5)/3 are two distinct global
        // closest occurrences, both with squared distance 2/3. The actual
        // pi/4 chart endpoint is NOT stationary: F=sin(theta)*(2-3*cos(theta)).
        const auto result = Trim::prepareConic(source, ellipse, {{1, 0}}, Trim::Pick{{0, .5}});
        CHECK((result.refusal == Trim::Refusal::AmbiguousPick || result.refusal == Trim::Refusal::LimitExceeded));
        CHECK(result.fragments.empty()); CHECK(result.conicFragments.empty()); CHECK(result.fragmentCount == 0);
        CHECK_FALSE(result.removed); equalData(source, before);
    }
}

TEST_CASE("Conic exact storage seam stationary roots do not turn axis clicks into unsupported positive cases", tags) {
    const auto source = cache({{-6, .5}, {6, .5}}); const auto before = source;
    for (bool rotated : {false, true}) for (bool reverse : {false, true}) {
        const RS_Vector major = rotated ? RS_Vector{2, 1} : RS_Vector{2, 0};
        RS_Ellipse ellipse{nullptr, {{1, 0}, major, .75, 0, 0, reverse}};
        const auto result = Trim::prepareConic(source, ellipse, {{1 - major.x, -major.y}}, Trim::Pick{{0, .5}});
        CAPTURE(rotated, reverse, result.refusal); REQUIRE(result.refusal == Trim::Refusal::None);
        REQUIRE(result.conicFragments.size() == 1); REQUIRE(result.fragments.size() == 2);
        for (unsigned i = 0; i < 256; ++i) {
            const double theta = (i + .375) * (2*pi/256);
            const double y = major.y*std::cos(theta) + .75*major.x*std::sin(theta);
            if (std::abs(y - .5) < 1e-8) continue;
            CHECK(angularMember(result.conicFragments.front().data, theta) == (y > .5));
        }
        equalData(source, before);
    }
}

TEST_CASE("Conic typed staging preserves rotated and exchanged axes without normalization drift", tags) {
    const auto source = cache({{-6, 0}, {6, 0}});
    for (double ratio : {.5, 2.}) {
        RS_Ellipse ellipse{nullptr, {{0, 0}, {2, 1}, ratio}};
        const auto result = Trim::prepareConic(source, ellipse, {{-ratio, 2*ratio}}, Trim::Pick{{0, 0}});
        CAPTURE(ratio, result.refusal); REQUIRE(result.refusal == Trim::Refusal::None);
        REQUIRE(result.conicFragments.size() == 1); CHECK(result.conicFragments.front().data.majorP == RS_Vector(2, 1));
        CHECK(result.conicFragments.front().data.ratio == ratio);
        for (unsigned i = 0; i < 256; ++i) {
            const double theta = (i + .375) * (2*pi/256), y = std::cos(theta) + 2*ratio*std::sin(theta);
            if (std::abs(y) < 1e-8) continue;
            CHECK(angularMember(result.conicFragments.front().data, theta) == (y < 0));
        }
    }
}

TEST_CASE("Conic paired preparation shares output and root limits and never publishes its first prepared side", tags) {
    const auto source = cache({{-3, .5}, {3, .5}}); RS_Circle cutter{nullptr, {{0, 0}, 2}};
    const auto baseline = Trim::prepareConic(source, cutter, {{0, 2}}, Trim::Pick{{0, .5}});
    REQUIRE(baseline.refusal == Trim::Refusal::None); REQUIRE(baseline.work.outputSpans > 1); REQUIRE(baseline.work.rootRecords > 1);
    for (bool roots : {false, true}) {
        Trim::Limits limits;
        if (roots) limits.maxRoots = baseline.work.rootRecords - 1;
        else limits.maxOutputSpans = baseline.work.outputSpans - 1;
        const auto result = Trim::prepareConic(source, cutter, {{0, 2}}, Trim::Pick{{0, .5}}, limits);
        REQUIRE(result.refusal == Trim::Refusal::LimitExceeded);
        CHECK(result.fragments.empty()); CHECK(result.conicFragments.empty()); CHECK(result.fragmentCount == 0); CHECK_FALSE(result.removed);
        CHECK(result.work.outputSpans > 0); // Native side was private, not published as partial success.
        CHECK(result.work.outputSpans <= limits.maxOutputSpans); CHECK(result.work.rootRecords <= limits.maxRoots);
    }
}

TEST_CASE("Conic ambiguous picks cancellation and tangent contacts expose neither replacement set", tags) {
    RS_Circle cutter{nullptr, {{0, 0}, 2}};
    for (unsigned defect = 0; defect < 3; ++defect) {
        const auto source = cache({{-3, defect == 2 ? 2. : .5}, {3, defect == 2 ? 2. : .5}});
        Trim::Cancellation cancellation;
        if (defect == 1) cancellation.requested = std::make_shared<std::atomic_bool>(true);
        const auto result = Trim::prepareConic(source, cutter, {{0, defect == 0 ? 0. : 2.}}, Trim::Pick{{0, .5}}, {}, cancellation);
        CHECK(result.refusal != Trim::Refusal::None); CHECK(result.fragments.empty()); CHECK(result.conicFragments.empty());
        CHECK(result.fragmentCount == 0); CHECK_FALSE(result.removed);
    }
}
