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

// Tests for RS_Settings cross-major migration helpers (copyAll and
// migrateFromPriorMajor). The unit-under-test is the static helpers on
// RS_Settings, not RS_Settings::init() — init() owns a global singleton
// that can't be safely re-initialised between Catch2 cases.
//
// copyAll() works on arbitrary QSettings pairs so we drive it with
// QSettings(filePath, QSettings::IniFormat) instances under a
// QTemporaryDir for hermetic, platform-independent isolation.
//
// migrateFromPriorMajor() probes QSettings(org, app) — i.e. goes through
// the platform-native QSettings backend. We use a unique organization
// name per test case so probes can't pick up real user settings on the
// developer's machine.

#include <catch2/catch_test_macros.hpp>

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>

#include "rs_settings.h"

namespace {

QString uniqueOrgName() {
    return QStringLiteral("LibreCAD-test-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// Clean every QSettings store created under `org` so tests can't leak
// values into the developer's machine config between runs.
void purgeOrgSettings(const QString& org, const QStringList& appNames) {
    for (const QString& app : appNames) {
        QSettings s(org, app);
        s.clear();
        s.sync();
    }
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("copyAll: empty source produces empty destination",
          "[rs_settings][copyAll]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QSettings src(dir.path() + "/src.ini", QSettings::IniFormat);
    QSettings dst(dir.path() + "/dst.ini", QSettings::IniFormat);
    RS_Settings::copyAll(&src, &dst);
    REQUIRE(dst.allKeys().isEmpty());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("copyAll: flat top-level keys are copied with their values",
          "[rs_settings][copyAll]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QSettings src(dir.path() + "/src.ini", QSettings::IniFormat);
    src.setValue("aString", QStringLiteral("hello"));
    src.setValue("anInt", 42);
    src.setValue("aBool", true);
    src.sync();

    QSettings dst(dir.path() + "/dst.ini", QSettings::IniFormat);
    RS_Settings::copyAll(&src, &dst);
    dst.sync();

    REQUIRE(dst.value("aString").toString() == QStringLiteral("hello"));
    REQUIRE(dst.value("anInt").toInt() == 42);
    REQUIRE(dst.value("aBool").toBool() == true);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("copyAll: nested groups are copied recursively",
          "[rs_settings][copyAll]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QSettings src(dir.path() + "/src.ini", QSettings::IniFormat);
    src.setValue("Group1/keyA", QStringLiteral("alpha"));
    src.setValue("Group1/keyB", 7);
    src.setValue("Group2/Nested/keyC", QStringLiteral("gamma"));
    src.sync();

    QSettings dst(dir.path() + "/dst.ini", QSettings::IniFormat);
    RS_Settings::copyAll(&src, &dst);
    dst.sync();

    REQUIRE(dst.value("Group1/keyA").toString() == QStringLiteral("alpha"));
    REQUIRE(dst.value("Group1/keyB").toInt() == 7);
    REQUIRE(dst.value("Group2/Nested/keyC").toString()
            == QStringLiteral("gamma"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("copyAll: top-level underscore-prefixed keys are skipped",
          "[rs_settings][copyAll]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QSettings src(dir.path() + "/src.ini", QSettings::IniFormat);
    src.setValue("_schemaMajor", 99);
    src.setValue("_schemaMinor", 5);
    src.setValue("_migratedFrom", QStringLiteral("LibreCAD-3"));
    src.setValue("_anyUnderscored", QStringLiteral("metaState"));
    src.setValue("realKey", QStringLiteral("payload"));
    src.sync();

    QSettings dst(dir.path() + "/dst.ini", QSettings::IniFormat);
    RS_Settings::copyAll(&src, &dst);
    dst.sync();

    REQUIRE(dst.value("realKey").toString() == QStringLiteral("payload"));
    REQUIRE_FALSE(dst.contains("_schemaMajor"));
    REQUIRE_FALSE(dst.contains("_schemaMinor"));
    REQUIRE_FALSE(dst.contains("_migratedFrom"));
    REQUIRE_FALSE(dst.contains("_anyUnderscored"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("copyAll: underscore-prefixed keys *inside groups* are not skipped",
          "[rs_settings][copyAll]") {
    // The skip rule applies only at the root: a group such as
    // [Appearance] / _internalSetting is user-meaningful and must be
    // preserved.
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QSettings src(dir.path() + "/src.ini", QSettings::IniFormat);
    src.setValue("Appearance/_internalSetting", 12);
    src.sync();

    QSettings dst(dir.path() + "/dst.ini", QSettings::IniFormat);
    RS_Settings::copyAll(&src, &dst);
    dst.sync();

    REQUIRE(dst.value("Appearance/_internalSetting").toInt() == 12);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("migrateFromPriorMajor: returns empty when no prior store exists",
          "[rs_settings][migrate]") {
    const QString org = uniqueOrgName();
    QSettings dst(org, QStringLiteral("LibreCAD-2"));
    dst.clear();
    dst.sync();

    const QString from =
        RS_Settings::migrateFromPriorMajor(org, &dst, 2);

    INFO("returned from = '" << from.toStdString() << "'");
    REQUIRE(from.isEmpty());
    REQUIRE_FALSE(dst.contains("_migratedFrom"));

    purgeOrgSettings(org, {QStringLiteral("LibreCAD-2")});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("migrateFromPriorMajor: copies from a single prior major",
          "[rs_settings][migrate]") {
    const QString org = uniqueOrgName();
    {
        QSettings prior(org, QStringLiteral("LibreCAD-1"));
        prior.clear();
        prior.setValue("Appearance/Language", QStringLiteral("de"));
        prior.setValue("Defaults/Unit", QStringLiteral("Inch"));
        prior.sync();
    }

    QSettings dst(org, QStringLiteral("LibreCAD-3"));
    dst.clear();

    const QString from =
        RS_Settings::migrateFromPriorMajor(org, &dst, 3);
    dst.sync();

    REQUIRE(from == QStringLiteral("LibreCAD-1"));
    REQUIRE(dst.value("Appearance/Language").toString()
            == QStringLiteral("de"));
    REQUIRE(dst.value("Defaults/Unit").toString()
            == QStringLiteral("Inch"));
    REQUIRE(dst.value("_migratedFrom").toString()
            == QStringLiteral("LibreCAD-1"));
    REQUIRE(dst.value("_schemaMajor").toInt() == 3);
    REQUIRE(dst.value("_schemaMinor").toInt() == 0);

    purgeOrgSettings(org, {QStringLiteral("LibreCAD-1"),
                            QStringLiteral("LibreCAD-3")});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("migrateFromPriorMajor: picks the highest-numbered prior",
          "[rs_settings][migrate]") {
    const QString org = uniqueOrgName();
    {
        QSettings prior1(org, QStringLiteral("LibreCAD-1"));
        prior1.clear();
        prior1.setValue("source", QStringLiteral("v1"));
        prior1.sync();

        QSettings prior2(org, QStringLiteral("LibreCAD-2"));
        prior2.clear();
        prior2.setValue("source", QStringLiteral("v2"));
        prior2.sync();

        QSettings prior3(org, QStringLiteral("LibreCAD-3"));
        prior3.clear();
        prior3.setValue("source", QStringLiteral("v3"));
        prior3.sync();
    }

    QSettings dst(org, QStringLiteral("LibreCAD-5"));
    dst.clear();

    const QString from =
        RS_Settings::migrateFromPriorMajor(org, &dst, 5);
    dst.sync();

    REQUIRE(from == QStringLiteral("LibreCAD-3"));
    REQUIRE(dst.value("source").toString() == QStringLiteral("v3"));

    purgeOrgSettings(org, {QStringLiteral("LibreCAD-1"),
                            QStringLiteral("LibreCAD-2"),
                            QStringLiteral("LibreCAD-3"),
                            QStringLiteral("LibreCAD-5")});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("migrateFromPriorMajor: falls back to legacy un-versioned name",
          "[rs_settings][migrate]") {
    const QString org = uniqueOrgName();
    {
        QSettings legacy(org, QStringLiteral("LibreCAD"));
        legacy.clear();
        legacy.setValue("pre-versioned/value", QStringLiteral("kept"));
        legacy.sync();
    }

    QSettings dst(org, QStringLiteral("LibreCAD-2"));
    dst.clear();

    const QString from =
        RS_Settings::migrateFromPriorMajor(org, &dst, 2);
    dst.sync();

    REQUIRE(from == QStringLiteral("LibreCAD"));
    REQUIRE(dst.value("pre-versioned/value").toString()
            == QStringLiteral("kept"));
    REQUIRE(dst.value("_migratedFrom").toString()
            == QStringLiteral("LibreCAD"));

    purgeOrgSettings(org, {QStringLiteral("LibreCAD"),
                            QStringLiteral("LibreCAD-2")});
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("migrateFromPriorMajor: meta-state from prior is not propagated",
          "[rs_settings][migrate]") {
    // The prior store had its own _schemaMajor / _schemaMinor / _migratedFrom
    // sentinels. After migration into a new major, the new store's
    // sentinels must reflect the *current* major, not the prior's.
    const QString org = uniqueOrgName();
    {
        QSettings prior(org, QStringLiteral("LibreCAD-1"));
        prior.clear();
        prior.setValue("real", QStringLiteral("payload"));
        prior.setValue("_schemaMajor", 1);
        prior.setValue("_schemaMinor", 7);
        prior.setValue("_migratedFrom", QStringLiteral("LibreCAD"));
        prior.sync();
    }

    QSettings dst(org, QStringLiteral("LibreCAD-3"));
    dst.clear();

    const QString from =
        RS_Settings::migrateFromPriorMajor(org, &dst, 3);
    dst.sync();

    REQUIRE(from == QStringLiteral("LibreCAD-1"));
    REQUIRE(dst.value("real").toString() == QStringLiteral("payload"));
    REQUIRE(dst.value("_schemaMajor").toInt() == 3);
    REQUIRE(dst.value("_schemaMinor").toInt() == 0);
    REQUIRE(dst.value("_migratedFrom").toString()
            == QStringLiteral("LibreCAD-1"));

    purgeOrgSettings(org, {QStringLiteral("LibreCAD-1"),
                            QStringLiteral("LibreCAD-3")});
}
