/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * ********************************************************************************
 */

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QApplication>
#include <QLayout>
#include <QWidget>

#include "lc_action_draw_line_snake.h"
#include "lc_actioncontext.h"
#include "lc_actiontestsupport.h"
#include "lc_optionswidgetsholder.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

namespace {

class SnakeActionProbe final : public LC_ActionDrawLineSnake {
public:
    explicit SnakeActionProbe(LC_ActionContext* actionContext)
        : LC_ActionDrawLineSnake(actionContext, RS2::ActionDrawSnakeLineX) {}

    using LC_ActionDrawLineSnake::doProcessCommand;
    using LC_ActionDrawLineSnake::onCoordinateEvent;

    static constexpr int distanceStatus() { return SetDistance; }
    static constexpr int startPointStatus() { return SetStartPoint; }
};

struct SnakeActionFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;
    std::unique_ptr<SnakeActionProbe> m_action;

    SnakeActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<SnakeActionProbe>(&m_context);
    }
};

} // namespace

TEST_CASE("Snake line undo and redo restore the previous direction", "[snake][undo]") {
    SnakeActionFixture fixture;
    auto& action = *fixture.m_action;

    action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
    REQUIRE(action.getStatus() == SnakeActionProbe::distanceStatus());
    REQUIRE(action.doProcessCommand(action.getStatus(), QStringLiteral("10")));
    REQUIRE(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_Y);
    REQUIRE(action.doProcessCommand(action.getStatus(), QStringLiteral("5")));
    REQUIRE(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_X);

    action.undo();
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_Y);
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());

    action.redo();
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_X);
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());

    action.undo();
    action.undo();
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_X);
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());
    action.undo();
    CHECK(action.getStatus() == SnakeActionProbe::startPointStatus());
}

TEST_CASE("Snake line history remembers manually selected direction", "[snake][undo]") {
    SnakeActionFixture fixture;
    auto& action = *fixture.m_action;

    action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
    REQUIRE(action.doProcessCommand(action.getStatus(), QStringLiteral("10")));
    REQUIRE(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_Y);

    action.setSetXDirectionState();
    REQUIRE(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_X);
    REQUIRE(action.doProcessCommand(action.getStatus(), QStringLiteral("5")));
    REQUIRE(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_Y);

    action.undo();
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_X);
    action.redo();
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_Y);
}

TEST_CASE("Removed action options widgets leave the layout", "[options][widgets]") {
    (void)lc::test::application();
    LC_OptionsWidgetsHolder holder;
    auto* container = holder.findChild<QWidget*>(QStringLiteral("wOptionsWidgetsContainer"));
    REQUIRE(container != nullptr);
    auto* layout = container->layout();
    REQUIRE(layout != nullptr);

    auto* widget = new QWidget;
    holder.addOptionsWidget(widget);
    REQUIRE(layout->indexOf(widget) >= 0);

    holder.removeOptionsWidget(widget);
    CHECK(layout->indexOf(widget) == -1);
    delete widget;

    auto* replacement = new QWidget;
    holder.addOptionsWidget(replacement);
    CHECK(layout->indexOf(replacement) >= 0);
    holder.removeOptionsWidget(replacement);
    delete replacement;
}
