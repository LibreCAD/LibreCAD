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

#include "lc_actiontestsupport.h"
#include "lc_action_select_single.h"
#include "qc_actiongetselect.h"
#include "rs_line.h"
#include "rs_selection.h"

TEST_CASE("A plugin selection finishes on Return and on keypad Enter",
          "[plugins][selection][issue2241]") {
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        lc::test::ActionFixture<QC_ActionGetSelect> fixture;
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        fixture.m_action->keyPressEvent(&event);
        CHECK(fixture.m_action->isCompleted());
    }
}

TEST_CASE("Escape cancels a plugin selection instead of confirming it",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    fixture.m_action->keyPressEvent(&event);
    CHECK_FALSE(fixture.m_action->isCompleted());
}

TEST_CASE("The selection step hands Return on to the plugin's collector",
          "[plugins][selection][issue2241]") {
    lc::test::ActionFixture<QC_ActionGetSelect> fixture;
    auto* line = new RS_Line{&fixture.m_graphic, {{0., 0.}, {10., 10.}}};
    fixture.m_graphic.addEntity(line);
    RS_Selection(&fixture.m_view).selectSingle(line);
    REQUIRE(fixture.m_graphic.hasSelection());

    LC_ActionSelectSingle inner(&fixture.m_context, fixture.m_action.get());
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    inner.keyPressEvent(&event);

    CHECK(fixture.m_action->isCompleted());
}
