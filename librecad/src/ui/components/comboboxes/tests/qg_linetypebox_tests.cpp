/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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

// The linetype combo offers a drawing's table beside the built-ins: a table
// entry is a row that writes its own spelling into a pen, a built-in row writes
// its enum unless the drawing redefines it, and an unknown name gets one
// temporary row. Without a list the combo is exactly the built-in combo.

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "lc_actiontestsupport.h"
#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "qg_linetypebox.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_pen.h"

namespace {

// An imported record as the filter will make it from an LTYPE entry.
void addLineType(RS_Graphic& graphic, const QString& name, const std::vector<double>& pattern) {
    auto* entry = new LC_LineType(name);
    entry->pattern = pattern;
    entry->origin = LC_LineType::Origin::Imported;
    graphic.addLineType(entry);
}

RS_Pen namedPen(const QString& name) {
    RS_Pen pen(RS_Color(Qt::red), RS2::Width00, RS2::SolidLine);
    pen.setLineTypeName(name);
    return pen;
}

} // namespace

TEST_CASE("A drawing line type is a row of the combo", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0, 0.0, -5.0, 1.0, -5.0});

    QG_LineTypeBox builtIn;
    builtIn.init(true, false);
    const int builtInRows = builtIn.count();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);

    // The built-in rows, then one separator and the two table rows.
    REQUIRE(box.count() == builtInRows + 1 + 2);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    const int dotRow = box.findText(QStringLiteral("VENDOR_DOT"));
    REQUIRE(tabRow >= 0);
    REQUIRE(dotRow >= 0);
    CHECK(box.itemText(builtInRows).isEmpty());
    CHECK(tabRow > builtInRows);
    CHECK(dotRow > builtInRows);
    // Sorted by LC_LineType::key().
    CHECK(dotRow < tabRow);

    // The seeds are the built-in rows already: no table row repeats one.
    CHECK(box.findText(QStringLiteral("CONTINUOUS")) < 0);
    CHECK(box.findText(QStringLiteral("DASHED")) < 0);
    CHECK(box.findText(QStringLiteral("ByLayer")) < 0);

    RS_Pen pen;
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(box.currentIndex() == tabRow);
    box.applyLineTypeTo(pen);
    CHECK(pen.hasLineTypeName());
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(pen.getLineType() == RS2::SolidLine);
}

TEST_CASE("A built-in row writes the enum unless the drawing redefines it", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);

    // Only the seed: the row still means the enum, as every existing host needs.
    box.setLineType(RS2::DashLine);
    RS_Pen plain;
    box.applyLineTypeTo(plain);
    CHECK_FALSE(plain.hasLineTypeName());
    CHECK(plain.getLineType() == RS2::DashLine);

    // A record with other dashes makes the drawing paint the built-in from its
    // table, so the row must carry the canonical name.
    addLineType(graphic, "DASHED", {64.0, -32.0});
    box.rebuild(&graphic.getLineTypeList());
    box.setLineType(RS2::DashLine);
    RS_Pen redefined;
    box.applyLineTypeTo(redefined);
    CHECK(redefined.hasLineTypeName());
    CHECK(redefined.getLineTypeName() == QStringLiteral("DASHED"));
    CHECK(redefined.getLineType() == RS2::DashLine);
}

TEST_CASE("An alias spelling the drawing holds has its own row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    REQUIRE(LC_LineTypeNames::nameToLineType(QStringLiteral("ACAD_ISO09W100")) == RS2::PhantomLine);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "ACAD_ISO09W100", {24.0, -3.0, 6.0, -3.0, 6.0, -3.0});

    QG_LineTypeBox builtIn;
    builtIn.init(true, false);

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int row = box.findText(QStringLiteral("ACAD_ISO09W100"));
    REQUIRE(row >= 0);
    CHECK(row > builtIn.count());

    RS_Pen pen;
    box.setLineTypeOf(namedPen(QStringLiteral("ACAD_ISO09W100")));
    CHECK(box.currentIndex() == row);
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("ACAD_ISO09W100"));
    CHECK(pen.getLineType() == RS2::PhantomLine);
}

TEST_CASE("A name the drawing lacks gets one temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    // The separator that introduces the table row is already there.
    const int baselineWithTable = box.count();

    box.setLineTypeOf(namedPen(QStringLiteral("NOT_IN_LIST")));
    CHECK(box.currentIndex() >= 0);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("NOT_IN_LIST"));
    CHECK(box.count() == baselineWithTable + 1);

    // Another unknown name reuses the one temporary row.
    box.setLineTypeOf(namedPen(QStringLiteral("ALSO_MISSING")));
    CHECK(box.count() == baselineWithTable + 1);
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == baselineWithTable + 1);
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == baselineWithTable + 1);

    // A known name drops it.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(box.count() == baselineWithTable);
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine));
    CHECK(box.count() == baselineWithTable);
}

TEST_CASE("An untouched selection gives the pen back its own line type", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    // Another spelling of the same folded key.
    const RS_Pen source = namedPen(QStringLiteral("vendor_tab"));
    REQUIRE(source.hasLineTypeName());

    box.setLineTypeOf(source);
    RS_Pen back;
    box.applyLineTypeTo(back);
    CHECK(back.getLineTypeId() == source.getLineTypeId());
    CHECK(back.getLineTypeName() == QStringLiteral("vendor_tab"));

    // Leaving the row and returning keeps the remembered source pen.
    const int sourceRow = box.currentIndex();
    box.setLineType(RS2::DashLine);
    box.setCurrentIndex(sourceRow);
    RS_Pen again;
    box.applyLineTypeTo(again);
    CHECK(again.getLineTypeId() == source.getLineTypeId());

    // On another row the selection writes that row.
    box.setLineType(RS2::DashLine);
    RS_Pen other;
    box.applyLineTypeTo(other);
    CHECK(other.getLineType() == RS2::DashLine);
    CHECK_FALSE(other.hasLineTypeName());
}

TEST_CASE("A rebuild is silent and keeps the selection", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic first;
    first.initForNewDocument();
    addLineType(first, "VENDOR_TAB", {10.0, -5.0});
    RS_Graphic second;
    second.initForNewDocument();
    addLineType(second, "VENDOR_TAB", {12.0, -6.0});
    RS_Graphic third;
    third.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&first.getLineTypeList(), true, false);
    int emissions = 0;
    QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged,
                     [&emissions](RS2::LineType) { ++emissions; });
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    emissions = 0;

    box.rebuild(&second.getLineTypeList());
    CHECK(emissions == 0);
    RS_Pen secondPen;
    box.applyLineTypeTo(secondPen);
    CHECK(secondPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    box.rebuild(&third.getLineTypeList());
    CHECK(emissions == 0);
    RS_Pen thirdPen;
    box.applyLineTypeTo(thirdPen);
    CHECK(thirdPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    box.rebuild(nullptr);
    CHECK(emissions == 0);
    CHECK(box.findText(QStringLiteral("VENDOR_TAB")) < 0);
    QG_LineTypeBox builtIn;
    builtIn.init(true, false);
    CHECK(box.count() == builtIn.count());
}

TEST_CASE("The combo without a list is the built-in combo", "[gui][linetype][combo]") {
    (void)lc::test::application();
    QG_LineTypeBox box;
    box.init(true, true, true);
    REQUIRE(box.count() == 37);

    // Every built-in enum round-trips; the Unchanged row reports through
    // isUnchanged() instead of getLineType(), exactly as before.
    int builtInRows = 0;
    for (int i = 0; i < box.count(); ++i) {
        const int data = box.itemData(i).toInt();
        if (data == static_cast<int>(RS2::LineTypeUnchanged)) {
            continue;
        }
        ++builtInRows;
        box.setLineType(static_cast<RS2::LineType>(data));
        CHECK(box.getLineType() == static_cast<RS2::LineType>(data));
    }
    CHECK(builtInRows == box.count() - 1);

    const int dashSmall = box.findData(static_cast<int>(RS2::DashLine2));
    REQUIRE(dashSmall >= 0);
    CHECK(box.itemText(dashSmall) == QG_LineTypeBox::tr("Dash (small)"));

    box.setLineType(RS2::DashLine);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK_FALSE(pen.hasLineTypeName());
    CHECK(pen.getLineType() == RS2::DashLine);

    QG_LineTypeBox other;
    other.init(nullptr, true, true, true);
    REQUIRE(other.count() == box.count());
    for (int i = 0; i < box.count(); ++i) {
        CHECK(other.itemText(i) == box.itemText(i));
        CHECK(other.itemData(i).toInt() == box.itemData(i).toInt());
    }
}

TEST_CASE("A rebuild forgets a pen whose row the user left", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    // The user moves to another row; the remembered pen no longer applies.
    box.setLineType(RS2::DashLine);
    box.rebuild(&graphic.getLineTypeList());

    CHECK(box.getLineType() == RS2::DashLine);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineType() == RS2::DashLine);
    CHECK_FALSE(pen.hasLineTypeName());
}

TEST_CASE("Without a list the combo still keeps the pen it was set from", "[gui][linetype][combo]") {
    (void)lc::test::application();
    QG_LineTypeBox box;
    box.init(true, false);
    const int rows = box.count();
    const RS_Pen source = namedPen(QStringLiteral("VENDOR_TAB"));

    box.setLineTypeOf(source);
    // No list: the row count never grows and the name stays in the pen.
    CHECK(box.count() == rows);
    RS_Pen back;
    box.applyLineTypeTo(back);
    CHECK(back.getLineTypeId() == source.getLineTypeId());
    CHECK(back.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    const int sourceRow = box.currentIndex();
    box.setLineType(RS2::DashLine);
    RS_Pen other;
    box.applyLineTypeTo(other);
    CHECK(other.getLineType() == RS2::DashLine);
    CHECK_FALSE(other.hasLineTypeName());

    box.setCurrentIndex(sourceRow);
    RS_Pen again;
    box.applyLineTypeTo(again);
    CHECK(again.getLineTypeId() == source.getLineTypeId());
    CHECK(again.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
}

TEST_CASE("A silent rebuild reports the row it restored", "[gui][linetype][combo]") {
    (void)lc::test::application();
    REQUIRE(LC_LineTypeNames::nameToLineType(QStringLiteral("ACAD_ISO09W100")) == RS2::PhantomLine);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "ACAD_ISO09W100", {24.0, -3.0, 6.0, -3.0, 6.0, -3.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("ACAD_ISO09W100")));
    box.rebuild(nullptr);

    CHECK(box.getLineType() == RS2::PhantomLine);
    CHECK(box.currentIndex() == box.findData(static_cast<int>(RS2::PhantomLine)));
}

TEST_CASE("A separator leaves with the temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int base = box.count();
    box.setLineTypeOf(namedPen(QStringLiteral("NOT_IN_LIST")));
    // One separator and one temporary row, as no table row exists yet.
    CHECK(box.count() == base + 2);
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine));
    CHECK(box.count() == base);
}

TEST_CASE("A canonical spelling without a row gets no temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), false, false);
    const int base = box.count();
    box.setLineTypeOf(namedPen(QStringLiteral("BYLAYER")));
    CHECK(box.count() == base);
    CHECK(box.findText(QG_LineTypeBox::tr("%1 (not in this drawing)").arg(QStringLiteral("BYLAYER"))) < 0);
}

TEST_CASE("Blanks around a table spelling do not make another name", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    // A record keeps the blanks of its name; a pen trims them.
    addLineType(graphic, "VENDOR_PAD ", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int base = box.count();
    const int padRow = box.findText(QStringLiteral("VENDOR_PAD "));
    REQUIRE(padRow >= 0);

    // The table's row, not a temporary one.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_PAD")));
    CHECK(box.count() == base);
    CHECK(box.currentIndex() == padRow);

    // A rebuild finds the row again.
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == base);
    CHECK(box.currentIndex() == padRow);

    // The row a user picks writes the name a pen holds.
    QG_LineTypeBox other;
    other.init(&graphic.getLineTypeList(), true, false);
    other.setCurrentIndex(padRow);
    RS_Pen pen;
    other.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_PAD"));
}

TEST_CASE("A host that reads the pen inside the signal gets the pen it set",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    SECTION("with a list") {
        QG_LineTypeBox box;
        box.init(&graphic.getLineTypeList(), true, false);
        const RS_Pen source = namedPen(QStringLiteral("vendor_tab"));
        RS_Pen seen;
        bool read = false;
        QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged, [&](RS2::LineType) {
            box.applyLineTypeTo(seen);
            read = true;
        });
        box.setLineTypeOf(source);
        REQUIRE(read);
        CHECK(seen.getLineTypeId() == source.getLineTypeId());
        CHECK(seen.getLineTypeName() == QStringLiteral("vendor_tab"));
    }

    SECTION("without a list") {
        QG_LineTypeBox box;
        box.init(true, false);
        const RS_Pen source = namedPen(QStringLiteral("VENDOR_TAB"));
        RS_Pen seen;
        bool read = false;
        QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged, [&](RS2::LineType) {
            box.applyLineTypeTo(seen);
            read = true;
        });
        box.setLineTypeOf(source);
        REQUIRE(read);
        CHECK(seen.hasLineTypeName());
        CHECK(seen.getLineTypeId() == source.getLineTypeId());
        CHECK(seen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    }
}
