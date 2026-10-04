/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is distributed under the GNU General Public License version 2
** or later. See the LICENSE file for the full license text.
**
**********************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QImage>
#include <QPixmap>

#include "lc_navtest_support.h"
#include "rs_pen.h"
#include "rs_settings.h"

namespace {

struct DraftLinesSettingGuard {
    const bool previous = LC_GET_ONE_BOOL("Appearance", "DraftLinesMode", false);
    ~DraftLinesSettingGuard() { LC_SET_ONE("Appearance", "DraftLinesMode", previous); }
};

int lineThickness(lc::navtest::ViewFixture& fixture) {
    fixture.view->setAntialiasing(false);
    fixture.view->redraw(RS2::RedrawAll, true);
    const QImage image = fixture.view->grab().toImage();
    const double ratio = image.devicePixelRatio();
    const int x = qRound(fixture.viewport()->toGuiX(10.0) * ratio);
    const int y = qRound(fixture.viewport()->toGuiY(5.0) * ratio);
    REQUIRE(x >= 0);
    REQUIRE(x < image.width());
    REQUIRE(y >= 20 * ratio);
    REQUIRE(y + 20 * ratio < image.height());
    const QColor background = image.pixelColor(x, y - qRound(20 * ratio));
    int pixels = 0;
    for (int row = y - qRound(20 * ratio); row <= y + qRound(20 * ratio); ++row) {
        if (image.pixelColor(x, row) != background) {
            ++pixels;
        }
    }
    return pixels;
}

} // namespace

TEST_CASE("Draft-lines setters and getters use the same polarity", "[gui][draft-lines][3002]") {
    REQUIRE(lc::test::application() != nullptr);
    DraftLinesSettingGuard settings;
    const bool draft = GENERATE(false, true);
    lc::navtest::ViewFixture fixture;
    fixture.view->setDraftLinesMode(draft);
    CHECK(fixture.view->isDraftLinesMode() == draft);
    CHECK(fixture.view->getLineWidthScaling() == !draft);
    fixture.view->setDraftLinesMode(!draft);
    CHECK(fixture.view->isDraftLinesMode() == !draft);
    CHECK(fixture.view->getLineWidthScaling() == draft);
}

TEST_CASE("Draft lines remain consistent across settings reloads and views", "[gui][draft-lines][3002]") {
    REQUIRE(lc::test::application() != nullptr);
    DraftLinesSettingGuard settings;
    const bool draft = GENERATE(false, true);
    LC_SET_ONE("Appearance", "DraftLinesMode", draft);
    lc::navtest::ViewFixture first;
    first.view->setDraftLinesMode(draft);
    const bool scalingBefore = first.view->getLineWidthScaling();
    lc::navtest::ViewFixture second;
    CHECK(first.view->isDraftLinesMode() == second.view->isDraftLinesMode());
    CHECK(first.view->getLineWidthScaling() == second.view->getLineWidthScaling());

    // Document activation, grid changes, and preferences all reload view settings.
    for (int i = 0; i < 3; ++i) {
        first.view->loadSettings();
        CHECK(first.view->getLineWidthScaling() == scalingBefore);
        CHECK(first.view->isDraftLinesMode() == draft);
        CHECK(LC_GET_ONE_BOOL("Appearance", "DraftLinesMode", false) == draft);
    }
}

TEST_CASE("Draft lines render hairline strokes instead of physical pen widths", "[gui][draft-lines][3002]") {
    REQUIRE(lc::test::application() != nullptr);
    DraftLinesSettingGuard settings;
    lc::navtest::ViewFixture fixture(QSize(160, 100), false, [](RS_Graphic* graphic) {
        graphic->setUnit(RS2::Millimeter);
        graphic->setGridOn(false);
        auto* line = new RS_Line(graphic, RS_Vector(5.0, 5.0), RS_Vector(15.0, 5.0));
        line->setPen(RS_Pen(RS_Color(Qt::white), RS2::Width22, RS2::SolidLine));
        graphic->addEntity(line);
    });
    fixture.viewport()->justSetOffsetAndFactor(0, 0, 8.0);

    LC_SET_ONE("Appearance", "DraftLinesMode", false);
    fixture.view->setDraftLinesMode(false);
    const int physicalWidth = lineThickness(fixture);
    fixture.view->loadSettings();
    CHECK(lineThickness(fixture) == physicalWidth);

    LC_SET_ONE("Appearance", "DraftLinesMode", true);
    fixture.view->setDraftLinesMode(true);
    const int draftWidth = lineThickness(fixture);
    fixture.view->loadSettings();
    CHECK(lineThickness(fixture) == draftWidth);
    CAPTURE(physicalWidth, draftWidth);
    CHECK(draftWidth > 0);
    CHECK(physicalWidth > draftWidth);
}
