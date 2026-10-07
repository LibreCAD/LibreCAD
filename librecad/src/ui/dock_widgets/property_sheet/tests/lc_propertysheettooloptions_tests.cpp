/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 librecad.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
**
****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <memory>

#include <QAction>
#include <QCoreApplication>

#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"

namespace {

class TestOptionsProvider final : public LC_ToolOptionsPropertiesContainerProvider {
public:
    void fillToolOptionsContainer(LC_PropertyContainer*) override { ++fills; }
    int fills = 0;
};

struct PropertySheetFixture {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView drawingView;
    LC_ActionContext context;
    LC_ActionGroupManager actions{nullptr};
    LC_PropertySheetWidget sheet{nullptr, &context, &actions};

    PropertySheetFixture() {
        graphic.initForNewDocument();
        drawingView.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &drawingView);
        sheet.getOptions()->noSelectionWorkspace = false;
        sheet.getOptions()->noSelectionGraphicView = false;
        sheet.getOptions()->showToolOptions = true;
        sheet.show();
        QCoreApplication::processEvents();
        sheet.setGraphicView(&drawingView);
    }
};

enum class LeaveView { Detach, Switch, Destroy };

} // namespace

TEST_CASE("Properties forget tool options when leaving their view", "[gui][properties][3017]") {
    const auto leave = GENERATE(LeaveView::Detach, LeaveView::Switch, LeaveView::Destroy);
    CAPTURE(static_cast<int>(leave));
    PropertySheetFixture f;
    auto preview = std::make_unique<lc::test::TestGraphicView>();
    preview->setDocument(&f.graphic);
    preview->setPrintPreview(true);
    f.context.setDocumentAndView(&f.graphic, preview.get());
    f.sheet.setGraphicView(preview.get());
    auto provider = std::make_unique<TestOptionsProvider>();
    f.sheet.showToolOptions(provider.get());
    REQUIRE(provider->fills > 0);

    if (leave == LeaveView::Detach) {
        f.sheet.setGraphicView(nullptr);
    }
    else if (leave == LeaveView::Switch) {
        f.context.setDocumentAndView(&f.graphic, &f.drawingView);
        f.sheet.setGraphicView(&f.drawingView);
    }
    else {
        preview.reset();
        CHECK_FALSE(f.sheet.isEnabled());
    }
    const int previousFills = provider->fills;
    f.context.setDocumentAndView(&f.graphic, &f.drawingView);
    f.sheet.setGraphicView(&f.drawingView);

    // Drawing Preferences enters tool-options mode before its synchronous refresh.
    QAction preferences(QStringLiteral("Drawing Preferences"), nullptr);
    f.sheet.setCurrentQAction(&preferences);
    f.sheet.updateFormats();
    CHECK(provider->fills == previousFills);

    provider.reset();
    preview.reset();
    f.graphic.getPlotSettings()->setPaperFormat(RS2::A3, false);
    f.sheet.updateFormats();
    f.sheet.refill();
    CHECK(f.sheet.isEnabled());
}

TEST_CASE("Properties retain and replace tool options in the same view", "[gui][properties][3017]") {
    PropertySheetFixture f;
    TestOptionsProvider first;
    TestOptionsProvider second;
    f.sheet.showToolOptions(&first);
    REQUIRE(first.fills > 0);
    int previousFills = first.fills;

    f.sheet.setGraphicView(&f.drawingView);
    f.sheet.updateFormats();
    CHECK(first.fills > previousFills);

    previousFills = first.fills;
    f.sheet.showToolOptions(&second);
    REQUIRE(second.fills > 0);
    f.sheet.refill();
    CHECK(first.fills == previousFills);

    const int secondFills = second.fills;
    f.sheet.showToolOptions(nullptr);
    f.sheet.refill();
    CHECK(second.fills == secondFills);
}
