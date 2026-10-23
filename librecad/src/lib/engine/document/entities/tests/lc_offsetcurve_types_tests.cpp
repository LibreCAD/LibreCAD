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
#include <memory>
#include <vector>

#include "lc_action_modify_trim.h"
#include "lc_action_select_intersected.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "lc_selectedset.h"
#include "rs_circle.h"
#include "rs_information.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_pen.h"
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
    RS_Selection selection() { return RS_Selection{&graphic, view.getViewPort()}; }
};

struct Admitted {
    std::unique_ptr<LC_OffsetCurve> curve;
    LC_OffsetCheckResult checked;
};

// The fixture calls actual C admission once. Query/refusal checks below never
// rerun C or substitute a fabricated native success for an ordinary spline.
Admitted admitted(RS_EntityContainer* parent = nullptr, bool open = false) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, open ? .1 : 0, false};
    definition.distance = open ? .25 : 1;
    definition.relTolerance = .01;
    std::unique_ptr<RS_Entity> cache;
    if (open) {
        cache = std::make_unique<RS_Line>(parent, RS_Vector{1.25, 0},
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
        cache = std::make_unique<RS_Spline>(parent, data);
    }
    Admitted result;
    result.curve = LC_OffsetCurve::adopt(*cache, definition, open ? .01 : .02, &result.checked);
    CAPTURE(result.checked.reason);
    REQUIRE(result.checked);
    REQUIRE(result.curve);
    REQUIRE(result.curve->hasEditableDefinition());
    REQUIRE(result.curve->count() > 1);
    return result;
}

LC_OffsetCurve* addNative(Drawing& drawing, bool open = false) {
    auto fixture = admitted(&drawing.graphic, open);
    auto* curve = fixture.curve.release();
    curve->setLayer(drawing.graphic.getActiveLayer());
    drawing.graphic.addEntity(curve);
    return curve;
}

RS_Line* addLine(Drawing& drawing, const RS_Vector& a, const RS_Vector& b) {
    auto* line = new RS_Line(&drawing.graphic, a, b);
    line->setLayer(drawing.graphic.getActiveLayer());
    drawing.graphic.addEntity(line);
    return line;
}

void point(const RS_Vector& actual, const RS_Vector& expected) {
    CHECK(actual.valid == expected.valid);
    CHECK(actual.x == expected.x);
    CHECK(actual.y == expected.y);
    CHECK(actual.z == expected.z);
}

void points(const std::vector<RS_Vector>& actual, const std::vector<RS_Vector>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); point(actual[i], expected[i]); }
}

void cacheData(const RS_SplineData& actual, const RS_SplineData& expected) {
    CHECK(actual.degree == expected.degree);
    CHECK(actual.type == expected.type);
    CHECK(actual.savedOpenType == expected.savedOpenType);
    CHECK(actual.knotslist == expected.knotslist);
    CHECK(actual.weights == expected.weights);
    CHECK(actual.savedOpenKnots == expected.savedOpenKnots);
    CHECK(actual.m_closedFlag == expected.m_closedFlag);
    CHECK(actual.m_periodicFlag == expected.m_periodicFlag);
    points(actual.controlPoints, expected.controlPoints);
    points(actual.fitPoints, expected.fitPoints);
}

struct Unchanged {
    const LC_OffsetCurve& curve;
    const RS_SplineData data;
    const LC_EllipseOffsetDefinition definition;
    const std::vector<RS_Vector> grips;
    const RS_Vector* controlStorage;
    const unsigned long long id;
    const unsigned long long lineage;
    RS_EntityContainer* const parent;
    RS_Layer* const layer;
    const RS_Pen pen;
    const bool defined;
    const bool consistent;
    const double tolerance;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> childEnds;

    explicit Unchanged(const LC_OffsetCurve& c) : curve(c), data(c.getData()), definition(c.definition()),
        grips(c.cachedDefinitionGrips()), controlStorage(c.getData().controlPoints.data()), id(c.getId()),
        lineage(c.lineage()), parent(c.getParent()), layer(c.getLayer(false)), pen(c.getPen(false)),
        defined(c.isDefined()), consistent(c.isConsistent()), tolerance(c.checkedWorldTolerance()) {
        for (const auto* child : c) {
            REQUIRE(child);
            children.push_back(child);
            childEnds.push_back(child->getStartpoint());
            childEnds.push_back(child->getEndpoint());
        }
    }
    void check() const {
        cacheData(curve.getData(), data);
        CHECK(curve.getData().controlPoints.data() == controlStorage);
        CHECK(curve.getId() == id);
        CHECK(curve.lineage() == lineage);
        CHECK(curve.getParent() == parent);
        CHECK(curve.getLayer(false) == layer);
        CHECK(curve.getPen(false) == pen);
        CHECK(curve.isDefined() == defined);
        CHECK(curve.isConsistent() == consistent);
        CHECK(curve.checkedWorldTolerance() == tolerance);
        const auto& d = curve.definition();
        point(d.basis.center, definition.basis.center);
        point(d.basis.majorP, definition.basis.majorP);
        CHECK(d.basis.ratio == definition.basis.ratio);
        CHECK(d.basis.angle1 == definition.basis.angle1);
        CHECK(d.basis.angle2 == definition.basis.angle2);
        CHECK(d.basis.reversed == definition.basis.reversed);
        CHECK(d.basis.isArc == definition.basis.isArc);
        CHECK(d.basis.angleDegrees == definition.basis.angleDegrees);
        CHECK(d.basis.startAngleDegrees == definition.basis.startAngleDegrees);
        CHECK(d.basis.otherAngleDegrees == definition.basis.otherAngleDegrees);
        CHECK(d.basis.angularLength == definition.basis.angularLength);
        CHECK(d.distance == definition.distance);
        CHECK(d.relTolerance == definition.relTolerance);
        CHECK(d.towardCentre == definition.towardCentre);
        CHECK(d.cacheReversed == definition.cacheReversed);
        points(curve.cachedDefinitionGrips(), grips);
        REQUIRE(curve.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            REQUIRE(curve.entityAt(static_cast<int>(i)) == children[i]);
            CHECK(children[i]->getParent() == &curve);
            point(children[i]->getStartpoint(), childEnds[2 * i]);
            point(children[i]->getEndpoint(), childEnds[2 * i + 1]);
            CHECK_FALSE(children[i]->isDeleted());
        }
    }
};

void selectionIs(Drawing& drawing, const QList<RS_Entity*>& expected) {
    const auto* selected = drawing.graphic.getSelection();
    QList<RS_Entity*> actual;
    for (auto* entity : *selected) actual.append(entity);
    REQUIRE(actual.size() == expected.size());
    for (auto* entity : expected) {
        CHECK(actual.contains(entity));
        CHECK(entity->isSelected());
    }
}

class IntersectProbe final : public LC_ActionSelectIntersected {
public:
    explicit IntersectProbe(LC_ActionContext* context) : LC_ActionSelectIntersected(context, true) {}
    using LC_ActionSelectIntersected::doInitWithContextEntity;
    using LC_ActionSelectIntersected::isValidEntityForSelection;
};

template <bool Both = false>
class TrimProbe final : public LC_ActionModifyTrim {
public:
    explicit TrimProbe(LC_ActionContext* context) : LC_ActionModifyTrim(context, Both) {}
    using RS_Snapper::catchEntity;
    using LC_ActionModifyTrim::ChooseLimitEntity;
    using LC_ActionModifyTrim::ChooseTrimEntity;
    using LC_ActionModifyTrim::doInitWithContextEntity;
    using LC_ActionModifyTrim::m_limitEntity;
    using LC_ActionModifyTrim::m_trimEntity;
    using LC_ActionModifyTrim::previewTrim;
    using LC_ActionModifyTrim::setLimitEntity;
    void click(const RS_Vector& p) {
        const auto event = lc::test::eventAt(p.x, p.y);
        onMouseLeftButtonRelease(getStatus(), &event);
    }
};
}

TEST_CASE("F native type is spline-derived and non-atomic while its generated chords are protected",
          "[lc-offsetcurve][types]") {
    Drawing drawing;
    const auto* curve = addNative(drawing);
    CHECK(curve->rtti() == RS2::EntityOffsetCurve);
    CHECK(dynamic_cast<const RS_Spline*>(curve) != nullptr);
    CHECK(curve->isContainer());
    CHECK_FALSE(curve->isAtomic());
    CHECK_FALSE(curve->trimmable());
    CHECK(RS_Information::isEditable(curve));
    CHECK_FALSE(curve->isParentIgnoredOnModifications());
    for (const auto* chord : *curve) {
        CHECK(chord->rtti() == RS2::EntityLine);
        CHECK(chord->isParentIgnoredOnModifications());
        CHECK_FALSE(RS_Information::isEditable(chord));
        CHECK_FALSE(RS_Information::isTrimmable(chord));
    }
}

TEST_CASE("F native length area and distance use the same display cache as an ordinary spline",
          "[lc-offsetcurve][types][information]") {
    for (bool open : {false, true}) {
        CAPTURE(open);
        const auto fixture = admitted(nullptr, open);
        const auto& curve = *fixture.curve;
        const Unchanged original(curve);
        const auto plain = curve.toPlainSpline();
        REQUIRE(plain);
        CHECK(plain->rtti() == RS2::EntitySpline);
        cacheData(plain->getData(), curve.getData());
        CHECK(curve.getLength() == Catch::Approx(plain->getLength()));
        CHECK(curve.areaLineIntegral() == Catch::Approx(plain->areaLineIntegral()));
        for (const RS_Vector query : {RS_Vector{3, .5}, RS_Vector{0, 0}, RS_Vector{-1, 3}}) {
            CAPTURE(query.x, query.y);
            double nativeDistance = RS_MAXDOUBLE, plainDistance = RS_MAXDOUBLE;
            const auto nativePoint = curve.getNearestPointOnEntity(query, true, &nativeDistance);
            const auto plainPoint = plain->getNearestPointOnEntity(query, true, &plainDistance);
            point(nativePoint, plainPoint);
            CHECK(nativeDistance == Catch::Approx(plainDistance));
            CHECK(curve.getDistanceToPoint(query) == Catch::Approx(plain->getDistanceToPoint(query)));
            CHECK(curve.isPointOnEntity(nativePoint) == plain->isPointOnEntity(nativePoint));
        }
        original.check();
    }
}

TEST_CASE("F native cached chord intersections match the ordinary spline display geometry",
          "[lc-offsetcurve][types][information]") {
    const auto fixture = admitted();
    const auto& curve = *fixture.curve;
    const Unchanged original(curve);
    const auto plain = curve.toPlainSpline();
    REQUIRE(plain);
    REQUIRE(plain->count() == curve.count());
    RS_Line crossing{nullptr, {-3, .25}, {3, .25}};
    std::size_t intersections = 0;
    for (unsigned i = 0; i < curve.count(); ++i) {
        const auto actual = RS_Information::getIntersection(curve.entityAt(i), &crossing, true);
        const auto expected = RS_Information::getIntersection(plain->entityAt(i), &crossing, true);
        points(actual.getVector(), expected.getVector());
        intersections += actual.size();
    }
    CHECK(intersections == 2);
    original.check();
}

TEST_CASE("F single selection of a native chord selects and toggles its owner only", "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    auto selection = drawing.selection();
    selection.selectSingle(curve->entityAt(0));
    selectionIs(drawing, {curve});
    for (const auto* chord : *curve) CHECK(chord->isSelected() == curve->isSelected());
    selection.selectSingle(curve->entityAt(1));
    selectionIs(drawing, {});
    CHECK_FALSE(curve->getFlag(RS2::FlagSelected));
    original.check();
}

TEST_CASE("F bulk chord selection stores the native owner once in list and vector APIs", "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto* first = curve->entityAt(0);
    auto* second = curve->entityAt(1);
    auto selection = drawing.selection();
    selection.selectEntitiesList({first, second, first}, true);
    selectionIs(drawing, {curve});
    RS_Selection::selectEntitiesVector(&drawing.graphic, drawing.view.getViewPort(), {first, second}, false);
    selectionIs(drawing, {});
    RS_Selection::selectEntitiesVector(&drawing.graphic, drawing.view.getViewPort(), {first, second}, true);
    selectionIs(drawing, {curve});
    RS_Selection::selectEntitiesList(&drawing.graphic, drawing.view.getViewPort(), {second, first}, false);
    selectionIs(drawing, {});
    CHECK_FALSE(first->getFlag(RS2::FlagSelected));
    CHECK_FALSE(second->getFlag(RS2::FlagSelected));
}

TEST_CASE("F direct selection bypass still registers only the native owner", "[lc-offsetcurve][types][selection]") {
    Drawing drawing; auto* curve = addNative(drawing); auto selection = drawing.selection();
    selection.justSelect(curve->entityAt(0)); selectionIs(drawing, {curve});
    selection.justSelect(curve->entityAt(1), false); selectionIs(drawing, {});
    selection.justSelect(nullptr); selectionIs(drawing, {});
}

TEST_CASE("F explicit native picking resolves owners while ordinary spline and line filters stay strict",
          "[lc-offsetcurve][types][snap][selection]") {
    Drawing drawing; auto* curve = addNative(drawing); TrimProbe<> action{&drawing.context};
    const auto query = curve->entityAt(0)->getStartpoint();
    for (auto level : {RS2::ResolveNone, RS2::ResolveAll}) {
        CHECK(action.catchEntity(query, RS2::EntitySpline, level) == nullptr);
        CHECK(action.catchEntity(query, EntityTypeList{RS2::EntitySpline, RS2::EntityOffsetCurve}, level) == curve);
        CHECK(action.catchEntity(query, RS2::EntityOffsetCurve, level) == curve);
        CHECK(action.catchEntity(query, RS2::EntityLine, level) == nullptr);
    }
    auto* line = addLine(drawing, query, query + RS_Vector{1, 0});
    CHECK(action.catchEntity(query, RS2::EntityLine, RS2::ResolveAll) == line);
}

TEST_CASE("F contour picking a generated chord toggles its native owner without an atomic cast",
          "[lc-offsetcurve][types][selection]") {
    Drawing drawing; auto* curve = addNative(drawing); auto selection = drawing.selection();
    selection.selectContour(curve->entityAt(0)); selectionIs(drawing, {curve});
    selection.selectContour(curve); selectionIs(drawing, {});
}

TEST_CASE("F conditional list matching sees a native owner rather than its disposable chord", "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto selection = drawing.selection();
    selection.selectIfMatched({curve->entityAt(0)}, true, [curve](RS_Entity* candidate) { return candidate == curve; });
    selectionIs(drawing, {curve});
    selection.selectIfMatched({curve->entityAt(0)}, false, [](RS_Entity* candidate) {
        return candidate->rtti() == RS2::EntityLine;
    });
    selectionIs(drawing, {curve});
}

TEST_CASE("F spline window selection includes native curves without matching unrelated types", "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto plain = curve->toPlainSpline();
    plain->setLayer(drawing.graphic.getActiveLayer());
    auto* spline = plain.release();
    drawing.graphic.addEntity(spline);
    auto* circle = new RS_Circle(&drawing.graphic, RS_CircleData{{0, 0}, 1});
    drawing.graphic.addEntity(circle);
    auto selection = drawing.selection();
    selection.selectWindow(RS2::EntitySpline, {-3, -3}, {3, 3});
    selectionIs(drawing, {curve, spline});
    CHECK_FALSE(circle->isSelected());
    selection.selectWindow(QList<RS2::EntityType>{RS2::EntitySpline}, {-3, -3}, {3, 3}, false);
    selectionIs(drawing, {});
    selection.selectWindow(QList<RS2::EntityType>{RS2::EntitySpline}, {-3, -3}, {3, 3});
    selectionIs(drawing, {curve, spline});
    selection.selectWindow(RS2::EntitySpline, {-3, -3}, {3, 3}, false);
    selectionIs(drawing, {});
    selection.selectWindow(RS2::EntityCircle, {-3, -3}, {3, 3});
    selectionIs(drawing, {circle});
}

TEST_CASE("F crossing window selection matches the native cache rather than its whole bounding box", "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto selection = drawing.selection();
    selection.selectWindow(RS2::EntitySpline, {-.1, 1.9}, {.1, 2.1}, true, true);
    selectionIs(drawing, {curve});
    selection.selectEntitiesList({curve}, false);
    selection.selectWindow(QList<RS2::EntityType>{RS2::EntitySpline}, {-.1, -.1}, {.1, .1}, true, true);
    selectionIs(drawing, {});
    selection.selectWindow(QList<RS2::EntityType>{RS2::EntitySpline}, {-.1, 1.9}, {.1, 2.1}, true, true);
    selectionIs(drawing, {curve});
}

TEST_CASE("F selecting intersections with a picked chord uses the whole native cache and excludes itself",
          "[lc-offsetcurve][types][selection]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    auto* crossing = addLine(drawing, {-1, -3}, {-1, 3});
    auto* outside = addLine(drawing, {-3, -3}, {-3, 3});
    drawing.selection().selectIntersected(curve->entityAt(0), true);
    selectionIs(drawing, {crossing});
    CHECK_FALSE(curve->isSelected());
    CHECK_FALSE(outside->isSelected());
    original.check();
}

TEST_CASE("F intersected action accepts a native owner and resolves a native context chord", "[lc-offsetcurve][types][selection][actions]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto* crossing = addLine(drawing, {-1, -3}, {-1, 3});
    IntersectProbe action(&drawing.context);
    CHECK_FALSE(action.isValidEntityForSelection(nullptr));
    CHECK(action.isValidEntityForSelection(curve));
    auto plain = curve->toPlainSpline();
    CHECK_FALSE(action.isValidEntityForSelection(plain.get()));
    action.doInitWithContextEntity(curve->entityAt(0), {2, 0});
    selectionIs(drawing, {crossing});
    CHECK_FALSE(curve->isSelected());
}

TEST_CASE("F updateSplines rebuilds the native display without restamping externally edited data", "[lc-offsetcurve][types][update]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    auto changed = curve->getData();
    changed.controlPoints.front().x += .125;
    curve->getData() = changed;
    REQUIRE_FALSE(curve->isConsistent());
    REQUIRE(curve->entityAt(0)->getStartpoint().x != changed.controlPoints.front().x);
    drawing.graphic.updateSplines();
    cacheData(curve->getData(), changed);
    point(curve->entityAt(0)->getStartpoint(), changed.controlPoints.front());
    CHECK_FALSE(curve->isConsistent());
    CHECK_FALSE(curve->hasEditableDefinition());
    CHECK(curve->isDefined());
}

TEST_CASE("F Trim context and preview explicitly refuse native targets and generated native chords",
          "[lc-offsetcurve][types][trim][actions]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    auto* limit = addLine(drawing, {-3, .25}, {3, .25});
    TrimProbe<> action(&drawing.context);
    action.doInitWithContextEntity(curve, {2, 0});
    CHECK(action.m_trimEntity == nullptr);
    action.doInitWithContextEntity(curve->entityAt(0), {2, 0});
    CHECK(action.m_trimEntity == nullptr);
    bool invalid = true;
    action.previewTrim(curve, limit, {2, 0}, {0, .25}, invalid);
    CHECK(invalid);
    action.previewTrim(curve->entityAt(0), limit, {2, 0}, {0, .25}, invalid);
    CHECK(invalid);
    CHECK(drawing.graphic.count() == 2);
    CHECK_FALSE(curve->isDeleted());
    original.check();
}

TEST_CASE("F Trim retains the actual editable native limit when given a generated chord", "[lc-offsetcurve][types][trim][actions]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    TrimProbe<> action(&drawing.context);
    action.setLimitEntity(curve->entityAt(0));
    CHECK(action.m_limitEntity == curve);
    CHECK(curve->hasEditableDefinition());
    CHECK_FALSE(curve->isDeleted());
    original.check();
}

TEST_CASE("F actual Trim clicks cut an ordinary line at the whole native limit without editing that limit",
          "[lc-offsetcurve][types][trim][actions]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    auto* line = addLine(drawing, {0, 0}, {0, 4});
    TrimProbe<> action(&drawing.context);
    action.click({-2, 0});
    REQUIRE(action.getStatus() == TrimProbe<>::ChooseTrimEntity);
    REQUIRE(action.m_limitEntity == curve);
    action.click({0, 3});
    REQUIRE(line->isDeleted());
    REQUIRE(drawing.graphic.count() == 3);
    const auto* trimmed = drawing.graphic.last();
    REQUIRE(trimmed->rtti() == RS2::EntityLine);
    CHECK(trimmed->getStartpoint().distanceTo({0, 2}) < 1e-6);
    CHECK(trimmed->getEndpoint().distanceTo({0, 4}) < 1e-6);
    CHECK(curve->hasEditableDefinition());
    CHECK_FALSE(curve->isDeleted());
    original.check();
}

TEST_CASE("F Trim Two refuses the native limit as a replacement target even through its chord", "[lc-offsetcurve][types][trim][actions]") {
    Drawing drawing;
    auto* curve = addNative(drawing);
    const Unchanged original(*curve);
    TrimProbe<true> action(&drawing.context);
    action.setLimitEntity(curve->entityAt(0));
    CHECK(action.m_limitEntity == nullptr);
    action.click({-2, 0});
    CHECK(action.getStatus() == TrimProbe<true>::ChooseLimitEntity);
    CHECK(action.m_limitEntity == nullptr);
    CHECK(drawing.graphic.count() == 1);
    original.check();
}

TEST_CASE("F direct modification APIs refuse generated native targets and partial native Trim Two",
          "[lc-offsetcurve][types][trim][modification]") {
    Drawing drawing; auto* curve = addNative(drawing); const Unchanged original(*curve);
    auto* chord = dynamic_cast<RS_AtomicEntity*>(curve->entityAt(0)); REQUIRE(chord);
    struct Pending {
        LC_DocumentModificationBatch ctx;
        ~Pending() { qDeleteAll(ctx.entitiesToAdd); }
    };
    Pending cut; CHECK_FALSE(RS_Modification::cut(chord->getMiddlePoint(), chord, cut.ctx));
    CHECK(cut.ctx.entitiesToAdd.empty()); CHECK(cut.ctx.entitiesToDelete.empty());
    Pending amount; bool start = false, end = false;
    CHECK(RS_Modification::trimAmount(chord->getMiddlePoint(), chord, .25, false, start, end, amount.ctx) == nullptr);
    CHECK_FALSE(start); CHECK_FALSE(end); CHECK(amount.ctx.entitiesToAdd.empty()); CHECK(amount.ctx.entitiesToDelete.empty());
    auto* line = addLine(drawing, {0, 0}, {0, 4}); const auto initial = line->getData();
    for (auto* limit : {static_cast<RS_Entity*>(curve), static_cast<RS_Entity*>(chord)}) {
        Pending both; const auto result = RS_Modification::trim({0, 3}, line, {2, 0}, limit, true, both.ctx);
        CHECK_FALSE(result.result); CHECK(both.ctx.entitiesToAdd.empty()); CHECK(both.ctx.entitiesToDelete.empty());
    }
    point(line->getStartpoint(), initial.startpoint); point(line->getEndpoint(), initial.endpoint);
    original.check();
}

TEST_CASE("F ordinary line selection and Trim context remain atomic and unchanged", "[lc-offsetcurve][types][ordinary-control]") {
    Drawing drawing;
    auto* line = addLine(drawing, {-3, 0}, {3, 0});
    CHECK(line->isAtomic());
    CHECK(line->trimmable());
    CHECK(RS_Information::isTrimmable(line));
    drawing.selection().selectSingle(line);
    selectionIs(drawing, {line});
    TrimProbe<> action(&drawing.context);
    action.doInitWithContextEntity(line, {2, 0});
    CHECK(action.m_trimEntity == line);
    CHECK_FALSE(line->isDeleted());
    point(line->getStartpoint(), {-3, 0});
    point(line->getEndpoint(), {3, 0});
}
