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

// Changing the unit of a drawing changes the numbers of its paper, not its
// physical size, and it is not a choice of paper by the user: the size the
// settings remember for new drawings (in millimetres) must come out unchanged.
// It used to be overwritten with the converted numbers read as millimetres, so
// every drawing set to inches divided the remembered page by 25.4, and the
// pages of the drawings made afterwards shrank with it.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "lc_actiontestsupport.h"
#include "lc_plot_settings.h"
#include "lc_settingguard.h"
#include "rs.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_units.h"

namespace {

using lc::test::SettingGuard;

constexpr double a4WidthMm = 210.0;
constexpr double a4HeightMm = 297.0;

// The page of a new drawing: the remembered size in millimetres and the unit
// new drawings are made in, pinned to values a test can compute with.
class NewDrawingDefaults {
public:
    explicit NewDrawingDefaults(const QString& unit)
        : m_unit{RS_SETTINGS, "Defaults", "Unit"},
          m_paperWidth{RS_SETTINGS, "Print", "PaperSizeX"},
          m_paperHeight{RS_SETTINGS, "Print", "PaperSizeY"} {
        m_unit.set(unit);
        m_paperWidth.set(a4WidthMm);
        m_paperHeight.set(a4HeightMm);
    }

private:
    SettingGuard m_unit;
    SettingGuard m_paperWidth;
    SettingGuard m_paperHeight;
};

double rememberedWidthMm() {
    return LC_GET_ONE_STR("Print", "PaperSizeX", "0").toDouble();
}

double rememberedHeightMm() {
    return LC_GET_ONE_STR("Print", "PaperSizeY", "0").toDouble();
}

RS_Vector paperOf(const RS_Graphic& graphic) {
    return graphic.getPlotSettings()->getPaperSize();
}

} // namespace

TEST_CASE("changing the unit of a drawing converts its paper and keeps the remembered paper size",
          "[graphic][units][paper]") {
    (void)lc::test::application();
    const NewDrawingDefaults defaults{QStringLiteral("None")};

    RS_Graphic graphic;
    REQUIRE(paperOf(graphic).x == Catch::Approx(a4WidthMm));
    REQUIRE(paperOf(graphic).y == Catch::Approx(a4HeightMm));

    SECTION("into inches") {
        graphic.setUnit(RS2::Inch);
        CHECK(graphic.getUnit() == RS2::Inch);
        CHECK(paperOf(graphic).x == Catch::Approx(a4WidthMm / 25.4));
        CHECK(paperOf(graphic).y == Catch::Approx(a4HeightMm / 25.4));
    }

    SECTION("into meters") {
        graphic.setUnit(RS2::Meter);
        CHECK(paperOf(graphic).x == Catch::Approx(a4WidthMm / 1000.0));
        CHECK(paperOf(graphic).y == Catch::Approx(a4HeightMm / 1000.0));
    }

    SECTION("again and again, and back") {
        for (int round = 0; round < 20; ++round) {
            graphic.setUnit(RS2::Inch);
            graphic.setUnit(RS2::Meter);
            graphic.setUnit(RS2::Inch);
            graphic.setUnit(RS2::Millimeter);
        }
        CHECK(paperOf(graphic).x == Catch::Approx(a4WidthMm));
        CHECK(paperOf(graphic).y == Catch::Approx(a4HeightMm));
    }

    SECTION("to the unit it already has") {
        graphic.setUnit(RS2::Millimeter);
        graphic.setUnit(RS2::Millimeter);
        CHECK(paperOf(graphic).x == Catch::Approx(a4WidthMm));
        CHECK(paperOf(graphic).y == Catch::Approx(a4HeightMm));
    }

    // whichever unit it was changed to, the paper size of new drawings is what it was
    CHECK(rememberedWidthMm() == Catch::Approx(a4WidthMm));
    CHECK(rememberedHeightMm() == Catch::Approx(a4HeightMm));
    const RS_Graphic next;
    CHECK(paperOf(next).x == Catch::Approx(a4WidthMm));
    CHECK(paperOf(next).y == Catch::Approx(a4HeightMm));
}

TEST_CASE("every drawing made in inches gets an A4 page", "[graphic][units][paper]") {
    (void)lc::test::application();
    const NewDrawingDefaults defaults{QStringLiteral("Inch")};

    // each of these used to start from what the one before had made of the setting
    for (int drawing = 0; drawing < 10; ++drawing) {
        const RS_Graphic graphic;
        INFO("drawing " << drawing);
        REQUIRE(graphic.getUnit() == RS2::Inch);
        CHECK(paperOf(graphic).x == Catch::Approx(a4WidthMm / 25.4));
        CHECK(paperOf(graphic).y == Catch::Approx(a4HeightMm / 25.4));
    }
    CHECK(rememberedWidthMm() == Catch::Approx(a4WidthMm));
    CHECK(rememberedHeightMm() == Catch::Approx(a4HeightMm));
}

TEST_CASE("choosing a paper size still remembers it for new drawings", "[graphic][units][paper]") {
    (void)lc::test::application();
    const NewDrawingDefaults defaults{QStringLiteral("Inch")};

    RS_Graphic graphic;
    // the paper of a drawing in inches: A3 landscape
    graphic.getPlotSettings()->setPaperSize(RS_Vector{420.0 / 25.4, 297.0 / 25.4});
    CHECK(rememberedWidthMm() == Catch::Approx(420.0));
    CHECK(rememberedHeightMm() == Catch::Approx(297.0));

    const RS_Graphic next;
    CHECK(paperOf(next).x == Catch::Approx(420.0 / 25.4));
    CHECK(paperOf(next).y == Catch::Approx(297.0 / 25.4));
}
