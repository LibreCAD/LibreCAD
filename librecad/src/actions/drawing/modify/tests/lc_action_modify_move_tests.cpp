/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include "lc_action_modify_move.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "rs_line.h"

namespace {

class MoveAction final : public LC_ActionModifyMove {
public:
    explicit MoveAction(LC_ActionContext* context) : LC_ActionModifyMove(context) {}
    using LC_ActionModifyMove::SetTargetPoint;

    void click(const RS_Vector& point, const bool control = false) {
        LC_MouseEvent event = lc::test::eventAt(point.x, point.y);
        event.isControl = control;
        onMouseLeftButtonRelease(getStatus(), &event);
    }
};

struct MoveFixture : lc::test::ActionFixture<MoveAction> {
    RS_Line* source;

    MoveFixture() {
        source = new RS_Line(&m_graphic, RS_LineData(RS_Vector{2., 3.}, RS_Vector{5., 7.}));
        m_graphic.addEntity(source);
        m_graphic.select(QList<RS_Entity*>{source});
        m_action->init(0);
        m_action->setKeepOriginals(false);
        m_action->setUseMultipleCopies(false);
        m_action->setCopiesNumber(1);
        m_action->setUseCurrentLayer(false);
        m_action->setUseCurrentAttributes(false);
        m_action->click({2., 3.});
    }
};

} // namespace

TEST_CASE("Move/Copy preserves selection when started from an entity context menu", "[modify][move][3083]") {
    for (const bool selected : {false, true}) {
        CAPTURE(selected);
        lc::test::ActionFixture<MoveAction> f;
        auto* source = new RS_Line(&f.m_graphic, RS_LineData(RS_Vector{2., 3.}, RS_Vector{5., 7.}));
        f.m_graphic.addEntity(source);
        if (selected) f.m_graphic.select(QList<RS_Entity*>{source});
        f.m_context.saveContextMenuActionContext(source, source->getStartpoint(), false);
        f.m_action->init(0);

        CHECK(source->isSelected());
        CHECK(f.m_graphic.hasSelection());
        CHECK_FALSE(f.m_action->isFinished());
        f.m_action->click(source->getStartpoint());
        CHECK(f.m_action->getStatus() == MoveAction::SetTargetPoint);
        CHECK(source->isSelected());
    }
}

TEST_CASE("Move/Copy keeps Ctrl-copy active for repeated target clicks", "[modify][move][3080]") {
    MoveFixture f;
    MoveAction& action = *f.m_action;
    REQUIRE(action.getStatus() == MoveAction::SetTargetPoint);
    unsigned copies = 0;
    for (const RS_Vector target : {RS_Vector{12., 8.}, RS_Vector{22., 3.}, RS_Vector{-8., 13.}}) {
        action.click(target, true);
        REQUIRE_FALSE(action.isFinished());
        CHECK(action.getStatus() == MoveAction::SetTargetPoint);
        ++copies;
        REQUIRE(f.m_graphic.count() == copies + 1);
        const RS_Entity* copy = f.m_graphic.lastEntity();
        CHECK(copy->getStartpoint().distanceTo(target) < 1e-9);
        CHECK(copy->getEndpoint().distanceTo(target + RS_Vector{3., 4.}) < 1e-9);
        CHECK_FALSE(f.source->isDeleted());
        CHECK(f.source->getStartpoint().distanceTo(RS_Vector{2., 3.}) < 1e-9);
        CHECK_FALSE(action.isKeepOriginals());
        CHECK(f.m_view.getViewPort()->getRelativeZero().distanceTo(RS_Vector{2., 3.}) < 1e-9);
    }

    const RS_Entity* lastCopy = f.m_graphic.lastEntity();
    REQUIRE(f.m_graphic.undo());
    CHECK(lastCopy->isDeleted());
    CHECK_FALSE(f.source->isDeleted());
    REQUIRE(f.m_graphic.redo());
    CHECK_FALSE(lastCopy->isDeleted());
    CHECK(lastCopy->getStartpoint().distanceTo(RS_Vector{-8., 13.}) < 1e-9);
}

TEST_CASE("Move/Copy returns to moving when a target click no longer holds Ctrl", "[modify][move][3080]") {
    MoveFixture f;
    MoveAction& action = *f.m_action;
    action.click({12., 3.}, true);
    REQUIRE_FALSE(action.isFinished());
    REQUIRE(f.m_graphic.count() == 2);
    const RS_Entity* copy = f.m_graphic.lastEntity();

    action.click({22., 3.});
    CHECK(action.isFinished());
    REQUIRE(f.m_graphic.count() == 3);
    CHECK(f.source->isDeleted());
    CHECK_FALSE(copy->isDeleted());
    CHECK(copy->getStartpoint().distanceTo(RS_Vector{12., 3.}) < 1e-9);
    CHECK(f.m_graphic.lastEntity()->getStartpoint().distanceTo(RS_Vector{22., 3.}) < 1e-9);
}

TEST_CASE("Move/Copy without Ctrl still finishes after one target click", "[modify][move][3080]") {
    MoveFixture f;
    bool keepOriginals = false;
    SECTION("move") {}
    SECTION("copy through the tool options") { keepOriginals = true; }
    f.m_action->setKeepOriginals(keepOriginals);
    f.m_action->click({12., 3.});

    CHECK(f.m_action->isFinished());
    REQUIRE(f.m_graphic.count() == 2);
    CHECK(f.source->isDeleted() == !keepOriginals);
    CHECK(f.m_graphic.lastEntity()->getStartpoint().distanceTo(RS_Vector{12., 3.}) < 1e-9);
}
