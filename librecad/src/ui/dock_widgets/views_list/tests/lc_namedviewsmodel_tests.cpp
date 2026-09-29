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

// LC_NamedViewsModel keeps one heap-allocated row item per named view and
// rebuilds all of them on every setViewsList(), which the Named Views dock
// calls on each refresh. These tests drive that cycle; run them under
// `leaks -atExit` to check that replaced rows are freed, and under Guard
// Malloc to check that no row outlives its item.

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_formatter.h"
#include "lc_namedviewslistoptions.h"
#include "lc_namedviewsmodel.h"
#include "lc_view.h"
#include "lc_viewslist.h"

namespace {

LC_View* addView(LC_ViewList& list, const QString& name, const double x) {
    auto* view = new LC_View(name);
    view->setCenter(RS_Vector(x, 0.0));
    view->setSize(RS_Vector(40.0, 20.0));
    list.add(view);
    return view;
}

} // namespace

TEST_CASE("LC_NamedViewsModel rows follow the views list across refreshes", "[views][model][ownership]") {
    lc::test::application();
    LC_NamedViewsListOptions options;
    LC_Formatter formatter(nullptr);
    LC_ViewList list;
    LC_View* plan = addView(list, "PLAN", 0.0);
    LC_View* detail = addView(list, "DETAIL", 50.0);
    LC_View* unnamed = addView(list, "", 100.0);

    {
        LC_NamedViewsModel model(&options);
        for (int i = 0; i < 20; ++i) {
            model.setViewsList(&list, &formatter);
        }
        REQUIRE(model.rowCount({}) == 3);
        for (int row = 0; row < 3; ++row) {
            CHECK(model.getItemForIndex(model.index(row, 0)) == list.at(row));
        }
        CHECK(model.data(model.index(0, LC_NamedViewsModel::NAME), Qt::DisplayRole).toString() == "PLAN");
        CHECK(model.data(model.index(2, LC_NamedViewsModel::NAME), Qt::DisplayRole).toString() == "<No name>");
        CHECK(model.getItemForIndex(model.getIndexForView(detail)) == detail);

        // A refresh after a view is deleted drops its row.
        list.remove(detail);
        model.setViewsList(&list, &formatter);
        REQUIRE(model.rowCount({}) == 2);
        CHECK(model.getItemForIndex(model.index(0, 0)) == plan);
        CHECK(model.getItemForIndex(model.index(1, 0)) == unnamed);
        QList<LC_View*> rows;
        model.fillViewsList(rows);
        CHECK(rows == QList<LC_View*>{plan, unnamed});

        model.clear();
        CHECK(model.count() == 0);

        model.setViewsList(&list, &formatter);
        model.setViewsList(nullptr, &formatter);
        CHECK(model.rowCount({}) == 0);

        // Rows still held when the model is destroyed are freed by it.
        model.setViewsList(&list, &formatter);
        REQUIRE(model.count() == 2);
    }

    // remove() frees the view; leave the list empty so this does not depend on
    // whether ~LC_ViewList frees the views it still holds.
    list.remove(plan);
    list.remove(unnamed);
    CHECK(list.isEmpty());
}
