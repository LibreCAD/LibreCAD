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

#ifndef LC_TESTSETTINGSISOLATION_H
#define LC_TESTSETTINGSISOLATION_H

#include <QSettings>
#include <QString>

namespace lc::test {

/**
 * Gives a test process a settings store of its own.
 *
 * The tests open their settings as organization "LibreCAD", application
 * "LibreCAD-tests" (RS_Settings::init). Left alone, that names one per-user
 * store (a plist on macOS, the registry on Windows, a .conf file on Linux)
 * that every test process on the machine, the developer's other sessions and
 * earlier runs included, reads and writes. What a test reads then depends on
 * which other process wrote last, and a value one run leaves behind (a paper
 * size, a unit, a font directory) reaches the next.
 *
 * begin() makes INI the default QSettings format, which RS_Settings::init
 * opens its store in, and points it at a private temporary directory, so the
 * registry and the user's preferences are left alone on every platform: the
 * store starts empty, sees only what this process writes, and end() removes
 * it when the process exits. (QSettings(organization, application) ignores the
 * default format and always opens the native store; tests that open a store
 * themselves use openSettings() below.) The store is still shared by the tests
 * of the one process, in order, as before: a value one test file writes is
 * what the next one reads.
 *
 * A Catch2 listener in lc_testsettingsisolation.cpp calls begin() before the
 * first test case, so it applies to every executable that lists that file
 * among its sources and no test has to ask for it. Nothing ends the isolation
 * when the last test case does: objects that a test case created, the
 * QC_ApplicationWindow singleton above all, are destroyed while the process exits and
 * write settings as they go, which puts the directory back. begin() registers
 * an exit handler that removes it after those destructors have run.
 */
class SettingsIsolation {
public:
    /**
     * Redirects every QSettings opened from now on. Objects opened before the
     * call keep the store they have. Does nothing when already active.
     */
    static void begin();

    /**
     * Removes the private store. QSettings opened after the call have no
     * store to read, and one that writes recreates the directory, so nothing
     * may use settings afterwards; the exit handler begin() registers is the
     * only caller, and it runs after the static destructors of everything a
     * test case created. Does nothing when not active.
     */
    static void end();

    /// True between begin() and end().
    static bool isActive();

    /// The private directory holding this process's settings, empty when inactive.
    static QString directory();
};

/**
 * Opens the store RS_Settings::init(organization, application) opens, without
 * going through RS_Settings. QSettings(organization, application) always
 * opens the native store, which is not the store the process's settings are
 * kept in while the isolation is active; this opens the default format.
 */
inline QSettings openSettings(const QString& organization, const QString& application) {
    return QSettings(QSettings::defaultFormat(), QSettings::UserScope, organization, application);
}

} // namespace lc::test

#endif
