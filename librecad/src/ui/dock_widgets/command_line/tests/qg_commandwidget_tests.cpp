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
#include <QSplitter>
#include <QKeyEvent>

#include "lc_actiontestsupport.h"
#include "qg_commandwidget.h"
#include "rs_settings.h"

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

TEST_CASE("Command completion eligibility never includes drafting arguments", "[command_line][suggestions]") {
    for (const char* input : {"", "100", "-10", "+5", ".5", "1e3", "10,20", "@10,20", "@5<45", "#2,3",
                             "line 1,2", "cal 1+2", "2*pi", "\\variable", "a=2", "line;0,0", "a b"}) {
        INFO(input);
        CHECK_FALSE(QG_CommandEdit::isCommandToken(QString::fromUtf8(input)));
    }
    CHECK(QG_CommandEdit::isCommandToken("LI"));
    CHECK(QG_CommandEdit::isCommandToken("phase2alias"));
    CHECK(QG_CommandEdit::isCommandToken("snap-endpoint"));
}

TEST_CASE("Closed-popup history and Ctrl-arrow precise input retain their meanings", "[command_line]") {
    lc::test::application();
    QG_CommandEdit edit;
    QStringList commands;
    QObject::connect(&edit, &QG_CommandEdit::command, &edit, [&](const QString& command) {commands << command;});
    const auto key = [&](int value, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, value, modifiers);
        QApplication::sendEvent(&edit, &event);
    };
    edit.setText("@10,20");
    key(Qt::Key_Return);
    REQUIRE(commands == QStringList{"@10,20"});
    key(Qt::Key_Up);
    CHECK(edit.text() == "@10,20");
    key(Qt::Key_Down);
    CHECK(edit.text().isEmpty());
    edit.setText("5.25");
    key(Qt::Key_Right, Qt::ControlModifier);
    CHECK(commands.last() == "@5.25,0");
    edit.setText("@5<45");
    key(Qt::Key_Return);
    CHECK(commands.last() == "@5<45");
}

TEST_CASE("Compact command row reserves input and accepts empty Enter without a drawing", "[command_line]") {
    lc::test::application();
    QG_CommandWidget widget(nullptr, nullptr, "command_widget");
    widget.resize(360, 120);
    widget.show();
    QCoreApplication::processEvents();
    widget.setCommand("Select an object or specify reference point [topl|top|topr|left|middle|right|bottoml|bottom|bottomr]");
    QCoreApplication::processEvents();
    CHECK(widget.leCommand->width() >= 120);
    CHECK(widget.lCommand->geometry().center().y() == widget.leCommand->geometry().center().y());
    CHECK(widget.lCommand->toolTip().contains("bottomr"));
    CHECK(widget.lCommand->text().contains(QChar(0x2026)));
    widget.handleCommand({});
    CHECK(widget.leCommand->text().isEmpty());
    widget.historySplitter->setSizes({0, 28});
    QCoreApplication::processEvents();
    CHECK(widget.leCommand->isVisible());
    CHECK(widget.leCommand->height() >= 24);
}

TEST_CASE("Command Space submits once and Shift Space inserts a literal space", "[command_line][space-enter]") {
    lc::test::application();
    const bool previous = LC_GET_ONE_BOOL("Keyboard", "EvaluateCommandOnSpace", true);
    LC_SET_ONE("Keyboard", "EvaluateCommandOnSpace", true);
    QG_CommandEdit edit;
    QStringList commands;
    QObject::connect(&edit, &QG_CommandEdit::command, &edit, [&](const QString& command) { commands << command; });
    auto space = [&](Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Space, modifiers, " ");
        QApplication::sendEvent(&edit, &event);
    };
    edit.setText("line");
    space();
    REQUIRE(commands == QStringList{"line"});
    CHECK(edit.text().isEmpty());
    space();
    REQUIRE(commands == QStringList{"line", ""});
    edit.setText("@15<30");
    space();
    CHECK(commands.last() == "@15<30");
    edit.setText("some");
    space(Qt::ShiftModifier);
    CHECK(edit.text() == "some ");
    CHECK(commands.size() == 3);
    LC_SET_ONE("Keyboard", "EvaluateCommandOnSpace", false);
    edit.setText("text");
    space();
    CHECK(edit.text() == "text ");
    CHECK(commands.size() == 3);
    LC_SET_ONE("Keyboard", "EvaluateCommandOnSpace", previous);
}
