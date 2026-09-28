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
#include "lc_hyperbola.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_polyline.h"
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

// ---------------------------------------------------------------------------
// Polylines: RS_Polyline::offset() joins neighbouring offsets and never trims,
// so what it makes is checked before it is used.
// ---------------------------------------------------------------------------
namespace {
/** A polyline through @p vertices, owned by the caller. */
std::unique_ptr<RS_Polyline> polylineThrough(const std::vector<RS_Vector>& vertices, const bool closed) {
    auto polyline = std::make_unique<RS_Polyline>(nullptr);
    for (const RS_Vector& v : vertices) {
        polyline->addVertex(v);
    }
    if (closed) {
        polyline->setClosed(true);
        polyline->endPolyline(); // adds the closing segment
    }
    return polyline;
}

/** Two 4 x 4 squares joined by a neck 1 high. */
std::unique_ptr<RS_Polyline> dumbbell() {
    return polylineThrough({{0, 0}, {4, 0}, {4, 1.5}, {8, 1.5}, {8, 0}, {12, 0}, {12, 4}, {8, 4}, {8, 2.5}, {4, 2.5},
                            {4, 4}, {0, 4}},
                           true);
}

std::unique_ptr<RS_Polyline> rectangle() {
    return polylineThrough({{0, 0}, {10, 0}, {10, 4}, {0, 4}}, true);
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A polyline whose offset would cross itself is kept, and nothing is made", "[modification][offset]") {
    // the dumbbell shrunk by 0.6: the legacy offset is one loop that crosses
    // itself four times at the neck
    const auto source = dumbbell();
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{1.0, 2.0}, 0.6), {source.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(outcome.sources.front().detail == LC_OffsetFailureDetail::PolylineNotTrimmed);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
    CHECK_FALSE(outcome.sources.front().sourceRemoved);

    // at 0.4 the legacy offset is a proper ring, and is used
    BatchGuard ring;
    const LC_OffsetBatchOutcome kept = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{1.0, 2.0}, 0.4), {source.get()}, false, LC_OffsetBatchLimits{}, ring.ctx);
    CHECK(kept.sources.front().succeeded());
    REQUIRE(ring.ctx.entitiesToAdd.size() == 1);
    CHECK(static_cast<const RS_Polyline*>(ring.ctx.entitiesToAdd.front())->count() == 12);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A polyline shrunk past its size vanishes and is kept", "[modification][offset]") {
    // the 10 x 4 rectangle shrunk by 3: the legacy offset is an inverted 4 x 2 rectangle
    const auto source = rectangle();
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{5.0, 1.5}, 3.0), {source.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::Vanished);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A polyline series stops where nothing is left", "[modification][offset]") {
    // M-M1: the rectangle at 1, 2 and 3; at 2 its legacy offset is a zero-area loop
    const auto source = rectangle();
    RS_OffsetData data = towards(RS_Vector{5.0, 1.5}, 1.0);
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {source.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    const LC_OffsetSourceOutcome& result = outcome.sources.front();
    CHECK(result.succeeded());
    CHECK(result.copiesMade == 1);
    CHECK(result.copiesRequested == 3);
    CHECK(result.stoppedBy == LC_OffsetSourceStatus::Vanished);
    REQUIRE(guard.ctx.entitiesToAdd.size() == 1);
    const auto* copy = static_cast<const RS_Polyline*>(guard.ctx.entitiesToAdd.front());
    CHECK(copy->getMin().distanceTo(RS_Vector{1, 1}) < 1e-9);
    CHECK(copy->getMax().distanceTo(RS_Vector{9, 3}) < 1e-9);
    CHECK(guard.ctx.entitiesToDelete.isEmpty()); // short of copies: the source stays
}

namespace {
/**
 * Straight arms exactly 10 apart (lc_curveoffset_trim_tests.cpp): between
 * them, at 5, their offsets lie on one line, which the engine refuses to trim.
 */
std::unique_ptr<LC_SplinePoints> keyhole(const RS_Vector& shift = RS_Vector{0.0, 0.0}) {
    LC_SplinePointsData d(false, false);
    d.useControlPoints = true;
    for (const RS_Vector& p : {RS_Vector{-20, 0}, RS_Vector{-10, 0}, RS_Vector{0, 0}, RS_Vector{10, -8},
                               RS_Vector{24, -8}, RS_Vector{24, 18}, RS_Vector{10, 18}, RS_Vector{0, 10},
                               RS_Vector{-10, 10}, RS_Vector{-20, 10}}) {
        d.controlPoints.push_back(p + shift);
    }
    return std::make_unique<LC_SplinePoints>(nullptr, d);
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A failure after the first copy keeps the copies made", "[modification][offset]") {
    const auto source = keyhole();
    // between the arms, nearer the lower one: the left of the curve
    RS_OffsetData data = towards(RS_Vector{-10.0, 2.0}, 2.5);
    BatchGuard first;
    const LC_OffsetBatchOutcome one =
        RS_Modification::offsetWithOutcome(data, {source.get()}, false, LC_OffsetBatchLimits{}, first.ctx);
    REQUIRE(one.sources.front().succeeded()); // 2.5 is regular
    data.multipleCopies = true;
    data.number = 2;
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {source.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    const LC_OffsetSourceOutcome& result = outcome.sources.front();
    CHECK(result.succeeded());
    CHECK(result.copiesMade == 1);
    CHECK(result.stoppedBy == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(result.engineStatus == LC_CurveOffsetStatus::AmbiguousTopology);
    CHECK(guard.ctx.entitiesToAdd.size() == static_cast<qsizetype>(result.createdEntities.size()));
    CHECK_FALSE(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
    CHECK_FALSE(result.sourceRemoved);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A zero-length line and an unbounded hyperbola are refused with their reasons", "[modification][offset]") {
    RS_Line point{nullptr, RS_LineData{{1.0, 1.0}, {1.0, 1.0}}};
    BatchGuard line;
    const LC_OffsetBatchOutcome fromLine = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{1.0, 3.0}, 1.0), {&point}, false, LC_OffsetBatchLimits{}, line.ctx);
    CHECK(fromLine.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(fromLine.sources.front().detail == LC_OffsetFailureDetail::ZeroLength);
    CHECK(line.ctx.entitiesToAdd.isEmpty());

    // x^2/9 - y^2/4 = 1, the whole right branch
    LC_Hyperbola branch{nullptr, LC_HyperbolaData{RS_Vector{0.0, 0.0}, RS_Vector{3.0, 0.0}, 2.0 / 3.0}};
    REQUIRE(branch.isInfinite());
    BatchGuard hyperbola;
    const LC_OffsetBatchOutcome fromHyperbola = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{10.0, 0.0}, 1.0), {&branch}, false, LC_OffsetBatchLimits{}, hyperbola.ctx);
    CHECK(fromHyperbola.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(fromHyperbola.sources.front().detail == LC_OffsetFailureDetail::Unbounded);
    CHECK(hyperbola.ctx.entitiesToAdd.isEmpty());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An arc shrunk past its radius vanishes, and an arc series stops", "[modification][offset]") {
    // radius 5 about the origin, 0 to 90 degrees; (1, 1) is inside it
    RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 5.0, 0.0, M_PI_2, false}};
    BatchGuard none;
    const LC_OffsetBatchOutcome vanished = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{1.0, 1.0}, 6.0), {&arc}, false, LC_OffsetBatchLimits{}, none.ctx);
    CHECK(vanished.sources.front().status == LC_OffsetSourceStatus::Vanished);
    CHECK(none.ctx.entitiesToAdd.isEmpty());
    CHECK(none.ctx.entitiesToDelete.isEmpty());

    RS_OffsetData data = towards(RS_Vector{1.0, 1.0}, 2.0);
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard some;
    const LC_OffsetBatchOutcome outcome =
        RS_Modification::offsetWithOutcome(data, {&arc}, false, LC_OffsetBatchLimits{}, some.ctx);
    const LC_OffsetSourceOutcome& result = outcome.sources.front();
    CHECK(result.succeeded());
    CHECK(result.copiesMade == 2);
    CHECK(result.stoppedBy == LC_OffsetSourceStatus::Vanished);
    REQUIRE(some.ctx.entitiesToAdd.size() == 2);
    CHECK(static_cast<const RS_Arc*>(some.ctx.entitiesToAdd[0])->getRadius() == Catch::Approx(3.0));
    CHECK(static_cast<const RS_Arc*>(some.ctx.entitiesToAdd[1])->getRadius() == Catch::Approx(1.0));
    CHECK(some.ctx.entitiesToDelete.isEmpty());
}

namespace {
/** Whether @p polyline's vertices are @p expected, in any order and from any start. */
bool hasVertices(const RS_Polyline& polyline, const std::vector<RS_Vector>& expected) {
    std::vector<RS_Vector> vertices{polyline.getStartpoint()};
    for (const RS_Entity* segment : polyline) {
        vertices.push_back(segment->getEndpoint());
    }
    if (polyline.isClosed() && vertices.size() > 1 && vertices.front().distanceTo(vertices.back()) < 1e-9) {
        vertices.pop_back();
    }
    if (vertices.size() != expected.size()) {
        return false;
    }
    return std::all_of(vertices.begin(), vertices.end(), [&expected](const RS_Vector& v) {
        return std::any_of(expected.begin(), expected.end(), [&v](const RS_Vector& e) { return v.distanceTo(e) < 1e-9; });
    });
}

/** The one entity @p guard's batch holds. */
const RS_Entity& onlyAdded(const BatchGuard& guard) {
    REQUIRE(guard.ctx.entitiesToAdd.size() == 1);
    return *guard.ctx.entitiesToAdd.front();
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A zero distance is refused for every type", "[modification][offset]") {
    RS_Line line{nullptr, RS_LineData{{0.0, 0.0}, {10.0, 0.0}}};
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 5.0, 0.0, M_PI_2, false}};
    RS_Ellipse ellipse{nullptr, RS_EllipseData{RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}, 0.5, 0.0, 0.0, false}};
    const auto polyline = rectangle();
    std::unique_ptr<RS_Spline> spline{sCurve()};
    for (RS_Entity* source : std::initializer_list<RS_Entity*>{&line, &circle, &arc, &ellipse, polyline.get(), spline.get()}) {
        INFO("rtti " << source->rtti());
        BatchGuard guard;
        const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
            towards(RS_Vector{1.0, 1.0}, 0.0), {source}, false, LC_OffsetBatchLimits{}, guard.ctx);
        REQUIRE(outcome.sources.size() == 1);
        CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
        CHECK(outcome.sources.front().detail == LC_OffsetFailureDetail::InvalidDistance);
        CHECK(guard.ctx.entitiesToAdd.isEmpty());
        CHECK(guard.ctx.entitiesToDelete.isEmpty());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A negative distance offsets by its size", "[modification][offset]") {
    // the pick decides the side, not the sign: a line flipped over, and a
    // polyline's segments went to alternating sides
    RS_Line line{nullptr, RS_LineData{{0.0, 0.0}, {10.0, 0.0}}};
    BatchGuard fromLine;
    RS_Modification::offsetWithOutcome(towards(RS_Vector{5.0, 1.0}, -2.0), {&line}, false, LC_OffsetBatchLimits{},
                                       fromLine.ctx);
    const RS_Entity& parallel = onlyAdded(fromLine);
    CHECK(parallel.getStartpoint().distanceTo(RS_Vector{0.0, 2.0}) < 1e-9);
    CHECK(parallel.getEndpoint().distanceTo(RS_Vector{10.0, 2.0}) < 1e-9);

    const auto l = polylineThrough({{0, 0}, {10, 0}, {10, 10}}, false);
    BatchGuard fromPolyline;
    RS_Modification::offsetWithOutcome(towards(RS_Vector{5.0, -1.0}, -1.0), {l.get()}, false, LC_OffsetBatchLimits{},
                                       fromPolyline.ctx);
    const RS_Entity& offset = onlyAdded(fromPolyline);
    REQUIRE(offset.rtti() == RS2::EntityPolyline);
    CHECK(hasVertices(static_cast<const RS_Polyline&>(offset), {{0, -1}, {11, -1}, {11, 10}}));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A point on the entity takes its side from the second click", "[modification][offset]") {
    const auto fromPoints = [](RS_Entity& source, const RS_Vector& reference, const RS_Vector& fallback,
                               const double distance, BatchGuard& guard) {
        RS_OffsetData data = towards(reference, distance);
        data.sideFallback = fallback;
        const LC_OffsetBatchOutcome outcome =
            RS_Modification::offsetWithOutcome(data, {&source}, false, LC_OffsetBatchLimits{}, guard.ctx);
        REQUIRE(outcome.sources.size() == 1);
        CHECK(outcome.sources.front().succeeded());
        return &onlyAdded(guard);
    };
    SECTION("a line") {
        RS_Line line{nullptr, RS_LineData{{0.0, 0.0}, {10.0, 0.0}}};
        BatchGuard guard;
        const RS_Entity* parallel = fromPoints(line, {5.0, 0.0}, {5.0, -3.0}, 3.0, guard);
        CHECK(parallel->getStartpoint().y == Catch::Approx(-3.0));
        CHECK(parallel->getEndpoint().y == Catch::Approx(-3.0));
    }
    SECTION("a circle") {
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
        BatchGuard guard;
        const RS_Entity* offset = fromPoints(circle, {5.0, 0.0}, {8.0, 0.0}, 3.0, guard);
        CHECK(offset->getRadius() == Catch::Approx(8.0));
    }
    SECTION("an arc") {
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 5.0, 0.0, M_PI_2, false}};
        const double onIt = 5.0 * std::cos(M_PI_4);
        BatchGuard guard;
        const RS_Entity* offset = fromPoints(arc, {onIt, onIt}, {1.0, 1.0}, 1.0, guard);
        CHECK(offset->getRadius() == Catch::Approx(4.0));
    }
    SECTION("a polyline") {
        const auto source = rectangle();
        struct Row {
            RS_Vector fallback;
            std::vector<RS_Vector> expected;
        };
        for (const Row& row : {Row{{5, 1}, {{1, 1}, {9, 1}, {9, 3}, {1, 3}}},
                               Row{{5, -1}, {{-1, -1}, {11, -1}, {11, 5}, {-1, 5}}},
                               Row{{9.5, 2}, {{1, 1}, {9, 1}, {9, 3}, {1, 3}}}}) { // nearer another segment
            INFO("fallback " << row.fallback.x << ", " << row.fallback.y);
            BatchGuard guard;
            const RS_Entity* offset = fromPoints(*source, {5.0, 0.0}, row.fallback, 1.0, guard);
            REQUIRE(offset->rtti() == RS2::EntityPolyline);
            CHECK(hasVertices(*static_cast<const RS_Polyline*>(offset), row.expected));
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A point on a line with no second click gives no side", "[modification][offset]") {
    RS_Line line{nullptr, RS_LineData{{0.0, 0.0}, {10.0, 0.0}}};
    for (const RS_Vector& onIt : {RS_Vector{5.0, 0.0}, RS_Vector{15.0, 0.0}}) { // its line, beyond its end too
        BatchGuard guard;
        const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
            towards(onIt, 3.0), {&line}, false, LC_OffsetBatchLimits{}, guard.ctx);
        CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
        CHECK(outcome.sources.front().engineStatus == LC_CurveOffsetStatus::AmbiguousSide);
        CHECK(outcome.sources.front().detail == LC_OffsetFailureDetail::AmbiguousSide);
        CHECK(guard.ctx.entitiesToAdd.isEmpty());
        CHECK(guard.ctx.entitiesToDelete.isEmpty());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A circle inside the vanish band vanishes", "[modification][offset]") {
    // radius 5: its band is 1.25e-6 of its box's diagonal, 1.77e-5; a circle
    // drawn as two bulges vanishes within the same band
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    for (const double left : {1e-9, 5e-6}) {
        INFO("radius left " << left);
        BatchGuard guard;
        const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
            inwardOffset(5.0 - left), {&circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
        CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::Vanished);
        CHECK(guard.ctx.entitiesToAdd.isEmpty());
        CHECK(guard.ctx.entitiesToDelete.isEmpty());
    }
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        inwardOffset(5.0 - 1e-4), {&circle}, false, LC_OffsetBatchLimits{}, guard.ctx);
    CHECK(outcome.sources.front().succeeded());
    CHECK(onlyAdded(guard).getRadius() == Catch::Approx(1e-4).epsilon(1e-6));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A polyline with an elliptic segment is refused", "[modification][offset]") {
    // as a non-uniform scale leaves one: a line, the lower half of an ellipse
    // about (15, 0), 5 by 2.5, and a line
    auto polyline = std::make_unique<RS_Polyline>(nullptr);
    polyline->addVertex(RS_Vector{0, 0});
    polyline->addVertex(RS_Vector{10, 0});
    polyline->RS_EntityContainer::addEntity(new RS_Ellipse(
        polyline.get(), RS_EllipseData{RS_Vector{15, 0}, RS_Vector{5, 0}, 0.5, M_PI, 2.0 * M_PI, false}));
    polyline->setEndpoint(RS_Vector{20, 0});
    polyline->addVertex(RS_Vector{30, 0});
    REQUIRE(polyline->count() == 3);
    BatchGuard guard;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        towards(RS_Vector{15.0, -6.0}, 1.0), {polyline.get()}, false, LC_OffsetBatchLimits{}, guard.ctx);
    REQUIRE(outcome.sources.size() == 1);
    CHECK(outcome.sources.front().status == LC_OffsetSourceStatus::OffsetFailed);
    CHECK(outcome.sources.front().detail == LC_OffsetFailureDetail::EllipticSegments);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}
