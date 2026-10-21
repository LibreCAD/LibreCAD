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

// Issue #2969, for the other sources a dock lists: the UCS list, the list of named views and the
// selection. The docks keep a pointer to the list (or the drawing) and rows for its items; a drawing
// that is destroyed while a dock is attached to it (QC_ApplicationWindow::doClose() detaches the docks
// first, and nothing found skips that) must not leave them naming freed objects, and a dock that is
// destroyed first must not leave a dangling listener in the drawing.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QAbstractItemModel>
#include <QTableView>

#include "lc_actiontestsupport.h"
#include "lc_namedviewslistwidget.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "lc_ucslistmodel.h"
#include "lc_ucslistoptions.h"
#include "lc_ucslistwidget.h"
#include "lc_view.h"
#include "lc_viewslist.h"
#include "qg_selectionwidget.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

/// reads every row the way a view does; under ASan this fails on a freed UCS or view
int readAllRows(const QAbstractItemModel& model) {
    int read = 0;
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        for (int column = 0; column < model.columnCount(QModelIndex{}); ++column) {
            for (const int role : {Qt::DisplayRole, Qt::DecorationRole, Qt::FontRole}) {
                if (model.data(model.index(row, column, QModelIndex{}), role).isValid()) {
                    ++read;
                }
            }
        }
    }
    return read;
}

QAbstractItemModel* tableModel(const QWidget& widget) {
    auto* view = widget.findChild<QTableView*>();
    return view != nullptr ? view->model() : nullptr;
}

void fill(RS_Graphic& graphic) {
    graphic.initForNewDocument();
    auto* site = new LC_UCS(QStringLiteral("SITE"));
    site->setOrigin(RS_Vector(10.0, 20.0, 0.0));
    graphic.getUCSList()->add(site);
    graphic.getViewList()->add(new LC_View(QStringLiteral("Overall")));
    graphic.getViewList()->add(new LC_View(QStringLiteral("Detail")));
}

}

TEST_CASE("A UCS dock outlives the drawing it shows", "[gui][ucs][2969]") {
    (void)lc::test::application();
    LC_UCSListWidget dock(QStringLiteral("UCS"), nullptr); // first, so that it is destroyed after the drawings
    auto graphic = std::make_unique<RS_Graphic>();
    fill(*graphic);
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    dock.setGraphicView(&view);
    QAbstractItemModel* model = tableModel(dock);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 2); // the WCS and SITE
    REQUIRE(readAllRows(*model) > 0);

    // the drawing goes with the dock still attached to it (the view is not used again)
    graphic.reset();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);
    // a UCS change has no drawing to be applied to
    LC_UCS stray(QStringLiteral("Stray"));
    dock.onViewUCSChanged(&stray);
    // detaching finds nothing of the destroyed list to unregister from
    dock.setGraphicView(nullptr);
    CHECK(model->rowCount(QModelIndex{}) == 0);

    // and the dock shows another drawing
    RS_Graphic second;
    fill(second);
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    dock.setGraphicView(&secondView);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    CHECK(readAllRows(*model) > 0);
}

TEST_CASE("A named views dock outlives the drawing it shows", "[gui][views][2969]") {
    (void)lc::test::application();
    LC_NamedViewsListWidget dock(QStringLiteral("Views"), nullptr);
    auto graphic = std::make_unique<RS_Graphic>();
    fill(*graphic);
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    dock.setGraphicView(&view);
    QAbstractItemModel* model = tableModel(dock);
    REQUIRE(model != nullptr);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    REQUIRE(readAllRows(*model) > 0);
    REQUIRE(graphic->getViewList()->listenerCount() == 1);

    graphic.reset();
    REQUIRE(model->rowCount(QModelIndex{}) == 0);
    CHECK(readAllRows(*model) == 0);
    dock.setGraphicView(nullptr);
    CHECK(model->rowCount(QModelIndex{}) == 0);

    RS_Graphic second;
    fill(second);
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    dock.setGraphicView(&secondView);
    REQUIRE(model->rowCount(QModelIndex{}) == 2);
    CHECK(readAllRows(*model) > 0);
}

TEST_CASE("A selection widget outlives the drawing it shows", "[gui][selection][2969]") {
    (void)lc::test::application();
    QG_SelectionWidget widget;
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* line = new RS_Line(graphic.get(), RS_LineData(RS_Vector{0, 0}, RS_Vector{10, 0}));
    graphic->addEntity(line);
    lc::test::TestGraphicView view;
    view.setDocument(graphic.get());
    widget.setGraphicView(&view);
    REQUIRE(graphic->getSelection()->listenerCount() == 1);

    graphic.reset();
    // told when the selection went: it forgot the drawing, and reads nothing of it
    widget.selectionChanged();
    widget.setGraphicView(nullptr);

    RS_Graphic second;
    second.initForNewDocument();
    lc::test::TestGraphicView secondView;
    secondView.setDocument(&second);
    widget.setGraphicView(&secondView);
    CHECK(second.getSelection()->listenerCount() == 1);
}

TEST_CASE("The UCS, views and selection docks destroyed before the drawing leave nothing registered on it", "[gui][ucs][views][selection][2969]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    fill(graphic);
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    const int ucsListeners = graphic.getUCSList()->listenerCount();
    const int viewListeners = graphic.getViewList()->listenerCount();
    const int selectionListeners = graphic.getSelection()->listenerCount();
    {
        LC_UCSListWidget ucs(QStringLiteral("UCS"), nullptr);
        LC_NamedViewsListWidget views(QStringLiteral("Views"), nullptr);
        QG_SelectionWidget selection;
        ucs.setGraphicView(&view);
        views.setGraphicView(&view);
        selection.setGraphicView(&view);
        CHECK(graphic.getUCSList()->listenerCount() == ucsListeners + 1);
        CHECK(graphic.getViewList()->listenerCount() == viewListeners + 1);
        CHECK(graphic.getSelection()->listenerCount() == selectionListeners + 1);
    }
    // each unregistered when it was destroyed
    CHECK(graphic.getUCSList()->listenerCount() == ucsListeners);
    CHECK(graphic.getViewList()->listenerCount() == viewListeners);
    CHECK(graphic.getSelection()->listenerCount() == selectionListeners);
    // so changing the drawing, and destroying it at the end of the test, reach no destroyed dock
    graphic.getViewList()->add(new LC_View(QStringLiteral("Another")));
    graphic.getUCSList()->setModified(true);
    graphic.getSelection()->clear();
}

TEST_CASE("The UCS list model does nothing without a list", "[gui][ucs][2969]") {
    (void)lc::test::application();
    LC_UCSListOptions options;
    LC_UCSListModel model(&options);
    model.setUCSList(nullptr, nullptr);
    LC_UCS stray(QStringLiteral("Stray"));
    // a UCS that was current when its drawing went is marked active in no list
    model.markActive(&stray);
    CHECK_FALSE(model.getIndexForUCS(&stray).isValid());
    CHECK(model.getWCS() == nullptr);
    CHECK(model.getActiveUCS() == nullptr);
}
