/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>
#include <type_traits>

#include <QApplication>
#include <QCursor>
#include <QSettings>
#include <QStringList>

#include "drw_base.h"
#include "lc_action_offsetcurve_updatesource.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurveeditsnapshot.h"
#include <QScopeGuard>
#include "lc_offsetcurveui.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_dimlinear.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_preview.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;
static_assert(std::is_same_v<decltype(LC_ActionOffsetCurveUpdateSource::Outcome{}.targetId),
    decltype(std::declval<const RS_Entity&>().getId())>);

class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override { messages.append(message); }
};

class Probe final : public LC_ActionOffsetCurveUpdateSource {
public:
    explicit Probe(LC_ActionContext* context) : LC_ActionOffsetCurveUpdateSource(context) {}
    using LC_ActionOffsetCurveUpdateSource::isEntityAllowedToSelect;
    using LC_ActionOffsetCurveUpdateSource::m_limits;
    using LC_ActionOffsetCurveUpdateSource::useSource;
    using LC_ActionPreSelectionAwareBase::m_selectionComplete;
    using RS_PreviewActionInterface::m_preview;
    // Call the real protected preparation callback while useSource supplies its private source identity.
    LC_DocumentModificationBatch* preparationOnlyBatch = nullptr;
    std::optional<bool> preparationOnlyResult;
    void trigger() override {
        if (!preparationOnlyBatch) {
            LC_ActionOffsetCurveUpdateSource::trigger();
            return;
        }
        preparationOnlyResult =
            LC_ActionOffsetCurveUpdateSource::doTriggerModifications(*preparationOnlyBatch);
    }
    void hover(const RS_Vector& point) {
        deletePreviewAndHighlights();
        const auto event = lc::test::eventAt(point.x, point.y);
        onMouseMoveEventSelected(0, &event);
        drawPreviewAndHighlights();
    }
    void click(const RS_Vector& point) {
        const auto event = lc::test::eventAt(point.x, point.y);
        onMouseLeftButtonReleaseSelected(0, &event);
    }
};

struct KeepSelected {
    QSettings* store = RS_Settings::instance()->getSettings();
    bool present = store->contains("Modify/KeepModifiedSelected");
    QVariant previous = store->value("Modify/KeepModifiedSelected");
    explicit KeepSelected(bool value) {
        RS_SETTINGS->writeSingle("Modify", "KeepModifiedSelected", value);
    }
    ~KeepSelected() {
        const auto group = RS_SETTINGS->beginGroupGuard("Modify");
        if (present) RS_SETTINGS->writeEntry("KeepModifiedSelected", previous);
        else RS_SETTINGS->remove("KeepModifiedSelected");
    }
};

struct Fixture {
    const bool ready{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    Context context;
    std::unique_ptr<Probe> action;

    Fixture() {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
    }
    RS_Document& document() { return *context.getDocument(); }
    template <typename T> T* add(T* entity) {
        entity->setLayer(graphic.findLayer("0"));
        document().addEntity(entity);
        return entity;
    }
    RS_Ellipse* ellipse(const RS_EllipseData& basis) {
        return add(new RS_Ellipse(&document(), basis));
    }
    LC_OffsetCurve* arc(double distance = .25, bool inward = false, double major = 1.) {
        LC_EllipseOffsetDefinition definition;
        definition.basis = {{0, 0}, {major, 0}, 1., 0., .1, false};
        definition.distance = distance;
        definition.towardCentre = inward;
        definition.relTolerance = .01;
        const double radius = major + (inward ? -distance : distance);
        RS_Line cache(nullptr, {radius, 0}, {radius*std::cos(.1), radius*std::sin(.1)});
        auto curve = LC_OffsetCurve::adopt(cache, definition, .01);
        REQUIRE(curve);
        return add(curve.release());
    }
    LC_OffsetCurve* ring() {
        RS_SplineData data{3, false};
        std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
        for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
            for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
            for (auto& point : net) point = {-point.y, point.x};
        }
        for (unsigned i = 0; i <= 4; ++i)
            for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
        data.weights.assign(data.controlPoints.size(), 1.);
        data.m_closedFlag = true;
        RS_Spline cache(nullptr, data);
        LC_EllipseOffsetDefinition definition;
        definition.basis = {{0, 0}, {1, 0}, 1.};
        definition.distance = 1.;
        definition.relTolerance = .01;
        auto curve = LC_OffsetCurve::adopt(cache, definition, .02);
        REQUIRE(curve);
        return add(curve.release());
    }
    void start(std::initializer_list<RS_Entity*> targets) {
        for (auto* target : targets) document().select(QList<RS_Entity*>{target}, true);
        action = std::make_unique<Probe>(&context);
        action->init(0);
    }
    QList<LC_OffsetCurve*> live() {
        QList<LC_OffsetCurve*> result;
        for (auto* entity : document())
            if (entity && !entity->isDeleted() && entity->rtti() == RS2::EntityOffsetCurve)
                result.append(static_cast<LC_OffsetCurve*>(entity));
        return result;
    }
};

void noUndo(RS_Document& document) {
    bool undo = true, redo = true;
    document.collectUndoState(undo, redo);
    CHECK_FALSE(undo);
    CHECK_FALSE(redo);
}

void notRun(const LC_OffsetCurveEditResult& edit) {
    CHECK(edit.engineRuns == 0);
    CHECK_FALSE(edit.engineStatus);
    CHECK_FALSE(edit.checkReason);
    CHECK(edit.checkWork.cacheSpans == 0);
    CHECK(edit.checkWork.vertices == 0);
    CHECK(edit.checkWork.distancePairs == 0);
    CHECK(edit.checkWork.orderingComparisons == 0);
    CHECK(edit.checkWork.deepest == 0);
}

void regenerated(const LC_OffsetCurveEditResult& edit) {
    REQUIRE(edit);
    CHECK(edit.changed);
    CHECK(edit.engineRuns == 1);
    REQUIRE(edit.engineStatus);
    CHECK(*edit.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(edit.checkReason);
    CHECK(*edit.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(edit.checkWork.cacheSpans > 0);
    CHECK(edit.checkWork.vertices > 0);
}

void sameBasis(const RS_EllipseData& actual, RS_EllipseData expected) {
    REQUIRE(geo::normaliseBasis(expected));
    CHECK(actual.center == expected.center);
    CHECK(actual.majorP == expected.majorP);
    CHECK(actual.ratio == expected.ratio);
    CHECK(actual.angle1 == expected.angle1);
    CHECK(actual.angle2 == expected.angle2);
    CHECK(actual.reversed == expected.reversed);
}

void certified(const LC_OffsetCurve& curve) {
    REQUIRE(curve.hasEditableDefinition());
    const auto check = geo::checkCache(curve.getData(), curve.definition(), curve.checkedWorldTolerance());
    CAPTURE(check.reason, check.span);
    REQUIRE(check);
}

const LC_OffsetCurveEditResult& onlyEdit(const Probe& action) {
    const auto& outcomes = action.lastReport().outcomes;
    REQUIRE(outcomes.size() == 1);
    REQUIRE(outcomes.front().edit.has_value());
    return *outcomes.front().edit;
}
}

TEST_CASE("I update ignores wrong type context and collects real native preselection",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    auto* line = f.add(new RS_Line(&f.graphic, {0, 3}, {1, 3}));
    f.context.saveContextMenuActionContext(line, {0, 3}, false);
    f.start({curve, line});
    CHECK(f.action->rtti() == RS2::ActionOffsetCurveUpdateSource);
    CHECK(f.action->m_selectionComplete);
    CHECK(curve->isSelected());
    CHECK(line->isSelected());
    CHECK_FALSE(f.action->isEntityAllowedToSelect(line));
    auto* source = f.ellipse(curve->definition().basis);
    f.action->useSource(source);
    CHECK(f.action->lastReport().unchanged == 1);
    CHECK(f.action->lastReport().refused == 0);
    CHECK(line->isSelected());
    noUndo(f.graphic);
}

TEST_CASE("I update wrong context without targets stays in target selection",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* source = f.ellipse({{0, 0}, {2, 0}, 1.});
    f.context.saveContextMenuActionContext(source, {2, 0}, false);
    f.start({});
    CHECK_FALSE(f.action->m_selectionComplete);
    CHECK_FALSE(f.action->isFinished());
    CHECK_FALSE(source->isSelected());
    CHECK(f.action->lastReport().outcomes.isEmpty());
    noUndo(f.graphic);
}

TEST_CASE("I update valid context starts source picking and hover changes no document state",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    const auto sourceData = source->getData();
    f.context.saveContextMenuActionContext(curve, {2, 0}, false);
    const bool preselected = GENERATE(false, true);
    if (preselected) f.document().select(QList<RS_Entity*>{curve}, true);
    f.start({});
    REQUIRE(f.action->m_selectionComplete);
    CHECK(curve->isSelected());
    for (unsigned i = 0; i < 8; ++i) f.action->hover({5, 8});
    CHECK(f.action->lastReport().outcomes.isEmpty());
    CHECK(f.action->m_preview->count() == 0);
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    sameBasis(source->getData(), sourceData);
    noUndo(f.graphic);
}

TEST_CASE("I update exact rigid translation replaces once preserves attributes and has one undo",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    KeepSelected keep(true);
    auto* curve = f.ring();
    const auto oldData = curve->getData();
    const auto oldDefinition = curve->definition();
    const auto oldFingerprint = RS_Spline::geometryFingerprint(oldData);
    curve->setPen(RS_Pen(RS_Color(17, 31, 47), RS2::Width11, RS2::DashLine));
    const auto pen = curve->getPen(false);
    const auto layer = curve->getLayer();
    REQUIRE(curve->setOwnedExtensions(0x10, {{1000, QStringLiteral("old proof tail")}}));
    auto basis = oldDefinition.basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    const auto sourceId = source->getId();
    f.start({curve});

    struct Cursor {
        Cursor() { QApplication::setOverrideCursor(Qt::CrossCursor); }
        ~Cursor() { QApplication::restoreOverrideCursor(); }
    } cursor;
    f.action->click({5, 8});
    const auto& report = f.action->lastReport();
    CHECK(report.updated == 1);
    CHECK(report.unchanged == 0);
    CHECK(report.refused == 0);
    const auto& edit = onlyEdit(*f.action);
    REQUIRE(edit);
    CHECK(edit.changed);
    notRun(edit);
    REQUIRE(QApplication::overrideCursor());
    CHECK(QApplication::overrideCursor()->shape() == Qt::CrossCursor);
    auto live = f.live();
    REQUIRE(live.size() == 1);
    auto* updated = live.front();
    CHECK(updated->getParent() == &f.graphic);
    CHECK(updated->getPen(false) == pen);
    CHECK(updated->getLayer() == layer);
    CHECK(updated->isSelected());
    CHECK(updated->unknownOwnedFlags() == 0);
    CHECK(updated->unknownOwnedTail().empty());
    CHECK(updated->definition().distance == oldDefinition.distance);
    CHECK(updated->definition().towardCentre == oldDefinition.towardCentre);
    CHECK(updated->definition().cacheReversed == oldDefinition.cacheReversed);
    CHECK(updated->definition().relTolerance == oldDefinition.relTolerance);
    sameBasis(updated->definition().basis, basis);
    REQUIRE(updated->getData().controlPoints.size() == oldData.controlPoints.size());
    for (std::size_t i = 0; i < oldData.controlPoints.size(); ++i)
        CHECK(updated->getData().controlPoints[i] == oldData.controlPoints[i] + RS_Vector(4, 8));
    certified(*updated);
    const auto newFingerprint = RS_Spline::geometryFingerprint(updated->getData());

    REQUIRE(f.graphic.undo());
    live = f.live();
    REQUIRE(live.size() == 1);
    CHECK(RS_Spline::geometryFingerprint(live.front()->getData()) == oldFingerprint);
    CHECK(live.front()->unknownOwnedFlags() == 0x10);
    REQUIRE(f.graphic.redo());
    live = f.live();
    REQUIRE(live.size() == 1);
    CHECK(RS_Spline::geometryFingerprint(live.front()->getData()) == newFingerprint);
    CHECK(source->getId() == sourceId);
    sameBasis(source->getData(), basis);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("I update no op retains identity selection and extensions without an undo cycle",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    KeepSelected keep(false);
    auto* curve = f.ring();
    REQUIRE(curve->setOwnedExtensions(0x10, {{1000, QStringLiteral("retain")}}));
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    const auto count = f.graphic.count();
    const auto id = curve->getId();
    auto* source = f.ellipse(curve->definition().basis);
    f.start({curve});
    f.action->useSource(source);
    CHECK(f.action->lastReport().updated == 0);
    CHECK(f.action->lastReport().unchanged == 1);
    CHECK(f.action->lastReport().refused == 0);
    notRun(onlyEdit(*f.action));
    REQUIRE(f.live().size() == 1);
    CHECK(f.live().front() == curve);
    CHECK(curve->getId() == id);
    CHECK(curve->isSelected());
    CHECK(curve->unknownOwnedFlags() == 0x10);
    CHECK(curve->unknownOwnedTail().size() == 1);
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    CHECK(f.graphic.count() == count + 1);
    noUndo(f.graphic);
}

TEST_CASE("I update reads a visible locked source through the actual typed picker",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    auto* locked = new RS_Layer("I_UPDATE_LOCKED_SOURCE");
    f.graphic.addLayer(locked);
    source->setLayer(locked);
    locked->lock(true);
    REQUIRE(source->isVisible());
    REQUIRE(source->isLocked());
    f.start({curve});
    f.action->hover({5, 8});
    f.action->click({5, 8});
    CHECK(f.action->lastReport().updated == 1);
    notRun(onlyEdit(*f.action));
    CHECK(source->isLocked());
    CHECK_FALSE(source->isDeleted());
    sameBasis(source->getData(), basis);
}

TEST_CASE("I update shape sweep rotation and directed reversal run the real native setter once",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    const auto before = curve->definition();
    const unsigned change = GENERATE(0u, 1u, 2u, 3u, 4u, 5u);
    auto basis = before.basis;
    const double pi = std::acos(-1.);
    if (change == 0) basis.majorP = {2, 0};
    if (change == 1) basis.ratio = .8;
    if (change == 2) basis.angle2 = .2;
    if (change == 3) { std::swap(basis.angle1, basis.angle2); basis.reversed = true; }
    if (change == 4) { basis.majorP = {-1, 0}; basis.angle1 += pi; basis.angle2 += pi; basis.center = {4, 8}; }
    if (change == 5) basis.angle1 = .02;
    auto* source = f.ellipse(basis);
    f.start({curve});
    f.action->useSource(source);
    CAPTURE(change);
    const auto& report = f.action->lastReport();
    REQUIRE(report.updated == 1);
    CHECK(report.refused == 0);
    const auto& edit = onlyEdit(*f.action);
    REQUIRE(edit);
    CHECK(edit.engineRuns == 1);
    REQUIRE(edit.engineStatus);
    CHECK(*edit.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(edit.checkReason);
    CHECK(*edit.checkReason == LC_OffsetCheckResult::Reason::None);
    REQUIRE(f.live().size() == 1);
    const auto* updated = f.live().front();
    sameBasis(updated->definition().basis, basis);
    CHECK(updated->definition().distance == before.distance);
    CHECK(updated->definition().towardCentre == before.towardCentre);
    CHECK(updated->definition().cacheReversed == before.cacheReversed);
    CHECK(updated->definition().relTolerance == before.relTolerance);
    certified(*updated);
}

TEST_CASE("I update takes a mirrored source definition without changing physical side or cache reversal",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    curve->revertDirection();
    REQUIRE(curve->hasEditableDefinition());
    const auto before = curve->definition();
    auto* source = f.ellipse(before.basis);
    source->mirror({0, 0}, {1, 0});
    const auto mirrored = source->getData();
    f.start({curve});
    f.action->useSource(source);
    REQUIRE(f.action->lastReport().updated == 1);
    CHECK(onlyEdit(*f.action).engineRuns == 1);
    REQUIRE(f.live().size() == 1);
    const auto* updated = f.live().front();
    sameBasis(updated->definition().basis, mirrored);
    CHECK(updated->definition().towardCentre == before.towardCentre);
    CHECK(updated->definition().cacheReversed == before.cacheReversed);
    certified(*updated);
}

TEST_CASE("I update rechecks visibility lock definition deletion and owner at commit",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    const unsigned invalid = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u);
    struct Parent {
        RS_Entity* entity;
        RS_EntityContainer* original;
        bool restore = false;
        ~Parent() { if (restore) entity->setParent(original); }
    } parent{curve, curve->getParent()};
    RS_EntityContainer foreign;
    if (invalid == 0) curve->setVisible(false);
    if (invalid == 1) curve->getLayer()->lock(true);
    if (invalid == 2) curve->scale({0, 0}, {2, 3});
    if (invalid == 3) curve->getData().controlPoints.front().x += .5;
    if (invalid == 4) curve->setFlag(RS2::FlagDeleted);
    if (invalid == 5) { parent.restore = true; curve->setParent(&foreign); }
    if (invalid == 6) REQUIRE(f.graphic.removeEntity(curve));
    const auto fingerprint = invalid == 6 ? 0 : RS_Spline::geometryFingerprint(curve->getData());
    f.action->useSource(source);
    CHECK(f.action->lastReport().updated == 0);
    CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    const auto& outcome = f.action->lastReport().outcomes.front();
    CHECK_FALSE(outcome.refusal.isEmpty());
    CHECK_FALSE(outcome.edit.has_value());
    if (invalid != 6) {
        CHECK(curve->getFlag(RS2::FlagSelected));
        CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    }
    CHECK_FALSE(f.context.messages.isEmpty());
    noUndo(f.graphic);
}

TEST_CASE("I update source must be visible live directly owned and exactly an ellipse",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    RS_EntityContainer nested(nullptr, false);
    RS_Ellipse child(&nested, basis);
    nested.addEntity(&child);
    RS_Graphic other;
    other.initForNewDocument();
    auto* foreign = new RS_Ellipse(&other, basis);
    other.addEntity(foreign);
    RS_Line line(nullptr, {0, 0}, {1, 0});
    const unsigned invalid = GENERATE(0u, 1u, 2u, 3u, 4u, 5u);
    RS_Entity* picked = source;
    if (invalid == 0) picked = nullptr;
    if (invalid == 1) picked = &line;
    if (invalid == 2) picked = &child;
    if (invalid == 3) picked = foreign;
    if (invalid == 4) source->setVisible(false);
    if (invalid == 5) source->setFlag(RS2::FlagDeleted);
    f.action->useSource(picked);
    const auto& report = f.action->lastReport();
    CHECK(report.updated == 0);
    CHECK(report.refused == 1);
    CHECK_FALSE(report.sourceRefusal.isEmpty());
    REQUIRE(report.outcomes.size() == 1);
    CHECK_FALSE(report.outcomes.front().edit.has_value());
    CHECK(curve->isSelected());
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    noUndo(f.graphic);
}

TEST_CASE("I update refuses a changed or missing document view context before overlay or batch work",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    auto* source = f.ellipse(curve->definition().basis);
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    f.start({curve});
    RS_Graphic other;
    other.initForNewDocument();
    lc::test::TestGraphicView otherView;
    otherView.setDocument(&other);
    struct Restore {
        Fixture& fixture;
        ~Restore() {
            fixture.view.setDocument(&fixture.graphic);
            fixture.context.setDocumentAndView(&fixture.graphic, &fixture.view);
        }
    } restore{f};
    const unsigned change = GENERATE(0u, 1u, 2u, 3u);
    if (change == 0) f.context.setDocumentAndView(nullptr, nullptr);
    if (change == 1) f.context.setDocumentAndView(&other, &otherView);
    if (change == 2) f.context.setDocumentAndView(&f.graphic, &otherView);
    if (change == 3) f.view.setDocument(&other);
    f.action->useSource(source);
    CHECK(f.action->lastReport().updated == 0);
    CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK_FALSE(f.action->lastReport().outcomes.front().edit.has_value());
    CHECK(curve->getFlag(RS2::FlagSelected));
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    noUndo(f.graphic);
    noUndo(other);
}

TEST_CASE("I update reports actual native refusal with bounded work and retains the original",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    const auto id = curve->getId();
    auto basis = curve->definition().basis;
    basis.ratio = .8;
    auto* source = f.ellipse(basis);
    f.start({curve});
    const bool invalidBasis = GENERATE(false, true);
    if (invalidBasis) {
        auto bad = basis;
        bad.center.z = 1.;
        source = f.ellipse(bad);
    } else {
        f.action->m_limits.maxSamples = 1;
    }
    f.action->useSource(source);
    CHECK(f.action->lastReport().updated == 0);
    CHECK(f.action->lastReport().refused == 1);
    const auto& edit = onlyEdit(*f.action);
    CHECK_FALSE(edit);
    CHECK_FALSE(edit.changed);
    CHECK(edit.engineRuns <= 1);
    if (invalidBasis) {
        CHECK(edit.refusal == LC_OffsetCurveRefusal::InvalidInput);
        notRun(edit);
    } else {
        REQUIRE(edit.engineStatus);
        CHECK(*edit.engineStatus == LC_CurveOffsetStatus::LimitExceeded);
        CHECK(edit.engineRuns == 1);
    }
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK(f.action->lastReport().outcomes.front().refusal == LC_OffsetCurveUI::refusalText(edit));
    CHECK_FALSE(f.action->lastReport().outcomes.front().refusal.isEmpty());
    REQUIRE(f.live().size() == 1);
    CHECK(f.live().front() == curve);
    CHECK(curve->getId() == id);
    CHECK(curve->isSelected());
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    noUndo(f.graphic);
}

TEST_CASE("I update mixes changed no op refused and ordinary selections in one batch",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    KeepSelected keep(false);
    auto* changed = f.arc();
    auto* refused = f.arc(.25, true);
    auto* unchanged = f.arc(.25, false, .125);
    auto* line = f.add(new RS_Line(&f.graphic, {0, 4}, {1, 4}));
    const auto oldChanged = RS_Spline::geometryFingerprint(changed->getData());
    const auto oldRefused = RS_Spline::geometryFingerprint(refused->getData());
    const auto unchangedId = unchanged->getId();
    auto* source = f.ellipse(unchanged->definition().basis);
    f.start({changed, refused, unchanged, line});
    f.action->useSource(source);
    const auto& report = f.action->lastReport();
    REQUIRE(report.outcomes.size() == 3);
    CHECK(report.updated == 1);
    CHECK(report.unchanged == 1);
    CHECK(report.refused == 1);
    CHECK(refused->isSelected());
    CHECK(unchanged->isSelected());
    CHECK(line->isSelected());
    CHECK_FALSE(refused->isDeleted());
    CHECK(unchanged->getId() == unchangedId);
    CHECK(RS_Spline::geometryFingerprint(refused->getData()) == oldRefused);
    unsigned unselected = 0;
    for (auto* curve : f.live()) if (!curve->isSelected()) ++unselected;
    CHECK(unselected == 1);
    REQUIRE(f.graphic.undo());
    CHECK(RS_Spline::geometryFingerprint(changed->getData()) == oldChanged);
    CHECK_FALSE(changed->isDeleted());
    CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo());
    CHECK(f.live().size() == 3);
}

TEST_CASE("I update accepts direct entities inside the active block but no cross container source",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* block = new RS_Block(&f.graphic, RS_BlockData("I_UPDATE_BLOCK", {0, 0}, false));
    REQUIRE(f.graphic.addBlock(block));
    f.graphic.activateBlock("I_UPDATE_BLOCK");
    f.view.setDocument(block);
    f.context.setDocumentAndView(block, &f.view);
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    f.action->useSource(source);
    REQUIRE(f.action->lastReport().updated == 1);
    REQUIRE(f.live().size() == 1);
    CHECK(f.live().front()->getParent() == block);
    noUndo(f.graphic);
    REQUIRE(block->undo());
    REQUIRE(f.live().size() == 1);
    CHECK(f.live().front()->definition().basis.center == RS_Vector(0, 0));
}

TEST_CASE("I update stores no live source link after successful replacement",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    f.action->useSource(source);
    REQUIRE(f.action->lastReport().updated == 1);
    REQUIRE(f.graphic.removeEntity(source));
    REQUIRE(f.live().size() == 1);
    sameBasis(f.live().front()->definition().basis, basis);
    certified(*f.live().front());
    REQUIRE(f.graphic.undo());
    REQUIRE(f.live().size() == 1);
    CHECK(f.live().front()->definition().basis.center == RS_Vector(0, 0));
    REQUIRE(f.graphic.redo());
    REQUIRE(f.live().size() == 1);
    sameBasis(f.live().front()->definition().basis, basis);
}

TEST_CASE("I update excludes INSERT and dimension source children and native display chords",
          "[lc-offsetcurve][i-update][action]") {
    Fixture f;
    auto* curve = f.arc();
    const auto basis = curve->definition().basis;
    const bool insertCarrier = GENERATE(false, true);
    RS_EntityContainer* carrier = nullptr;
    if (insertCarrier) {
        auto* block = new RS_Block(&f.graphic, RS_BlockData("I_UPDATE_SOURCE_BLOCK", {0, 0}, false));
        block->addEntity(new RS_Ellipse(block, basis));
        REQUIRE(f.graphic.addBlock(block));
        carrier = f.add(new RS_Insert(&f.graphic,
            RS_InsertData("I_UPDATE_SOURCE_BLOCK", {0, 0}, {1, 1}, 0., 1, 1, {0, 0})));
    } else {
        const RS_DimensionData data({0, 6}, {5, 8}, RS_MTextData::VABottom, RS_MTextData::HACenter,
            RS_MTextData::Exact, 1., "<>", "Standard", 0., 0., true, nullptr, false, false);
        auto* dimension = new RS_DimLinear(&f.graphic, data, RS_DimLinearData({0, 0}, {1, 0}, 0., 0.));
        dimension->addEntity(new RS_Ellipse(dimension, basis));
        carrier = f.add(dimension);
    }
    RS_Entity* child = nullptr;
    for (auto* entity : *carrier) if (entity->rtti() == RS2::EntityEllipse) child = entity;
    REQUIRE(child);
    REQUIRE(child->getParent() == carrier);
    f.start({curve});
    CHECK_FALSE(f.action->isEntityAllowedToSelect(curve->entityAt(0)));
    f.action->useSource(child);
    CHECK(f.action->lastReport().updated == 0);
    CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK_FALSE(f.action->lastReport().outcomes.front().edit.has_value());
    CHECK(curve->isSelected());
    noUndo(f.graphic);
}


// Readiness controls use real native admission, real selection and actual action entry points.
TEST_CASE("I update admits bounded native display trees before any virtual child clone",
          "[lc-offsetcurve][i-update][action][readiness][admission]") {
    class CountedTree final : public RS_EntityContainer {
    public:
        CountedTree(RS_EntityContainer* parent, unsigned& calls) : RS_EntityContainer(parent), calls(calls) {}
        RS_Entity* clone() const override { ++calls; return RS_EntityContainer::clone(); }
        unsigned& calls;
    };
    class CountedLine final : public RS_Line {
    public:
        CountedLine(RS_EntityContainer* parent, RS_Vector a, RS_Vector b, unsigned& calls)
            : RS_Line(parent, a, b), calls(calls) {}
        RS_Entity* clone() const override { ++calls; return RS_Line::clone(); }
        unsigned& calls;
    };
    for (unsigned kind = 0; kind < 4; ++kind) {
        CAPTURE(kind);
        Fixture f;
        auto* curve = f.arc();
        auto basis = curve->definition().basis; basis.center = {4, 8};
        auto* source = f.ellipse(basis);
        f.start({curve});
        unsigned clones = 0;
        RS_EntityContainer foreign;
        RS_Entity* displaced = nullptr;
        const auto restoreParent = qScopeGuard([&] {
            if (displaced) displaced->setParent(curve);
            curve->setOwner(true);
        });
        if (kind == 0) curve->addEntity(new CountedTree(curve, clones));
        else {
            const auto a = curve->getStartpoint(), b = curve->getEndpoint();
            curve->clear();
            auto* line = new CountedLine(curve, a, b, clones);
            curve->addEntity(line);
            if (kind == 1) { displaced = line; line->setParent(&foreign); }
            else if (kind == 2) curve->setDrwExtData(std::vector<std::shared_ptr<DRW_Variant>>(
                LC_OffsetCurveEditSnapshot::MaximumSnapshotItems + 1,
                std::make_shared<DRW_Variant>(1070, std::int32_t{1})));
            else curve->setOwner(false);
        }
        REQUIRE(curve->isConsistent()); // The cache stamp alone does not admit its mutable display tree.
        if (kind != 3) REQUIRE_FALSE(LC_OffsetCurveEditSnapshot::capture(*curve));
        else REQUIRE_FALSE(curve->isOwner());
        const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
        const auto count = f.graphic.count();
        const auto flags = curve->getFlags();
        const auto tags = curve->getDrwExtData();
        f.graphic.setModified(false);
        f.action->useSource(source);
        CHECK(clones == 0); CHECK(f.action->lastReport().updated == 0);
        CHECK(f.action->lastReport().refused == 1);
        REQUIRE(f.action->lastReport().outcomes.size() == 1);
        CHECK_FALSE(f.action->lastReport().outcomes.front().edit);
        CHECK(f.action->lastReport().outcomes.front().refusal.contains(kind == 3 ? "own" : "admission"));
        CHECK(curve->getFlags() == flags); CHECK(curve->getDrwExtData() == tags);
        CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
        CHECK(f.graphic.count() == count); CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
    }
}

TEST_CASE("I update preserves a late source mutation made during a real child clone",
          "[lc-offsetcurve][i-update][action][readiness][snapshot]") {
    class MutatingLine final : public RS_Line {
    public:
        MutatingLine(LC_OffsetCurve& root, unsigned& calls)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()), root(root), calls(calls) {}
        RS_Entity* clone() const override {
            ++calls;
            root.setUserDefVar("I_REAL_LATE_EDIT", "preserved");
            return RS_Line::clone();
        }
        LC_OffsetCurve& root;
        unsigned& calls;
    };
    Fixture f;
    auto* curve = f.arc();
    auto basis = curve->definition().basis; basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    unsigned clones = 0;
    auto* line = new MutatingLine(*curve, clones); // Read endpoints BEFORE clearing the old display.
    curve->clear(); curve->addEntity(line);
    REQUIRE(LC_OffsetCurveEditSnapshot::capture(*curve));
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    f.graphic.setModified(false);
    f.action->useSource(source);
    CHECK(clones == 1); CHECK(curve->getUserDefVar("I_REAL_LATE_EDIT") == "preserved");
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    CHECK(curve->definition().basis.center == RS_Vector(0, 0));
    CHECK(f.action->lastReport().updated == 0); CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK_FALSE(f.action->lastReport().outcomes.front().edit); // Rejected BEFORE the native setter.
    CHECK(f.action->lastReport().outcomes.front().refusal.contains("changed"));
    CHECK_FALSE(curve->isDeleted()); CHECK(curve->isSelected()); noUndo(f.graphic);
}

TEST_CASE("I update refuses actual same-document view switches and closing views before batch work",
          "[lc-offsetcurve][i-update][action][readiness][view]") {
    const bool closing = GENERATE(false, true);
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis; basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({curve});
    auto snapshot = LC_OffsetCurveEditSnapshot::capture(*curve); REQUIRE(snapshot);
    lc::test::TestGraphicView second;
    second.setDocument(&f.graphic); // A coherent second view of the SAME document, not the old invalid fixture.
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    if (closing) f.view.beginClose();
    else f.context.setDocumentAndView(&f.graphic, &second);
    f.view.redrawMethod = second.redrawMethod = RS2::RedrawNone;
    f.action->useSource(source);
    CHECK(f.action->lastReport().updated == 0); CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK_FALSE(f.action->lastReport().outcomes.front().edit);
    CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(second.redrawMethod == RS2::RedrawNone);
    REQUIRE(snapshot->matches(*curve)); noUndo(f.graphic);
    if (!closing) {
        f.context.setDocumentAndView(&f.graphic, &f.view);
        f.action->useSource(source); // Retained target/source identities remain usable in their ORIGINAL context.
        CHECK(f.action->lastReport().updated == 1); CHECK(onlyEdit(*f.action).engineRuns == 0);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(curve->isDeleted());
    }
}

TEST_CASE("I update discards a removed saved context before base init unselection",
          "[lc-offsetcurve][i-update][action][readiness][context-init]") {
    Fixture f;
    auto* curve = f.arc();
    f.document().select(QList<RS_Entity*>{curve}, true);
    f.context.saveContextMenuActionContext(curve, {1.25, 0}, true);
    f.action = std::make_unique<Probe>(&f.context);
    REQUIRE(f.graphic.removeEntity(curve));
    f.action->init(0);
    CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
    CHECK_FALSE(f.context.getContextMenuActionClickPosition().valid);
    CHECK_FALSE(f.action->m_selectionComplete);
    CHECK(f.action->lastReport().outcomes.isEmpty()); noUndo(f.graphic);
}

TEST_CASE("I update forgets a destroyed document even when view and context retain its raw identity",
          "[lc-offsetcurve][i-update][action][readiness][document]") {
    Fixture f;
    auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
    auto* curve = f.arc(); curve->setLayer(document->findLayer("0"));
    auto basis = curve->definition().basis; basis.center = {4, 8};
    auto* source = f.ellipse(basis); source->setLayer(document->findLayer("0"));
    f.start({curve});
    const auto targetId = curve->getId();
    auto* const deadIdentity = document.get();
    document.reset();
    CHECK(f.view.getDocument() == deadIdentity); CHECK(f.context.getDocument() == deadIdentity);
    f.action->useSource(source); // This is a retained dangling source address, never dereferenced.
    CHECK(f.action->lastReport().updated == 0); CHECK(f.action->lastReport().refused == 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK(f.action->lastReport().outcomes.front().targetId == targetId);
    CHECK_FALSE(f.action->lastReport().outcomes.front().edit); CHECK(f.action->isFinished());
    noUndo(f.graphic);
}

TEST_CASE("I update prepares complete shared batch prefixes using only actual changed rows",
          "[lc-offsetcurve][i-update][action][publication][prefix]") {
    Fixture f;
    auto* first = f.ring();
    auto* second = f.ring();
    auto* unchanged = f.ring();
    unchanged->move({4, 8});
    auto* refused = f.ring();
    auto* source = f.ellipse(unchanged->definition().basis);
    auto* oldPrefix = f.add(new RS_Line(&f.graphic, {0, 20}, {1, 20}));
    auto addA = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 21}, RS_Vector{1, 21});
    auto addB = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 22}, RS_Vector{1, 22});
    QList<RS_Entity*> addPrefix, deletePrefix;
    addPrefix.reserve(64); deletePrefix.reserve(64);
    addPrefix.append(addA.get()); addPrefix.append(addB.get()); deletePrefix.append(oldPrefix);
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = addPrefix; batch.entitiesToDelete = deletePrefix;
    const auto addCount = addPrefix.size(), deleteCount = deletePrefix.size();
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > addCount) delete batch.entitiesToAdd.takeLast();
    });
    REQUIRE(batch.entitiesToAdd.isSharedWith(addPrefix));
    REQUIRE(batch.entitiesToDelete.isSharedWith(deletePrefix));
    f.start({first, second, unchanged, refused});
    refused->setOwner(false);
    const auto restoreOwner = qScopeGuard([&] { refused->setOwner(true); });
    auto firstState = LC_OffsetCurveEditSnapshot::capture(*first);
    auto secondState = LC_OffsetCurveEditSnapshot::capture(*second);
    auto unchangedState = LC_OffsetCurveEditSnapshot::capture(*unchanged);
    REQUIRE(firstState); REQUIRE(secondState); REQUIRE(unchangedState);
    QList<RS_Entity*> selection;
    f.graphic.collectSelected(selection);
    const auto count = f.graphic.count();
    f.graphic.setModified(false);
    f.action->preparationOnlyBatch = &batch;
    f.action->useSource(source);

    REQUIRE(f.action->preparationOnlyResult.has_value());
    REQUIRE(*f.action->preparationOnlyResult);
    const auto& report = f.action->lastReport();
    CHECK(report.updated == 2); CHECK(report.unchanged == 1); CHECK(report.refused == 1);
    REQUIRE(report.outcomes.size() == 4);
    for (qsizetype i = 0; i < 3; ++i) {
        REQUIRE(report.outcomes.at(i).edit);
        notRun(*report.outcomes.at(i).edit);
    }
    CHECK_FALSE(report.outcomes.back().edit);
    REQUIRE(batch.entitiesToAdd.size() == addCount + 2);
    REQUIRE(batch.entitiesToDelete.size() == deleteCount + 2);
    CHECK(batch.entitiesToAdd.at(0) == addA.get()); CHECK(batch.entitiesToAdd.at(1) == addB.get());
    CHECK(batch.entitiesToDelete.at(0) == oldPrefix);
    CHECK(batch.entitiesToDelete.at(deleteCount) == first);
    CHECK(batch.entitiesToDelete.at(deleteCount + 1) == second);
    CHECK_FALSE(batch.entitiesToAdd.isSharedWith(addPrefix));
    CHECK_FALSE(batch.entitiesToDelete.isSharedWith(deletePrefix));
    CHECK(batch.entitiesToAdd.isDetached()); CHECK(batch.entitiesToDelete.isDetached());
    CHECK(batch.entitiesToAdd.capacity() >= addCount + 2);
    CHECK(batch.entitiesToDelete.capacity() >= deleteCount + 2);
    CHECK((addPrefix == QList<RS_Entity*>{addA.get(), addB.get()}));
    CHECK(deletePrefix == QList<RS_Entity*>{oldPrefix});
    CHECK_FALSE(batch.setActiveLayer); CHECK_FALSE(batch.setActivePen);
    for (qsizetype i = addCount; i < batch.entitiesToAdd.size(); ++i) {
        const auto* candidate = dynamic_cast<const LC_OffsetCurve*>(batch.entitiesToAdd.at(i));
        REQUIRE(candidate);
        CHECK(candidate->getParent() == nullptr); // Private setter and handed-off cache stay detached.
        CHECK_FALSE(candidate->isSelected());
        CHECK(candidate->definition().basis.center == RS_Vector(4, 8));
        certified(*candidate);
    }
    CHECK(firstState->matches(*first)); CHECK(secondState->matches(*second));
    CHECK(unchangedState->matches(*unchanged));
    QList<RS_Entity*> afterSelection;
    f.graphic.collectSelected(afterSelection);
    CHECK(afterSelection == selection); CHECK(f.graphic.count() == count);
    CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("R03 final successful getter preserves late metadata instead of publishing an older clone",
          "[lc-offsetcurve][i-update][r03-final-closure]") {
    bool earlierPeer = false;
    SECTION("the final getter changes its own root") {}
    SECTION("the final getter changes an already checked peer") { earlierPeer = true; }
    struct Observation {
        bool fired = false;
        unsigned updatedAtMutation = 0;
    } observation;
    class LateSuccessfulLine final : public RS_Line {
    public:
        LateSuccessfulLine(LC_OffsetCurve& root, LC_OffsetCurve& victim,
                           const Probe& action, Observation& observation)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()),
              victim(victim), action(action), observation(observation) {}
        RS_Entity* clone() const override {
            auto* copy = RS_Line::clone();
            copied = true;
            readsSinceClone = 0;
            return copy;
        }
        RS_Vector getStartpoint() const override {
            // Read four is the final child comparison, after root metadata was read.
            if (copied && ++readsSinceClone == 4) {
                observation.fired = true;
                observation.updatedAtMutation = action.lastReport().updated;
                victim.setUserDefVar("R03_LATE_SUCCESSFUL_GETTER", "must survive");
            }
            return RS_Line::getStartpoint();
        }
        LC_OffsetCurve& victim;
        const Probe& action;
        Observation& observation;
        mutable bool copied = false;
        mutable unsigned readsSinceClone = 0;
    };
    Fixture f;
    auto* victim = f.arc();
    auto* later = earlierPeer ? f.arc(.5) : victim;
    auto basis = victim->definition().basis;
    basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    if (earlierPeer) f.start({victim, later});
    else f.start({victim});
    auto* line = new LateSuccessfulLine(*later, *victim, *f.action, observation);
    later->clear();
    later->addEntity(line);
    const auto originalFingerprint = RS_Spline::geometryFingerprint(victim->getData());
    f.graphic.setModified(false);
    f.action->useSource(source);

    REQUIRE(observation.fired);
    CHECK(observation.updatedAtMutation == (earlierPeer ? 2 : 1));
    CHECK(f.action->lastReport().updated == (earlierPeer ? 1 : 0));
    CHECK(f.action->lastReport().refused == 1);
    CHECK_FALSE(victim->isDeleted());
    CHECK(f.live().contains(victim));
    CHECK(victim->getUserDefVar("R03_LATE_SUCCESSFUL_GETTER") == "must survive");
    CHECK(RS_Spline::geometryFingerprint(victim->getData()) == originalFingerprint);
    unsigned liveCopiesOfExternalValue = 0;
    for (const auto* curve : f.live())
        if (curve->getUserDefVar("R03_LATE_SUCCESSFUL_GETTER") == "must survive")
            ++liveCopiesOfExternalValue;
    CHECK(liveCopiesOfExternalValue == 1);
    if (!earlierPeer) {
        CHECK_FALSE(f.graphic.isModified());
        noUndo(f.graphic);
    }
}

TEST_CASE("R03 capture rechecks storage bounds after display admission callbacks",
          "[lc-offsetcurve][i-update][r03-final-closure]") {
    class OversizingLine final : public RS_Line {
    public:
        explicit OversizingLine(LC_OffsetCurve& root)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()), root(root) {}
        RS_Vector getStartpoint() const override {
            if (++reads == 1)
                root.getData().controlPoints.resize(LC_OffsetCheckLimits{}.maxVertices + 1);
            return RS_Line::getStartpoint();
        }
        LC_OffsetCurve& root;
        mutable unsigned reads = 0;
    };
    Fixture f;
    auto* curve = f.arc();
    auto* line = new OversizingLine(*curve);
    curve->clear();
    curve->addEntity(line);
    const auto snapshot = LC_OffsetCurveEditSnapshot::capture(*curve);
    CHECK_FALSE(snapshot);
    CHECK(line->reads == 1); // No Data constructor/second display read after the cap became stale.
    CHECK(curve->getData().controlPoints.size() == LC_OffsetCheckLimits{}.maxVertices + 1);
    CHECK_FALSE(curve->isDeleted());
    noUndo(f.graphic);
}

TEST_CASE("I update retains caller prefixes when a later final snapshot read throws",
          "[lc-offsetcurve][i-update][action][publication][exception]") {
    struct SnapshotReadFailure final : std::runtime_error {
        SnapshotReadFailure() : std::runtime_error("I update final snapshot read") {}
    };
    class LateSnapshotLine final : public RS_Line {
    public:
        LateSnapshotLine(LC_OffsetCurve& root, const Probe& action)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()), action(action) {}
        RS_Entity* clone() const override {
            auto* copy = RS_Line::clone();
            copied = true; readsSinceClone = 0;
            return copy; // Genuine line copy; no engine/checker result is replaced.
        }
        RS_Vector getStartpoint() const override {
            // The immediate post-clone matches reads twice: bounded length, then child equality.
            // The next read is the FINAL revalidation, after both real setters have completed.
            if (copied && ++readsSinceClone == 3) {
                updatedAtThrow = action.lastReport().updated;
                throw SnapshotReadFailure();
            }
            return RS_Line::getStartpoint();
        }
        const Probe& action;
        mutable bool copied = false;
        mutable unsigned readsSinceClone = 0, updatedAtThrow = 0;
    };
    Fixture f;
    auto* first = f.arc();
    auto* later = f.arc(.5);
    auto basis = first->definition().basis; basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    f.start({first, later});
    auto* throwing = new LateSnapshotLine(*later, *f.action);
    later->clear(); later->addEntity(throwing);
    later->setUserDefVar("I_SNAPSHOT_ALLOCATION", "retained");
    auto firstState = LC_OffsetCurveEditSnapshot::capture(*first);
    auto laterState = LC_OffsetCurveEditSnapshot::capture(*later);
    REQUIRE(firstState); REQUIRE(laterState);
    auto prefixAdd = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 20}, RS_Vector{1, 20});
    auto* prefixDelete = f.add(new RS_Line(&f.graphic, {0, 21}, {1, 21}));
    QList<RS_Entity*> addPrefix, deletePrefix;
    addPrefix.reserve(64); deletePrefix.reserve(64);
    addPrefix.append(prefixAdd.get()); deletePrefix.append(prefixDelete);
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = addPrefix; batch.entitiesToDelete = deletePrefix;
    const auto addCount = addPrefix.size();
    const auto cleanup = qScopeGuard([&] {
        // Also owns a suffix from a rejected old implementation; do not leak a failing test's prefix.
        while (batch.entitiesToAdd.size() > addCount) delete batch.entitiesToAdd.takeLast();
    });
    const auto* addStorage = std::as_const(batch.entitiesToAdd).constData();
    const auto* deleteStorage = std::as_const(batch.entitiesToDelete).constData();
    const auto count = f.graphic.count();
    f.graphic.setModified(false);
    f.action->preparationOnlyBatch = &batch;
    bool caught = false;
    try { f.action->useSource(source); }
    catch (const SnapshotReadFailure& error) {
        caught = true;
        CHECK(QString::fromLatin1(error.what()) == "I update final snapshot read");
    }
    REQUIRE(caught);
    CHECK(throwing->updatedAtThrow == 2); CHECK(throwing->readsSinceClone == 3);
    CHECK_FALSE(f.action->preparationOnlyResult.has_value());
    REQUIRE(f.action->lastReport().outcomes.size() == 2);
    for (const auto& outcome : f.action->lastReport().outcomes) {
        REQUIRE(outcome.edit); regenerated(*outcome.edit);
    }
    CHECK(batch.entitiesToAdd == addPrefix); CHECK(batch.entitiesToDelete == deletePrefix);
    CHECK(std::as_const(batch.entitiesToAdd).constData() == addStorage);
    CHECK(std::as_const(batch.entitiesToDelete).constData() == deleteStorage);
    CHECK(batch.entitiesToAdd.isSharedWith(addPrefix));
    CHECK(batch.entitiesToDelete.isSharedWith(deletePrefix));
    CHECK(firstState->matches(*first)); CHECK(laterState->matches(*later));
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
    CHECK(first->isSelected()); CHECK(later->isSelected());
    CHECK(f.graphic.count() == count); CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("I update filters late-invalidated staging before preparing complete batch prefixes",
          "[lc-offsetcurve][i-update][action][publication][late-refusal]") {
    class InvalidatingLine final : public RS_Line {
    public:
        InvalidatingLine(LC_OffsetCurve& root, LC_OffsetCurve& earlier, unsigned& calls)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()), earlier(earlier), calls(calls) {}
        RS_Entity* clone() const override {
            ++calls;
            earlier.setUserDefVar("I_LATE_STAGED_MUTATION", "must survive");
            return RS_Line::clone();
        }
        LC_OffsetCurve& earlier;
        unsigned& calls;
    };
    Fixture f;
    auto* first = f.arc();
    auto* second = f.arc(.5);
    auto basis = first->definition().basis; basis.center = {4, 8};
    auto* source = f.ellipse(basis);
    unsigned copies = 0;
    auto* line = new InvalidatingLine(*second, *first, copies);
    second->clear(); second->addEntity(line);
    const auto firstFingerprint = RS_Spline::geometryFingerprint(first->getData());
    const auto secondFingerprint = RS_Spline::geometryFingerprint(second->getData());
    auto prefixAdd = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 20}, RS_Vector{1, 20});
    auto* prefixDelete = f.add(new RS_Line(&f.graphic, {0, 21}, {1, 21}));
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd.append(prefixAdd.get()); batch.entitiesToDelete.append(prefixDelete);
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > 1) delete batch.entitiesToAdd.takeLast();
    });
    f.start({first, second}); f.graphic.setModified(false);
    f.action->preparationOnlyBatch = &batch;
    f.action->useSource(source);
    REQUIRE(f.action->preparationOnlyResult); REQUIRE(*f.action->preparationOnlyResult);
    CHECK(copies == 1);
    const auto& report = f.action->lastReport();
    CHECK(report.updated == 1); CHECK(report.refused == 1); CHECK(report.unchanged == 0);
    REQUIRE(report.outcomes.size() == 2);
    CHECK(report.outcomes.front().refusal.contains("changed"));
    REQUIRE(report.outcomes.front().edit); CHECK(report.outcomes.front().edit->changed);
    REQUIRE(report.outcomes.back().edit); REQUIRE(*report.outcomes.back().edit);
    regenerated(*report.outcomes.front().edit); regenerated(*report.outcomes.back().edit);
    REQUIRE(batch.entitiesToAdd.size() == 2); REQUIRE(batch.entitiesToDelete.size() == 2);
    CHECK(batch.entitiesToAdd.front() == prefixAdd.get()); CHECK(batch.entitiesToDelete.front() == prefixDelete);
    CHECK(batch.entitiesToDelete.back() == second);
    const auto* candidate = dynamic_cast<const LC_OffsetCurve*>(batch.entitiesToAdd.back());
    REQUIRE(candidate); CHECK(candidate->getParent() == nullptr); certified(*candidate);
    CHECK(first->getUserDefVar("I_LATE_STAGED_MUTATION") == "must survive");
    CHECK(RS_Spline::geometryFingerprint(first->getData()) == firstFingerprint);
    CHECK(RS_Spline::geometryFingerprint(second->getData()) == secondFingerprint);
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(second->isDeleted());
    CHECK(first->isSelected()); CHECK(second->isSelected());
    CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}
