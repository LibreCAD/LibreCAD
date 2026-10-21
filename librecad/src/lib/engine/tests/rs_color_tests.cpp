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

// Tests for RS_Color's WCAG 2 helpers (relative luminance, contrast ratio), used by the
// drawing-extents band on the scrollbars (QG_ScrollBar::contentBandColorFor()).

#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "rs_color.h"

using Catch::Approx;

TEST_CASE("RS_Color relative luminance follows WCAG 2", "[rs_color]") {
    CHECK(RS_Color::relativeLuminance(Qt::black) == 0.0);
    CHECK(RS_Color::relativeLuminance(Qt::white) == Approx(1.0));
    // the channel weights
    CHECK(RS_Color::relativeLuminance(QColor(255, 0, 0)) == Approx(0.2126));
    CHECK(RS_Color::relativeLuminance(QColor(0, 255, 0)) == Approx(0.7152));
    CHECK(RS_Color::relativeLuminance(QColor(0, 0, 255)) == Approx(0.0722));
    // sRGB linearisation: mid grey #808080 is about 0.2159, not 0.5
    CHECK(RS_Color::relativeLuminance(QColor(128, 128, 128)) == Approx(0.21586).margin(1e-4));
    // alpha is ignored; an RS_Color's flags are too
    CHECK(RS_Color::relativeLuminance(QColor(128, 128, 128, 10)) == RS_Color::relativeLuminance(QColor(128, 128, 128)));
    RS_Color byLayer(128, 128, 128);
    byLayer.setFlags(RS2::FlagByLayer);
    CHECK(RS_Color::relativeLuminance(byLayer) == RS_Color::relativeLuminance(QColor(128, 128, 128)));
}

TEST_CASE("RS_Color contrast ratio follows WCAG 2", "[rs_color]") {
    CHECK(RS_Color::contrastRatio(Qt::black, Qt::white) == Approx(21.0));
    CHECK(RS_Color::contrastRatio(Qt::white, Qt::black) == Approx(21.0));
    CHECK(RS_Color::contrastRatio(QColor(10, 96, 255), QColor(10, 96, 255)) == 1.0);
    // symmetric
    const QColor accent(10, 96, 255);
    const QColor track(250, 250, 250);
    CHECK(RS_Color::contrastRatio(accent, track) == RS_Color::contrastRatio(track, accent));
    // known values: #767676 on white is the classic 4.54:1, #0a60ff on #fafafa is 4.87:1
    CHECK(RS_Color::contrastRatio(QColor(0x76, 0x76, 0x76), Qt::white) == Approx(4.54).margin(0.01));
    CHECK(RS_Color::contrastRatio(accent, track) == Approx(4.87).margin(0.01));
}
