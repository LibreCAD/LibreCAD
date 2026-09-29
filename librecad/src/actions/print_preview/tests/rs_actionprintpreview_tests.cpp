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

#include <memory>

#include <QApplication>
#include <QSettings>

#include "lc_actioncontext.h"
#include "lc_graphicviewrenderer.h"
#include "lc_settingguard.h"
#include "rs_actionprintpreview.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

using lc::test::SettingGuard;

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
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

class PrintPreviewTestView final : public RS_GraphicView {
public:
    PrintPreviewTestView() : RS_GraphicView(nullptr) {
        getViewPort()->setSize(getWidth(), getHeight());
        setRenderer(std::make_unique<LC_GraphicViewRenderer>(getViewPort(), this));
    }

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

void addTestGeometry(RS_Graphic& graphic) {
    graphic.addEntity(new RS_Line(&graphic, RS_Vector{0.0, 0.0}, RS_Vector{100.0, 100.0}));
    graphic.calculateBorders();
}

} // namespace

TEST_CASE("fixed print preview preserves document scale and placement",
          "[print_preview][issue2741]") {
    (void)application();
    SettingGuard legacyFixed{RS_SETTINGS, "PrintPreview", "PrintScaleFixed"};
    SettingGuard currentFixed{RS_SETTINGS, "ActionFilePrintPreview", "PrintScaleFixed"};
    SettingGuard savedScale{RS_SETTINGS, "ActionFilePrintPreview", "PrintScaleValue"};
    legacyFixed.set(false);
    currentFixed.set(true);
    savedScale.set(0.125);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addTestGeometry(graphic);
    constexpr double documentScale = 2.5;
    const RS_Vector documentBase{17.0, -9.0};
    graphic.getPlotSettings()->setPaperScale(documentScale);
    graphic.setPaperInsertionBase(documentBase);

    PrintPreviewTestView view;
    view.setDocument(&graphic);
    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);
    auto action = std::make_unique<RS_ActionPrintPreview>(&context);
    action->postCreateInit();

    CHECK(action->isPaperScaleFixed());
    CHECK(action->getScale() == Catch::Approx(documentScale));
    CHECK(graphic.getPaperInsertionBase() == documentBase);

    CHECK_FALSE(action->setScale(0.25, false));
    CHECK(action->getScale() == Catch::Approx(documentScale));
    CHECK(graphic.getPaperInsertionBase() == documentBase);
}

TEST_CASE("automatic print preview fits once after loading options",
          "[print_preview][issue2741]") {
    (void)application();
    SettingGuard legacyFixed{RS_SETTINGS, "PrintPreview", "PrintScaleFixed"};
    SettingGuard currentFixed{RS_SETTINGS, "ActionFilePrintPreview", "PrintScaleFixed"};
    SettingGuard savedScale{RS_SETTINGS, "ActionFilePrintPreview", "PrintScaleValue"};
    legacyFixed.set(true);
    currentFixed.set(false);
    savedScale.set(0.125);

    // The page of a new drawing comes from these two settings. Fitting a
    // drawing to a page of a few millionths of a unit gives a scale that
    // getPaperScale() takes for "no scale" and reports as 1, so the test
    // fixes the page it measures instead of taking whatever is stored.
    SettingGuard unit{RS_SETTINGS, "Defaults", "Unit"};
    SettingGuard paperWidth{RS_SETTINGS, "Print", "PaperSizeX"};
    SettingGuard paperHeight{RS_SETTINGS, "Print", "PaperSizeY"};
    unit.set(QStringLiteral("Millimeter"));
    paperWidth.set(210.0);
    paperHeight.set(297.0);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addTestGeometry(graphic);
    graphic.getPlotSettings()->setPaperScale(2.5);
    graphic.setPaperInsertionBase(RS_Vector{17.0, -9.0});

    const RS_Vector printArea = graphic.getPlotSettings()->getPrintAreaSize(false);
    const double expectedScale = std::min(printArea.x, printArea.y) / 100.0;

    PrintPreviewTestView view;
    view.setDocument(&graphic);
    LC_ActionContext context;
    context.setDocumentAndView(&graphic, &view);
    auto action = std::make_unique<RS_ActionPrintPreview>(&context);
    action->postCreateInit();

    CHECK_FALSE(action->isPaperScaleFixed());
    CHECK(action->getScale() == Catch::Approx(expectedScale));
    CHECK(action->getScale() != Catch::Approx(0.125));
}
