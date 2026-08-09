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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Regression tests for the LC_ActionDrawLineParallel left-click guard
// (PR #2710, follow-up to the #2708 null-entity crash fixed in #2715).
//
// The RS_Creation-level null guard (rs_creation_null_entity_tests.cpp) stops
// the crash, but an unguarded trigger() still opens an undo cycle before the
// modification batch is known to be empty.  RS_Undo::startUndoCycle prunes
// everything past m_redoPointer at cycle START, so a no-op click over empty
// space silently destroys the user's redo history.  The action-level guard is
// what protects that; these tests pin it.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QApplication>

#include "lc_action_draw_line_parallel.h"
#include "lc_actioncontext.h"
#include "lc_undosection.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

/**
 * Returns a QApplication, reusing the process-wide one if another test built it
 * first. The pointer is deliberately leaked: only one QApplication may exist at
 * a time and only one ~QApplication may run at exit, so every test file uses
 * this reuse-or-create form and none of them destroys the instance.
 */
QApplication* application() {
    static int argc = 1;
    static char name[] = "librecad_tests";
    static char* argv[] = {name, nullptr};
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
    return app;
}

class ParallelActionTestView final : public RS_GraphicView {
public:
    ParallelActionTestView() : RS_GraphicView(nullptr) {}

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

/**
 * The action caches the hovered entity (m_entity, private) in
 * onMouseMoveEvent, and onMouseLeftButtonRelease acts on that cache, so the
 * probe only needs to expose the two protected mouse handlers and drive them
 * the way RS_PreviewActionInterface's event dispatcher would.
 */
class ParallelActionProbe final : public LC_ActionDrawLineParallel {
public:
    explicit ParallelActionProbe(LC_ActionContext* actionContext)
        : LC_ActionDrawLineParallel(actionContext, RS2::ActionDrawLineParallel) {}

    using LC_ActionDrawLineParallel::onMouseMoveEvent;
    using LC_ActionDrawLineParallel::onMouseLeftButtonRelease;
};

// LC_ActionDrawLineParallel's Status enum is private; its first enumerator is
// pinned to RS_ActionInterface::InitialActionStatus (== 0) by the framework
// contract, and SetNumber is the next one.  Any non-SetEntity value must hit
// the release handler's default arm, so a stale SetNumber value would still
// exercise the guarded branch.
constexpr int STATUS_SET_ENTITY = 0;
constexpr int STATUS_SET_NUMBER = 1;

LC_MouseEvent eventAt(const double x, const double y) {
    LC_MouseEvent e;
    e.graphPoint = RS_Vector{x, y};
    e.snapPoint = RS_Vector{x, y};
    return e;
}

/**
 * Member order matters: the action is destroyed before the view, because
 * ~RS_PreviewActionInterface reaches into overlay containers the view owns.
 */
struct ParallelActionFixture {
    // Declared first on purpose: members are initialised before the constructor
    // body, and RS_Graphic's own constructor already reads RS_Settings.
    const bool m_qtReady{application() != nullptr};
    RS_Graphic m_graphic;
    ParallelActionTestView m_view;
    LC_ActionContext m_context;
    std::unique_ptr<ParallelActionProbe> m_action;

    ParallelActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<ParallelActionProbe>(&m_context);
    }

    /// One committed modification, then Ctrl+Z: leaves exactly one redoable cycle.
    void makeRedoPending() {
        {
            LC_UndoSection undo{&m_graphic, m_view.getViewPort()};
            undo.undoableExecute([this](LC_DocumentModificationBatch& ctx) -> bool {
                ctx.entitiesToAdd.append(
                    new RS_Line(&m_graphic, RS_Vector{0.0, 20.0}, RS_Vector{10.0, 20.0}));
                return true;
            });
        }
        REQUIRE(m_graphic.undo());
        bool undoAvailable = false;
        bool redoAvailable = false;
        m_graphic.collectUndoState(undoAvailable, redoAvailable);
        REQUIRE(redoAvailable);
    }

    /// The hover target for the catch tests: a plain (non-undoable) line.
    RS_Line* addCatchableLine() {
        auto* line = new RS_Line(&m_graphic, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
        m_graphic.addEntity(line);
        return line;
    }

    bool redoAvailable() {
        bool undoState = false;
        bool redoState = false;
        m_graphic.collectUndoState(undoState, redoState);
        return redoState;
    }

    bool undoAvailable() {
        bool undoState = false;
        bool redoState = false;
        m_graphic.collectUndoState(undoState, redoState);
        return undoState;
    }
};

} // namespace

TEST_CASE("parallel tool: click over empty space must not wipe redo history",
          "[parallel_action][null]") {
    ParallelActionFixture f;
    f.makeRedoPending();
    const unsigned countBefore = f.m_graphic.count();

    // The #2708 gesture: hover over empty space (no entity caught), click.
    LC_MouseEvent moveEvent = eventAt(100.0, 100.0);
    f.m_action->onMouseMoveEvent(STATUS_SET_ENTITY, &moveEvent);
    LC_MouseEvent clickEvent = eventAt(100.0, 100.0);
    f.m_action->onMouseLeftButtonRelease(STATUS_SET_ENTITY, &clickEvent);

    CHECK(f.m_graphic.count() == countBefore);  // nothing created
    CHECK(f.redoAvailable());                   // redo survived the no-op click
    CHECK(f.m_graphic.redo());                  // and it still replays
}

TEST_CASE("parallel tool: click while entering the count must not create parallels",
          "[parallel_action][status]") {
    ParallelActionFixture f;
    f.addCatchableLine();
    const unsigned countBefore = f.m_graphic.count();

    // Hover catches the line (m_entity != nullptr) ...
    LC_MouseEvent moveEvent = eventAt(5.0, 0.25);
    f.m_action->onMouseMoveEvent(STATUS_SET_ENTITY, &moveEvent);
    // ... then the user types "number" and clicks while in SetNumber.
    LC_MouseEvent clickEvent = eventAt(5.0, 0.25);
    f.m_action->onMouseLeftButtonRelease(STATUS_SET_NUMBER, &clickEvent);

    CHECK(f.m_graphic.count() == countBefore);  // no un-previewed parallel
    CHECK_FALSE(f.undoAvailable());             // and no undo cycle was opened
}

TEST_CASE("parallel tool: click on a caught entity still creates the parallel",
          "[parallel_action][positive]") {
    ParallelActionFixture f;
    f.addCatchableLine();
    const unsigned countBefore = f.m_graphic.count();

    LC_MouseEvent moveEvent = eventAt(5.0, 0.25);
    f.m_action->onMouseMoveEvent(STATUS_SET_ENTITY, &moveEvent);
    LC_MouseEvent clickEvent = eventAt(5.0, 0.25);
    f.m_action->onMouseLeftButtonRelease(STATUS_SET_ENTITY, &clickEvent);

    CHECK(f.m_graphic.count() == countBefore + 1);  // the parallel was created
    CHECK(f.undoAvailable());                       // undoably
    // NOTE: count() includes undone entities, so no count check after undo().
    REQUIRE(f.m_graphic.undo());
    CHECK(f.redoAvailable());
}
