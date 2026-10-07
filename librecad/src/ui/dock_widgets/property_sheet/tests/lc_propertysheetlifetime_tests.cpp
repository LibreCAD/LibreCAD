/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

// Issue #2969, for the Properties dock: it keeps the drawing (not a list), the drawing's viewport and
// the view, and is listed on four sources of the drawing (its selection, UCS list, layer list and view
// list). QC_ApplicationWindow::doClose() detaches it before the drawing and the view are destroyed, and
// no flow found skips that; when one does, the drawing tells the sheet as it goes and a view tells it
// when it is destroyed, and the sheet must not read either afterwards, or stay listed on the drawing
// when the sheet itself is destroyed first.
//
// The sheet is shown with a selected line, and with the no-selection sections that need the
// application window (which a test must not create) and a QG_GraphicView (which the test view is not)
// switched off.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QAction>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_property_container.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

class TestOptionsProvider final : public LC_ToolOptionsPropertiesContainerProvider {
public:
    void fillToolOptionsContainer(LC_PropertyContainer*) override { ++fills; }
    int fills = 0;
};

/// a drawing with a selected line
RS_Line* fillWithSelectedLine(RS_Graphic& graphic) {
    graphic.initForNewDocument();
    auto* line = new RS_Line(&graphic, RS_LineData(RS_Vector{0, 0}, RS_Vector{10, 0}));
    graphic.addEntity(line);
    graphic.select(QList<RS_Entity*>{line}, true);
    return line;
}

/// runs the event loop for \p milliseconds (a property edit is applied by a 30 ms timer)
void wait(const int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

int liveLines(const RS_Graphic& graphic) {
    int count = 0;
    for (const RS_Entity* e : graphic) {
        if (e != nullptr && !e->isDeleted() && e->rtti() == RS2::EntityLine) {
            ++count;
        }
    }
    return count;
}

void switchOffSectionsForTests(LC_PropertySheetWidget& sheet) {
    sheet.getOptions()->noSelectionWorkspace = false;
    sheet.getOptions()->noSelectionGraphicView = false;
}

}

TEST_CASE("The property sheet outlives the drawing it shows", "[gui][properties][2969]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions); // before the drawing, so that it is destroyed after it
    switchOffSectionsForTests(sheet);
    auto graphic = std::make_unique<RS_Graphic>();
    fillWithSelectedLine(*graphic);
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    context.setDocumentAndView(graphic.get(), &view);
    sheet.show();
    QCoreApplication::processEvents();
    sheet.setGraphicView(&view);
    REQUIRE(sheet.isEnabled());
    REQUIRE(graphic->getSelection()->listenerCount() == 1);
    REQUIRE(graphic->getUCSList()->listenerCount() == 1);
    REQUIRE(graphic->getLayerList()->listenerCount() == 1);
    REQUIRE(graphic->getViewList()->listenerCount() == 1);

    // the drawing goes with the sheet still attached to it (the view is not used again)
    graphic.reset();
    CHECK_FALSE(sheet.isEnabled());
    // repainting, refilling and detaching read nothing of the destroyed drawing
    QCoreApplication::processEvents();
    (void)sheet.grab();
    sheet.updateFormats();
    sheet.selectionChanged();
    sheet.setGraphicView(nullptr);
    CHECK_FALSE(sheet.isEnabled());
}

TEST_CASE("The property sheet is safe when the view is destroyed before the drawing", "[gui][properties][2969]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    fillWithSelectedLine(graphic);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    switchOffSectionsForTests(sheet);
    auto view = std::make_unique<lc::test::TestGraphicView>();
    view->setDocument(&graphic);
    context.setDocumentAndView(&graphic, view.get());
    sheet.show();
    QCoreApplication::processEvents();
    sheet.setGraphicView(view.get());
    REQUIRE(sheet.isEnabled());
    REQUIRE(graphic.getSelection()->listenerCount() == 1);

    // the view goes first: the sheet leaves the drawing and empties
    view.reset();
    CHECK_FALSE(sheet.isEnabled());
    CHECK(graphic.getSelection()->listenerCount() == 0);
    CHECK(graphic.getUCSList()->listenerCount() == 0);
    CHECK(graphic.getLayerList()->listenerCount() == 0);
    CHECK(graphic.getViewList()->listenerCount() == 0);
    // so changes of the drawing do not reach it, and refilling or detaching reads no view
    graphic.getSelection()->clear();
    QCoreApplication::processEvents();
    (void)sheet.grab();
    sheet.updateFormats();
    sheet.setGraphicView(nullptr);
    CHECK_FALSE(sheet.isEnabled());
}

TEST_CASE("A property sheet destroyed before the drawing leaves nothing registered on it", "[gui][properties][2969]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    fillWithSelectedLine(graphic);
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    context.setDocumentAndView(&graphic, &view);
    {
        LC_PropertySheetWidget sheet(nullptr, &context, &actions);
        switchOffSectionsForTests(sheet);
        sheet.show();
        QCoreApplication::processEvents();
        sheet.setGraphicView(&view);
        REQUIRE(graphic.getSelection()->listenerCount() == 1);
        REQUIRE(graphic.getLayerList()->listenerCount() == 1);
    }
    // it unregistered from all four when it was destroyed
    CHECK(graphic.getSelection()->listenerCount() == 0);
    CHECK(graphic.getUCSList()->listenerCount() == 0);
    CHECK(graphic.getLayerList()->listenerCount() == 0);
    CHECK(graphic.getViewList()->listenerCount() == 0);
    // so the drawing changing, and being destroyed at the end of the test, reach no destroyed sheet
    graphic.getSelection()->clear();
    graphic.getUCSList()->setModified(true);
}

TEST_CASE("An edit begun in a drawing that is closed does not reach the next drawing", "[gui][properties][2969]") {
    // a property edit is applied 30 ms after it: if the drawing is closed in between, the sheet must
    // not keep the entities of the edit (the original of the destroyed drawing, the clone of it) for
    // the next edit, which would delete a freed entity and add the orphan clone to its drawing
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    switchOffSectionsForTests(sheet);
    LC_PropertyContainer property;
    {
        auto first = std::make_unique<RS_Graphic>();
        RS_Line* line = fillWithSelectedLine(*first);
        lc::test::TestGraphicView view;
        view.setDocument(first.get());
        context.setDocumentAndView(first.get(), &view);
        sheet.show();
        sheet.setGraphicView(&view);
        sheet.entityModified(line, line->clone());
        sheet.onPropertyEdited(&property);
        // the window is closed at once: detached, and destroyed before the timer fires
        sheet.setGraphicView(nullptr);
        context.setDocumentAndView(nullptr, nullptr);
        first.reset();
    }
    wait(100);

    RS_Graphic second;
    RS_Line* line = fillWithSelectedLine(second);
    lc::test::TestGraphicView view;
    view.setDocument(&second);
    context.setDocumentAndView(&second, &view);
    sheet.setGraphicView(&view);
    sheet.entityModified(line, line->clone());
    sheet.onPropertyEdited(&property);
    wait(100);
    // the edit replaced the line by its clone: one line, not the clone and the orphan of the first drawing
    CHECK(liveLines(second) == 1);
    sheet.setGraphicView(nullptr);
}

TEST_CASE("A property sheet with no drawing ignores a late request", "[gui][properties][2969]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    LC_ActionContext::InteractiveInputInfo info;
    info.inputType = LC_ActionContext::InteractiveInputInfo::POINT;
    info.requestorTag = QStringLiteral("point");
    info.wcsPoint = RS_Vector{1, 2};
    // it reads the viewport of the view: there is none
    sheet.doProcessLateRequest(info);
    info.inputType = LC_ActionContext::InteractiveInputInfo::POINT_X;
    sheet.doProcessLateRequest(info);
}

TEST_CASE("Tool options do not outlive their view", "[gui][properties][3017]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView drawingView;
    drawingView.setDocument(&graphic);
    auto preview = std::make_unique<lc::test::TestGraphicView>();
    preview->setDocument(&graphic);
    preview->setPrintPreview(true);
    LC_ActionContext context;
    context.setDocumentAndView(&graphic, preview.get());
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    switchOffSectionsForTests(sheet);
    sheet.getOptions()->showToolOptions = true;
    sheet.show();
    sheet.setGraphicView(preview.get());
    auto provider = std::make_unique<TestOptionsProvider>();
    sheet.showToolOptions(provider.get());
    REQUIRE(provider->fills > 0);

    const int initialFills = provider->fills;
    sheet.setGraphicView(preview.get());
    sheet.updateFormats();
    REQUIRE(provider->fills > initialFills); // the same view keeps its options
    const int fills = provider->fills;

    SECTION("detach") { sheet.setGraphicView(nullptr); }
    SECTION("switch views") {
        context.setDocumentAndView(&graphic, &drawingView);
        sheet.setGraphicView(&drawingView);
    }
    SECTION("destroy the view") { preview.reset(); }
    context.setDocumentAndView(&graphic, &drawingView);
    sheet.setGraphicView(&drawingView);
    preview.reset();

    // Drawing Preferences enters tool-options mode before refreshing formats.
    QAction preferences(QStringLiteral("Drawing Preferences"), nullptr);
    sheet.setCurrentQAction(&preferences);
    sheet.updateFormats();
    REQUIRE(provider->fills == fills);
    provider.reset();
    graphic.getPlotSettings()->setPaperFormat(RS2::A3, false);
    sheet.updateFormats();
    CHECK(sheet.isEnabled());
}
