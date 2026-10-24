/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <functional>
#include <utility>
#include <QPointer>
#include <QScopeGuard>
#include <QMouseEvent>
#include "lc_action_modify_trim.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvexdata.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_polyline.h"
#include "lc_undosection.h"
#include "rs_selection.h"

namespace {
class Messages final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& s) override { messages << s; }
};
template<bool Both> class CacheTrim final : public LC_ActionModifyTrim {
public:
    explicit CacheTrim(LC_ActionContext* c) : LC_ActionModifyTrim(c, Both) {}
    LC_StoredCacheTrimOptions options;
    using LC_ActionModifyTrim::ChooseLimitEntity;
    using LC_ActionModifyTrim::ChooseTrimEntity;
    using RS_PreviewActionInterface::m_preview;
    void contextTarget(RS_Entity* e, RS_Vector p) { doInitWithContextEntity(e, p); }
    void guardedHover(RS_Vector p) { auto e = lc::test::eventAt(p.x, p.y); onMouseMoveEvent(getStatus(), &e); }
    void cancel() { auto e = lc::test::eventAt(0, 0); onMouseRightButtonRelease(getStatus(), &e); }
    RS_Entity* retainedLimit() const { return m_limitEntity; }
    void stalePublicEvents() {
        QMouseEvent move(QEvent::MouseMove, QPointF{1, 1}, QPointF{1, 1}, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        mouseMoveEvent(&move);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF{1, 1}, QPointF{1, 1}, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mousePressEvent(&press);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF{1, 1}, QPointF{1, 1}, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&release);
        QMouseEvent cancel(QEvent::MouseButtonRelease, QPointF{1, 1}, QPointF{1, 1}, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&cancel);
    }
    void click(RS_Vector p) { auto e = lc::test::eventAt(p.x, p.y); onMouseLeftButtonRelease(getStatus(), &e); }
    void hover(RS_Vector p) {
        deletePreviewAndHighlights(); auto e = lc::test::eventAt(p.x, p.y);
        onMouseMoveEvent(getStatus(), &e); drawPreviewAndHighlights();
    }
protected:
    LC_StoredCacheTrimOptions storedCacheTrimOptions() const override { return options; }
};
template<bool Both> struct Drawing {
    const bool ready = lc::test::application() != nullptr;
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    Messages context;
    std::unique_ptr<CacheTrim<Both>> action;
    Drawing() {
        graphic.initForNewDocument(); view.setDocument(&graphic); context.setDocumentAndView(&graphic, &view);
        action = std::make_unique<CacheTrim<Both>>(&context);
    }
};
LC_OffsetCurve* addNative(RS_Graphic& graphic, double dx = 0) {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
    for (unsigned q = 0; q < 4; ++q) {
        for (unsigned i = q ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i] + RS_Vector{dx, 0});
        for (auto& p : net) p = {-p.y, p.x};
    }
    for (unsigned q = 0; q <= 4; ++q) for (unsigned i = 0; i < (q == 0 || q == 4 ? 4u : 3u); ++i) data.knotslist.push_back(q);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline cache{&graphic, data}; LC_EllipseOffsetDefinition def;
    def.basis = {{dx, 0}, {1, 0}, 1}; def.distance = 1; def.relTolerance = .01;
    LC_OffsetCheckResult check; auto native = LC_OffsetCurve::adopt(cache, def, .02, &check);
    REQUIRE(check); REQUIRE(native);
    REQUIRE(native->setOwnedExtensions(0x10, {{1000, QStringLiteral("owned trim tail")}}));
    native->setUserDefVar("trim-foreign", "keep"); native->setOwnerLineage(67);
    native->setSourceHandle(101); native->setXDictHandle(102); native->setReactorHandles({103});
    native->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("TRIM_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("keep exactly"))});
    auto* out = native.release(); graphic.addEntity(out); return out;
}
RS_Line* line(RS_Graphic& g, RS_Vector a = {-3, .3}, RS_Vector b = {3, .3}) {
    auto* e = new RS_Line(&g, a, b); g.addEntity(e); return e;
}
std::vector<RS_Entity*> live(RS_Graphic& g) {
    std::vector<RS_Entity*> out; for (auto* e : g) if (e && !e->isDeleted()) out.push_back(e); return out;
}
bool canUndo(RS_Graphic& g) { bool a = false, b = false; g.collectUndoState(a, b); return a; }
void plain(const RS_Entity& e, const LC_OffsetCurve& source) {
    REQUIRE(e.rtti() == RS2::EntitySpline); CHECK(e.getParent() == source.getParent());
    CHECK(e.getPen(false) == source.getPen(false)); CHECK(e.getLayer(false) == source.getLayer(false));
    CHECK(e.getUserDefVar("trim-foreign") == "keep"); CHECK(e.sourceHandle() == 0);
    CHECK(e.xDictHandle() == 0); CHECK(e.reactorHandles().empty()); CHECK(e.ownerLineage() == 0);
    CHECK(e.lineage() != source.lineage());
    const auto parsed = LC_OffsetCurveXData::parseAndStrip(e.getDrwExtData());
    REQUIRE(parsed.remaining); CHECK(parsed.knownCandidates == 0); REQUIRE(e.getDrwExtData().size() == 2);
    REQUIRE(e.getDrwExtData()[1]->type() == DRW_Variant::STRING);
    CHECK(*e.getDrwExtData()[1]->content.s == "keep exactly");
}
}

TEST_CASE("T actions trim a native owner rather than its generated chord with one undo", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
    auto* layer = new RS_Layer("TrimOwner"); f.graphic.addLayer(layer); source->setLayer(layer);
    source->setPen(RS_Pen{RS_Color{20, 80, 120}, RS2::Width07, RS2::DashLine});
    const auto stamp = RS_Spline::geometryFingerprint(source->getData());
    f.action->click({-2.8, .3}); REQUIRE(f.action->getStatus() == CacheTrim<false>::ChooseTrimEntity);
    f.action->hover({2, 0}); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    REQUIRE(f.action->m_preview->count() > 0);
    std::vector<std::uint64_t> preview;
    for (const auto* e : *f.action->m_preview) if (const auto* spline = dynamic_cast<const RS_Spline*>(e)) preview.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    f.action->click({2, 0}); REQUIRE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 2);
    auto* fragment = entities[0] == limit ? entities[1] : entities[0]; plain(*fragment, *source);
    REQUIRE(preview.size() == 1); CHECK(preview.front() == RS_Spline::geometryFingerprint(static_cast<RS_Spline*>(fragment)->getData()));
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("ordinary splines"));
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(fragment->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(source->getData()) == stamp); CHECK(source->hasEditableDefinition());
    CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo()); CHECK(source->isDeleted()); CHECK_FALSE(fragment->isDeleted());
}

TEST_CASE("T actions use a native as an unchanged certified limit", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f; auto* limit = addNative(f.graphic); auto* target = line(f.graphic);
    const auto stamp = RS_Spline::geometryFingerprint(limit->getData());
    f.action->click({2, 0}); f.action->click({0, .3});
    REQUIRE(target->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == stamp);
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
    for (const auto* e : entities) if (e != limit) CHECK(e->rtti() == RS2::EntityLine);
    CHECK(f.context.messages.isEmpty());
}

TEST_CASE("T actions prepare mixed Trim Two in both mouse pick orders", "[lc-offsetcurve][trim-action]") {
    for (bool firstNative : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic); auto* ordinary = line(f.graphic);
        f.action->click(firstNative ? RS_Vector{2, 0} : RS_Vector{0, .3});
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->click(firstNative ? RS_Vector{0, .3} : RS_Vector{2, 0});
        REQUIRE(source->isDeleted()); REQUIRE(ordinary->isDeleted());
        const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
        CHECK(std::count_if(entities.begin(), entities.end(), [](const auto* e) { return e->rtti() == RS2::EntitySpline; }) == 1);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
        for (const auto* e : entities) CHECK(e->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
    }
}

TEST_CASE("T actions stage two native sources before either is replaced", "[lc-offsetcurve][trim-action]") {
    Drawing<true> f; auto* a = addNative(f.graphic); auto* b = addNative(f.graphic, 1);
    f.action->click({-2, 0}); f.action->click({3, 0});
    REQUIRE(a->isDeleted()); REQUIRE(b->isDeleted()); const auto entities = live(f.graphic); REQUIRE(entities.size() == 2);
    for (const auto* e : entities) CHECK(e->rtti() == RS2::EntitySpline);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(a->isDeleted()); CHECK_FALSE(b->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("T actions preserve both sources and selection on cancellation or exhausted second-side budget", "[lc-offsetcurve][trim-action]") {
    for (bool cancel : {false, true}) {
        Drawing<true> f; auto* a = addNative(f.graphic); auto* b = line(f.graphic);
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {a, b}, true);
        const auto before = f.graphic.count();
        if (cancel) f.action->options.cancellation.requested = std::make_shared<std::atomic_bool>(true);
        else f.action->options.maxOutputEntities = 1;
        f.action->click({2, 0}); f.action->click({0, .3});
        CHECK_FALSE(a->isDeleted()); CHECK_FALSE(b->isDeleted()); CHECK(a->isSelected()); CHECK(b->isSelected());
        CHECK(f.graphic.count() == before); CHECK_FALSE(canUndo(f.graphic));
        REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T actions refuse locked hidden stale and foreign-owned native replacements", "[lc-offsetcurve][trim-action]") {
    for (int defect = 0; defect < 4; ++defect) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        LC_DocumentModificationBatch batch;
        if (defect == 0) { auto* layer = new RS_Layer("Locked"); layer->lock(true); f.graphic.addLayer(layer); source->setLayer(layer); }
        if (defect == 1) source->setVisible(false);
        if (defect == 2) source->getData().controlPoints[1].x += .01;
        RS_EntityContainer foreign{nullptr, false}; if (defect == 3) source->setParent(&foreign);
        const auto result = RS_Modification::trimStoredCache(f.graphic, *source, {2, 0}, *limit, {0, .3}, false, batch);
        CHECK_FALSE(result.result); CHECK(batch.entitiesToAdd.empty()); CHECK(batch.entitiesToDelete.empty());
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        if (defect == 3) source->setParent(&f.graphic);
    }
}

TEST_CASE("T actions refuse a root pick and a display cap before publication", "[lc-offsetcurve][trim-action]") {
    for (bool cap : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic, {0, -3}, {0, 3});
        if (cap) f.action->options.maxDisplayEntities = 0;
        f.action->contextTarget(source, cap ? RS_Vector{2, 0} : RS_Vector{0, 2});
        f.action->click({0, -2.8});
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
        CHECK_FALSE(canUndo(f.graphic)); REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T actions resolve an open native child and retain two ordinary fragments", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f;
    const RS_Vector a{2, 0}, b{2 * std::cos(.01), 2 * std::sin(.01)};
    RS_SplineData data{1, false}; data.controlPoints = {a, b}; data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    RS_Spline cache{&f.graphic, data}; LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, .01, false}; definition.distance = 1; definition.relTolerance = .01;
    LC_OffsetCheckResult check; auto native = LC_OffsetCurve::adopt(cache, definition, .02, &check);
    REQUIRE(check); REQUIRE(native); auto* source = native.release(); f.graphic.addEntity(source);
    const double low = b.y / 4, high = 3 * b.y / 4;
    RS_SplineData u{1, false}; u.controlPoints = {{1, low}, {3, low}, {3, high}, {1, high}};
    u.weights.assign(4, 1); u.knotslist = {0, 0, 1, 2, 3, 3};
    auto* limit = new RS_Spline(&f.graphic, u); f.graphic.addEntity(limit);
    REQUIRE(source->size() > 0); f.action->contextTarget(source->entityAt(0), (a + b) * .5);
    f.action->click({3, b.y * .5}); REQUIRE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
    std::vector<RS_Spline*> fragments;
    for (auto* e : entities) if (e != limit) { REQUIRE(e->rtti() == RS2::EntitySpline); fragments.push_back(static_cast<RS_Spline*>(e)); }
    REQUIRE(fragments.size() == 2); CHECK(fragments[0]->getData().controlPoints.front() == a);
    CHECK(fragments[1]->getData().controlPoints.back() == b);
    CHECK(std::abs(fragments[0]->getData().controlPoints.back().y - low) < 1e-11);
    CHECK(std::abs(fragments[1]->getData().controlPoints.front().y - high) < 1e-11);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

namespace {
template<bool Both> void removedLimit(bool hover) {
    Drawing<Both> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    f.action->click({2, 0}); REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    REQUIRE(f.graphic.removeEntity(source)); // Actually frees the captured native address.
    const auto count = f.graphic.count();
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    if (hover) f.action->hover({0, .3}); else f.action->click({0, .3});
    CHECK_FALSE(target->isDeleted()); CHECK(f.graphic.count() == count); CHECK_FALSE(canUndo(f.graphic));
    CHECK(f.action->getStatus() == CacheTrim<Both>::ChooseLimitEntity); CHECK(f.action->m_preview->isEmpty());
    std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("stored source"));
}
}

TEST_CASE("T actions reject a freed retained native limit before hover or commit getters",
          "[lc-offsetcurve][trim-action][retained-owner][review-fix]") {
    for (bool hover : {false, true}) { removedLimit<false>(hover); removedLimit<true>(hover); }
}

TEST_CASE("T actions reject a freed context native target before picking a limit",
          "[lc-offsetcurve][trim-action][retained-owner][review-fix]") {
    for (bool hover : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->contextTarget(source, {2, 0}); REQUIRE(f.graphic.removeEntity(source));
        const auto count = f.graphic.count();
        if (hover) f.action->hover({-2.8, .3}); else f.action->click({-2.8, .3});
        CHECK_FALSE(limit->isDeleted()); CHECK(f.graphic.count() == count); CHECK_FALSE(canUndo(f.graphic));
        CHECK(f.action->m_preview->isEmpty()); REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front().contains("stored source"));
    }
}

namespace {
std::string trimGeometry(const RS_Entity& entity) {
    std::ostringstream out; out << std::hexfloat << int(entity.rtti()) << ':';
    const auto vector = [&](const RS_Vector& p) { out << p.x << ',' << p.y << ',' << p.z << ';'; };
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
        const auto& d = spline->getData(); out << d.degree << ':' << int(d.type) << ':' << d.m_closedFlag << d.m_periodicFlag;
        for (const auto& p : d.controlPoints) vector(p);
        for (double x : d.knotslist) out << x << ';';
        for (double x : d.weights) out << x << ';';
    } else if (const auto* arc = dynamic_cast<const RS_Arc*>(&entity)) {
        const auto& d = arc->getData(); vector(d.center); out << d.radius << ':' << d.angle1 << ':' << d.angle2 << ':' << d.reversed;
    } else if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity)) {
        const auto& d = ellipse->getData(); vector(d.center); vector(d.majorP); out << d.ratio << ':' << d.angle1 << ':' << d.angle2 << ':' << d.reversed;
    }
    return out.str();
}
std::vector<std::string> trimPreview(const RS_EntityContainer& container) {
    std::vector<std::string> out;
    for (const auto* e : container) if (e && (e->rtti() == RS2::EntitySpline || e->rtti() == RS2::EntityArc || e->rtti() == RS2::EntityEllipse))
        out.push_back(trimGeometry(*e));
    return out;
}
}

// Mandatory section 11.3 positive gates. Authored only: no build or execution
// is implied by completing the typed staging path; refusal/skip is not success.
TEST_CASE("T actions must replace ordinary conics with a native in both Trim Two pick orders",
          "[lc-offsetcurve][trim-action][mandatory-conic]") {
    for (unsigned kind = 0; kind < 4; ++kind) for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic);
        std::unique_ptr<RS_Entity> ordinary;
        if (kind == 0) ordinary = std::make_unique<RS_Circle>(nullptr, RS_CircleData{{1, 0}, 2});
        if (kind == 1) ordinary = std::make_unique<RS_Arc>(nullptr, RS_ArcData{{1, 0}, 2, .5, 5.8, false});
        if (kind >= 2) ordinary = std::make_unique<RS_Ellipse>(nullptr,
            RS_EllipseData{{1, 0}, {2, 0}, .75, kind == 2 ? 0. : .5, kind == 2 ? 0. : 5.8, false});
        auto* conic = ordinary.release(); conic->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(conic);
        conic->setUserDefVar("ordinary-trim", "retained");
        conic->setOwnerLineage(267); conic->setSourceHandle(301); conic->setXDictHandle(302); conic->setReactorHandles({303});
        conic->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("CONIC_FOREIGN")),
            std::make_shared<DRW_Variant>(1000, std::string("retain exactly"))});
        const auto conicTags = conic->getDrwExtData();
        const RS_Vector nativePick{2, 0}, conicPick{-1, 0};
        f.action->click(nativeFirst ? nativePick : conicPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->hover(nativeFirst ? conicPick : nativePick);
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        const auto preview = trimPreview(*f.action->m_preview);
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(conic->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        f.action->click(nativeFirst ? conicPick : nativePick);
        REQUIRE(source->isDeleted()); REQUIRE(conic->isDeleted());
        const auto outputs = live(f.graphic); REQUIRE(outputs.size() >= 2); REQUIRE(outputs.size() <= 4);
        std::vector<std::string> committed; for (const auto* e : outputs) committed.push_back(trimGeometry(*e));
        CHECK(committed == preview);
        std::size_t natives = 0, conics = 0;
        for (const auto* output : outputs) {
            REQUIRE(output->getParent() == &f.graphic); CHECK(output->isVisible());
            CHECK(dynamic_cast<const LC_OffsetCurve*>(output) == nullptr);
            if (output->rtti() == RS2::EntitySpline) { ++natives; plain(*output, *source); }
            else {
                ++conics; CHECK(output->rtti() == (kind < 2 ? RS2::EntityArc : RS2::EntityEllipse));
                CHECK(output->getUserDefVar("ordinary-trim") == "retained");
                CHECK(output->getPen(false) == conic->getPen(false)); CHECK(output->getLayer(false) == conic->getLayer(false));
                REQUIRE(output->getLength() > 0); CHECK(output->getLength() < conic->getLength());
                CHECK(output->ownerLineage() == 0); CHECK(output->sourceHandle() == 0);
                CHECK(output->xDictHandle() == 0); CHECK(output->reactorHandles().empty()); CHECK(output->lineage() != conic->lineage());
                CHECK(output->getDrwExtData() == conicTags);
                const auto parsed = LC_OffsetCurveXData::parseAndStrip(output->getDrwExtData());
                REQUIRE(parsed.remaining); CHECK(parsed.knownCandidates == 0);
            }
        }
        CHECK(natives == 1); CHECK(conics >= 1); REQUIRE(f.graphic.undo());
        CHECK(conic->getDrwExtData() == conicTags); CHECK(conic->ownerLineage() == 267);
        CHECK(conic->sourceHandle() == 301); CHECK(conic->xDictHandle() == 302); CHECK(conic->reactorHandles() == std::vector<quint32>{303});
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(conic->isDeleted()); CHECK(source->hasEditableDefinition());
        for (const auto* output : outputs) CHECK(output->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        CHECK(source->isDeleted()); CHECK(conic->isDeleted());
        for (const auto* output : outputs) CHECK_FALSE(output->isDeleted());
    }
}

TEST_CASE("T actions must retain a native limit while replacing an ordinary circle target",
          "[lc-offsetcurve][trim-action][mandatory-conic]") {
    Drawing<false> f; auto* limit = addNative(f.graphic);
    auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
    const auto fingerprint = RS_Spline::geometryFingerprint(limit->getData());
    const auto definition = limit->definition();
    f.action->click({2, 0}); f.action->click({-1, 0});
    REQUIRE(circle->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == fingerprint);
    CHECK(limit->definition().distance == definition.distance); CHECK(limit->unknownOwnedFlags() == 0x10);
    const auto outputs = live(f.graphic); REQUIRE(outputs.size() >= 2);
    for (const auto* output : outputs) if (output != limit) REQUIRE(output->rtti() == RS2::EntityArc);
    CHECK(f.context.messages.isEmpty()); REQUIRE(f.graphic.undo()); CHECK_FALSE(circle->isDeleted());
    CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo()); CHECK(circle->isDeleted());
}

TEST_CASE("T actions trim normalized ordinary spline targets with a native in both pick orders",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic);
        RS_SplineData data{1, false}; data.controlPoints = {{-3, .3}, {3, .3}};
        data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
        auto* ordinary = new RS_Spline(&f.graphic, data); REQUIRE(ordinary->validate());
        ordinary->setUserDefVar("ordinary-trim", "spline"); f.graphic.addEntity(ordinary);
        f.action->click(nativeFirst ? RS_Vector{2, 0} : RS_Vector{0, .3});
        f.action->hover(nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0});
        const auto preview = trimPreview(*f.action->m_preview); REQUIRE(preview.size() == 3);
        f.action->click(nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0});
        REQUIRE(source->isDeleted()); REQUIRE(ordinary->isDeleted()); const auto outputs = live(f.graphic); REQUIRE(outputs.size() == 3);
        std::vector<std::string> committed; unsigned ordinaryPieces = 0;
        for (const auto* e : outputs) {
            REQUIRE(e->rtti() == RS2::EntitySpline); CHECK(dynamic_cast<const LC_OffsetCurve*>(e) == nullptr);
            committed.push_back(trimGeometry(*e));
            if (e->getUserDefVar("ordinary-trim") == "spline") ++ordinaryPieces;
        }
        CHECK(ordinaryPieces == 2); CHECK(committed == preview);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        CHECK(source->isDeleted()); CHECK(ordinary->isDeleted());
    }
}

TEST_CASE("T conic staging refusals preserve both sources selection redo metadata and an existing batch prefix",
          "[lc-offsetcurve][trim-action][conic-atomic-refusal]") {
    for (bool nativeFirst : {false, true}) for (unsigned defect = 0; defect < 6; ++defect) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
        circle->setUserDefVar("ordinary-trim", "keep"); circle->setSourceHandle(201); circle->setXDictHandle(202); circle->setReactorHandles({203});
        circle->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("CONIC_FOREIGN")),
            std::make_shared<DRW_Variant>(1000, std::string("untouched"))});
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, circle}, true);
        auto* marker = line(f.graphic, {-5, -5}, {-4, -5});
        REQUIRE(f.graphic.undoableModify(f.view.getViewPort(), [&](LC_DocumentModificationBatch& b) { b.remove(marker); return true; }));
        REQUIRE(f.graphic.undo());
        const auto nativeShape = trimGeometry(*native), circleShape = trimGeometry(*circle);
        const auto nativeTags = native->getDrwExtData(), circleTags = circle->getDrwExtData();
        std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
        LC_StoredCacheTrimOptions options;
        if (defect == 0) options.maxOutputEntities = 1;
        if (defect == 1) options.maxDisplayEntities = 0;
        if (defect == 2) options.kernel.maxOutputSpans = 0;
        if (defect == 3) options.cancellation.requested = std::make_shared<std::atomic_bool>(true);
        if (defect == 4) options.kernel.maxRoots = 1;
        auto* locked = new RS_Layer("ConicReplacementLocked"); f.graphic.addLayer(locked);
        if (defect == 5) { circle->setLayer(locked); locked->lock(true); }
        const auto* circleLayer = circle->getLayer(false); const auto circlePen = circle->getPen(false);
        const auto* nativeLayer = native->getLayer(false); const auto nativePen = native->getPen(false);
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{-8, -8}, RS_Vector{-7, -7});
        LC_DocumentModificationBatch batch; batch.add(prefix.get()); batch.remove(marker);
        const auto addBefore = batch.entitiesToAdd, deleteBefore = batch.entitiesToDelete;
        const auto result = RS_Modification::trimStoredCache(f.graphic,
            nativeFirst ? static_cast<RS_Entity&>(*native) : static_cast<RS_Entity&>(*circle),
            nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0},
            nativeFirst ? static_cast<RS_Entity&>(*circle) : static_cast<RS_Entity&>(*native),
            nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0}, true, batch, options);
        CHECK_FALSE(result.result); CHECK(batch.entitiesToAdd == addBefore); CHECK(batch.entitiesToDelete == deleteBefore);
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(circle->isDeleted()); CHECK(native->isSelected()); CHECK(circle->getFlag(RS2::FlagSelected));
        CHECK(trimGeometry(*native) == nativeShape); CHECK(trimGeometry(*circle) == circleShape);
        CHECK(native->getDrwExtData() == nativeTags); CHECK(circle->getDrwExtData() == circleTags);
        CHECK(circle->getLayer(false) == circleLayer); CHECK(circle->getPen(false) == circlePen);
        CHECK(native->getLayer(false) == nativeLayer); CHECK(native->getPen(false) == nativePen);
        CHECK(native->unknownOwnedFlags() == 0x10); CHECK(circle->getUserDefVar("ordinary-trim") == "keep");
        CHECK(circle->sourceHandle() == 201); CHECK(circle->xDictHandle() == 202); CHECK(circle->reactorHandles() == std::vector<quint32>{203});
        std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
        if (defect == 5) locked->lock(false);
        REQUIRE(f.graphic.redo()); CHECK(marker->isDeleted());
    }
}

TEST_CASE("T conic Trim Two action refuses before publishing either side in both pick orders",
          "[lc-offsetcurve][trim-action][conic-atomic-refusal]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, circle}, true);
        f.action->options.maxOutputEntities = 1;
        f.action->click(nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0});
        f.action->hover(nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0}); CHECK(f.action->m_preview->isEmpty());
        f.action->click(nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0});
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(circle->isDeleted()); CHECK(native->isSelected()); CHECK(circle->isSelected());
        CHECK_FALSE(canUndo(f.graphic)); REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T conic Trim Two real mouse events share preview commit and exact one step history",
          "[gui][lc-offsetcurve][trim-action][conic-events]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* ellipse = new RS_Ellipse(&f.graphic, {{1, 0}, {2, 0}, .75}); f.graphic.addEntity(ellipse);
        f.view.getViewPort()->setSize(640, 480); f.view.getViewPort()->setOffsetAndFactor(192, 240, 64);
        f.action->init(0);
        const auto nativeBefore = trimGeometry(*native), ellipseBefore = trimGeometry(*ellipse);
        const auto send = [&](QEvent::Type type, RS_Vector point) {
            const QPointF pos(f.view.getViewPort()->toGuiX(point.x), f.view.getViewPort()->toGuiY(point.y));
            QMouseEvent event(type, pos, pos, type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
            if (type == QEvent::MouseButtonPress) f.action->mousePressEvent(&event);
            else if (type == QEvent::MouseButtonRelease) f.action->mouseReleaseEvent(&event);
            else f.action->mouseMoveEvent(&event);
        };
        const RS_Vector first = nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0};
        const RS_Vector second = nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0};
        send(QEvent::MouseButtonPress, first); send(QEvent::MouseButtonRelease, first);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        send(QEvent::MouseMove, second); const auto preview = trimPreview(*f.action->m_preview); REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ellipse->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        send(QEvent::MouseButtonPress, second); send(QEvent::MouseButtonRelease, second);
        REQUIRE(native->isDeleted()); REQUIRE(ellipse->isDeleted()); const auto outputs = live(f.graphic);
        std::vector<std::string> committed; for (const auto* output : outputs) committed.push_back(trimGeometry(*output));
        CHECK(committed == preview); REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ellipse->isDeleted()); CHECK(native->hasEditableDefinition());
        CHECK(trimGeometry(*native) == nativeBefore); CHECK(trimGeometry(*ellipse) == ellipseBefore);
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        for (const auto* output : outputs) CHECK_FALSE(output->isDeleted());
    }
}

TEST_CASE("T native pairing explicitly refuses disabled through points spline trimming without a no op clone",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    Drawing<true> f; auto* native = addNative(f.graphic);
    LC_SplinePointsData data; data.useControlPoints = true; data.controlPoints = {{-3, .3}, {0, 1}, {3, .3}};
    auto* points = new LC_SplinePoints(&f.graphic, data); f.graphic.addEntity(points);
    const auto* controls = points->getData().controlPoints.data();
    LC_DocumentModificationBatch batch;
    const auto result = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *points, {0, 1}, true, batch);
    CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal); CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
    CHECK(batch.entitiesToAdd.empty()); CHECK(batch.entitiesToDelete.empty());
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(points->isDeleted()); CHECK(native->hasEditableDefinition());
    CHECK(points->getData().controlPoints.data() == controls); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("T plain cache target availability never enables one sided ordinary only Trim Two publication",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    Drawing<true> f; auto* unrelatedNative = addNative(f.graphic, 10);
    RS_SplineData data{1, false}; data.controlPoints = {{-3, .3}, {3, .3}}; data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    auto* spline = new RS_Spline(&f.graphic, data); f.graphic.addEntity(spline);
    auto* ordinaryLine = line(f.graphic, {0, -2}, {0, 2});
    f.action->click({-2, .3}); f.action->hover({0, 1}); f.action->click({0, 1});
    CHECK_FALSE(spline->isDeleted()); CHECK_FALSE(ordinaryLine->isDeleted()); CHECK_FALSE(unrelatedNative->isDeleted());
    CHECK_FALSE(canUndo(f.graphic)); CHECK(f.graphic.count() == 3);
}

TEST_CASE("T trim forgets a destroyed document even while Context and view retain its address",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    Drawing<false> f;
    auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    f.action.reset(); f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
    f.action = std::make_unique<CacheTrim<false>>(&f.context);
    auto* source = addNative(*document); line(*document);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == source);
    f.action->guardedHover({0, .3});
    auto* const identity = document.get(); document.reset();
    CHECK(f.view.getDocument() == identity); CHECK(f.context.getDocument() == identity);
    f.view.redrawMethod = RS2::RedrawNone;
    f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
    CHECK(f.action->isFinished()); CHECK(f.action->retainedLimit() == nullptr);
    CHECK(f.action->m_preview->isEmpty()); CHECK(f.action->m_preview->getParent() == nullptr);
    CHECK(f.view.redrawMethod == RS2::RedrawNone);
    f.action.reset(); // The preview/base action destructor must not visit the dead document.
}

TEST_CASE("T trim forgets a destroyed captured view before hover trigger cancel and destruction",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    Drawing<false> f; f.action.reset();
    auto view = std::make_unique<lc::test::TestGraphicView>(); view->setDocument(&f.graphic);
    QPointer<RS_GraphicView> probe = view.get(); f.context.setDocumentAndView(&f.graphic, view.get());
    const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
    f.action = std::make_unique<CacheTrim<false>>(&f.context);
    auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == source);
    f.action->guardedHover({0, .3}); view.reset(); REQUIRE(probe.isNull());
    f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
    CHECK(f.action->isFinished()); CHECK(f.action->m_preview->isEmpty());
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    f.action.reset();
}

TEST_CASE("T trim rejects a closing or switched captured view without drawing through it",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    for (bool closing : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
        f.action->click({2, 0});
        REQUIRE(f.action->retainedLimit() == source);
        // Draw real Trim preparation before invalidating the captured context.
        f.action->hover({0, .3});
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        auto* oldPreviewStore = f.view.getViewPort()->getOverlayEntitiesContainer(RS2::ActionPreviewEntity);
        auto* oldHighlightStore = f.view.getViewPort()->getOverlayEntitiesContainer(RS2::OverlayEffects);
        REQUIRE_FALSE(oldPreviewStore->isEmpty()); REQUIRE_FALSE(oldHighlightStore->isEmpty());
        REQUIRE(oldPreviewStore->findEntityIndex(f.action->m_preview.get()) >= 0);
        lc::test::TestGraphicView other; other.setDocument(&f.graphic);
        Messages otherContext; otherContext.setDocumentAndView(&f.graphic, &other);
        auto otherAction = std::make_unique<CacheTrim<false>>(&otherContext);
        otherAction->click({2, 0}); otherAction->hover({0, .3});
        REQUIRE_FALSE(otherAction->m_preview->isEmpty());
        auto* newPreviewStore = other.getViewPort()->getOverlayEntitiesContainer(RS2::ActionPreviewEntity);
        auto* newHighlightStore = other.getViewPort()->getOverlayEntitiesContainer(RS2::OverlayEffects);
        REQUIRE_FALSE(newPreviewStore->isEmpty()); REQUIRE_FALSE(newHighlightStore->isEmpty());
        REQUIRE(newPreviewStore->findEntityIndex(otherAction->m_preview.get()) >= 0);
        const std::vector<RS_Entity*> newPreviewEntries(newPreviewStore->begin(), newPreviewStore->end());
        const std::vector<RS_Entity*> newHighlightEntries(newHighlightStore->begin(), newHighlightStore->end());
        const auto newPreviewCount = otherAction->m_preview->count();
        const auto members = live(f.graphic);
        const auto sourceStamp = RS_Spline::geometryFingerprint(source->getData());
        const auto targetStart = target->getStartpoint(), targetEnd = target->getEndpoint();
        std::ostringstream undoBefore; undoBefore << static_cast<const RS_Undo&>(f.graphic);
        const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
        if (closing) f.view.beginClose(); else f.context.setDocumentAndView(&f.graphic, &other);
        f.view.redrawMethod = other.redrawMethod = RS2::RedrawNone;
        f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
        CHECK(f.action->isFinished()); CHECK(f.action->m_preview->isEmpty());
        CHECK(f.action->m_preview->getParent() == nullptr);
        CHECK(oldPreviewStore->isEmpty()); CHECK(oldHighlightStore->isEmpty());
        CHECK(std::vector<RS_Entity*>(newPreviewStore->begin(), newPreviewStore->end()) == newPreviewEntries);
        CHECK(std::vector<RS_Entity*>(newHighlightStore->begin(), newHighlightStore->end()) == newHighlightEntries);
        CHECK(otherAction->m_preview->count() == newPreviewCount);
        CHECK_FALSE(otherAction->isFinished());
        // Destruction of the abandoned action must not clear the second view either.
        f.action.reset();
        CHECK(oldPreviewStore->isEmpty()); CHECK(oldHighlightStore->isEmpty());
        CHECK(std::vector<RS_Entity*>(newPreviewStore->begin(), newPreviewStore->end()) == newPreviewEntries);
        CHECK(std::vector<RS_Entity*>(newHighlightStore->begin(), newHighlightStore->end()) == newHighlightEntries);
        CHECK(otherAction->m_preview->count() == newPreviewCount);
        CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(other.redrawMethod == RS2::RedrawNone);
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        CHECK(live(f.graphic) == members); CHECK(f.graphic.count() == members.size());
        CHECK(RS_Spline::geometryFingerprint(source->getData()) == sourceStamp); CHECK(source->hasEditableDefinition());
        CHECK(target->getStartpoint() == targetStart); CHECK(target->getEndpoint() == targetEnd);
        std::ostringstream undoAfter; undoAfter << static_cast<const RS_Undo&>(f.graphic);
        CHECK(undoAfter.str() == undoBefore.str());
    }
}

TEST_CASE("T trim discards freed saved native owner and child Context picks before base initialization",
          "[lc-offsetcurve][trim-action][correctness][context-init]") {
    for (bool child : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        REQUIRE(source->size() > 0);
        f.context.saveContextMenuActionContext(child ? source->entityAt(0) : source, {2, 0}, true);
        REQUIRE(f.graphic.removeEntity(source));
        f.action->init(0);
        CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
        CHECK_FALSE(f.context.getContextMenuActionClickPosition().valid);
        CHECK(f.action->m_preview->isEmpty()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T trim maps a live saved native child through its direct owning container",
          "[lc-offsetcurve][trim-action][correctness][context-init]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
    REQUIRE(source->size() > 0);
    f.context.saveContextMenuActionContext(source->entityAt(0), {2, 0}, false);
    f.action->init(0); f.action->click({-2.8, .3});
    CHECK(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("T trim cancels retained picks after an actual undo epoch change",
          "[lc-offsetcurve][trim-action][correctness][epoch]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    auto* history = line(f.graphic, {10, 10}, {11, 10});
    { LC_UndoSection section(&f.graphic, f.view.getViewPort()); section.addUndoable(history); }
    f.action->click({2, 0}); const auto epoch = f.graphic.undoStateEpoch();
    REQUIRE(f.graphic.undo()); REQUIRE(f.graphic.undoStateEpoch() != epoch);
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    f.action->click({0, .3}); // One actual attempted target pick is refused before any new limit pick.
    CHECK(f.action->retainedLimit() == nullptr); CHECK(f.action->m_preview->isEmpty());
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted());
    std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
}

namespace {
RS_Polyline* polylineLimit(RS_Graphic& graphic, bool arc) {
    auto polyline = std::make_unique<RS_Polyline>(&graphic);
    if (arc) { polyline->addVertex({2, 0}, 1); polyline->addVertex({-2, 0}); }
    else { polyline->addVertex({-3, .3}); polyline->addVertex({3, .3}); }
    polyline->calculateBorders(); auto* out = polyline.release(); graphic.addEntity(out); return out;
}
}

TEST_CASE("T ordinary one sided Trim preserves owned polyline line and arc limits",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    for (bool arc : {false, true}) {
        Drawing<false> f; auto* limit = polylineLimit(f.graphic, arc);
        auto* target = line(f.graphic, {0, -3}, {0, 3});
        auto* edge = limit->entityAt(0); const auto start = edge->getStartpoint(), end = edge->getEndpoint();
        f.action->click(arc ? RS_Vector{2, 0} : RS_Vector{-2.8, .3});
        REQUIRE(f.action->retainedLimit()); CHECK(f.action->retainedLimit() != edge);
        CHECK(f.action->retainedLimit()->getParent() == nullptr);
        f.action->guardedHover({0, -1}); f.action->click({0, -1});
        REQUIRE(target->isDeleted()); CHECK_FALSE(limit->isDeleted());
        CHECK(edge->getStartpoint() == start); CHECK(edge->getEndpoint() == end);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(f.graphic.undo());
    }
}

TEST_CASE("T native one sided Trim uses an owned polyline edge copy after its real owner is freed",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    for (bool removeOwner : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = polylineLimit(f.graphic, false);
        auto* edge = limit->entityAt(0); f.action->click({-2.8, .3});
        REQUIRE(f.action->retainedLimit()); REQUIRE(f.action->retainedLimit() != edge);
        CHECK(f.action->retainedLimit()->getParent() == nullptr);
        if (removeOwner) REQUIRE(f.graphic.removeEntity(limit));
        f.action->guardedHover({2, 0}); f.action->click({2, 0});
        REQUIRE(source->isDeleted()); if (!removeOwner) CHECK_FALSE(limit->isDeleted());
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
    }
}

TEST_CASE("T nested native limits are not converted into detached admissible native clones",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    RS_EntityContainer borrowed(nullptr, false); source->setParent(&borrowed);
    f.action->click({2, 0}); f.action->click({0, .3});
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    source->setParent(&f.graphic);
}

namespace {
class LateLimitLine final : public RS_Line {
public:
    explicit LateLimitLine(RS_EntityContainer* parent) : RS_Line(parent, {-3, .3}, {3, .3}) {}
    mutable unsigned startReads = 0;
    mutable std::function<void()> finalRead;
    RS_Vector getStartpoint() const override {
        const auto point = RS_Line::getStartpoint();
        // Initial source stamp, raw line admission, then final source stamp.
        if (++startReads == 3 && finalRead) { auto callback = std::move(finalRead); callback(); }
        return point;
    }
};
}

TEST_CASE("T one sided native limit closure is revalidated after an actual late target getter",
          "[lc-offsetcurve][trim-action][correctness][late-limit]") {
    for (unsigned defect = 0; defect < 3; ++defect) {
        CAPTURE(defect);
        Drawing<false> f; auto* limit = addNative(f.graphic);
        auto target = std::make_unique<LateLimitLine>(&f.graphic); auto* actual = target.get();
        f.graphic.addEntity(target.release());
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* old = line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
        const auto additions = batch.entitiesToAdd, deletions = batch.entitiesToDelete;
        bool called = false, removed = false;
        actual->startReads = 0;
        actual->finalRead = [&] {
            called = true;
            if (defect == 0) removed = f.graphic.removeEntity(limit);
            else if (defect == 1) limit->getData().controlPoints[1].x += .01;
            else {
                limit->delUserDefVar("trim-foreign");
                limit->setUserDefVar("trim-foreign", "late changed value");
                REQUIRE(limit->getUserDefVar("trim-foreign") == "late changed value");
            }
        };
        const auto result = RS_Modification::trimStoredCache(f.graphic, *actual, {0, .3}, *limit, {2, 0}, false, batch);
        REQUIRE(called); CHECK(actual->startReads == 3); if (defect == 0) CHECK(removed);
        CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
        CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr);
        CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedCutter);
        CHECK(batch.entitiesToAdd == additions); CHECK(batch.entitiesToDelete == deletions);
        CHECK_FALSE(actual->isDeleted()); CHECK_FALSE(old->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T unchanged locked native limits remain read only admissible with full snapshots",
          "[lc-offsetcurve][trim-action][correctness][late-limit]") {
    Drawing<false> f; auto* limit = addNative(f.graphic); auto* target = line(f.graphic);
    auto* layer = new RS_Layer("ReadOnlyNativeLimit"); f.graphic.addLayer(layer); limit->setLayer(layer); layer->lock(true);
    const auto stamp = RS_Spline::geometryFingerprint(limit->getData());
    LC_DocumentModificationBatch batch;
    const auto cleanup = qScopeGuard([&] { for (auto* output : std::as_const(batch.entitiesToAdd)) delete output; });
    const auto result = RS_Modification::trimStoredCache(f.graphic, *target, {0, .3}, *limit, {2, 0}, false, batch);
    REQUIRE(result.result); REQUIRE(batch.entitiesToAdd.size() > 0);
    CHECK(batch.entitiesToDelete == QList<RS_Entity*>{target});
    CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == stamp);
    CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
}

namespace {
class AllNativeClosureLine final : public RS_Line {
public:
    explicit AllNativeClosureLine(RS_EntityContainer* parent) : RS_Line(parent, {-3, .3}, {3, .3}) {}
    mutable std::size_t startReads = 0;
    std::size_t callbackAt = 0;
    mutable std::function<void()> onFinalRead;
    RS_Vector getStartpoint() const override {
        const auto point = RS_Line::getStartpoint();
        if (++startReads == callbackAt && onFinalRead) {
            auto callback = std::move(onFinalRead); callback();
        }
        return point;
    }
};
LC_TrimResult trimNativeLinePair(Drawing<true>& f, LC_OffsetCurve& native, AllNativeClosureLine& line,
                                bool nativeFirst, LC_DocumentModificationBatch& batch) {
    line.startReads = 0;
    if (nativeFirst) return RS_Modification::trimStoredCache(f.graphic, native, {2, 0}, line, {0, .3}, true, batch);
    return RS_Modification::trimStoredCache(f.graphic, line, {0, .3}, native, {2, 0}, true, batch);
}
std::size_t finalNativeLineStartRead(bool nativeFirst) {
    // An actual unchanged preparation identifies the final endpoint read in
    // each pick order. No injected success or guessed read-count can skip it.
    Drawing<true> f; auto* native = addNative(f.graphic);
    auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
    LC_DocumentModificationBatch batch;
    const auto cleanup = qScopeGuard([&] { for (auto* e : std::as_const(batch.entitiesToAdd)) delete e; });
    const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
    REQUIRE(result.result); REQUIRE(line->startReads >= 3);
    REQUIRE(result.trimmed1); REQUIRE(result.trimmed2);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    // In this nonconic pair the last root-line start read is the final source
    // stamp, after both output sets have already been fully constructed.
    return line->startReads;
}
}

TEST_CASE("T Trim Two all native input closure accepts an unchanged final getter in both pick orders",
          "[lc-offsetcurve][trim-action][correctness][all-native-closure]") {
    for (bool nativeFirst : {false, true}) {
        const auto finalRead = finalNativeLineStartRead(nativeFirst);
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
        auto snapshot = LC_OffsetCurveEditSnapshot::capture(*native); REQUIRE(snapshot);
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* old = ::line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
        const auto cleanup = qScopeGuard([&] {
            for (qsizetype i = 1; i < batch.entitiesToAdd.size(); ++i) delete batch.entitiesToAdd.at(i);
        });
        bool called = false; line->callbackAt = finalRead; line->onFinalRead = [&] { called = true; };
        const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
        REQUIRE(called); REQUIRE(line->startReads == finalRead); REQUIRE(result.result);
        REQUIRE(result.trimmed1); REQUIRE(result.trimmed2); CHECK(result.nativeDemotions == 1);
        REQUIRE(batch.entitiesToAdd.size() > 1); CHECK(std::as_const(batch.entitiesToAdd).front() == prefix.get());
        REQUIRE(batch.entitiesToDelete.size() == 3); CHECK(std::as_const(batch.entitiesToDelete).front() == old);
        CHECK(batch.entitiesToDelete.at(1) == (nativeFirst ? static_cast<RS_Entity*>(native) : line));
        CHECK(batch.entitiesToDelete.at(2) == (nativeFirst ? static_cast<RS_Entity*>(line) : native));
        CHECK(batch.entitiesToAdd.contains(result.trimmed1)); CHECK(batch.entitiesToAdd.contains(result.trimmed2));
        for (qsizetype i = 1; i < batch.entitiesToAdd.size(); ++i)
            CHECK(batch.entitiesToAdd.at(i)->rtti() != RS2::EntityOffsetCurve);
        REQUIRE(snapshot->matches(*native)); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted());
        CHECK(line->RS_Line::getStartpoint() == (RS_Vector{-3, .3}));
        CHECK(line->RS_Line::getEndpoint() == (RS_Vector{3, .3})); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T Trim Two revalidates every native after a later ordinary endpoint getter in both pick orders",
          "[lc-offsetcurve][trim-action][correctness][all-native-closure]") {
    for (bool nativeFirst : {false, true}) {
        const auto finalRead = finalNativeLineStartRead(nativeFirst);
        for (unsigned defect = 0; defect < 4; ++defect) {
            CAPTURE(nativeFirst, defect, finalRead);
            Drawing<true> f; auto* native = addNative(f.graphic);
            auto* layer = new RS_Layer("AllNativeClosure"); f.graphic.addLayer(layer); native->setLayer(layer);
            auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
            line->setUserDefVar("ordinary-target", "preserved");
            const auto lineId = line->getId(), lineage = line->lineage();
            const auto pen = line->getPen(false); auto* lineLayer = line->getLayer(false);
            RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, line}, true);
            auto* history = ::line(f.graphic, {12, 12}, {13, 12});
            { LC_UndoSection section(&f.graphic, f.view.getViewPort()); section.addUndoable(history); }
            REQUIRE(f.graphic.undo()); // Keep a genuine redo state across refusal.
            auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
            auto* old = ::line(f.graphic, {10, 1}, {11, 1});
            LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
            const auto additions = batch.entitiesToAdd, deletions = batch.entitiesToDelete;
            std::vector<RS_Entity*> expectedMembers(f.graphic.begin(), f.graphic.end());
            std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
            const auto epoch = f.graphic.undoStateEpoch();
            bool called = false, removed = false;
            std::unique_ptr<LC_OffsetCurveEditSnapshot> expectedNative;
            line->callbackAt = finalRead;
            line->onFinalRead = [&] {
                called = true;
                if (defect == 0) {
                    removed = f.graphic.removeEntity(native);
                    expectedMembers.erase(std::remove(expectedMembers.begin(), expectedMembers.end(), native), expectedMembers.end());
                } else {
                    if (defect == 1) native->getData().controlPoints[1].x += .01;
                    else if (defect == 2) {
                        native->delUserDefVar("trim-foreign");
                        native->setUserDefVar("trim-foreign", "late Trim Two mutation");
                        REQUIRE(native->getUserDefVar("trim-foreign") == "late Trim Two mutation");
                    }
                    else layer->lock(true);
                    expectedNative = LC_OffsetCurveEditSnapshot::capture(*native);
                }
            };
            const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
            REQUIRE(called); CHECK(line->startReads == finalRead);
            CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
            CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
            CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr); CHECK(result.nativeDemotions == 0);
            CHECK(batch.entitiesToAdd == additions); CHECK(batch.entitiesToDelete == deletions);
            CHECK(std::vector<RS_Entity*>(f.graphic.begin(), f.graphic.end()) == expectedMembers);
            if (defect == 0) { CHECK(removed); CHECK(f.graphic.findEntityIndex(native) < 0); }
            else {
                REQUIRE(f.graphic.findEntityIndex(native) >= 0); REQUIRE(expectedNative);
                REQUIRE(expectedNative->matches(*native)); CHECK_FALSE(native->isDeleted()); CHECK(native->isSelected());
            }
            CHECK(line->getId() == lineId); CHECK(line->lineage() == lineage);
            CHECK(line->RS_Line::getStartpoint() == (RS_Vector{-3, .3}));
            CHECK(line->RS_Line::getEndpoint() == (RS_Vector{3, .3}));
            CHECK(line->getPen(false) == pen); CHECK(line->getLayer(false) == lineLayer);
            CHECK(line->getUserDefVar("ordinary-target") == "preserved");
            CHECK_FALSE(line->isDeleted()); CHECK(line->isSelected()); CHECK_FALSE(old->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == epoch);
            std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
        }
    }
}

namespace {
template<bool Both> void ownTrimUndoCycleRetry() {
    CAPTURE(Both);
    Drawing<Both> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    auto* history = line(f.graphic, {10, 10}, {11, 10});
    { LC_UndoSection cycle(&f.graphic, f.view.getViewPort()); cycle.addUndoable(history); }
    REQUIRE(f.graphic.undo()); // A failed empty own cycle must preserve this real redo tail.
    RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, ordinary}, true);
    auto nativeBefore = LC_OffsetCurveEditSnapshot::capture(*native); REQUIRE(nativeBefore);
    const auto nativeGeometry = trimGeometry(*native);
    const auto nativeDataStamp = RS_Spline::geometryFingerprint(native->getData());
    const auto nativeId = native->getId(), nativeLineage = native->lineage();
    const auto nativePen = native->getPen(false); auto* nativeLayer = native->getLayer(false);
    const auto nativeDefinition = native->definition(); const auto nativeTolerance = native->checkedWorldTolerance();
    std::vector<unsigned long long> childIdsBefore;
    std::vector<std::pair<RS_Vector, RS_Vector>> childEndsBefore;
    for (const auto* child : *native) {
        childIdsBefore.push_back(child->getId()); childEndsBefore.emplace_back(child->getStartpoint(), child->getEndpoint());
    }
    f.view.getViewPort()->setSize(640, 480); f.view.getViewPort()->setOffsetAndFactor(192, 240, 64);
    f.action->init(0);
    const auto send = [&](QEvent::Type type, RS_Vector point) {
        const QPointF pos(f.view.getViewPort()->toGuiX(point.x), f.view.getViewPort()->toGuiY(point.y));
        QMouseEvent event(type, pos, pos, type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
            type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        if (type == QEvent::MouseButtonPress) f.action->mousePressEvent(&event);
        else if (type == QEvent::MouseButtonRelease) f.action->mouseReleaseEvent(&event);
        else f.action->mouseMoveEvent(&event);
    };
    const auto click = [&](RS_Vector point) { send(QEvent::MouseButtonPress, point); send(QEvent::MouseButtonRelease, point); };
    click({2, 0}); REQUIRE(f.action->retainedLimit() == native);
    REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    f.action->options.maxOutputEntities = 0;
    const auto beforeEpoch = f.graphic.undoStateEpoch();
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    click({0, .3}); // Real core budget refusal, after the action opens its own empty cycle.
    REQUIRE(f.graphic.undoStateEpoch() == beforeEpoch + 1);
    REQUIRE_FALSE(native->isDeleted()); REQUIRE_FALSE(ordinary->isDeleted());
    CHECK(native->isSelected()); CHECK(ordinary->isSelected()); REQUIRE(nativeBefore->matches(*native));
    REQUIRE(f.action->retainedLimit() == native);
    REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("work or output limit"));
    std::ostringstream refused; refused << static_cast<const RS_Undo&>(f.graphic); CHECK(refused.str() == before.str());
    bool undoAvailable = false, redoAvailable = false; f.graphic.collectUndoState(undoAvailable, redoAvailable);
    CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    f.action->options = {}; // Retry the same retained limit, without init or selecting it again.
    send(QEvent::MouseMove, {0, .3}); REQUIRE_FALSE(f.action->m_preview->isEmpty());
    click({0, .3});
    REQUIRE(ordinary->isDeleted()); CHECK(native->isDeleted() == Both);
    std::vector<RS_Entity*> outputs;
    for (auto* entity : live(f.graphic)) if (entity != native) outputs.push_back(entity);
    REQUIRE_FALSE(outputs.empty());
    for (const auto* entity : outputs) CHECK(entity->rtti() != RS2::EntityOffsetCurve);
    if constexpr (!Both) REQUIRE(nativeBefore->matches(*native));
    f.graphic.collectUndoState(undoAvailable, redoAvailable); CHECK(undoAvailable); CHECK_FALSE(redoAvailable);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
    for (const auto* entity : outputs) CHECK(entity->isDeleted());
    std::vector<unsigned long long> childIdsAfter;
    for (const auto* child : *native) childIdsAfter.push_back(child->getId());
    INFO("Undo native root ID " << nativeId << " -> " << native->getId());
    REQUIRE_FALSE(childIdsBefore.empty()); REQUIRE_FALSE(childIdsAfter.empty());
    INFO("Undo first display child ID " << childIdsBefore.front() << " -> " << childIdsAfter.front());
    // Undoing deletion calls update(): the root/semantic cache are restored,
    // but a replaced native's temporary display children get fresh identities.
    CHECK((childIdsAfter != childIdsBefore) == Both);
    if constexpr (!Both) REQUIRE(nativeBefore->matches(*native));
    CHECK(native->getId() == nativeId); CHECK(native->lineage() == nativeLineage);
    CHECK(native->getParent() == &f.graphic); CHECK(native->getLayer(false) == nativeLayer); CHECK(native->getPen(false) == nativePen);
    CHECK(trimGeometry(*native) == nativeGeometry);
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == nativeDataStamp); CHECK(native->hasEditableDefinition());
    CHECK(native->checkedWorldTolerance() == nativeTolerance);
    const auto& definitionAfter = native->definition();
    CHECK(definitionAfter.distance == nativeDefinition.distance); CHECK(definitionAfter.towardCentre == nativeDefinition.towardCentre);
    CHECK(definitionAfter.relTolerance == nativeDefinition.relTolerance); CHECK(definitionAfter.cacheReversed == nativeDefinition.cacheReversed);
    const auto& a = definitionAfter.basis; const auto& b = nativeDefinition.basis;
    CHECK(a.center == b.center); CHECK(a.majorP == b.majorP); CHECK(a.ratio == b.ratio);
    CHECK(a.angle1 == b.angle1); CHECK(a.angle2 == b.angle2); CHECK(a.reversed == b.reversed); CHECK(a.isArc == b.isArc);
    CHECK(a.angleDegrees == b.angleDegrees); CHECK(a.startAngleDegrees == b.startAngleDegrees);
    CHECK(a.otherAngleDegrees == b.otherAngleDegrees); CHECK(a.angularLength == b.angularLength);
    CHECK(native->sourceHandle() == 101); CHECK(native->xDictHandle() == 102);
    CHECK(native->reactorHandles() == std::vector<quint32>{103}); CHECK(native->ownerLineage() == 67);
    CHECK(native->getUserDefVar("trim-foreign") == "keep"); CHECK(native->unknownOwnedFlags() == 0x10);
    REQUIRE(native->unknownOwnedTail().size() == 1); CHECK(native->unknownOwnedTail().front().code == 1000);
    CHECK(std::get<QString>(native->unknownOwnedTail().front().value) == QStringLiteral("owned trim tail"));
    REQUIRE(native->count() == childEndsBefore.size()); std::size_t childIndex = 0;
    for (const auto* child : *native) {
        CHECK(child->getParent() == native); CHECK(child->rtti() == RS2::EntityLine);
        CHECK(child->getStartpoint() == childEndsBefore[childIndex].first);
        CHECK(child->getEndpoint() == childEndsBefore[childIndex].second); ++childIndex;
    }
    f.graphic.collectUndoState(undoAvailable, redoAvailable); CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    REQUIRE(f.graphic.redo()); CHECK(ordinary->isDeleted()); CHECK(native->isDeleted() == Both);
    for (const auto* entity : outputs) CHECK_FALSE(entity->isDeleted());
}
}

TEST_CASE("T actual public mouse Trim admits only its own undo cycle and retries an empty refusal in both modes",
          "[gui][lc-offsetcurve][trim-action][correctness][own-undo-epoch]") {
    ownTrimUndoCycleRetry<false>(); ownTrimUndoCycleRetry<true>();
}

namespace {
class TrimCompletionObserver final : public LC_DocumentModificationListener {
public:
    std::function<void()> completed;
    unsigned calls = 0;
    void graphicModified(const RS_Graphic*, bool) override {}
    void undoStateChanged(const RS_Document*, bool undoAvailable, bool) override {
        if (undoAvailable && calls == 0) {
            ++calls; // Arm once before invoking an observer that itself changes undo state.
            completed();
        }
    }
};
}

TEST_CASE("T Trim completion does not adopt an external undo from the actual end cycle notification",
          "[lc-offsetcurve][trim-action][correctness][completion-boundary]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* target = line(f.graphic);
    const auto sourceGeometry = trimGeometry(*native);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == native);
    const auto admitted = f.graphic.undoStateEpoch();
    TrimCompletionObserver observer; bool undone = false;
    observer.completed = [&] { undone = f.graphic.undo(); };
    f.graphic.setModificationListener(&observer, true);
    const auto detach = qScopeGuard([&] { f.graphic.setModificationListener(&observer, false); });
    f.action->click({0, .3});
    REQUIRE(observer.calls == 1); REQUIRE(undone);
    REQUIRE(f.graphic.undoStateEpoch() == admitted + 2);
    CHECK_FALSE(target->isDeleted()); CHECK_FALSE(native->isDeleted());
    CHECK(trimGeometry(*native) == sourceGeometry); CHECK(native->hasEditableDefinition());
    CHECK(f.action->retainedLimit() == nullptr);
    CHECK(f.action->getStatus() == CacheTrim<false>::ChooseLimitEntity);
    CHECK(f.action->m_preview->isEmpty());
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("stored source"));
    bool undoAvailable = false, redoAvailable = false; f.graphic.collectUndoState(undoAvailable, redoAvailable);
    CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    REQUIRE(f.graphic.redo()); CHECK(target->isDeleted()); CHECK_FALSE(native->isDeleted());
    CHECK(observer.calls == 1);
}

TEST_CASE("T Trim completion abandons detached closing destroyed and switched views at the end cycle boundary",
          "[lc-offsetcurve][trim-action][correctness][completion-boundary]") {
    for (unsigned teardown = 0; teardown < 4; ++teardown) {
        CAPTURE(teardown);
        Drawing<true> f; f.action.reset();
        auto view = std::make_unique<lc::test::TestGraphicView>(); view->setDocument(&f.graphic);
        QPointer<RS_GraphicView> probe = view.get();
        lc::test::TestGraphicView other; other.setDocument(&f.graphic);
        f.context.setDocumentAndView(&f.graphic, view.get());
        const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
        f.action = std::make_unique<CacheTrim<true>>(&f.context);
        auto* native = addNative(f.graphic); auto* target = line(f.graphic);
        f.action->click({2, 0});
        TrimCompletionObserver observer;
        observer.completed = [&] {
            if (teardown == 0) f.context.setDocumentAndView(nullptr, nullptr);
            else if (teardown == 1) view->beginClose();
            else if (teardown == 2) view.reset();
            else f.context.setDocumentAndView(&f.graphic, &other);
            if (view) view->redrawMethod = RS2::RedrawNone;
            f.view.redrawMethod = other.redrawMethod = RS2::RedrawNone;
        };
        f.graphic.setModificationListener(&observer, true);
        const auto detach = qScopeGuard([&] { f.graphic.setModificationListener(&observer, false); });
        f.action->click({0, .3});
        REQUIRE(observer.calls == 1); CHECK(f.action->isFinished());
        CHECK(f.action->retainedLimit() == nullptr); CHECK(f.action->m_preview->isEmpty());
        CHECK(f.action->m_preview->getParent() == nullptr);
        CHECK(f.context.messages.isEmpty()); // No success notice through the obsolete context.
        CHECK(native->isDeleted()); CHECK(target->isDeleted()); // The already committed batch remains intact.
        if (teardown == 2) CHECK(probe.isNull());
        else CHECK(view->redrawMethod == RS2::RedrawNone);
        CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(other.redrawMethod == RS2::RedrawNone);
        f.action.reset(); // Base teardown must also avoid the invalidated view.
        if (view) CHECK(view->redrawMethod == RS2::RedrawNone);
        CHECK(other.redrawMethod == RS2::RedrawNone);
    }
}

TEST_CASE("R03 Trim Two seals both natives after final peer mutation or late cancellation",
          "[lc-offsetcurve][trim-action][r03-final-closure]") {
    class FourthReadLine final : public RS_Line {
    public:
        FourthReadLine(LC_OffsetCurve& root, const RS_LineData& data) : RS_Line(&root, data) {}
        RS_Vector getStartpoint() const override {
            const auto point = RS_Line::getStartpoint();
            if (armed && ++reads == 4 && onFourth) {
                auto callback = std::move(onFourth);
                callback();
            }
            return point;
        }
        bool armed = false;
        mutable unsigned reads = 0;
        mutable std::function<void()> onFourth;
    };
    for (bool reverse : {false, true}) for (unsigned mode : {0u, 1u, 2u}) {
        CAPTURE(reverse, mode); // New metadata, cancellation, unchanged positive control.
        Drawing<true> f;
        auto* left = addNative(f.graphic);
        auto* right = addNative(f.graphic, 1);
        auto* first = reverse ? right : left;
        auto* later = reverse ? left : right;
        const RS_Vector firstPick = reverse ? RS_Vector{3, 0} : RS_Vector{-2, 0};
        const RS_Vector laterPick = reverse ? RS_Vector{-2, 0} : RS_Vector{3, 0};
        const auto* old = dynamic_cast<const RS_Line*>(later->entityAt(0));
        REQUIRE(old);
        const auto data = old->getData();
        auto* observed = new FourthReadLine(*later, data);
        observed->setPen(old->getPen(false)); observed->setLayer(old->getLayer(false));
        REQUIRE(later->removeEntity(const_cast<RS_Line*>(old)));
        later->insertEntity(0, observed);
        REQUIRE(first->hasEditableDefinition()); REQUIRE(later->hasEditableDefinition());
        const auto firstGeometry = RS_Spline::geometryFingerprint(first->getData());
        const auto laterGeometry = RS_Spline::geometryFingerprint(later->getData());
        REQUIRE(first->getUserDefVar("R03_TRIM_FINAL_METADATA").isEmpty());
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* oldPrefix = line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch;
        batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(oldPrefix);
        const auto addPrefix = batch.entitiesToAdd, deletePrefix = batch.entitiesToDelete;
        const auto cleanup = qScopeGuard([&] {
            while (batch.entitiesToAdd.size() > 1) delete batch.entitiesToAdd.takeLast();
        });
        LC_StoredCacheTrimOptions options;
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        options.cancellation.requested = cancelled;
        unsigned fired = 0, readAtMutation = 0;
        observed->onFourth = [&] {
            ++fired; readAtMutation = observed->reads;
            if (mode == 0) first->setUserDefVar("R03_TRIM_FINAL_METADATA", "external final change");
            if (mode == 1) cancelled->store(true, std::memory_order_relaxed);
        };
        observed->armed = true;
        f.graphic.setModified(false);
        const auto result = RS_Modification::trimStoredCache(f.graphic, *first, firstPick,
            *later, laterPick, true, batch, options);
        REQUIRE(fired == 1); REQUIRE(readAtMutation == 4);
        CHECK(observed->getData().startpoint == data.startpoint);
        CHECK(observed->getData().endpoint == data.endpoint);
        CHECK(RS_Spline::geometryFingerprint(first->getData()) == firstGeometry);
        CHECK(RS_Spline::geometryFingerprint(later->getData()) == laterGeometry);
        CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
        CHECK_FALSE(oldPrefix->isDeleted()); CHECK_FALSE(f.graphic.isModified());
        CHECK_FALSE(canUndo(f.graphic));
        if (mode < 2) {
            CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
            CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr);
            CHECK(batch.entitiesToAdd == addPrefix); CHECK(batch.entitiesToDelete == deletePrefix);
            if (mode == 0) {
                CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
                CHECK(first->getUserDefVar("R03_TRIM_FINAL_METADATA") == "external final change");
            } else CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::Cancelled);
        } else {
            REQUIRE(result.result); CHECK_FALSE(result.cacheRefusal);
            REQUIRE(result.trimmed1); REQUIRE(result.trimmed2);
            CHECK(result.nativeDemotions == 2);
            REQUIRE(batch.entitiesToAdd.size() == 3); REQUIRE(batch.entitiesToDelete.size() == 3);
            CHECK(batch.entitiesToAdd.front() == prefix.get()); CHECK(batch.entitiesToDelete.front() == oldPrefix);
            plain(*result.trimmed1, *first); plain(*result.trimmed2, *later);
        }
    }
}
