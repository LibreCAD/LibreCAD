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

// Issue #3187: the Paper tab of Drawing Preferences changed the drawing's paper
// before OK, in the unit chosen on the Units tab but kept as numbers of the
// drawing's unit, and on OK took a size typed in the new unit as one in the old.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>

#include "lc_actiontestsupport.h"
#include "lc_plot_settings.h"
#include "lc_settingguard.h"
#include "qg_dlgoptionsdrawing.h"
#include "rs_graphic.h"
#include "rs_settings.h"

namespace {

using lc::test::SettingGuard;

RS_Vector rememberedPaperMm() {
    return {LC_GET_ONE_STR("Print", "PaperSizeX", "0").toDouble(), LC_GET_ONE_STR("Print", "PaperSizeY", "0").toDouble()};
}

// What choosing an item of a combo box does.
void choose(const QG_DlgOptionsDrawing& dialog, const char* name, const int index) {
    auto* box = dialog.findChild<QComboBox*>(name);
    REQUIRE(box != nullptr);
    box->setCurrentIndex(index);
    emit box->activated(index);
}

void type(const QG_DlgOptionsDrawing& dialog, const char* name, const QString& text) {
    auto* edit = dialog.findChild<QLineEdit*>(name);
    REQUIRE(edit != nullptr);
    edit->setText(text);
}

void press(const QG_DlgOptionsDrawing& dialog, const QDialogButtonBox::StandardButton which) {
    auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
    REQUIRE(buttons != nullptr);
    buttons->button(which)->click();
}

} // namespace

TEST_CASE("Drawing Preferences take the paper in the unit chosen", "[gui][dialog][paper][issue3187]") {
    REQUIRE(lc::test::application() != nullptr);
    const SettingGuard unit{RS_SETTINGS, "Defaults", "Unit"};
    const SettingGuard paperWidth{RS_SETTINGS, "Print", "PaperSizeX"};
    const SettingGuard paperHeight{RS_SETTINGS, "Print", "PaperSizeY"};
    unit.set(QStringLiteral("Inch"));

    // a drawing in inches on a custom 30 x 15 in paper, margins of 10 mm
    RS_Graphic graphic;
    LC_PlotSettings* ps = graphic.getPlotSettings();
    ps->setPaperSize(RS_Vector{30.0, 15.0});
    ps->setMarginsInMm(10.0, 10.0, 10.0, 10.0);
    bool landscape = false;
    REQUIRE(graphic.getUnit() == RS2::Inch);
    REQUIRE(ps->getPaperFormat(&landscape) == RS2::Custom);

    QG_DlgOptionsDrawing dialog;
    dialog.setGraphic(&graphic);
    choose(dialog, "cbUnit", RS2::Foot);
    // the Paper tab names the unit its sizes are in
    CHECK(dialog.findChild<QGroupBox*>("formatGroupBox")->title() == "Format (Foot)");
    CHECK(dialog.findChild<QGroupBox*>("marginsGroupBox")->title() == "Margins (Foot)");

    SECTION("Cancel leaves the paper") {
        choose(dialog, "cbPaperFormat", RS2::Letter);
        press(dialog, QDialogButtonBox::Cancel);
        CHECK(graphic.getUnit() == RS2::Inch);
        CHECK(ps->getPaperSize().x == Catch::Approx(30.0));
        CHECK(ps->getPaperSize().y == Catch::Approx(15.0));
        CHECK(rememberedPaperMm().x == Catch::Approx(762.0));
        CHECK(rememberedPaperMm().y == Catch::Approx(381.0));
    }

    SECTION("a size typed in feet is taken in feet") {
        type(dialog, "lePaperWidth", "1.5");
        type(dialog, "lePaperHeight", "1");
        press(dialog, QDialogButtonBox::Ok);
        REQUIRE(graphic.getUnit() == RS2::Foot);
        CHECK(ps->getPaperSize().x == Catch::Approx(1.5));
        CHECK(ps->getPaperSize().y == Catch::Approx(1.0));
        CHECK(rememberedPaperMm().x == Catch::Approx(457.2));
        CHECK(rememberedPaperMm().y == Catch::Approx(304.8));
    }

    SECTION("a size and margins left as they were keep their length") {
        press(dialog, QDialogButtonBox::Ok);
        REQUIRE(graphic.getUnit() == RS2::Foot);
        CHECK(ps->getPaperSize().x == Catch::Approx(2.5));
        CHECK(ps->getPaperSize().y == Catch::Approx(1.25));
        for (const double margin : graphic.activeLayoutMargins()) {
            CHECK(margin == Catch::Approx(10.0).epsilon(1e-4));
        }
    }
}
