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
#include <catch2/generators/catch_generators.hpp>

#include <memory>

#include <QApplication>
#include <QLayout>
#include <QLineEdit>
#include <QWidget>

#include "lc_action_draw_line.h"
#include "lc_action_draw_line_snake.h"
#include "lc_action_options_editor.h"
#include "lc_action_options_widget.h"
#include "lc_actioncontext.h"
#include "lc_actiontestsupport.h"
#include "lc_defaultactioncontext.h"
#include "lc_optionswidgetsholder.h"
#include "lc_snapmanager.h"
#include "qg_actionhandler.h"
#include "rs_actionsetrelativezero.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

namespace {

class SnakeActionProbe final : public LC_ActionDrawLineSnake {
public:
    explicit SnakeActionProbe(LC_ActionContext* actionContext,
                              RS2::ActionType type = RS2::ActionDrawSnakeLineX)
        : LC_ActionDrawLineSnake(actionContext, type) {}

    using LC_ActionDrawLineSnake::doProcessCommand;
    using LC_ActionDrawLineSnake::onCoordinateEvent;

    static constexpr int distanceStatus() { return SetDistance; }
    static constexpr int startPointStatus() { return SetStartPoint; }

    void enableToolOptions() {
        createOptionsEditor();
        m_optionsEditor->setup(true, false);
    }
};

class SnakeOptionsContext final : public LC_ActionContext {
public:
    explicit SnakeOptionsContext(LC_OptionsWidgetsHolder& holder) : m_holder(holder) {}
    void addOptionsWidget(LC_ActionOptionsWidget* widget) override { m_holder.addOptionsWidget(widget); }
    void removeOptionsWidget(LC_ActionOptionsWidget* widget) override { m_holder.removeOptionsWidget(widget); }
private:
    LC_OptionsWidgetsHolder& m_holder;
};

// Use the real dispatcher while keeping options out of the application's property sheet.
class SnakeCommandHandler final : public QG_ActionHandler {
public:
    explicit SnakeCommandHandler(LC_ActionContext& context) : QG_ActionHandler(nullptr), m_context(context) {}
    mutable std::weak_ptr<RS_ActionInterface> latestAction;

    std::shared_ptr<RS_ActionInterface> createActionInstance(RS2::ActionType type, void*) const override {
        if (type == RS2::ActionDrawLine) {
            return std::make_shared<LC_ActionDrawLine>(&m_context);
        }
        auto action = std::make_shared<SnakeActionProbe>(&m_context, type);
        action->enableToolOptions();
        latestAction = action;
        return action;
    }
private:
    LC_ActionContext& m_context;
};

struct SnakeActionFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    LC_OptionsWidgetsHolder m_optionsHolder;
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    SnakeOptionsContext m_context{m_optionsHolder};
    std::shared_ptr<SnakeActionProbe> m_action;

    SnakeActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_shared<SnakeActionProbe>(&m_context);
    }

    ~SnakeActionFixture() {
        m_view.beginClose();
    }
};

} // namespace

TEST_CASE("Snake leaves unrelated commands to the action dispatcher", "[snake][commands]") {
    const auto type = GENERATE(RS2::ActionDrawSnakeLine, RS2::ActionDrawSnakeLineX, RS2::ActionDrawSnakeLineY);
    const auto command = GENERATE("sline", "slinex", "sliney", "line", "circle", "not-a-command");
    SnakeActionFixture fixture;
    SnakeActionProbe action(&fixture.m_context, type);

    SECTION("waiting for the first point") {}
    SECTION("waiting for a distance or a free point") {
        action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
    }
    SECTION("waiting for a direction") {
        action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
        action.next();
    }
    SECTION("waiting for an angle") {
        action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
        action.setSetAngleDirectionState();
    }
    const int status = action.getStatus();
    const int direction = action.getDirection();
    CAPTURE(type, command, status);
    RS_CommandEvent event(QString::fromLatin1(command));
    action.commandEvent(&event);
    CHECK_FALSE(event.isAccepted());
    CHECK(action.getStatus() == status);
    CHECK(action.getDirection() == direction);
    CHECK(fixture.m_graphic.count() == 0);
}

TEST_CASE("Refreshing Snake options preserves the input state", "[snake][options]") {
    SnakeActionFixture fixture;
    auto& action = *fixture.m_action;
    action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
    action.setSetAngleDirectionState();
    action.setAngleValueDegrees(30.0);
    REQUIRE(action.getStatus() == SnakeActionProbe::distanceStatus());

    action.enableToolOptions();
    action.showOptions();
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());
    action.updateOptions();
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());
    action.hideOptions();
    action.showOptions();
    CHECK(action.getStatus() == SnakeActionProbe::distanceStatus());
    REQUIRE(action.doProcessCommand(action.getStatus(), QStringLiteral("10")));
    CHECK(fixture.m_graphic.count() == 1);
    CHECK(action.getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE);
}

TEST_CASE("Restarting and switching Snake keeps a single set of options", "[snake][options]") {
    SnakeActionFixture fixture;
    fixture.m_optionsHolder.show();
    auto* container = fixture.m_optionsHolder.findChild<QWidget*>(QStringLiteral("wOptionsWidgetsContainer"));
    REQUIRE(container != nullptr);
    auto* layout = container->layout();
    REQUIRE(layout != nullptr);
    const int emptyCount = layout->count();

    SnakeCommandHandler handler(fixture.m_context);
    LC_DefaultActionContext commandContext(&handler);
    LC_SnapManager snapManager(nullptr);
    handler.setActionContext(&commandContext);
    handler.setDocumentAndView(&fixture.m_graphic, &fixture.m_view);
    handler.setSnapManager(&snapManager);

    std::weak_ptr<RS_ActionInterface> previous;
    for (const auto command : {"slinex", "slinex", "sliney", "sline"}) {
        REQUIRE(handler.command(QString::fromLatin1(command)));
        CHECK(previous.expired());
        auto* action = dynamic_cast<SnakeActionProbe*>(fixture.m_view.getCurrentAction());
        REQUIRE(action != nullptr);
        CHECK(action->rtti() == RS_COMMANDS->cmdToAction(QString::fromLatin1(command)));
        previous = handler.latestAction;
        CHECK(layout->count() == emptyCount + 1);
        action->onCoordinateEvent(action->getStatus(), false, RS_Vector{0.0, 0.0});
        action->setSetAngleDirectionState();
        action->updateOptions();
        auto* angle = container->findChild<QLineEdit*>(QStringLiteral("leAngle"));
        REQUIRE(angle != nullptr);
        angle->setFocus();
        angle->setText(QStringLiteral("30"));
        fixture.m_view.back(Qt::NoModifier);
    }

    REQUIRE(handler.command(QStringLiteral("line")));
    CHECK(previous.expired());
    CHECK(fixture.m_view.getCurrentAction()->rtti() == RS2::ActionDrawLine);
    CHECK(layout->count() == emptyCount);
    CHECK(container->findChildren<LC_ActionOptionsWidget*>().isEmpty());
}

TEST_CASE("A temporary action restores Snake options without duplication", "[snake][options]") {
    SnakeActionFixture fixture;
    fixture.m_action->enableToolOptions();
    fixture.m_optionsHolder.show();
    REQUIRE(fixture.m_view.setCurrentAction(fixture.m_action));
    fixture.m_action->onCoordinateEvent(fixture.m_action->getStatus(), false, RS_Vector{0.0, 0.0});

    auto* container = fixture.m_optionsHolder.findChild<QWidget*>(QStringLiteral("wOptionsWidgetsContainer"));
    REQUIRE(container != nullptr);
    auto* layout = container->layout();
    REQUIRE(layout != nullptr);
    const int snakeOptionsCount = layout->count();

    auto setZero = std::make_shared<RS_ActionSetRelativeZero>(&fixture.m_context);
    REQUIRE(fixture.m_view.setCurrentAction(setZero));
    CHECK(layout->count() == snakeOptionsCount - 1);
    RS_CommandEvent point(QStringLiteral("5,5"));
    fixture.m_view.commandEvent(&point);
    CHECK(point.isAccepted());
    CHECK(setZero->isFinished());
    CHECK(fixture.m_view.getCurrentAction() == fixture.m_action.get());
    CHECK(fixture.m_action->getStatus() == SnakeActionProbe::distanceStatus());
    CHECK(layout->count() == snakeOptionsCount);
    CHECK(container->findChildren<LC_ActionOptionsWidget*>().size() == 1);
}

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
