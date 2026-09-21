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

#include <memory>

#include <QStringList>

#include "lc_action_modify_offset.h"
#include "lc_actiontestsupport.h"
#include "lc_splinepoints.h"
#include "rs_circle.h"
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

    /** y = x^2 on [-2, 2]: an inward offset of 0.5, its vertex radius, is singular. */
    RS_Spline* addParabola() {
        RS_SplineData d(2, false);
        d.controlPoints = {{-2, 4}, {0, -4}, {2, 4}};
        d.knotslist = {0, 0, 0, 1, 1, 1};
        d.weights.assign(3, 1.0);
        return add(new RS_Spline(&m_graphic, d));
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
    CHECK(pieces > 1);
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
    RS_Spline* spline = f.addParabola();
    f.select({spline});
    f.start(0.5, false); // inside, at the vertex radius: singular
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
    RS_Spline* spline = f.addParabola();
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
    CHECK(detailed > 1);

    // the same request commits the same pieces
    f.clickAt(6.0, 9.0);
    CHECK(f.liveCount(RS2::EntitySpline) == 1 + detailed);

    // More output than MaxPreview allows in detail, zero, or a malformed negative
    // value: a box, from entities the preview never adopts.
    for (const int maxPreview : {detailed - 1, 0, -5}) {
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
    CHECK(previewed > 1);

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
    REQUIRE(f.previewCount(RS2::EntitySpline) > 1);
    // on the curve: no side, so no offset now
    LC_CurveJet jet;
    REQUIRE(spline->tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, jet));
    f.hoverAt(jet.point.x, jet.point.y);
    CHECK(f.previewCount(RS2::EntitySpline) == 0);
}
