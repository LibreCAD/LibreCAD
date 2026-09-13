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

TEST_CASE("RS_Painter keeps the pen the painter already holds", "[gui][painter]") {
    (void)application();

    // Skipping the call and making it again with the pen already installed are
    // indistinguishable from outside: QPainter::setPen() itself returns early when
    // the pen it is given equals the installed one, so neither the painter nor the
    // pixels can tell which happened. What the cache saves is the work of getting
    // there, which is why it is measured with a call count rather than asserted.
    // One consequence of a skip does show. The solid branch does not compare the
    // painter's join and cap, so a cap change on its own installs nothing - as was
    // the case before any of this caching existed. Read the other way round, a cap
    // still Round after a repeated request is proof the request was answered from
    // the cache: a branch that installed unconditionally would have applied it.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    // Width 2: a black solid pen of width 1 is the pen QPainter starts with, so
    // the request would match the painter as it stands and install nothing, and
    // the cap this case reads back would never have been applied in the first
    // place.
    const RS_Pen solid = testPen(RS2::SolidLine, 2.0);
    painter.setPen(solid);
    REQUIRE(painter.pen().capStyle() == Qt::RoundCap);

    painter.setPenCapStyle(Qt::FlatCap);
    painter.setPen(solid); // byte-identical request
    CHECK(painter.pen().capStyle() == Qt::RoundCap);
    CHECK(painter.pen().color() == QColor(Qt::black));
    CHECK(painter.pen().widthF() == 2.0);
    CHECK(painter.pen().style() == Qt::SolidLine);
}

TEST_CASE("RS_Painter installs a pen the painter no longer holds", "[gui][painter]") {
    (void)application();

    // A pen installed straight on the QPainter base is invisible to this class, so
    // the copy it compares a request against stops describing the painter. The
    // comparison is made against the painter's own pen, so a request cannot be
    // answered with "already installed" when it is not - whatever moved the pen,
    // and whether or not this class knows the member that moved it.
    const QPen marker{QColor(Qt::green)};

    PenTestPainter rig;
    RS_Painter& painter = rig.painter;
    QPainter& asQPainter = rig.painter;

    const RS_Pen solid = testPen(RS2::SolidLine);
    painter.setPen(solid);
    asQPainter.setPen(marker);
    painter.setPen(solid); // byte-identical request
    CHECK(painter.pen().color() == QColor(Qt::black));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().style() == Qt::SolidLine);

    const RS_Pen dashed = testPen(RS2::DashLine);
    painter.setPen(dashed);
    const QPen installed = painter.pen();
    REQUIRE(installed.style() == Qt::CustomDashLine);
    REQUIRE_FALSE(installed.dashPattern().isEmpty());
    asQPainter.setPen(marker);
    painter.setPen(dashed); // byte-identical request
    CHECK(painter.pen().color() == QColor(Qt::black));
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.pen().dashPattern() == installed.dashPattern());
}

TEST_CASE("RS_Painter installs a pen that QPainter::restore() put back",
          "[gui][painter]") {
    (void)application();

    // RS_Painter overrides neither save() nor restore(). LC_OverlayInfoCursor
    // brackets its four colour pens in a save()/restore() pair, and RS_Hatch and
    // LC_Hyperbola bracket their own drawing the same way, so restore() can
    // reinstate a pen this class never chose to install - here the black pen it
    // held at save() time, in place of the red one installed in between.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen black = testPen(RS2::SolidLine, 2.0);
    painter.setPen(black);
    REQUIRE(painter.pen().color() == QColor(Qt::black));
    REQUIRE(painter.pen().widthF() == 2.0);

    painter.save();
    painter.setPen(RS_Color(Qt::red)); // the overlay's colour-only pen
    REQUIRE(painter.pen().color() == QColor(Qt::red));
    painter.restore();
    REQUIRE(painter.pen().color() == QColor(Qt::black));
    REQUIRE(painter.pen().widthF() == 2.0);

    // The request repeats the pen the overlay installed - QPen(QColor) is that
    // colour at width 1 - and not the pen restore() brought back. A copy that had
    // recorded the overlay's pen answers "already installed" and leaves the next
    // entity black at width 2.
    RS_Pen red = testPen(RS2::SolidLine);
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().widthF() == 1.0);
    CHECK(painter.pen().style() == Qt::SolidLine);
}

TEST_CASE("RS_Painter installs the same dashed pen as before", "[gui][painter]") {
    (void)application();

    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen solid = testPen(RS2::SolidLine);
    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK(painter.pen().color() == QColor(Qt::black));
    CHECK(painter.pen().widthF() == 1.0);

    const RS_Pen dashed = testPen(RS2::DashLine);
    painter.setPen(dashed);
    const QPen blackDashed = painter.pen();
    CHECK(blackDashed.style() == Qt::CustomDashLine);
    CHECK(blackDashed.color() == QColor(Qt::black));
    CHECK(blackDashed.widthF() == 1.0);
    CHECK_FALSE(blackDashed.dashPattern().isEmpty());
    CHECK(blackDashed.dashOffset() == 0.0);
    CHECK(blackDashed.capStyle() == Qt::RoundCap);   // the painter's defaults
    CHECK(blackDashed.joinStyle() == Qt::RoundJoin);

    RS_Pen red = dashed;
    red.setColor(RS_Color(Qt::red));
    painter.setPen(red);
    CHECK(painter.pen().color() == QColor(Qt::red));
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // A different dashed line type is a different pattern, and the comparison is
    // keyed on the line type rather than on the expanded pattern, so this is the
    // case that pins it discriminating.
    const RS_Pen dotted = testPen(RS2::DotLine);
    painter.setPen(dotted);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());
    CHECK(painter.pen().dashPattern() != blackDashed.dashPattern());
    painter.setPen(dashed);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());

    // The painter scales the offset into device units, so pin the ratio rather
    // than a value that depends on the device resolution.
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

    const RS_Pen wide = testPen(RS2::DashLine, 2.0);
    painter.setPen(wide);
    CHECK(painter.pen().widthF() == 2.0);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());

    painter.setPen(solid);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK(painter.pen().dashPattern().isEmpty());

    // Join and cap come from the painter, not from the RS_Pen, and they must keep
    // reaching QPainter for a pen that is otherwise unchanged.
    painter.setPen(dashed);
    painter.setPenCapStyle(Qt::FlatCap);
    painter.setPenJoinStyle(Qt::BevelJoin);
    painter.setPen(dashed);
    CHECK(painter.pen().capStyle() == Qt::FlatCap);
    CHECK(painter.pen().joinStyle() == Qt::BevelJoin);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.pen().dashPattern() == blackDashed.dashPattern());
}

TEST_CASE("RS_Painter re-installs a pen that noCapStyle() replaced", "[gui][painter]") {
    (void)application();

    // noCapStyle() installs a pen straight on the QPainter base: the pen it had
    // just been given, with a flat cap. The next request reaches QPainter and puts
    // the painter's own cap style back - the dashed branch compares the cap, and
    // a pen whose cap was changed behind this class's back fails the comparison
    // against the painter in any case. LC_GridSystem is the caller that does this,
    // once per grid pen, and grid pens are often dashed.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen dashed = testPen(RS2::DashLine);
    painter.setPen(dashed);
    REQUIRE(painter.pen().capStyle() == Qt::RoundCap);

    painter.noCapStyle();
    CHECK(painter.pen().capStyle() == Qt::FlatCap);

    painter.setPen(dashed); // byte-identical request
    CHECK(painter.pen().capStyle() == Qt::RoundCap);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK_FALSE(painter.pen().dashPattern().isEmpty());
}

TEST_CASE("RS_Painter re-installs a pen that setPen(RS_Color) replaced", "[gui][painter]") {
    (void)application();

    // Same invariant through the colour-only overload, which the overlays and the
    // print preview use between entity pens: the request that follows it must not
    // be skipped against a pen the painter no longer holds.
    PenTestPainter rig;
    RS_Painter& painter = rig.painter;

    const RS_Pen solid = testPen(RS2::SolidLine);
    painter.setPen(solid);
    REQUIRE(painter.pen().color() == QColor(Qt::black));

    painter.setPen(RS_Color(Qt::red));
    CHECK(painter.pen().color() == QColor(Qt::red));

    painter.setPen(solid); // byte-identical request
    CHECK(painter.pen().color() == QColor(Qt::black));
    CHECK(painter.pen().style() == Qt::SolidLine);

    // The pen that overload installs is QPen(QColor): width 1, with Qt's own
    // SquareCap and BevelJoin rather than the painter's configured Round and
    // Round. A request for that colour at width 1 matches it on everything the
    // solid branch compares, so a record of it would answer "already installed"
    // and leave the square cap and bevel join on the painter.
    painter.setPen(RS_Color(Qt::red));
    REQUIRE(painter.pen().widthF() == 1.0);
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
