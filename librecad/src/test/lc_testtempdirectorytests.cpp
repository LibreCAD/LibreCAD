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

// Test files write fixed file names into the temporary directory, so two test
// processes that shared it overwrote each other's files. Each has a directory
// of its own now (see lc_testtempdirectory.h); the first case runs this
// executable again to check that with two real processes.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include "lc_actiontestsupport.h"

namespace {

constexpr auto reportVariable = "LC_TEMP_DIRECTORY_REPORT";
constexpr auto fixedName = "lc_temp_directory_probe.txt";

std::string firstLine(const std::filesystem::path& file) {
    std::ifstream in(file);
    std::string line;
    std::getline(in, line);
    return line;
}

} // namespace

TEST_CASE("a test process does not write into the temporary directory of another",
          "[temp][isolation][process]") {
    (void)lc::test::application();
    const std::filesystem::path own = std::filesystem::temp_directory_path();
    const std::filesystem::path file = own / fixedName;
    const std::filesystem::path report = own / "lc_temp_directory_report.txt";
    std::ofstream(file) << "parent\n";

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QLatin1String(reportVariable), QString::fromStdWString(report.wstring()));
    QProcess child;
    child.setProcessEnvironment(environment);
    child.setProcessChannelMode(QProcess::MergedChannels);
    child.start(QCoreApplication::applicationFilePath(), QStringList{QStringLiteral("[temp_directory_probe]")});
    const bool finished = child.waitForFinished(120000);
    INFO("output of the child process:\n" << child.readAllStandardOutput().toStdString());
    REQUIRE(finished);
    REQUIRE(child.exitStatus() == QProcess::NormalExit);
    REQUIRE(child.exitCode() == 0);

    // the child wrote the same name, into a directory of its own that is gone now
    const std::filesystem::path childDirectory = firstLine(report);
    REQUIRE_FALSE(childDirectory.empty());
    CHECK(childDirectory != own);
    CHECK_FALSE(std::filesystem::exists(childDirectory));
    CHECK(firstLine(file) == "parent");

    std::filesystem::remove(file);
    std::filesystem::remove(report);
}

// Run by the case above, in a process of its own; does nothing when run by hand.
TEST_CASE("temporary directory probe run by the test of another process",
          "[.][temp_directory_probe]") {
    const QString report = qEnvironmentVariable(reportVariable);
    if (report.isEmpty()) {
        return;
    }
    const std::filesystem::path own = std::filesystem::temp_directory_path();
    std::ofstream(own / fixedName) << "child\n";
    std::ofstream(std::filesystem::path(report.toStdWString())) << own.string() << '\n';
}
