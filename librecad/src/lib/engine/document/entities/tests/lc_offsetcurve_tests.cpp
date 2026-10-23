/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

#include "lc_curveoffset.h"
#include "lc_offsetcurve.h"
#include "rs_line.h"

namespace geo = LC_EllipseOffsetGeometry;
namespace {
LC_EllipseOffsetDefinition circle(const bool whole = true) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, whole ? 0 : .25, whole ? 0 : .2501, false};
    definition.distance = 1; definition.relTolerance = .01;
    REQUIRE(geo::normaliseBasis(definition.basis));
    return definition;
}

RS_SplineData ring() {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto &point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned n = 0; n < (i == 0 || i == 4 ? 4u : 3u); ++n) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1);
    data.m_closedFlag = true;
    return data;
}

RS_SplineData arc(const LC_EllipseOffsetDefinition &definition, const unsigned degree = 1) {
    const auto frame = geo::ReferenceFrame::create(definition.basis,
        definition.towardCentre ? -definition.distance : definition.distance);
    REQUIRE(frame);
    LC_CurveJet a, b;
    REQUIRE(frame->offsetJet(0, a)); REQUIRE(frame->offsetJet(frame->parameterEnd(), b));
    const auto world = [&](const RS_Vector &p) { return definition.basis.center + p*frame->scale(); };
    RS_SplineData data{static_cast<int>(degree), false};
    data.controlPoints.push_back(world(a.point));
    if (degree == 3) {
        data.controlPoints.push_back(world(a.point + a.first*(frame->parameterEnd()/3)));
        data.controlPoints.push_back(world(b.point - b.first*(frame->parameterEnd()/3)));
    }
    data.controlPoints.push_back(world(b.point));
    data.weights.assign(degree + 1, 1);
    for (unsigned i = 0; i <= degree; ++i) data.knotslist.push_back(0);
    for (unsigned i = 0; i <= degree; ++i) data.knotslist.push_back(1);
    return data;
}

std::unique_ptr<LC_OffsetCurve> nativeRing() {
    RS_Spline cache{nullptr, ring()};
    LC_OffsetCheckResult check;
    auto result = LC_OffsetCurve::adopt(cache, circle(), .02, &check);
    CAPTURE(check.reason, check.span);
    REQUIRE(result); REQUIRE(check); REQUIRE(check.analysis->closed());
    return result;
}

void certified(const LC_OffsetCurve &curve) {
    REQUIRE(curve.hasEditableDefinition());
    const auto check = geo::checkCache(curve.getData(), curve.definition(), curve.checkedWorldTolerance());
    CAPTURE(check.reason, check.span);
    REQUIRE(check);
    CHECK(check.analysis->closed() == curve.cacheFormsClosedChain());
}

LC_OffsetCurve::OwnedTail tail() { return {{1000, QStringLiteral("kept")}, {1070, std::int16_t{17}}}; }
}

TEST_CASE("native offset adoption certifies lines without changing saved direction", "[lc-offsetcurve][native]") {
    for (const bool sourceReversed : {false, true}) for (const bool cacheReversed : {false, true}) {
        auto definition = circle(false); definition.basis.reversed = sourceReversed;
        if (sourceReversed) {
            const double start = definition.basis.angle1;
            definition.basis.setAngle1(definition.basis.angle2);
            definition.basis.setAngle2(start);
        }
        definition.cacheReversed = cacheReversed;
        auto data = arc(definition);
        if (cacheReversed) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        RS_Line line{nullptr, data.controlPoints.front(), data.controlPoints.back()};
        LC_OffsetCheckResult check;
        auto result = LC_OffsetCurve::adopt(line, definition, .01, &check);
        CAPTURE(sourceReversed, cacheReversed, check.reason);
        REQUIRE(result);
        CHECK(result->getData().degree == 1);
        CHECK(result->getData().knotslist == data.knotslist);
        CHECK(result->getData().weights == data.weights);
        CHECK(result->getData().controlPoints == data.controlPoints);
        CHECK(result->definition().cacheReversed == cacheReversed);
        CHECK_FALSE(result->sourceIsWhole()); CHECK_FALSE(result->cacheFormsClosedChain());
        certified(*result);
    }
}

TEST_CASE("native offset adoption retains cubic controls and refuses off-curve data", "[lc-offsetcurve][native]") {
    auto definition = circle(false);
    RS_Spline cache{nullptr, arc(definition, 3)};
    auto result = LC_OffsetCurve::adopt(cache, definition, .01);
    REQUIRE(result);
    CHECK(RS_Spline::geometryFingerprint(result->getData()) == RS_Spline::geometryFingerprint(cache.getData()));
    certified(*result);
    cache.getData().controlPoints[1].x += 100;
    LC_OffsetCheckResult check;
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, definition, .01, &check));
    CHECK_FALSE(check);
}

TEST_CASE("native offset adopts generated cubic engine output through real checker", "[lc-offsetcurve][native]") {
    auto definition = circle(false); definition.basis.setAngle2(.5); definition.relTolerance = 1e-4;
    const RS_Ellipse source{nullptr, definition.basis};
    const auto made = LC_CurveOffset::createEntities(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, definition.distance),
        LC_CurveOffset::makeDirectOptions(source, definition.distance, 1e-4), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
    auto result = LC_OffsetCurve::adopt(*made.entities.front(), definition, 1e-4);
    REQUIRE(result); CHECK(result->getData().degree == 3); certified(*result);
}

TEST_CASE("native offset adopts inward circular arcs and trimmed elliptic lenses", "[lc-offsetcurve][native]") {
    {
        auto definition = circle(false); definition.distance = .25; definition.towardCentre = true;
        RS_Spline cache{nullptr, arc(definition, 3)};
        auto result = LC_OffsetCurve::adopt(cache, definition, .01);
        REQUIRE(result); CHECK(result->definition().towardCentre); certified(*result);
    }
    for (const bool sourceWhole : {true, false}) {
        auto definition = circle();
        const double pi = std::acos(-1.);
        definition.basis = {{0, 0}, {100, 0}, .5, sourceWhole ? 0 : pi/18,
                            sourceWhole ? 0 : 35*pi/18, false};
        definition.distance = sourceWhole ? 49 : 49.5; definition.towardCentre = true;
        const RS_Ellipse source{nullptr, definition.basis};
        const auto made = LC_CurveOffset::createEntities(source,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, definition.distance),
            LC_CurveOffset::makeOffsetOptions(source, definition.distance, 1e-3), LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(made.status == LC_CurveOffsetStatus::Ok); REQUIRE(made.entities.size() == 1);
        auto result = LC_OffsetCurve::adopt(*made.entities.front(), definition, .01);
        REQUIRE(result); CHECK(result->sourceIsWhole() == sourceWhole);
        CHECK(result->cacheFormsClosedChain()); certified(*result);
    }
}

TEST_CASE("native offset adoption rejects counts and invalid raw basis before copying", "[lc-offsetcurve][native]") {
    auto definition = circle(); RS_Spline cache{nullptr, ring()};
    LC_OffsetCheckLimits limits; limits.maxCacheSpans = 3;
    LC_OffsetCheckResult check;
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, definition, .02, &check, limits));
    CHECK(check.reason == LC_OffsetCheckResult::Reason::LimitExceeded);
    definition.basis.majorP.x = std::numeric_limits<double>::infinity();
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, definition, .02));
    definition = circle(); definition.basis.center.z = 1;
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, definition, .02));
    definition = circle(); definition.basis.setAngle2(1e-13);
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, definition, .02));
}

TEST_CASE("native offset rejects unsupported raw layouts without repairing them", "[lc-offsetcurve][native]") {
    for (unsigned kind = 0; kind < 6; ++kind) {
        RS_Spline cache{nullptr, ring()};
        if (kind == 0) cache.getData().degree = 2;
        if (kind == 1) cache.getData().weights[0] = 2;
        if (kind == 2) cache.getData().fitPoints.push_back({0, 0});
        if (kind == 3) cache.getData().savedOpenKnots.push_back(0);
        if (kind == 4) cache.getData().type = RS_SplineData::SplineType::Standard;
        if (kind == 5) cache.getData().knotslist.pop_back();
        const auto stamp = RS_Spline::geometryFingerprint(cache.getData());
        LC_OffsetCheckResult check;
        CHECK_FALSE(LC_OffsetCurve::adopt(cache, circle(), .02, &check));
        CHECK(check.reason == LC_OffsetCheckResult::Reason::Layout);
        CHECK(RS_Spline::geometryFingerprint(cache.getData()) == stamp);
    }
}

TEST_CASE("native offset callers cannot raise the hard cache admission limits", "[lc-offsetcurve][native]") {
    RS_Spline cache{nullptr, ring()};
    auto &data = cache.getData(); data = RS_SplineData{1, false};
    data.controlPoints.assign(4098, {2, 0}); data.weights.assign(4098, 1);
    data.knotslist.push_back(0);
    for (unsigned i = 0; i <= 4097; ++i) data.knotslist.push_back(i);
    data.knotslist.push_back(4097);
    LC_OffsetCheckLimits raised;
    raised.maxCacheSpans = 100000; raised.maxVertices = 100000;
    LC_OffsetCheckResult check;
    CHECK_FALSE(LC_OffsetCurve::adopt(cache, circle(), .02, &check, raised));
    CHECK(check.reason == LC_OffsetCheckResult::Reason::LimitExceeded);
    CHECK(check.work.cacheSpans == 0); CHECK(check.work.vertices == 0);
}

TEST_CASE("spline fingerprint covers every mutable representation field", "[lc-offsetcurve][native][spline]") {
    const auto original = ring(); const auto stamp = RS_Spline::geometryFingerprint(original);
    for (unsigned field = 0; field < 13; ++field) {
        auto data = original;
        switch (field) {
        case 0: ++data.degree; break;
        case 1: data.type = RS_SplineData::SplineType::Standard; break;
        case 2: data.savedOpenType = RS_SplineData::SplineType::Standard; break;
        case 3: data.controlPoints[0].x += 1; break;
        case 4: data.controlPoints[0].y += 1; break;
        case 5: data.controlPoints[0].z = 1; break;
        case 6: data.controlPoints[0].valid = false; break;
        case 7: data.knotslist[0] = -1; break;
        case 8: data.weights[0] = 2; break;
        case 9: data.fitPoints.push_back({0, 0}); break;
        case 10: data.savedOpenKnots.push_back(0); break;
        case 11: data.m_closedFlag = !data.m_closedFlag; break;
        case 12: data.m_periodicFlag = !data.m_periodicFlag; break;
        }
        CAPTURE(field); CHECK(RS_Spline::geometryFingerprint(data) != stamp);
    }
}

TEST_CASE("native offset external mutation cannot be laundered by update clone or reversal", "[lc-offsetcurve][native]") {
    auto result = nativeRing();
    result->getData().controlPoints[0].x += .01;
    CHECK_FALSE(result->isConsistent());
    result->update(); CHECK_FALSE(result->hasEditableDefinition());
    auto clone = std::unique_ptr<LC_OffsetCurve>{static_cast<LC_OffsetCurve *>(result->clone())};
    CHECK_FALSE(clone->hasEditableDefinition());
    result->revertDirection(); CHECK_FALSE(result->hasEditableDefinition());
    result->move({1, 0}); CHECK_FALSE(result->hasEditableDefinition());
    CHECK(result->count() > 0);
}

TEST_CASE("native offset rejects stale degree overflow and oversized representations before use", "[lc-offsetcurve][native]") {
    auto result = nativeRing();
    result->getData() = RS_SplineData{};
    result->getData().degree = std::numeric_limits<std::size_t>::max();
    CHECK_FALSE(result->hasEditableDefinition());
    REQUIRE_NOTHROW(result->update());
    REQUIRE_NOTHROW(result->move({1, 0}));
    REQUIRE_NOTHROW(result->revertDirection());
    CHECK(result->getData().degree == std::numeric_limits<std::size_t>::max());
    RS_Spline unsafeBefore{nullptr, result->getData()}; CHECK_FALSE(unsafeBefore.validate());
    result->getData() = ring();
    result->getData().fitPoints.resize(65537, {0, 0});
    CHECK_FALSE(result->hasEditableDefinition());
    REQUIRE_NOTHROW(result->update()); REQUIRE_NOTHROW(result->move({1, 0}));
    CHECK(result->getData().fitPoints.size() == 65537);
}

TEST_CASE("native offset common attributes and original source have independent ownership", "[lc-offsetcurve][native]") {
    RS_EntityContainer parent;
    RS_Spline cache{&parent, ring()};
    cache.setVisible(false); cache.setSourceHandle(123); cache.setMaterialHandle(456);
    cache.setPlotStyleHandle(789); cache.setUserDefVar("custom", "value");
    auto result = LC_OffsetCurve::adopt(cache, circle(), .02);
    REQUIRE(result); CHECK(result->getParent() == &parent);
    CHECK_FALSE(result->getFlag(RS2::FlagVisible));
    CHECK(result->sourceHandle() == 123); CHECK(result->materialHandle() == 456);
    CHECK(result->plotStyleHandle() == 789); CHECK(result->getUserDefVar("custom") == "value");
    auto original = result->originalEllipse();
    REQUIRE(original); CHECK(original->getParent() == nullptr);
    CHECK(original->sourceHandle() == 0); CHECK_FALSE(original->hasDrwExtData());
    auto plain = result->toPlainSpline();
    REQUIRE(plain); CHECK(plain->rtti() == RS2::EntitySpline);
    CHECK(plain->sourceHandle() == 123); CHECK(plain->materialHandle() == 456);
    CHECK(plain->first() != result->first()); CHECK(plain->first()->getParent() == plain.get());
}

TEST_CASE("native offset exact translation transports its certificate", "[lc-offsetcurve][native]") {
    auto result = nativeRing(); result->move({4, 8});
    CHECK(result->definition().basis.center == RS_Vector(4, 8));
    CHECK(result->getData().controlPoints.front() == RS_Vector(6, 8));
    certified(*result);
}

TEST_CASE("native offset signed permutation rotations and uniform scales remain certified", "[lc-offsetcurve][native]") {
    auto result = nativeRing();
    result->rotate({0, 0}, RS_Vector{0, 1});
    CHECK(result->getData().controlPoints.front() == RS_Vector(0, 2));
    CHECK(result->definition().basis.majorP == RS_Vector(0, 1)); certified(*result);
    result->scale({0, 0}, {2, 2});
    CHECK(result->definition().distance == 2); CHECK(result->checkedWorldTolerance() == .04); certified(*result);
    result->scale({0, 0}, {-2, -2});
    CHECK(result->definition().distance == 4); certified(*result);
}

TEST_CASE("native offset whole mirror transports orientation and reversal transports direction", "[lc-offsetcurve][native]") {
    auto result = nativeRing();
    result->mirror({0, 0}, {1, 0});
    CHECK(result->definition().basis.reversed); certified(*result);
    result->revertDirection(); CHECK(result->definition().cacheReversed); certified(*result);
    result->revertDirection(); CHECK_FALSE(result->definition().cacheReversed); certified(*result);
}

TEST_CASE("native offset uncertifiable rounded similarities refuse unchanged", "[lc-offsetcurve][native]") {
    auto result = nativeRing();
    REQUIRE(result->setOwnedExtensions(0x10, tail()));
    const auto stamp = RS_Spline::geometryFingerprint(result->getData());
    result->move({.1, .1});
    CHECK(RS_Spline::geometryFingerprint(result->getData()) == stamp);
    result->rotate({0, 0}, .3);
    CHECK(RS_Spline::geometryFingerprint(result->getData()) == stamp);
    result->mirror({0, 0}, {2, 1});
    CHECK(RS_Spline::geometryFingerprint(result->getData()) == stamp);
    CHECK(result->unknownOwnedFlags() == 0x10); CHECK(result->unknownOwnedTail().size() == 2);
    certified(*result);
}

TEST_CASE("native offset nonuniform singular and shear transformations lose definition", "[lc-offsetcurve][native]") {
    for (unsigned kind = 0; kind < 4; ++kind) {
        auto result = nativeRing();
        REQUIRE(result->setOwnedExtensions(0x20, tail()));
        if (kind == 0) result->scale({0, 0}, {2, 3});
        if (kind == 1) result->scale({0, 0}, {0, 0});
        if (kind == 2) result->shear(.25);
        if (kind == 3) result->scale({0, 0}, {1, std::nextafter(1., 2.)});
        CHECK_FALSE(result->isDefined()); CHECK_FALSE(result->hasEditableDefinition());
        CHECK(result->rtti() == RS2::EntityOffsetCurve);
        CHECK(result->unknownOwnedFlags() == 0); CHECK(result->unknownOwnedTail().empty());
        auto plain = result->toPlainSpline(); CHECK(plain->rtti() == RS2::EntitySpline);
    }
}

TEST_CASE("native offset undefined instances retain ordinary rounded rotation behavior", "[lc-offsetcurve][native]") {
    auto result = nativeRing(); result->scale({0, 0}, {2, 3});
    REQUIRE_FALSE(result->hasEditableDefinition());
    const auto before = result->getData().controlPoints.front();
    result->rotate({0, 0}, .3);
    CHECK(result->getData().controlPoints.front().x == Catch::Approx(before.x*std::cos(.3) - before.y*std::sin(.3)));
    CHECK(result->getData().controlPoints.front().y == Catch::Approx(before.x*std::sin(.3) + before.y*std::cos(.3)));
    CHECK_FALSE(result->hasEditableDefinition());
}

TEST_CASE("native offset invalid no-op and extreme transforms preserve complete state", "[lc-offsetcurve][native]") {
    auto result = nativeRing(); REQUIRE(result->setOwnedExtensions(0x10, tail()));
    const auto stamp = RS_Spline::geometryFingerprint(result->getData());
    result->move({0, 0}); result->rotate({0, 0}, 0); result->scale({0, 0}, {1, 1}); result->shear(0);
    result->mirror({1, 1}, {1, 1}); result->rotate({0, 0}, RS_Vector{0, 0});
    result->move({std::numeric_limits<double>::infinity(), 0});
    result->move({std::numeric_limits<double>::max(), 0});
    result->scale({0, 0}, {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()});
    CHECK(RS_Spline::geometryFingerprint(result->getData()) == stamp);
    CHECK(result->unknownOwnedFlags() == 0x10); CHECK(result->unknownOwnedTail().size() == 2);
    certified(*result);
}

TEST_CASE("native offset extension items are typed bounded and value-owned", "[lc-offsetcurve][native]") {
    auto result = nativeRing(); auto extensions = tail();
    REQUIRE(result->setOwnedExtensions(0x20, extensions));
    std::get<QString>(extensions[0].value) = "mutated caller";
    CHECK(std::get<QString>(result->unknownOwnedTail()[0].value) == "kept");
    CHECK_FALSE(result->setOwnedExtensions(1, tail()));
    CHECK_FALSE(result->setOwnedExtensions(0, {{1002, QStringLiteral("{")}}));
    CHECK_FALSE(result->setOwnedExtensions(0, {{1005, QStringLiteral("123")}}));
    CHECK_FALSE(result->setOwnedExtensions(0, {{1040, QStringLiteral("bad type")}}));
    CHECK_FALSE(result->setOwnedExtensions(0, LC_OffsetCurve::OwnedTail(65, tail()[0])));
    CHECK_FALSE(result->setOwnedExtensions(0, {{1000, QString(1024, QChar('x'))}}));
    CHECK(result->unknownOwnedFlags() == 0x20); CHECK(result->unknownOwnedTail().size() == 2);
    result->scale({0, 0}, {2, 2}); CHECK(result->unknownOwnedTail().empty()); CHECK(result->unknownOwnedFlags() == 0);
}
