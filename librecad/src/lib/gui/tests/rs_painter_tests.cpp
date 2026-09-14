/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Regression tests for RS_Painter::setPen(): which QPen it installs for a given
// RS_Pen, and when it may keep the pen it already holds instead.

#include <cmath>

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QImage>

#include "rs_color.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    // Reuse the process-wide QApplication if another test file built it first,
    // and leak the pointer on the create path: only one QApplication may exist
    // at a time, and only one ~QApplication may run at exit. Test files are
    // linked into one executable, so this helper is per translation unit;
    // rs_graphicview_close_tests.cpp carries the same one.
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
    return *app;
}

/**
 * A painter on a plain image. RS_Painter::setPen() reads only the drawing mode,
 * the device resolution and the painter's current join/cap styles, so no
 * viewport and no renderer are needed; rs_hatch_tests.cpp builds its test
 * painter on a QImage the same way.
 */
struct PenTestPainter {
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
};

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

} // namespace

TEST_CASE("RS_Painter paints solid for a line type with no pattern",
          "[gui][painter][linetype]") {
    (void)application();

    // RS2::LineTypeUnchanged is the one enumerator RS_LineTypePattern::getPattern()
    // has no table entry for; the out-of-range value stands for a stale settings
    // integer, which is how the grid, snap and overlay pens reach the painter.
    for (const RS2::LineType lineType : {RS2::LineTypeUnchanged, static_cast<RS2::LineType>(999)}) {
        PenTestPainter rig;
        rig.painter.setPen(testPen(lineType));

        CHECK(rig.painter.pen().style() == Qt::SolidLine);
        CHECK(rig.painter.pen().dashPattern().isEmpty());
    }
}

TEST_CASE("RS_Painter answers a repeated request with the same pen",
          "[gui][painter]") {
    (void)application();

    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;

        const RS_Pen requested = testPen(lineType, 2.0);
        painter.setPen(requested);
        const QPen first = painter.pen();
        painter.setPen(requested);
        CHECK(painter.pen() == first);

        // Join and cap come from the painter, and a change reaches QPainter on the
        // next request even when the RS_Pen is the same.
        painter.setPenCapStyle(Qt::FlatCap);
        painter.setPenJoinStyle(Qt::BevelJoin);
        painter.setPen(requested);
        CHECK(painter.pen().capStyle() == Qt::FlatCap);
        CHECK(painter.pen().joinStyle() == Qt::BevelJoin);
        CHECK(painter.pen().style() == first.style());
        CHECK(painter.pen().dashPattern() == first.dashPattern());
    }
}

TEST_CASE("RS_Painter installs a pen the painter no longer holds", "[gui][painter]") {
    (void)application();

    // A pen set on the QPainter base replaces the painter's pen without
    // RS_Painter knowing; the next identical request must still install.
    const QPen marker{QColor(Qt::green)};

    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;
        QPainter& asQPainter = rig.painter;

        const RS_Pen requested = testPen(lineType);
        painter.setPen(requested);
        const QPen installed = painter.pen();
        asQPainter.setPen(marker);
        painter.setPen(requested);
        CHECK(painter.pen() == installed);
    }
}

TEST_CASE("RS_Painter installs a pen that QPainter::restore() put back",
          "[gui][painter]") {
    (void)application();

    // RS_Painter overrides neither save() nor restore(); LC_OverlayInfoCursor
    // sets colour pens inside such a pair.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    painter.setPen(testPen(RS2::SolidLine, 2.0));
    painter.save();
    painter.setPen(RS_Color(Qt::red));
    painter.restore();
    REQUIRE(painter.pen().color() == QColor(Qt::black));
    REQUIRE(painter.pen().widthF() == 2.0);

    RS_Pen red = testPen(RS2::SolidLine);
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().style() == Qt::SolidLine);
}

TEST_CASE("RS_Painter builds dashed pens from line type, width and offset",
          "[gui][painter]") {
    (void)application();

    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen solid = testPen(RS2::SolidLine);
    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);

    const RS_Pen dashed = testPen(RS2::DashLine);
    painter.setPen(dashed);
    const QPen blackDashed = painter.pen();
    CHECK(blackDashed.style() == Qt::CustomDashLine);
    CHECK(blackDashed.color() == QColor(Qt::black));
    CHECK(blackDashed.widthF() == 1.0);
    CHECK_FALSE(blackDashed.dashPattern().isEmpty());
    CHECK(blackDashed.dashOffset() == 0.0);
    CHECK(blackDashed.capStyle() == Qt::RoundCap);
    CHECK(blackDashed.joinStyle() == Qt::RoundJoin);

    RS_Pen red = dashed;
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // The pattern is keyed on the line type, not on the expanded pattern.
    painter.setPen(testPen(RS2::DotLine));
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());
    CHECK(painter.pen().dashPattern() != blackDashed.dashPattern());
    painter.setPen(dashed);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // The offset is scaled to device units, so compare a ratio.
    RS_Pen phaseOne = dashed;
    phaseOne.setDashOffset(-1.0);
    painter.setPen(phaseOne);
    const double offsetOne = painter.pen().dashOffset();
    RS_Pen phaseThree = dashed;
    phaseThree.setDashOffset(-3.0);
    painter.setPen(phaseThree);
    const double offsetThree = painter.pen().dashOffset();
    CHECK(offsetOne != 0.0);
    CHECK(std::abs(offsetThree - 3.0 * offsetOne) <= 1e-9 * std::abs(offsetOne));
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    painter.setPen(testPen(RS2::DashLine, 2.0));
    CHECK(painter.pen().widthF() == 2.0);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());

    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK(painter.pen().dashPattern().isEmpty());
    painter.setPen(dashed);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());
    CHECK(painter.pen().dashOffset() == 0.0);
}

TEST_CASE("RS_Painter re-installs a pen that noCapStyle() replaced", "[gui][painter]") {
    (void)application();

    // LC_GridSystem sets a grid pen, then noCapStyle(). The next request, even for
    // the same pen, gets the painter's configured cap back.
    for (const RS2::LineType lineType : {RS2::SolidLine, RS2::DashLine}) {
        PenTestPainter rig;
        RS_Painter& painter = rig.painter;

        const RS_Pen requested = testPen(lineType);
        painter.setPen(requested);
        painter.noCapStyle();
        REQUIRE(painter.pen().capStyle() == Qt::FlatCap);

        painter.setPen(requested);
        CHECK(painter.pen().capStyle() == Qt::RoundCap);
        CHECK(painter.pen().style() == (lineType == RS2::SolidLine ? Qt::SolidLine : Qt::CustomDashLine));
    }
}

TEST_CASE("RS_Painter re-installs a pen that setPen(RS_Color) replaced", "[gui][painter]") {
    (void)application();

    // setPen(RS_Color) installs QPen(QColor): width 1 with Qt's SquareCap and
    // BevelJoin. A request for that colour at width 1 must still get the painter's
    // configured cap and join.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    painter.setPen(testPen(RS2::SolidLine));
    painter.setPen(RS_Color(Qt::red));
    REQUIRE(painter.pen().capStyle() == Qt::SquareCap);
    REQUIRE(painter.pen().joinStyle() == Qt::BevelJoin);

    RS_Pen red = testPen(RS2::SolidLine);
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().capStyle() == Qt::RoundCap);
    CHECK(painter.pen().joinStyle() == Qt::RoundJoin);
}
