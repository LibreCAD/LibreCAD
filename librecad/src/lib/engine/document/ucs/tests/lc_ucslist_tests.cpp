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

// LC_UCSList owns every LC_UCS it keeps (except its built-in WCS, which a
// unique_ptr owns). These tests pin that contract: entries are freed with the
// list, candidates the list rejects are freed on the spot, and the cached
// active entry never outlives the entry it points at.

#include <cmath>

#include <catch2/catch_test_macros.hpp>

#include <QApplication>

#include "lc_graphicviewport.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
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

// Counts live instances so a test can tell whether the list freed an entry.
class CountedUCS final : public LC_UCS {
public:
    CountedUCS(const QString& name, int& liveCount)
        : LC_UCS(name), m_liveCount{liveCount} {
        ++m_liveCount;
    }
    ~CountedUCS() override { --m_liveCount; }

private:
    int& m_liveCount;
};

// A UCS rotated 90 degrees about a non-zero origin, so it never matches the WCS.
CountedUCS* makeRotatedUCS(int& liveCount, const QString& name = QString()) {
    auto* ucs = new CountedUCS(name, liveCount);
    ucs->setOrigin(RS_Vector(10.0, 20.0, 0.0));
    ucs->setXAxis(RS_Vector(0.0, 1.0, 0.0));
    ucs->setYAxis(RS_Vector(-1.0, 0.0, 0.0));
    return ucs;
}

} // namespace

TEST_CASE("LC_UCSList frees the UCSs it holds when destroyed", "[ucs][ownership]") {
    int live = 0;
    {
        LC_UCSList list;
        LC_UCS* rotated = makeRotatedUCS(live);
        REQUIRE(list.tryAddUCS(rotated) == rotated);
        list.add(new CountedUCS(QStringLiteral("NAMED"), live));
        REQUIRE(list.count() == 3); // WCS + the two above
        REQUIRE(live == 2);
    }
    // The WCS is owned separately; a double delete of it would abort here.
    CHECK(live == 0);
}

TEST_CASE("RS_Graphic frees its UCS list entries when destroyed", "[ucs][ownership]") {
    application();
    int live = 0;
    {
        RS_Graphic graphic;
        graphic.addUCS(new CountedUCS(QStringLiteral("SITE"), live));
        LC_UCS* rotated = makeRotatedUCS(live);
        REQUIRE(graphic.getUCSList()->tryAddUCS(rotated) == rotated);
        REQUIRE(live == 2);
    }
    CHECK(live == 0);
}

TEST_CASE("LC_UCSList::tryAddUCS frees a candidate it does not keep", "[ucs][ownership]") {
    int live = 0;
    LC_UCSList list;
    LC_UCS* first = makeRotatedUCS(live);
    REQUIRE(list.tryAddUCS(first) == first);

    SECTION("an equivalent candidate resolves to the existing entry and is freed") {
        LC_UCS* duplicate = makeRotatedUCS(live);
        REQUIRE(live == 2);
        CHECK(list.tryAddUCS(duplicate) == first);
        CHECK(live == 1);
        CHECK(list.count() == 2);
    }

    SECTION("a candidate equivalent to the WCS resolves to the WCS and is freed") {
        auto* wcsLike = new CountedUCS(QString(), live);
        REQUIRE(live == 2);
        CHECK(list.tryAddUCS(wcsLike) == list.getWCS());
        CHECK(live == 1);
        CHECK(list.count() == 2);
    }

    SECTION("re-offering an entry the list already holds keeps it") {
        CHECK(list.tryAddUCS(first) == first);
        CHECK(live == 1);
        CHECK(list.count() == 2);
    }

    SECTION("re-offering the WCS keeps it") {
        CHECK(list.tryAddUCS(list.getWCS()) == list.getWCS());
        CHECK(list.count() == 2);
    }
}

TEST_CASE("LC_UCSList::add frees a UCS whose name is already taken", "[ucs][ownership]") {
    int live = 0;
    LC_UCSList list;
    auto* named = makeRotatedUCS(live, QStringLiteral("SITE"));
    list.add(named);
    REQUIRE(list.find(QStringLiteral("SITE")) == named);

    list.add(new CountedUCS(QStringLiteral("SITE"), live));
    CHECK(live == 1);
    CHECK(list.find(QStringLiteral("SITE")) == named);

    // Adding the entry that already holds the name must not free it.
    list.add(named);
    CHECK(live == 1);
    CHECK(list.count() == 2);

    // A clone of the WCS carries the WCS name, so the list rejects and frees it
    // (LC_LayersExporter::exportUCSList relies on this for the WCS row).
    list.add(list.getWCS()->clone());
    CHECK(list.count() == 2);
}

TEST_CASE("Removing the active UCS clears the list's active entry", "[ucs][ownership]") {
    int live = 0;
    LC_UCSList list;
    LC_UCS* ucs = makeRotatedUCS(live, QStringLiteral("SITE"));
    REQUIRE(list.tryAddUCS(ucs) == ucs);
    list.tryToSetActive(ucs);
    REQUIRE(list.getActive() == ucs);

    list.remove(ucs);
    CHECK(live == 0);
    CHECK(list.getActive() == nullptr);

    // Switching to the WCS reads the previous active entry; it must not be the
    // freed one (LC_UCSListWidget does this right after removing a UCS).
    list.tryToSetActive(list.getWCS());
    CHECK(list.getActive() == list.getWCS());
}

TEST_CASE("LC_UCSList::clear frees every entry except the WCS", "[ucs][ownership]") {
    int live = 0;
    LC_UCSList list;
    LC_UCS* ucs = makeRotatedUCS(live);
    list.tryAddUCS(ucs);
    list.add(new CountedUCS(QStringLiteral("NAMED"), live));
    list.tryToSetActive(ucs);
    REQUIRE(live == 2);

    list.clear();
    CHECK(live == 0);
    CHECK(list.count() == 1);
    CHECK(list.at(0) == list.getWCS());
    CHECK(list.getActive() == nullptr);
}

// The allocation the leak report traced: LC_GraphicViewport::createUCS ->
// createUCSEntity -> tryAddUCS. Run under `leaks -atExit` or ASan to see that
// both the kept entry and the rejected duplicate are freed.
TEST_CASE("UCSs created through the viewport are freed with the drawing", "[ucs][ownership]") {
    application();
    RS_Graphic graphic;
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    viewport.loadSettings();

    const RS_Vector origin(10.0, 20.0, 0.0);
    const double angle = M_PI / 6.0;
    viewport.createUCS(origin, angle);
    LC_UCSList* ucsList = graphic.getUCSList();
    REQUIRE(ucsList->count() == 2);
    const LC_UCS* created = ucsList->at(1);

    // The same UCS again resolves to the existing entry; the fresh candidate
    // is freed rather than dropped.
    viewport.createUCS(origin, angle);
    CHECK(ucsList->count() == 2);
    CHECK(ucsList->at(1) == created);
}
