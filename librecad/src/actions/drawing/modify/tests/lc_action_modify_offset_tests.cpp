/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

// Modify Offset with spline sources: admission, commit, per-source selection,
// a failed trigger that keeps the action and selection, and preview detail.

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>

#include <QStringList>

#include "lc_action_draw_line_parallel_through.h"
#include "lc_action_modify_offset.h"
#include "lc_actiontestsupport.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "rs_circle.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_spline.h"

namespace {

using lc::test::eventAt;

class CapturingContext final : public LC_ActionContext {
public:
    QString prompt;
    QStringList messages;

    void updateActionPrompt(const QString& left, [[maybe_unused]] const QString& right,
                            [[maybe_unused]] const LC_ModifiersInfo& modifiers) override {
        prompt = left;
    }

    void commandMessage(const QString& message) override {
        messages << message;
    }
};

class OffsetProbe final : public LC_ActionModifyOffset {
public:
    explicit OffsetProbe(LC_ActionContext* context) : LC_ActionModifyOffset(context) {}

    using LC_ActionModifyOffset::SetReferencePoint;
    using LC_ActionModifyOffset::onMouseLeftButtonReleaseSelected;
    using LC_ActionModifyOffset::onMouseMoveEventSelected;
    using LC_ActionModifyOffset::updateActionPromptForSelection;
    using LC_ActionPreSelectionAwareBase::m_selectedEntities;
    using LC_ActionPreSelectionAwareBase::m_selectionComplete;
    using RS_ActionSelectBase::m_catchForSelectionEntityTypes;
    using RS_PreviewActionInterface::catchEntityByEvent;
    using RS_PreviewActionInterface::deletePreviewAndHighlights;
    using RS_PreviewActionInterface::drawPreviewAndHighlights;
    using RS_PreviewActionInterface::m_preview;
};

class ParallelThroughProbe final : public LC_ActionDrawLineParallelThrough {
public:
    explicit ParallelThroughProbe(LC_ActionContext* context) : LC_ActionDrawLineParallelThrough(context) {}

    using LC_ActionDrawLineParallelThrough::SetEntity;
    using LC_ActionDrawLineParallelThrough::SetPos;
    using LC_ActionDrawLineParallelThrough::m_entity;
    using LC_ActionDrawLineParallelThrough::onMouseMoveEvent;
    using RS_PreviewActionInterface::m_preview;
};

struct OffsetFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    CapturingContext m_context;
    std::unique_ptr<OffsetProbe> m_action;

    OffsetFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        LC_SET_ONE("Appearance", "MaxPreview", 100);
    }

    ~OffsetFixture() {
        m_action.reset();
        LC_SET_ONE("Appearance", "MaxPreview", 100);
    }

    template <typename Entity>
    Entity* add(Entity* entity) {
        m_graphic.addEntity(entity);
        return entity;
    }

    RS_Spline* addSCurve() {
        RS_SplineData d(3, false);
        d.controlPoints = {{0, 0}, {4, 6}, {8, -6}, {12, 0}};
        d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
        d.weights.assign(4, 1.0);
        return add(new RS_Spline(&m_graphic, d));
    }

    /**
     * An arc of radius 0.5 about (0, 2.6), from 45 to 135 degrees: offset inwards
     * by 0.5 it shrinks to its centre, which the engine refuses. (0, 3) lies
     * inside it.
     */
    RS_Spline* addCollapsingArc() {
        RS_SplineData d(2, false);
        const double r = 0.5;
        const RS_Vector c{0.0, 2.6};
        d.controlPoints = {c + RS_Vector{r * M_SQRT1_2, r * M_SQRT1_2}, c + RS_Vector{0.0, r * M_SQRT2},
                           c + RS_Vector{-r * M_SQRT1_2, r * M_SQRT1_2}};
        d.knotslist = {0, 0, 0, 1, 1, 1};
        d.weights = {1.0, M_SQRT1_2, 1.0};
        return add(new RS_Spline(&m_graphic, d));
    }

    /** A closed cubic through 8 points of a circle of radius 10: its radius of curvature is 8.5 to 9.3. */
    RS_Spline* addRing() {
        auto* spline = add(new RS_Spline(&m_graphic, RS_SplineData(3, false)));
        for (int k = 0; k < 8; ++k) {
            const double a = k * M_PI / 4.0;
            spline->addControlPoint(RS_Vector{10.0 * std::cos(a), 10.0 * std::sin(a)});
        }
        spline->setClosed(true);
        return spline;
    }

    LC_SplinePoints* addSplinePoints() {
        LC_SplinePointsData d(false, false);
        d.splinePoints = {{20, 0}, {23, 4}, {27, 3}, {30, 6}, {34, 2}};
        return add(new LC_SplinePoints(&m_graphic, d));
    }

    void select(const std::initializer_list<RS_Entity*> entities) {
        const RS_Selection selection(&m_graphic, m_view.getViewPort());
        for (RS_Entity* e : entities) {
            selection.selectSingle(e);
        }
    }

    /** Starts the action on the current selection, with a fixed distance. */
    void start(const double distance, const bool keepOriginals) {
        m_action = std::make_unique<OffsetProbe>(&m_context);
        m_action->setDistanceFixed(true);
        m_action->setDistance(distance);
        m_action->setKeepOriginals(keepOriginals);
        m_action->setUseMultipleCopies(false);
        m_action->init(OffsetProbe::SetReferencePoint);
    }

    void clickAt(const double x, const double y) const {
        const LC_MouseEvent e = eventAt(x, y);
        m_action->onMouseLeftButtonReleaseSelected(OffsetProbe::SetReferencePoint, &e);
    }

    /** A mouse move, as RS_PreviewActionInterface::mouseMoveEvent() runs it. */
    void hoverAt(const double x, const double y) const {
        m_action->deletePreviewAndHighlights();
        const LC_MouseEvent e = eventAt(x, y);
        m_action->onMouseMoveEventSelected(OffsetProbe::SetReferencePoint, &e);
        m_action->drawPreviewAndHighlights();
    }

    /** Live entities of the drawing with this type. */
    int liveCount(const RS2::EntityType type) const {
        int count = 0;
        for (const RS_Entity* e : m_graphic) {
            count += (e->rtti() == type && !e->isDeleted()) ? 1 : 0;
        }
        return count;
    }

    /** Preview entities with this type, counting the reference entities RS_Preview keeps apart. */
    int previewCount(const RS2::EntityType type) const {
        RS_EntityContainer references(nullptr, false);
        m_action->m_preview->addReferenceEntitiesToContainer(&references);
        int count = 0;
        for (const RS_EntityContainer* container : {static_cast<RS_EntityContainer*>(m_action->m_preview.get()),
                                                    &references}) {
            for (const RS_Entity* e : *container) {
                count += (e->rtti() == type) ? 1 : 0;
            }
        }
        return count;
    }
};

} // namespace

TEST_CASE("Modify Offset admits both spline types and names them", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    LC_SplinePoints* points = f.addSplinePoints();
    RS_Line* line = f.add(new RS_Line(&f.m_graphic, RS_LineData{{0, 20}, {10, 20}}));
    f.select({spline, points, line});
    f.start(0.5, true);
    CHECK(f.m_action->m_selectionComplete);
    CHECK(f.m_action->m_selectedEntities.size() == 3);
    CHECK(f.m_action->m_selectedEntities.contains(spline));
    CHECK(f.m_action->m_selectedEntities.contains(points));

    // the pre-existing types keep their order; the spline types come after them
    const auto& types = f.m_action->m_catchForSelectionEntityTypes;
    CHECK(types.indexOf(RS2::EntitySpline) > types.indexOf(RS2::EntityPolyline));
    CHECK(types.indexOf(RS2::EntitySplinePoints) > types.indexOf(RS2::EntityPolyline));

    f.m_action->updateActionPromptForSelection();
    CHECK(f.m_context.prompt.contains("spline"));
    CHECK(f.m_context.prompt.contains("spline through points"));
}

TEST_CASE("A selection window takes the spline, not the segments it is drawn with", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    spline->update();
    REQUIRE(spline->count() > 0);
    f.start(0.5, true);
    RS_Selection selection(&f.m_graphic, f.m_view.getViewPort());
    selection.selectWindow(f.m_action->m_catchForSelectionEntityTypes, RS_Vector{-1.0, -3.0}, RS_Vector{13.0, 3.0},
                           true, false);
    CHECK(spline->isSelected());

    f.start(0.5, true); // picks the preselection up, as it does for the other types
    CHECK(f.m_action->m_selectionComplete);
    REQUIRE(f.m_action->m_selectedEntities.size() == 1);
    CHECK(f.m_action->m_selectedEntities.front() == spline);
}

TEST_CASE("A click selects the spline itself, not a segment it is drawn with", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    f.start(0.5, true);
    LC_CurveJet jet;
    REQUIRE(spline->tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, jet));
    const LC_MouseEvent e = eventAt(jet.point.x, jet.point.y);
    CHECK(f.m_action->catchEntityByEvent(&e, f.m_action->m_catchForSelectionEntityTypes) == spline);
}

TEST_CASE("Committing a spline offset replaces it with its offset, undoably", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    f.select({spline});
    f.start(0.75, false);
    f.clickAt(6.0, 9.0);

    CHECK(spline->isDeleted());
    const int pieces = f.liveCount(RS2::EntitySpline);
    CHECK(pieces == 1); // one spline for the whole offset
    CHECK(f.m_context.messages.isEmpty());

    f.m_graphic.undo();
    CHECK_FALSE(spline->isDeleted());
    CHECK(f.liveCount(RS2::EntitySpline) == 1);
    f.m_graphic.redo();
    CHECK(spline->isDeleted());
    CHECK(f.liveCount(RS2::EntitySpline) == pieces);
}

TEST_CASE("A trigger that offsets nothing keeps the action, its step and the selection", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addCollapsingArc();
    f.select({spline});
    f.start(0.5, false); // inside, by its radius: shrinks to a point
    f.clickAt(0.0, 3.0);

    CHECK_FALSE(spline->isDeleted());
    CHECK(spline->isSelected());
    CHECK(f.liveCount(RS2::EntitySpline) == 1);
    CHECK(f.m_action->getStatus() == OffsetProbe::SetReferencePoint);
    CHECK(f.m_action->m_selectionComplete);
    CHECK(f.m_action->m_selectedEntities.contains(spline));
    REQUIRE(f.m_context.messages.size() == 1);
    CHECK(f.m_context.messages.front().contains("1 of 1"));

    // another distance works from where the user left off
    f.m_action->setDistance(0.4);
    f.clickAt(0.0, 3.0);
    CHECK(spline->isDeleted());
}

TEST_CASE("Only sources that were offset leave the selection", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addCollapsingArc();
    RS_Circle* circle = f.add(new RS_Circle(&f.m_graphic, RS_CircleData{RS_Vector{0.0, 3.0}, 20.0}));
    f.select({spline, circle});
    f.start(0.5, true);
    f.clickAt(0.0, 3.0); // inside the circle: its inward offset works; the spline's does not

    CHECK_FALSE(circle->isSelected());
    CHECK(spline->isSelected());
    CHECK(f.liveCount(RS2::EntityCircle) == 2);
    REQUIRE(f.m_context.messages.size() == 1);
    CHECK(f.m_context.messages.front().contains("1 of 2"));
}

TEST_CASE("The preview shows the committed geometry, or its box past MaxPreview", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    f.select({spline});
    f.start(0.75, true);

    f.hoverAt(6.0, 9.0);
    const int detailed = f.previewCount(RS2::EntitySpline);
    CHECK(detailed == 1);

    // the same request commits the same pieces
    f.clickAt(6.0, 9.0);
    CHECK(f.liveCount(RS2::EntitySpline) == 1 + detailed);

    // A MaxPreview of zero, or a malformed negative value: a box, from entities
    // the preview never adopts.
    for (const int maxPreview : {0, -5}) {
        OffsetFixture g;
        RS_Spline* s = g.addSCurve();
        g.select({s});
        LC_SET_ONE("Appearance", "MaxPreview", maxPreview);
        g.start(0.75, true);
        g.hoverAt(6.0, 9.0);
        INFO("MaxPreview " << maxPreview);
        CHECK(g.previewCount(RS2::EntitySpline) == 0);
        CHECK(g.previewCount(RS2::EntityRefLine) == 4);
    }
}

TEST_CASE("With current attributes, the preview has the pen the commit gives", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    spline->setPen(RS_Pen{RS_Color{10, 120, 200}, RS2::Width04, RS2::DotLine});
    const RS_Pen active{RS_Color{200, 20, 20}, RS2::Width13, RS2::DashLine};
    f.m_graphic.setActivePen(active);
    f.select({spline});
    f.start(0.75, true);
    f.m_action->setUseCurrentAttributes(true);
    f.hoverAt(6.0, 9.0);
    int previewed = 0;
    for (const RS_Entity* e : *f.m_action->m_preview) {
        CHECK(e->getPen(false) == active);
        ++previewed;
    }
    CHECK(previewed == 1);

    f.clickAt(6.0, 9.0);
    for (const RS_Entity* e : f.m_graphic) {
        if (e != spline && !e->isDeleted()) {
            CHECK(e->getPen(false) == active);
        }
    }
}

TEST_CASE("A failed preview request leaves no stale preview", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    f.select({spline});
    f.start(0.75, true);
    f.hoverAt(6.0, 9.0);
    REQUIRE(f.previewCount(RS2::EntitySpline) == 1);
    // on the curve: no side, so no offset now
    LC_CurveJet jet;
    REQUIRE(spline->tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, jet));
    f.hoverAt(jet.point.x, jet.point.y);
    CHECK(f.previewCount(RS2::EntitySpline) == 0);
}

TEST_CASE("Without additive selection every offset, and every failed source, stays selected", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Circle* inner = f.add(new RS_Circle(&f.m_graphic, RS_CircleData{RS_Vector{0.0, 3.0}, 20.0}));
    RS_Circle* outer = f.add(new RS_Circle(&f.m_graphic, RS_CircleData{RS_Vector{0.0, 3.0}, 25.0}));
    RS_Spline* arc = f.addCollapsingArc(); // shrinks to a point at 0.5 inside
    f.select({inner, outer, arc});
    // one select() per source cleared the others' results when selection is not additive
    struct Additivity {
        Additivity() { LC_SET_ONE("Selection", "Additivity", false); }
        ~Additivity() { LC_SET_ONE("Selection", "Additivity", true); }
    } off;
    f.start(0.5, true);
    f.clickAt(0.0, 3.0);

    CHECK_FALSE(inner->isSelected());
    CHECK_FALSE(outer->isSelected());
    CHECK(arc->isSelected());
    int selectedOffsets = 0;
    for (const RS_Entity* e : f.m_graphic) {
        if (e->rtti() == RS2::EntityCircle && e != inner && e != outer) {
            selectedOffsets += e->isSelected() ? 1 : 0;
        }
    }
    CHECK(selectedOffsets == 2);
}

TEST_CASE("Parallel Through previews the whole offset of a parabola", "[curve-offset][action]") {
    OffsetFixture f;
    auto* parabola = f.add(new LC_Parabola(&f.m_graphic, LC_ParabolaData{std::array<RS_Vector, 3>{
                                                             RS_Vector{-4.0, 4.0}, RS_Vector{0.0, -4.0},
                                                             RS_Vector{4.0, 4.0}}}));
    // what the command will create through (0, -1)
    QList<RS_Entity*> created;
    RS_Creation::createParallelThrough(RS_Vector{0.0, -1.0}, 1, parabola, false, false, created);
    const qsizetype pieces = created.size();
    REQUIRE(pieces == 1);
    const auto* offset = static_cast<const RS_Spline*>(created.front());
    // more spans than a list of 32-segment splines fits in the preview limit of 100
    REQUIRE(offset->getNumberOfControlPoints() > 4 * 3);
    const size_t points = offset->getNumberOfControlPoints();
    qDeleteAll(created);

    ParallelThroughProbe action(&f.m_context);
    action.m_entity = parabola;
    const LC_MouseEvent e = eventAt(0.0, -1.0);
    action.onMouseMoveEvent(ParallelThroughProbe::SetPos, &e);
    int previewed = 0;
    for (const RS_Entity* entity : *action.m_preview) {
        if (entity->rtti() == RS2::EntitySpline) {
            ++previewed;
            CHECK(static_cast<const RS_Spline*>(entity)->getNumberOfControlPoints() == points);
        }
    }
    CHECK(previewed == pieces);
}

TEST_CASE("Parallel Through takes a spline, not a line it is drawn with, and passes through the point",
          "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    ParallelThroughProbe action(&f.m_context);
    const LC_MouseEvent hover = eventAt(0.2, 0.1); // beside its first drawn line
    action.onMouseMoveEvent(ParallelThroughProbe::SetEntity, &hover);
    CHECK(action.m_entity == spline);

    // through (6, 0.5), measured from the curve: the parallel passes through it
    const RS_Vector through{6.0, 0.5};
    QList<RS_Entity*> created;
    RS_Creation::createParallelThrough(through, 1, spline, false, false, created);
    REQUIRE_FALSE(created.empty());
    double nearest = RS_MAXDOUBLE;
    for (const RS_Entity* e : created) {
        REQUIRE(e->rtti() == RS2::EntitySpline);
        const auto* piece = static_cast<const RS_Spline*>(e);
        double t0 = 0.0;
        double t1 = 0.0;
        REQUIRE(piece->getParameterDomain(t0, t1));
        const auto distanceAt = [&](const double t) {
            LC_CurveJet jet;
            REQUIRE(piece->tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet));
            return jet.point.distanceTo(through);
        };
        // sample, then narrow down around the nearest sample
        constexpr int samples = 4000;
        int best = 0;
        for (int k = 1; k <= samples; ++k) {
            if (distanceAt(t0 + (t1 - t0) * k / samples) < distanceAt(t0 + (t1 - t0) * best / samples)) {
                best = k;
            }
        }
        double lo = t0 + (t1 - t0) * std::max(best - 1, 0) / samples;
        double hi = t0 + (t1 - t0) * std::min(best + 1, samples) / samples;
        for (int i = 0; i < 200; ++i) {
            const double a = lo + (hi - lo) / 3.0;
            const double b = hi - (hi - lo) / 3.0;
            if (distanceAt(a) < distanceAt(b)) {
                hi = b;
            } else {
                lo = a;
            }
        }
        nearest = std::min(nearest, distanceAt(0.5 * (lo + hi)));
    }
    qDeleteAll(created);
    CHECK(nearest < 1e-4);
}

TEST_CASE("A spline shrunk away keeps its source, and the message says why", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* ring = f.addRing();
    f.select({ring});
    f.start(20.0, false);
    f.clickAt(0.0, 0.0);

    CHECK_FALSE(ring->isDeleted());
    CHECK(f.liveCount(RS2::EntitySpline) == 1);
    REQUIRE(f.m_context.messages.size() == 1);
    CHECK(f.m_context.messages.front().contains("1 of 1"));
    CHECK(f.m_context.messages.front().contains("nothing is left at this distance"));
}

TEST_CASE("Copies that stop short keep the source, and the message says how many fit", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* ring = f.addRing();
    f.select({ring});
    f.start(4.0, false);
    f.m_action->setUseMultipleCopies(true);
    f.m_action->setCopiesNumber(3);
    f.clickAt(0.0, 0.0); // 4 and 8 inwards exist; 12 is past every radius of curvature

    CHECK_FALSE(ring->isDeleted());
    CHECK(f.liveCount(RS2::EntitySpline) == 3);
    REQUIRE(f.m_context.messages.size() == 1);
    CHECK(f.m_context.messages.front().contains("Only 2 of 3 copies fit"));
}

TEST_CASE("A spline refused by the engine is reported with the reason", "[curve-offset][action]") {
    OffsetFixture f;
    RS_Spline* spline = f.addSCurve();
    f.select({spline});
    f.start(0.5, false);
    LC_CurveJet jet;
    REQUIRE(spline->tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, jet));
    f.clickAt(jet.point.x, jet.point.y); // on the curve: no side

    CHECK_FALSE(spline->isDeleted());
    REQUIRE(f.m_context.messages.size() == 1);
    CHECK(f.m_context.messages.front().contains("no side"));
}


namespace {
/** A spline as Draw > Spline makes it: degree 3, a control point per click. */
RS_Spline* addDrawnSpline(OffsetFixture& f) {
    auto* spline = f.add(new RS_Spline(&f.m_graphic, RS_SplineData(3, false)));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{10, 15}, RS_Vector{25, -5}, RS_Vector{40, 10},
                               RS_Vector{55, 0}}) {
        spline->addControlPoint(p);
    }
    spline->update();
    return spline;
}

/** The two-click Offset, as the options the tool saves by default have it. */
void startTwoClick(OffsetFixture& f) {
    f.m_action = std::make_unique<OffsetProbe>(&f.m_context);
    f.m_action->setDistanceFixed(false);
    f.m_action->setDistance(10.0);
    f.m_action->setKeepOriginals(false);
    f.m_action->setUseMultipleCopies(false);
    f.m_action->init(OffsetProbe::SetReferencePoint);
}

/** The only live spline of the drawing that is not @p source, drawn. */
RS_Spline* offsetOf(OffsetFixture& f, const RS_Spline* source) {
    RS_Spline* found = nullptr;
    int count = 0;
    for (RS_Entity* e : f.m_graphic) {
        if (!e->isDeleted() && e->rtti() == RS2::EntitySpline && e != source) {
            found = static_cast<RS_Spline*>(e);
            ++count;
        }
    }
    REQUIRE(count == 1);
    REQUIRE(found->count() > 0); // drawn
    return found;
}
} // namespace

TEST_CASE("A reference point on the spline takes the offset's side from the second click",
          "[curve-offset][action]") {
    // With the distance taken from two clicks, the side comes from the first,
    // the reference point, and the natural first click is on the spline, where
    // snapping puts it. A point on the curve gives no side, so nothing was
    // offset, the preview stayed empty and one line in the command widget said
    // why: the tool looked as if it did nothing. The second click shows the
    // side, and decides it then.
    OffsetFixture f;
    RS_Spline* spline = addDrawnSpline(f);
    f.select({spline});
    startTwoClick(f);

    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    LC_CurveJet middle;
    REQUIRE(spline->tryEvaluateJet(0.5 * (t0 + t1), LC_CurveEvaluationSide::Interior, middle));
    const RS_Vector normal = RS_Vector{-middle.first.y, middle.first.x} / middle.first.magnitude();

    for (const double side : {1.0, -1.0}) {
        DYNAMIC_SECTION("towards " << (side > 0 ? "the left" : "the right")) {
            const RS_Vector reference = middle.point; // on the curve
            const RS_Vector position = middle.point + normal * (5.0 * side);
            const LC_MouseEvent first = eventAt(reference.x, reference.y);
            f.m_action->onMouseLeftButtonReleaseSelected(OffsetProbe::SetReferencePoint, &first);
            REQUIRE(f.m_action->getStatus() != OffsetProbe::SetReferencePoint);

            // the preview follows the pointer
            f.m_action->deletePreviewAndHighlights();
            const LC_MouseEvent move = eventAt(position.x, position.y);
            f.m_action->onMouseMoveEventSelected(f.m_action->getStatus(), &move);
            CHECK(f.previewCount(RS2::EntitySpline) > 0);

            const LC_MouseEvent second = eventAt(position.x, position.y);
            f.m_action->onMouseLeftButtonReleaseSelected(f.m_action->getStatus(), &second);

            CHECK(f.m_context.messages.isEmpty());
            CHECK(spline->isDeleted()); // replaced, as the tool keeps no originals by default
            const RS_Spline* offset = offsetOf(f, spline);
            // 5 from the curve, on the side of the second click
            double toWanted = 0.0;
            double toOther = 0.0;
            offset->getNearestPointOnEntity(middle.point + normal * (5.0 * side), true, &toWanted);
            offset->getNearestPointOnEntity(middle.point - normal * (5.0 * side), true, &toOther);
            CHECK(toWanted < 1e-3);
            CHECK(toOther > 9.0);
        }
    }
}

TEST_CASE("A reference point off the spline still decides the offset's side", "[curve-offset][action]") {
    // Only a reference point on the curve gives up its say: one beside it
    // keeps the side, wherever the second click is.
    OffsetFixture f;
    RS_Spline* spline = addDrawnSpline(f);
    f.select({spline});
    startTwoClick(f);

    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    LC_CurveJet middle;
    REQUIRE(spline->tryEvaluateJet(0.5 * (t0 + t1), LC_CurveEvaluationSide::Interior, middle));
    const RS_Vector normal = RS_Vector{-middle.first.y, middle.first.x} / middle.first.magnitude();

    const RS_Vector reference = middle.point + normal * 1.0;  // left of the curve
    const RS_Vector position = middle.point - normal * 4.0;   // across it, 5 away
    const LC_MouseEvent first = eventAt(reference.x, reference.y);
    f.m_action->onMouseLeftButtonReleaseSelected(OffsetProbe::SetReferencePoint, &first);
    const LC_MouseEvent move = eventAt(position.x, position.y);
    f.m_action->onMouseMoveEventSelected(f.m_action->getStatus(), &move);
    const LC_MouseEvent second = eventAt(position.x, position.y);
    f.m_action->onMouseLeftButtonReleaseSelected(f.m_action->getStatus(), &second);

    const RS_Spline* offset = offsetOf(f, spline);
    double toLeft = 0.0;
    offset->getNearestPointOnEntity(middle.point + normal * 5.0, true, &toLeft);
    CHECK(toLeft < 1e-3);
}


TEST_CASE("A reference point on a spline through points takes the side from the second click",
          "[curve-offset][action]") {
    OffsetFixture f;
    LC_SplinePoints* spline = f.addSplinePoints();
    f.select({spline});
    startTwoClick(f);

    // (27, 3) is one of its points, so on the curve; (27, -2) is 5 below it
    const LC_MouseEvent first = eventAt(27.0, 3.0);
    f.m_action->onMouseLeftButtonReleaseSelected(OffsetProbe::SetReferencePoint, &first);
    const LC_MouseEvent move = eventAt(27.0, -2.0);
    f.m_action->onMouseMoveEventSelected(f.m_action->getStatus(), &move);
    const LC_MouseEvent second = eventAt(27.0, -2.0);
    f.m_action->onMouseLeftButtonReleaseSelected(f.m_action->getStatus(), &second);

    CHECK(f.m_context.messages.isEmpty());
    CHECK(spline->isDeleted());
    int offsets = 0;
    for (const RS_Entity* e : f.m_graphic) {
        offsets += !e->isDeleted() && e != spline ? 1 : 0;
    }
    CHECK(offsets > 0);
}
