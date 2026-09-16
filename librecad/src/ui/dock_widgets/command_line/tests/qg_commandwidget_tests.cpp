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

// Issue #2871: a long action prompt must not widen the Command Line panel.

#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "lc_actiontestsupport.h"
#include "qg_commandwidget.h"

TEST_CASE("A long prompt leaves the command line panel as narrow as before",
          "[gui][command_line][issue2871]") {
    (void)lc::test::application();

    QG_CommandWidget widget(nullptr, nullptr, "command_widget");
    widget.setCommand(QStringLiteral("Command"));
    const int narrow = widget.minimumSizeHint().width();

    // The prompt of the offset action, reached by typing "mo", and the
    // longest prompt in the actions, from draw rectangle by one point.
    for (const char *prompt : {
             "Select line, polyline, ellipse, circle or arc to create offset, "
             "or press Enter to finish the selection",
             "Specify reference point "
             "[topl|top|topr|left|middle|right|bottoml|bottom|bottomr]"}) {
        widget.setCommand(QString::fromUtf8(prompt));
        CHECK(widget.minimumSizeHint().width() <= narrow);
    }
}

