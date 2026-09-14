// File: lc_printingtests.cpp

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

#include <QFileInfo>
#include <QPrinter>
#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "lc_printing.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

// A new drawing holding one line.
struct LineDrawing {
    RS_Graphic graphic;

    LineDrawing() {
        graphic.initForNewDocument();
        graphic.addEntity(new RS_Line{&graphic, RS_LineData{RS_Vector{0., 0.}, RS_Vector{100., 50.}}});
    }
};

// Prints the drawing into a PDF file, as Export to PDF does.
bool printToPdf(LineDrawing& drawing, const QString& fileName) {
    QPrinter printer{QPrinter::HighResolution};
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    return LC_Printing::printGraphic(printer, drawing.graphic, RS2::ModeAuto, false);
}

} // namespace

TEST_CASE("Export to PDF reports a file it cannot write", "[printing][pdf]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    LineDrawing drawing;

    // A file in a directory that does not exist cannot be opened, even by root.
    const QString fileName = dir.filePath("missing/drawing.pdf");
    CHECK_FALSE(printToPdf(drawing, fileName));
    CHECK_FALSE(QFileInfo::exists(fileName));
}

TEST_CASE("Export to PDF writes a file it can write", "[printing][pdf]") {
    REQUIRE(lc::test::application() != nullptr);
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());
    LineDrawing drawing;

    const QString fileName = dir.filePath("drawing.pdf");
    CHECK(printToPdf(drawing, fileName));
    CHECK(QFileInfo(fileName).size() > 0);
}
