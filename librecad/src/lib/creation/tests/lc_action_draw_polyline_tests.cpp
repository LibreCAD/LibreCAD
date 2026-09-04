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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QApplication>
#include <QSettings>

#include "lc_action_draw_polyline.h"
#include "lc_actioncontext.h"
#include "lc_actionhandlerfactory.h"
#include "lc_defaultactioncontext.h"
#include "lc_snapmanager.h"
#include "qg_actionhandler.h"
#include "rs_commands.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

namespace {

QApplication* application() {
    static int argc = 1;
    static char name[] = "librecad_tests";
    static char* argv[] = {name, nullptr};
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
    return app;
}

class SettingGuard {
public:
    SettingGuard(RS_Settings* settings, QString group, QString key)
        : m_settings(settings), m_group(std::move(group)), m_key(std::move(key)),
          m_fullKey(QString("/%1/%2").arg(m_group, m_key)),
          m_existed(settings->getSettings()->contains(m_fullKey)),
          m_value(settings->getSettings()->value(m_fullKey)) {}

    ~SettingGuard() {
        const auto groupGuard = m_settings->beginGroupGuard(m_group);
        if (m_existed) {
            m_settings->write(m_key, m_value);
        }
        else {
            m_settings->remove(m_key);
        }
    }

    void set(const int value) const { m_settings->writeSingle(m_group, m_key, value); }

private:
    RS_Settings* m_settings;
    QString m_group;
    QString m_key;
    QString m_fullKey;
    bool m_existed;
    QVariant m_value;
};

class PolylineActionTestView final : public RS_GraphicView {
public:
    PolylineActionTestView() : RS_GraphicView(nullptr) {}

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

class CommandCapturingActionHandler final : public QG_ActionHandler {
public:
    CommandCapturingActionHandler() : QG_ActionHandler(nullptr) {}

    mutable RS2::ActionType actionType = RS2::ActionNone;
    mutable bool startInLineMode = false;

    std::shared_ptr<RS_ActionInterface> createActionInstance(const RS2::ActionType type,
                                                              void* data) const override {
        actionType = type;
        startInLineMode = data != nullptr && *static_cast<const bool*>(data);
        return {};
    }
};

struct PolylineActionFixture {
    const bool m_qtReady{application() != nullptr};
    RS_Graphic m_graphic;
    PolylineActionTestView m_view;
    LC_ActionContext m_context;

    PolylineActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }
};

struct CommandDispatchFixture {
    const bool m_qtReady{application() != nullptr};
    RS_Graphic m_graphic;
    CommandCapturingActionHandler m_actionHandler;
    LC_DefaultActionContext m_context{&m_actionHandler};
    LC_SnapManager m_snapManager{nullptr};
    PolylineActionTestView m_view;

    CommandDispatchFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_actionHandler.setActionContext(&m_context);
        m_actionHandler.setDocumentAndView(&m_graphic, &m_view);
        m_actionHandler.setSnapManager(&m_snapManager);
    }
};

} // namespace

TEST_CASE("polyline command always starts in line mode without resetting the tool preference",
          "[polyline][issue1134]") {
    PolylineActionFixture fixture;
    CommandDispatchFixture dispatchFixture;
    SettingGuard modeGuard{RS_SETTINGS, "ActionDrawPolyline", "Mode"};
    modeGuard.set(LC_ActionDrawPolyline::TangentalArcFixedAngle);

    CHECK(RS_COMMANDS->cmdToAction("polyline") == RS2::ActionDrawPolyline);
    CHECK(RS_COMMANDS->cmdToAction("pl") == RS2::ActionDrawPolyline);
    REQUIRE(dispatchFixture.m_actionHandler.command("pl"));
    CHECK(dispatchFixture.m_actionHandler.actionType == RS2::ActionDrawPolyline);
    CHECK(dispatchFixture.m_actionHandler.startInLineMode);

    bool startInLineMode = true;
    const auto commandAction = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionDrawPolyline, &fixture.m_context, &startInLineMode);
    const auto* commandPolyline = dynamic_cast<const LC_ActionDrawPolyline*>(commandAction.get());
    REQUIRE(commandPolyline != nullptr);
    CHECK(commandPolyline->rtti() == RS2::ActionDrawPolyline);
    CHECK(commandPolyline->getMode() == LC_ActionDrawPolyline::Line);

    commandAction->saveOptions();
    const auto toolAction = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionDrawPolyline, &fixture.m_context);
    const auto* toolPolyline = dynamic_cast<const LC_ActionDrawPolyline*>(toolAction.get());
    REQUIRE(toolPolyline != nullptr);
    CHECK(toolPolyline->getMode() == LC_ActionDrawPolyline::TangentalArcFixedAngle);
}

TEST_CASE("explicit polyline mode changes update the tool preference", "[polyline][issue1134]") {
    PolylineActionFixture fixture;
    SettingGuard modeGuard{RS_SETTINGS, "ActionDrawPolyline", "Mode"};
    modeGuard.set(LC_ActionDrawPolyline::Line);

    bool startInLineMode = true;
    const auto commandAction = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionDrawPolyline, &fixture.m_context, &startInLineMode);
    auto* commandPolyline = dynamic_cast<LC_ActionDrawPolyline*>(commandAction.get());
    REQUIRE(commandPolyline != nullptr);
    commandPolyline->setMode(LC_ActionDrawPolyline::TangentalArcFixedAngle);
    commandAction->saveOptions();

    const auto toolAction = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionDrawPolyline, &fixture.m_context);
    const auto* toolPolyline = dynamic_cast<const LC_ActionDrawPolyline*>(toolAction.get());
    REQUIRE(toolPolyline != nullptr);
    CHECK(toolPolyline->getMode() == LC_ActionDrawPolyline::TangentalArcFixedAngle);
}
