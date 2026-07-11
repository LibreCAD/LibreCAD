// File: console_command_utils_tests.cpp

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

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QStringList>

#include "console_command_utils.h"

namespace {

struct ArgvFixture {
    explicit ArgvFixture(const QStringList& args) {
        storage.reserve(args.size());
        for (const QString& arg : args)
            storage.append(arg.toLocal8Bit());
        argv.reserve(storage.size());
        for (QByteArray& arg : storage)
            argv.append(arg.data());
    }

    int argc() const { return argv.size(); }
    char** data() { return argv.data(); }

    QVector<QByteArray> storage;
    QVector<char*> argv;
};

} // namespace

TEST_CASE("console command detection supports subcommand and symlink forms",
          "[console][commands]") {
    ArgvFixture subcommand({"/tmp/librecad", "dwg2pdf", "drawing.dwg"});
    const LC_Console::CommandContext subcommandContext =
        LC_Console::detectCommand(subcommand.argc(), subcommand.data(),
                                  LC_Console::converterCommandNames());
    CHECK(subcommandContext.commandName == "dwg2pdf");
    CHECK(subcommandContext.usesSubcommand);
    CHECK(subcommandContext.displayCommand() == "/tmp/librecad dwg2pdf");

    ArgvFixture symlink({"/tmp/dxf2svg", "drawing.dxf"});
    const LC_Console::CommandContext symlinkContext =
        LC_Console::detectCommand(symlink.argc(), symlink.data(),
                                  LC_Console::converterCommandNames());
    CHECK(symlinkContext.commandName == "dxf2svg");
    CHECK_FALSE(symlinkContext.usesSubcommand);
    CHECK(symlinkContext.displayCommand() == "/tmp/dxf2svg");
}

TEST_CASE("normalized argv removes only the subcommand token",
          "[console][commands]") {
    ArgvFixture args({"/tmp/librecad", "dwg2dxf", "-V", "r2018", "a.dwg"});
    const LC_Console::CommandContext context =
        LC_Console::contextForCommand(args.argc(), args.data(), "dwg2dxf");
    LC_Console::NormalizedArgv normalized(args.argc(), args.data(), context);

    REQUIRE(normalized.argc() == 4);
    CHECK(QString::fromLocal8Bit(normalized.argv()[0]) == "/tmp/librecad");
    CHECK(QString::fromLocal8Bit(normalized.argv()[1]) == "-V");
    CHECK(QString::fromLocal8Bit(normalized.argv()[2]) == "r2018");
    CHECK(QString::fromLocal8Bit(normalized.argv()[3]) == "a.dwg");
}

TEST_CASE("console helper filters input extensions and describes them",
          "[console][commands]") {
    const QStringList extensions =
        LC_Console::acceptedExtensions("dxf", {"dwg", ".DXF"});
    CHECK(extensions == QStringList({"dxf", "dwg"}));
    CHECK(LC_Console::extensionDescription(extensions) == ".dxf/.dwg");

    const QStringList inputs =
        LC_Console::collectInputFiles({"a.dxf", "b.DWG", "notes.txt"},
                                      extensions);
    CHECK(inputs == QStringList({"a.dxf", "b.DWG"}));
    CHECK(LC_Console::containsDwgInput(inputs));
}

TEST_CASE("console helper creates default output paths",
          "[console][commands]") {
    const QString tempDir = QDir::tempPath();
    const QString outDir = tempDir + "/out";
    const QString sourceDir = tempDir + "/source";

    const QString output =
        LC_Console::defaultOutputPath(sourceDir + "/drawing.dwg", "pdf", outDir);
    CHECK(QFileInfo(output).fileName() == "drawing.pdf");
    CHECK(QDir::cleanPath(output).startsWith(QDir::cleanPath(outDir)));

    const QString sameDir =
        LC_Console::defaultOutputPath(sourceDir + "/drawing.dxf", ".svg");
    CHECK(QFileInfo(sameDir).fileName() == "drawing.svg");
    CHECK(QDir::cleanPath(sameDir).startsWith(QDir::cleanPath(sourceDir)));
}

TEST_CASE("console helper validates command output option rules",
          "[console][commands]") {
    QString error;

    CHECK_FALSE(LC_Console::validateOutputOptions(2, "one.png", {}, false,
                                                  false, &error));
    CHECK(error == "-o/--output can only be used with a single input file.");

    CHECK_FALSE(LC_Console::validateOutputOptions(1, "one.png", "/tmp/out",
                                                  false, false, &error));
    CHECK(error == "-o/--output and -t/--directory are mutually exclusive.");

    CHECK(LC_Console::validateOutputOptions(2, "combined.pdf", "/tmp/out",
                                            true, true, &error));
    CHECK(LC_Console::validateOutputOptions(2, {}, "/tmp/out", false, false,
                                            &error));
}
