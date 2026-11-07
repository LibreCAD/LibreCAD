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

// Issue #3084: while a modify tool still asks for its selection, CTRL+LMB
// is offered as the mouse's Enter. It must complete the selection that is
// there, not pick (and so toggle off, or replace the selection with) what
// lies under the cursor.

#include <catch2/catch_test_macros.hpp>

#include "lc_action_modify_delete.h"
#include "lc_action_modify_move.h"
#include "lc_actiontestsupport.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

template <typename Base>
class ClickingAction final : public Base {
public:
    explicit ClickingAction(LC_ActionContext* context) : Base(context) {}

    void click(const RS_Vector& point, const bool control) {
        LC_MouseEvent event = lc::test::eventAt(point.x, point.y);
        event.isControl = control;
        this->onMouseLeftButtonRelease(this->getStatus(), &event);
    }
    bool selectionComplete() const { return this->m_selectionComplete; }
};

using MoveAction = ClickingAction<LC_ActionModifyMove>;
using DeleteAction = ClickingAction<LC_ActionModifyDelete>;

// sets the option for one test; the settings exist once the application does
struct Additivity {
    const bool m_qtReady{lc::test::application() != nullptr};

    explicit Additivity(const bool additive) { LC_SET_ONE("Selection", "Additivity", additive); }
    ~Additivity() { LC_SET_ONE("Selection", "Additivity", true); }
};

// a line along y = 0 from x = 0 to 4 and a circle of radius 2 about (10, 0)
template <typename Action>
struct Drawing : lc::test::ActionFixture<Action> {
    RS_Line* line;
    RS_Circle* circle;

    Drawing() {
        line = new RS_Line(&this->m_graphic, RS_LineData(RS_Vector{0., 0.}, RS_Vector{4., 0.}));
        circle = new RS_Circle(&this->m_graphic, RS_CircleData(RS_Vector{10., 0.}, 2.));
        this->m_graphic.addEntity(line);
        this->m_graphic.addEntity(circle);
        this->m_action->init(0); // nothing selected yet
    }

    const RS_Vector onLine{2., 0.};
    const RS_Vector onCircle{12., 0.};
    const RS_Vector nowhere{50., 50.};
};

} // namespace

TEST_CASE("CTRL+click completes the selection a modify tool has", "[selection][3084]") {
    for (const bool additive : {true, false}) {
        CAPTURE(additive);
        const Additivity setting(additive);
        Drawing<MoveAction> f;
        MoveAction& action = *f.m_action;

        SECTION("over a selected entity it is not deselected") {
            f.m_graphic.select(QList<RS_Entity*>{f.line, f.circle});
            action.click(f.onLine, true);
            CHECK(action.selectionComplete());
            CHECK(f.line->isSelected());
            CHECK(f.circle->isSelected());
        }
        SECTION("over another entity it is not picked, nor the selection replaced") {
            f.m_graphic.select(QList<RS_Entity*>{f.line});
            action.click(f.onCircle, true);
            CHECK(action.selectionComplete());
            CHECK(f.line->isSelected());
            CHECK_FALSE(f.circle->isSelected());
        }
        SECTION("over nothing") {
            f.m_graphic.select(QList<RS_Entity*>{f.line, f.circle});
            action.click(f.nowhere, true);
            CHECK(action.selectionComplete());
            CHECK(f.line->isSelected());
            CHECK(f.circle->isSelected());
        }
    }
}

TEST_CASE("CTRL+click picks the entity under the cursor when nothing is selected", "[selection][3084]") {
    for (const bool additive : {true, false}) {
        CAPTURE(additive);
        const Additivity setting(additive);
        Drawing<MoveAction> f;
        f.m_action->click(f.onCircle, true);
        CHECK(f.m_action->selectionComplete());
        CHECK(f.circle->isSelected());
        CHECK_FALSE(f.line->isSelected());
    }
}

TEST_CASE("A click without CTRL still toggles the entity and keeps selecting", "[selection][3084]") {
    const Additivity setting(true);
    Drawing<MoveAction> f;
    f.m_graphic.select(QList<RS_Entity*>{f.line, f.circle});
    f.m_action->click(f.onLine, false);
    CHECK_FALSE(f.m_action->selectionComplete());
    CHECK_FALSE(f.line->isSelected());
    CHECK(f.circle->isSelected());
}

TEST_CASE("Delete with CTRL+click removes the selected entities, not the other ones", "[selection][3084]") {
    for (const bool additive : {true, false}) {
        CAPTURE(additive);
        const Additivity setting(additive);
        Drawing<DeleteAction> f;
        f.m_graphic.select(QList<RS_Entity*>{f.line, f.circle});
        f.m_action->click(f.onLine, true);
        CHECK(f.line->isDeleted());
        CHECK(f.circle->isDeleted());
    }
}

TEST_CASE("Delete with CTRL+click on an entity deletes it at once when nothing is selected", "[selection][3084]") {
    Drawing<DeleteAction> f;
    f.m_action->click(f.onCircle, true);
    CHECK(f.circle->isDeleted());
    CHECK_FALSE(f.line->isDeleted());
}
