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

// Issue #1324: the Filter box of the block list read what was typed as a wildcard
// pattern with character classes, so names with brackets - all the blocks of a font
// that is being edited are named "[0041]" and the like - were never found.

#include <catch2/catch_test_macros.hpp>

#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#include "qg_blockwidget.h"

namespace {

/// the names the filter box leaves in the list when `typed` is entered
QStringList shownFor(const QString& typed) {
    QStringListModel source(QStringList{"[0041]", "[0042]", "(rail)", "A<1>", "Door 1", "door 2", "Window", "a.b"});
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&source);
    proxy.setFilterRegularExpression(QG_BlockWidget::blockNameFilter(typed));
    QStringList shown;
    for (int row = 0; row < proxy.rowCount(); ++row) {
        shown << proxy.index(row, 0).data().toString();
    }
    return shown;
}

} // namespace

TEST_CASE("The block list filter finds names with brackets and other punctuation",
          "[blocks][filter][issue1324]") {
    SECTION("brackets in the typed text stand for themselves") {
        CHECK(shownFor("[0041]") == QStringList{"[0041]"});
        CHECK(shownFor("[0042") == QStringList{"[0042]"});
        CHECK(shownFor("[") == QStringList{"[0041]", "[0042]"});
        CHECK(shownFor("]") == QStringList{"[0041]", "[0042]"});
    }
    SECTION("parentheses, angle brackets and dots stand for themselves") {
        CHECK(shownFor("(rail)") == QStringList{"(rail)"});
        CHECK(shownFor("<1>") == QStringList{"A<1>"});
        CHECK(shownFor("a.b") == QStringList{"a.b"});
    }
    SECTION("plain text still matches anywhere in the name, ignoring case") {
        CHECK(shownFor("door") == QStringList{"Door 1", "door 2"});
        CHECK(shownFor("INDO") == QStringList{"Window"});
        CHECK(shownFor("nothing like it").isEmpty());
    }
    SECTION("an empty filter shows everything") {
        CHECK(shownFor("").size() == 8);
    }
}

TEST_CASE("The block list filter keeps * and ? as wildcards",
          "[blocks][filter][issue1324]") {
    // with a wildcard the whole name has to match
    CHECK(shownFor("door*") == QStringList{"Door 1", "door 2"});
    CHECK(shownFor("*1") == QStringList{"Door 1"});
    CHECK(shownFor("door ?") == QStringList{"Door 1", "door 2"});
    CHECK(shownFor("[00??]") == QStringList{"[0041]", "[0042]"});
    CHECK(shownFor("[*") == QStringList{"[0041]", "[0042]"});
    CHECK(shownFor("oor*").isEmpty());
}
