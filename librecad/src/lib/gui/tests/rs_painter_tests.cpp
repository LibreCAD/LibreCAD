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

// Regression tests for RS_Painter::setPen(): which QPen it installs for a given
// RS_Pen, and when it may keep the pen it already holds instead.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QImage>

#include "rs_color.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"

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

/**
 * A painter on a plain image. RS_Painter::setPen() reads only the drawing mode,
 * the device resolution and the painter's current join/cap styles, so no
 * viewport and no renderer are needed; rs_hatch_tests.cpp builds its test
 * painter on a QImage the same way.
 */
struct PenTestPainter {
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
};

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

} // namespace

TEST_CASE("RS_Painter paints solid for a line type with no pattern",
          "[gui][painter][linetype]") {
    (void)application();

    // RS2::LineTypeUnchanged is the one enumerator RS_LineTypePattern::getPattern()
    // has no table entry for; the out-of-range value stands for a stale settings
    // integer, which is how the grid, snap and overlay pens reach the painter.
    for (const RS2::LineType lineType : {RS2::LineTypeUnchanged, static_cast<RS2::LineType>(999)}) {
        PenTestPainter rig;
        rig.painter.setPen(testPen(lineType));

        CHECK(rig.painter.pen().style() == Qt::SolidLine);
        CHECK(rig.painter.pen().dashPattern().isEmpty());
    }
}
