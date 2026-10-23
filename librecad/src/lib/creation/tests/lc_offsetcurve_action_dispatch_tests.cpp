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
#include <type_traits>
#include <vector>

#include "lc_action_draw_line_parallel.h"
#include "lc_action_draw_line_parallel_through.h"
#include "lc_action_modify_offset.h"
#include "lc_action_modify_revert_direction.h"
#include "lc_action_spline_add_point.h"
#include "lc_action_spline_append_point.h"
#include "lc_action_spline_modify_explode.h"
#include "lc_action_spline_remove_between.h"
#include "lc_action_spline_remove_points.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "rs_creation.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_selection.h"

namespace {
struct Drawing {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    Drawing() {
        graphic.initForNewDocument();
        graphic.onLoadingCompleted();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
    }
};

struct Pending {
    LC_DocumentModificationBatch batch;
    ~Pending() { qDeleteAll(batch.entitiesToAdd); }
};

struct Output {
    QList<RS_Entity*> entities;
    ~Output() { qDeleteAll(entities); }
};

LC_OffsetCurve* native(Drawing& drawing, bool whole = false, bool publish = true) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, whole ? 0 : .1, false};
    definition.distance = whole ? 1 : .25;
    definition.relTolerance = .01;
    std::unique_ptr<RS_Entity> carrier;
    if (!whole) {
        carrier = std::make_unique<RS_Line>(&drawing.graphic, RS_Vector{1.25, 0},
            RS_Vector{1.25 * std::cos(.1), 1.25 * std::sin(.1)});
    } else {
        RS_SplineData data{3, false};
        std::array<RS_Vector, 4> net{{{2, 0}, {2, 9. / 8}, {9. / 8, 2}, {0, 2}}};
        for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
            for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
            for (auto& p : net) p = {-p.y, p.x};
        }
        for (unsigned i = 0; i <= 4; ++i)
            for (unsigned n = 0; n < (i == 0 || i == 4 ? 4u : 3u); ++n) data.knotslist.push_back(i);
        data.weights.assign(data.controlPoints.size(), 1);
        data.m_closedFlag = true;
        carrier = std::make_unique<RS_Spline>(&drawing.graphic, data);
    }
    LC_OffsetCheckResult checked;
    auto curve = LC_OffsetCurve::adopt(*carrier, definition, whole ? .02 : .01, &checked);
    CAPTURE(checked.reason);
    REQUIRE(checked);
    REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition());
    curve->setLayer(drawing.graphic.getActiveLayer());
    curve->setPen(RS_Pen{RS_Color{31, 47, 79}, RS2::Width11, RS2::DashLine});
    auto* result = curve.release();
    if (publish) drawing.graphic.addEntity(result);
    return result;
}

void point(const RS_Vector& a, const RS_Vector& b) {
    CHECK(a.valid == b.valid);
    CHECK(a.x == Catch::Approx(b.x).margin(1e-12));
    CHECK(a.y == Catch::Approx(b.y).margin(1e-12));
    CHECK(a.z == b.z);
}

void points(const std::vector<RS_Vector>& a, const std::vector<RS_Vector>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) { CAPTURE(i); point(a[i], b[i]); }
}

void exactPoint(const RS_Vector& a, const RS_Vector& b) {
    CHECK(a.valid == b.valid);
    CHECK(a.x == b.x);
    CHECK(a.y == b.y);
    CHECK(a.z == b.z);
}

void cache(const RS_SplineData& a, const RS_SplineData& b) {
    CHECK(a.degree == b.degree);
    CHECK(a.type == b.type);
    CHECK(a.savedOpenType == b.savedOpenType);
    CHECK(a.knotslist == b.knotslist);
    CHECK(a.weights == b.weights);
    CHECK(a.savedOpenKnots == b.savedOpenKnots);
    CHECK(a.m_closedFlag == b.m_closedFlag);
    CHECK(a.m_periodicFlag == b.m_periodicFlag);
    points(a.controlPoints, b.controlPoints);
    points(a.fitPoints, b.fitPoints);
}

struct Source {
    const LC_OffsetCurve& curve;
    const RS_SplineData data;
    const LC_EllipseOffsetDefinition definition;
    const std::vector<RS_Vector> grips;
    const RS_Vector* storage;
    const unsigned long long id;
    const unsigned long long lineage;
    RS_EntityContainer* const parent;
    RS_Layer* const layer;
    const RS_Pen pen;
    const bool defined;
    const bool consistent;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> ends;
    explicit Source(const LC_OffsetCurve& c) : curve(c), data(c.getData()), definition(c.definition()),
        grips(c.cachedDefinitionGrips()), storage(c.getData().controlPoints.data()), id(c.getId()),
        lineage(c.lineage()), parent(c.getParent()), layer(c.getLayer(false)), pen(c.getPen(false)),
        defined(c.isDefined()), consistent(c.isConsistent()) {
        for (const auto* child : c) {
            children.push_back(child);
            ends.push_back(child->getStartpoint());
            ends.push_back(child->getEndpoint());
        }
    }
    void check(bool stableObjects = true) const {
        cache(curve.getData(), data);
        for (std::size_t i = 0; i < data.controlPoints.size(); ++i)
            exactPoint(curve.getData().controlPoints[i], data.controlPoints[i]);
        if (stableObjects) CHECK(curve.getData().controlPoints.data() == storage);
        CHECK(curve.getId() == id);
        CHECK(curve.lineage() == lineage);
        CHECK(curve.getParent() == parent);
        CHECK(curve.getLayer(false) == layer);
        CHECK(curve.getPen(false) == pen);
        CHECK(curve.isDefined() == defined);
        CHECK(curve.isConsistent() == consistent);
        exactPoint(curve.definition().basis.center, definition.basis.center);
        exactPoint(curve.definition().basis.majorP, definition.basis.majorP);
        CHECK(curve.definition().basis.ratio == definition.basis.ratio);
        CHECK(curve.definition().basis.angle1 == definition.basis.angle1);
        CHECK(curve.definition().basis.angle2 == definition.basis.angle2);
        CHECK(curve.definition().basis.reversed == definition.basis.reversed);
        CHECK(curve.definition().distance == definition.distance);
        CHECK(curve.definition().towardCentre == definition.towardCentre);
        CHECK(curve.definition().cacheReversed == definition.cacheReversed);
        points(curve.cachedDefinitionGrips(), grips);
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            const auto* actual = curve.entityAt(static_cast<int>(i));
            REQUIRE(actual);
            if (stableObjects) CHECK(actual == children[i]);
            exactPoint(actual->getStartpoint(), ends[2 * i]);
            exactPoint(actual->getEndpoint(), ends[2 * i + 1]);
        }
    }
};

RS_Vector sidePoint(const RS_Spline& spline, double distance = .2) {
    const auto a = spline.getStartpoint(), b = spline.getEndpoint();
    const auto d = b - a;
    REQUIRE(d.magnitude() > RS_TOLERANCE);
    return (a + b) / 2 + RS_Vector{-d.y, d.x} * (distance / d.magnitude());
}

void sameGeometry(const RS_Entity& a, const RS_Entity& b) {
    REQUIRE(a.rtti() == b.rtti());
    REQUIRE(a.rtti() != RS2::EntityOffsetCurve);
    if (a.rtti() == RS2::EntitySpline) {
        cache(static_cast<const RS_Spline&>(a).getData(), static_cast<const RS_Spline&>(b).getData());
    } else if (a.isContainer()) {
        const auto& x = static_cast<const RS_EntityContainer&>(a);
        const auto& y = static_cast<const RS_EntityContainer&>(b);
        REQUIRE(x.count() == y.count());
        for (unsigned i = 0; i < x.count(); ++i) sameGeometry(*x.entityAt(i), *y.entityAt(i));
    } else {
        point(a.getStartpoint(), b.getStartpoint());
        point(a.getEndpoint(), b.getEndpoint());
    }
}

void sameOutput(const Output& a, const Output& b) {
    REQUIRE_FALSE(a.entities.isEmpty());
    REQUIRE(a.entities.size() == b.entities.size());
    for (int i = 0; i < a.entities.size(); ++i) sameGeometry(*a.entities[i], *b.entities[i]);
}

class ParallelProbe final : public LC_ActionDrawLineParallel {
public:
    explicit ParallelProbe(LC_ActionContext* context) : LC_ActionDrawLineParallel(context, RS2::ActionDrawLineParallel) {}
    using LC_ActionDrawLineParallel::doInitWithContextEntity;
    using LC_ActionDrawLineParallel::doTriggerModifications;
    void hover(const RS_Vector& p) {
        const auto e = lc::test::eventAt(p.x, p.y);
        onMouseMoveEvent(InitialActionStatus, &e);
    }
    void click(const RS_Vector& p) {
        const auto e = lc::test::eventAt(p.x, p.y);
        onMouseLeftButtonRelease(InitialActionStatus, &e);
    }
};

class ThroughProbe final : public LC_ActionDrawLineParallelThrough {
public:
    explicit ThroughProbe(LC_ActionContext* context) : LC_ActionDrawLineParallelThrough(context) {}
    using LC_ActionDrawLineParallelThrough::doInitWithContextEntity;
    using LC_ActionDrawLineParallelThrough::doTriggerModifications;
    RS_Entity* source() const { return m_entity; }
    void through(const RS_Vector& p) { *m_coord = p; }
    void choose(const RS_Vector& p) {
        const auto e = lc::test::eventAt(p.x, p.y);
        onMouseLeftButtonRelease(SetEntity, &e);
    }
};

class OffsetProbe final : public LC_ActionModifyOffset {
public:
    explicit OffsetProbe(LC_ActionContext* context) : LC_ActionModifyOffset(context) {}
    bool acceptsNative() const { return m_catchForSelectionEntityTypes.contains(RS2::EntityOffsetCurve); }
    bool run(RS_Entity* entity, const RS_Vector& p, LC_DocumentModificationBatch& batch) {
        m_selectedEntities = {entity};
        m_offsetData->coord = p;
        m_offsetData->distance = .125;
        m_offsetData->keepOriginals = true;
        m_offsetData->useCurrentLayer = false;
        m_offsetData->useCurrentAttributes = false;
        CHECK_FALSE(m_offsetData->asOffsetCurve);
        return doTriggerModifications(batch);
    }
};

class ReverseProbe final : public LC_ActionModifyRevertDirection {
public:
    explicit ReverseProbe(LC_ActionContext* context) : LC_ActionModifyRevertDirection(context) {}
    using LC_ActionModifyRevertDirection::isEntityAllowedToSelect;
    bool run(const QList<RS_Entity*>& entities, LC_DocumentModificationBatch& batch) {
        m_selectedEntities = entities;
        return doTriggerModifications(batch);
    }
};

class ExplodeProbe final : public LC_ActionSplineExplode {
public:
    explicit ExplodeProbe(LC_ActionContext* context) : LC_ActionSplineExplode(context) {
        setUseCustomSegmentsCount(true);
        setSegmentsCountValue(8);
        setUseCurrentLayer(false);
        setUseCurrentAttributes(false);
    }
    using LC_ActionSplineExplode::mayModifySplineEntity;
    using LC_ActionSplineExplode::createModifiedSplineEntity;
    using LC_ActionSplineExplode::fillStrokePoints;
    bool run(RS_Entity* entity, LC_DocumentModificationBatch& batch) {
        m_entityToModify = entity;
        return doTriggerModifications(batch);
    }
};

template <typename Action>
class ControlProbe final : public Action {
public:
    explicit ControlProbe(LC_ActionContext* context) : Action(context) {}
    using Action::mayModifySplineEntity;
    using LC_ActionSplineModifyBase::doInitWithContextEntity;
    RS_Entity* source() const { return this->m_entityToModify; }
    void click(const RS_Vector& point) {
        const auto event = lc::test::eventAt(point.x, point.y);
        this->onMouseLeftButtonRelease(this->getStatus(), &event);
    }
};

template <typename Action>
void excludesNative(Drawing& drawing, LC_OffsetCurve& curve, RS_Spline& plain) {
    ControlProbe<Action> action(&drawing.context);
    CHECK_FALSE(action.mayModifySplineEntity(&curve));
    CHECK(action.mayModifySplineEntity(&plain));
    action.doInitWithContextEntity(&curve, curve.getStartpoint());
    CHECK(action.source() == nullptr);
    action.doInitWithContextEntity(curve.entityAt(0), curve.getStartpoint());
    CHECK(action.source() == nullptr);
}
}

TEST_CASE("F actions parallel creation offsets the actual native cache as plain geometry", "[lc-offsetcurve][f-actions][parallel]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    const auto plain = curve->toPlainSpline();
    const auto p = sidePoint(*curve);
    Output actual, expected;
    RS_Creation::createParallel(p, .125, 2, curve, false, actual.entities);
    RS_Creation::createParallel(p, .125, 2, plain.get(), false, expected.entities);
    sameOutput(actual, expected);
    REQUIRE(actual.entities.size() == 2);
    for (const auto* entity : actual.entities) {
        CHECK(entity->rtti() == RS2::EntityLine);
        CHECK(entity->getLength() == Catch::Approx(curve->getLength()));
        CHECK(entity->getParent() == nullptr);
    }
    CHECK(actual.entities.front()->getStartpoint().distanceTo(curve->getStartpoint()) == Catch::Approx(.125));
    before.check();
}

TEST_CASE("F actions direct parallel creation promotes a native chord to the full cache", "[lc-offsetcurve][f-actions][parallel]") {
    Drawing drawing;
    auto* curve = native(drawing);
    Output owner, chord;
    const auto p = sidePoint(*curve);
    RS_Creation::createParallel(p, .125, 1, curve, false, owner.entities);
    RS_Creation::createParallel(p, .125, 1, curve->entityAt(0), false, chord.entities);
    sameOutput(chord, owner);
    CHECK(chord.entities.front()->getLength() == Catch::Approx(curve->getLength()));
}

TEST_CASE("F actions native parallel preview uses the ordinary spline preview engine", "[lc-offsetcurve][f-actions][parallel]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const auto plain = curve->toPlainSpline();
    Output actual, expected;
    const auto p = sidePoint(*curve);
    RS_Creation::createParallel(p, .125, 1, curve, false, actual.entities, true);
    RS_Creation::createParallel(p, .125, 1, plain.get(), false, expected.entities, true);
    sameOutput(actual, expected);
    CHECK(curve->hasEditableDefinition());
}

TEST_CASE("F actions native parallels through a point derive distance from the plain cache", "[lc-offsetcurve][f-actions][parallel]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    const auto plain = curve->toPlainSpline();
    const auto p = sidePoint(*curve);
    for (bool within : {false, true}) {
        CAPTURE(within);
        Output actual, expected;
        RS_Creation::createParallelThrough(p, 2, curve->entityAt(0), false, within, actual.entities);
        RS_Creation::createParallelThrough(p, 2, plain.get(), false, within, expected.entities);
        sameOutput(actual, expected);
        CHECK(actual.entities.back()->getStartpoint().distanceTo(curve->getStartpoint()) ==
            Catch::Approx(within ? .2 : .4).margin(1e-10));
    }
    before.check();
}

TEST_CASE("F actions native parallel requests refuse invalid bounds without adding a prefix", "[lc-offsetcurve][f-actions][parallel][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    for (int count : {0, -1, 100, std::numeric_limits<int>::max()}) {
        CAPTURE(count);
        Output output;
        RS_Creation::createParallel(sidePoint(*curve), .125, count, curve, false, output.entities);
        RS_Creation::createParallelThrough(sidePoint(*curve), count, curve, false, true, output.entities);
        CHECK(output.entities.isEmpty());
    }
    Output output;
    RS_Creation::createParallel({std::numeric_limits<double>::quiet_NaN(), 0}, .125, 1, curve, false, output.entities);
    RS_Creation::createParallel(sidePoint(*curve), std::numeric_limits<double>::infinity(), 1, curve, false, output.entities);
    CHECK(output.entities.isEmpty());
    before.check();
}

TEST_CASE("F actions malformed native cache refuses parallel creation without consuming existing output", "[lc-offsetcurve][f-actions][parallel][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    curve->getData().degree = std::numeric_limits<std::size_t>::max();
    Output output;
    auto* prefix = new RS_Line(nullptr, {0, 0}, {1, 0});
    output.entities.append(prefix);
    RS_Creation::createParallel({2, .2}, .125, 1, curve, false, output.entities);
    RS_Creation::createParallelCurve({2, .2}, .125, 1, curve, output.entities);
    RS_Creation::createParallelThrough({2, .2}, 1, curve, false, false, output.entities);
    REQUIRE(output.entities.size() == 1);
    CHECK(output.entities.front() == prefix);
    CHECK(curve->getData().degree == std::numeric_limits<std::size_t>::max());
    CHECK_FALSE(curve->isDeleted());
}

TEST_CASE("F actions stale but finite native geometry offsets as plain without restoring editability", "[lc-offsetcurve][f-actions][parallel]") {
    Drawing drawing;
    auto* curve = native(drawing);
    curve->getData().controlPoints.front().y += .01;
    REQUIRE_FALSE(curve->hasEditableDefinition());
    const Source before(*curve);
    RS_Spline plain{nullptr, curve->getData()};
    Output actual, expected;
    const auto p = sidePoint(plain);
    RS_Creation::createParallel(p, .125, 1, curve, false, actual.entities);
    RS_Creation::createParallel(p, .125, 1, &plain, false, expected.entities);
    sameOutput(actual, expected);
    before.check();
    CHECK_FALSE(curve->hasEditableDefinition());
}

TEST_CASE("F actions actual parallel hover and click create full-cache plain output undoably", "[lc-offsetcurve][f-actions][parallel][actions]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    ParallelProbe action(&drawing.context);
    action.setDistance(.125);
    const auto p = sidePoint(*curve);
    action.hover(p);
    action.click(p);
    REQUIRE(drawing.graphic.count() == 2);
    const auto* made = drawing.graphic.last();
    REQUIRE(made != curve);
    CHECK(made->rtti() == RS2::EntityLine);
    CHECK(made->getLength() == Catch::Approx(curve->getLength()));
    REQUIRE(drawing.graphic.undo());
    CHECK(made->isDeleted());
    CHECK_FALSE(curve->isDeleted());
    REQUIRE(drawing.graphic.redo());
    CHECK_FALSE(made->isDeleted());
    before.check(false);
}

TEST_CASE("F actions native context chords are accepted by parallel and through actions", "[lc-offsetcurve][f-actions][parallel][actions]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    const auto p = sidePoint(*curve);
    ParallelProbe parallel(&drawing.context);
    parallel.setDistance(.125);
    parallel.doInitWithContextEntity(curve->entityAt(0), p);
    Pending a;
    REQUIRE(parallel.doTriggerModifications(a.batch));
    REQUIRE_FALSE(a.batch.entitiesToAdd.isEmpty());
    CHECK(a.batch.entitiesToAdd.front()->getLength() == Catch::Approx(curve->getLength()));
    ThroughProbe through(&drawing.context);
    through.doInitWithContextEntity(curve->entityAt(0), curve->getStartpoint());
    REQUIRE(through.source() == curve);
    through.through(p);
    Pending b;
    REQUIRE(through.doTriggerModifications(b.batch));
    REQUIRE_FALSE(b.batch.entitiesToAdd.isEmpty());
    CHECK(b.batch.entitiesToAdd.front()->rtti() == RS2::EntityLine);
    CHECK(b.batch.entitiesToAdd.front()->getLength() == Catch::Approx(curve->getLength()));
    before.check();
}

TEST_CASE("F actions parallel callbacks report actual native refusal without claiming a modification", "[lc-offsetcurve][f-actions][parallel][actions][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    ParallelProbe parallel(&drawing.context);
    parallel.setNumber(100);
    parallel.doInitWithContextEntity(curve->entityAt(0), {2, .2});
    Pending a;
    CHECK_FALSE(parallel.doTriggerModifications(a.batch));
    CHECK(a.batch.entitiesToAdd.isEmpty());
    ThroughProbe through(&drawing.context);
    through.doInitWithContextEntity(curve->entityAt(0), curve->getStartpoint());
    through.setNumber(100);
    through.through({2, .2});
    Pending b;
    CHECK_FALSE(through.doTriggerModifications(b.batch));
    CHECK(b.batch.entitiesToAdd.isEmpty());
    CHECK_FALSE(curve->isDeleted());
}

TEST_CASE("F actions through mouse picking resolves the native owner", "[lc-offsetcurve][f-actions][parallel][actions]") {
    Drawing drawing;
    auto* curve = native(drawing);
    ThroughProbe action(&drawing.context);
    action.choose((curve->getStartpoint() + curve->getEndpoint()) / 2);
    CHECK(action.source() == curve);
}

TEST_CASE("F actions Modify Offset includes native inputs but defaults to plain cache output", "[lc-offsetcurve][f-actions][offset][actions]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    OffsetProbe action(&drawing.context);
    REQUIRE(action.acceptsNative());
    Pending pending;
    REQUIRE(action.run(curve, sidePoint(*curve), pending.batch));
    REQUIRE_FALSE(pending.batch.entitiesToAdd.isEmpty());
    CHECK(pending.batch.entitiesToDelete.isEmpty());
    for (const auto* made : pending.batch.entitiesToAdd) CHECK(made->rtti() != RS2::EntityOffsetCurve);
    Pending chord;
    REQUIRE(action.run(curve->entityAt(0), sidePoint(*curve), chord.batch));
    REQUIRE(chord.batch.entitiesToAdd.size() == pending.batch.entitiesToAdd.size());
    for (int i = 0; i < chord.batch.entitiesToAdd.size(); ++i)
        sameGeometry(*chord.batch.entitiesToAdd[i], *pending.batch.entitiesToAdd[i]);
    before.check();
}

TEST_CASE("F actions native reversal stages real reversed cache and retains a valid transported definition", "[lc-offsetcurve][f-actions][reverse]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    ReverseProbe action(&drawing.context);
    REQUIRE(action.isEntityAllowedToSelect(curve));
    CHECK_FALSE(action.isEntityAllowedToSelect(curve->entityAt(0)));
    Pending pending;
    REQUIRE(action.run({curve}, pending.batch));
    REQUIRE(pending.batch.entitiesToAdd.size() == 1);
    REQUIRE(pending.batch.entitiesToDelete == QList<RS_Entity*>{curve});
    const auto* result = dynamic_cast<const LC_OffsetCurve*>(pending.batch.entitiesToAdd.front());
    REQUIRE(result);
    CHECK(result->hasEditableDefinition());
    CHECK(result->definition().cacheReversed != curve->definition().cacheReversed);
    auto expected = curve->getData().controlPoints;
    std::reverse(expected.begin(), expected.end());
    points(result->getData().controlPoints, expected);
    CHECK_FALSE(pending.batch.setActiveLayer);
    CHECK_FALSE(pending.batch.setActivePen);
    before.check();
}

TEST_CASE("F actions reversal promotes repeated native chords and queues one owner replacement", "[lc-offsetcurve][f-actions][reverse]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    ReverseProbe action(&drawing.context);
    Pending pending;
    REQUIRE(action.run({curve->entityAt(0), curve->entityAt(1), curve}, pending.batch));
    REQUIRE(pending.batch.entitiesToDelete == QList<RS_Entity*>{curve});
    REQUIRE(pending.batch.entitiesToAdd.size() == 1);
    before.check();
}

TEST_CASE("F actions native reversal publishes and undoes without editing its original", "[lc-offsetcurve][f-actions][reverse][undo]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    ReverseProbe action(&drawing.context);
    REQUIRE(drawing.graphic.undoableModify(drawing.view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        return action.run({curve}, batch);
    }));
    REQUIRE(curve->isDeleted());
    const auto* made = dynamic_cast<const LC_OffsetCurve*>(drawing.graphic.last());
    REQUIRE(made);
    CHECK(made->hasEditableDefinition());
    REQUIRE(drawing.graphic.undo());
    CHECK_FALSE(curve->isDeleted());
    CHECK(made->isDeleted());
    before.check(false);
    REQUIRE(drawing.graphic.redo());
    CHECK(curve->isDeleted());
    CHECK_FALSE(made->isDeleted());
}

TEST_CASE("F actions refused native reversal retains that source while ordinary siblings still reverse", "[lc-offsetcurve][f-actions][reverse][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    curve->getData().degree = std::numeric_limits<std::size_t>::max();
    auto* line = new RS_Line(&drawing.graphic, {0, 0}, {4, 0});
    drawing.graphic.addEntity(line);
    ReverseProbe action(&drawing.context);
    Pending pending;
    REQUIRE(action.run({curve, line}, pending.batch));
    REQUIRE(pending.batch.entitiesToDelete == QList<RS_Entity*>{line});
    REQUIRE(pending.batch.entitiesToAdd.size() == 1);
    point(pending.batch.entitiesToAdd.front()->getStartpoint(), {4, 0});
    CHECK_FALSE(curve->isDeleted());
    CHECK(curve->getData().degree == std::numeric_limits<std::size_t>::max());
}

TEST_CASE("F actions native reversal refuses uncommitted and borrowed sources without deleting them", "[lc-offsetcurve][f-actions][reverse][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    std::unique_ptr<RS_Entity> uncommitted(curve->clone());
    auto* borrowed = new RS_EntityContainer(&drawing.graphic, false);
    borrowed->addEntity(curve);
    drawing.graphic.addEntity(borrowed);
    ReverseProbe action(&drawing.context);
    Pending pending;
    CHECK_FALSE(action.run({uncommitted.get(), borrowed}, pending.batch));
    CHECK(pending.batch.entitiesToAdd.isEmpty());
    CHECK(pending.batch.entitiesToDelete.isEmpty());
    before.check();
}

TEST_CASE("F actions stale native reversal becomes an ordinary spline rather than a stale native clone", "[lc-offsetcurve][f-actions][reverse]") {
    Drawing drawing;
    auto* curve = native(drawing);
    curve->getData().controlPoints.front().y += .01;
    REQUIRE_FALSE(curve->hasEditableDefinition());
    const Source before(*curve);
    ReverseProbe action(&drawing.context);
    Pending pending;
    REQUIRE(action.run({curve}, pending.batch));
    REQUIRE(pending.batch.entitiesToAdd.size() == 1);
    REQUIRE(pending.batch.entitiesToAdd.front()->rtti() == RS2::EntitySpline);
    auto expected = curve->getData().controlPoints;
    std::reverse(expected.begin(), expected.end());
    points(static_cast<const RS_Spline*>(pending.batch.entitiesToAdd.front())->getData().controlPoints, expected);
    before.check();
}

TEST_CASE("F actions native explode uses ordinary cache stroking for preview and polyline output", "[lc-offsetcurve][f-actions][explode]") {
    Drawing drawing;
    auto* curve = native(drawing, true);
    const Source before(*curve);
    ExplodeProbe action(&drawing.context);
    action.setUsePolyline(true);
    action.setSegmentsCountValue(16);
    CHECK(action.mayModifySplineEntity(curve));
    CHECK(action.mayModifySplineEntity(curve->entityAt(0)));
    const auto plain = curve->toPlainSpline();
    std::vector<RS_Vector> expected, actual;
    plain->fillStrokePoints(16, expected);
    bool closed = true;
    action.fillStrokePoints(curve->entityAt(0), 16, actual, closed);
    points(actual, expected);
    CHECK(closed == plain->isClosed());
    std::unique_ptr<RS_Entity> preview(action.createModifiedSplineEntity(curve, {0, 0}, false));
    REQUIRE(preview);
    CHECK(preview->rtti() == RS2::EntityPolyline);
    Pending pending;
    REQUIRE(action.run(curve->entityAt(0), pending.batch));
    REQUIRE(pending.batch.entitiesToDelete == QList<RS_Entity*>{curve});
    REQUIRE(pending.batch.entitiesToAdd.size() == 1);
    sameGeometry(*pending.batch.entitiesToAdd.front(), *preview);
    before.check();
}

TEST_CASE("F actions native explode to lines produces only plain finite entities and preserves source attributes", "[lc-offsetcurve][f-actions][explode]") {
    Drawing drawing;
    auto* curve = native(drawing);
    const Source before(*curve);
    ExplodeProbe action(&drawing.context);
    action.setUsePolyline(false);
    action.setKeepOriginals(true);
    Pending pending;
    REQUIRE(action.run(curve, pending.batch));
    REQUIRE(pending.batch.entitiesToAdd.size() == 8);
    CHECK(pending.batch.entitiesToDelete.isEmpty());
    for (const auto* line : pending.batch.entitiesToAdd) {
        CHECK(line->rtti() == RS2::EntityLine);
        CHECK(line->hasValidBorders());
        CHECK(line->getLayer(false) == curve->getLayer(false));
        CHECK(line->getPen(false) == curve->getPen(false));
    }
    point(pending.batch.entitiesToAdd.front()->getStartpoint(), curve->getStartpoint());
    point(pending.batch.entitiesToAdd.back()->getEndpoint(), curve->getEndpoint());
    before.check();
}

TEST_CASE("F actions native explode refuses invalid counts caches and uncommitted targets without a deletion", "[lc-offsetcurve][f-actions][explode][refusal]") {
    Drawing drawing;
    auto* curve = native(drawing);
    ExplodeProbe action(&drawing.context);
    for (int count : {0, -1, 65536, std::numeric_limits<int>::max()}) {
        CAPTURE(count);
        action.setSegmentsCountValue(count);
        Pending pending;
        CHECK_FALSE(action.run(curve, pending.batch));
        CHECK(pending.batch.entitiesToAdd.isEmpty());
        CHECK(pending.batch.entitiesToDelete.isEmpty());
    }
    action.setSegmentsCountValue(8);
    std::unique_ptr<RS_Entity> uncommitted(curve->clone());
    Pending missing;
    CHECK_FALSE(action.run(uncommitted.get(), missing.batch));
    CHECK(missing.batch.entitiesToDelete.isEmpty());
    curve->getData().degree = std::numeric_limits<std::size_t>::max();
    Pending invalid;
    CHECK_FALSE(action.run(curve, invalid.batch));
    CHECK(invalid.batch.entitiesToAdd.isEmpty());
    CHECK(invalid.batch.entitiesToDelete.isEmpty());
    CHECK_FALSE(curve->isDeleted());
}

TEST_CASE("F actions ordinary spline control-point actions exclude native owners and native chords", "[lc-offsetcurve][f-actions][control-refusal]") {
    Drawing drawing;
    auto* curve = native(drawing, true);
    const Source before(*curve);
    auto plain = curve->toPlainSpline();
    excludesNative<LC_ActionSplineAddPoint>(drawing, *curve, *plain);
    excludesNative<LC_ActionSplineAppendPoint>(drawing, *curve, *plain);
    excludesNative<LC_ActionRemoveSplinePoints>(drawing, *curve, *plain);
    excludesNative<LC_ActionSplineRemoveBetween>(drawing, *curve, *plain);
    before.check();
}

TEST_CASE("F actions strict control picking skips a nearer native curve and reaches an ordinary spline",
          "[lc-offsetcurve][f-actions][control-refusal][actions]") {
    for (bool nativeFirst : {false, true}) for (double separation : {0., .01}) {
        CAPTURE(nativeFirst, separation);
        Drawing drawing;
        std::unique_ptr<LC_OffsetCurve> pending{native(drawing, true, false)};
        auto plain = pending->toPlainSpline();
        plain->move({0, separation});
        plain->setLayer(drawing.graphic.getActiveLayer());
        auto* ordinary = plain.get();
        auto* curve = pending.get();
        if (nativeFirst) drawing.graphic.addEntity(pending.release());
        drawing.graphic.addEntity(plain.release());
        if (!nativeFirst) drawing.graphic.addEntity(pending.release());
        const Source before(*curve);
        const auto query = curve->entityAt(0)->getStartpoint();
        const auto check = [&](auto* actionType) {
            using Action = std::remove_pointer_t<decltype(actionType)>;
            ControlProbe<Action> action(&drawing.context);
            action.init(0);
            action.click(query);
            CHECK(action.source() == ordinary);
            RS_Selection{&drawing.graphic, drawing.view.getViewPort()}.selectEntitiesList({ordinary}, false);
        };
        check(static_cast<LC_ActionSplineAddPoint*>(nullptr));
        check(static_cast<LC_ActionSplineAppendPoint*>(nullptr));
        check(static_cast<LC_ActionRemoveSplinePoints*>(nullptr));
        check(static_cast<LC_ActionSplineRemoveBetween*>(nullptr));
        before.check();
    }
}
