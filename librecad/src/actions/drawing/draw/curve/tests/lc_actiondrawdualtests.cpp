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

// The steps of Draw Dual: select the entities, then give the center the dual
// is taken about. Completing the selection must only move on to the center.
// The action used to trigger right there, with no center yet, so it found
// nothing to dualize and finished before a center could be clicked.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QKeyEvent>

#include "lc_actiondrawdual.h"
#include "lc_actiontestsupport.h"
#include "rs_circle.h"
#include "rs_commandevent.h"

namespace {

class DualProbe final : public LC_ActionDrawDual {
public:
    using LC_ActionDrawDual::LC_ActionDrawDual;
    using LC_ActionPreSelectionAwareBase::m_selectionComplete;
    using LC_ActionPreSelectionAwareBase::onMouseLeftButtonRelease;
    using RS_ActionInterface::select;
};

/**
 * A circle of radius 5 at the origin; about any point outside it, its dual is
 * a hyperbola, which arrives as two branches.
 *
 * The action runs the way the application runs one: the view's event handler
 * starts it and hands it the Return key and what is typed on the command line,
 * and drops it once it has finished. Only clicks go to the action directly.
 */
struct DualFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;
    std::shared_ptr<DualProbe> m_action;
    RS_Circle* m_circle{nullptr};

    DualFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_shared<DualProbe>(&m_context);
        m_circle = new RS_Circle(&m_graphic, RS_CircleData(RS_Vector{0.0, 0.0}, 5.0));
        m_graphic.addEntity(m_circle);
    }

    ~DualFixture() {
        m_view.beginClose();
    }

    /// @return whether the action is running once the view has started it
    bool start() const {
        return m_view.setCurrentAction(m_action);
    }

    bool isCurrent() const {
        return m_view.getCurrentAction() == m_action.get();
    }

    void click(const double x, const double y, const bool control = false) const {
        LC_MouseEvent e = lc::test::eventAt(x, y);
        e.isControl = control;
        m_action->onMouseLeftButtonRelease(m_action->getStatus(), &e);
    }

    void pressReturn() {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        m_view.keyPressEvent(&e);
    }

    void type(const QString& text) const {
        RS_CommandEvent e(text);
        m_view.commandEvent(&e);
    }

    int hyperbolas() const {
        int n = 0;
        for (const RS_Entity* e : m_graphic) {
            n += (e->rtti() == RS2::EntityHyperbola && !e->isDeleted()) ? 1 : 0;
        }
        return n;
    }
};

} // namespace

TEST_CASE("Draw Dual waits for a center after Return completes the selection", "[actions][dual]") {
    DualFixture f;
    REQUIRE(f.start());

    f.click(5.0, 0.0); // on the circle
    REQUIRE(f.m_graphic.hasSelection());
    f.pressReturn();

    CHECK(f.m_action->m_selectionComplete);
    CHECK(f.isCurrent());
    CHECK(f.hyperbolas() == 0);

    f.click(20.0, 0.0); // the center
    CHECK(f.hyperbolas() == 2);
    CHECK(f.m_action->isFinished());
}

TEST_CASE("Draw Dual started on a selection runs until its center is typed", "[actions][dual]") {
    DualFixture f;
    f.m_action->select(f.m_circle);

    CHECK(f.start());
    CHECK(f.m_action->m_selectionComplete);
    CHECK(f.isCurrent());
    CHECK(f.hyperbolas() == 0);

    f.type(QStringLiteral("20,0"));
    CHECK(f.hyperbolas() == 2);
    CHECK(f.m_action->isFinished());
    CHECK_FALSE(f.isCurrent());
}

TEST_CASE("Draw Dual ignores a center typed before the selection is complete", "[actions][dual]") {
    DualFixture f;
    REQUIRE(f.start());
    f.click(5.0, 0.0);

    f.type(QStringLiteral("20,0"));
    CHECK(f.hyperbolas() == 0);
    CHECK(f.isCurrent());

    // completing the selection must not fall back on that center
    f.pressReturn();
    CHECK(f.hyperbolas() == 0);
    CHECK(f.isCurrent());

    f.click(20.0, 0.0);
    CHECK(f.hyperbolas() == 2);
}

TEST_CASE("Draw Dual stays in selection when nothing is selected", "[actions][dual]") {
    DualFixture f;
    REQUIRE(f.start());

    // Ctrl+click off any entity asks to complete the selection
    f.click(100.0, 100.0, true);

    CHECK_FALSE(f.m_graphic.hasSelection());
    CHECK_FALSE(f.m_action->m_selectionComplete);
    CHECK_FALSE(f.m_action->isFinished());
}

TEST_CASE("Draw Dual reports its own action type", "[actions][dual]") {
    DualFixture f;
    CHECK(f.m_action->rtti() == RS2::ActionDrawDual);
}
