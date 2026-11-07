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

// Issue #2178: the Snake tool listed its commands by looking each one up in
// the command dictionary. "pl" is the same word in every language and has no
// dictionary entry, so every key typed while Snake was running printed
// "Command not found: pl" and put an empty entry in the list of commands.

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QStringList>

#include "lc_action_draw_line_snake.h"
#include "lc_actiontestsupport.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_vector.h"

namespace {

// Keeps the command line messages of RS_DIALOGFACTORY while it lives.
class CommandMessages final : public RS_DialogFactoryInterface {
public:
    QStringList m_messages;

    CommandMessages() : m_previous{RS_DialogFactory::instance()->getFactoryObject()} {
        RS_DialogFactory::instance()->setFactoryObject(this);
    }
    ~CommandMessages() override { RS_DialogFactory::instance()->setFactoryObject(m_previous); }
    CommandMessages(const CommandMessages&) = delete;
    CommandMessages& operator=(const CommandMessages&) = delete;

    void commandMessage(const QString& message) override { m_messages << message; }

private:
    RS_DialogFactoryInterface* m_previous;
};

class SnakeProbe final : public LC_ActionDrawLineSnake {
public:
    explicit SnakeProbe(LC_ActionContext* context) : LC_ActionDrawLineSnake(context, RS2::ActionDrawSnakeLine) {}

    using LC_ActionDrawLineSnake::onCoordinateEvent;
};

} // namespace

TEST_CASE("Snake lists its commands without unknown or empty entries", "[snake][commands][issue2178]") {
    lc::test::ActionFixture<SnakeProbe> f;
    CommandMessages messages;

    // waiting for the start point, then for the next point
    for (int step = 0; step < 2; ++step) {
        const QStringList commands = f.m_action->getAvailableCommands();
        INFO("after " << step << " point(s)");
        CHECK(commands.contains(QStringLiteral("pl")));
        CHECK_FALSE(commands.contains(QString{}));
        if (step == 0) {
            f.m_action->onCoordinateEvent(f.m_action->getStatus(), false, RS_Vector{0.0, 0.0});
        }
    }

    INFO("command line messages: " << messages.m_messages.join(QStringLiteral(" | ")).toStdString());
    CHECK(messages.m_messages.isEmpty());
}
