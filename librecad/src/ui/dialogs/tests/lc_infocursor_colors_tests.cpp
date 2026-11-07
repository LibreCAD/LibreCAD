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

// Issue #2142: in Application Preferences > Info Cursor, the colour buttons of
// the snap, relative-position and command-prompt lines all worked on the
// absolute-position colour: the picker opened on that colour, and the colour
// picked ended up in its field instead of the field next to the button.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include "lc_actiontestsupport.h"
#include "qg_dlgoptionsgeneral.h"

namespace {

// Opens the picker through the button, notes the colour it starts on, and
// confirms it with a different colour. Returns the starting colour.
QColor pickThrough(QPushButton* button, const QColor& pick) {
    QColor shown;
    QTimer::singleShot(0, [&shown, &pick] {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* picker = qobject_cast<QColorDialog*>(widget); picker != nullptr && picker->isVisible()) {
                shown = picker->currentColor();
                picker->setCurrentColor(pick);
                picker->accept();
                return;
            }
        }
    });
    button->click();
    return shown;
}

} // namespace

TEST_CASE("Each Info Cursor colour button works on its own colour field", "[gui][options][issue2142]") {
    REQUIRE(lc::test::application() != nullptr);
    QG_DlgOptionsGeneral dialog;

    struct Line {
        QPushButton* button;
        QComboBox* field;
        const char* name;
        QColor initial;
        QColor pick;
    };
    const Line lines[] = {
        {dialog.pbcbInfoOverlayAbsolutePositionColor, dialog.cbInfoOverlayAbsolutePositionColor, "absolute position",
         QColor("#102030"), QColor("#a1b1c1")},
        {dialog.pbInfoOverlaySnapColor, dialog.cbInfoOverlaySnapColor, "snap", QColor("#405060"), QColor("#a2b2c2")},
        {dialog.pbInfoOverlayRelativeColor, dialog.cbInfoOverlayRelativeColor, "relative position", QColor("#708090"),
         QColor("#a3b3c3")},
        {dialog.pbInfoOverlayCommandPromptColor, dialog.cbInfoOverlayCommandPromptColor, "command prompt",
         QColor("#a0b0c0"), QColor("#a4b4c4")},
    };
    for (const Line& line : lines) {
        line.field->lineEdit()->setText(line.initial.name());
    }

    for (const Line& line : lines) {
        INFO("colour of the " << line.name << " line");
        // the picker opens on the colour of the field it belongs to ...
        CHECK(pickThrough(line.button, line.pick).name().toStdString() == line.initial.name().toStdString());
        // ... and puts the colour picked into that field only
        for (const Line& other : lines) {
            const QColor expected = (&other == &line) ? line.pick : other.initial;
            CHECK(QColor::fromString(other.field->lineEdit()->text()).name().toStdString() == expected.name().toStdString());
        }
        // restore the field so the next line starts from known values
        line.field->lineEdit()->setText(line.initial.name());
    }
}
