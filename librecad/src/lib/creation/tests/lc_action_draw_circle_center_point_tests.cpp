/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

// Circle, Center Point (issue #2985): Shift snaps the point on the circle to the relative zero, unless that
// is at the center.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lc_action_draw_circle_center_point.h"
#include "lc_actiontestsupport.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_modifiersinfo.h"
#include "lc_relative_point_data.h"
#include "rs_circle.h"

namespace {

struct CenterPoint final : LC_ActionDrawCircleCenterPoint {
    explicit CenterPoint(LC_ActionContext* context) : LC_ActionDrawCircleCenterPoint(context) {}
    using RS_PreviewActionInterface::m_lastMouseMoveEvent;
    using RS_Snapper::m_infoCursorOverlayData;

    void move(const double x, const double y, const bool shift = false) {
        LC_MouseEvent event = lc::test::eventAt(x, y);
        event.graphPoint = RS_Vector{x + 1., y + 1.}; // the mouse is not at the snap point
        event.isShift = shift;
        onMouseMoveEvent(getStatus(), &event);
    }
    void click(const double x, const double y, const bool shift = false) {
        LC_MouseEvent event = lc::test::eventAt(x, y);
        event.graphPoint = RS_Vector{x + 1., y + 1.}; // the mouse is not at the snap point
        event.isShift = shift;
        onMouseLeftButtonRelease(getStatus(), &event);
    }
    const RS_Circle* previewed() const { return dynamic_cast<const RS_Circle*>(m_preview->firstEntity()); }
};

// The center (0, 0) is taken. The relative zero is at `relZero` if it is locked, else at the center.
struct CenterTaken : lc::test::ActionFixture<CenterPoint> {
    CenterTaken(const RS_Vector& relZero, const bool locked) {
        m_view.getInfoCursorOverlayPreferences()->showCommandPrompt = true; // the prompt goes to the info cursor
        m_view.getViewPort()->setRelativeZero(relZero);
        m_view.getViewPort()->lockRelativeZero(locked);
        m_action->init(0);
        m_action->click(0., 0.);
    }
    const RS_Circle* drawn() const { return dynamic_cast<const RS_Circle*>(m_graphic.lastEntity()); }
};

// The mouse is at (30, 40), 50 from the center: the radius of the preview and of the circle that a click draws.
void checkRadius(const RS_Vector& relZero, const bool locked, const bool shift, const double radius) {
    CenterTaken f(relZero, locked);
    f.m_action->move(30., 40., shift);
    REQUIRE(f.m_action->previewed() != nullptr);
    CHECK(f.m_action->previewed()->getRadius() == Catch::Approx(radius));
    f.m_action->click(30., 40., shift);
    REQUIRE(f.drawn() != nullptr);
    CHECK(f.drawn()->getCenter().distanceTo(RS_Vector{0., 0.}) < 1e-9);
    CHECK(f.drawn()->getRadius() == Catch::Approx(radius));
}

// Whether the prompt of the radius state names Shift for the relative zero.
bool hintsRelativeZero(const RS_Vector& relZero, const bool locked) {
    CenterTaken f(relZero, locked);
    return f.m_action->m_infoCursorOverlayData->getZone4().contains(LC_ModifiersInfo::SHIFT_RELATIVE_ZERO().getShiftMessage());
}

} // namespace

TEST_CASE("Circle Center Point: Shift snaps the point on the circle to a locked relative zero", "[circle-center-point]") {
    // (100, 0) is 100 from the center
    SECTION("with Shift") { checkRadius({100., 0.}, true, true, 100.); }
    SECTION("without Shift") { checkRadius({100., 0.}, true, false, 50.); }
    SECTION("at the center, which is no point on a circle") { checkRadius({100., 0.}, false, true, 50.); }
    SECTION("without a relative zero") { checkRadius(RS_Vector{false}, true, true, 50.); }
}

TEST_CASE("Circle Center Point: a radius typed in the relative input assistant is not replaced by the relative zero",
          "[circle-center-point]") {
    // the mouse does not move while the assistant is open: the last mouse move, with its Shift, is replayed
    CenterTaken f({100., 0.}, true);
    f.m_action->m_lastMouseMoveEvent.isShift = true;
    LC_RelativePositionData typed;
    typed.wcsProjection = RS_Vector{25., 0.};

    SECTION("the preview") {
        f.m_action->moveMouseToRefreshPreview(typed.wcsProjection);
        REQUIRE(f.m_action->previewed() != nullptr);
        CHECK(f.m_action->previewed()->getRadius() == Catch::Approx(25.));
    }
    SECTION("the circle") {
        f.m_action->addProjectedRelativePointToVisualSnap(&typed, true);
        REQUIRE(f.drawn() != nullptr);
        CHECK(f.drawn()->getRadius() == Catch::Approx(25.));
    }
}

TEST_CASE("Circle Center Point: the prompt names Shift for the point on the circle where it applies", "[circle-center-point]") {
    CHECK(hintsRelativeZero({100., 0.}, true));
    CHECK_FALSE(hintsRelativeZero({100., 0.}, false)); // at the center
    CHECK_FALSE(hintsRelativeZero(RS_Vector{false}, true));
}
