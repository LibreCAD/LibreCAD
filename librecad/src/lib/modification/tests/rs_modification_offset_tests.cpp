/*******************************************************************************
**
** This file is part of the LibreCAD project, unit tests for RS_Modification
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

// RS_Modification::offset() used to queue every selected original for deletion
// when "keep originals" was off, whether or not an offset had been produced for
// it, and to report success unconditionally. An inward offset past a circle's
// radius therefore removed the circle, created nothing, and claimed success.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include <QCoreApplication>

#include "lc_curveoffset.h"
#include "lc_splinepoints.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"
#include "rs_spline.h"
#include "rs_vector.h"

namespace {

RS_OffsetData inwardOffset(const double distance) {
    RS_OffsetData data;
    data.coord = RS_Vector{0.0, 0.0}; // the circles below are centred here
    data.distance = distance;
    data.keepOriginals = false;
    data.useCurrentLayer = true;
    data.useCurrentAttributes = true;
    return data;
}

// The batch never owns what it holds; tests that do not apply it free the additions.
struct BatchGuard {
    LC_DocumentModificationBatch ctx;
    ~BatchGuard() { qDeleteAll(ctx.entitiesToAdd); }
};

} // namespace

TEST_CASE("RS_Modification::offset keeps a source it could not offset",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    BatchGuard guard;

    const bool result = RS_Modification::offset(inwardOffset(10.0), {&circle}, false, guard.ctx);

    CHECK_FALSE(result);
    CHECK_FALSE(guard.ctx.success);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("RS_Modification::offset removes only the sources it offset",
          "[modification][offset]") {
    RS_Circle small{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    RS_Circle large{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    BatchGuard guard;

    const bool result = RS_Modification::offset(inwardOffset(10.0), {&small, &large}, false, guard.ctx);

    CHECK(result);
    CHECK(guard.ctx.success);
    REQUIRE(guard.ctx.entitiesToAdd.size() == 1);
    const auto* offsetCircle = dynamic_cast<RS_Circle*>(guard.ctx.entitiesToAdd.front());
    REQUIRE(offsetCircle != nullptr);
    CHECK(offsetCircle->getRadius() == Catch::Approx(10.0));
    REQUIRE(guard.ctx.entitiesToDelete.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.front() == &large);
}

TEST_CASE("RS_Modification::offset ends a series at a copy with nothing left, and keeps the source",
          "[modification][offset]") {
    // Radius 5, three inward copies 2 apart: radii 3 and 1 exist, the third does not.
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    RS_OffsetData data = inwardOffset(2.0);
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard guard;

    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    const LC_OffsetSourceOutcome& source = outcome.sources.front();
    CHECK(source.succeeded());
    CHECK_FALSE(source.complete());
    CHECK(source.copiesMade == 2);
    CHECK(source.copiesRequested == 3);
    CHECK(guard.ctx.entitiesToAdd.size() == 2);
    CHECK(guard.ctx.entitiesToDelete.isEmpty()); // short of a copy: the source stays

    // every copy made: the source goes
    BatchGuard all;
    data.number = 2;
    CHECK(RS_Modification::offset(data, {&circle}, false, all.ctx));
    CHECK(all.ctx.entitiesToAdd.size() == 2);
    CHECK(all.ctx.entitiesToDelete.size() == 1);

    // nothing left at the first copy
    BatchGuard none;
    const LC_OffsetBatchOutcome vanished =
        RS_Modification::offsetWithOutcome(inwardOffset(6.0), {&circle}, false, LC_OffsetBatchLimits{}, none.ctx);
    CHECK(vanished.sources.front().status == LC_OffsetSourceStatus::Vanished);
    CHECK(vanished.sources.front().copiesMade == 0);
    CHECK(none.ctx.entitiesToAdd.isEmpty());
    CHECK(none.ctx.entitiesToDelete.isEmpty());
    CHECK_FALSE(none.ctx.success);
}

TEST_CASE("RS_Modification::offset keeps the originals when asked to",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData data = inwardOffset(10.0);
    data.keepOriginals = true;
    BatchGuard guard;

    CHECK(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK(guard.ctx.entitiesToAdd.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("RS_Modification::offset honours the current layer and pen options",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData data = inwardOffset(10.0);
    data.useCurrentLayer = false;
    data.useCurrentAttributes = false;
    BatchGuard guard;

    REQUIRE(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK_FALSE(guard.ctx.setActiveLayer);
    CHECK_FALSE(guard.ctx.setActivePen);

    data.useCurrentLayer = true;
    BatchGuard second;
    REQUIRE(RS_Modification::offset(data, {&circle}, false, second.ctx));
    CHECK(second.ctx.setActiveLayer);
    CHECK_FALSE(second.ctx.setActivePen);
}

// ---------------------------------------------------------------------------
// The per-source transaction (offsetWithOutcome), with spline sources.
// ---------------------------------------------------------------------------
namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app =
        QCoreApplication::instance() ? QCoreApplication::instance() : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

RS_Spline* sCurve(RS_EntityContainer* parent = nullptr) {
    RS_SplineData d(3, false);
    d.controlPoints = {{0, 0}, {4, 6}, {8, -6}, {12, 0}};
    d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    d.weights.assign(4, 1.0);
    return new RS_Spline(parent, d);
}

/**
 * An arc of radius 0.5 about (0, 2.6), from 45 to 135 degrees, as a rational
 * quadratic. Offset inwards by its radius it shrinks to the centre, which the
 * engine refuses; (0, 3) lies inside it, nearest to its apex (0, 3.1).
 */
RS_Spline* collapsingArc(RS_EntityContainer* parent = nullptr) {
    RS_SplineData d(2, false);
    const double r = 0.5;
    const RS_Vector c{0.0, 2.6};
    d.controlPoints = {c + RS_Vector{r * M_SQRT1_2, r * M_SQRT1_2}, c + RS_Vector{0.0, r * M_SQRT2},
                       c + RS_Vector{-r * M_SQRT1_2, r * M_SQRT1_2}};
    d.knotslist = {0, 0, 0, 1, 1, 1};
    d.weights = {1.0, M_SQRT1_2, 1.0};
    return new RS_Spline(parent, d);
}

RS_OffsetData towards(const RS_Vector& point, const double distance) {
    RS_OffsetData data;
    data.coord = point;
    data.distance = distance;
    data.keepOriginals = false;
    data.useCurrentLayer = true;
    data.useCurrentAttributes = true;
    return data;
}

const LC_OffsetSourceOutcome& outcomeFor(const LC_OffsetBatchOutcome& outcome, const RS_Entity* source) {
    for (const LC_OffsetSourceOutcome& s : outcome.sources) {
        if (s.source == source) {
            return s;
        }
    }
    FAIL("no outcome for the source");
    return outcome.sources.front();
}

} // namespace

TEST_CASE("Offset copies are clamped for Offset only", "[modification][offset]") {
    RS_OffsetData offset;
    offset.multipleCopies = true;
    offset.number = 250;
    CHECK(offset.obtainNumberOfCopies() == RS_OffsetData::kMaximumOffsetCopies);
    offset.number = 0;
    CHECK(offset.obtainNumberOfCopies() == 1);
    // Move, Rotate and Scale share the base helper and keep its range
    RS_MoveData move;
    move.multipleCopies = true;
    move.number = 250;
    CHECK(move.obtainNumberOfCopies() == 250);
    RS_RotateData rotate;
    rotate.multipleCopies = true;
    rotate.number = 250;
    CHECK(rotate.obtainNumberOfCopies() == 250);
    RS_ScaleData scale;
    scale.multipleCopies = true;
    scale.number = 250;
    CHECK(scale.obtainNumberOfCopies() == 250);
}

TEST_CASE("A spline source is offset by the engine and keeps its own outcome", "[modification][offset]") {
    std::unique_ptr<RS_Spline> spline{sCurve()};
    std::unique_ptr<RS_Spline> singular{collapsingArc()};
    RS_Line line{nullptr, RS_LineData{{0.0, 20.0}, {10.0, 20.0}}};
    BatchGuard guard;

    // Towards (6, 9): 0.75 is regular for the curve, and for the line a parallel.
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(towards(RS_Vector{6.0, 9.0}, 0.75), {spline.get(), &line}, false,
                                           LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 2);
    const LC_OffsetSourceOutcome& fromSpline = outcomeFor(outcome, spline.get());
    CHECK(fromSpline.succeeded());
    CHECK(fromSpline.createdEntities.size() == 1); // one spline, its pieces its spans
    CHECK(fromSpline.usage.outputEntities == static_cast<size_t>(fromSpline.createdEntities.size()));
    CHECK(fromSpline.usage.cubicPieces > 1);
    for (const RS_Entity* piece : fromSpline.createdEntities) {
        CHECK(piece->rtti() == RS2::EntitySpline);
    }
    CHECK(outcomeFor(outcome, &line).succeeded());

    BatchGuard failing;
    const LC_OffsetBatchOutcome refused = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{0.0, 3.0}, 0.5), {singular.get()}, false, LC_OffsetBatchLimits{}, failing.ctx);
    // a spline the engine refuses is not retried by mutating a clone
    CHECK(refused.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(failing.ctx.entitiesToAdd.isEmpty());
    CHECK(failing.ctx.entitiesToDelete.isEmpty());
    CHECK_FALSE(failing.ctx.success);
}

TEST_CASE("A mixed selection offsets and removes only the sources that succeed", "[modification][offset]") {
    // From (0, 3): inside the circle, whose inward offset of 0.5 exists; inside
    // the arc of radius 0.5, whose inward offset of 0.5 shrinks to a point.
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 3.0}, 20.0}};
    std::unique_ptr<RS_Spline> spline{collapsingArc()};
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{0.0, 3.0}, 0.5), {spline.get(), &circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 2);
    CHECK(outcome.sources[0].source == spline.get()); // incoming order is kept
    CHECK(outcome.sources[0].status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(outcome.sources[0].createdEntities.isEmpty());
    CHECK(outcome.sources[1].succeeded());
    REQUIRE(outcome.sources[1].createdEntities.size() == 1);
    CHECK(guard.ctx.entitiesToAdd == outcome.sources[1].createdEntities);
    REQUIRE(guard.ctx.entitiesToDelete.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.front() == &circle);
    CHECK(guard.ctx.success);
}

TEST_CASE("Each source is offset once, in the order given", "[modification][offset]") {
    RS_Circle first{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_Circle second{nullptr, RS_CircleData{RS_Vector{100.0, 0.0}, 20.0}};
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        inwardOffset(5.0), {&second, &first, &second}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 2);
    CHECK(outcome.sources[0].source == &second);
    CHECK(outcome.sources[1].source == &first);
    CHECK(guard.ctx.entitiesToAdd.size() == 2);
}

TEST_CASE("Deleted, hidden and locked sources are left alone", "[modification][offset]") {
    RS_Layer locked{"locked"};
    locked.lock(true);
    RS_Circle onLocked{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    onLocked.setLayer(&locked);
    RS_Circle hidden{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    hidden.setVisible(false);
    RS_Circle deleted{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    deleted.setFlag(RS2::FlagDeleted);
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        inwardOffset(5.0), {&onLocked, &hidden, &deleted, nullptr}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 4);
    CHECK(outcomeFor(outcome, &onLocked).status == LC_OffsetSourceStatus::NotVisibleOrLocked);
    CHECK(outcomeFor(outcome, &hidden).status == LC_OffsetSourceStatus::NotVisibleOrLocked);
    CHECK(outcomeFor(outcome, &deleted).status == LC_OffsetSourceStatus::NotVisibleOrLocked);
    CHECK(outcomeFor(outcome, nullptr).status == LC_OffsetSourceStatus::InvalidSource);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("An unusable current layer stops every source before anything is built", "[modification][offset]") {
    ensureSettings();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* target = new RS_Layer("target");
    graphic.addLayer(target);
    graphic.activateLayer(target);
    auto* circle = new RS_Circle(&graphic, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0});
    graphic.addEntity(circle);
    circle->setLayer(graphic.findLayer("0")); // the source itself stays editable

    for (const bool frozen : {false, true}) {
        target->lock(!frozen);
        target->freeze(frozen);
        BatchGuard guard;
        const LC_OffsetBatchOutcome outcome =
            RS_Modification::offsetWithOutcome(inwardOffset(5.0), {circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
        CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::TargetLayerUnavailable);
        CHECK(guard.ctx.entitiesToAdd.isEmpty());
        CHECK(guard.ctx.entitiesToDelete.isEmpty());
    }
    // with the option off, the source's own layer is kept and the offset proceeds
    target->lock(false);
    target->freeze(true);
    RS_OffsetData data = inwardOffset(5.0);
    data.useCurrentLayer = false;
    BatchGuard guard;
    CHECK(RS_Modification::offsetWithOutcome(data, {circle}, false, LC_OffsetBatchLimits{}, guard.ctx)
              .anySourceSucceeded());
}

TEST_CASE("Output limits count every copy of a source, and the request", "[modification][offset][limits]") {
    std::unique_ptr<RS_Spline> spline{sCurve()};
    RS_OffsetData data = towards(RS_Vector{6.0, 9.0}, 0.5);
    BatchGuard one;
    const LC_OffsetBatchOutcome single =
        RS_Modification::offsetWithOutcome(data, {spline.get()}, false, LC_OffsetBatchLimits{}, one.ctx);
    REQUIRE(single.sources.front().succeeded());
    const LC_OffsetOutputUsage perCopy = single.sources.front().usage;

    // room for one copy's pieces: the first copy fits, the second does not, and
    // neither is published nor the source removed
    LC_OffsetBatchLimits tight;
    REQUIRE(perCopy.cubicPieces > 1);
    tight.perSource.maxCubicPieces = perCopy.cubicPieces + 1;
    data.multipleCopies = true;
    data.number = 2;
    BatchGuard two;
    const LC_OffsetBatchOutcome capped =
        RS_Modification::offsetWithOutcome(data, {spline.get()}, false, tight, two.ctx);
    CHECK(capped.sources.front().status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(two.ctx.entitiesToAdd.isEmpty());
    CHECK(two.ctx.entitiesToDelete.isEmpty());

    // a source too large for what is left of the request fails without using it,
    // and a smaller later source still fits
    RS_Circle small{nullptr, RS_CircleData{RS_Vector{50.0, 50.0}, 5.0}};
    RS_OffsetData both = towards(RS_Vector{6.0, 9.0}, 0.5);
    LC_OffsetBatchLimits request;
    request.maxDeepEntitiesPerRequest = perCopy.deepEntities - 1;
    BatchGuard three;
    const LC_OffsetBatchOutcome shared =
        RS_Modification::offsetWithOutcome(both, {spline.get(), &small}, false, request, three.ctx);
    CHECK(outcomeFor(shared, spline.get()).status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(outcomeFor(shared, &small).succeeded());
    CHECK(three.ctx.entitiesToAdd.size() == 1);

    // other entities count their copies the same way: a circle's copy is one
    // entity, so room for one fits the first copy and not the second
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData copies = towards(RS_Vector{0.0, 0.0}, 1.0);
    copies.multipleCopies = true;
    copies.number = 50;
    LC_OffsetBatchLimits oneEntity;
    oneEntity.perSource.maxOutputEntities = 1;
    BatchGuard four;
    const LC_OffsetBatchOutcome refused =
        RS_Modification::offsetWithOutcome(copies, {&circle}, false, oneEntity, four.ctx);
    CHECK(refused.sources.front().status == LC_OffsetSourceStatus::LimitExceeded);
    CHECK(four.ctx.entitiesToAdd.isEmpty());
    CHECK(four.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("A preview batch holds additions only", "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    BatchGuard guard;
    CHECK(RS_Modification::offsetWithOutcome(inwardOffset(5.0), {&circle}, true, LC_OffsetBatchLimits{}, guard.ctx)
              .anySourceSucceeded());
    CHECK(guard.ctx.entitiesToAdd.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("A distance that overflows over the copies is refused", "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData data = inwardOffset(std::numeric_limits<double>::max());
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
    CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
}

namespace {
/** A closed cubic through 8 points of a circle of radius 10: its radius of curvature is 8.5 to 9.3. */
RS_Spline* ring() {
    auto* spline = new RS_Spline(nullptr, RS_SplineData(3, false));
    for (int k = 0; k < 8; ++k) {
        const double a = k * M_PI / 4.0;
        spline->addControlPoint(RS_Vector{10.0 * std::cos(a), 10.0 * std::sin(a)});
    }
    spline->setClosed(true);
    return spline;
}
} // namespace

TEST_CASE("A spline shrunk away is kept, and its copies stop where nothing is left", "[modification][offset]") {
    std::unique_ptr<RS_Spline> source{ring()};
    BatchGuard none;
    const LC_OffsetBatchOutcome vanished = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{0.0, 0.0}, 20.0), {source.get()}, false, LC_OffsetBatchLimits{}, none.ctx);
    REQUIRE(vanished.sources.size() == 1);
    CHECK(vanished.sources.front().status == LC_OffsetSourceStatus::Vanished);
    CHECK(none.ctx.entitiesToAdd.isEmpty());
    CHECK(none.ctx.entitiesToDelete.isEmpty());

    // 4 and 8 inwards exist; 12 is past every radius of curvature
    RS_OffsetData data = towards(RS_Vector{0.0, 0.0}, 4.0);
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard some;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {source.get()}, false, LC_OffsetBatchLimits{}, some.ctx);
    const LC_OffsetSourceOutcome& result = outcome.sources.front();
    CHECK(result.succeeded());
    CHECK(result.copiesMade == 2);
    CHECK(result.copiesRequested == 3);
    CHECK(some.ctx.entitiesToAdd.size() == 2);
    CHECK(some.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("Offsetting a spline cuts the loop past a tight bend", "[modification][offset]") {
    // Inside y = x^2, whose smallest radius of curvature is 0.5, at 1: the Direct
    // offset has two cusps and a loop between them; what the tools make does not.
    RS_SplineData d(2, false);
    d.controlPoints = {{-2, 4}, {0, -4}, {2, 4}};
    d.knotslist = {0, 0, 0, 1, 1, 1};
    d.weights.assign(3, 1.0);
    const RS_Spline parabola(nullptr, d);
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{0.0, 3.0}, 1.0), {const_cast<RS_Spline*>(&parabola)}, false, LC_OffsetBatchLimits{},
        guard.ctx);
    REQUIRE(outcome.sources.front().succeeded());
    REQUIRE_FALSE(guard.ctx.entitiesToAdd.isEmpty());
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeOffsetOptions(parabola, 1.0);
    // the least distance from the source over dense samples of the entities
    const auto nearestApproach = [&](const auto& entities) {
        double least = RS_MAXDOUBLE;
        for (const auto& e : entities) {
            const auto* spline = dynamic_cast<const RS_Spline*>(&*e);
            REQUIRE(spline != nullptr);
            double t0 = 0.0;
            double t1 = 0.0;
            REQUIRE(spline->getParameterDomain(t0, t1));
            for (int k = 0; k <= 200; ++k) {
                LC_CurveJet jet;
                REQUIRE(spline->tryEvaluateJet(t0 + (t1 - t0) * k / 200.0, LC_CurveEvaluationSide::Interior, jet));
                const LC_OffsetSideResolution nearest = LC_CurveOffset::resolveSide(parabola, jet.point, options);
                REQUIRE(nearest.status == LC_CurveOffsetStatus::Ok);
                least = std::min(least, nearest.distance);
            }
        }
        return least;
    };
    CHECK(nearestApproach(guard.ctx.entitiesToAdd) >= 1.0 - 2.0 * options.tolerance.requestedGeometry);
    const LC_CurveOffsetMaterializationResult direct = LC_CurveOffset::createEntities(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0),
        LC_CurveOffset::makeDirectOptions(parabola, 1.0), LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
    CHECK(nearestApproach(direct.entities) < 0.9);
}

TEST_CASE("The engine's reason for refusing a spline is kept", "[modification][offset]") {
    std::unique_ptr<RS_Spline> spline{sCurve()};
    LC_CurveJet onCurve;
    REQUIRE(spline->tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, onCurve));
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(onCurve.point, 0.5), {spline.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(outcome.sources.front().engineStatus == LC_CurveOffsetStatus::AmbiguousSide);

    std::unique_ptr<RS_Spline> singular{collapsingArc()};
    BatchGuard failing;
    const LC_OffsetBatchOutcome refused = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{0.0, 3.0}, 0.5), {singular.get()}, false, LC_OffsetBatchLimits{}, failing.ctx);
    CHECK(refused.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(refused.sources.front().engineStatus != LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Preview limits are an eighth of the commit's", "[modification][offset][limits]") {
    const LC_OffsetBatchLimits full;
    const LC_OffsetBatchLimits preview = LC_OffsetBatchLimits::preview();
    CHECK(preview.perSource.maxCubicPieces * 8 == full.perSource.maxCubicPieces);
    CHECK(preview.perSource.maxOutputEntities * 8 == full.perSource.maxOutputEntities);
    CHECK(preview.perSource.maxDeepEntities * 8 == full.perSource.maxDeepEntities);
    CHECK(preview.maxDeepEntitiesPerRequest * 8 == full.maxDeepEntitiesPerRequest);
    CHECK(preview.maxSamples * 8 == LC_CurveOffset::kDefaultMaxSamples);
    CHECK(preview.maxIntersectionPairs * 8 == LC_CurveOffset::kDefaultMaxIntersectionPairs);
    CHECK(isValidOffsetBudget(preview.perSource));
}

TEST_CASE("A preview cache hands back copies of what fits the budget left", "[modification][offset]") {
    std::unique_ptr<RS_Spline> spline{sCurve()};
    LC_OffsetPreviewCache cache;
    const LC_OffsetSourceBudget budget = makeDefaultOffsetSourceBudget();

    // a success is reused while its output fits
    LC_CurveOffsetMaterializationResult made;
    made.status = LC_CurveOffsetStatus::Ok;
    made.entities.emplace_back(new RS_Line(nullptr, RS_LineData{{0, 0}, {1, 1}}));
    made.usage = {1, 1, 1};
    cache.keep(spline.get(), LC_CurveOffsetSide::Left, 0.5, budget, made);
    LC_CurveOffsetMaterializationResult found;
    REQUIRE(cache.find(spline.get(), LC_CurveOffsetSide::Left, 0.5, budget, found));
    CHECK(found.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(found.entities.size() == 1);
    CHECK(found.entities.front().get() != made.entities.front().get()); // a copy
    CHECK(found.entities.front()->getEndpoint() == RS_Vector(1, 1));
    CHECK_FALSE(cache.find(spline.get(), LC_CurveOffsetSide::Right, 0.5, budget, found));
    CHECK_FALSE(cache.find(spline.get(), LC_CurveOffsetSide::Left, 0.75, budget, found));
    CHECK_FALSE(cache.find(spline.get(), LC_CurveOffsetSide::Left, 0.5, LC_OffsetSourceBudget{0, 0, 0}, found));

    // a refusal for want of budget holds for no larger a budget
    LC_CurveOffsetMaterializationResult tooMuch;
    tooMuch.status = LC_CurveOffsetStatus::LimitExceeded;
    const LC_OffsetSourceBudget small{8, 8, 8};
    cache.keep(spline.get(), LC_CurveOffsetSide::Left, 2.0, small, tooMuch);
    CHECK(cache.find(spline.get(), LC_CurveOffsetSide::Left, 2.0, LC_OffsetSourceBudget{4, 8, 8}, found));
    CHECK(found.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK_FALSE(cache.find(spline.get(), LC_CurveOffsetSide::Left, 2.0, budget, found));

    // any other refusal always
    LC_CurveOffsetMaterializationResult singular;
    singular.status = LC_CurveOffsetStatus::SingularOffset;
    cache.keep(spline.get(), LC_CurveOffsetSide::Left, 3.0, small, singular);
    CHECK(cache.find(spline.get(), LC_CurveOffsetSide::Left, 3.0, budget, found));
    CHECK(found.status == LC_CurveOffsetStatus::SingularOffset);

    cache.clear();
    CHECK_FALSE(cache.find(spline.get(), LC_CurveOffsetSide::Left, 0.5, budget, found));
}

TEST_CASE("A preview made through the cache is the preview made without it", "[modification][offset]") {
    std::unique_ptr<RS_Spline> spline{sCurve()};
    LC_OffsetPreviewCache cache;
    const RS_OffsetData data = towards(RS_Vector{6.0, 9.0}, 0.75);
    std::vector<std::vector<RS_Vector>> runs;
    for (int run = 0; run < 3; ++run) {
        LC_DocumentModificationBatch ctx;
        RS_Modification::offsetWithOutcome(data, {spline.get()}, true, LC_OffsetBatchLimits::preview(), ctx,
                                           run == 0 ? nullptr : &cache);
        REQUIRE(ctx.entitiesToAdd.size() == 1);
        const auto* offset = dynamic_cast<const RS_Spline*>(ctx.entitiesToAdd.front());
        REQUIRE(offset != nullptr);
        runs.push_back(offset->getData().controlPoints);
        qDeleteAll(ctx.entitiesToAdd);
    }
    CHECK(runs[1] == runs[0]); // filled the cache
    CHECK(runs[2] == runs[0]); // from the cache
}
