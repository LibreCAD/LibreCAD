/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
// Apply after H-sheet (busy guard/refusal text). No producer/checker is replaced.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <QApplication>
#include <QCursor>
#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QScopeGuard>
#include <QStringList>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include "drw_base.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurve.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_visual_snap_manager.h"
#include "rs_actiondefault.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_preview.h"
#include "rs_previewactioninterface.h"
#include "rs_selection.h"
#include "rs_spline.h"
#include "rs_undo.h"

namespace {
class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
    mutable unsigned intersectionSearches = 0;
protected:
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity,
                               RS2::ResolveLevel level, double solidDist) const override {
        // Observe the real intersection snapper's nearest-entity search, then
        // delegate unchanged. No intersection, producer, or checker result is replaced.
        if (level == RS2::ResolveAllButTextImage) ++intersectionSearches;
        return RS_Graphic::doGetDistanceToPoint(coord, entity, level, solidDist);
    }
};
class CountedNestedChild final : public RS_EntityContainer {
public:
    CountedNestedChild(RS_EntityContainer* parent, unsigned& cloneCalls)
        : RS_EntityContainer(parent), m_cloneCalls(cloneCalls) {}
    RS_Entity* clone() const override {
        ++m_cloneCalls;
        return RS_EntityContainer::clone();
    }
private:
    unsigned& m_cloneCalls;
};
class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& text) override { messages.append(text); }
};
class DragAction final : public RS_ActionDefault {
public:
    explicit DragAction(LC_ActionContext* context) : RS_ActionDefault(context) {}
    using RS_ActionDefault::lastOffsetGripResult;
    using RS_ActionDefault::isOffsetGripDragging;
    using RS_ActionDefault::Neutral;
    using RS_ActionDefault::MovingRef;
    using RS_Snapper::clearVisualSnap;
    using RS_PreviewActionInterface::m_preview;
    LC_OffsetRunLimits limits;
    bool throwLimits = false;
    std::function<void()> beforeLimits;
    mutable unsigned limitsCalls = 0;
protected:
    LC_OffsetRunLimits offsetGripRunLimits() const override {
        ++limitsCalls;
        if (beforeLimits) beforeLimits();
        if (throwLimits) throw std::runtime_error("test failure before setThrough arguments are complete");
        return limits;
    }
};
std::uint64_t bits(double value) {
    std::uint64_t result; std::memcpy(&result, &value, sizeof(result)); return result;
}
void point(std::ostream& out, const RS_Vector& p) {
    out << p.valid << ':' << bits(p.x) << ',' << bits(p.y) << ',' << bits(p.z) << ';';
}
std::string geometry(const LC_OffsetCurve& curve) {
    std::ostringstream out;
    const auto& d = curve.getData(); const auto& g = curve.definition();
    out << d.degree << ':' << static_cast<int>(d.type) << ':' << static_cast<int>(d.savedOpenType)
        << ':' << d.m_closedFlag << d.m_periodicFlag << ';';
    for (const auto* points : {&d.controlPoints, &d.fitPoints}) {
        out << points->size() << ':'; for (const auto& p : *points) point(out, p);
    }
    for (const auto* numbers : {&d.knotslist, &d.weights, &d.savedOpenKnots}) {
        out << numbers->size() << ':'; for (double value : *numbers) out << bits(value) << ';';
    }
    point(out, g.basis.center); point(out, g.basis.majorP);
    for (double value : {g.basis.ratio, g.basis.angle1, g.basis.angle2, g.basis.angleDegrees,
        g.basis.startAngleDegrees, g.basis.otherAngleDegrees, g.basis.angularLength, g.distance, g.relTolerance})
        out << bits(value) << ';';
    out << g.basis.reversed << g.basis.isArc << g.towardCentre << g.cacheReversed
        << curve.isDefined() << curve.isConsistent() << ':' << bits(curve.checkedWorldTolerance()) << ';';
    for (const auto& p : curve.cachedDefinitionGrips()) point(out, p);
    out << curve.unknownOwnedFlags() << ';';
    for (const auto& item : curve.unknownOwnedTail()) {
        out << item.code << ':' << item.value.index() << ':';
        std::visit([&](const auto& value) {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, QString>) out << value.size() << ':' << value.toUtf8().toStdString();
            else if constexpr (std::is_same_v<Value, RS_Vector>) point(out, value);
            else if constexpr (std::is_same_v<Value, double>) out << bits(value);
            else out << value;
        }, item.value);
        out << ';';
    }
    return out.str();
}
std::string tags(const RS_Entity& entity) {
    std::ostringstream out;
    for (const auto& tag : entity.getDrwExtData()) {
        REQUIRE(tag); out << tag.get() << ':' << tag->code() << ':' << tag->type() << ':';
        REQUIRE(tag->type() == DRW_Variant::STRING); out << tag->content.s->size() << ':' << *tag->content.s;
    }
    return out.str();
}
std::string history(const RS_Graphic& graphic) {
    std::ostringstream out; out << static_cast<const RS_Undo&>(graphic); return out.str();
}
struct State {
    const LC_OffsetCurve& source;
    std::string shape, metadata, undo;
    RS_Pen pen;
    RS_Layer* layer;
    unsigned long long id, lineage;
    const RS_Vector* controls;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> endpoints;
    bool selected, modified;
    State(const LC_OffsetCurve& curve, const RS_Graphic& graphic)
        : source(curve), shape(geometry(curve)), metadata(tags(curve)), undo(history(graphic)),
          pen(curve.getPen(false)), layer(curve.getLayer(false)), id(curve.getId()), lineage(curve.lineage()),
          controls(curve.getData().controlPoints.data()), selected(curve.isSelected()), modified(graphic.isModified()) {
        for (const auto* child : curve) {
            children.push_back(child); endpoints.push_back(child->getStartpoint()); endpoints.push_back(child->getEndpoint());
        }
    }
    void sourceExact(bool sameStorage = true) const {
        CHECK(geometry(source) == shape); CHECK(tags(source) == metadata); CHECK(source.getPen(false) == pen);
        CHECK(source.getLayer(false) == layer); CHECK(source.getId() == id); CHECK(source.lineage() == lineage);
        CHECK(source.sourceHandle() == 0x71); CHECK(source.xDictHandle() == 0x72);
        CHECK(source.reactorHandles() == std::vector<quint32>{0x73});
        if (sameStorage) CHECK(source.getData().controlPoints.data() == controls);
        REQUIRE(source.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            const auto* child = source.entityAt(static_cast<int>(i)); REQUIRE(child);
            if (sameStorage) REQUIRE(child == children[i]);
            CHECK(child->getParent() == &source);
            CHECK(bits(child->getStartpoint().x) == bits(endpoints[2*i].x));
            CHECK(bits(child->getStartpoint().y) == bits(endpoints[2*i].y));
            CHECK(bits(child->getEndpoint().x) == bits(endpoints[2*i+1].x));
            CHECK(bits(child->getEndpoint().y) == bits(endpoints[2*i+1].y));
        }
    }
    void unchanged(const RS_Graphic& graphic) const {
        sourceExact(); CHECK_FALSE(source.isDeleted()); CHECK(source.isSelected() == selected);
        CHECK(graphic.isModified() == modified); CHECK(history(graphic) == undo);
    }
};
struct Fixture {
    const bool qt{lc::test::application() != nullptr};
    Graphic graphic;
    lc::test::TestGraphicView view;
    Context context;
    std::unique_ptr<DragAction> action;
    LC_OffsetCurve* source = nullptr;
    Fixture(bool whole = false, bool inwardLens = false, bool toleranceBand = false) {
        graphic.initForNewDocument(); view.setDocument(&graphic);
        view.getViewPort()->setSize(640, 480);
        view.getViewPort()->setOffsetAndFactor(100, 240, 128);
        context.setDocumentAndView(&graphic, &view);
        if (toleranceBand) {
            // The same real C-admitted tolerance-band carrier as the grip-order
            // regression: ideal radius 2, visible stored quarter endpoint 2.025.
            RS_SplineData data{3, false};
            std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
            for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
                for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
                for (auto& point : net) point = {-point.y, point.x};
            }
            for (unsigned i = 0; i <= 4; ++i)
                for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
            data.weights.assign(data.controlPoints.size(), 1.); data.m_closedFlag = true;
            RS_Spline cache{nullptr, data}; cache.scale({0, 0}, {1.0125, 1.0125});
            LC_EllipseOffsetDefinition definition;
            definition.basis = {{0, 0}, {1, 0}, 1.}; definition.distance = 1.; definition.relTolerance = .01;
            LC_OffsetCheckResult checked;
            auto admitted = LC_OffsetCurve::adopt(cache, definition, .02, &checked);
            REQUIRE(checked); REQUIRE(admitted); REQUIRE(admitted->hasEditableDefinition());
            source = admitted.release();
        } else if (whole) {
            const RS_Ellipse ellipse{nullptr, RS_EllipseData{{0, 0}, {inwardLens ? 100. : 1., 0},
                inwardLens ? .6 : 1., 0, 0, false}};
            auto made = LC_OffsetCurve::create(ellipse, inwardLens ? 49 : .25, inwardLens, {}, .01);
            REQUIRE(made); REQUIRE(made.entity); source = made.entity.release();
        } else {
            LC_EllipseOffsetDefinition definition;
            definition.basis = RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false};
            definition.distance = .25; definition.relTolerance = .01;
            const RS_Line cache{nullptr, RS_Vector{1.25, 0}, RS_Vector{1.25*std::cos(.1), 1.25*std::sin(.1)}};
            auto admitted = LC_OffsetCurve::adopt(cache, definition, .01); REQUIRE(admitted); source = admitted.release();
        }
        source->setParent(&graphic); source->setLayer(graphic.getActiveLayer());
        source->setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
        source->setSourceHandle(0x71); source->setXDictHandle(0x72); source->setReactorHandles({0x73});
        source->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_H_DRAG")),
            std::make_shared<DRW_Variant>(1000, std::string("retained"))});
        REQUIRE(source->setOwnedExtensions(0x10, {{1000, QStringLiteral("drag-owned")}, {1070, std::int16_t{7}}}));
        graphic.addEntity(source);
        RS_Selection::selectEntitiesList(&graphic, view.getViewPort(), {source}, true);
        action = std::make_unique<DragAction>(&context); action->init(0);
        RS_SnapMode mode; mode.snapFree = true; action->setSnapMode(mode); graphic.setModified(false);
    }
    void mouse(QEvent::Type type, const RS_Vector& p, Qt::MouseButton button = Qt::LeftButton,
               Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        auto* port = view.getViewPort(); const QPointF gui(port->toGuiX(p.x), port->toGuiY(p.y));
        QMouseEvent event(type, gui, gui, type == QEvent::MouseMove ? Qt::NoButton : button,
            type == QEvent::MouseButtonRelease ? Qt::NoButton : button, modifiers);
        if (type == QEvent::MouseMove) action->mouseMoveEvent(&event);
        else if (type == QEvent::MouseButtonPress) action->mousePressEvent(&event);
        else action->mouseReleaseEvent(&event);
    }
    void begin(const RS_Vector& grip = RS_Vector{1.25, 0}) {
        mouse(QEvent::MouseButtonPress, grip);
        mouse(QEvent::MouseMove, grip + RS_Vector{.125, 0});
        REQUIRE(action->isOffsetGripDragging()); REQUIRE(action->getStatus() == DragAction::MovingRef);
    }
    void clean() const {
        CHECK(action->getStatus() == DragAction::Neutral); CHECK_FALSE(action->isOffsetGripDragging());
        CHECK(action->m_preview->isEmpty());
        auto* snap = view.getViewPort()->getOverlaysDrawablesContainer(RS2::Snapper);
        REQUIRE(snap); CHECK(snap->first() == nullptr);
    }
    std::vector<LC_OffsetCurve*> live() {
        std::vector<LC_OffsetCurve*> result;
        for (auto* e : graphic) if (e && !e->isDeleted()) if (auto* curve = dynamic_cast<LC_OffsetCurve*>(e)) result.push_back(curve);
        return result;
    }
};
}

TEST_CASE("H drag repeated real mouse motion allocates only bounded feedback and never a native clone",
          "[gui][lc-offsetcurve][h-drag][motion]") {
    Fixture f; const State before{*f.source, f.graphic}; f.begin();
    REQUIRE(f.action->lastOffsetGripResult() == nullptr);
    for (unsigned i = 0; i < 40; ++i) {
        const RS_Point first{nullptr, RS_Vector{0, 0}};
        f.mouse(QEvent::MouseMove, {1.5, .125 + i/1024.});
        const RS_Point last{nullptr, RS_Vector{0, 0}};
        REQUIRE(f.action->isOffsetGripDragging());
        std::size_t points = 0;
        for (const auto* entity : *f.action->m_preview) {
            REQUIRE(entity);
            REQUIRE((entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntityPoint));
            const auto a = entity->getStartpoint(), b = entity->getEndpoint();
            CHECK(a.valid); CHECK(std::isfinite(a.x)); CHECK(std::isfinite(a.y)); CHECK(a.z == 0);
            CHECK(b.valid); CHECK(std::isfinite(b.x)); CHECK(std::isfinite(b.y)); CHECK(b.z == 0);
            points += entity->rtti() == RS2::EntityLine ? 2 : 1;
        }
        CHECK(points <= 128); CHECK(f.action->m_preview->count() <= 65);
        // Global entity IDs witness temporary construction, including discarded
        // clones/producer entities, not merely what remains in the preview.
        CHECK(last.getId() - first.getId() - 1 == f.action->m_preview->count());
        CHECK(f.action->lastOffsetGripResult() == nullptr);
        before.unchanged(f.graphic);
    }
    f.mouse(QEvent::MouseButtonPress, {1.5, 0}, Qt::RightButton); f.clean();
    before.unchanged(f.graphic);
}

TEST_CASE("H drag detached view initialization does not construct an application window",
          "[gui][lc-offsetcurve][h-drag][lifetime][ordinary]") {
    REQUIRE(lc::test::application());
    const auto countWindows = [] {
        std::size_t result = 0;
        for (const auto* widget : QApplication::topLevelWidgets())
            if (widget->inherits("QC_ApplicationWindow")) ++result;
        return result;
    };
    const auto before = countWindows();
    Fixture f;
    f.mouse(QEvent::MouseMove, {1.5, 0});
    CHECK(countWindows() == before);
    CHECK(f.action->lastOffsetGripResult() == nullptr);
}

TEST_CASE("H drag commits one real Through edit and one replacement undo cycle on release",
          "[gui][lc-offsetcurve][h-drag][commit]") {
    Fixture f; const State before{*f.source, f.graphic};
    const auto cycles = f.graphic.countUndoCycles(); f.begin();
    for (int i = 0; i < 8; ++i) f.mouse(QEvent::MouseMove, {1.5, i/128.});
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE(*result);
    CHECK(result->changed); CHECK(result->engineRuns == 1);
    REQUIRE(result->engineStatus); CHECK(*result->engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result->checkReason); CHECK(*result->checkReason == LC_OffsetCheckResult::Reason::None);
    auto live = f.live(); REQUIRE(live.size() == 1); auto* replacement = live.front();
    REQUIRE(replacement != f.source); CHECK(f.source->isDeleted()); CHECK(f.graphic.count() == 2);
    CHECK(f.graphic.countUndoCycles() == cycles + 1);
    CHECK(replacement->definition().distance == Catch::Approx(.5));
    CHECK(replacement->unknownOwnedFlags() == 0); CHECK(replacement->unknownOwnedTail().empty());
    CHECK_FALSE(replacement->definition().towardCentre); CHECK(replacement->hasEditableDefinition());
    CHECK(replacement->getParent() == &f.graphic); CHECK(replacement->isSelected());
    CHECK(replacement->getPen(false) == before.pen); CHECK(replacement->getLayer(false) == before.layer);
    CHECK(replacement->sourceHandle() == 0x71); CHECK(replacement->xDictHandle() == 0x72);
    CHECK(tags(*replacement) == before.metadata); before.sourceExact();
    const auto committed = geometry(*replacement);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(f.source->isDeleted()); CHECK(replacement->isDeleted()); before.sourceExact(false);
    REQUIRE(f.graphic.redo()); CHECK(f.source->isDeleted()); CHECK_FALSE(replacement->isDeleted());
    CHECK(geometry(*replacement) == committed); before.sourceExact(false);
}

TEST_CASE("H drag side flip reaches setThrough without an angle or grip projection",
          "[gui][lc-offsetcurve][h-drag][side]") {
    Fixture f; f.begin();
    f.mouse(QEvent::MouseMove, {.5, .25}, Qt::LeftButton, Qt::ShiftModifier | Qt::ControlModifier);
    const auto* mark = f.action->m_preview->lastEntity(); REQUIRE(mark); CHECK(mark->rtti() == RS2::EntityPoint);
    CHECK(mark->getStartpoint().x == Catch::Approx(.5)); CHECK(mark->getStartpoint().y == Catch::Approx(.25));
    f.mouse(QEvent::MouseButtonRelease, {.5, 0}); f.clean();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE(*result); CHECK(result->engineRuns == 1);
    const auto live = f.live(); REQUIRE(live.size() == 1);
    CHECK(live.front()->definition().towardCentre); CHECK(live.front()->definition().distance == Catch::Approx(.5));
    REQUIRE(f.graphic.undo()); CHECK_FALSE(f.source->definition().towardCentre);
}

TEST_CASE("H drag unchanged endpoint does not replace or prune existing redo",
          "[gui][lc-offsetcurve][h-drag][noop]") {
    Fixture f;
    auto* marker = new RS_Line(&f.graphic, RS_Vector{-2, 0}, RS_Vector{-1, 0});
    REQUIRE(f.graphic.undoableModify(f.view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        batch.add(marker); return true;
    }));
    REQUIRE(f.graphic.undo()); REQUIRE(marker->isDeleted());
    const State before{*f.source, f.graphic}; const auto count = f.graphic.count();
    f.begin(); f.mouse(QEvent::MouseMove, {1.5, .1}); f.mouse(QEvent::MouseButtonRelease, {1.25, 0}); f.clean();
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.action->limitsCalls == 0);
    CHECK(f.graphic.count() == count); before.unchanged(f.graphic);
    REQUIRE(f.graphic.redo()); CHECK_FALSE(marker->isDeleted());
}

TEST_CASE("H drag tolerance band visible grip away and back is a pre-clone no op with redo retained",
          "[gui][lc-offsetcurve][h-drag][noop][review-fix]") {
    Fixture f(false, false, true);
    const auto& grips = f.source->cachedDefinitionGrips(); REQUIRE(grips.size() == 4);
    const auto found = std::max_element(grips.begin(), grips.end(), [](const auto& a, const auto& b) { return a.x < b.x; });
    const RS_Vector grip = *found;
    CHECK(grip.x == Catch::Approx(2.025)); CHECK(grip.y == Catch::Approx(0).margin(1e-14));
    REQUIRE(std::hypot(grip.x, grip.y) - 2 > f.source->checkedWorldTolerance());
    auto* marker = new RS_Line(&f.graphic, RS_Vector{-4, -2}, RS_Vector{-3, -2});
    REQUIRE(f.graphic.undoableModify(f.view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        batch.add(marker); return true;
    }));
    REQUIRE(f.graphic.undo()); REQUIRE(marker->isDeleted());
    const State before{*f.source, f.graphic}; const auto count = f.graphic.count();
    const auto epoch = f.graphic.undoStateEpoch(); const auto cycles = f.graphic.countUndoCycles();
    RS_SnapMode mode; mode.snapFree = true; mode.snapEndpoint = true; f.action->setSnapMode(mode);
    f.action->throwLimits = true; // The unchanged-grip branch must not even request setter limits.
    f.begin(grip); f.mouse(QEvent::MouseMove, grip + RS_Vector{.5, .125});
    f.mouse(QEvent::MouseMove, grip);
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.action->limitsCalls == 0);
    REQUIRE_FALSE(f.action->m_preview->isEmpty());
    RS_Point first{nullptr, {0, 0}};
    f.mouse(QEvent::MouseButtonRelease, grip); f.clean();
    RS_Point last{nullptr, {0, 0}};
    CHECK(last.getId() - first.getId() == 1); // No native clone or producer entity at release.
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.action->limitsCalls == 0);
    CHECK(f.context.messages.isEmpty()); CHECK(f.graphic.count() == count);
    CHECK(f.graphic.undoStateEpoch() == epoch); CHECK(f.graphic.countUndoCycles() == cycles);
    CHECK(f.source->unknownOwnedFlags() == 0x10); CHECK(f.source->unknownOwnedTail().size() == 2);
    before.unchanged(f.graphic);
    REQUIRE(f.graphic.redo()); CHECK_FALSE(marker->isDeleted()); before.sourceExact();
}

TEST_CASE("H drag tolerance band grip does not suppress a genuinely changed release",
          "[gui][lc-offsetcurve][h-drag][commit][review-fix]") {
    Fixture f(false, false, true);
    const auto& grips = f.source->cachedDefinitionGrips(); REQUIRE(grips.size() == 4);
    const RS_Vector grip = *std::max_element(grips.begin(), grips.end(), [](const auto& a, const auto& b) { return a.x < b.x; });
    const State before{*f.source, f.graphic}; const auto cycles = f.graphic.countUndoCycles();
    f.begin(grip); f.mouse(QEvent::MouseButtonRelease, {2.5, 0}); f.clean();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE(*result);
    CHECK(result->changed); CHECK(result->engineRuns == 1); CHECK(f.action->limitsCalls == 1);
    REQUIRE(result->engineStatus); CHECK(*result->engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result->checkReason); CHECK(*result->checkReason == LC_OffsetCheckResult::Reason::None);
    const auto live = f.live(); REQUIRE(live.size() == 1); REQUIRE(live.front() != f.source);
    CHECK(live.front()->definition().distance == Catch::Approx(1.5));
    CHECK(live.front()->unknownOwnedFlags() == 0); CHECK(live.front()->unknownOwnedTail().empty());
    CHECK(f.graphic.countUndoCycles() == cycles + 1); before.sourceExact();
    REQUIRE(f.graphic.undo()); CHECK_FALSE(f.source->isDeleted()); before.sourceExact(false);
    REQUIRE(f.graphic.redo()); CHECK_FALSE(live.front()->isDeleted());
}

TEST_CASE("H drag first threshold crossing disables actual intersection search before event conversion",
          "[gui][lc-offsetcurve][h-drag][snapping][review-fix]") {
    Fixture f; const RS_Vector grip{1.25, 0}, crossing{1.375, .015625};
    auto* horizontal = new RS_Line(&f.graphic, RS_Vector{1., .03125}, RS_Vector{2., .03125});
    auto* vertical = new RS_Line(&f.graphic, RS_Vector{1.375, -.5}, RS_Vector{1.375, .5});
    for (auto* line : {horizontal, vertical}) { line->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(line); }
    RS_SnapMode mode; mode.snapFree = true; mode.snapIntersection = true; f.action->setSnapMode(mode);
    const State before{*f.source, f.graphic};
    f.mouse(QEvent::MouseButtonPress, grip);
    REQUIRE(f.graphic.intersectionSearches > 0); // Positive control: mode and real snap dispatch are active.
    f.graphic.intersectionSearches = 0;
    f.mouse(QEvent::MouseMove, crossing);
    REQUIRE(f.action->isOffsetGripDragging()); REQUIRE(f.action->getStatus() == DragAction::MovingRef);
    CHECK(f.graphic.intersectionSearches == 0);
    REQUIRE_FALSE(f.action->m_preview->isEmpty());
    const auto* mark = f.action->m_preview->lastEntity(); REQUIRE(mark); REQUIRE(mark->rtti() == RS2::EntityPoint);
    CHECK(mark->getStartpoint().x == crossing.x); CHECK(mark->getStartpoint().y == crossing.y);
    REQUIRE(f.action->getSnapMode()); CHECK(f.action->getSnapMode()->snapIntersection);
    f.mouse(QEvent::MouseMove, {1.5, .0625}); CHECK(f.graphic.intersectionSearches == 0);
    f.mouse(QEvent::MouseButtonRelease, grip); f.clean();
    CHECK(f.graphic.intersectionSearches == 0); CHECK(f.action->getSnapMode()->snapIntersection);
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.action->limitsCalls == 0);
    before.unchanged(f.graphic);
}

TEST_CASE("H drag ordinary reference transition retains enabled intersection snapping",
          "[gui][lc-offsetcurve][h-drag][ordinary][snapping][review-fix]") {
    Fixture f;
    RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {f.source}, false);
    auto* line = new RS_Line(&f.graphic, RS_Vector{3, 0}, RS_Vector{4, 0});
    line->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(line);
    RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {line}, true);
    RS_SnapMode mode; mode.snapFree = true; mode.snapIntersection = true; f.action->setSnapMode(mode);
    f.mouse(QEvent::MouseButtonPress, {3, 0}); f.graphic.intersectionSearches = 0;
    f.mouse(QEvent::MouseMove, {3.125, 0});
    REQUIRE(f.action->getStatus() == DragAction::MovingRef); CHECK_FALSE(f.action->isOffsetGripDragging());
    CHECK(f.graphic.intersectionSearches > 0); CHECK(f.action->getSnapMode()->snapIntersection);
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.action->limitsCalls == 0);
    f.mouse(QEvent::MouseButtonPress, {3.125, 0}, Qt::RightButton); f.clean();
    CHECK_FALSE(line->isDeleted());
}

TEST_CASE("H drag actual bounded producer and checker refusals leave source and undo unchanged",
          "[gui][lc-offsetcurve][h-drag][refusal]") {
    Fixture f; const bool producer = GENERATE(false, true);
    if (producer) f.action->limits.maxSamples = 1;
    else f.action->limits.checkLimits.maxVertices = 1;
    const State before{*f.source, f.graphic}; const auto count = f.graphic.count();
    QGuiApplication::setOverrideCursor(Qt::CrossCursor);
    f.begin(); f.mouse(QEvent::MouseButtonRelease, {.5, 0}); f.clean();
    REQUIRE(QGuiApplication::overrideCursor()); CHECK(QGuiApplication::overrideCursor()->shape() == Qt::CrossCursor);
    QGuiApplication::restoreOverrideCursor();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE_FALSE(*result);
    CHECK(result->engineRuns == 1);
    if (producer) {
        REQUIRE(result->engineStatus); CHECK(*result->engineStatus == LC_CurveOffsetStatus::LimitExceeded);
        CHECK_FALSE(result->checkReason);
    } else {
        REQUIRE(result->checkReason); CHECK(*result->checkReason == LC_OffsetCheckResult::Reason::LimitExceeded);
    }
    REQUIRE_FALSE(f.context.messages.isEmpty());
    CHECK(f.context.messages.join(" ").contains("bounded")); CHECK(f.graphic.count() == count);
    before.unchanged(f.graphic);
}

TEST_CASE("H drag point outside the retained source sweep is an actual setter refusal",
          "[gui][lc-offsetcurve][h-drag][refusal]") {
    Fixture f; const State before{*f.source, f.graphic}; f.begin();
    f.mouse(QEvent::MouseButtonRelease, {0, 2}); f.clean();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE_FALSE(*result);
    CHECK(result->engineRuns == 0); CHECK_FALSE(result->engineStatus); CHECK_FALSE(result->checkReason);
    before.unchanged(f.graphic);
}

TEST_CASE("H drag invalid real mouse coordinates never reach integer conversion or commit",
          "[gui][lc-offsetcurve][h-drag][invalid]") {
    Fixture f; const bool release = GENERATE(false, true);
    const State before{*f.source, f.graphic}; f.begin();
    const QPointF invalid(std::numeric_limits<double>::quiet_NaN(), 0);
    QMouseEvent event(release ? QEvent::MouseButtonRelease : QEvent::MouseMove, invalid, invalid,
        release ? Qt::LeftButton : Qt::NoButton, release ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
    if (release) f.action->mouseReleaseEvent(&event); else f.action->mouseMoveEvent(&event);
    f.clean(); CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean(); before.unchanged(f.graphic);
}

TEST_CASE("H drag Escape right cancel suspend and finish all release preview without a setter",
          "[gui][lc-offsetcurve][h-drag][cancel]") {
    Fixture f; const unsigned kind = GENERATE(0u, 1u, 2u, 3u);
    const State before{*f.source, f.graphic}; f.begin();
    if (kind == 0) {
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); event.ignore();
        f.action->keyPressEvent(&event);
    } else if (kind == 1) f.mouse(QEvent::MouseButtonPress, {1.5, 0}, Qt::RightButton);
    else if (kind == 2) f.action->suspend();
    else f.action->finish();
    f.clean(); CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean(); before.unchanged(f.graphic);
}

TEST_CASE("H drag redraw invalidates its epoch and consumes the late release",
          "[gui][lc-offsetcurve][h-drag][epoch]") {
    Fixture f; const State before{*f.source, f.graphic}; f.begin();
    f.view.redraw(RS2::RedrawDrawing);
    f.clean(); f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
}

TEST_CASE("H drag undo redo ABA is stale even when geometry and source identity return",
          "[gui][lc-offsetcurve][h-drag][epoch]") {
    Fixture f;
    auto* marker = new RS_Line(&f.graphic, RS_Vector{-2, 0}, RS_Vector{-1, 0});
    REQUIRE(f.graphic.undoableModify(f.view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        batch.add(marker); return true;
    }));
    const State before{*f.source, f.graphic}; const auto epoch = f.graphic.undoStateEpoch();
    f.begin(); REQUIRE(f.graphic.undo()); REQUIRE(f.graphic.redo());
    CHECK(f.graphic.undoStateEpoch() != epoch); CHECK(history(f.graphic) == before.undo);
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
}

TEST_CASE("H drag removed original is never dereferenced after membership fails",
          "[gui][lc-offsetcurve][h-drag][stale]") {
    Fixture f; f.begin(); auto* stale = f.source;
    f.graphic.removeEntity(stale); f.source = nullptr;
    const auto before = history(f.graphic); const auto count = f.graphic.count();
    f.mouse(QEvent::MouseMove, {1.5, 0}); f.clean();
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.graphic.count() == count); CHECK(history(f.graphic) == before);
}

TEST_CASE("H drag swapped action context or view document cannot publish into either document",
          "[gui][lc-offsetcurve][h-drag][owner]") {
    Fixture f; const bool viewOnly = GENERATE(false, true);
    Graphic other; other.initForNewDocument(); const State before{*f.source, f.graphic}; f.begin();
    if (viewOnly) f.view.setDocument(&other);
    else f.context.setDocumentAndView(&other, &f.view);
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    CHECK(other.isEmpty()); before.unchanged(f.graphic); CHECK(f.action->lastOffsetGripResult() == nullptr);
    // The action/view lifecycle contract keeps both documents alive for the gesture.
    f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
}

TEST_CASE("H drag appended nested display child refuses before cloning and preserves the concurrent source",
          "[gui][lc-offsetcurve][h-drag][stale][ownership][readiness]") {
    unsigned cloneCalls = 0;
    Fixture f;
    f.begin();
    auto nested = std::make_unique<CountedNestedChild>(f.source, cloneCalls);
    nested->addEntity(new RS_Line(nested.get(), RS_Vector{4, 0}, RS_Vector{5, 0}));
    auto* nestedAddress = nested.get();
    f.source->addEntity(nested.get());
    nested.release();
    REQUIRE(f.action->isOffsetGripDragging());
    REQUIRE(f.source->hasEditableDefinition()); // Carrier fingerprint alone cannot detect the child edit.
    REQUIRE(nestedAddress->getParent() == f.source);
    const State afterAppend{*f.source, f.graphic};
    const auto count = f.graphic.count();
    const auto epoch = f.graphic.undoStateEpoch();
    const auto cycles = f.graphic.countUndoCycles();
    RS_Point first{nullptr, {0, 0}};
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0});
    f.clean();
    RS_Point last{nullptr, {0, 0}};
    CHECK(cloneCalls == 0);
    CHECK(last.getId() - first.getId() == 1);
    CHECK(f.action->limitsCalls == 0);
    CHECK(f.action->lastOffsetGripResult() == nullptr);
    CHECK_FALSE(f.context.messages.isEmpty());
    CHECK(f.graphic.count() == count);
    CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK(f.graphic.countUndoCycles() == cycles);
    REQUIRE(f.source->count() == afterAppend.children.size());
    CHECK(f.source->lastEntity() == nestedAddress);
    CHECK(nestedAddress->getParent() == f.source);
    afterAppend.unchanged(f.graphic);
}

TEST_CASE("H drag same document other view context cancels the old view release before cloning",
          "[gui][lc-offsetcurve][h-drag][owner][context][readiness]") {
    Fixture f;
    lc::test::TestGraphicView otherView;
    otherView.setDocument(&f.graphic);
    struct RestoreContext {
        Fixture& fixture;
        ~RestoreContext() { fixture.context.setDocumentAndView(&fixture.graphic, &fixture.view); }
    } restore{f};
    const State before{*f.source, f.graphic};
    const auto count = f.graphic.count();
    const auto epoch = f.graphic.undoStateEpoch();
    const auto cycles = f.graphic.countUndoCycles();
    f.begin();
    f.context.setDocumentAndView(&f.graphic, &otherView);
    REQUIRE(f.action->isOffsetGripDragging());
    REQUIRE(f.context.getDocument() == &f.graphic);
    REQUIRE(f.context.getGraphicView() == &otherView);
    REQUIRE(f.view.getDocument() == &f.graphic);
    REQUIRE(otherView.getDocument() == &f.graphic);
    RS_Point first{nullptr, {0, 0}};
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0});
    f.clean();
    RS_Point last{nullptr, {0, 0}};
    CHECK(last.getId() - first.getId() == 1);
    CHECK(f.action->limitsCalls == 0);
    CHECK(f.action->lastOffsetGripResult() == nullptr);
    CHECK_FALSE(f.context.messages.isEmpty());
    CHECK(f.graphic.count() == count);
    CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK(f.graphic.countUndoCycles() == cycles);
    before.unchanged(f.graphic);
}

TEST_CASE("H drag late lock hidden or deselected target is refused without replacement",
          "[gui][lc-offsetcurve][h-drag][owner]") {
    Fixture f; const unsigned kind = GENERATE(0u, 1u, 2u);
    f.begin();
    if (kind == 0) f.source->getLayer()->lock(true);
    else if (kind == 1) f.source->setVisible(false);
    else RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {f.source}, false);
    const State before{*f.source, f.graphic};
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    before.unchanged(f.graphic); CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.graphic.count() == 1);
    if (kind == 0) f.source->getLayer()->lock(false);
}

TEST_CASE("H drag direct native geometry edit invalidates the exact entry baseline",
          "[gui][lc-offsetcurve][h-drag][stale]") {
    Fixture f; f.begin();
    const auto edit = f.source->setDistance(.5); REQUIRE(edit);
    const State afterOtherEdit{*f.source, f.graphic};
    f.mouse(QEvent::MouseButtonRelease, {.5, 0}); f.clean();
    afterOtherEdit.unchanged(f.graphic); CHECK(f.action->lastOffsetGripResult() == nullptr); CHECK(f.graphic.count() == 1);
}

TEST_CASE("H drag native grip hit ties are declined instead of resolved by entity order",
          "[gui][lc-offsetcurve][h-drag][grip]") {
    Fixture f(true); f.view.getViewPort()->setOffsetAndFactor(100, 240, 8);
    const State before{*f.source, f.graphic};
    const RS_Vector between{.625, .625};
    f.mouse(QEvent::MouseButtonPress, between); f.mouse(QEvent::MouseMove, between + RS_Vector{2, 0});
    f.clean(); CHECK(f.action->lastOffsetGripResult() == nullptr);
    f.mouse(QEvent::MouseButtonRelease, between + RS_Vector{2, 0}); f.clean(); before.unchanged(f.graphic);
}

TEST_CASE("H drag lens preview contains only actual sampled cache segments and proposed point",
          "[gui][lc-offsetcurve][h-drag][topology]") {
    Fixture f(true, true); f.view.getViewPort()->setOffsetAndFactor(100, 240, 2);
    const auto& grips = f.source->cachedDefinitionGrips();
    const auto found = std::find_if(grips.begin(), grips.end(), [](const auto& p) { return std::abs(p.x) < .01 && p.y > 0; });
    REQUIRE(found != grips.end()); const auto grip = *found; const State before{*f.source, f.graphic};
    f.mouse(QEvent::MouseButtonPress, grip); f.mouse(QEvent::MouseMove, grip + RS_Vector{8, 0});
    REQUIRE(f.action->isOffsetGripDragging());
    f.mouse(QEvent::MouseMove, {51, 0}); // Hidden untrimmed x-axis branch must not be previewed as a new curve.
    std::size_t points = 0;
    for (const auto* entity : *f.action->m_preview) {
        REQUIRE(entity); REQUIRE((entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntityPoint));
        if (entity->rtti() == RS2::EntityLine) {
            bool actual = false;
            for (const auto* child : *f.source) {
                if (bits(child->getStartpoint().x) == bits(entity->getStartpoint().x) &&
                    bits(child->getStartpoint().y) == bits(entity->getStartpoint().y) &&
                    bits(child->getEndpoint().x) == bits(entity->getEndpoint().x) &&
                    bits(child->getEndpoint().y) == bits(entity->getEndpoint().y)) actual = true;
            }
            CHECK(actual); points += 2;
        } else ++points;
    }
    CHECK(points <= 128); CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
    f.mouse(QEvent::MouseButtonPress, {51, 0}, Qt::RightButton); f.clean(); before.unchanged(f.graphic);
}

TEST_CASE("H drag undefined or inconsistent native retains ordinary spline reference behavior",
          "[gui][lc-offsetcurve][h-drag][ordinary]") {
    Fixture f; const bool inconsistent = GENERATE(false, true);
    if (inconsistent) f.source->getData().controlPoints.back().y += .01;
    else f.source->scale({0, 0}, {2, 1}); // A real non-similarity makes this native definition undefined.
    REQUIRE_FALSE(f.source->hasEditableDefinition());
    const auto* target = f.graphic.first(); REQUIRE(target);
    const auto refs = target->getRefPoints(); REQUIRE(refs.getNumber() > 0);
    const auto ref = refs.get(0); f.mouse(QEvent::MouseButtonPress, ref);
    f.mouse(QEvent::MouseMove, ref + RS_Vector{.125, 0});
    CHECK_FALSE(f.action->isOffsetGripDragging()); CHECK(f.action->lastOffsetGripResult() == nullptr);
    f.mouse(QEvent::MouseButtonPress, ref, Qt::RightButton); f.clean();
}

TEST_CASE("H drag redo state epoch advances on each history operation without fabricating a cycle",
          "[gui][lc-offsetcurve][h-drag][epoch]") {
    Fixture f; const auto before = history(f.graphic); const auto epoch = f.graphic.undoStateEpoch();
    CHECK_FALSE(f.graphic.undo()); CHECK(f.graphic.undoStateEpoch() != epoch);
    const auto next = f.graphic.undoStateEpoch(); CHECK_FALSE(f.graphic.redo()); CHECK(f.graphic.undoStateEpoch() != next);
    CHECK(history(f.graphic) == before);
}

TEST_CASE("H drag thrown staging preparation unwinds its clone cursor and preview without an undo entry",
          "[gui][lc-offsetcurve][h-drag][exception]") {
    Fixture f; const State before{*f.source, f.graphic}; const auto count = f.graphic.count();
    f.action->throwLimits = true; f.begin();
    QGuiApplication::setOverrideCursor(Qt::CrossCursor);
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    REQUIRE(QGuiApplication::overrideCursor()); CHECK(QGuiApplication::overrideCursor()->shape() == Qt::CrossCursor);
    QGuiApplication::restoreOverrideCursor();
    CHECK(f.action->lastOffsetGripResult() == nullptr); REQUIRE_FALSE(f.context.messages.isEmpty());
    CHECK(f.graphic.count() == count); before.unchanged(f.graphic);
}

TEST_CASE("H drag nonfinite or unrepresentable viewport input cancels before the snapper",
          "[gui][lc-offsetcurve][h-drag][invalid]") {
    Fixture f; const bool viewport = GENERATE(false, true); const State before{*f.source, f.graphic}; f.begin();
    if (viewport) {
        f.view.getViewPort()->setOffsetAndFactor(100, 240, 0);
        f.mouse(QEvent::MouseMove, {1.5, 0});
        f.view.getViewPort()->setOffsetAndFactor(100, 240, 128);
    } else {
        const QPointF huge(std::numeric_limits<double>::max(), 1);
        QMouseEvent event(QEvent::MouseMove, huge, huge, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        f.action->mouseMoveEvent(&event);
    }
    f.clean(); CHECK(f.action->lastOffsetGripResult() == nullptr); before.unchanged(f.graphic);
}

TEST_CASE("H drag ordinary spline control points still use the established default action",
          "[gui][lc-offsetcurve][h-drag][ordinary]") {
    const bool qt = lc::test::application() != nullptr; REQUIRE(qt);
    Graphic graphic; graphic.initForNewDocument(); lc::test::TestGraphicView view; view.setDocument(&graphic);
    view.getViewPort()->setSize(640, 480); view.getViewPort()->setOffsetAndFactor(100, 240, 128);
    Context context; context.setDocumentAndView(&graphic, &view);
    RS_SplineData data{1, false}; data.controlPoints = {{1, 0}, {2, 0}};
    data.type = RS_SplineData::SplineType::ClampedOpen; data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    auto* source = new RS_Spline(&graphic, data); REQUIRE(source->validate()); graphic.addEntity(source);
    RS_Selection::selectEntitiesList(&graphic, view.getViewPort(), {source}, true);
    DragAction action(&context); action.init(0); RS_SnapMode mode; mode.snapFree = true; action.setSnapMode(mode);
    const auto send = [&](QEvent::Type type, RS_Vector p) {
        const QPointF gui(view.getViewPort()->toGuiX(p.x), view.getViewPort()->toGuiY(p.y));
        QMouseEvent event(type, gui, gui, type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
            type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        if (type == QEvent::MouseButtonPress) action.mousePressEvent(&event);
        else if (type == QEvent::MouseButtonRelease) action.mouseReleaseEvent(&event);
        else action.mouseMoveEvent(&event);
    };
    send(QEvent::MouseButtonPress, {1, 0}); send(QEvent::MouseMove, {1.25, .25});
    REQUIRE(action.getStatus() == DragAction::MovingRef); CHECK_FALSE(action.isOffsetGripDragging());
    send(QEvent::MouseButtonRelease, {1.25, .25});
    if (action.getStatus() == DragAction::MovingRef) {
        // The ordinary preference may complete on a subsequent press rather than release.
        send(QEvent::MouseButtonPress, {1.25, .25}); send(QEvent::MouseButtonRelease, {1.25, .25});
    }
    CHECK(action.getStatus() == DragAction::Neutral); CHECK(action.lastOffsetGripResult() == nullptr);
    REQUIRE(graphic.count() == 2); CHECK(source->isDeleted());
    const auto* replacement = dynamic_cast<const RS_Spline*>(graphic.lastEntity()); REQUIRE(replacement);
    CHECK(replacement->rtti() == RS2::EntitySpline);
    CHECK(replacement->getData().controlPoints.front().x == Catch::Approx(1.25));
    CHECK(replacement->getData().controlPoints.front().y == Catch::Approx(.25));
    REQUIRE(graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(source->getData().controlPoints.front().x == 1);
}

namespace {
void queuedLifetimeOperation(const std::function<void()>& operation) {
    QEventLoop loop;
    std::exception_ptr error;
    QTimer::singleShot(0, &loop, [&] {
        try { operation(); } catch (...) { error = std::current_exception(); }
        loop.quit();
    });
    loop.exec();
    if (error) std::rethrow_exception(error);
}

struct DragLifetimeFixture {
    const bool qt{lc::test::application() != nullptr};
    Graphic fallback;
    std::unique_ptr<Graphic> drawing = std::make_unique<Graphic>();
    std::unique_ptr<lc::test::TestGraphicView> view = std::make_unique<lc::test::TestGraphicView>();
    Context context;
    std::unique_ptr<DragAction> action;
    LC_OffsetCurve* source = nullptr;
    DragLifetimeFixture() {
        REQUIRE(qt);
        fallback.initForNewDocument(); drawing->initForNewDocument();
        view->setDocument(drawing.get());
        view->getViewPort()->setSize(640, 480);
        view->getViewPort()->setOffsetAndFactor(100, 240, 128);
        context.setDocumentAndView(drawing.get(), view.get());
        LC_EllipseOffsetDefinition definition;
        definition.basis = RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false};
        definition.distance = .25; definition.relTolerance = .01;
        const RS_Line cache{nullptr, RS_Vector{1.25, 0}, RS_Vector{1.25*std::cos(.1), 1.25*std::sin(.1)}};
        auto admitted = LC_OffsetCurve::adopt(cache, definition, .01); REQUIRE(admitted);
        source = admitted.release(); source->setParent(drawing.get());
        source->setLayer(drawing->getActiveLayer());
        source->setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
        source->setSourceHandle(0x71); source->setXDictHandle(0x72); source->setReactorHandles({0x73});
        source->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_H_DRAG")),
            std::make_shared<DRW_Variant>(1000, std::string("retained"))});
        REQUIRE(source->setOwnedExtensions(0x10, {{1000, QStringLiteral("drag-owned")}, {1070, std::int16_t{7}}}));
        drawing->addEntity(source);
        RS_Selection::selectEntitiesList(drawing.get(), view->getViewPort(), {source}, true);
        action = std::make_unique<DragAction>(&context); action->init(0);
        RS_SnapMode mode; mode.snapFree = true; action->setSnapMode(mode); drawing->setModified(false);
    }
    // Cached device coordinates: late input must never consult the destroyed viewport.
    void mouse(QEvent::Type type, const QPointF& position) {
        QMouseEvent event(type, position, position,
            type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
            type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        if (type == QEvent::MouseButtonPress) action->mousePressEvent(&event);
        else if (type == QEvent::MouseMove) action->mouseMoveEvent(&event);
        else action->mouseReleaseEvent(&event);
    }
    void press(bool crossThreshold) {
        mouse(QEvent::MouseButtonPress, {260, 240}); // WCS (1.25, 0).
        if (crossThreshold) {
            mouse(QEvent::MouseMove, {276, 240});
            REQUIRE(action->isOffsetGripDragging());
        }
    }
    ~DragLifetimeFixture() {
        // Orphaned action/base destruction is deliberately BEFORE repairing the borrowed context.
        action.reset();
        if (view) {
            if (!drawing) { view->setDocument(&fallback); context.setDocumentAndView(&fallback, view.get()); }
            view->beginClose();
        }
    }
};
}

TEST_CASE("H drag document death invalidates pending and active input before any borrowed read",
          "[gui][lc-offsetcurve][h-drag][lifetime][document]") {
    const bool active = GENERATE(false, true);
    DragLifetimeFixture f;
    f.press(active);
    const auto* identity = f.drawing.get();
    const std::weak_ptr<DRW_Variant> payload = f.source->getDrwExtData().back();
    QPointer<DragAction> action(f.action.get());
    queuedLifetimeOperation([&] {
        f.drawing.reset(); f.source = nullptr;
        CHECK(f.context.getDocument() == identity); CHECK(f.view->getDocument() == identity);
        REQUIRE(action); CHECK(action->isFinished());
        CHECK(action->getStatus() == DragAction::Neutral); CHECK_FALSE(action->isOffsetGripDragging());
        CHECK(action->m_preview->isEmpty()); CHECK(action->limitsCalls == 0);
        CHECK(action->lastOffsetGripResult() == nullptr); CHECK(payload.expired());
        f.mouse(QEvent::MouseMove, {292, 240}); f.mouse(QEvent::MouseButtonRelease, {292, 240});
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        action->keyPressEvent(&escape); CHECK(escape.isAccepted());
        action->init(0); action->resume(); action->suspend(); action->finish();
        action->clearVisualSnap(); // Detached manager clear is safe before its eventual destructor.
        CHECK(action->limitsCalls == 0); CHECK(action->lastOffsetGripResult() == nullptr);
        f.action.reset(); CHECK(action.isNull()); // Runs the real preview and visual-snap base destructors.
        CHECK(f.context.getDocument() == identity); CHECK(f.view->getDocument() == identity);
    });
    CHECK(f.fallback.isEmpty()); CHECK(f.fallback.countUndoCycles() == 0);
}

TEST_CASE("H drag a destroyed view leaves late input and eventual action destruction safe",
          "[gui][lc-offsetcurve][h-drag][lifetime][view]") {
    DragLifetimeFixture f;
    const State before(*f.source, *f.drawing);
    f.press(true);
    const auto listeners = f.drawing->getSelection()->listenerCount();
    const auto* identity = f.view.get();
    QPointer<RS_GraphicView> view(f.view.get());
    QPointer<DragAction> action(f.action.get());
    queuedLifetimeOperation([&] {
        f.view.reset(); // No beginClose: exercise QObject destruction after viewport/snap-data destruction.
        CHECK(view.isNull()); CHECK(f.context.getGraphicView() == identity);
        REQUIRE(action); CHECK(action->isFinished()); CHECK_FALSE(action->isOffsetGripDragging());
        CHECK(action->m_preview->isEmpty()); CHECK(action->getStatus() == DragAction::Neutral);
        f.mouse(QEvent::MouseMove, {292, 240}); f.mouse(QEvent::MouseButtonRelease, {292, 240});
        action->init(0); action->resume(); action->suspend(); action->finish();
        action->clearVisualSnap();
        CHECK(action->limitsCalls == 0); CHECK(action->lastOffsetGripResult() == nullptr);
        before.unchanged(*f.drawing);
        f.action.reset(); CHECK(action.isNull()); // Must not leak the orphan to avoid its base destructors.
        CHECK(f.drawing->getSelection()->listenerCount() == listeners - 1);
        before.unchanged(*f.drawing);
    });
}

TEST_CASE("H drag ownership and document locks refuse both entry and late release without staging",
          "[gui][lc-offsetcurve][h-drag][ownership]") {
    const bool late = GENERATE(false, true);
    const unsigned kind = GENERATE(0u, 1u, 2u);
    Fixture f;
    auto* documentLock = new RS_Layer("H_DRAG_DOCUMENT_LOCK");
    f.graphic.addLayer(documentLock); documentLock->lock(true); f.graphic.setModified(false);
    auto* rootLayer = f.graphic.getLayer(false);
    const auto restore = qScopeGuard([&] {
        f.source->setOwner(true); f.graphic.setOwner(true); f.graphic.setLayer(rootLayer);
    });
    const State before(*f.source, f.graphic);
    const auto epoch = f.graphic.undoStateEpoch(); const auto count = f.graphic.count();
    if (late) f.begin();
    if (kind == 0) f.source->setOwner(false);
    else if (kind == 1) f.graphic.setOwner(false);
    else f.graphic.setLayer(documentLock); // Registered root layer; source's own layer stays unlocked.
    if (kind == 2) { REQUIRE(f.graphic.isLocked()); REQUIRE_FALSE(f.source->isLocked()); }
    if (!late) {
        f.mouse(QEvent::MouseButtonPress, {1.25, 0});
        f.mouse(QEvent::MouseMove, {1.375, 0});
    }
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    CHECK(f.action->limitsCalls == 0); CHECK(f.action->lastOffsetGripResult() == nullptr);
    CHECK(f.source->isOwner() == (kind != 0)); CHECK(f.graphic.isOwner() == (kind != 1));
    CHECK(f.graphic.undoStateEpoch() == epoch); CHECK(f.graphic.count() == count);
    CHECK(f.graphic.countUndoCycles() == 0); before.unchanged(f.graphic);
    REQUIRE_FALSE(f.context.messages.isEmpty());
}

TEST_CASE("H drag ownership and document locks are checked again after the real setter before publication",
          "[gui][lc-offsetcurve][h-drag][ownership][publication]") {
    const unsigned kind = GENERATE(0u, 1u, 2u);
    Fixture f;
    auto* documentLock = new RS_Layer("H_DRAG_LATE_DOCUMENT_LOCK");
    f.graphic.addLayer(documentLock); documentLock->lock(true); f.graphic.setModified(false);
    auto* rootLayer = f.graphic.getLayer(false);
    const auto restore = qScopeGuard([&] {
        f.source->setOwner(true); f.graphic.setOwner(true); f.graphic.setLayer(rootLayer);
    });
    const State before(*f.source, f.graphic);
    const auto epoch = f.graphic.undoStateEpoch(); const auto count = f.graphic.count();
    f.action->beforeLimits = [&] {
        if (kind == 0) f.source->setOwner(false);
        else if (kind == 1) f.graphic.setOwner(false);
        else f.graphic.setLayer(documentLock);
    };
    f.begin(); f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    const auto* attempted = f.action->lastOffsetGripResult(); REQUIRE(attempted); REQUIRE(*attempted);
    CHECK(attempted->changed); CHECK(attempted->engineRuns == 1); CHECK(f.action->limitsCalls == 1);
    CHECK(f.graphic.undoStateEpoch() == epoch); CHECK(f.graphic.count() == count);
    CHECK(f.graphic.countUndoCycles() == 0); before.unchanged(f.graphic);
    REQUIRE_FALSE(f.context.messages.isEmpty());
    CHECK(f.context.messages.last().contains("source changed"));
}

TEST_CASE("H drag live-context destruction still clears overlays and unregisters its original selection",
          "[gui][lc-offsetcurve][h-drag][lifetime][adjacent]") {
    Fixture f;
    const State before(*f.source, f.graphic);
    const auto listeners = f.graphic.getSelection()->listenerCount();
    f.begin(); f.action.reset();
    CHECK(f.graphic.getSelection()->listenerCount() == listeners - 1);
    CHECK(f.view.getViewPort()->getOverlayEntitiesContainer(RS2::ActionPreviewEntity)->isEmpty());
    CHECK(f.view.getViewPort()->getOverlaysDrawablesContainer(RS2::Snapper)->first() == nullptr);
    before.unchanged(f.graphic);
}

namespace {
class HDragPeerPreview final : public RS_PreviewActionInterface {
public:
    explicit HDragPeerPreview(LC_ActionContext* context)
        : RS_PreviewActionInterface(QStringLiteral("HDragPeerPreview"), context) {}
    ~HDragPeerPreview() override = default;
    unsigned clearNotifications = 0, entityNotifications = 0;
    void clearSharedData() { RS_ActionInterface::clearVisualSnap(); }
    void registerEntityDelayed(RS_Entity* entity) { m_visualSnapManager->processEntityDelayed(entity); }
    bool contains(RS_Entity* entity) const { return !m_visualSnapManager->isNotInVisualSnap(entity); }
protected:
    void onVisualSnapEntityRegistered(RS_Entity* entity) override {
        // storeEntityRef supplies a fresh notification clone. Retire that payload
        // after forwarding the actual preview callback; the shared data owns its own refs.
        const std::unique_ptr<RS_Entity> notification(entity);
        if (entity) ++entityNotifications; else ++clearNotifications;
        RS_PreviewActionInterface::onVisualSnapEntityRegistered(entity);
    }
};
struct HDragPeerLockedRelease {
    Fixture& fixture;
    unsigned deliveries = 0;
    bool neutralBefore = false, neutralAfter = false;
    std::exception_ptr error;
    QObject receiver;
    explicit HDragPeerLockedRelease(Fixture& f) : fixture(f) {
        // Default's listener was connected first. This listener is still inside
        // the peer manager's locked redraw; do not assert or unwind through it.
        QObject::connect(&f.view, &RS_GraphicView::drawingRedrawRequested, &receiver, [this] {
            if (++deliveries != 1) return;
            try {
                neutralBefore = !fixture.action->isOffsetGripDragging() &&
                    fixture.action->getStatus() == DragAction::Neutral;
                fixture.mouse(QEvent::MouseButtonRelease, {1.5, 0});
                neutralAfter = !fixture.action->isOffsetGripDragging() &&
                    fixture.action->getStatus() == DragAction::Neutral;
            } catch (...) { error = std::current_exception(); }
        }, Qt::DirectConnection);
    }
    void check(unsigned expected) const {
        if (error) std::rethrow_exception(error);
        REQUIRE(deliveries == expected);
        CHECK(neutralBefore); CHECK(neutralAfter);
    }
};
void hDragPeerFreshCommit(Fixture& f, const State& before, int cycles) {
    f.begin();
    queuedLifetimeOperation([&] {
        REQUIRE(f.action->isOffsetGripDragging());
        REQUIRE(f.action->getStatus() == DragAction::MovingRef);
    });
    f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE(*result);
    CHECK(result->changed); CHECK(result->engineRuns == 1); CHECK(f.action->limitsCalls == 1);
    const auto live = f.live(); REQUIRE(live.size() == 1); REQUIRE(live.front() != f.source);
    CHECK(live.front()->definition().distance == Catch::Approx(.5));
    CHECK(f.source->isDeleted()); CHECK(f.graphic.countUndoCycles() == cycles + 1);
    before.sourceExact();
}
}

TEST_CASE("H drag a second actual preview manager clears the shared view without a recursive data lock",
          "[gui][lc-offsetcurve][h-drag][shared-snap-lock][clear]") {
    const unsigned clears = GENERATE(1u, 3u);
    Fixture f; HDragPeerPreview peer{&f.context};
    const State before{*f.source, f.graphic};
    const auto epoch = f.graphic.undoStateEpoch(); const auto cycles = f.graphic.countUndoCycles();
    const auto count = f.graphic.count();
    f.begin();
    HDragPeerLockedRelease release{f};
    for (unsigned i = 0; i < clears; ++i) {
        peer.clearSharedData();
        f.clean(); // Must be synchronous, before any explicit event-loop turn.
    }
    release.check(clears); CHECK(peer.clearNotifications == clears); CHECK(peer.entityNotifications == 0);
    CHECK(f.action->limitsCalls == 0); CHECK(f.action->lastOffsetGripResult() == nullptr);
    CHECK(f.graphic.count() == count); CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK(f.graphic.countUndoCycles() == cycles); before.unchanged(f.graphic);
    queuedLifetimeOperation([&] { f.clean(); before.unchanged(f.graphic); });
    hDragPeerFreshCommit(f, before, cycles);
}

TEST_CASE("H drag a second actual preview registration cancels under lock without clearing its new snap data",
          "[gui][lc-offsetcurve][h-drag][shared-snap-lock][registration]") {
    Fixture f; HDragPeerPreview peer{&f.context};
    auto* line = new RS_Line(&f.graphic, RS_Vector{-3, -1}, RS_Vector{-2, -1});
    line->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(line); f.graphic.setModified(false);
    const State before{*f.source, f.graphic};
    const auto epoch = f.graphic.undoStateEpoch(); const auto cycles = f.graphic.countUndoCycles();
    const auto count = f.graphic.count();
    f.begin(); REQUIRE_FALSE(peer.contains(line));
    HDragPeerLockedRelease release{f};
    QEventLoop loop;
    QTimer poll, deadline;
    bool expired = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (peer.entityNotifications != 0 || release.error) loop.quit();
    });
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] { expired = true; loop.quit(); });
    deadline.setSingleShot(true);
    // Exercise the real manager timer and registration path without changing settings.
    peer.registerEntityDelayed(line); poll.start(1); deadline.start(10000); loop.exec();
    REQUIRE_FALSE(expired);
    release.check(1); REQUIRE(peer.entityNotifications == 1); CHECK(peer.clearNotifications == 0);
    f.clean(); REQUIRE(peer.contains(line)); CHECK(peer.hasVisualSnap());
    CHECK(f.action->limitsCalls == 0); CHECK(f.action->lastOffsetGripResult() == nullptr);
    CHECK(f.graphic.count() == count); CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK(f.graphic.countUndoCycles() == cycles); before.unchanged(f.graphic);
    queuedLifetimeOperation([&] { f.clean(); CHECK(peer.contains(line)); before.unchanged(f.graphic); });
    hDragPeerFreshCommit(f, before, cycles);
}

TEST_CASE("R03 native drag rechecks same document view context after a successful final source endpoint read",
          "[gui][lc-offsetcurve][h-drag][r03][final-read][context]") {
    class EndpointReadLine final : public RS_Line {
    public:
        EndpointReadLine(RS_EntityContainer* parent, const RS_Vector& start, const RS_Vector& end)
            : RS_Line(parent, start, end) {}
        std::function<void()> onFinalRead;
        mutable unsigned remaining = 0, callbacks = 0;
        mutable RS_Vector returned{false};
        RS_Vector getEndpoint() const override {
            const auto endpoint = RS_Line::getEndpoint();
            if (remaining && --remaining == 0) {
                ++callbacks; returned = endpoint; onFinalRead();
            }
            return endpoint;
        }
    };
    const bool changeView = GENERATE(false, true);
    Fixture f;
    lc::test::TestGraphicView otherView; otherView.setDocument(&f.graphic);
    REQUIRE(f.source->count() > 0);
    const auto index = f.source->count() - 1;
    auto* old = dynamic_cast<RS_Line*>(f.source->entityAt(index));
    REQUIRE(old);
    const auto start = old->RS_Line::getStartpoint();
    const auto end = old->RS_Line::getEndpoint();
    auto* line = new EndpointReadLine(f.source, start, end);
    line->setPen(old->getPen(false)); line->setLayer(old->getLayer(false));
    REQUIRE(f.source->removeEntity(old));
    f.source->insertEntity(index, line);
    REQUIRE(f.source->hasEditableDefinition());
    f.graphic.setModified(false);
    const State before(*f.source, f.graphic);
    const auto epoch = f.graphic.undoStateEpoch();
    const auto cycles = f.graphic.countUndoCycles(); const auto count = f.graphic.count();
    unsigned preparations = 0;
    const auto restore = qScopeGuard([&] {
        f.action->beforeLimits = {};
        line->remaining = 0; line->onFinalRead = {};
        f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    f.action->beforeLimits = [&] {
        ++preparations;
        // The real clone already exists; its inherited RS_Line::clone has no
        // observer. Arm only the original's post-setter snapshot comparison:
        // cachedLength reads once, then matches() makes its final point read.
        line->onFinalRead = [&] {
            if (changeView) f.context.setDocumentAndView(&f.graphic, &otherView);
        };
        line->remaining = 2;
    };
    f.begin(); f.mouse(QEvent::MouseButtonRelease, {1.5, 0}); f.clean();
    REQUIRE(preparations == 1); REQUIRE(line->callbacks == 1); CHECK(line->remaining == 0);
    CHECK(bits(line->returned.x) == bits(end.x)); CHECK(bits(line->returned.y) == bits(end.y));
    CHECK(bits(line->returned.z) == bits(end.z)); CHECK(line->returned.valid == end.valid);
    const auto* result = f.action->lastOffsetGripResult(); REQUIRE(result); REQUIRE(*result);
    CHECK(result->changed); CHECK(result->engineRuns == 1); CHECK(f.action->limitsCalls == 1);
    REQUIRE(f.context.getDocument() == &f.graphic);
    REQUIRE(f.view.getDocument() == &f.graphic); REQUIRE(otherView.getDocument() == &f.graphic);
    const auto live = f.live(); REQUIRE(live.size() == 1);
    if (changeView) {
        CHECK(f.context.getGraphicView() == &otherView); CHECK(live.front() == f.source);
        CHECK(f.graphic.count() == count); CHECK(f.graphic.undoStateEpoch() == epoch);
        CHECK(f.graphic.countUndoCycles() == cycles); before.unchanged(f.graphic);
    } else {
        // An unchanged context under the identical getter observation still
        // publishes the actual successful Through edit exactly once.
        CHECK(f.context.getGraphicView() == &f.view); CHECK(live.front() != f.source);
        CHECK(live.front()->definition().distance == Catch::Approx(.5));
        CHECK(f.source->isDeleted()); CHECK(f.graphic.count() == count + 1);
        CHECK(f.graphic.countUndoCycles() == cycles + 1); before.sourceExact();
    }
}
