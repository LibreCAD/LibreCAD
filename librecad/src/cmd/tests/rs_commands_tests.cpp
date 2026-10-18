/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

// Issue #2407: commands typed in another letter case.

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QSettings>
#include <QTextStream>
#include <QVariant>

#include "lc_actiontestsupport.h"
#include "rs_commands.h"
#include "rs_settings.h"

TEST_CASE("Commands match in any letter case when only one action fits", "[commands][issue2407]") {
    (void)lc::test::application();

    CHECK(RS_COMMANDS->cmdToAction("line") == RS2::ActionDrawLine);
    CHECK(RS_COMMANDS->cmdToAction("LINE") == RS2::ActionDrawLine);
    CHECK(RS_COMMANDS->cmdToAction("Circle") == RS2::ActionDrawCircleCenterPoint);
    CHECK(RS_COMMANDS->cmdToAction("PL") == RS2::ActionDrawPolyline);
    CHECK(RS_COMMANDS->keycodeToAction("LI") == RS2::ActionDrawLine);
    CHECK(RS_COMMANDS->cmdToAction("LINEX") == RS2::ActionNone);
}

TEST_CASE("Aliases that differ only in case keep their own actions", "[commands][issue2407]") {
    (void)lc::test::application();

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile aliasFile(dir.filePath("librecad.alias"));
    REQUIRE(aliasFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream(&aliasFile) << QStringLiteral("Zq\tline2p\nzQ\tcircle\n\u7dda\tline2p\n");
    aliasFile.close();

    const QVariant oldDir = RS_SETTINGS->getSettings()->value("/Paths/OtherSettingsDir");
    LC_SET_ONE("Paths", "OtherSettingsDir", dir.path());
    RS_COMMANDS->updateAlias();
    {
        const auto groupGuard = RS_SETTINGS->beginGroupGuard("Paths");
        if (oldDir.isValid()) {
            RS_SETTINGS->write("OtherSettingsDir", oldDir);
        }
        else {
            RS_SETTINGS->remove("OtherSettingsDir");
        }
    }

    CHECK(RS_COMMANDS->cmdToAction("Zq") == RS2::ActionDrawLine);
    CHECK(RS_COMMANDS->cmdToAction("zQ") == RS2::ActionDrawCircleCenterPoint);
    CHECK(RS_COMMANDS->cmdToAction("ZQ") == RS2::ActionNone);
    CHECK(RS_COMMANDS->cmdToAction("zq") == RS2::ActionNone);

    // Non-Latin aliases must work in both the command line and keycode lookup.
    CHECK(RS_COMMANDS->cmdToAction(QStringLiteral("\u7dda")) == RS2::ActionDrawLine);
    CHECK(RS_COMMANDS->keycodeToAction(QStringLiteral("\u7dda")) == RS2::ActionDrawLine);
}

TEST_CASE("Line gap commands start Line Gap, not Break/Divide", "[commands]") {
    (void)lc::test::application();

    CHECK(RS_COMMANDS->cmdToAction("gapline") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->cmdToAction("gl") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->keycodeToAction("gl") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->cmdToAction("breakdivide") == RS2::ActionModifyBreakDivide);
    CHECK(RS_COMMANDS->cmdToAction("bd") == RS2::ActionModifyBreakDivide);
    CHECK(RS_COMMANDS->keycodeToAction("bd") == RS2::ActionModifyBreakDivide);
}

TEST_CASE("Line gap aliases survive saving and respect user overrides", "[commands]") {
    (void)lc::test::application();

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile aliasFile(dir.filePath("librecad.alias"));
    const auto loadAlias = [&aliasFile](const QByteArray& contents) {
        if (!aliasFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        const bool written = aliasFile.write(contents) == contents.size();
        aliasFile.close();
        if (!written) return false;
        RS_COMMANDS->updateAlias();
        return true;
    };
    const QString oldCommand = RS_COMMANDS->getCommandForAction(RS_COMMANDS->cmdToAction("gl", false));
    REQUIRE(!oldCommand.isEmpty());
    const QVariant oldDir = RS_SETTINGS->getSettings()->value("/Paths/OtherSettingsDir");
    LC_SET_ONE("Paths", "OtherSettingsDir", dir.path());
    const auto restore = qScopeGuard([&] {
        loadAlias("gl\t" + oldCommand.toUtf8() + '\n');
        const auto groupGuard = RS_SETTINGS->beginGroupGuard("Paths");
        if (oldDir.isValid()) RS_SETTINGS->write("OtherSettingsDir", oldDir);
        else RS_SETTINGS->remove("OtherSettingsDir");
    });

    RS_COMMANDS->updateAlias();
    REQUIRE(aliasFile.open(QIODevice::ReadOnly));
    CHECK(aliasFile.readAll().contains("gl\tgapline\n"));
    aliasFile.close();
    RS_COMMANDS->updateAlias();
    CHECK(RS_COMMANDS->cmdToAction("gl") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->keycodeToAction("gl") == RS2::ActionModifyLineGap);

    // Existing generated aliases cannot be distinguished from user overrides.
    REQUIRE(loadAlias("gl\tbreakdivide\n"));
    CHECK(RS_COMMANDS->cmdToAction("gapline") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->cmdToAction("gl") == RS2::ActionModifyBreakDivide);
    CHECK(RS_COMMANDS->keycodeToAction("gl") == RS2::ActionModifyBreakDivide);

    REQUIRE(loadAlias("gl\tgapline\n"));
    CHECK(RS_COMMANDS->cmdToAction("gl") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->keycodeToAction("gl") == RS2::ActionModifyLineGap);
    CHECK(RS_COMMANDS->cmdToAction("bd") == RS2::ActionModifyBreakDivide);
    CHECK(RS_COMMANDS->keycodeToAction("bd") == RS2::ActionModifyBreakDivide);
}
