/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
**********************************************************************/

// Issue #2241: a plugin that asks for a selection must finish on the Return
// key, not only on the keypad Enter.

#include <catch2/catch_test_macros.hpp>

#include <QKeyEvent>

#include "doc_plugin_interface.h"
#include "lc_actiontestsupport.h"
#include "lc_action_select_single.h"
#include "qc_actiongetselect.h"
#include "rs_line.h"
#include "rs_selection.h"

namespace {
// Stands in for the selection actions that route the finish key differently
// from LC_ActionSelectSingle: those that refuse to finish on an empty
// selection, and the pre-selection-aware modify actions, whose
// selectionFinishedByKey() runs the operation rather than ending the step.
class ProbeSelectSingle : public LC_ActionSelectSingle {
public:
    using LC_ActionSelectSingle::LC_ActionSelectSingle;

    bool m_finishedByKey = false;

protected:
    bool isAllowSelectionFinishByEnterForEmptySelection() override {return false;}

    void selectionFinishedByKey(QKeyEvent* e, bool escape) override {
        m_finishedByKey = true;
        LC_ActionSelectSingle::selectionFinishedByKey(e, escape);
    }
};
}

TEST_CASE("A plugin selection finishes on Return and on keypad Enter",
          "[plugins][selection][issue2241]") {
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        lc::test::ActionFixture<QC_ActionGetSelect> fixture;
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        fixture.m_action->keyPressEvent(&event);
        CHECK(fixture.m_action->isCompleted());
        CHECK_FALSE(fixture.m_action->wasCanceled());
    }
}

// Cancelling still has to complete the step: performSelect() spins until
// isCompleted(), and then drops the selection because it was cancelled.
TEST_CASE("Escape cancels a plugin selection instead of confirming it",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    fixture.m_action->keyPressEvent(&event);
    CHECK(fixture.m_action->isCompleted());
    CHECK(fixture.m_action->wasCanceled());
}

TEST_CASE("Escape reaches the collector through the selection step",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    auto* line = new RS_Line{&fixture.m_graphic, {{0., 0.}, {10., 10.}}};
    fixture.m_graphic.addEntity(line);
    RS_Selection(&fixture.m_view).selectSingle(line);
    REQUIRE(fixture.m_graphic.hasSelection());

    LC_ActionSelectSingle inner(&fixture.m_context, fixture.m_action.get());
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    inner.keyPressEvent(&event);

    CHECK(fixture.m_action->isCompleted());
    CHECK(fixture.m_action->wasCanceled());
}

TEST_CASE("The selection step hands both finish keys to the plugin's collector",
          "[plugins][selection][issue2241]") {
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        lc::test::ActionFixture<QC_ActionGetSelect> fixture;
        auto* line = new RS_Line{&fixture.m_graphic, {{0., 0.}, {10., 10.}}};
        fixture.m_graphic.addEntity(line);
        RS_Selection(&fixture.m_view).selectSingle(line);
        REQUIRE(fixture.m_graphic.hasSelection());

        LC_ActionSelectSingle inner(&fixture.m_context, fixture.m_action.get());
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        inner.keyPressEvent(&event);

        CHECK(fixture.m_action->isCompleted());

        // getSelected() hands the plugin what it asked for: the picked entity.
        QList<Plug_Entity*> selected;
        fixture.m_action->getSelected(&selected, nullptr);
        CHECK(selected.size() == 1);
        for (auto* entity : selected) {
            delete reinterpret_cast<Plugin_Entity*>(entity);
        }
    }
}

// The reporter of #2241 saw nothing happen "whether anything is selected or
// not": with an empty selection the step must still end, or the plugin's
// event loop keeps spinning.
TEST_CASE("The selection step ends on Return with nothing selected",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    REQUIRE_FALSE(fixture.m_graphic.hasSelection());

    LC_ActionSelectSingle inner(&fixture.m_context, fixture.m_action.get());
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    inner.keyPressEvent(&event);

    CHECK(fixture.m_action->isCompleted());
}

// Both keys must reach selectionFinishedByKey(), because that is what runs the
// operation for the pre-selection-aware modify actions that share this base.
TEST_CASE("Return and keypad Enter complete a selection by the same path",
          "[plugins][selection][issue2241]") {
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        lc::test::ActionFixture<QC_ActionGetSelect> fixture;
        auto* line = new RS_Line{&fixture.m_graphic, {{0., 0.}, {10., 10.}}};
        fixture.m_graphic.addEntity(line);
        RS_Selection(&fixture.m_view).selectSingle(line);
        REQUIRE(fixture.m_graphic.hasSelection());

        ProbeSelectSingle probe(&fixture.m_context, fixture.m_action.get());
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        probe.keyPressEvent(&event);

        CHECK(probe.m_finishedByKey);
    }
}

// What the deleted keyReleaseEvent() used to do: an action that will not finish
// on an empty selection still hands control back to its predecessor, without
// completing a selection that is not there.
TEST_CASE("With nothing selected the step returns to its predecessor",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    REQUIRE_FALSE(fixture.m_graphic.hasSelection());

    const auto predecessor = std::make_shared<QC_ActionGetSelect>(&fixture.m_context);
    ProbeSelectSingle probe(&fixture.m_context, predecessor.get());
    probe.setPredecessor(predecessor);

    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    probe.keyPressEvent(&event);

    CHECK_FALSE(probe.m_finishedByKey);
    CHECK(probe.isFinished());
    CHECK_FALSE(predecessor->isCompleted());
}
