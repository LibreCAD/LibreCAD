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
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

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

void sameOwnedNumber(const double &actual, const double &expected) {
    CHECK(std::memcmp(&actual, &expected, sizeof(double)) == 0);
}

struct OwnedTailState {
    unsigned flags;
    LC_OffsetCurve::OwnedTail values;
    const LC_OffsetCurveExtension *storage;
    std::vector<const QChar *> textStorage;

    explicit OwnedTailState(const LC_OffsetCurve &curve) : flags(curve.unknownOwnedFlags()),
        values(curve.unknownOwnedTail()), storage(curve.unknownOwnedTail().data()) {
        for (auto &item : values) {
            auto *text = std::get_if<QString>(&item.value);
            textStorage.push_back(text ? text->constData() : nullptr);
            // The expected values must not share the strings being checked.
            if (text) *text = QString(text->constData(), text->size());
        }
    }

    void check(const LC_OffsetCurve &curve) const {
        CHECK(curve.unknownOwnedFlags() == flags);
        const auto &actual = curve.unknownOwnedTail();
        REQUIRE(actual.size() == values.size());
        CHECK(actual.data() == storage);
        for (std::size_t i = 0; i < values.size(); ++i) {
            CAPTURE(i);
            CHECK(actual[i].code == values[i].code);
            REQUIRE(actual[i].value.index() == values[i].value.index());
            if (const auto *text = std::get_if<QString>(&values[i].value)) {
                const auto &stored = std::get<QString>(actual[i].value);
                CHECK(stored == *text);
                CHECK(stored.constData() == textStorage[i]);
            } else if (const auto *point = std::get_if<RS_Vector>(&values[i].value)) {
                const auto &stored = std::get<RS_Vector>(actual[i].value);
                CHECK(stored.valid == point->valid);
                sameOwnedNumber(stored.x, point->x); sameOwnedNumber(stored.y, point->y);
                sameOwnedNumber(stored.z, point->z);
            } else if (const auto *number = std::get_if<double>(&values[i].value))
                sameOwnedNumber(std::get<double>(actual[i].value), *number);
            else if (const auto *number = std::get_if<std::int16_t>(&values[i].value))
                CHECK(std::get<std::int16_t>(actual[i].value) == *number);
            else CHECK(std::get<std::int32_t>(actual[i].value) == std::get<std::int32_t>(values[i].value));
        }
    }
};

void refusesOwnedTail(LC_OffsetCurve &curve, const unsigned flags, const LC_OffsetCurve::OwnedTail &items) {
    const OwnedTailState before{curve};
    CHECK_FALSE(curve.setOwnedExtensions(flags, items));
    before.check(curve);
}

LC_OffsetCurve::OwnedTail nestedOwnedTail(const unsigned depth) {
    LC_OffsetCurve::OwnedTail items;
    for (unsigned i = 0; i < depth; ++i) items.push_back({1002, QStringLiteral("{")});
    items.push_back({1000, QStringLiteral("leaf")});
    for (unsigned i = 0; i < depth; ++i) items.push_back({1002, QStringLiteral("}")});
    return items;
}
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

TEST_CASE("native offset adoption refuses certified geometry outside spline display knot resolution",
          "[lc-offsetcurve][native][display]") {
    const auto definition = circle(false);
    auto data = arc(definition);
    for (auto &knot : data.knotslist) knot *= 1e-12;
    // This is still the same exact unit-weight Bezier line in a different
    // parameter domain. C's geometric admission is not a display certificate.
    const auto geometric = geo::checkCache(data, definition, .01);
    REQUIRE(geometric);
    RS_Spline cache{nullptr, data};
    CHECK_FALSE(cache.validate()); CHECK(cache.count() == 0);
    LC_OffsetCheckResult checked;
    LC_OffsetCurveRefusal refusal;
    auto adopted = LC_OffsetCurve::adopt(cache, definition, .01, &checked, {}, &refusal);
    CHECK_FALSE(adopted); CHECK(checked);
    CHECK(checked.reason == LC_OffsetCheckResult::Reason::None);
    CHECK(refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK(checked.work.cacheSpans == geometric.work.cacheSpans);
    CHECK(checked.work.vertices == geometric.work.vertices);
    CHECK(checked.work.distancePairs == geometric.work.distancePairs);
    CHECK(cache.getData().knotslist == data.knotslist);
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
    CHECK_FALSE(result->setOwnedExtensions(0, {{1000, QString(4097, QChar('x'))}}));
    CHECK(result->unknownOwnedFlags() == 0x20); CHECK(result->unknownOwnedTail().size() == 2);
    result->scale({0, 0}, {2, 2}); CHECK(result->unknownOwnedTail().empty()); CHECK(result->unknownOwnedFlags() == 0);
}

TEST_CASE("native offset extension string limits count UTF16 units independently of target encoding",
          "[lc-offsetcurve][native][owned-tail]") {
    static_assert(LC_OffsetCurve::MaximumOwnedStringUnits == 4096);
    auto result = nativeRing();
    const auto maximum = static_cast<qsizetype>(LC_OffsetCurve::MaximumOwnedStringUnits);
    for (const auto units : {qsizetype{1024}, maximum}) {
        const QString text(units, QChar('x'));
        REQUIRE(result->setOwnedExtensions(0x7ff0, {{1000, text}}));
        CHECK(std::get<QString>(result->unknownOwnedTail().front().value) == text);
        refusesOwnedTail(*result, 0x10, {{1000, QString(maximum + 1, QChar('x'))}});
    }
    QString unicode(maximum, QChar(char16_t{0x4e2d}));
    unicode[0] = QChar(char16_t{0xfeff});
    unicode[maximum - 2] = QChar(char16_t{0xd83d});
    unicode[maximum - 1] = QChar(char16_t{0xde42});
    REQUIRE(result->setOwnedExtensions(0x30, {{1000, unicode}}));
    CHECK(std::get<QString>(result->unknownOwnedTail().front().value) == unicode);
    CHECK(std::get<QString>(result->unknownOwnedTail().front().value).size() == maximum);
    refusesOwnedTail(*result, 0, {{1000, unicode + QStringLiteral("x")}});
}

TEST_CASE("native offset extension logical storage admits exactly 16 KiB and refuses one byte more",
          "[lc-offsetcurve][native][owned-tail]") {
    static_assert(LC_OffsetCurve::MaximumOwnedTailLogicalBytes == 16384);
    auto result = nativeRing();
    // (3+2*4096) + (3+2*4093) = 16384, including the fixed item charges.
    const LC_OffsetCurve::OwnedTail exact{{1000, QString(4096, QChar('x'))},
                                         {1000, QString(4093, QChar('y'))}};
    REQUIRE(result->setOwnedExtensions(0x40, exact));
    REQUIRE(result->unknownOwnedTail().size() == 2);
    CHECK(std::get<QString>(result->unknownOwnedTail()[0].value) == std::get<QString>(exact[0].value));
    CHECK(std::get<QString>(result->unknownOwnedTail()[1].value) == std::get<QString>(exact[1].value));
    // (3+2*4096) + (3+2*4089) + 9 = 16385; all individual items remain valid.
    refusesOwnedTail(*result, 0x20, {{1000, QString(4096, QChar('x'))},
        {1000, QString(4089, QChar('y'))}, {1040, 1.0}});
    auto larger = exact;
    std::get<QString>(larger[1].value).append(QChar('y'));
    refusesOwnedTail(*result, 0, larger);
}

TEST_CASE("native offset extension logical boundaries charge every scalar vector and brace item",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing();
    for (const auto code : {1010, 1040, 1070, 1071}) {
        CAPTURE(code);
        LC_OffsetCurveExtension last;
        std::size_t charge;
        if (code == 1010) { last = {code, RS_Vector{1, 2, 3}}; charge = 25; }
        else if (code == 1040) { last = {code, 1.25}; charge = 9; }
        else if (code == 1070) { last = {code, std::int16_t{7}}; charge = 3; }
        else { last = {code, std::int32_t{7}}; charge = 5; }
        const auto units = static_cast<qsizetype>((8183 - charge)/2);
        // Two strings, one int16 and the final typed item total exactly 16384.
        const LC_OffsetCurve::OwnedTail exact{{1000, QString(4096, QChar('x'))},
            {1000, QString(units, QChar('y'))}, {1070, std::int16_t{9}}, last};
        REQUIRE(result->setOwnedExtensions(0x10, exact));
        // Replace the 3-byte int16 with two 5-byte logical brace items and
        // remove three UTF16 units: +10-3-6 = one byte over the cap.
        refusesOwnedTail(*result, 0, {{1000, QString(4096, QChar('x'))},
            {1000, QString(units - 3, QChar('y'))}, {1002, QStringLiteral("{")},
            {1002, QStringLiteral("}")}, last});
    }
}

TEST_CASE("native offset extension storage accepts both exact 4096 encoded byte tail witnesses",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing();
    // ASCII XDATA witness: 15*(255+3) + (223+3) = 4096 encoded bytes.
    LC_OffsetCurve::OwnedTail ascii(15, {1000, QString(255, QChar('x'))});
    ascii.push_back({1000, QString(223, QChar('x'))});
    REQUIRE(result->setOwnedExtensions(0x10, ascii));
    REQUIRE(result->unknownOwnedTail().size() == 16);
    for (std::size_t i = 0; i < ascii.size(); ++i)
        CHECK(std::get<QString>(result->unknownOwnedTail()[i].value) == std::get<QString>(ascii[i].value));
    // UTF16 EED witnesses: 3 + 2*2045 + 3 = 4096 encoded bytes.
    // These are native storage tests, not an encoder/checker qualification.
    for (const bool surrogate : {false, true}) {
        QString text(surrogate ? 2043 : 2045, QChar(char16_t{0x4e2d}));
        if (surrogate) { text.append(QChar(char16_t{0xd83d})); text.append(QChar(char16_t{0xde42})); }
        REQUIRE(text.size() == 2045);
        REQUIRE(result->setOwnedExtensions(0x20, {{1000, text}, {1070, std::int16_t{7}}}));
        REQUIRE(result->unknownOwnedTail().size() == 2);
        CHECK(std::get<QString>(result->unknownOwnedTail()[0].value) == text);
        CHECK(std::get<std::int16_t>(result->unknownOwnedTail()[1].value) == 7);
    }
}

TEST_CASE("native offset extension item and brace bounds retain only complete tails",
          "[lc-offsetcurve][native][owned-tail]") {
    static_assert(LC_OffsetCurve::MaximumOwnedTailItems == 64);
    static_assert(LC_OffsetCurve::MaximumOwnedTailDepth == 8);
    auto result = nativeRing();
    LC_OffsetCurve::OwnedTail items(64, {1070, std::int16_t{7}});
    REQUIRE(result->setOwnedExtensions(0x7ff0, items));
    CHECK(result->unknownOwnedTail().size() == 64);
    items.push_back({1070, std::int16_t{8}});
    refusesOwnedTail(*result, 0, items);
    const auto nested = nestedOwnedTail(8);
    REQUIRE(result->setOwnedExtensions(0x10, nested));
    CHECK(result->unknownOwnedTail().size() == 17);
    refusesOwnedTail(*result, 0, nestedOwnedTail(9));
    refusesOwnedTail(*result, 0, {{1002, QStringLiteral("{")}});
    refusesOwnedTail(*result, 0, {{1002, QStringLiteral("}")}});
    refusesOwnedTail(*result, 0, {{1002, QStringLiteral("not a brace")}});
    refusesOwnedTail(*result, 0, {{1002, QString()}});
    REQUIRE(result->setOwnedExtensions(0x20, {{1002, QStringLiteral("{")}, {1002, QStringLiteral("}")}}));
    CHECK(result->unknownOwnedTail().size() == 2);
    REQUIRE(result->setOwnedExtensions(0x40, {}));
    CHECK(result->unknownOwnedTail().empty()); CHECK(result->unknownOwnedFlags() == 0x40);
    refusesOwnedTail(*result, 1, {});
    REQUIRE(result->setOwnedExtensions(0, {}));
    CHECK(result->unknownOwnedFlags() == 0);
}

TEST_CASE("native offset extension strings reject malformed UTF16 and forbidden controls atomically",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing(); REQUIRE(result->setOwnedExtensions(0x30, tail()));
    const QString high(1, QChar(char16_t{0xd800})), low(1, QChar(char16_t{0xdc00}));
    const std::array<QString, 5> invalid{{high, low, high + QStringLiteral("x"), low + high,
                                        QStringLiteral("prefix") + high}};
    for (const auto &text : invalid) {
        refusesOwnedTail(*result, 0, {{1000, text}});
        refusesOwnedTail(*result, 0, {{1002, text}});
    }
    for (const auto unit : std::array<char16_t, 3>{{0, u'\r', u'\n'}}) {
        for (const auto position : {0, 2, 4}) {
            QString text = QStringLiteral("safe"); text.insert(position, QChar(unit));
            refusesOwnedTail(*result, 0, {{1000, text}});
        }
    }
    QString legalControls;
    for (char16_t unit = 1; unit < 0x20; ++unit)
        if (unit != u'\r' && unit != u'\n') legalControls.append(QChar(unit));
    legalControls.append(QChar(char16_t{0x7f}));
    REQUIRE(result->setOwnedExtensions(0x10, {{1000, legalControls}, {1000, QString()}}));
    CHECK(std::get<QString>(result->unknownOwnedTail()[0].value) == legalControls);
    CHECK(std::get<QString>(result->unknownOwnedTail()[1].value).isEmpty());
}

TEST_CASE("native offset extension admission preserves exact prior typed values on every scalar refusal",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing();
    const LC_OffsetCurve::OwnedTail supported{
        {1000, QStringLiteral("kept")}, {1002, QStringLiteral("{")},
        {1010, RS_Vector{-0.0, 2, 3}}, {1011, RS_Vector{4, -5, 6}},
        {1012, RS_Vector{7, 8, -9}}, {1013, RS_Vector{10, 11, 12}},
        {1040, -0.0}, {1041, 1.25}, {1042, -2.5},
        {1070, std::numeric_limits<std::int16_t>::min()},
        {1071, std::numeric_limits<std::int32_t>::max()}, {1002, QStringLiteral("}")}};
    REQUIRE(result->setOwnedExtensions(0x7ff0, supported));
    for (const unsigned flags : {1u, 8u, 0x8000u, std::numeric_limits<unsigned>::max()})
        refusesOwnedTail(*result, flags, tail());
    for (const auto code : {1001, 1003, 1004, 1005, 1006})
        refusesOwnedTail(*result, 0, {{code, QStringLiteral("unsupported")}});
    refusesOwnedTail(*result, 0, {{1000, std::int16_t{7}}});
    refusesOwnedTail(*result, 0, {{1002, std::int16_t{7}}});
    refusesOwnedTail(*result, 0, {{1010, QStringLiteral("wrong type")}});
    refusesOwnedTail(*result, 0, {{1040, QStringLiteral("wrong type")}});
    refusesOwnedTail(*result, 0, {{1070, std::int32_t{7}}});
    refusesOwnedTail(*result, 0, {{1071, std::int16_t{7}}});
    refusesOwnedTail(*result, 0, {{1010, RS_Vector{false}}});
    for (const double invalid : {std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}) {
        for (const auto code : {1040, 1041, 1042}) refusesOwnedTail(*result, 0, {{code, invalid}});
        for (const auto code : {1010, 1011, 1012, 1013}) {
            refusesOwnedTail(*result, 0, {{code, RS_Vector{invalid, 2, 3}}});
            refusesOwnedTail(*result, 0, {{code, RS_Vector{1, invalid, 3}}});
            refusesOwnedTail(*result, 0, {{code, RS_Vector{1, 2, invalid}}});
        }
    }
}

TEST_CASE("native offset extension text copies do not retain excess caller capacity",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing();
    QString text = QStringLiteral("kept"); text.reserve(65536);
    const auto *callerStorage = text.constData(); const auto callerCapacity = text.capacity();
    REQUIRE(callerCapacity > static_cast<qsizetype>(LC_OffsetCurve::MaximumOwnedStringUnits));
    const LC_OffsetCurve::OwnedTail input{{1000, text}};
    REQUIRE(result->setOwnedExtensions(0x10, input));
    const auto &stored = std::get<QString>(result->unknownOwnedTail().front().value);
    CHECK(stored == text); CHECK(stored.constData() != callerStorage);
    CHECK(stored.capacity() <= static_cast<qsizetype>(LC_OffsetCurve::MaximumOwnedStringUnits));
    CHECK(text.constData() == callerStorage); CHECK(text.capacity() == callerCapacity);
    text[0] = QChar('X'); CHECK(stored == QStringLiteral("kept"));
    // Passing the current tail back must also be safe while the old tail is alive.
    REQUIRE(result->setOwnedExtensions(result->unknownOwnedFlags(), result->unknownOwnedTail()));
    CHECK(std::get<QString>(result->unknownOwnedTail().front().value) == QStringLiteral("kept"));
    CHECK(result->unknownOwnedFlags() == 0x10);
}

TEST_CASE("native offset extension text owns fromRawData values beyond caller mutation and lifetime",
          "[lc-offsetcurve][native][owned-tail]") {
    auto result = nativeRing();
    std::unique_ptr<RS_Entity> cloned;
    {
        std::array<QChar, 4> raw{{QChar('k'), QChar('e'), QChar('p'), QChar('t')}};
        const QString borrowed = QString::fromRawData(raw.data(), static_cast<qsizetype>(raw.size()));
        REQUIRE(borrowed.constData() == raw.data());
        const LC_OffsetCurve::OwnedTail input{{1000, borrowed}};
        REQUIRE(result->setOwnedExtensions(0x20, input));
        CHECK(std::get<QString>(result->unknownOwnedTail().front().value).constData() != raw.data());
        cloned.reset(result->clone());
        raw[0] = QChar('X');
        CHECK(std::get<QString>(input.front().value) == QStringLiteral("Xept"));
        CHECK(std::get<QString>(result->unknownOwnedTail().front().value) == QStringLiteral("kept"));
    }
    CHECK(std::get<QString>(result->unknownOwnedTail().front().value) == QStringLiteral("kept"));
    const auto *copy = dynamic_cast<const LC_OffsetCurve *>(cloned.get()); REQUIRE(copy);
    CHECK(copy->unknownOwnedFlags() == 0x20);
    REQUIRE(copy->unknownOwnedTail().size() == 1);
    CHECK(std::get<QString>(copy->unknownOwnedTail().front().value) == QStringLiteral("kept"));
}
