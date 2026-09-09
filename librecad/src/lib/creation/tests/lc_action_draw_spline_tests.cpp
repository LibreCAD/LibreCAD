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

#include <QApplication>

#include "lc_action_draw_spline.h"
#include "lc_actioncontext.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "rs_spline.h"

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

class SplineActionTestView final : public RS_GraphicView {
public:
    SplineActionTestView() : RS_GraphicView(nullptr) {}

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

class SplineActionProbe final : public LC_ActionDrawSpline {
public:
    explicit SplineActionProbe(LC_ActionContext* actionContext)
        : LC_ActionDrawSpline(actionContext) {}

    using LC_ActionDrawSpline::doProcessCommand;
    using LC_ActionDrawSpline::onCoordinateEvent;
    using LC_ActionDrawSpline::onMouseRightButtonRelease;
};

struct SplineActionFixture {
    const bool m_qtReady{application() != nullptr};
    RS_Graphic m_graphic;
    SplineActionTestView m_view;
    LC_ActionContext m_context;

    SplineActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }
};

} // namespace

TEST_CASE("spline close command creates a valid closed spline", "[spline][command]") {
    SplineActionFixture fixture;
    SplineActionProbe action(&fixture.m_context);
    action.setDegree(3);
    const auto addPoint = [&action](const double x, const double y) {
        action.onCoordinateEvent(action.getStatus(), false, RS_Vector{x, y});
    };

    addPoint(0.0, 0.0);
    addPoint(10.0, 0.0);
    addPoint(10.0, 10.0);

    CHECK_FALSE(action.getAvailableCommands().contains("close"));
    const int nextPointStatus = action.getStatus();
    action.onMouseRightButtonRelease(nextPointStatus, nullptr);
    CHECK(action.getStatus() == nextPointStatus);
    CHECK(fixture.m_graphic.count() == 0);

    addPoint(0.0, 10.0);
    REQUIRE(action.getAvailableCommands().contains("close"));
    REQUIRE(action.doProcessCommand(action.getStatus(), "close"));

    REQUIRE(fixture.m_graphic.count() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(fixture.m_graphic.first());
    REQUIRE(spline != nullptr);
    CHECK(spline->isClosed());
    CHECK(spline->validate());
    CHECK(spline->getNumberOfControlPoints() == 4);
    CHECK_FALSE(action.isClosed());
    CHECK(action.getStatus() == RS_ActionInterface::InitialActionStatus);
}
