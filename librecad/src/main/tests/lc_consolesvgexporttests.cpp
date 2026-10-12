// File: lc_consolesvgexporttests.cpp

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 raffa <raffa00025@gmail.com>
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

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QSize>
#include <QTemporaryDir>

#include "console_command_utils.h"
#include "console_dxf2png.h"
#include "lc_actiontestsupport.h"
#include "lc_plot_settings.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

// Makes the font sources of the source tree available, the same way the
// text tests do: the export computes the text geometry from these fonts.
class SourceFonts {
public:
    SourceFonts() {
        static bool systemReady = [] {
            RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
            return true;
        }();
        (void)systemReady;

        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Fonts", "");
        LC_SET("Fonts", QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }

    ~SourceFonts() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Fonts", m_previous);
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }

private:
    QString m_previous;
};

// A drawing with four lines framing a box and a TEXT and an MTEXT entity
// inside it. The lines are written by every export backend; the text is
// written only by a painter-based export: LC_MakerCamSVG has no TEXT or
// MTEXT case and silently skips both. The text lies inside the line
// geometry because the viewport fit is based on the borders (the text
// entities do not extend them) and entities outside the clip are skipped.
constexpr const char* kTextDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\n0\n10\n-10.0\n20\n-10.0\n30\n0.0\n11\n210.0\n21\n-10.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n210.0\n20\n-10.0\n30\n0.0\n11\n210.0\n21\n110.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n210.0\n20\n110.0\n30\n0.0\n11\n-10.0\n21\n110.0\n31\n0.0\n"
    "0\nLINE\n8\n0\n10\n-10.0\n20\n110.0\n30\n0.0\n11\n-10.0\n21\n-10.0\n31\n0.0\n"
    "0\nTEXT\n8\n0\n10\n10.0\n20\n20.0\n30\n0.0\n40\n10.0\n1\nDRAWING NUMBER 4711\n"
    "0\nMTEXT\n8\n0\n10\n10.0\n20\n50.0\n30\n0.0\n40\n10.0\n1\nSHEET TITLE 12345\n"
    "0\nENDSEC\n0\nEOF\n";

QString writeFile(const QTemporaryDir& dir, const QString& name, const char* content) {
    const QString path = dir.filePath(name);
    QFile file{path};
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(content) > 0);
    return path;
}

// The same preparation the console converters apply before exporting.
void prepareForExport(RS_Graphic& graphic) {
    graphic.calculateBorders();
    LC_PlotSettings* ps = graphic.getPlotSettings();
    ps->setMarginsInMm(-1.0, -1.0, -1.0, -1.0);
    ps->setPagesNum(0, 0);
    graphic.fitToPage();
}

void importTextDrawing(const QTemporaryDir& dir, RS_Graphic& graphic) {
    REQUIRE(LC_Console::importGraphic(graphic, writeFile(dir, "text.dxf", kTextDxf)));
    CHECK(graphic.count() == 6);
    prepareForExport(graphic);
}

} // namespace

TEST_CASE("console SVG export reports success and writes the file", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    RS_Graphic graphic;
    importTextDrawing(dir, graphic);
    const QString out = dir.filePath("text.svg");

    CHECK(slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5),
                          false, false));
    CHECK(QFileInfo(out).isFile());
    CHECK(QFileInfo(out).size() > 0);
}

TEST_CASE("console SVG export draws text", "[console][svg]") {
    REQUIRE(lc::test::application() != nullptr);
    SourceFonts fonts;
    const QTemporaryDir dir;
    REQUIRE(dir.isValid());

    RS_Graphic graphic;
    importTextDrawing(dir, graphic);
    const QString out = dir.filePath("text.svg");

    CHECK(slotFileExport(&graphic, out, "SVG", QSize(1200, 800), QSize(5, 5),
                          false, false));

    QFile file{out};
    REQUIRE(file.open(QIODevice::ReadOnly));
    const QByteArray svg = file.readAll();

    // The text is painted through the same RS_Painter as the raster formats,
    // which writes the glyphs as vector paths. An export that skips text
    // writes no path for this drawing.
    CHECK(svg.contains("<path"));
}
