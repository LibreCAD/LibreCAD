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

// RS_Settings::init() replaces the process-wide settings singleton. Every test
// file that needs settings calls it once, so a test run calls it many times;
// the instance it replaces must be freed, not orphaned along with its cache.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QPointer>

#include "rs_settings.h"

namespace {

// The store every other test file uses, so re-initialising leaves the process
// in the state those files expect.
constexpr auto testOrganization = "LibreCAD";
constexpr auto testApplication = "LibreCAD-tests";

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

} // namespace

TEST_CASE("RS_Settings::init frees the instance it replaces", "[settings][ownership]") {
    application();
    RS_Settings::init(testOrganization, testApplication);
    const QPointer<RS_Settings> first = RS_Settings::instance();
    REQUIRE(!first.isNull());

    RS_Settings::init(testOrganization, testApplication);
    RS_Settings* second = RS_Settings::instance();

    CHECK(first.isNull());
    REQUIRE(second != nullptr);

    SECTION("the replacement is live") {
        const auto group = second->beginGroupGuard(QStringLiteral("LcSettingsInitTest"));
        REQUIRE(second->write(QStringLiteral("Key"), QStringLiteral("value")));
        CHECK(second->readStr(QStringLiteral("Key")) == QStringLiteral("value"));
        second->remove(QStringLiteral("Key"));
    }

    SECTION("a third call replaces the second the same way") {
        const QPointer<RS_Settings> tracked = second;
        RS_Settings::init(testOrganization, testApplication);
        CHECK(tracked.isNull());
        CHECK(RS_Settings::instance() != nullptr);
    }
}
