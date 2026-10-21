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

// LC_UCSListModel keeps one heap-allocated row item per UCS and rebuilds all
// of them on every setUCSList(), which the UCS dock calls on each refresh.
// These tests drive that cycle; run them under `leaks -atExit` to check that
// replaced rows, and the rows left when the model is destroyed, are freed.

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_formatter.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "lc_ucslistmodel.h"
#include "lc_ucslistoptions.h"

namespace {

LC_UCS* makeRotatedUCS(const QString& name, const double originX) {
    auto* ucs = new LC_UCS(name);
    ucs->setOrigin(RS_Vector(originX, 20.0, 0.0));
    ucs->setXAxis(RS_Vector(0.0, 1.0, 0.0));
    ucs->setYAxis(RS_Vector(-1.0, 0.0, 0.0));
    return ucs;
}

} // namespace

TEST_CASE("LC_UCSListModel rows follow the UCS list across refreshes", "[ucs][model][ownership]") {
    lc::test::application();
    LC_UCSListOptions options;
    LC_Formatter formatter(nullptr);
    LC_UCSList list;
    LC_UCS* site = makeRotatedUCS("SITE", 10.0);
    list.add(site);
    LC_UCS* unnamed = makeRotatedUCS("", 30.0);
    REQUIRE(list.tryAddUCS(unnamed) == unnamed);
    REQUIRE(list.count() == 3); // WCS + the two above

    LC_UCSListModel model(&options);
    for (int i = 0; i < 20; ++i) {
        model.setUCSList(&list, &formatter);
    }
    REQUIRE(model.count() == 3);
    CHECK(model.getItemForIndex(model.index(0, 0)) == list.getWCS());
    CHECK(model.getItemForIndex(model.index(1, 0)) == site);
    CHECK(model.getItemForIndex(model.index(2, 0)) == unnamed);
    CHECK(model.data(model.index(1, LC_UCSListModel::NAME), Qt::DisplayRole).toString() == "SITE");
    CHECK(model.data(model.index(2, LC_UCSListModel::NAME), Qt::DisplayRole).toString() == "<No name>");
    QList<LC_UCS*> named;
    model.fillUCSsList(named);
    CHECK(named == QList<LC_UCS*>{site, unnamed});

    // A refresh after a UCS is deleted drops its row.
    list.remove(site);
    model.setUCSList(&list, &formatter);
    REQUIRE(model.count() == 2);
    CHECK(model.getItemForIndex(model.index(1, 0)) == unnamed);

    model.setUCSList(nullptr, &formatter);
    CHECK(model.count() == 0);

    // Rows still held when the model is destroyed are freed by it.
    model.setUCSList(&list, &formatter);
    REQUIRE(model.count() == 2);
}
