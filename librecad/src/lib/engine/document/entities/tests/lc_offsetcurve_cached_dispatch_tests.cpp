/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <list>
#include <memory>
#include <vector>

#include <QImage>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_looputils.h"
#include "lc_offsetcurve.h"
#include "lc_offsetoutputbudget.h"
#include "lc_pathbuilder.h"
#include "rs_circle.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_pen.h"
#include "rs_preview.h"
#include "rs_text.h"
#include "rs_mtext.h"

namespace {
std::unique_ptr<LC_OffsetCurve> admittedRing(bool reversed = false) {
    REQUIRE(lc::test::application());
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9. / 8}, {9. / 8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto& point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned n = 0; n < (i == 0 || i == 4 ? 4u : 3u); ++n) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1);
    data.m_closedFlag = true;
    if (reversed) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    RS_Spline cache{nullptr, data};
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1};
    definition.distance = 1;
    definition.relTolerance = .01;
    definition.cacheReversed = reversed;
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(cache, definition, .02, &checked);
    CAPTURE(reversed, checked.reason);
    REQUIRE(checked);
    REQUIRE(curve);
    REQUIRE(curve->cacheFormsClosedChain());
    REQUIRE(curve->count() > 4);
    curve->setPen(RS_Pen{RS_Color{19, 83, 127}, RS2::Width07, RS2::DashLine});
    curve->setSelectionFlag(true);
    return curve;
}

void samePoint(const RS_Vector& actual, const RS_Vector& expected) {
    CHECK(actual.valid == expected.valid);
    CHECK(actual.x == expected.x);
    CHECK(actual.y == expected.y);
    CHECK(actual.z == expected.z);
}

struct Snapshot {
    const LC_OffsetCurve& curve;
    const RS_SplineData data;
    const LC_EllipseOffsetDefinition definition;
    RS_EntityContainer* const parent;
    const RS_Layer* const layer;
    const RS_Pen pen;
    const unsigned flags;
    const unsigned long long id, lineage;
    const bool defined, consistent;
    const std::vector<RS_Vector> grips;
    const RS_Vector middle;
    struct Child {
        const RS_Entity* entity;
        RS_EntityContainer* parent;
        const RS_Layer* layer;
        RS_Pen pen;
        unsigned flags;
        RS_Vector start, end;
    };
    std::vector<Child> children;

    explicit Snapshot(const LC_OffsetCurve& source) : curve(source), data(source.getData()),
        definition(source.definition()), parent(source.getParent()), layer(source.getLayer(false)),
        pen(source.getPen(false)), flags(source.getFlags()), id(source.getId()), lineage(source.lineage()),
        defined(source.isDefined()), consistent(source.isConsistent()), grips(source.cachedDefinitionGrips()),
        middle(source.getMiddlePoint()) {
        for (const auto* edge : source)
            children.push_back({edge, edge->getParent(), edge->getLayer(false), edge->getPen(false),
                               edge->getFlags(), edge->getStartpoint(), edge->getEndpoint()});
    }

    void check() const {
        const auto& actual = curve.getData();
        CHECK(actual.degree == data.degree);
        CHECK(actual.type == data.type);
        CHECK(actual.knotslist == data.knotslist);
        CHECK(actual.weights == data.weights);
        REQUIRE(actual.controlPoints.size() == data.controlPoints.size());
        for (std::size_t i = 0; i < data.controlPoints.size(); ++i) samePoint(actual.controlPoints[i], data.controlPoints[i]);
        CHECK(curve.getParent() == parent);
        CHECK(curve.getLayer(false) == layer);
        CHECK(curve.getPen(false) == pen);
        CHECK(curve.getFlags() == flags);
        CHECK(curve.getId() == id);
        CHECK(curve.lineage() == lineage);
        CHECK(curve.isDefined() == defined);
        CHECK(curve.isConsistent() == consistent);
        samePoint(curve.definition().basis.center, definition.basis.center);
        samePoint(curve.definition().basis.majorP, definition.basis.majorP);
        CHECK(curve.definition().distance == definition.distance);
        CHECK(curve.definition().cacheReversed == definition.cacheReversed);
        REQUIRE(curve.cachedDefinitionGrips().size() == grips.size());
        for (std::size_t i = 0; i < grips.size(); ++i) samePoint(curve.cachedDefinitionGrips()[i], grips[i]);
        samePoint(curve.getMiddlePoint(), middle);
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            const auto* edge = curve.entityAt(i);
            const auto& saved = children[i];
            CHECK(edge == saved.entity);
            CHECK(edge->getParent() == saved.parent);
            CHECK(edge->getLayer(false) == saved.layer);
            CHECK(edge->getPen(false) == saved.pen);
            CHECK(edge->getFlags() == saved.flags);
            samePoint(edge->getStartpoint(), saved.start);
            samePoint(edge->getEndpoint(), saved.end);
        }
    }
};

struct Painter {
    QImage image{256, 256, QImage::Format_RGB32};
    LC_GraphicViewport viewport;
    RS_Painter painter{&image};
    Painter() {
        viewport.setSize(256, 256);
        viewport.justSetOffsetAndFactor(117, 109, 23.);
        painter.setViewPort(&viewport);
    }
};

template<typename Entity>
struct ProxyProbe : Entity {
    using Entity::Entity;
    mutable unsigned calls = 0;
    RS_Entity* cloneProxy() const override {
        ++calls;
        return Entity::cloneProxy();
    }
};

struct ObservedLine : RS_Line {
    mutable unsigned reads = 0, clones = 0, deepCalls = 0, windowCalls = 0;
    ObservedLine() : RS_Line(nullptr, {0, 0}, {1, 1}) {}
    RS_Vector getStartpoint() const override { ++reads; return RS_Line::getStartpoint(); }
    RS_Vector getEndpoint() const override { ++reads; return RS_Line::getEndpoint(); }
    RS_Entity* clone() const override { ++clones; return new RS_Line(*this); }
    unsigned int countDeep() const override { ++deepCalls; return RS_Line::countDeep(); }
    bool hasEndpointsWithinWindow(const RS_Vector& a, const RS_Vector& b) const override {
        ++windowCalls;
        return RS_Line::hasEndpointsWithinWindow(a, b);
    }
};

// Start with real C admission, then model a hostile legacy display list without
// allocating a geometry object per alias. Restore ownership before destruction.
struct DisplayAliases {
    LC_OffsetCurve& curve;
    const QList<RS_Entity*> original;
    explicit DisplayAliases(LC_OffsetCurve& source, RS_Entity& edge, std::size_t count)
        : curve(source), original(source.getEntityList()) {
        curve.setOwner(false);
        curve.clear();
        for (std::size_t i = 0; i < count; ++i) curve.push_back(&edge);
    }
    ~DisplayAliases() {
        curve.clear();
        curve.setOwner(true);
        for (auto* edge : original) curve.push_back(edge);
        curve.forcedCalculateBorders();
    }
};

void sameProxy(const RS_Entity& actual, const RS_Entity& expected) {
    REQUIRE(actual.rtti() == expected.rtti());
    REQUIRE(actual.count() == expected.count());
    if (const auto* container = dynamic_cast<const RS_EntityContainer*>(&actual)) {
        const auto* other = dynamic_cast<const RS_EntityContainer*>(&expected);
        REQUIRE(other);
        for (std::size_t i = 0; i < container->count(); ++i)
            sameProxy(*container->entityAt(i), *other->entityAt(i));
    } else {
        samePoint(actual.getStartpoint(), expected.getStartpoint());
        samePoint(actual.getEndpoint(), expected.getEndpoint());
    }
}

void plainOwned(const RS_EntityContainer& container) {
    REQUIRE(container.isOwner());
    for (const auto* entity : container) {
        REQUIRE(entity);
        CHECK(entity->rtti() != RS2::EntityOffsetCurve);
        CHECK(entity->getParent() == &container);
        if (const auto* nested = dynamic_cast<const RS_EntityContainer*>(entity)) plainOwned(*nested);
    }
}

using Segment = std::array<double, 4>;
std::vector<Segment> segments(const RS_EntityContainer& container) {
    std::vector<Segment> result;
    for (const auto* entity : container) {
        REQUIRE(entity->rtti() == RS2::EntityLine);
        auto a = entity->getStartpoint(), b = entity->getEndpoint();
        if (std::array<double, 2>{b.x, b.y} < std::array<double, 2>{a.x, a.y}) std::swap(a, b);
        result.push_back({a.x, a.y, b.x, b.y});
    }
    std::sort(result.begin(), result.end());
    return result;
}

struct LoadedPattern : RS_Pattern {
    LoadedPattern() : RS_Pattern(QStringLiteral("cached-dispatch")) {
        m_loaded = true;
        setOwner(false);
    }
};

std::shared_ptr<RS_EntityContainer> rectangle() {
    auto result = std::make_shared<RS_EntityContainer>();
    const std::array<RS_Vector, 4> points{{{-3, -3}, {3, -3}, {3, 3}, {-3, 3}}};
    for (std::size_t i = 0; i < points.size(); ++i)
        result->addEntity(new RS_Line{result.get(), points[i], points[(i + 1) % points.size()]});
    return result;
}
}

TEST_CASE("Native previews are independently owned ordinary cache copies",
          "[lc-offsetcurve][types][cached-dispatch]") {
    for (bool reversed : {false, true}) {
        auto curve = admittedRing(reversed);
        const Snapshot saved{*curve};
        LC_GraphicViewport viewport;
        RS_Preview preview{nullptr, &viewport};
        preview.addEntity(curve->clone());
        REQUIRE(preview.count() == 1);
        auto* copy = dynamic_cast<RS_Spline*>(preview.first());
        REQUIRE(copy);
        CHECK(copy->rtti() == RS2::EntitySpline);
        CHECK(RS_Spline::geometryFingerprint(copy->getData()) == RS_Spline::geometryFingerprint(curve->getData()));
        CHECK(segments(*copy) == segments(*curve));
        plainOwned(preview);
        for (std::size_t i = 0; i < curve->count(); ++i) CHECK(copy->entityAt(i) != curve->entityAt(i));
        preview.move({7, -4});
        preview.clear();
        saved.check();
    }
}

TEST_CASE("Native preview clone APIs do not mutate borrowed source groups",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    LC_GraphicViewport viewport;
    RS_Preview preview{nullptr, &viewport};
    RS_EntityContainer borrowed{nullptr, false};
    borrowed.addEntity(curve.get());
    preview.addAllFrom(borrowed, nullptr);
    plainOwned(preview);
    REQUIRE(preview.count() == 1);
    CHECK(preview.first()->rtti() == RS2::EntitySpline);
    preview.clear();
    preview.addClonesFromList(QList<RS_Entity*>{curve.get()});
    plainOwned(preview);
    REQUIRE(preview.count() == 1);
    CHECK(preview.first()->rtti() == RS2::EntitySpline);
    preview.clear();

    auto* transferred = new RS_EntityContainer{nullptr, false};
    transferred->addEntity(curve.get());
    preview.addEntity(transferred);
    CHECK(preview.empty()); // borrowed native trees refuse without touching sources
    preview.clear();
    saved.check();
}

TEST_CASE("Native painter paths exactly follow displayed edges in both cache orders",
          "[lc-offsetcurve][types][cached-dispatch]") {
    for (bool reversed : {false, true}) {
        auto curve = admittedRing(reversed);
        const Snapshot saved{*curve};
        Painter rig;
        LC_LoopUtils::PathBuilder actual{&rig.painter}, expected{&rig.painter};
        actual.append(curve.get());
        for (auto* edge : *curve) expected.append(edge);
        REQUIRE(!expected.getPath().isEmpty());
        CHECK(actual.getPath() == expected.getPath());
        auto nativeLoop = std::make_shared<RS_EntityContainer>(nullptr, false);
        auto flatLoop = std::make_shared<RS_EntityContainer>(nullptr, false);
        nativeLoop->addEntity(curve.get());
        for (auto* edge : *curve) flatLoop->addEntity(edge);
        LC_LoopUtils::LC_Loops nativeLoops{nativeLoop, false}, flatLoops{flatLoop, false};
        CHECK(nativeLoops.getPainterPath(&rig.painter) == flatLoops.getPainterPath(&rig.painter));
        RS_Line next{nullptr, {2, 0}, {3, 1}};
        actual.append(&next);
        expected.append(&next);
        CHECK(actual.getPath() == expected.getPath());
        saved.check();
    }
}

TEST_CASE("Stale and undefined native path dispatch reads only displayed cache edges",
          "[lc-offsetcurve][types][cached-dispatch]") {
    for (bool undefined : {false, true}) {
        auto curve = admittedRing();
        if (undefined) curve->scale({0, 0}, {2, 1});
        else curve->getData().controlPoints[1].x += .125;
        REQUIRE(!curve->hasEditableDefinition());
        const Snapshot saved{*curve};
        Painter rig;
        LC_LoopUtils::PathBuilder actual{&rig.painter}, expected{&rig.painter};
        actual.append(curve.get());
        for (auto* edge : *curve) expected.append(edge);
        CHECK(actual.getPath() == expected.getPath());
        saved.check();
    }
}

TEST_CASE("Native loop extraction deduplicates borrowed cache aliases without source changes",
          "[lc-offsetcurve][types][cached-dispatch]") {
    for (bool reversed : {false, true}) {
        auto curve = admittedRing(reversed);
        const Snapshot saved{*curve};
        RS_EntityContainer mixed{nullptr, false}, flat{nullptr, false};
        mixed.addEntity(curve.get());
        for (auto* edge : *curve) {
            flat.addEntity(edge);
            mixed.addEntity(edge); // aliases of edges already reached through the native
        }
        LC_LoopUtils::LoopExtractor actual{mixed}, expected{flat};
        auto actualLoops = actual.extract(), expectedLoops = expected.extract();
        REQUIRE(expectedLoops.size() == 1);
        REQUIRE(actualLoops.size() == expectedLoops.size());
        CHECK(segments(*actualLoops.front()) == segments(*expectedLoops.front()));
        plainOwned(*actualLoops.front());
        for (const auto* edge : *actualLoops.front()) CHECK(edge->getPen(false) == curve->getPen(true));
        LC_LoopUtils::LoopSorter sorter{std::move(actualLoops)};
        auto ordered = sorter.getResults();
        REQUIRE(ordered);
        REQUIRE(ordered->size() == 1);
        const auto& cache = *ordered->front().getOuterLoop();
        REQUIRE(cache.isOwner());
        for (const auto* edge : cache) {
            CHECK(edge->getParent() == nullptr);
            CHECK(edge->getLayer(false) == nullptr);
            CHECK(edge->rtti() == RS2::EntityLine);
            CHECK(edge->getPen(false) == curve->getPen(true));
            for (const auto* original : *curve) CHECK(edge != original);
        }
        saved.check();
    }
}

TEST_CASE("Borrowed native patterns clone as independent attributed cached edges",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    RS_Line ordinary{nullptr, {-1, -1}, {1, 1}};
    ordinary.setPen(RS_Pen{RS_Color{41, 23, 91}, RS2::Width03, RS2::SolidLine});
    LoadedPattern pattern;
    pattern.addEntity(curve.get());
    pattern.addEntity(&ordinary);
    {
        std::unique_ptr<RS_Entity> cloned{pattern.clone()};
        auto* copy = dynamic_cast<RS_Pattern*>(cloned.get());
        REQUIRE(copy);
        REQUIRE(copy->count() == curve->count() + 1);
        plainOwned(*copy);
        for (std::size_t i = 0; i < curve->count(); ++i) {
            auto* edge = copy->entityAt(i);
            CHECK(edge != curve->entityAt(i));
            samePoint(edge->getStartpoint(), curve->entityAt(i)->getStartpoint());
            samePoint(edge->getEndpoint(), curve->entityAt(i)->getEndpoint());
            CHECK(edge->getPen(false) == curve->getPen(true));
        }
        CHECK(copy->last() != &ordinary);
        CHECK(copy->last()->getPen(false) == ordinary.getPen(false));
        copy->move({3, 4});
    }
    samePoint(ordinary.getStartpoint(), {-1, -1});
    samePoint(ordinary.getEndpoint(), {1, 1});
    CHECK(ordinary.getParent() == nullptr);
    RS_EntityContainer group{nullptr, false};
    group.addEntity(curve.get());
    LoadedPattern nested;
    nested.addEntity(&group);
    std::unique_ptr<RS_Entity> refused{nested.clone()};
    REQUIRE(dynamic_cast<RS_Pattern*>(refused.get()));
    CHECK(static_cast<RS_Pattern*>(refused.get())->empty());
    saved.check();
}

TEST_CASE("Native pattern trimming matches the existing ordinary displayed-edge pipeline",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    LoadedPattern native, flat;
    native.addEntity(curve.get());
    for (auto* edge : *curve) flat.addEntity(edge);
    native.forcedCalculateBorders();
    flat.forcedCalculateBorders();
    LC_LoopUtils::LC_Loops loops{rectangle()};
    auto actual = loops.trimPatternEntities(native);
    auto expected = loops.trimPatternEntities(flat);
    REQUIRE(actual);
    REQUIRE(expected);
    REQUIRE(!expected->empty());
    CHECK(segments(*actual) == segments(*expected));
    plainOwned(*actual);
    for (const auto* edge : *actual) CHECK(edge->getPen(false) == curve->getPen(true));
    saved.check();
}

TEST_CASE("Solid hatch consumes native cached edges without adopting borrowed sources",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    {
        RS_Hatch hatch{nullptr, RS_HatchData{true, 1, 0, QStringLiteral("SOLID")}};
        auto* boundary = new RS_EntityContainer{&hatch, false};
        boundary->addEntity(curve.get());
        hatch.addEntity(boundary);
        hatch.update();
        REQUIRE(hatch.getUpdateError() == RS_Hatch::HATCH_OK);
        REQUIRE(hatch.countAllLoops() == 1);
        CHECK(hatch.getTotalArea() > 10);
        CHECK(hatch.getTotalArea() < 14);
        saved.check();
    }
    saved.check();
}

TEST_CASE("Unsupported native display caches refuse whole path and pattern publication",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    curve->addEntity(new RS_Circle{curve.get(), RS_CircleData{{0, 0}, 1}});
    const Snapshot saved{*curve};
    Painter rig;
    LC_LoopUtils::PathBuilder builder{&rig.painter};
    RS_Line sentinel{nullptr, {-4, 0}, {-3, 1}};
    builder.append(&sentinel);
    const QPainterPath before = builder.getPath();
    builder.append(curve.get());
    CHECK(builder.getPath() == before);
    auto partial = std::make_shared<RS_EntityContainer>(nullptr, false);
    partial->addEntity(&sentinel);
    partial->addEntity(curve.get());
    LC_LoopUtils::LC_Loops partialLoop{partial, false};
    CHECK(partialLoop.getPainterPath(&rig.painter).isEmpty());
    LoadedPattern pattern;
    pattern.addEntity(&sentinel);
    pattern.addEntity(curve.get());
    std::unique_ptr<RS_Entity> cloned{pattern.clone()};
    auto* copy = dynamic_cast<RS_Pattern*>(cloned.get());
    REQUIRE(copy);
    CHECK(copy->empty());
    auto ordinary = rectangle();
    LC_LoopUtils::LoopExtractor baseline{*ordinary};
    REQUIRE(baseline.extract().size() == 1);
    for (bool nativeFirst : {false, true}) {
        RS_EntityContainer borrowed{nullptr, false};
        if (nativeFirst) borrowed.addEntity(curve.get());
        for (auto* edge : *ordinary) borrowed.addEntity(edge);
        if (!nativeFirst) borrowed.addEntity(curve.get());
        LC_LoopUtils::LoopExtractor extractor{borrowed};
        CHECK(extractor.extract().empty());
    }
    saved.check();
}

TEST_CASE("Native loop extraction refuses coordinates outside its endpoint-key range",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    curve->move({1e12, 0});
    REQUIRE(curve->first()->getStartpoint().x > 1e11);
    const Snapshot saved{*curve};
    auto ordinary = rectangle();
    LC_LoopUtils::LoopExtractor baseline{*ordinary};
    REQUIRE(baseline.extract().size() == 1);
    for (bool nativeFirst : {false, true}) {
        RS_EntityContainer borrowed{nullptr, false};
        if (nativeFirst) borrowed.addEntity(curve.get());
        for (auto* edge : *ordinary) borrowed.addEntity(edge);
        if (!nativeFirst) borrowed.addEntity(curve.get());
        LC_LoopUtils::LoopExtractor extractor{borrowed};
        CHECK(extractor.extract().empty());
    }
    saved.check();
}

TEST_CASE("Ordinary spline path refusal and ordinary primitive paths remain unchanged",
          "[lc-offsetcurve][types][cached-dispatch]") {
    REQUIRE(lc::test::application());
    Painter rig;
    LC_LoopUtils::PathBuilder builder{&rig.painter};
    RS_Line line{nullptr, {0, 0}, {1, 1}};
    builder.append(&line);
    const QPainterPath before = builder.getPath();
    RS_SplineData data{1, false};
    data.controlPoints = {{1, 1}, {2, 0}};
    data.weights = {1, 1};
    data.knotslist = {0, 0, 1, 1};
    RS_Spline ordinary{nullptr, data};
    builder.append(&ordinary);
    CHECK(builder.getPath() == before);
    const auto gui = rig.painter.toGui(line.getEndpoint());
    CHECK((before.currentPosition() == QPointF{gui.x, gui.y}));
}

TEST_CASE("Ordinary preview APIs retain exact legacy proxy dispatch",
          "[lc-offsetcurve][types][cached-dispatch]") {
    REQUIRE(lc::test::application());
    RS_SplineData data{1, false};
    data.controlPoints = {{0, 0}, {1, 1}};
    data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    ProxyProbe<RS_Spline> spline{nullptr, data};
    RS_TextData textData;
    textData.insertionPoint = {3, 4}; textData.height = 2;
    textData.widthRel = 1; textData.text = QStringLiteral("legacy");
    textData.updateMode = RS2::NoUpdate;
    ProxyProbe<RS_Text> text{nullptr, textData};
    ProxyProbe<RS_MText> mtext{nullptr, RS_MTextData{}};
    mtext.addEntity(new RS_Line{&mtext, {0, 0}, {1, 1}});
    ProxyProbe<RS_Insert> insert{nullptr, RS_InsertData{QStringLiteral("fixture"),
        {0, 0}, {1, 1, 1}, 0, 1, 1, {0, 0}, nullptr, RS2::NoUpdate}};
    insert.addEntity(new RS_Line{&insert, {0, 0}, {1, 1}});
    ProxyProbe<RS_Hatch> hatch{nullptr, RS_HatchData{true, 1, 0, QStringLiteral("SOLID")}};
    ProxyProbe<RS_Image> image{nullptr, RS_ImageData{0, {1, 2}, {1, 0}, {0, 1}, {2, 3}, QString{}, 50, 50, 0}};
    const auto check = [](auto& source) {
        LC_GraphicViewport viewport;
        RS_Preview expected{nullptr, &viewport};
        expected.addEntity(source.cloneProxy());
        for (bool fromList : {false, true}) {
            source.calls = 0;
            RS_Preview actual{nullptr, &viewport};
            if (fromList) actual.addClonesFromList(QList<RS_Entity*>{&source});
            else {
                RS_EntityContainer borrowed{nullptr, false};
                borrowed.addEntity(&source);
                actual.addAllFrom(borrowed, nullptr);
            }
            CHECK(source.calls == 1);
            sameProxy(actual, expected);
        }
    };
    check(spline); check(text); check(mtext); check(insert); check(hatch); check(image);
    ProxyProbe<RS_Text> generatedText{nullptr, textData};
    ProxyProbe<RS_MText> generatedMText{nullptr, RS_MTextData{}};
    generatedText.setOwner(false); generatedMText.setOwner(false);
    ObservedLine glyph;
    for (std::size_t i = 0; i <= kDefaultOffsetDeepEntitiesPerSource; ++i) {
        generatedText.push_back(&glyph); generatedMText.push_back(&glyph);
    }
    check(generatedText); check(generatedMText);
    CHECK(glyph.reads == 0);
    CHECK(glyph.clones == 0);
}

TEST_CASE("Native preview staging refuses specialized paths and malformed legacy data",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    LC_GraphicViewport viewport;
    RS_Preview preview{nullptr, &viewport};
    RS_Insert specialized{nullptr, RS_InsertData{QStringLiteral("fixture"),
        {0, 0}, {1, 1, 1}, 0, 1, 1, {0, 0}, nullptr, RS2::NoUpdate}};
    specialized.setOwner(false);
    specialized.addEntity(curve.get());
    preview.addClonesFromList(QList<RS_Entity*>{&specialized});
    CHECK(preview.empty());
    saved.check();
    const auto original = curve->getData();
    curve->getData().controlPoints.resize(LC_OffsetCheckLimits{}.maxVertices + 1, {0, 0});
    preview.addClonesFromList(QList<RS_Entity*>{curve.get()});
    CHECK(preview.empty());
    CHECK(curve->getData().controlPoints.size() == LC_OffsetCheckLimits{}.maxVertices + 1);
    curve->getData() = original;
    saved.check();
}

TEST_CASE("Native display caps refuse before edge callbacks or clones",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    ObservedLine edge;
    {
        DisplayAliases aliases{*curve, edge, kDefaultOffsetDeepEntitiesPerSource + 1};
        Painter rig;
        LC_LoopUtils::PathBuilder builder{&rig.painter};
        builder.append(curve.get());
        CHECK(builder.getPath().isEmpty());
        RS_EntityContainer borrowed{nullptr, false};
        borrowed.addEntity(curve.get());
        LC_LoopUtils::LoopExtractor extractor{borrowed};
        CHECK(extractor.extract().empty());
        LoadedPattern pattern;
        pattern.addEntity(curve.get());
        std::unique_ptr<RS_Entity> cloned{pattern.clone()};
        REQUIRE(dynamic_cast<RS_Pattern*>(cloned.get()));
        CHECK(static_cast<RS_Pattern*>(cloned.get())->empty());
        RS_Preview preview{nullptr, &rig.viewport};
        preview.addClonesFromList(QList<RS_Entity*>{curve.get()});
        CHECK(preview.empty());
        CHECK(edge.reads == 0);
        CHECK(edge.clones == 0);
    }
    saved.check();
}

TEST_CASE("Native aggregate display caps precede all edge walks",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto first = admittedRing(), second = admittedRing();
    ObservedLine edge;
    DisplayAliases a{*first, edge, kDefaultOffsetDeepEntitiesPerSource / 2 + 1};
    DisplayAliases b{*second, edge, kDefaultOffsetDeepEntitiesPerSource / 2 + 1};
    RS_EntityContainer borrowed{nullptr, false};
    borrowed.addEntity(first.get()); borrowed.addEntity(second.get());
    LC_LoopUtils::LoopExtractor extractor{borrowed};
    CHECK(extractor.extract().empty());
    LoadedPattern pattern;
    pattern.addEntity(first.get()); pattern.addEntity(second.get());
    std::unique_ptr<RS_Entity> cloned{pattern.clone()};
    REQUIRE(dynamic_cast<RS_Pattern*>(cloned.get()));
    CHECK(static_cast<RS_Pattern*>(cloned.get())->empty());
    CHECK(edge.reads == 0);
    CHECK(edge.clones == 0);
}

TEST_CASE("Native display dispatch refuses finite nonzero Z before publication",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    auto* edge = dynamic_cast<RS_Line*>(curve->first());
    REQUIRE(edge);
    auto start = edge->getStartpoint(); start.z = 1;
    edge->setStartpoint(start);
    const Snapshot saved{*curve};
    Painter rig;
    LC_LoopUtils::PathBuilder builder{&rig.painter};
    builder.append(curve.get());
    CHECK(builder.getPath().isEmpty());
    RS_EntityContainer borrowed{nullptr, false};
    borrowed.addEntity(curve.get());
    LC_LoopUtils::LoopExtractor extractor{borrowed};
    CHECK(extractor.extract().empty());
    LoadedPattern pattern;
    pattern.addEntity(curve.get());
    std::unique_ptr<RS_Entity> cloned{pattern.clone()};
    REQUIRE(dynamic_cast<RS_Pattern*>(cloned.get()));
    CHECK(static_cast<RS_Pattern*>(cloned.get())->empty());
    RS_Preview preview{nullptr, &rig.viewport};
    preview.addClonesFromList(QList<RS_Entity*>{curve.get()});
    CHECK(preview.empty());
    saved.check();
}

TEST_CASE("Ordinary owned pattern containers retain legacy typed clones",
          "[lc-offsetcurve][types][cached-dispatch]") {
    REQUIRE(lc::test::application());
    LoadedPattern pattern;
    pattern.setOwner(true);
    RS_SplineData data{1, false};
    data.controlPoints = {{0, 0}, {1, 1}};
    data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    auto* spline = new RS_Spline{&pattern, data};
    pattern.addEntity(spline);
    std::unique_ptr<RS_Entity> cloned{pattern.clone()};
    auto* copy = dynamic_cast<RS_Pattern*>(cloned.get());
    REQUIRE(copy);
    REQUIRE(copy->count() == 1);
    CHECK(copy->first()->rtti() == RS2::EntitySpline);
    REQUIRE(dynamic_cast<RS_Spline*>(copy->first()));
    CHECK(RS_Spline::geometryFingerprint(static_cast<RS_Spline*>(copy->first())->getData()) ==
          RS_Spline::geometryFingerprint(spline->getData()));
}

TEST_CASE("Native pattern cloning refuses borrowed descendants of owned siblings before cloning",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    ObservedLine line;
    RS_EntityContainer owned;
    auto* borrowed = new RS_EntityContainer{&owned, false};
    borrowed->addEntity(&line);
    owned.addEntity(borrowed);
    for (bool nativeFirst : {false, true}) {
        LoadedPattern pattern;
        if (nativeFirst) pattern.addEntity(curve.get());
        pattern.addEntity(&owned);
        if (!nativeFirst) pattern.addEntity(curve.get());
        std::unique_ptr<RS_Entity> cloned{pattern.clone()};
        REQUIRE(dynamic_cast<RS_Pattern*>(cloned.get()));
        CHECK(static_cast<RS_Pattern*>(cloned.get())->empty());
        cloned->move({4, 7});
        CHECK(line.clones == 0);
        samePoint(line.getStartpoint(), {0, 0});
        samePoint(line.getEndpoint(), {1, 1});
        CHECK(line.getParent() == nullptr);
        saved.check();
    }
}

TEST_CASE("Adopting preview lists preflight oversized and cyclic native trees before recursive counts",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    ObservedLine line;
    {
        DisplayAliases aliases{*curve, line, kDefaultOffsetDeepEntitiesPerSource + 1};
        for (bool standardList : {false, true}) for (bool cyclic : {false, true}) {
            auto* tree = new RS_EntityContainer;
            if (cyclic) {
                auto* cycle = new RS_EntityContainer{tree, false};
                cycle->push_back(tree);
                tree->push_back(cycle);
            }
            auto* borrowed = new RS_EntityContainer{tree, false};
            borrowed->push_back(curve.get());
            tree->push_back(borrowed);
            LC_GraphicViewport viewport;
            RS_Preview preview{nullptr, &viewport};
            auto* ordinary = new RS_Line{nullptr, {-1, -1}, {1, 1}};
            if (standardList) preview.addAllFromList(std::list<RS_Entity*>{tree, ordinary});
            else preview.addAllFromList(QList<RS_Entity*>{tree, ordinary});
            REQUIRE(preview.count() == 1);
            CHECK(preview.first()->rtti() == RS2::EntityLine);
            CHECK(line.deepCalls == 0);
            CHECK(line.windowCalls == 0);
            CHECK(line.clones == 0);
        }
    }
    saved.check();
}

TEST_CASE("Stretch preview preflights partial-window native trees before endpoint recursion",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    const Snapshot saved{*curve};
    ObservedLine line;
    {
        DisplayAliases aliases{*curve, line, kDefaultOffsetDeepEntitiesPerSource + 1};
        for (bool cyclic : {false, true}) {
            RS_EntityContainer tree;
            if (cyclic) {
                auto* cycle = new RS_EntityContainer{&tree, false};
                cycle->push_back(&tree);
                tree.push_back(cycle);
            }
            auto* borrowed = new RS_EntityContainer{&tree, false};
            borrowed->push_back(curve.get());
            tree.push_back(borrowed);
            tree.addEntity(new RS_Line{&tree, {-2, -2}, {2, 2}});
            RS_EntityContainer sources{nullptr, false};
            sources.addEntity(&tree);
            RS_Line ordinary{nullptr, {0, 0}, {1, 1}};
            sources.addEntity(&ordinary);
            LC_GraphicViewport viewport;
            RS_Preview preview{nullptr, &viewport};
            preview.addStretchablesFrom(sources, nullptr, {-.1, -.1}, {.1, .1});
            REQUIRE(preview.count() == 1);
            CHECK(preview.first()->rtti() == RS2::EntityLine);
            CHECK(line.deepCalls == 0);
            CHECK(line.windowCalls == 0);
            CHECK(line.clones == 0);
        }
    }
    saved.check();
}

TEST_CASE("Deleted native inputs never reappear through adopting or clone preview APIs",
          "[lc-offsetcurve][types][cached-dispatch]") {
    auto curve = admittedRing();
    curve->setFlag(RS2::FlagDeleted);
    const Snapshot saved{*curve};
    LC_GraphicViewport viewport;
    RS_Preview preview{nullptr, &viewport};
    preview.addClonesFromList(QList<RS_Entity*>{curve.get()});
    CHECK(preview.empty());
    RS_EntityContainer borrowed{nullptr, false};
    borrowed.push_back(curve.get());
    preview.addAllFrom(borrowed, nullptr);
    CHECK(preview.empty());
    for (int route = 0; route < 3; ++route) {
        auto* transferred = curve->clone();
        transferred->setFlag(RS2::FlagDeleted);
        if (route == 0) preview.addAllFromList(QList<RS_Entity*>{transferred});
        else if (route == 1) preview.addAllFromList(std::list<RS_Entity*>{transferred});
        else preview.addEntity(transferred);
        CHECK(preview.empty());
    }
    saved.check();
}
