// File: lc_pdfprintlooptests.cpp

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

#include <cstdlib>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "pdf_print_loop.h"

namespace {

// A drawing holding one line.
constexpr const char* kLineDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n100.0\n21\n50.0\n31\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

QString writeLineDxf(const QTemporaryDir& dir, const QString& name) {
    const QString path = dir.filePath(name);
    QFile file{path};
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(kLineDxf) > 0);
    return path;
}

// The exit code the console command returns for these parameters.
int printAndGetExitCode(const PdfPrintParams& params) {
    PdfPrintLoop loop{params};
    int exitCode = -1;
    QObject::connect(&loop, &PdfPrintLoop::finished, &loop,
                     [&exitCode](const int code) { exitCode = code; });
    loop.run();
    return exitCode;
}

} // namespace

TEST_CASE("PDF printing fails when the output file cannot be written",
          "[console][pdf]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString missingDir = dir.filePath("missing");

    PdfPrintParams params;
    params.inputFiles = {writeLineDxf(dir, "first.dxf")};

    SECTION("one input into a named file") {
        params.outFile = missingDir + "/out.pdf";
        CHECK(printAndGetExitCode(params) == EXIT_FAILURE);
        CHECK_FALSE(QFileInfo::exists(params.outFile));
    }

    SECTION("several inputs into one named file") {
        params.inputFiles.append(writeLineDxf(dir, "second.dxf"));
        params.outFile = missingDir + "/out.pdf";
        CHECK(printAndGetExitCode(params) == EXIT_FAILURE);
        CHECK_FALSE(QFileInfo::exists(params.outFile));
    }

    SECTION("each input to its own file in the output directory") {
        // dxf2pdf creates the output directory before printing. A missing one
        // stands in here for a directory that cannot be written, since a test
        // running as root can write to any directory.
        params.outDir = missingDir;
        CHECK(printAndGetExitCode(params) == EXIT_FAILURE);
        CHECK_FALSE(QFileInfo::exists(missingDir + "/first.pdf"));
    }
}

TEST_CASE("PDF printing succeeds when the output file can be written",
          "[console][pdf]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    PdfPrintParams params;
    params.inputFiles = {writeLineDxf(dir, "first.dxf")};

    SECTION("one input into a named file") {
        params.outFile = dir.filePath("out.pdf");
        CHECK(printAndGetExitCode(params) == EXIT_SUCCESS);
        CHECK(QFileInfo(params.outFile).size() > 0);
    }

    SECTION("each input to its own file in the output directory") {
        params.outDir = dir.path();
        CHECK(printAndGetExitCode(params) == EXIT_SUCCESS);
        CHECK(QFileInfo(dir.filePath("first.pdf")).size() > 0);
    }
}
