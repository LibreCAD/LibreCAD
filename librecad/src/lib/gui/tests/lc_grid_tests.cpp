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

#include "lc_graphicviewport.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first.
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

} // namespace

// RS_Grid::loadSettings() builds a new grid system on every call and RS_Grid
// owns the last one until the viewport goes away. Nothing here can count those
// deletions in-process; run under `leaks -atExit` or ASan to see that neither
// the grid systems nor their options outlive the viewport.
TEST_CASE("RS_Grid frees the grid systems it builds", "[grid][ownership]") {
    application();
    RS_Graphic graphic;
    {
        LC_GraphicViewport viewport;
        viewport.setSize(640, 480);
        viewport.setDocument(&graphic);

        // Switching the grid type replaces the grid system with the other
        // concrete class; the replaced one must be freed, not dropped.
        for (const bool isometric : {false, true, false}) {
            graphic.setIsometricGrid(isometric);
            viewport.loadSettings();
            RS_Grid* grid = viewport.getGrid();
            REQUIRE(grid != nullptr);
            CHECK(grid->isIsometric() == isometric);

            // The new grid system is the live one.
            grid->calculateGrid();
            CHECK(grid->snapGrid(RS_Vector(1.3, 2.7)).valid);
        }
    }
}
