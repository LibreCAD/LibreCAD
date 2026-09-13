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

// Regression tests for LC_GraphicViewRenderer's pen cache. The pen it caches is
// not always the pen it installs - a selected entity is painted with
// RS2::DashLineTiny whatever line type it resolved to - which is where a
// predicate that reads the cached line type gets the answer wrong.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QImage>

#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "rs_color.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first,
    // and leak the pointer on the create path: only one QApplication may exist
    // at a time, and only one ~QApplication may run at exit. Test files are
    // linked into one executable, so this helper is per translation unit;
    // rs_graphicview_close_tests.cpp carries the same one.
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

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

/**
 * The widget renderer with its pen preparation reachable. Nothing is overridden:
 * the case below is about what the shipped code does.
 */
class TestViewRenderer final : public LC_GraphicViewRenderer {
public:
    TestViewRenderer(LC_GraphicViewport* viewport, QPaintDevice* device)
        : LC_GraphicViewRenderer(viewport, device) {
    }

    using LC_GraphicViewRenderer::setPenForEntity;
    void beginPass() { doSetupBeforeContainerDraw(); }
};

} // namespace

TEST_CASE("a run of selected entities keeps advancing the selection dash phase",
          "[gui][pen][linetype]") {
    (void)application();

    // LC_GraphicViewRenderer paints a selected entity with RS2::DashLineTiny
    // whatever line type it resolved to, and then gives it the painter's running
    // dash offset - but the pen it caches is the resolved one, still solid. Two
    // selected entities on one layer resolve to the same pen, so the second one
    // reaches the cache probe. It must not be skipped there: it is drawn a dash
    // phase further along than the first, and the selection dashes stop running
    // continuously along the selection if it inherits the first one's phase.
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);

    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
    TestViewRenderer renderer{&viewport, &image};
    renderer.beginPass();

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{100., 0.}};
    first.setPen(testPen(RS2::SolidLine));
    first.setFlag(RS2::FlagSelected);
    RS_Line second{RS_Vector{0., 10.}, RS_Vector{100., 10.}};
    second.setPen(testPen(RS2::SolidLine));
    second.setFlag(RS2::FlagSelected);

    renderer.setPenForEntity(&painter, &first, false);
    REQUIRE(painter.pen().style() == Qt::CustomDashLine);
    const double firstPhase = painter.pen().dashOffset();

    // What RS_Line::draw() does for every line it draws.
    painter.updateDashOffset(&first);
    REQUIRE(painter.currentDashOffset() != 0.0);

    renderer.setPenForEntity(&painter, &second, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.pen().dashOffset() != firstPhase);
}
