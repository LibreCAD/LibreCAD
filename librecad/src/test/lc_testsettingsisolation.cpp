/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "lc_testsettingsisolation.h"

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <cstdlib>
#include <initializer_list>
#include <memory>

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>

namespace lc::test {

namespace {

std::unique_ptr<QTemporaryDir> settingsDirectory;

} // namespace

void SettingsIsolation::begin() {
    if (settingsDirectory != nullptr) {
        return;
    }
    auto directory = std::make_unique<QTemporaryDir>(
        QDir::temp().filePath(QStringLiteral("librecad-test-settings-XXXXXX")));
    if (!directory->isValid()) {
        // Running on without it would read and write the user's real settings.
        qFatal("Cannot create a private settings directory: %s",
               qPrintable(directory->errorString()));
    }

    // INI everywhere: the native store is the registry on Windows and a plist
    // on macOS, neither of which can be moved to a directory of our choosing.
    // NativeFormat is redirected as well because it is an INI file on Unix, so
    // a store opened with it explicitly stays inside the directory there.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    const QString root = directory->path();
    for (const QSettings::Format format : {QSettings::IniFormat, QSettings::NativeFormat}) {
        QSettings::setPath(format, QSettings::UserScope, root + QStringLiteral("/user"));
        QSettings::setPath(format, QSettings::SystemScope, root + QStringLiteral("/system"));
    }
    settingsDirectory = std::move(directory);

    // The directory is removed by this handler, when the process exits, and by
    // nothing earlier. exit() runs atexit handlers and the destructors of
    // statics in reverse order of their construction, so a handler registered
    // here, before any test case has created anything, runs after the
    // destructor of every object a test case created. That matters: the
    // application window is a function-local static that a test creates, its
    // widgets write settings from their destructors (ColorWizard opens a
    // QSettings, QG_CommandWidget writes through RS_Settings), and every one of
    // those writes creates the store's directory again. Removing it any
    // sooner, when the last test case ends, leaves that directory behind.
    static const bool exitHandlerRegistered = [] {
        return std::atexit(&SettingsIsolation::end) == 0;
    }();
    (void)exitHandlerRegistered;
}

void SettingsIsolation::end() {
    settingsDirectory.reset();
}

bool SettingsIsolation::isActive() {
    return settingsDirectory != nullptr;
}

QString SettingsIsolation::directory() {
    return settingsDirectory != nullptr ? settingsDirectory->path() : QString();
}

namespace {

// Catch2 starts a listener before the first test case, so no test file has to
// ask for the isolation and none can forget to. There is deliberately no
// testRunEnded(): the directory is removed by the exit handler begin()
// registers, see there.
class SettingsIsolationListener final : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(const Catch::TestRunInfo&) override { SettingsIsolation::begin(); }
};

} // namespace

} // namespace lc::test

CATCH_REGISTER_LISTENER(lc::test::SettingsIsolationListener)
