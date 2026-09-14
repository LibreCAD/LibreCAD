/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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

// Scaffolding for Catch2 tests that run actions on a drawing without a window.

#ifndef LC_ACTIONTESTSUPPORT_H
#define LC_ACTIONTESTSUPPORT_H

#include <memory>

#include <QApplication>
#include <QString>

#include "lc_actioncontext.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_previewactioninterface.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace lc::test {

/**
 * Returns a QApplication, reusing the process-wide one if another test built it
 * first. The pointer is deliberately leaked: only one QApplication may exist at
 * a time and only one ~QApplication may run at exit.
 */
inline QApplication* application() {
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

/// A 640x480 view that draws nothing.
class TestGraphicView final : public RS_GraphicView {
public:
    TestGraphicView() : RS_GraphicView(nullptr) {}

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

inline LC_MouseEvent eventAt(const double x, const double y) {
    LC_MouseEvent e;
    e.graphPoint = RS_Vector{x, y};
    e.snapPoint = RS_Vector{x, y};
    return e;
}

/**
 * A new drawing shown in a TestGraphicView, with an action of the given type on it.
 * Member order matters: the action is destroyed before the view, because
 * ~RS_PreviewActionInterface reaches into overlay containers the view owns.
 */
template <typename Action>
struct ActionFixture {
    const bool m_qtReady{application() != nullptr};
    RS_Graphic m_graphic;
    TestGraphicView m_view;
    LC_ActionContext m_context;
    std::unique_ptr<Action> m_action;

    ActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<Action>(&m_context);
    }
};

} // namespace lc::test

#endif
