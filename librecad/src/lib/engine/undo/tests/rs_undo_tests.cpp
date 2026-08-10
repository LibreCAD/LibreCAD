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

// Regression tests for the RS_Undo redo-tail prune.
//
// Invariants under test: an empty (no-op) cycle leaves pending redo history
// and its entities untouched; a non-empty commit prunes the redo tail,
// keeping any undoable still referenced by a remaining cycle or by the
// cycle being committed.
//
// Engine-level on RS_Graphic directly: no actions, no QApplication.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>

#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

void ensureQtApp() {
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char *qargv[] = {qarg0, nullptr};
    static QCoreApplication *qapp = QCoreApplication::instance()
                                        ? QCoreApplication::instance()
                                        : new QCoreApplication(qargc, qargv);
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)qapp;
    (void)settingsReady;
}

// Expose the protected RS_Undo/RS_Document API for direct cycle control.
class TestGraphic : public RS_Graphic {
public:
    using RS_Document::startUndoCycle;
    using RS_Document::endUndoCycle;
    using RS_Undo::addUndoable;
    using RS_Undo::countUndoCycles;
    using RS_Undo::countRedoCycles;
};

// A line that reports its destruction, so entity frees are observable.
class TrackedLine : public RS_Line {
public:
    TrackedLine(RS_EntityContainer *parent, int *destroyCount, double y)
        : RS_Line(parent, RS_Vector(0., y), RS_Vector(10., y))
        , m_destroyCount(destroyCount) {}
    ~TrackedLine() override {
        if (m_destroyCount != nullptr) {
            ++(*m_destroyCount);
        }
    }

private:
    int *m_destroyCount = nullptr;
};

// Commit one undo cycle that adds a tracked line (mirrors undoableAdd()).
TrackedLine *commitTrackedLine(TestGraphic &g, int *destroyCount, double y) {
    auto *line = new TrackedLine(&g, destroyCount, y);
    g.startUndoCycle();
    g.addEntity(line);
    g.addUndoable(line);
    g.endUndoCycle();
    return line;
}

} // namespace

// A no-op action (open + close an empty cycle) must neither wipe pending
// redo history nor free the undone entities.
TEST_CASE("empty undo cycle preserves pending redo history",
          "[rs_undo]") {
    ensureQtApp();
    int destroyed = 0;
    TestGraphic g;

    TrackedLine *line = commitTrackedLine(g, &destroyed, 0.);
    REQUIRE(g.countUndoCycles() == 1);
    REQUIRE(g.countRedoCycles() == 0);

    REQUIRE(g.undo());
    REQUIRE(g.countUndoCycles() == 0);
    REQUIRE(g.countRedoCycles() == 1);
    CHECK(line->isDeleted());

    // the no-op click
    g.startUndoCycle();
    g.endUndoCycle();

    CHECK(g.countRedoCycles() == 1);   // redo wiped if the prune runs at cycle start
    REQUIRE(destroyed == 0);           // entity freed if the prune runs at cycle start
    REQUIRE(g.redo());
    CHECK(line->isAlive());
    CHECK(destroyed == 0);
}

TEST_CASE("repeated empty cycles keep a multi-step redo tail redoable",
          "[rs_undo]") {
    ensureQtApp();
    int d1 = 0;
    int d2 = 0;
    TestGraphic g;

    TrackedLine *l1 = commitTrackedLine(g, &d1, 0.);
    TrackedLine *l2 = commitTrackedLine(g, &d2, 5.);
    REQUIRE(g.undo());
    REQUIRE(g.undo());
    REQUIRE(g.countRedoCycles() == 2);

    for (int i = 0; i < 3; ++i) {
        g.startUndoCycle();
        g.endUndoCycle();
    }

    CHECK(g.countRedoCycles() == 2);
    REQUIRE(d1 == 0);
    REQUIRE(d2 == 0);
    REQUIRE(g.redo());
    REQUIRE(g.redo());
    CHECK(l1->isAlive());
    CHECK(l2->isAlive());
    CHECK(g.countUndoCycles() == 2);
    CHECK(g.countRedoCycles() == 0);
}

TEST_CASE("non-empty commit discards redo tail and frees obsolete entity once",
          "[rs_undo]") {
    ensureQtApp();
    int destroyedOld = 0;
    int destroyedNew = 0;
    TestGraphic g;

    REQUIRE(g.getAutoUpdateBorders());
    commitTrackedLine(g, &destroyedOld, 0.);
    REQUIRE(g.undo());
    REQUIRE(g.countRedoCycles() == 1);

    TrackedLine *fresh = commitTrackedLine(g, &destroyedNew, 5.);
    CHECK(g.countUndoCycles() == 1);
    CHECK(g.countRedoCycles() == 0);   // tail correctly discarded
    CHECK(destroyedOld == 1);          // obsolete entity freed exactly once
    CHECK(destroyedNew == 0);
    CHECK(fresh->isAlive());

    // regression probe: prune-at-commit must not leave the document with
    // border auto-updating permanently disabled
    CHECK(g.getAutoUpdateBorders());

    CHECK(!g.redo());
    REQUIRE(g.undo());                 // the new cycle still works both ways
    REQUIRE(g.redo());
    CHECK(destroyedOld == 1);
    CHECK(destroyedNew == 0);
}

TEST_CASE("partial redo tail prune keeps earlier history intact",
          "[rs_undo]") {
    ensureQtApp();
    int d1 = 0;
    int d2 = 0;
    int d3 = 0;
    int dn = 0;
    TestGraphic g;

    TrackedLine *l1 = commitTrackedLine(g, &d1, 0.);
    commitTrackedLine(g, &d2, 5.);
    commitTrackedLine(g, &d3, 10.);
    REQUIRE(g.undo());
    REQUIRE(g.undo());                 // tail = {cycle2, cycle3}
    REQUIRE(g.countUndoCycles() == 1);
    REQUIRE(g.countRedoCycles() == 2);

    commitTrackedLine(g, &dn, 20.);
    CHECK(g.countUndoCycles() == 2);
    CHECK(g.countRedoCycles() == 0);
    CHECK(d1 == 0);
    CHECK(d2 == 1);
    CHECK(d3 == 1);
    CHECK(dn == 0);
    CHECK(l1->isAlive());

    REQUIRE(g.undo());
    REQUIRE(g.undo());
    CHECK(g.countRedoCycles() == 2);
    REQUIRE(g.redo());
    REQUIRE(g.redo());
}

TEST_CASE("nested empty inner cycle does not disturb the outer commit",
          "[rs_undo]") {
    ensureQtApp();
    int destroyedOld = 0;
    int destroyedNew = 0;
    TestGraphic g;

    commitTrackedLine(g, &destroyedOld, 0.);
    REQUIRE(g.undo());
    REQUIRE(g.countRedoCycles() == 1);

    g.startUndoCycle();                // outer
    g.startUndoCycle();                // nested inner
    g.endUndoCycle();                  // inner end: no commit, no prune
    CHECK(g.countRedoCycles() == 1);   // fails if the prune runs at the outer cycle start

    auto *line = new TrackedLine(&g, &destroyedNew, 5.);
    g.addEntity(line);
    g.addUndoable(line);
    g.endUndoCycle();                  // outer end: commit + prune once

    CHECK(g.countUndoCycles() == 1);
    CHECK(g.countRedoCycles() == 0);
    CHECK(destroyedOld == 1);          // pruned exactly once
    CHECK(destroyedNew == 0);
    REQUIRE(g.undo());
    REQUIRE(g.redo());
}

// The committed cycle still references the shared undoable; freeing it in
// the prune would be a use-after-free on the next undo.
TEST_CASE("undoable in both redo tail and committed cycle is not freed",
          "[rs_undo]") {
    ensureQtApp();
    int destroyed = 0;
    TestGraphic g;

    TrackedLine *line = commitTrackedLine(g, &destroyed, 0.);
    REQUIRE(g.undo());
    REQUIRE(g.countRedoCycles() == 1);

    g.startUndoCycle();
    // a prune at cycle start would already have freed the entity here
    REQUIRE(destroyed == 0);
    g.addUndoable(line);               // same object recorded in the new cycle
    g.endUndoCycle();

    // fails if the prune's keep-set omits the cycle being committed
    REQUIRE(destroyed == 0);
    CHECK(g.countUndoCycles() == 1);
    CHECK(g.countRedoCycles() == 0);

    REQUIRE(g.undo());                 // toggles the shared undoable: alive
    CHECK(line->isAlive());
    REQUIRE(g.redo());
    CHECK(line->isDeleted());
    REQUIRE(destroyed == 0);
}

TEST_CASE("undoable shared between kept history and redo tail survives",
          "[rs_undo]") {
    ensureQtApp();
    int d = 0;
    int dn = 0;
    TestGraphic g;

    TrackedLine *line = commitTrackedLine(g, &d, 0.);   // cycle1: add line

    g.startUndoCycle();                // cycle2: delete the same line
    line->markDeleted();
    g.addUndoable(line);
    g.endUndoCycle();

    REQUIRE(g.undo());                 // undo the delete: line alive again
    CHECK(line->isAlive());
    REQUIRE(g.countRedoCycles() == 1);

    commitTrackedLine(g, &dn, 5.);     // prune: line is obsolete AND kept
    CHECK(d == 0);                     // must not be freed: cycle1 keeps it
    CHECK(line->isAlive());
    CHECK(g.countUndoCycles() == 2);   // cycle1 + new cycle; cycle2 pruned
    CHECK(g.countRedoCycles() == 0);
}

TEST_CASE("empty cycle with no pending redo is a no-op", "[rs_undo]") {
    ensureQtApp();
    TestGraphic g;

    g.startUndoCycle();
    g.endUndoCycle();
    CHECK(g.countUndoCycles() == 0);
    CHECK(g.countRedoCycles() == 0);
    CHECK(!g.undo());
    CHECK(!g.redo());
}
