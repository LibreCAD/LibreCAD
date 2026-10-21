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

// RS_Settings::init() is called once by the application and once per test file
// by the test helpers. A repeat call must keep the singleton (the application
// connects to it and holds it for its whole life) and switch it to the new
// store: the replaced QSettings is freed, and the old store's cache and open
// group do not carry over.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QObject>
#include <QPointer>
#include <QSettings>

#include "lc_testsettingsisolation.h"
#include "rs_settings.h"

namespace {

// The store every other test file uses, so re-initialising leaves the process
// in the state those files expect.
constexpr auto testOrganization = "LibreCAD";
constexpr auto testApplication = "LibreCAD-tests";

constexpr auto testGroup = "LcSettingsInitTest";

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first.
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool applicationReady = [] {
        QCoreApplication::setOrganizationName(testOrganization);
        QCoreApplication::setApplicationName(testApplication);
        return true;
    }();
    (void)applicationReady;
    return *app;
}

// Removes what the tests wrote and closes any group they left open, whether
// or not a check failed, so later tests see the settings they expect.
class StoreCleanup final {
public:
    ~StoreCleanup() {
        RS_Settings::instance()->endGroup();
        QSettings store = lc::test::openSettings(testOrganization, testApplication);
        store.remove(testGroup);
        store.remove("GroupProbe");
        store.sync();
    }
};

} // namespace

TEST_CASE("RS_Settings::init keeps the singleton and swaps its store", "[settings][ownership]") {
    application();
    const StoreCleanup cleanup;
    RS_Settings::init(testOrganization, testApplication);
    RS_Settings* settings = RS_Settings::instance();
    REQUIRE(settings != nullptr);

    // A QPointer, not an address comparison: a destroyed instance and its
    // replacement often share an address, and only the QPointer notices.
    const QPointer<RS_Settings> tracked = settings;
    const QPointer<QSettings> firstStore = settings->getSettings();
    REQUIRE(!firstStore.isNull());

    SECTION("the instance and its signal connections survive") {
        int optionsChangedCount = 0;
        // the context object drops the connection before the counter goes away
        QObject context;
        QObject::connect(settings, &RS_Settings::optionsChanged, &context,
                         [&optionsChangedCount] { ++optionsChangedCount; });

        for (int i = 0; i < 3; ++i) {
            RS_Settings::init(testOrganization, testApplication);
        }

        CHECK(!tracked.isNull());
        CHECK(RS_Settings::instance() == settings);
        settings->emitOptionsChanged();
        CHECK(optionsChangedCount == 1);
    }

    SECTION("the replaced backing store is freed") {
        RS_Settings::init(testOrganization, testApplication);

        CHECK(firstStore.isNull());
        REQUIRE(settings->getSettings() != nullptr);
        CHECK(settings->getSettings()->applicationName() == QString::fromLatin1(testApplication));
    }

    SECTION("the cache is dropped, so reads see the new store") {
        settings->beginGroup(testGroup);
        REQUIRE(settings->write("Key", QStringLiteral("v1")));
        // proves the value is cached: the store below changes and this read does not
        REQUIRE(settings->readStr("Key") == QStringLiteral("v1"));

        {
            QSettings other = lc::test::openSettings(testOrganization, testApplication);
            other.setValue(QStringLiteral("/%1/Key").arg(testGroup), QStringLiteral("v2"));
            other.sync();
        }
        CHECK(settings->readStr("Key") == QStringLiteral("v1"));

        RS_Settings::init(testOrganization, testApplication);
        settings->beginGroup(testGroup);
        CHECK(settings->readStr("Key") == QStringLiteral("v2"));
    }

    SECTION("an open group does not carry over") {
        settings->beginGroup(testGroup);
        RS_Settings::init(testOrganization, testApplication);

        REQUIRE(settings->write("GroupProbe", QStringLiteral("x")));
        QSettings store = lc::test::openSettings(testOrganization, testApplication);
        CHECK(store.value(QStringLiteral("GroupProbe")).toString() == QStringLiteral("x"));
        CHECK(!store.contains(QStringLiteral("%1/GroupProbe").arg(testGroup)));
    }
}
