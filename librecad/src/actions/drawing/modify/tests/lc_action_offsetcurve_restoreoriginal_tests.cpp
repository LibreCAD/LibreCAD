/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <QSettings>
#include <QStringList>

#include "lc_action_offsetcurve_restoreoriginal.h"
#include "lc_action_offsetcurve_updatesource.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurveactiontestsupport.h"
#include "lc_offsetcurveeditsnapshot.h"
#include <QScopeGuard>
#include "lc_offsetcurve.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;
constexpr const char* tags = "[lc-offsetcurve][i-restore][action]";
using EntityId = decltype(std::declval<const RS_Entity&>().getId());
static_assert(std::is_same_v<decltype(LC_ActionOffsetCurveRestoreOriginal::Outcome::targetId), EntityId>,
    "Restore outcome IDs must preserve the entity ID type.");

class Graphic final : public RS_Graphic {
public:
    using RS_Undo::countUndoCycles;
};
class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override { messages.append(message); }
};
class Probe final : public LC_ActionOffsetCurveRestoreOriginal {
public:
    explicit Probe(LC_ActionContext* context) : LC_ActionOffsetCurveRestoreOriginal(context) {}
    using LC_ActionOffsetCurveRestoreOriginal::isEntityAllowedToSelect;
    using LC_ActionOffsetCurveRestoreOriginal::m_comparisonLimit;
    bool defer = false;
    void trigger() override { if (!defer) LC_ActionOffsetCurveRestoreOriginal::trigger(); }
    void commit() { LC_ActionOffsetCurveRestoreOriginal::trigger(); }
    bool prepare(LC_DocumentModificationBatch& batch) {
        return LC_ActionOffsetCurveRestoreOriginal::doTriggerModifications(batch);
    }
};

// Real finite display line; the one-shot read callback exercises source changes
// during the production bounded snapshot, never substitutes geometry or a C result.
class ReadCallbackLine final : public RS_Line {
public:
    explicit ReadCallbackLine(LC_OffsetCurve& root)
        : RS_Line(&root, root.getStartpoint(), root.getEndpoint()) {}
    RS_Vector getStartpoint() const override {
        if (onRead) {
            auto callback = std::move(onRead);
            callback();
        }
        return RS_Line::getStartpoint();
    }
    mutable std::function<void()> onRead;
};

class TypeCallbackLine final : public RS_Line {
public:
    using RS_Line::RS_Line;
    RS2::EntityType rtti() const override {
        if (onType) {
            auto callback = std::move(onType);
            callback();
        }
        return RS2::EntityLine;
    }
    mutable std::function<void()> onType;
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
    Graphic graphic;
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
        return add(new RS_Ellipse(nullptr, basis));
    }
    LC_OffsetCurve* arc(double distance = .25, double end = .1) {
        LC_EllipseOffsetDefinition definition;
        definition.basis = {{0, 0}, {1, 0}, 1., 0., end, false};
        definition.distance = distance;
        definition.relTolerance = .01;
        const double radius = 1. + distance;
        RS_Line cache(nullptr, {radius, 0}, {radius*std::cos(end), radius*std::sin(end)});
        auto curve = LC_OffsetCurve::adopt(cache, definition, .01);
        REQUIRE(curve);
        REQUIRE(curve->hasEditableDefinition());
        return add(curve.release());
    }
    LC_OffsetCurve* ring(const RS_Vector& center = {0, 0}) {
        RS_SplineData data{3, false};
        std::array<RS_Vector, 4> net{{{6, 0}, {6, 27./8}, {27./8, 6}, {0, 6}}};
        for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
            for (unsigned i = quadrant ? 1 : 0; i < 4; ++i)
                data.controlPoints.push_back(net[i] + center);
            for (auto& point : net) point = {-point.y, point.x};
        }
        for (unsigned i = 0; i <= 4; ++i)
            for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
        data.weights.assign(data.controlPoints.size(), 1.);
        data.m_closedFlag = true;
        RS_Spline cache(nullptr, data);
        LC_EllipseOffsetDefinition definition;
        definition.basis = {center, {5, 0}, 1.};
        definition.distance = 1.;
        definition.relTolerance = .01;
        auto curve = LC_OffsetCurve::adopt(cache, definition, .06);
        REQUIRE(curve);
        REQUIRE(curve->hasEditableDefinition());
        return add(curve.release());
    }
    LC_OffsetCurve* generated(const RS_EllipseData& basis) {
        RS_Ellipse source(nullptr, basis);
        auto result = LC_OffsetCurve::create(source, .25, false, {}, .01);
        CAPTURE(result.refusal);
        REQUIRE(result);
        REQUIRE(result.entity->hasEditableDefinition());
        return add(result.entity.release());
    }
    void start(std::initializer_list<RS_Entity*> targets, bool defer = false) {
        for (auto* target : targets)
            if (!target->getFlag(RS2::FlagSelected)) document().select(QList<RS_Entity*>{target}, true);
        action = std::make_unique<Probe>(&context);
        action->defer = defer;
        action->init(0);
    }
    QList<RS_Ellipse*> liveEllipses() {
        QList<RS_Ellipse*> result;
        for (auto* entity : document())
            if (entity && !entity->isDeleted() && entity->getParent() == &document() &&
                entity->rtti() == RS2::EntityEllipse)
                result.append(static_cast<RS_Ellipse*>(entity));
        return result;
    }
};

void noUndo(RS_Document& document) {
    bool undo = true, redo = true;
    document.collectUndoState(undo, redo);
    CHECK_FALSE(undo);
    CHECK_FALSE(redo);
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
struct NativeSnapshot {
    const LC_OffsetCurve& curve;
    const EntityId id = curve.getId();
    const std::uint64_t fingerprint = RS_Spline::geometryFingerprint(curve.getData());
    const LC_EllipseOffsetDefinition definition = curve.definition();
    RS_Layer* const layer = curve.getLayer(false);
    const RS_Pen pen = curve.getPen(false);
    const unsigned flags = curve.unknownOwnedFlags();
    const std::size_t tailSize = curve.unknownOwnedTail().size();
    const RS_Vector* const controls = curve.getData().controlPoints.data();
    const int children = curve.count();
    explicit NativeSnapshot(const LC_OffsetCurve& value) : curve(value) {}
    void check() const {
        CHECK(curve.getId() == id);
        CHECK_FALSE(curve.isDeleted());
        CHECK(curve.isSelected());
        CHECK(curve.hasEditableDefinition());
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == fingerprint);
        sameBasis(curve.definition().basis, definition.basis);
        CHECK(curve.definition().distance == definition.distance);
        CHECK(curve.definition().towardCentre == definition.towardCentre);
        CHECK(curve.definition().cacheReversed == definition.cacheReversed);
        CHECK(curve.definition().relTolerance == definition.relTolerance);
        CHECK(curve.getLayer(false) == layer);
        CHECK(curve.getPen(false) == pen);
        CHECK(curve.unknownOwnedFlags() == flags);
        CHECK(curve.unknownOwnedTail().size() == tailSize);
        CHECK(curve.getData().controlPoints.data() == controls);
        CHECK(curve.count() == children);
    }
};
void counts(const Probe& action, unsigned restored, unsigned present, unsigned refused) {
    const auto& report = action.lastReport();
    CHECK(report.restored == restored);
    CHECK(report.alreadyPresent == present);
    CHECK(report.refused == refused);
    CHECK(report.outcomes.size() == restored + present + refused);
    for (const auto& outcome : report.outcomes) {
        CHECK(unsigned(outcome.restored) + unsigned(outcome.alreadyPresent) +
              unsigned(!outcome.refusal.isEmpty()) == 1);
    }
}
}

TEST_CASE("I restore inserts a fresh embedded ellipse and only that insertion is undoable", tags) {
    Fixture f;
    KeepSelected keep(true);
    auto* curve = f.arc();
    curve->revertDirection();
    REQUIRE(curve->hasEditableDefinition());
    REQUIRE(curve->setOwnedExtensions(0x10, {{1000, QStringLiteral("retained native extension")}}));
    curve->setSourceHandle(123);
    const auto basis = curve->definition().basis;
    NativeSnapshot before(*curve);
    f.start({curve});
    CHECK(f.action->rtti() == RS2::ActionOffsetCurveRestoreOriginal);
    counts(*f.action, 1, 0, 0);
    auto live = f.liveEllipses();
    REQUIRE(live.size() == 1);
    auto* restored = live.front();
    sameBasis(restored->getData(), basis);
    CHECK(restored->getParent() == &f.graphic);
    CHECK(restored->sourceHandle() == 0);
    CHECK(restored->ownerLineage() == 0);
    CHECK(restored->isSelected());
    CHECK(curve->sourceHandle() == 123);
    before.check();
    CHECK(f.graphic.countUndoCycles() == 1);

    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    before.check();
    REQUIRE(f.graphic.redo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == restored);
    sameBasis(restored->getData(), basis);
    before.check();
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("I restore uses effective attributes and makes no live relationship", tags) {
    Fixture f;
    KeepSelected keep(false);
    auto* layer = new RS_Layer("I_RESTORE_ATTRIBUTES");
    layer->setPen(RS_Pen(RS_Color(17, 31, 47), RS2::Width11, RS2::DashLine));
    f.graphic.addLayer(layer);
    auto* curve = f.arc();
    curve->setLayer(layer);
    curve->setPen(RS_Pen());
    const auto effective = curve->getPen(true);
    const auto basis = curve->definition().basis;
    f.graphic.activateLayer("0");
    f.graphic.setActivePen(RS_Pen(RS_Color(255, 0, 0), RS2::Width07, RS2::SolidLine));
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    auto* restored = f.liveEllipses().front();
    CHECK(restored->getLayer(false) == layer);
    CHECK(restored->getPen(false) == effective);
    CHECK_FALSE(restored->isSelected());
    CHECK(curve->isSelected());
    auto shifted = basis;
    shifted.center = {4, 8};
    REQUIRE(curve->setBasis(shifted));
    sameBasis(restored->getData(), basis);
    REQUIRE(f.graphic.removeEntity(curve));
    sameBasis(restored->getData(), basis);
    REQUIRE(f.graphic.undo());
    CHECK(restored->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK_FALSE(restored->isDeleted());
    sameBasis(restored->getData(), basis);
}

TEST_CASE("I restore second run is a graph and undo no op", tags) {
    Fixture f;
    auto* curve = f.ring();
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    auto* existing = f.liveEllipses().front();
    const auto id = existing->getId();
    const auto count = f.graphic.count();
    const auto cycles = f.graphic.countUndoCycles();
    const auto basis = existing->getData();
    NativeSnapshot before(*curve);
    f.graphic.setModified(false);
    f.start({curve});
    counts(*f.action, 0, 1, 0);
    CHECK(f.graphic.count() == count);
    CHECK(f.graphic.countUndoCycles() == cycles);
    CHECK_FALSE(f.graphic.isModified());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    CHECK(existing->getId() == id);
    sameBasis(existing->getData(), basis);
    before.check();
    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("I restore shared embedded sources deduplicate staged inserts in one undo batch", tags) {
    Fixture f;
    auto* first = f.arc(.25);
    auto* second = f.arc(.5);
    auto* third = f.ring({16, 32});
    NativeSnapshot a(*first), b(*second), c(*third);
    f.start({first, second, third});
    counts(*f.action, 2, 1, 0);
    REQUIRE(f.liveEllipses().size() == 2);
    a.check(); b.check(); c.check();
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    a.check(); b.check(); c.check();
    CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo());
    CHECK(f.liveEllipses().size() == 2);
}

TEST_CASE("I restore counts a readable live equivalent on another layer as already present", tags) {
    Fixture f;
    auto* curve = f.ring();
    auto* existing = f.ellipse(curve->definition().basis);
    auto* layer = new RS_Layer("I_RESTORE_OTHER_LAYER");
    f.graphic.addLayer(layer);
    existing->setLayer(layer);
    // The source is read-only dedup evidence, not a modification target.
    const unsigned state = GENERATE(0u, 1u, 2u);
    if (state == 1) layer->lock(true);
    if (state == 2) layer->freeze(true);
    const auto rawPen = existing->getPen(false);
    f.graphic.setModified(false);
    f.start({curve});
    counts(*f.action, 0, 1, 0);
    CHECK(existing->getLayer(false) == layer);
    CHECK(existing->getPen(false) == rawPen);
    CHECK_FALSE(existing->isDeleted());
    CHECK(curve->isSelected());
    CHECK_FALSE(f.graphic.isModified());
    noUndo(f.graphic);
}

TEST_CASE("I restore whole exact circles have no unique major axis", tags) {
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    const unsigned form = GENERATE(0u, 1u, 2u, 3u);
    if (form == 0) basis.majorP = {-5, 0};
    if (form == 1) basis.majorP = {0, 5};
    if (form == 2) basis.majorP = {3, 4};
    if (form == 3) { basis.majorP = {-3, 4}; basis.angle2 = 2*std::acos(-1.); basis.reversed = true; }
    auto* existing = f.ellipse(basis);
    f.start({curve});
    counts(*f.action, 0, 1, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    noUndo(f.graphic);
}

TEST_CASE("I restore whole ellipses recognize antiparallel and exchanged axes", tags) {
    Fixture f;
    auto* curve = f.generated({{0, 0}, {4, 0}, .5});
    auto basis = curve->definition().basis;
    const bool exchangeAxes = GENERATE(false, true);
    if (exchangeAxes) { basis.majorP = {0, 2}; basis.ratio = 2.; }
    else basis.majorP = {-4, 0};
    auto* existing = f.ellipse(basis);
    f.start({curve});
    counts(*f.action, 0, 1, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    noUndo(f.graphic);
}

TEST_CASE("I restore arcs compare local endpoints and midpoint including reversed forms", tags) {
    Fixture f;
    const unsigned form = GENERATE(0u, 1u, 2u, 3u, 4u, 5u);
    CAPTURE(form);
    const double pi = std::acos(-1.);
    auto* curve = form < 2 || form >= 4 ? f.arc() : f.generated({{0, 0}, {4, 0}, .5, 0., .1, false});
    auto basis = curve->definition().basis;
    if (form == 0 || form == 2) {
        std::swap(basis.angle1, basis.angle2);
        basis.reversed = !basis.reversed;
    } else if (form == 1 || form == 4) {
        basis.majorP = {0, 1};
        basis.angle1 = 1.5*pi;
        basis.angle2 = 1.5*pi + .1;
        if (form == 4) {
            std::swap(basis.angle1, basis.angle2);
            basis.reversed = true;
        }
    } else if (form == 5) {
        basis.majorP = {0, -1};
        basis.angle1 = .5*pi;
        basis.angle2 = .5*pi + .1;
    } else {
        basis.majorP = {-4, 0};
        basis.angle1 += pi;
        basis.angle2 += pi;
    }
    auto* existing = f.ellipse(basis);
    f.start({curve});
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    INFO(f.action->lastReport().outcomes.front().refusal.toStdString());
    counts(*f.action, 0, 1, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    noUndo(f.graphic);
}

TEST_CASE("I restore recognizes real C admitted wrapped circular sources in rotated charts", tags) {
    const bool reverseSource = GENERATE(false, true);
    const int quarterTurns = GENERATE(-1, 1, 2);
    const bool oppositeTraversal = GENERATE(false, true);
    CAPTURE(reverseSource, quarterTurns, oppositeTraversal);
    Fixture f;
    RS_EllipseData source{{0, 0}, {1, 0}, 1., 6.2, .02, false};
    if (reverseSource) {
        std::swap(source.angle1, source.angle2);
        source.reversed = true;
    }
    // The actual producer and its C gate must admit this source; no native
    // definition or validation result is changed after creation.
    auto* curve = f.generated(source);
    const auto& nativeBasis = curve->definition().basis;
    REQUIRE(nativeBasis.angle1 == source.angle1);
    REQUIRE(nativeBasis.angle2 == source.angle2);
    REQUIRE(nativeBasis.reversed == reverseSource);
    REQUIRE(nativeBasis.ratio == 1.);
    REQUIRE(nativeBasis.majorP == RS_Vector(1, 0));
    const auto domain = geo::sweep(nativeBasis);
    REQUIRE(domain); REQUIRE_FALSE(domain->whole);
    if (reverseSource) {
        REQUIRE(nativeBasis.angle2 > nativeBasis.angle1); // actual negative sweep winding
        REQUIRE(domain->signedSweep < 0);
    } else {
        REQUIRE(nativeBasis.angle2 < nativeBasis.angle1); // actual positive sweep winding
        REQUIRE(domain->signedSweep > 0);
    }
    const auto checked = geo::checkCache(curve->getData(), curve->definition(), curve->checkedWorldTolerance());
    CAPTURE(checked.reason);
    REQUIRE(checked);
    NativeSnapshot before(*curve);

    const double pi = std::acos(-1.), turn = 2*pi;
    const auto rotatedAngle = [quarterTurns, pi, turn](double angle) {
        angle -= quarterTurns*(.5*pi);
        if (angle < 0) angle += turn;
        else if (angle >= turn) angle -= turn;
        return angle;
    };
    auto ordinary = nativeBasis;
    if (quarterTurns == 1) ordinary.majorP = {0, 1};
    else if (quarterTurns == -1) ordinary.majorP = {0, -1};
    else ordinary.majorP = {-1, 0};
    ordinary.angle1 = rotatedAngle(nativeBasis.angle1);
    ordinary.angle2 = rotatedAngle(nativeBasis.angle2);
    if (oppositeTraversal) {
        std::swap(ordinary.angle1, ordinary.angle2);
        ordinary.reversed = !ordinary.reversed;
    }
    // Only the source crosses zero in its own chart. The candidate's shifted
    // chart and optional opposite traversal exercise both endpoint pairings.
    const bool ordinaryWinds = (!ordinary.reversed && ordinary.angle2 < ordinary.angle1) ||
        (ordinary.reversed && ordinary.angle2 > ordinary.angle1);
    REQUIRE_FALSE(ordinaryWinds);
    auto* existing = f.ellipse(ordinary);
    const auto existingId = existing->getId();
    const auto count = f.graphic.count();
    f.start({curve});
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    INFO(f.action->lastReport().outcomes.front().refusal.toStdString());
    counts(*f.action, 0, 1, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    CHECK(existing->getId() == existingId);
    CHECK(existing->getData().majorP == ordinary.majorP);
    CHECK(existing->getAngle1() == ordinary.angle1);
    CHECK(existing->getAngle2() == ordinary.angle2);
    CHECK(existing->isReversed() == ordinary.reversed);
    CHECK_FALSE(existing->isDeleted());
    CHECK(f.graphic.count() == count);
    before.check(); noUndo(f.graphic);
}

TEST_CASE("I restore does not merge complementary circular arcs", tags) {
    Fixture f;
    const double pi = std::acos(-1.);
    auto* curve = f.generated({{0, 0}, {1, 0}, 1., 0., pi, false});
    auto basis = curve->definition().basis;
    basis.reversed = true;
    auto* complementary = f.ellipse(basis);
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    CHECK(f.liveEllipses().size() == 2);
    CHECK_FALSE(complementary->isDeleted());
    REQUIRE(f.graphic.undo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == complementary);
}

TEST_CASE("I restore never merges near circles nearly full arcs or uncertain radii", tags) {
    Fixture f;
    auto* curve = f.ring();
    auto basis = curve->definition().basis;
    const unsigned distinction = GENERATE(0u, 1u, 2u);
    if (distinction == 0) basis.ratio = std::nextafter(1., 0.);
    if (distinction == 1) basis.angle2 = std::nextafter(2*std::acos(-1.), 0.);
    if (distinction == 2) basis.majorP = {std::nextafter(3., 4.), 4.};
    auto* different = f.ellipse(basis);
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    CHECK(f.liveEllipses().size() == 2);
    CHECK_FALSE(different->isDeleted());
    REQUIRE(f.graphic.undo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == different);
}

TEST_CASE("I restore exact full turn guards reject rounded gap and overshoot differences", tags) {
    Fixture f;
    const unsigned form = GENERATE(0u, 1u, 2u, 3u);
    const double turn = 2*std::acos(-1.);
    const double shift = 0x1p-52; // nonzero exact dyadic, smaller than half a turn ULP
    auto* curve = f.ring();
    NativeSnapshot before(*curve);
    auto basis = curve->definition().basis;
    basis.angle1 = (form == 0 || form == 3) ? shift : -shift;
    basis.angle2 = form < 2 ? turn : -turn;
    basis.reversed = form >= 2;
    // This is the OLD acceptance condition, not the geometry oracle. Exact
    // subtraction of the stored doubles differs from +/-turn by +/-shift.
    REQUIRE(std::abs(basis.angle2 - basis.angle1) == turn);
    REQUIRE(basis.angle1 != 0);
    auto* distinct = f.ellipse(basis);
    REQUIRE(distinct->getAngle1() == basis.angle1);
    REQUIRE(distinct->getAngle2() == basis.angle2);
    distinct->setUserDefVar("restore-near-full", "keep");
    distinct->setSourceHandle(727);
    const auto count = f.graphic.count();
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    CHECK(f.graphic.count() == count + 1);
    CHECK(f.liveEllipses().size() == 2);
    CHECK_FALSE(distinct->isDeleted());
    CHECK(distinct->getAngle1() == basis.angle1); CHECK(distinct->getAngle2() == basis.angle2);
    CHECK(distinct->isReversed() == basis.reversed);
    CHECK(distinct->getUserDefVar("restore-near-full") == "keep"); CHECK(distinct->sourceHandle() == 727);
    auto* restored = f.liveEllipses().back(); REQUIRE(restored != distinct);
    sameBasis(restored->getData(), curve->definition().basis);
    CHECK(restored->getLayer(false) == curve->getLayer(true)); CHECK(restored->getPen(false) == curve->getPen(true));
    CHECK(restored->sourceHandle() == 0); CHECK(restored->xDictHandle() == 0); CHECK(restored->reactorHandles().empty());
    before.check();
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == distinct);
    CHECK(restored->isDeleted()); before.check();
    CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
    CHECK_FALSE(restored->isDeleted()); CHECK_FALSE(distinct->isDeleted()); before.check();
}

TEST_CASE("I restore explicitly refuses a numeric equality boundary instead of inserting or deduplicating", tags) {
    Fixture f;
    const bool staged = GENERATE(false, true);
    auto* first = f.arc();
    NativeSnapshot before(*first);
    auto nearby = first->definition().basis;
    const double turn = 2*std::acos(-1.);
    const double allowance = 16*(std::nextafter(turn, std::numeric_limits<double>::infinity()) - turn);
    nearby.angle2 += allowance;
    // Establish that real outward extent arithmetic straddles the decision
    // boundary. The expected refusal is not derived from equivalent() labels.
    const auto a = geo::ReferenceFrame::create(first->definition().basis, 0.);
    const auto b = geo::ReferenceFrame::create(nearby, 0.);
    REQUIRE(a); REQUIRE(b);
    const auto delta = b->traversalRate()*LC_Interval::point(b->parameterEnd()) -
        a->traversalRate()*LC_Interval::point(a->parameterEnd());
    REQUIRE(delta.lo() < allowance); REQUIRE(delta.hi() > allowance);
    if (!staged) {
        auto* existing = f.ellipse(nearby);
        existing->setUserDefVar("restore-uncertain", "untouched");
        existing->setSourceHandle(728);
        const auto count = f.graphic.count();
        f.graphic.setModified(false);
        f.start({first});
        counts(*f.action, 0, 0, 1);
        REQUIRE(f.action->lastReport().outcomes.size() == 1);
        CHECK(f.action->lastReport().outcomes.front().refusal.contains("numerically uncertain"));
        CHECK(f.graphic.count() == count); CHECK_FALSE(f.graphic.isModified());
        REQUIRE(f.liveEllipses().size() == 1); CHECK(f.liveEllipses().front() == existing);
        CHECK(existing->getAngle2() == nearby.angle2);
        CHECK(existing->getUserDefVar("restore-uncertain") == "untouched"); CHECK(existing->sourceHandle() == 728);
        before.check(); noUndo(f.graphic);
    } else {
        auto* second = f.arc(.5, nearby.angle2); // genuine independent C admission, no definition mutation
        NativeSnapshot secondBefore(*second);
        const auto count = f.graphic.count();
        f.start({first, second});
        counts(*f.action, 1, 0, 1);
        REQUIRE(f.action->lastReport().outcomes.size() == 2);
        CHECK(f.action->lastReport().outcomes.back().refusal.contains("numerically uncertain"));
        CHECK(f.graphic.count() == count + 1); REQUIRE(f.liveEllipses().size() == 1);
        auto* restored = f.liveEllipses().front(); sameBasis(restored->getData(), first->definition().basis);
        before.check(); secondBefore.check(); CHECK(f.graphic.countUndoCycles() == 1);
        REQUIRE(f.graphic.undo()); CHECK(f.liveEllipses().isEmpty()); before.check(); secondBefore.check();
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        REQUIRE(f.liveEllipses().size() == 1); CHECK(f.liveEllipses().front() == restored);
        before.check(); secondBefore.check();
    }
}

TEST_CASE("I restore local identity never consumes large world center neighbors", tags) {
    Fixture f;
    const double large = std::ldexp(1., 40);
    auto* curve = f.ring({large, -large});
    auto basis = curve->definition().basis;
    basis.center.x = std::nextafter(basis.center.x, std::numeric_limits<double>::infinity());
    auto* neighbor = f.ellipse(basis);
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    CHECK(f.liveEllipses().size() == 2);
    CHECK_FALSE(neighbor->isDeleted());
    REQUIRE(f.graphic.undo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == neighbor);
}

TEST_CASE("I restore ignores deleted foreign block and INSERT child originals", tags) {
    Fixture f;
    auto* curve = f.arc();
    const auto basis = curve->definition().basis;
    const unsigned location = GENERATE(0u, 1u, 2u);
    if (location == 0) {
        auto* deleted = f.ellipse(basis);
        deleted->setFlag(RS2::FlagDeleted);
    } else {
        auto* block = new RS_Block(&f.graphic, RS_BlockData("I_RESTORE_FOREIGN", {0, 0}, false));
        block->addEntity(new RS_Ellipse(block, basis));
        REQUIRE(f.graphic.addBlock(block));
        if (location == 2) {
            auto* insert = f.add(new RS_Insert(&f.graphic,
                RS_InsertData("I_RESTORE_FOREIGN", {0, 0}, {1, 1}, 0., 1, 1, {0, 0})));
            bool hasChild = false;
            for (auto* child : *insert)
                if (child->rtti() == RS2::EntityEllipse) hasChild = true;
            REQUIRE(hasChild);
        }
    }
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front()->getParent() == &f.graphic);
    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK(curve->isSelected());
    CHECK_FALSE(curve->isDeleted());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("I restore confines dedup insertion and undo to the editable block", tags) {
    Fixture f;
    auto* block = new RS_Block(&f.graphic, RS_BlockData("I_RESTORE_EDIT_BLOCK", {0, 0}, false));
    REQUIRE(f.graphic.addBlock(block));
    f.graphic.activateBlock("I_RESTORE_EDIT_BLOCK");
    f.view.setDocument(block);
    f.context.setDocumentAndView(block, &f.view);
    auto* curve = f.arc();
    auto* modelOriginal = new RS_Ellipse(&f.graphic, curve->definition().basis);
    f.graphic.addEntity(modelOriginal);
    f.start({curve});
    counts(*f.action, 1, 0, 0);
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front()->getParent() == block);
    noUndo(f.graphic);
    REQUIRE(block->undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK_FALSE(modelOriginal->isDeleted());
    CHECK_FALSE(curve->isDeleted());
    CHECK_FALSE(block->undo());
    REQUIRE(block->redo());
    REQUIRE(f.liveEllipses().size() == 1);
    f.start({curve});
    counts(*f.action, 0, 1, 0);
    REQUIRE(block->undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK_FALSE(block->undo());
}

TEST_CASE("I restore revalidates stale hidden locked undefined inconsistent and removed targets", tags) {
    Fixture f;
    auto* curve = f.arc();
    f.start({curve}, true);
    const auto id = curve->getId();
    const unsigned refusal = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u);
    RS_EntityContainer foreign;
    struct Parent {
        RS_Entity* entity;
        RS_EntityContainer* original;
        bool restore = false;
        ~Parent() { if (restore) entity->setParent(original); }
    } parent{curve, curve->getParent()};
    if (refusal == 0) curve->setVisible(false);
    if (refusal == 1) curve->getLayer()->lock(true);
    if (refusal == 2) curve->scale({0, 0}, {2, 3});
    if (refusal == 3) curve->getData().controlPoints.front().x += .5;
    if (refusal == 4) curve->setFlag(RS2::FlagDeleted);
    if (refusal == 5) { parent.restore = true; curve->setParent(&foreign); }
    if (refusal == 6) REQUIRE(f.graphic.removeEntity(curve)); // frees the retained address
    const auto count = f.graphic.count();
    const auto fingerprint = refusal == 6 ? 0 : RS_Spline::geometryFingerprint(curve->getData());
    f.action->commit();
    counts(*f.action, 0, 0, 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    const auto& outcome = f.action->lastReport().outcomes.front();
    CHECK(outcome.targetId == id);
    const QString expected = refusal == 0 ? "hidden" : refusal == 1 ? "locked" :
        refusal == 2 ? "definition" : refusal == 3 ? "cache" : "live";
    CHECK(outcome.refusal.contains(expected, Qt::CaseInsensitive));
    CHECK(f.liveEllipses().isEmpty());
    CHECK(f.graphic.count() == count);
    if (refusal != 6) {
        CHECK(curve->getFlag(RS2::FlagSelected));
        CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    }
    CHECK_FALSE(f.context.messages.isEmpty());
    noUndo(f.graphic);
}

TEST_CASE("I restore rejects a changed document view or locked editable container", tags) {
    Fixture f;
    auto* curve = f.arc();
    f.start({curve}, true);
    RS_Graphic other;
    other.initForNewDocument();
    lc::test::TestGraphicView otherView;
    otherView.setDocument(&other);
    struct Restore {
        Fixture& f;
        ~Restore() {
            f.graphic.setVisible(true);
            f.graphic.setLayer(nullptr);
            f.graphic.findLayer("0")->lock(false);
            f.view.setDocument(&f.graphic);
            f.context.setDocumentAndView(&f.graphic, &f.view);
        }
    } restore{f};
    const unsigned invalid = GENERATE(0u, 1u, 2u, 3u, 4u, 5u);
    if (invalid == 0) f.context.setDocumentAndView(nullptr, nullptr);
    if (invalid == 1) f.context.setDocumentAndView(&other, &otherView);
    if (invalid == 2) f.context.setDocumentAndView(&f.graphic, &otherView);
    if (invalid == 3) f.view.setDocument(&other);
    if (invalid == 4) f.graphic.setVisible(false);
    if (invalid == 5) {
        f.graphic.setLayer(f.graphic.findLayer("0"));
        f.graphic.findLayer("0")->lock(true);
    }
    const auto count = f.graphic.count();
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    f.action->commit();
    counts(*f.action, 0, 0, 1);
    CHECK(f.action->lastReport().outcomes.front().refusal.contains("document", Qt::CaseInsensitive));
    CHECK(curve->getFlag(RS2::FlagSelected));
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    CHECK(f.graphic.count() == count);
    noUndo(f.graphic);
    noUndo(other);
}

TEST_CASE("I restore wrong type context and ordinary selections never become targets", tags) {
    Fixture f;
    auto* curve = f.arc();
    auto* ordinary = f.add(new RS_Line(nullptr, {0, 3}, {1, 3}));
    f.context.saveContextMenuActionContext(ordinary, {0, 3}, false);
    f.start({curve, ordinary});
    counts(*f.action, 1, 0, 0);
    CHECK(ordinary->isSelected());
    CHECK_FALSE(ordinary->isDeleted());
    CHECK_FALSE(f.action->isEntityAllowedToSelect(ordinary));
    CHECK_FALSE(f.action->isEntityAllowedToSelect(curve->entityAt(0)));
    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK(ordinary->isSelected());

    f.document().select(QList<RS_Entity*>{curve}, false);
    f.context.saveContextMenuActionContext(ordinary, {0, 3}, false);
    f.start({ordinary});
    counts(*f.action, 0, 0, 0);
    CHECK(f.liveEllipses().isEmpty());
    CHECK(ordinary->isSelected());
    CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo()); // no-op did not discard the previous redo insertion
    CHECK(f.liveEllipses().size() == 1);
}

TEST_CASE("I restore context activation selects a native explicitly without toggling it", tags) {
    Fixture f;
    auto* curve = f.arc();
    const unsigned state = GENERATE(0u, 1u, 2u);
    const bool selected = state != 0, hidden = state == 2;
    if (selected) f.document().select(QList<RS_Entity*>{curve}, true);
    if (hidden) curve->setVisible(false);
    f.context.saveContextMenuActionContext(curve, {1.25, 0}, false);
    f.start({});
    counts(*f.action, hidden ? 0 : 1, 0, hidden ? 1 : 0);
    CHECK(curve->getFlag(RS2::FlagSelected));
    QList<RS_Entity*> selection;
    f.graphic.collectSelected(selection);
    CHECK(selection.count(curve) == 1);
    CHECK_FALSE(curve->isDeleted());
    CHECK(f.liveEllipses().size() == (hidden ? 0 : 1));
    if (hidden) noUndo(f.graphic);
    else {
        REQUIRE(f.graphic.undo());
        CHECK(curve->isSelected()); CHECK_FALSE(curve->isDeleted());
    }
}

TEST_CASE("I restore mixes accepted already present and refused targets without deselection", tags) {
    Fixture f;
    KeepSelected keep(false);
    auto* accepted = f.arc();
    auto* already = f.ring({16, 32});
    auto* refused = f.ring({64, 128});
    auto* ordinary = f.add(new RS_Line(nullptr, {0, 3}, {1, 3}));
    auto* existing = f.ellipse(already->definition().basis);
    auto* locked = new RS_Layer("I_RESTORE_LOCKED_TARGET");
    f.graphic.addLayer(locked);
    refused->setLayer(locked);
    const auto count = f.graphic.count();
    f.start({accepted, already, refused, ordinary}, true);
    REQUIRE(refused->isSelected());
    REQUIRE(f.graphic.count() == count);
    REQUIRE(f.graphic.countUndoCycles() == 0);
    // The selected target becomes locked after capture, before publication.
    locked->lock(true);
    f.action->commit();
    counts(*f.action, 1, 1, 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 3);
    bool lockedRefusal = false;
    for (const auto& outcome : f.action->lastReport().outcomes) {
        if (outcome.targetId == refused->getId())
            lockedRefusal = outcome.refusal.contains("locked", Qt::CaseInsensitive);
    }
    CHECK(lockedRefusal);
    CHECK(f.graphic.count() == count + 1);
    CHECK(accepted->isSelected());
    CHECK(already->isSelected());
    CHECK(refused->getFlag(RS2::FlagSelected));
    CHECK(ordinary->isSelected());
    CHECK_FALSE(accepted->isDeleted());
    CHECK_FALSE(already->isDeleted());
    CHECK_FALSE(refused->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    REQUIRE(f.liveEllipses().size() == 1);
    CHECK(f.liveEllipses().front() == existing);
    CHECK_FALSE(refused->isDeleted());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("I restore bounded duplicate refusal neither inserts uncertain geometry nor loses accepted work", tags) {
    Fixture f;
    auto* first = f.arc();
    auto* second = f.ring({16, 32});
    f.start({first, second}, true);
    const unsigned passes = GENERATE(0u, 1u, 2u);
    const bool allowFirst = passes == 2;
    // The callback prepass costs one direct-member pass before any decision.
    // The OLD one-pass allowance now refuses honestly; two admit only the first
    // complete raw search, without increasing the production comparison cap.
    f.action->m_comparisonLimit = passes*f.graphic.count();
    f.action->commit();
    counts(*f.action, allowFirst ? 1 : 0, 0, allowFirst ? 1 : 2);
    CHECK(first->isSelected());
    CHECK(second->isSelected());
    CHECK_FALSE(first->isDeleted());
    CHECK_FALSE(second->isDeleted());
    CHECK(f.action->lastReport().outcomes.back().refusal.contains("limit", Qt::CaseInsensitive));
    CHECK(f.liveEllipses().size() == (allowFirst ? 1 : 0));
    if (allowFirst) {
        CHECK(f.graphic.countUndoCycles() == 1);
        REQUIRE(f.graphic.undo());
        CHECK(f.liveEllipses().isEmpty());
        CHECK_FALSE(f.graphic.undo());
    } else noUndo(f.graphic);
}

TEST_CASE("I restore prepares exact shared caller prefixes with add-only ownership handoff",
          "[lc-offsetcurve][i-restore][action][publication][prefix]") {
    Fixture f;
    auto* first = f.arc();
    auto* duplicate = f.arc(.5);
    auto* second = f.ring({16, 32});
    auto* present = f.ring({32, 64});
    auto* refused = f.ring({64, 128});
    auto* existing = f.ellipse(present->definition().basis);
    auto* oldPrefix = f.add(new RS_Line(nullptr, {0, 20}, {1, 20}));
    auto addA = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 21}, RS_Vector{1, 21});
    auto addB = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 22}, RS_Vector{1, 22});
    QList<RS_Entity*> addPrefix, deletePrefix;
    addPrefix.reserve(64); deletePrefix.reserve(64);
    addPrefix.append(addA.get()); addPrefix.append(addB.get()); deletePrefix.append(oldPrefix);
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = addPrefix; batch.entitiesToDelete = deletePrefix;
    const auto addCount = addPrefix.size();
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > addCount) delete batch.entitiesToAdd.takeLast();
    });
    const auto* deleteStorage = std::as_const(batch.entitiesToDelete).constData();
    REQUIRE(batch.entitiesToAdd.isSharedWith(addPrefix));
    REQUIRE(batch.entitiesToDelete.isSharedWith(deletePrefix));
    f.start({first, duplicate, second, present, refused}, true);
    refused->setVisible(false);
    auto firstState = LC_OffsetCurveEditSnapshot::capture(*first);
    auto secondState = LC_OffsetCurveEditSnapshot::capture(*second);
    REQUIRE(firstState); REQUIRE(secondState);
    QList<RS_Entity*> selection;
    f.graphic.collectSelected(selection);
    const auto count = f.graphic.count(); f.graphic.setModified(false);
    REQUIRE(f.action->prepare(batch));
    counts(*f.action, 2, 2, 1);
    REQUIRE(batch.entitiesToAdd.size() == addCount + 2);
    CHECK(batch.entitiesToAdd.at(0) == addA.get()); CHECK(batch.entitiesToAdd.at(1) == addB.get());
    CHECK(batch.entitiesToDelete == deletePrefix);
    CHECK(std::as_const(batch.entitiesToDelete).constData() == deleteStorage);
    CHECK(batch.entitiesToDelete.isSharedWith(deletePrefix));
    CHECK_FALSE(batch.entitiesToAdd.isSharedWith(addPrefix)); CHECK(batch.entitiesToAdd.isDetached());
    CHECK(batch.entitiesToAdd.capacity() >= addCount + 2);
    CHECK((addPrefix == QList<RS_Entity*>{addA.get(), addB.get()}));
    CHECK_FALSE(batch.setActiveLayer); CHECK_FALSE(batch.setActivePen);
    for (qsizetype i = addCount; i < batch.entitiesToAdd.size(); ++i) {
        const auto* candidate = dynamic_cast<const RS_Ellipse*>(batch.entitiesToAdd.at(i));
        REQUIRE(candidate); CHECK(candidate->rtti() == RS2::EntityEllipse);
        CHECK(candidate->getParent() == nullptr); CHECK_FALSE(candidate->isSelected());
        CHECK(candidate->sourceHandle() == 0); CHECK(candidate->ownerLineage() == 0);
        CHECK(candidate->xDictHandle() == 0); CHECK(candidate->reactorHandles().empty());
        const auto* source = i == addCount ? first : second;
        sameBasis(candidate->getData(), source->definition().basis);
        CHECK(candidate->getLayer(false) == source->getLayer(true));
        CHECK(candidate->getPen(false) == source->getPen(true));
    }
    CHECK(firstState->matches(*first)); CHECK(secondState->matches(*second));
    QList<RS_Entity*> afterSelection; f.graphic.collectSelected(afterSelection);
    CHECK(afterSelection == selection); CHECK(f.graphic.count() == count);
    CHECK(f.liveEllipses() == QList<RS_Ellipse*>{existing});
    CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("I restore honors caller additions and retirements while preserving every prefix owner",
          "[lc-offsetcurve][i-restore][action][publication][no-op]") {
    Fixture f;
    auto* curve = f.arc();
    const bool inPrefix = GENERATE(false, true);
    auto* existing = inPrefix ? nullptr : f.ellipse(curve->definition().basis);
    std::unique_ptr<RS_Entity> addition;
    if (inPrefix) addition = std::make_unique<RS_Ellipse>(nullptr, curve->definition().basis);
    else addition = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 20}, RS_Vector{1, 20});
    RS_Entity* oldPrefix = existing ? static_cast<RS_Entity*>(existing) :
        f.add(new RS_Line(nullptr, {0, 21}, {1, 21}));
    QList<RS_Entity*> addPrefix, deletePrefix;
    addPrefix.reserve(64); deletePrefix.reserve(64);
    addPrefix.append(addition.get()); deletePrefix.append(oldPrefix);
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = addPrefix; batch.entitiesToDelete = deletePrefix;
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > addPrefix.size()) delete batch.entitiesToAdd.takeLast();
    });
    const auto* addStorage = std::as_const(batch.entitiesToAdd).constData();
    const auto* deleteStorage = std::as_const(batch.entitiesToDelete).constData();
    f.start({curve}, true); f.graphic.setModified(false);
    auto state = LC_OffsetCurveEditSnapshot::capture(*curve); REQUIRE(state);
    CHECK(f.action->prepare(batch) == !inPrefix);
    counts(*f.action, inPrefix ? 0 : 1, inPrefix ? 1 : 0, 0);
    REQUIRE(batch.entitiesToAdd.size() == addPrefix.size() + (inPrefix ? 0 : 1));
    CHECK(std::as_const(batch.entitiesToAdd).front() == addition.get()); CHECK(batch.entitiesToDelete == deletePrefix);
    if (inPrefix) {
        CHECK(batch.entitiesToAdd == addPrefix);
        CHECK(std::as_const(batch.entitiesToAdd).constData() == addStorage);
        CHECK(batch.entitiesToAdd.isSharedWith(addPrefix));
    } else {
        const auto* restored = dynamic_cast<const RS_Ellipse*>(batch.entitiesToAdd.back());
        REQUIRE(restored); sameBasis(restored->getData(), curve->definition().basis);
        CHECK(restored != existing); CHECK(restored->getParent() == nullptr);
        CHECK_FALSE(existing->isDeleted()); // Preparation does not perform the caller's retirement.
    }
    CHECK(std::as_const(batch.entitiesToDelete).constData() == deleteStorage);
    CHECK(batch.entitiesToDelete.isSharedWith(deletePrefix));
    CHECK(state->matches(*curve)); CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("I restore retains all caller prefixes when a later final snapshot getter throws",
          "[lc-offsetcurve][i-restore][action][publication][exception]") {
    struct SnapshotReadFailure final : std::runtime_error {
        SnapshotReadFailure() : std::runtime_error("I restore final snapshot read") {}
    };
    class ThrowingLine final : public RS_Line {
    public:
        ThrowingLine(LC_OffsetCurve& root, const Probe& action)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()), action(action) {}
        RS_Vector getStartpoint() const override {
            // Capture reads once in cachedLength, once in the child snapshot.
            // Read three belongs to final matches, after both source captures.
            if (armed && ++reads == 3) {
                armed = false; outcomesAtThrow = action.lastReport().outcomes.size();
                throw SnapshotReadFailure();
            }
            return RS_Line::getStartpoint();
        }
        const Probe& action;
        mutable bool armed = false;
        mutable unsigned reads = 0;
        mutable qsizetype outcomesAtThrow = 0;
    };
    Fixture f;
    auto* first = f.arc(); auto* later = f.arc(.5);
    f.start({first, later}, true);
    auto* line = new ThrowingLine(*later, *f.action);
    later->clear(); later->addEntity(line);
    auto firstState = LC_OffsetCurveEditSnapshot::capture(*first);
    auto laterState = LC_OffsetCurveEditSnapshot::capture(*later);
    REQUIRE(firstState); REQUIRE(laterState);
    auto addition = std::make_unique<RS_Line>(nullptr, RS_Vector{0, 20}, RS_Vector{1, 20});
    auto* oldPrefix = f.add(new RS_Line(nullptr, {0, 21}, {1, 21}));
    QList<RS_Entity*> addPrefix, deletePrefix;
    addPrefix.reserve(64); deletePrefix.reserve(64);
    addPrefix.append(addition.get()); deletePrefix.append(oldPrefix);
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = addPrefix; batch.entitiesToDelete = deletePrefix;
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > addPrefix.size()) delete batch.entitiesToAdd.takeLast();
    });
    const auto* addStorage = std::as_const(batch.entitiesToAdd).constData();
    const auto* deleteStorage = std::as_const(batch.entitiesToDelete).constData();
    const auto count = f.graphic.count(); f.graphic.setModified(false);
    line->armed = true;
    bool caught = false;
    try { (void)f.action->prepare(batch); }
    catch (const SnapshotReadFailure& error) {
        caught = true; CHECK(QString::fromLatin1(error.what()) == "I restore final snapshot read");
    }
    REQUIRE(caught); CHECK(line->reads == 3); CHECK(line->outcomesAtThrow == 2);
    CHECK(batch.entitiesToAdd == addPrefix); CHECK(batch.entitiesToDelete == deletePrefix);
    CHECK(std::as_const(batch.entitiesToAdd).constData() == addStorage);
    CHECK(std::as_const(batch.entitiesToDelete).constData() == deleteStorage);
    CHECK(batch.entitiesToAdd.isSharedWith(addPrefix)); CHECK(batch.entitiesToDelete.isSharedWith(deletePrefix));
    CHECK(firstState->matches(*first)); CHECK(laterState->matches(*later));
    CHECK(first->isSelected()); CHECK(later->isSelected());
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
    CHECK(f.graphic.count() == count); CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("I restore refuses a late changed source without suppressing its still-live duplicate target",
          "[lc-offsetcurve][i-restore][action][publication][late-refusal]") {
    Fixture f; KeepSelected keep(false);
    auto* first = f.arc(); auto* later = f.arc(.5);
    const auto firstCache = RS_Spline::geometryFingerprint(first->getData());
    const auto laterCache = RS_Spline::geometryFingerprint(later->getData());
    const bool hidden = GENERATE(false, true);
    auto* line = new ReadCallbackLine(*later);
    later->clear(); later->addEntity(line);
    f.start({first, later}, true);
    unsigned reads = 0;
    line->onRead = [&] {
        ++reads;
        if (hidden) first->setVisible(false);
        else first->setUserDefVar("I_RESTORE_LATE_VALUE", "must survive");
    };
    const auto count = f.graphic.count(); f.graphic.setModified(false);
    f.action->commit();
    CHECK(reads == 1); counts(*f.action, 1, 0, 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 2);
    CHECK(f.action->lastReport().outcomes.front().refusal.contains("changed"));
    CHECK(f.action->lastReport().outcomes.back().restored);
    REQUIRE(f.liveEllipses().size() == 1);
    auto* restored = f.liveEllipses().front();
    sameBasis(restored->getData(), later->definition().basis);
    CHECK(restored->getPen(false) == later->getPen(true)); CHECK(restored->getLayer(false) == later->getLayer(true));
    CHECK_FALSE(restored->isSelected()); CHECK(restored->sourceHandle() == 0); CHECK(restored->ownerLineage() == 0);
    CHECK(RS_Spline::geometryFingerprint(first->getData()) == firstCache);
    CHECK(RS_Spline::geometryFingerprint(later->getData()) == laterCache);
    CHECK(first->getFlag(RS2::FlagSelected)); CHECK(later->isSelected());
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
    CHECK(f.graphic.count() == count + 1); CHECK(f.graphic.countUndoCycles() == 1);
    if (hidden) CHECK_FALSE(first->isVisible());
    else CHECK(first->getUserDefVar("I_RESTORE_LATE_VALUE") == "must survive");
    REQUIRE(f.graphic.undo()); CHECK(restored->isDeleted());
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted()); CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo()); CHECK_FALSE(restored->isDeleted());
}

TEST_CASE("I restore decides live document duplicates after every source snapshot has been captured",
          "[lc-offsetcurve][i-restore][action][publication][late-duplicate]") {
    Fixture f; KeepSelected keep(false);
    auto* first = f.arc(); auto* later = f.arc(.5);
    auto shifted = later->definition().basis; shifted.center = {16, 32};
    REQUIRE(later->setBasis(shifted)); // Real native setter; no assumption about its engine count.
    auto* existing = f.ellipse(first->definition().basis);
    existing->setUserDefVar("ordinary-original", "retained"); existing->setSourceHandle(727);
    auto* line = new ReadCallbackLine(*later);
    later->clear(); later->addEntity(line);
    f.start({first, later}, true);
    const bool erased = GENERATE(false, true);
    bool removed = false; unsigned reads = 0;
    line->onRead = [&] {
        ++reads;
        if (erased) { removed = f.graphic.removeEntity(existing); existing = nullptr; }
        else existing->move({4, 8});
    };
    f.action->commit();
    CHECK(reads == 1); if (erased) REQUIRE(removed);
    counts(*f.action, 2, 0, 0);
    CHECK(f.liveEllipses().size() == (erased ? 2 : 3));
    unsigned firstOriginals = 0, laterOriginals = 0;
    for (const auto* ellipse : f.liveEllipses()) {
        if (ellipse == existing) continue;
        if (ellipse->getData().center == first->definition().basis.center) {
            ++firstOriginals; sameBasis(ellipse->getData(), first->definition().basis);
        }
        if (ellipse->getData().center == later->definition().basis.center) {
            ++laterOriginals; sameBasis(ellipse->getData(), later->definition().basis);
        }
        CHECK(ellipse->sourceHandle() == 0); CHECK(ellipse->ownerLineage() == 0);
    }
    CHECK(firstOriginals == 1); CHECK(laterOriginals == 1);
    if (existing) {
        CHECK(existing->getData().center == RS_Vector(4, 8));
        CHECK(existing->getUserDefVar("ordinary-original") == "retained"); CHECK(existing->sourceHandle() == 727);
    }
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 1); REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().size() == (erased ? 0 : 1)); CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo()); CHECK(f.liveEllipses().size() == (erased ? 2 : 3));
}

TEST_CASE("R03 Restore decides ordinary witnesses after all candidate type callbacks",
          "[lc-offsetcurve][i-restore][action][publication][r03-final-closure]") {
    const bool callerPrefix = GENERATE(false, true);
    const bool mutate = GENERATE(false, true);
    CAPTURE(callerPrefix, mutate);
    Fixture f;
    auto* first = f.arc();
    auto* later = f.ring({16, 32});
    auto witnessOwner = std::make_unique<RS_Ellipse>(nullptr, first->definition().basis);
    auto callbackOwner = std::make_unique<TypeCallbackLine>(nullptr, RS_Vector{30, 30}, RS_Vector{31, 30});
    auto* witness = witnessOwner.get();
    auto* callback = callbackOwner.get();
    witness->setLayer(f.graphic.findLayer("0"));
    witness->setUserDefVar("R03_ORDINARY_WITNESS", "external evidence");
    witness->setSourceHandle(727);
    QList<RS_Entity*> prefix;
    if (callerPrefix) {
        prefix.append(witness); prefix.append(callback);
    } else {
        f.add(witnessOwner.release()); f.add(callbackOwner.release());
    }
    f.start({first, later}, true);
    NativeSnapshot a(*first), b(*later);
    unsigned fired = 0;
    callback->onType = [&] {
        ++fired;
        if (mutate) witness->move({4, 8});
    };
    LC_DocumentModificationBatch batch;
    batch.entitiesToAdd = prefix;
    const auto cleanup = qScopeGuard([&] {
        while (batch.entitiesToAdd.size() > prefix.size()) delete batch.entitiesToAdd.takeLast();
    });
    const auto count = f.graphic.count();
    f.graphic.setModified(false);
    REQUIRE(f.action->prepare(batch)); // Actual production preparation, including its genuine snapshots.
    REQUIRE(fired == 1);
    counts(*f.action, mutate ? 2 : 1, mutate ? 0 : 1, 0);
    REQUIRE(batch.entitiesToAdd.size() == prefix.size() + (mutate ? 2 : 1));
    for (qsizetype i = 0; i < prefix.size(); ++i) CHECK(batch.entitiesToAdd.at(i) == prefix.at(i));
    unsigned firstOriginals = 0, laterOriginals = 0;
    for (qsizetype i = prefix.size(); i < batch.entitiesToAdd.size(); ++i) {
        const auto* restored = dynamic_cast<const RS_Ellipse*>(batch.entitiesToAdd.at(i));
        REQUIRE(restored);
        CHECK(restored->getParent() == nullptr); CHECK_FALSE(restored->isSelected());
        CHECK(restored->sourceHandle() == 0); CHECK(restored->ownerLineage() == 0);
        if (restored->getData().center == first->definition().basis.center) {
            ++firstOriginals; sameBasis(restored->getData(), first->definition().basis);
        } else {
            ++laterOriginals; sameBasis(restored->getData(), later->definition().basis);
        }
    }
    CHECK(firstOriginals == (mutate ? 1 : 0)); CHECK(laterOriginals == 1);
    CHECK(witness->getData().center == (mutate ? RS_Vector(4, 8) : RS_Vector(0, 0)));
    CHECK(witness->getUserDefVar("R03_ORDINARY_WITNESS") == "external evidence");
    CHECK(witness->sourceHandle() == 727); CHECK_FALSE(witness->isDeleted());
    a.check(); b.check();
    CHECK(batch.entitiesToDelete.isEmpty()); CHECK(f.graphic.count() == count);
    CHECK_FALSE(f.graphic.isModified()); noUndo(f.graphic);
}

TEST_CASE("R03 Restore refuses late effective layer and inherited pen changes without publishing stale attributes",
          "[lc-offsetcurve][i-restore][action][publication][r03-final-closure]") {
    const bool inherited = GENERATE(false, true);
    const bool mutate = GENERATE(false, true);
    CAPTURE(inherited, mutate);
    Fixture f; KeepSelected keep(false);
    auto* layer = new RS_Layer("R03_RESTORE_LATE_PEN");
    const RS_Pen oldPen(RS_Color(17, 31, 47), RS2::Width11, RS2::DashLine);
    const RS_Pen newPen(RS_Color(71, 53, 29), RS2::Width07, RS2::SolidLine);
    layer->setPen(oldPen); f.graphic.addLayer(layer);
    RS_Block* block = nullptr;
    if (inherited) {
        block = new RS_Block(&f.graphic, RS_BlockData("R03_RESTORE_PEN_BLOCK", {0, 0}, false));
        REQUIRE(f.graphic.addBlock(block));
        block->setLayer(layer); block->setPen(oldPen);
        f.graphic.activateBlock("R03_RESTORE_PEN_BLOCK");
        f.view.setDocument(block); f.context.setDocumentAndView(block, &f.view);
    }
    auto* curve = f.arc();
    curve->setLayer(layer);
    if (inherited)
        curve->setPen(RS_Pen(RS_Color(RS2::FlagByBlock), RS2::WidthByBlock, RS2::LineByBlock));
    else curve->setPen(RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer));
    const auto rawPen = curve->getPen(false);
    const auto effectiveBefore = curve->getPen(true);
    REQUIRE(effectiveBefore == oldPen);
    auto* callback = f.add(new TypeCallbackLine(nullptr, RS_Vector{30, 30}, RS_Vector{31, 30}));
    f.start({curve}, true);
    NativeSnapshot before(*curve);
    unsigned fired = 0;
    callback->onType = [&] {
        ++fired;
        if (mutate) {
            if (inherited) block->setPen(newPen);
            else layer->setPen(newPen);
        }
    };
    const auto count = f.document().count();
    f.action->commit(); // Public action and real undo publisher; no fake checker results.
    REQUIRE(fired == 1);
    before.check(); CHECK(curve->getPen(false) == rawPen);
    CHECK(curve->getPen(true) == (mutate ? newPen : oldPen));
    counts(*f.action, mutate ? 0 : 1, 0, mutate ? 1 : 0);
    CHECK(f.document().count() == count + (mutate ? 0 : 1));
    if (mutate) {
        REQUIRE(f.action->lastReport().outcomes.size() == 1);
        CHECK(f.action->lastReport().outcomes.front().refusal.contains("attributes"));
        CHECK(f.liveEllipses().isEmpty()); noUndo(f.document());
    } else {
        REQUIRE(f.liveEllipses().size() == 1);
        auto* restored = f.liveEllipses().front();
        CHECK(restored->getPen(false) == effectiveBefore); CHECK(restored->getLayer(false) == layer);
        sameBasis(restored->getData(), curve->definition().basis);
        REQUIRE(f.document().undo()); CHECK(restored->isDeleted()); before.check();
        CHECK_FALSE(f.document().undo());
        REQUIRE(f.document().redo()); CHECK_FALSE(restored->isDeleted()); before.check();
    }
}

TEST_CASE("I restore KeepSelected fixture updates the public settings cache across alternating actions",
          "[lc-offsetcurve][i-restore][action][settings]") {
    const bool initial = LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true);
    for (bool keep : {!initial, initial, !initial, initial}) {
        Fixture f; KeepSelected choice(keep);
        CHECK(LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true) == keep);
        auto* curve = f.arc(); f.start({curve});
        counts(*f.action, 1, 0, 0); REQUIRE(f.liveEllipses().size() == 1);
        CHECK(f.liveEllipses().front()->isSelected() == keep); CHECK(curve->isSelected());
        REQUIRE(f.graphic.undo()); CHECK_FALSE(curve->isDeleted());
    }
    CHECK(LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true) == initial);
}


// Restore reads embedded values only: it must never resurrect targets through stale action context.
TEST_CASE("I restore refuses actual same-document view switches and closing views without insertion",
          "[lc-offsetcurve][i-restore][action][readiness][view]") {
    const bool closing = GENERATE(false, true);
    Fixture f;
    auto* curve = f.arc(); f.start({curve}, true);
    auto snapshot = LC_OffsetCurveEditSnapshot::capture(*curve); REQUIRE(snapshot);
    lc::test::TestGraphicView second;
    second.setDocument(&f.graphic);
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    if (closing) f.view.beginClose();
    else f.context.setDocumentAndView(&f.graphic, &second);
    f.view.redrawMethod = second.redrawMethod = RS2::RedrawNone;
    f.action->commit();
    counts(*f.action, 0, 0, 1);
    CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(second.redrawMethod == RS2::RedrawNone);
    REQUIRE(snapshot->matches(*curve)); noUndo(f.graphic);
    CHECK(f.liveEllipses().isEmpty());
    if (!closing) {
        f.context.setDocumentAndView(&f.graphic, &f.view); f.action->commit();
        counts(*f.action, 1, 0, 0);
        REQUIRE(f.graphic.undo()); CHECK(f.liveEllipses().isEmpty());
    }
}

TEST_CASE("I restore discards a removed saved context before base init unselection",
          "[lc-offsetcurve][i-restore][action][readiness][context-init]") {
    Fixture f;
    auto* curve = f.arc();
    f.document().select(QList<RS_Entity*>{curve}, true);
    f.context.saveContextMenuActionContext(curve, {1.25, 0}, true);
    f.action = std::make_unique<Probe>(&f.context); f.action->defer = true;
    REQUIRE(f.graphic.removeEntity(curve));
    f.action->init(0);
    CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
    CHECK_FALSE(f.context.getContextMenuActionClickPosition().valid);
    counts(*f.action, 0, 0, 0); CHECK(f.liveEllipses().isEmpty()); noUndo(f.graphic);
}

TEST_CASE("I restore forgets a destroyed document before consulting retained target identities",
          "[lc-offsetcurve][i-restore][action][readiness][document]") {
    Fixture f;
    auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
    auto* curve = f.arc(); curve->setLayer(document->findLayer("0"));
    f.start({curve}, true);
    const auto targetId = curve->getId();
    auto* const deadIdentity = document.get();
    document.reset();
    CHECK(f.view.getDocument() == deadIdentity); CHECK(f.context.getDocument() == deadIdentity);
    f.action->commit();
    counts(*f.action, 0, 0, 1);
    REQUIRE(f.action->lastReport().outcomes.size() == 1);
    CHECK(f.action->lastReport().outcomes.front().targetId == targetId); CHECK(f.action->isFinished());
    noUndo(f.graphic);
}

TEST_CASE("I source update preserves hidden context selection without duplicate registration",
          "[lc-offsetcurve][i-restore][action][context]") {
    class UpdateProbe final : public LC_ActionOffsetCurveUpdateSource {
    public:
        using LC_ActionOffsetCurveUpdateSource::LC_ActionOffsetCurveUpdateSource;
        using LC_ActionOffsetCurveUpdateSource::useSource;
    };
    Fixture f;
    auto* curve = f.arc();
    auto* source = f.ellipse(curve->definition().basis);
    f.document().select(QList<RS_Entity*>{curve}, true);
    curve->setVisible(false);
    REQUIRE(curve->getFlag(RS2::FlagSelected));
    REQUIRE_FALSE(curve->isSelected());
    f.context.saveContextMenuActionContext(curve, {1.25, 0}, false);
    UpdateProbe action(&f.context);
    action.init(0);
    action.useSource(source);
    lc::test::awaitUpdate(action);
    CHECK(action.lastReport().updated == 0);
    CHECK(action.lastReport().unchanged == 0);
    CHECK(action.lastReport().refused == 1);
    REQUIRE(action.lastReport().outcomes.size() == 1);
    CHECK_FALSE(action.lastReport().outcomes.front().edit);
    CHECK(curve->getFlag(RS2::FlagSelected));
    CHECK_FALSE(curve->isDeleted());
    QList<RS_Entity*> selection;
    f.graphic.collectSelected(selection);
    CHECK(selection.count(curve) == 1);
    CHECK(f.graphic.count() == 2);
    noUndo(f.graphic);
}

TEST_CASE("R03 Restore seals peers after successful final points and ordinary duplicate callbacks",
          "[lc-offsetcurve][i-restore][action][r03-final-closure]") {
    class ObservedLine final : public RS_Line {
    public:
        explicit ObservedLine(LC_OffsetCurve& root)
            : RS_Line(&root, root.getStartpoint(), root.getEndpoint()) {}
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
    class OrdinaryCandidate final : public RS_Line {
    public:
        using RS_Line::RS_Line;
        RS2::EntityType rtti() const override {
            if (onType) {
                auto callback = std::move(onType);
                callback();
            }
            return RS2::EntityLine;
        }
        mutable std::function<void()> onType;
    };
    const bool mutate = GENERATE(true, false);
    CAPTURE(mutate);
    KeepSelected keep(true);
    Fixture f;
    auto* first = f.arc(.25);
    auto* later = f.arc(.5); // Same embedded original: exercise private dedup dependencies.
    auto* firstDisplay = new ObservedLine(*first);
    auto* laterDisplay = new ObservedLine(*later);
    first->clear(); first->addEntity(firstDisplay);
    later->clear(); later->addEntity(laterDisplay);
    auto* candidate = f.add(new OrdinaryCandidate(&f.graphic, RS_Vector{20, 20}, RS_Vector{21, 20}));
    f.start({first, later}, true);
    NativeSnapshot firstBefore(*first), laterBefore(*later);
    REQUIRE(first->getUserDefVar("R03_RESTORE_FINAL_METADATA").isEmpty());
    unsigned fired = 0, firstReadsAtMutation = 0, laterReadsAtMutation = 0;
    const auto mutation = [&] {
        ++fired;
        firstReadsAtMutation = firstDisplay->reads;
        laterReadsAtMutation = laterDisplay->reads;
        if (mutate) first->setUserDefVar("R03_RESTORE_FINAL_METADATA", "external final change");
    };
    SECTION("later native final point getter changes an earlier source") {
        laterDisplay->onFourth = mutation;
    }
    SECTION("ordinary duplicate candidate type getter changes an already checked source") {
        candidate->onType = mutation;
    }
    firstDisplay->armed = laterDisplay->armed = true;
    f.graphic.setModified(false);
    f.action->commit(); // Actual public action, source snapshots, dedup and document handoff.
    REQUIRE(fired == 1);
    REQUIRE(firstReadsAtMutation == 4);
    REQUIRE(laterReadsAtMutation == 4);
    firstBefore.check(); laterBefore.check();
    CHECK_FALSE(candidate->isDeleted());
    REQUIRE(f.liveEllipses().size() == 1);
    sameBasis(f.liveEllipses().front()->getData(), later->definition().basis);
    if (mutate) {
        counts(*f.action, 1, 0, 1);
        CHECK(first->getUserDefVar("R03_RESTORE_FINAL_METADATA") == "external final change");
        CHECK_FALSE(f.action->lastReport().outcomes.front().restored);
        CHECK_FALSE(f.action->lastReport().outcomes.front().alreadyPresent);
        CHECK(f.action->lastReport().outcomes.back().restored);
    } else counts(*f.action, 1, 1, 0);
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK(f.liveEllipses().isEmpty());
    CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
    if (mutate) CHECK(first->getUserDefVar("R03_RESTORE_FINAL_METADATA") == "external final change");
    CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo()); CHECK(f.liveEllipses().size() == 1);
}
