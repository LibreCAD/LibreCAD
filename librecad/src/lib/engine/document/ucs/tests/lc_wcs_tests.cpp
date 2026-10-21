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

// LC_WCS::instance is the shared WCS handed to the viewport when switching
// back to the world coordinate system. It used to be declared as a plain
// LC_UCS, slicing the LC_WCS it was built from, so isUCS() returned true and
// RS_Graphic::setCurrentUCS wrote "WCS" to $UCSNAME (and so to the DXF header)
// instead of leaving it empty. These tests pin the singleton as a WCS and check
// the header paths that apply it.

#include <memory>

#include <catch2/catch_test_macros.hpp>

#include <QApplication>

#include "lc_graphicviewport.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "lc_view.h"
#include "rs_graphic.h"
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

// A drawing shown in a viewport whose current UCS is the named UCS "SITE".
struct SiteFixture {
    SiteFixture() {
        m_viewport.setSize(640, 480);
        m_viewport.setDocument(&m_graphic);
        m_viewport.loadSettings();

        auto* ucs = new LC_UCS(QStringLiteral("SITE"));
        ucs->setOrigin(RS_Vector(10.0, 20.0, 0.0));
        ucs->setXAxis(RS_Vector(0.0, 1.0, 0.0));
        ucs->setYAxis(RS_Vector(-1.0, 0.0, 0.0));
        m_graphic.addUCS(ucs);
        m_site = m_graphic.getUCSList()->find(QStringLiteral("SITE"));
        REQUIRE(m_site != nullptr);
        m_viewport.applyUCS(m_site);
        REQUIRE(m_graphic.getVariableString("$UCSNAME", "") == QStringLiteral("SITE"));
    }

    void checkCurrentIsWCS() const {
        CHECK(m_graphic.getVariableString("$UCSNAME", QStringLiteral("unset")).isEmpty());
        const std::unique_ptr<LC_UCS> current{m_graphic.getCurrentUCS()};
        REQUIRE(current != nullptr);
        CHECK_FALSE(current->isUCS());
    }

    // first, so the settings RS_Graphic reads are set up before it is built
    const bool m_appReady{(application(), true)};
    RS_Graphic m_graphic;
    LC_GraphicViewport m_viewport;
    LC_UCS* m_site = nullptr;
};

} // namespace

TEST_CASE("LC_WCS::instance is a WCS", "[ucs][wcs]") {
    const LC_UCS& wcs = LC_WCS::instance;
    CHECK_FALSE(wcs.isUCS());
    CHECK(wcs.getName() == LC_WCS().getName());
    CHECK(wcs.getOrigin() == RS_Vector(0.0, 0.0, 0.0));
    CHECK(wcs.getXAxis() == RS_Vector(1.0, 0.0, 0.0));
    CHECK(wcs.getYAxis() == RS_Vector(0.0, 1.0, 0.0));

    // A clone of the singleton is a WCS too.
    const std::unique_ptr<LC_UCS> copy{wcs.clone()};
    CHECK_FALSE(copy->isUCS());
}

// LC_UCSListWidget (after removing a UCS, or "Set WCS" while editing a block)
// and LC_ActionUCSByDimOrdinate switch back to the WCS this way.
TEST_CASE("Applying the WCS singleton clears $UCSNAME", "[ucs][wcs]") {
    SiteFixture fixture;
    fixture.m_viewport.applyUCS(&LC_WCS::instance);
    fixture.checkCurrentIsWCS();
}

// LC_GraphicViewport::restoreView falls back to the singleton for a named view
// that has no UCS (e.g. a DXF VIEW record without one).
TEST_CASE("Restoring a named view without a UCS clears $UCSNAME", "[ucs][wcs][views]") {
    SiteFixture fixture;
    LC_View view(QStringLiteral("PLAN"));
    view.setCenter(RS_Vector(0.0, 0.0, 0.0));
    view.setSize(RS_Vector(100.0, 100.0, 0.0));
    REQUIRE_FALSE(view.isHasUCS());

    fixture.m_viewport.restoreView(&view);
    fixture.checkCurrentIsWCS();
}
