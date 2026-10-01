/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for RS_Modification
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

// Changing the linetype of entities copies it from the new pen whole, name
// included.

#include <catch2/catch_test_macros.hpp>

#include <QSet>
#include <QString>

#include "lc_actiontestsupport.h"
#include "rs_block.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_pen.h"

namespace {

RS_Pen namedPen() {
    RS_Pen pen;
    pen.setLineTypeName(QStringLiteral("VENDOR_TAB"));
    return pen;
}

RS_InsertData insertData(const QString &name) {
    return RS_InsertData(name, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0, 1, 1,
                         RS_Vector(0.0, 0.0));
}

RS_Line *addLine(RS_Block *block, const RS_Pen &pen) {
    auto *line = new RS_Line(block, RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0));
    line->setPen(pen);
    block->addEntity(line);
    return line;
}

// Applies data to a clone of en, as Modify > Attributes does, and returns its pen.
RS_Pen changedPen(RS_Entity &en, const RS_AttributesData &data) {
    RS_Entity *clone = nullptr;
    QSet<RS_Block *> blocks;
    RS_Modification::doChangeEntityAttributes(&en, clone, data, blocks);
    REQUIRE(clone != nullptr);
    const RS_Pen pen = clone->getPen(false);
    delete clone;
    return pen;
}

} // namespace

TEST_CASE("Changing the linetype keeps the name of the new pen",
          "[rs_modification][attributes][linetype]") {
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    line.setPen(RS_Pen(RS_Color(1, 2, 3), RS2::Width05, RS2::DashLine));

    RS_AttributesData data;
    data.pen = namedPen();
    data.changeLineType = true;

    const RS_Pen pen = changedPen(line, data);
    CHECK(pen.getLineTypeId() == data.pen.getLineTypeId());
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(pen.getLineType() == RS2::SolidLine);
    CHECK(pen.getColor() == RS_Color(1, 2, 3));
    CHECK(pen.getWidth() == RS2::Width05);
}

TEST_CASE("Copying the pen of an entity keeps its linetype name",
          "[rs_modification][attributes][linetype]") {
    RS_Pen source(RS_Color(4, 5, 6), RS2::Width09, RS2::SolidLine);
    source.setLineTypeName(QStringLiteral("VENDOR_TAB"));
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    line.setPen(RS_Pen(RS_Color(1, 2, 3), RS2::Width05, RS2::DashLine));

    // What Pen Copy applies.
    RS_AttributesData data;
    data.pen = source;
    data.changeColor = true;
    data.changeLineType = true;
    data.changeWidth = true;

    const RS_Pen pen = changedPen(line, data);
    CHECK(pen.getLineTypeId() == source.getLineTypeId());
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(pen.getColor() == RS_Color(4, 5, 6));
    CHECK(pen.getWidth() == RS2::Width09);
}

TEST_CASE("Changing the linetype deep in blocks keeps the name of the new pen",
          "[rs_modification][attributes][linetype]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    // OUTER holds a line and an insert of INNER, which holds another line.
    auto *inner = new RS_Block(&graphic,
                               RS_BlockData(QStringLiteral("INNER"), RS_Vector(0.0, 0.0), false));
    const RS_Line *innerLine = addLine(inner, RS_Pen(RS_Color(1, 2, 3), RS2::Width05, RS2::DashLine));
    graphic.addBlock(inner);

    auto *outer = new RS_Block(&graphic,
                               RS_BlockData(QStringLiteral("OUTER"), RS_Vector(0.0, 0.0), false));
    const RS_Line *outerLine = addLine(outer, RS_Pen(RS_Color(1, 2, 3), RS2::Width05, RS2::DotLine));
    outer->addEntity(new RS_Insert(outer, insertData(QStringLiteral("INNER"))));
    graphic.addBlock(outer);

    RS_Insert insert(&graphic, insertData(QStringLiteral("OUTER")));

    RS_AttributesData data;
    data.pen = namedPen();
    data.changeLineType = true;
    data.applyBlockDeep = true;

    LC_DocumentModificationBatch ctx;
    CHECK(RS_Modification::changeAttributes({&insert}, data, ctx));
    // The batch owns nothing until it is applied.
    qDeleteAll(ctx.entitiesToAdd);

    for (const RS_Line *member : {outerLine, innerLine}) {
        const RS_Pen pen = member->getPen(false);
        CHECK(pen.getLineTypeId() == data.pen.getLineTypeId());
        CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
        CHECK(pen.getLineType() == RS2::SolidLine);
        CHECK(pen.getColor() == RS_Color(1, 2, 3));
    }
}

TEST_CASE("Changing a named linetype to a plain one drops the name",
          "[rs_modification][attributes][linetype]") {
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    line.setPen(namedPen());
    REQUIRE(line.getPen(false).hasLineTypeName());

    RS_AttributesData data;
    data.pen.setLineType(RS2::DashLine);
    data.changeLineType = true;

    const RS_Pen pen = changedPen(line, data);
    CHECK_FALSE(pen.hasLineTypeName());
    CHECK(pen.getLineType() == RS2::DashLine);
    CHECK(pen.getLineTypeName() == QStringLiteral("DASHED"));
}
