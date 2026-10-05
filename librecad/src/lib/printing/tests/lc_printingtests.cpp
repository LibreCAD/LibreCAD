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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QFileInfo>
#include <QImage>
#include <QPrinter>
#include <QTemporaryDir>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_plot_settings.h"
#include "lc_printpreviewviewrenderer.h"
#include "lc_printing.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_units.h"

namespace {

class RecordingLine final : public RS_Line {
public:
    using RS_Line::RS_Line;

    void draw(RS_Painter* painter) override {
        screenWidth = painter->pen().widthF();
        RS_Line::draw(painter);
    }

    double screenWidth = -1.0;
};

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

TEST_CASE("Printed lineweights follow plot scale, not dimension scale", "[printing][pdf][lineweight]") {
    REQUIRE(lc::test::application() != nullptr);
    const double dimScale = GENERATE(0.0, 1.0, 4.0);
    const double paperScale = GENERATE(0.25, 1.0, 2.0);
    const bool scaleWidths = GENERATE(false, true);
    const auto unit = GENERATE(RS2::Millimeter, RS2::Inch);
    const auto width = GENERATE(RS2::Width00, RS2::Width11);
    CAPTURE(dimScale, paperScale, scaleWidths, unit, width);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.setUnit(unit);
    graphic.addVariable("$DIMSCALE", dimScale, 40);
    graphic.getPlotSettings()->setPaperScale(paperScale);
    graphic.setPaperInsertionBase(RS_Vector{0.0, 0.0});
    auto* line = new RecordingLine{&graphic, RS_LineData{
        RS_Units::convert(RS_Vector{20.0, 20.0}, RS2::Millimeter, unit),
        RS_Units::convert(RS_Vector{60.0, 20.0}, RS2::Millimeter, unit)}};
    line->setPen(RS_Pen{RS_Color(Qt::black), width, RS2::SolidLine});
    graphic.addEntity(line);
    const double widthMm = width == RS2::Width00 ? 0.0 : 0.5;
    const double plottedWidth = widthMm * (scaleWidths ? paperScale : 1.0);

    SECTION("print preview") {
        constexpr double pixelsPerMm = 10.0;
        QImage image{1000, 800, QImage::Format_RGB32};
        LC_GraphicViewport viewport;
        viewport.setDocument(&graphic);
        viewport.setSize(image.width(), image.height());
        viewport.justSetOffsetAndFactor(0, 0, pixelsPerMm * RS_Units::getFactorToMM(unit) * paperScale);
        LC_PrintPreviewViewRenderer renderer{&viewport, &image};
        renderer.loadSettings();
        renderer.setLineWidthScaling(scaleWidths);
        renderer.render();
        CHECK(line->screenWidth == Catch::Approx(plottedWidth * pixelsPerMm).margin(1e-7));
    }

    SECTION("GUI PDF export") {
        const int dpi = GENERATE(300, 1200);
        CAPTURE(dpi);
        const QTemporaryDir dir;
        REQUIRE(dir.isValid());
        QPrinter printer{QPrinter::HighResolution};
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(dir.filePath("lineweights.pdf"));
        printer.setFullPage(true);
        printer.setResolution(dpi);
        printer.setPageSize(QPageSize{QPageSize::A4});
        const double pixelsPerMm = (static_cast<double>(printer.width()) / printer.widthMM()
                                  + static_cast<double>(printer.height()) / printer.heightMM()) / 2.0;
        REQUIRE(LC_Printing::printGraphic(printer, graphic, RS2::ModeAuto, scaleWidths));
        CHECK(line->screenWidth == Catch::Approx(plottedWidth * pixelsPerMm).margin(1e-7));
    }
}
