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

// Every test process keeps its settings in a private directory (see
// lc_testsettingsisolation.h), so what a test reads does not depend on which
// other process wrote to the per-user store last, and nothing a test writes
// reaches the next run. The second case runs this executable again to check
// that across two real processes.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>

#include "lc_actiontestsupport.h"
#include "lc_testsettingsisolation.h"
#include "rs_settings.h"

namespace {

constexpr auto reportVariable = "LC_SETTINGS_ISOLATION_REPORT";
constexpr auto probeGroup = "LcSettingsIsolationProbe";
constexpr auto childMarkerKey = "ChildMarker";
constexpr auto parentMarkerKey = "ParentMarker";

QString fullKey(const char* key) {
    return QStringLiteral("%1/%2").arg(QLatin1String(probeGroup), QLatin1String(key));
}

// Removes what the cases below wrote to the process's store.
class ProbeCleanup final {
public:
    ~ProbeCleanup() {
        QSettings* store = RS_Settings::instance()->getSettings();
        store->remove(QLatin1String(probeGroup));
        store->sync();
    }
};

bool isInside(const QString& path, const QString& directory) {
    return QDir::cleanPath(path).startsWith(QDir::cleanPath(directory) + QLatin1Char('/'));
}

} // namespace

TEST_CASE("test processes keep their settings in a private directory",
          "[settings][isolation]") {
    (void)lc::test::application();
    REQUIRE(lc::test::SettingsIsolation::isActive());

    const QString directory = lc::test::SettingsIsolation::directory();
    REQUIRE(QFileInfo(directory).isDir());
    // a directory of the process's own, not the user's configuration
    CHECK(QFileInfo(directory).canonicalFilePath().startsWith(
        QFileInfo(QDir::tempPath()).canonicalFilePath()));

    // INI on every platform: the registry and the preferences plist cannot be moved
    CHECK(QSettings::defaultFormat() == QSettings::IniFormat);
    const QSettings* store = RS_Settings::instance()->getSettings();
    REQUIRE(store != nullptr);
    CHECK(store->format() == QSettings::IniFormat);
    CHECK(isInside(store->fileName(), directory));

    // and a store opened directly, not through RS_Settings, is the same file
    const QSettings plain = lc::test::openSettings(QStringLiteral("LibreCAD"), QStringLiteral("LibreCAD-tests"));
    CHECK(plain.fileName() == store->fileName());
}

TEST_CASE("a test process does not see what another test process writes",
          "[settings][isolation][process]") {
    (void)lc::test::application();
    const ProbeCleanup cleanup;
    RS_Settings* settings = RS_Settings::instance();
    REQUIRE(settings != nullptr);
    settings->writeSingle(QLatin1String(probeGroup), QLatin1String(parentMarkerKey),
                          QStringLiteral("written by the parent"));
    settings->getSettings()->sync();

    QTemporaryDir reportDirectory;
    REQUIRE(reportDirectory.isValid());
    const QString reportPath = reportDirectory.filePath(QStringLiteral("report.txt"));

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QLatin1String(reportVariable), reportPath);
    QProcess child;
    child.setProcessEnvironment(environment);
    child.setProcessChannelMode(QProcess::MergedChannels);
    child.start(QCoreApplication::applicationFilePath(),
                QStringList{QStringLiteral("[settings_isolation_probe]")});
    REQUIRE(child.waitForStarted(30000));
    const bool finished = child.waitForFinished(120000);
    const QString childOutput = QString::fromLocal8Bit(child.readAllStandardOutput());
    INFO("output of the child process:\n" << childOutput.toStdString());
    REQUIRE(finished);
    REQUIRE(child.exitStatus() == QProcess::NormalExit);
    REQUIRE(child.exitCode() == 0);

    QFile reportFile(reportPath);
    REQUIRE(reportFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString childDirectory;
    QString childStore;
    QString sawParent;
    QString sawChild;
    while (!reportFile.atEnd()) {
        const QString line = QString::fromUtf8(reportFile.readLine()).trimmed();
        const qsizetype split = line.indexOf(QLatin1Char('='));
        const QString name = line.left(split);
        const QString value = line.mid(split + 1);
        if (name == QLatin1String("directory")) {
            childDirectory = value;
        }
        else if (name == QLatin1String("store")) {
            childStore = value;
        }
        else if (name == QLatin1String("sawParentMarker")) {
            sawParent = value;
        }
        else if (name == QLatin1String("wroteChildMarker")) {
            sawChild = value;
        }
    }

    const QString parentDirectory = lc::test::SettingsIsolation::directory();
    REQUIRE_FALSE(childDirectory.isEmpty());
    CHECK(childDirectory != parentDirectory);
    CHECK(isInside(childStore, childDirectory));
    // the child removed its store when it ended
    CHECK_FALSE(QFileInfo::exists(childDirectory));

    // it did not see what this process wrote, nor this process what it wrote
    CHECK(sawParent == QStringLiteral("0"));
    CHECK(sawChild == QStringLiteral("1"));
    settings->getSettings()->sync();
    CHECK_FALSE(settings->getSettings()->contains(fullKey(childMarkerKey)));

    // and neither reached the per-user store the tests used to share
    QSettings user(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("LibreCAD"),
                   QStringLiteral("LibreCAD-tests"));
    const bool leaked = user.contains(fullKey(childMarkerKey)) || user.contains(fullKey(parentMarkerKey));
    if (leaked) {
        user.remove(QLatin1String(probeGroup));
        user.sync();
    }
    CHECK_FALSE(leaked);
}

// Run by the case above, in a process of its own; does nothing when run by hand.
TEST_CASE("settings isolation probe run by the test of another process",
          "[.][settings_isolation_probe]") {
    const QString reportPath = qEnvironmentVariable(reportVariable);
    if (reportPath.isEmpty()) {
        return;
    }
    (void)lc::test::application();
    RS_Settings* settings = RS_Settings::instance();
    REQUIRE(settings != nullptr);

    const bool sawParentMarker = settings->getSettings()->contains(fullKey(parentMarkerKey));
    settings->writeSingle(QLatin1String(probeGroup), QLatin1String(childMarkerKey),
                          QUuid::createUuid().toString(QUuid::WithoutBraces));
    settings->getSettings()->sync();
    const bool wroteChildMarker = settings->getSettings()->contains(fullKey(childMarkerKey));

    QFile report(reportPath);
    REQUIRE(report.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&report);
    out << "directory=" << lc::test::SettingsIsolation::directory() << '\n'
        << "store=" << settings->getSettings()->fileName() << '\n'
        << "sawParentMarker=" << (sawParentMarker ? 1 : 0) << '\n'
        << "wroteChildMarker=" << (wroteChildMarker ? 1 : 0) << '\n';
}
