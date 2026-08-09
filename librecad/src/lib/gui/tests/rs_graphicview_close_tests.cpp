/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QAction>
#include <QMouseEvent>

#include "lc_actioncontext.h"
#include "lc_eventhandler.h"
#include "rs_actioninterface.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first,
    // and leak the pointer on the create path: only one QApplication may exist
    // at a time, and only one ~QApplication may run at exit. The old by-value
    // static here constructed a second instance whenever another test file ran
    // first - silently tolerated on release Qt, an abort on assert-enabled Qt.
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
    return *app;
}

class TestGraphicView final : public RS_GraphicView {
public:
    TestGraphicView()
        : RS_GraphicView(nullptr) {
    }

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
    LC_EventHandler* eventHandler() const { return getEventHandler(); }
};

struct ActionState {
    int finishCount = 0;
    int initCount = 0;
    int destructorCount = 0;
};

class TrackingDefaultAction final : public RS_ActionInterface {
public:
    TrackingDefaultAction(LC_ActionContext* context, ActionState& state)
        : RS_ActionInterface("TrackingDefaultAction", context, RS2::ActionDefault)
        , m_state(state) {
    }

    ~TrackingDefaultAction() override {
        ++m_state.destructorCount;
    }

    void init([[maybe_unused]] int status) override {
        ++m_state.initCount;
    }

    void finish() override {
        ++m_state.finishCount;
        RS_ActionInterface::finish();
    }

private:
    ActionState& m_state;
};

} // namespace

TEST_CASE("closing a graphic view permanently quiesces its event handler",
          "[gui][close]") {
    (void)application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    TestGraphicView view;
    view.setDocument(&graphic);

    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);

    LC_EventHandler* eventHandler = view.eventHandler();
    REQUIRE(eventHandler != nullptr);

    ActionState defaultActionState;
    eventHandler->setDefaultAction(new TrackingDefaultAction(&context, defaultActionState));

    QAction activeUiAction;
    activeUiAction.setCheckable(true);
    eventHandler->setQAction(&activeUiAction);
    REQUIRE(activeUiAction.isChecked());

    view.beginClose();

    CHECK(view.isClosing());
    CHECK_FALSE(view.isEnabled());
    CHECK(eventHandler->getDefaultAction() == nullptr);
    CHECK(defaultActionState.finishCount == 1);
    CHECK(defaultActionState.initCount == 0);
    CHECK(defaultActionState.destructorCount == 1);
    CHECK_FALSE(activeUiAction.isChecked());
    CHECK(eventHandler->getQAction() == nullptr);

    QMouseEvent queuedMove(QEvent::MouseMove, QPointF{}, QPointF{},
                           Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    eventHandler->mouseMoveEvent(&queuedMove);
    CHECK(queuedMove.isAccepted());

    view.beginClose();
    CHECK(defaultActionState.finishCount == 1);

    ActionState lateActionState;
    eventHandler->setDefaultAction(new TrackingDefaultAction(&context, lateActionState));
    CHECK(lateActionState.finishCount == 0);
    CHECK(lateActionState.initCount == 0);
    CHECK(lateActionState.destructorCount == 1);

    QAction lateUiAction;
    lateUiAction.setCheckable(true);
    eventHandler->setQAction(&lateUiAction);
    CHECK_FALSE(lateUiAction.isChecked());
    CHECK(eventHandler->getQAction() == nullptr);
}

TEST_CASE("killAllActions is safe after beginClose quiesced the handler",
          "[gui][close]") {
    (void)application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    TestGraphicView view;
    view.setDocument(&graphic);

    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);

    LC_EventHandler* eventHandler = view.eventHandler();
    REQUIRE(eventHandler != nullptr);

    ActionState defaultActionState;
    eventHandler->setDefaultAction(new TrackingDefaultAction(&context, defaultActionState));

    view.beginClose();
    REQUIRE(eventHandler->getDefaultAction() == nullptr);

    CHECK_NOTHROW(eventHandler->killAllActions());
    CHECK(defaultActionState.finishCount == 1);
    CHECK(eventHandler->getDefaultAction() == nullptr);
}

TEST_CASE("close during mouse move then killAllActions is safe",
          "[gui][close]") {
    (void)application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    TestGraphicView view;
    view.setDocument(&graphic);

    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);

    LC_EventHandler* eventHandler = view.eventHandler();
    REQUIRE(eventHandler != nullptr);

    ActionState defaultActionState;
    eventHandler->setDefaultAction(new TrackingDefaultAction(&context, defaultActionState));

    QMouseEvent move(QEvent::MouseMove, QPointF{10, 10}, QPointF{10, 10},
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    eventHandler->mouseMoveEvent(&move);

    view.beginClose();
    CHECK(view.isClosing());
    CHECK(eventHandler->getDefaultAction() == nullptr);

    QMouseEvent lateMove(QEvent::MouseMove, QPointF{20, 20}, QPointF{20, 20},
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    eventHandler->mouseMoveEvent(&lateMove);
    CHECK(lateMove.isAccepted());

    CHECK_NOTHROW(eventHandler->killAllActions());
    view.beginClose(); // double-close
    CHECK(defaultActionState.finishCount == 1);
}
