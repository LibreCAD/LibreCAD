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

// A named view owns the UCS it holds, and LC_ViewList owns the views it
// holds. These tests pin that contract: a UCS is freed with its view or when
// replaced, clone() gives the copy a UCS of its own (so freeing one view never
// frees the other's), and views are freed with their list or when the list
// rejects them. LC_View has no virtual destructor, so the tests count views
// through the counted UCS each one owns.

#include <cmath>
#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QApplication>

#include "lc_graphicviewport.h"
#include "lc_layersexporter.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "lc_view.h"
#include "lc_viewslist.h"
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

// Counts live instances so a test can tell whether a view freed its UCS.
class CountedUCS final : public LC_UCS {
public:
    explicit CountedUCS(int& liveCount)
        : m_liveCount{liveCount} {
        ++m_liveCount;
    }
    ~CountedUCS() override { --m_liveCount; }

private:
    int& m_liveCount;
};

const RS_Vector kOrigin{10.0, 20.0, 0.0};
const RS_Vector kXAxis{0.0, 1.0, 0.0};
const RS_Vector kYAxis{-1.0, 0.0, 0.0};

// A UCS rotated 90 degrees about a non-zero origin.
CountedUCS* makeRotatedUCS(int& liveCount) {
    auto* ucs = new CountedUCS(liveCount);
    ucs->setName(QStringLiteral("SITE"));
    ucs->setOrigin(kOrigin);
    ucs->setXAxis(kXAxis);
    ucs->setYAxis(kYAxis);
    ucs->setElevation(5.0);
    ucs->setOrthoType(LC_UCS::TOP);
    return ucs;
}

LC_View* makeView(const QString& name, int& liveCount) {
    auto* view = new LC_View(name);
    view->setUCS(makeRotatedUCS(liveCount));
    return view;
}

void checkRotatedUCS(const LC_UCS* ucs) {
    REQUIRE(ucs != nullptr);
    CHECK(ucs->isUCS());
    CHECK(ucs->getName() == QStringLiteral("SITE"));
    CHECK(ucs->getOrigin() == kOrigin);
    CHECK(ucs->getXAxis() == kXAxis);
    CHECK(ucs->getYAxis() == kYAxis);
    CHECK(ucs->getElevation() == 5.0);
    CHECK(ucs->getOrthoType() == LC_UCS::TOP);
}

} // namespace

TEST_CASE("LC_View frees its UCS when destroyed", "[views][ownership]") {
    int live = 0;
    {
        LC_View view(QStringLiteral("V"));
        view.setUCS(makeRotatedUCS(live));
        REQUIRE(view.isHasUCS());
        REQUIRE(live == 1);
    }
    CHECK(live == 0);
}

TEST_CASE("LC_View::setUCS frees the UCS it replaces", "[views][ownership]") {
    int live = 0;
    LC_View view(QStringLiteral("V"));
    view.setUCS(makeRotatedUCS(live));
    auto* second = makeRotatedUCS(live);
    REQUIRE(live == 2);

    view.setUCS(second);
    CHECK(live == 1);
    CHECK(view.getUCS() == second);

    // Setting the UCS the view already holds must not free it.
    view.setUCS(view.getUCS());
    CHECK(live == 1);
    checkRotatedUCS(view.getUCS());

    view.setUCS(nullptr);
    CHECK(live == 0);
    CHECK_FALSE(view.isHasUCS());
}

TEST_CASE("LC_View::clone gives the copy a UCS of its own", "[views][ownership]") {
    int live = 0;
    auto* source = makeView(QStringLiteral("V"), live);
    LC_View* copy = source->clone();

    REQUIRE(copy->isHasUCS());
    CHECK(copy->getUCS() != source->getUCS());
    checkRotatedUCS(copy->getUCS());

    // Freeing the source frees only the source's UCS; the copy's stays usable
    // (a shared UCS would be a use-after-free here and a double free below).
    delete source;
    CHECK(live == 0);
    checkRotatedUCS(copy->getUCS());
    delete copy;
}

TEST_CASE("LC_View::clone keeps a WCS a WCS", "[views][ownership]") {
    LC_View source(QStringLiteral("V"));

    SECTION("a view without a UCS clones to a view without a UCS") {
        std::unique_ptr<LC_View> copy{source.clone()};
        CHECK_FALSE(copy->isHasUCS());
    }

    SECTION("a WCS stays a WCS, which RS_Graphic::setCurrentUCS relies on") {
        source.setUCS(new LC_WCS());
        REQUIRE_FALSE(source.getUCS()->isUCS());
        std::unique_ptr<LC_View> copy{source.clone()};
        REQUIRE(copy->isHasUCS());
        CHECK(copy->getUCS() != source.getUCS());
        CHECK_FALSE(copy->getUCS()->isUCS());
        CHECK(copy->getUCS()->getName() == source.getUCS()->getName());
    }
}

TEST_CASE("LC_ViewList frees its views and their UCSs when destroyed", "[views][ownership]") {
    int live = 0;
    {
        LC_ViewList list;
        list.add(makeView(QStringLiteral("A"), live));
        list.addNew(makeView(QStringLiteral("B"), live));
        REQUIRE(list.count() == 2);
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

TEST_CASE("LC_ViewList::clear and remove free the views", "[views][ownership]") {
    int live = 0;
    LC_ViewList list;
    list.add(makeView(QStringLiteral("A"), live));
    list.add(makeView(QStringLiteral("B"), live));
    list.add(makeView(QStringLiteral("C"), live));
    REQUIRE(live == 3);

    list.remove(QStringLiteral("B"));
    CHECK(live == 2);
    CHECK(list.count() == 2);

    list.clear();
    CHECK(live == 0);
    CHECK(list.isEmpty());
}

TEST_CASE("LC_ViewList frees a view rejected for a duplicate name", "[views][ownership]") {
    int live = 0;
    LC_ViewList list;
    LC_View* kept = makeView(QStringLiteral("A"), live);
    list.add(kept);

    SECTION("add") {
        list.add(makeView(QStringLiteral("A"), live));
    }
    SECTION("addNew") {
        list.addNew(makeView(QStringLiteral("A"), live));
    }
    CHECK(live == 1);
    CHECK(list.count() == 1);
    CHECK(list.find(QStringLiteral("A")) == kept);

    // Adding the view that already holds the name must not free it.
    list.add(kept);
    list.addNew(kept);
    CHECK(live == 1);
    CHECK(list.count() == 1);
    checkRotatedUCS(kept->getUCS());
}

TEST_CASE("RS_Graphic frees its named views when destroyed", "[views][ownership]") {
    application();
    int live = 0;
    {
        RS_Graphic graphic;
        graphic.addNamedView(makeView(QStringLiteral("A"), live));
        graphic.addNamedView(makeView(QStringLiteral("B"), live));
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

// LC_LayersExporter::exportViewsList copies each named view into the export
// drawing with LC_View::clone(). The copies must not share the source views'
// UCSs, or freeing the export drawing would free them under the source.
TEST_CASE("Exported named views do not share the source view's UCS", "[views][ownership]") {
    application();
    int live = 0;
    {
        RS_Graphic source;
        source.initForNewDocument();
        source.addNamedView(makeView(QStringLiteral("A"), live));
        REQUIRE(live == 1);

        LC_LayersExportOptions options;
        options.exportNamedViews = true;
        options.layers.push_back(source.findLayer(QStringLiteral("0")));

        LC_LayersExporter exporter;
        std::vector<LC_LayerExportData> exported;
        exporter.exportLayers(&options, &source, exported);
        REQUIRE(exported.size() == 1);

        RS_Graphic* exportGraphic = exported.front().graphic;
        const LC_View* exportedView = exportGraphic->findNamedView(QStringLiteral("A"));
        REQUIRE(exportedView != nullptr);
        const LC_View* sourceView = source.findNamedView(QStringLiteral("A"));
        CHECK(exportedView->getUCS() != sourceView->getUCS());
        checkRotatedUCS(exportedView->getUCS());

        delete exportGraphic;
        CHECK(live == 1);
        checkRotatedUCS(sourceView->getUCS());
    }
    CHECK(live == 0);
}

// The allocations the leak report traced: LC_GraphicViewport::createNamedView
// and updateNamedView hand each view a freshly allocated UCS. Run under
// `leaks -atExit` or ASan to see that every one of them, including the UCS
// each update replaces, is freed.
TEST_CASE("Named views created through the viewport own their UCS", "[views][ownership]") {
    application();
    RS_Graphic graphic;
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    viewport.loadSettings();

    SECTION("in the WCS the view holds a WCS") {
        LC_View* view = viewport.createNamedView(QStringLiteral("PLAN"));
        graphic.addNamedView(view);
        REQUIRE(view->isHasUCS());
        CHECK_FALSE(view->getUCS()->isUCS());

        viewport.updateNamedView(view);
        REQUIRE(view->isHasUCS());
        CHECK_FALSE(view->getUCS()->isUCS());
    }

    SECTION("in a UCS the view holds its own copy of it") {
        const RS_Vector origin(10.0, 20.0, 0.0);
        viewport.createUCS(origin, M_PI / 6.0);
        const LC_UCSList* ucsList = graphic.getUCSList();
        REQUIRE(ucsList->count() == 2);
        const LC_UCS* listed = ucsList->at(1);

        LC_View* view = viewport.createNamedView(QStringLiteral("SITE"));
        graphic.addNamedView(view);
        REQUIRE(view->isHasUCS());
        CHECK(view->getUCS() != listed);
        CHECK(view->getUCS()->isUCS());
        CHECK(view->getUCS()->isSameTo(listed));
        CHECK(view->getUCS()->getName() == listed->getName());

        viewport.updateNamedView(view);
        REQUIRE(view->isHasUCS());
        CHECK(view->getUCS() != listed);
        CHECK(view->getUCS()->isSameTo(listed));

        // Restoring the view reads its UCS; nothing keeps a pointer to it.
        viewport.restoreView(view);
        CHECK(view->getUCS()->isSameTo(listed));
    }
}
