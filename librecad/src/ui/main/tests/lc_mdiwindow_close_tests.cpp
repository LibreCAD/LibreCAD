/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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

// A drawing window frees its drawing when it is closed. QC_MDIWindow deleted
// its document only if the view was not "cleaning up", but
// RS_GraphicView::beginClose(), which QC_ApplicationWindow::doClose() calls on
// every close, sets that flag: no drawing closed in the GUI was ever freed.
// Everything that points into the drawing goes first: the windows that show it
// too (print previews, block editors) and the window's own view.
//
// The windows are never shown or closed: a close event and an Enter event on a
// view both reach the QC_ApplicationWindow singleton.

#include <catch2/catch_test_macros.hpp>

#include <QMdiArea>
#include <QPointer>

#include "lc_actioncontext.h"
#include "lc_actiontestsupport.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

// Counts its destruction, so a test can tell that a drawing was freed.
class CountedLine final : public RS_Line {
public:
    CountedLine(RS_EntityContainer* parent, int& destroyed)
        : RS_Line(parent, RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)), m_destroyed{destroyed} {
    }
    ~CountedLine() override { ++m_destroyed; }

private:
    int& m_destroyed;
};

// A block counted the same way.
class CountedBlock final : public RS_Block {
public:
    CountedBlock(RS_EntityContainer* parent, const QString& name, int& destroyed)
        : RS_Block(parent, RS_BlockData(name, RS_Vector(0.0, 0.0), false)), m_destroyed{destroyed} {
    }
    ~CountedBlock() override { ++m_destroyed; }

private:
    int& m_destroyed;
};

// A drawing window that creates and owns its drawing, as File > New and
// File > Open make it.
QC_MDIWindow* newDrawingWindow(QMdiArea& area, LC_ActionContext& context) {
    return new QC_MDIWindow(nullptr, &area, false, &context);
}

// What QC_ApplicationWindow::doClose() does to a window before the deferred
// delete: the child is no longer listed, and the view is closing.
void closeWindow(QC_MDIWindow* window) {
    QC_MDIWindow* parent = window->getParentWindow();
    if (parent != nullptr) {
        parent->removeChildWindow(window);
    }
    window->getGraphicView()->beginClose();
}

} // namespace

TEST_CASE("Closing a drawing window frees its drawing", "[gui][close][ownership]") {
    REQUIRE(lc::test::application() != nullptr);
    QMdiArea area;
    LC_ActionContext context;

    int destroyed = 0;
    QC_MDIWindow* window = newDrawingWindow(area, context);
    RS_Document* document = window->getDocument();
    REQUIRE(document != nullptr);
    document->addEntity(new CountedLine(document, destroyed));

    closeWindow(window);
    REQUIRE(window->getGraphicView()->isCleanUp());
    delete window;

    CHECK(destroyed == 1);
}

TEST_CASE("A window that shows another window's drawing does not free it", "[gui][close][ownership]") {
    REQUIRE(lc::test::application() != nullptr);
    QMdiArea area;
    LC_ActionContext context;

    int destroyed = 0;
    QC_MDIWindow* window = newDrawingWindow(area, context);
    RS_Document* document = window->getDocument();
    document->addEntity(new CountedLine(document, destroyed));

    auto* preview = new QC_MDIWindow(document, &area, true, &context);
    window->addChildWindow(preview);
    // the preview's view took the drawing over
    REQUIRE(document->getGraphicView() == preview->getGraphicView());

    closeWindow(preview);
    delete preview;

    CHECK(destroyed == 0);
    // and handed it back to the drawing window's view
    CHECK(document->getGraphicView() == window->getGraphicView());
    CHECK_FALSE(window->hasChildren());

    closeWindow(window);
    delete window;
    CHECK(destroyed == 1);
}

TEST_CASE("A drawing window destroys its closed print preview before the drawing", "[gui][close][ownership]") {
    REQUIRE(lc::test::application() != nullptr);
    QMdiArea area;
    LC_ActionContext context;

    int destroyed = 0;
    QC_MDIWindow* window = newDrawingWindow(area, context);
    RS_Document* document = window->getDocument();
    document->addEntity(new CountedLine(document, destroyed));

    auto* preview = new QC_MDIWindow(document, &area, true, &context);
    window->addChildWindow(preview);
    const QPointer<QC_MDIWindow> previewAlive(preview);

    // doClose() closes the preview first, and no longer lists it; its deferred
    // delete has not run when the drawing window is deleted
    closeWindow(preview);
    REQUIRE_FALSE(window->hasChildren());
    closeWindow(window);
    delete window;

    CHECK(previewAlive.isNull());
    CHECK(destroyed == 1);
    // with the preview left alive, this would free it after its drawing
    delete previewAlive.data();
}

TEST_CASE("A drawing window destroys its block editors before the drawing", "[gui][close][ownership]") {
    REQUIRE(lc::test::application() != nullptr);
    QMdiArea area;
    LC_ActionContext context;

    int linesDestroyed = 0;
    int blocksDestroyed = 0;
    QC_MDIWindow* window = newDrawingWindow(area, context);
    RS_Graphic* graphic = window->getGraphic();
    REQUIRE(graphic != nullptr);
    auto* block = new CountedBlock(graphic, "edited", blocksDestroyed);
    REQUIRE(graphic->getBlockList()->add(block));
    block->addEntity(new CountedLine(block, linesDestroyed));

    // as QC_ApplicationWindow::slotEditActiveBlock() opens it
    auto* editor = new QC_MDIWindow(block, &area, false, &context);
    graphic->addLayerListListener(editor->getGraphicView());
    window->addChildWindow(editor);
    const QPointer<QC_MDIWindow> editorAlive(editor);
    REQUIRE(block->getGraphicView() == editor->getGraphicView());

    SECTION("closed first") {
        closeWindow(editor);
        delete editor;
        CHECK(blocksDestroyed == 0);
        // the block no longer names the editor's view
        CHECK(block->getGraphicView() == nullptr);
    }
    SECTION("closed with the drawing, not destroyed yet") {
        closeWindow(editor);
    }
    SECTION("still open") {
    }

    closeWindow(window);
    delete window;

    CHECK(editorAlive.isNull());
    CHECK(blocksDestroyed == 1);
    CHECK(linesDestroyed == 1);
    delete editorAlive.data();
}

TEST_CASE("A context-menu entity does not outlive a switch to another drawing", "[gui][close]") {
    REQUIRE(lc::test::application() != nullptr);
    RS_Graphic first;
    first.initForNewDocument();
    RS_Graphic second;
    second.initForNewDocument();
    auto* line = new RS_Line(&first, RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0));
    first.addEntity(line);

    LC_ActionContext context;
    context.setDocumentAndView(&first, nullptr);
    context.saveContextMenuActionContext(line, RS_Vector(5.0, 0.0), true);

    // the same drawing keeps it, for the action the menu starts
    context.setDocumentAndView(&first, nullptr);
    CHECK(context.getContextMenuActionContextEntity() == line);

    // another drawing drops it: the first one is freed when its window closes
    context.setDocumentAndView(&second, nullptr);
    CHECK(context.getContextMenuActionContextEntity() == nullptr);
    CHECK_FALSE(context.getContextMenuActionClickPosition().valid);
}
