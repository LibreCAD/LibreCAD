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

TEST_CASE("Suggestions include effective aliases with canonical identities", "[commands][suggestions]") {
    lc::test::application();
    const auto line = RS_COMMANDS->suggestions("LI");
    REQUIRE_FALSE(line.isEmpty());
    CHECK(line.first().text == "li");
    CHECK(line.first().canonical == "line2p");
    CHECK_FALSE(line.first().description.isEmpty());
    for (const auto& entry : line) CHECK(RS_COMMANDS->cmdToAction(entry.text, false) == entry.action);
    CHECK(RS_COMMANDS->suggestions("not-a-real-command").isEmpty());
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QVariant oldDir = RS_SETTINGS->getSettings()->value("/Paths/OtherSettingsDir");
    LC_SET_ONE("Paths", "OtherSettingsDir", dir.path());
    for (const QString& target : {QString("line2p"), QString("circle")}) {
        QFile file(dir.filePath("librecad.alias"));
        REQUIRE(file.open(QIODevice::WriteOnly));
        file.write(QString("phase2alias\t%1\nphase2invalid\tunsupported-command\n").arg(target).toUtf8());
        file.close();
        RS_COMMANDS->updateAlias();
        const auto entries = RS_COMMANDS->suggestions("phase2alias");
        REQUIRE(entries.size() == 1);
        CHECK(entries.first().alias);
        CHECK(entries.first().canonical == target);
        CHECK(entries.first().action == RS_COMMANDS->cmdToAction(target, false));
        CHECK(RS_COMMANDS->suggestions("phase2invalid").isEmpty());
    }
    const auto guard = RS_SETTINGS->beginGroupGuard("Paths");
    if (oldDir.isValid()) RS_SETTINGS->write("OtherSettingsDir", oldDir);
    else RS_SETTINGS->remove("OtherSettingsDir");
}

TEST_CASE("Aliases that differ only in case keep their own actions", "[commands][issue2407]") {
    (void)lc::test::application();

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile aliasFile(dir.filePath("librecad.alias"));
    REQUIRE(aliasFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream(&aliasFile) << "Zq\tline2p\nzQ\tcircle\n";
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
    CHECK(RS_COMMANDS->suggestions("Zq").first().text == "Zq");
    CHECK(RS_COMMANDS->suggestions("zQ").first().text == "zQ");
}
