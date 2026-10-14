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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// The DXF/DWG filter creates every INSERT without its children and expands
// them all once the file has been read. A read that failed part-way returned
// before that pass, and the drawing could still be opened: RS_FileIO offers
// what was read to the user (of a DWG) or to the caller's error callback (of
// any file; the console tools keep it). Every block reference of such a
// drawing showed nothing, although its block had been read.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTemporaryDir>

#include "lc_filtertestsupport.h"
#include "rs_block.h"
#include "rs_entity.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_vector.h"

namespace {

// R12 records: no handles needed. Numbers are written as text so that the
// fixtures do not depend on the C locale.
const std::string kHeader = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n";

std::string dxfSection(const std::string& name, const std::string& records) {
    return "0\nSECTION\n2\n" + name + "\n" + records + "0\nENDSEC\n";
}

std::string dxfBlockBegin(const std::string& name) {
    return "0\nBLOCK\n8\n0\n2\n" + name + "\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n";
}

std::string dxfBlock(const std::string& name, const std::string& entities) {
    return dxfBlockBegin(name) + entities + "0\nENDBLK\n8\n0\n";
}

std::string dxfLine(const char* x1, const char* y1, const char* x2, const char* y2) {
    return std::string("0\nLINE\n8\n0\n10\n") + x1 + "\n20\n" + y1 + "\n11\n" + x2 + "\n21\n" + y2
           + "\n";
}

std::string dxfCircle(const char* x, const char* y, const char* radius) {
    return std::string("0\nCIRCLE\n8\n0\n10\n") + x + "\n20\n" + y + "\n40\n" + radius + "\n";
}

std::string dxfInsert(const std::string& name, const char* x, const char* y) {
    return "0\nINSERT\n8\n0\n2\n" + name + "\n10\n" + x + "\n20\n" + y + "\n";
}

// The file ends inside the entity that follows the last complete one.
const std::string kCutEntity = "0\nLINE\n8\n0\n10\n0.0\n";

const std::string kSquare = dxfBlock("SQUARE", dxfLine("0.0", "0.0", "10.0", "0.0")
                                                   + dxfLine("10.0", "0.0", "10.0", "10.0")
                                                   + dxfLine("10.0", "10.0", "0.0", "10.0")
                                                   + dxfLine("0.0", "10.0", "0.0", "0.0"));

// A square block, an INSERT of it, and the end of the file inside the next
// entity: the read fails in the ENTITIES section after the INSERT was read.
const std::string kCutAfterInsert = kHeader + dxfSection("BLOCKS", kSquare)
                                    + "0\nSECTION\n2\nENTITIES\n"
                                    + dxfInsert("SQUARE", "100.0", "50.0") + kCutEntity;

QString writeDxf(const QTemporaryDir& dir, const QString& name, const std::string& content) {
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};
    const QByteArray bytes = QByteArray::fromStdString(content);
    if (file.write(bytes) != bytes.size())
        return {};
    return path;
}

struct PartialOpen {
    bool m_opened = false;
    int m_callbacks = 0;
    bool m_partial = false;
    QString m_error;
};

// What a caller that keeps a drawing whenever something was read does (the
// console tools, and the GUI when the user answers "Yes").
PartialOpen openKeepingPartial(RS_Graphic& graphic, const QString& path) {
    PartialOpen result;
    graphic.initForNewDocument();
    result.m_opened = RS_FileIO::instance()->fileImport(
        graphic, path, RS2::FormatUnknown, [&result](bool partial, const QString& error) {
            ++result.m_callbacks;
            result.m_partial = partial;
            result.m_error = error;
            return partial;
        });
    if (result.m_opened)
        graphic.onLoadingCompleted();
    return result;
}

RS_Insert* findInsert(RS_EntityContainer& container, const QString& name) {
    for (RS_Entity* entity : container) {
        if (entity != nullptr && entity->rtti() == RS2::EntityInsert
            && static_cast<RS_Insert*>(entity)->getName() == name)
            return static_cast<RS_Insert*>(entity);
    }
    return nullptr;
}

void checkSquareAt(RS_Insert& insert, double x, double y) {
    CHECK(insert.count() == 4);
    for (RS_Entity* child : insert)
        CHECK(child->rtti() == RS2::EntityLine);
    CHECK(insert.getMin().x == Catch::Approx(x));
    CHECK(insert.getMin().y == Catch::Approx(y));
    CHECK(insert.getMax().x == Catch::Approx(x + 10.0));
    CHECK(insert.getMax().y == Catch::Approx(y + 10.0));
}

} // namespace

TEST_CASE("an INSERT read before a read error shows its block when the drawing is opened",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = writeDxf(dir, "cut_after_insert.dxf", kCutAfterInsert);
    REQUIRE_FALSE(path.isEmpty());

    RS_Graphic graphic;
    const PartialOpen outcome = openKeepingPartial(graphic, path);
    REQUIRE(outcome.m_opened);
    CHECK(outcome.m_callbacks == 1);
    CHECK(outcome.m_partial);
    CHECK(outcome.m_error == QObject::tr("error reading DXF/DWG entities", "RS_FilterDXFRW"));

    REQUIRE(graphic.count() == 1);
    RS_Insert* insert = findInsert(graphic, "SQUARE");
    REQUIRE(insert != nullptr);
    checkSquareAt(*insert, 100.0, 50.0);
}

TEST_CASE("the DXF filter expands the INSERTs it read when it reports a read error",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = writeDxf(dir, "cut_after_insert.dxf", kCutAfterInsert);
    REQUIRE_FALSE(path.isEmpty());

    // The filter itself leaves the drawing expanded; neither RS_FileIO nor
    // onLoadingCompleted() is involved here.
    RS_Graphic graphic;
    graphic.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        REQUIRE_FALSE(filter.fileImport(graphic, path, RS2::FormatDXFRW));
    }
    RS_Insert* insert = findInsert(graphic, "SQUARE");
    REQUIRE(insert != nullptr);
    checkSquareAt(*insert, 100.0, 50.0);
}

TEST_CASE("nested INSERTs expand independently of block definition order",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    bool partial = true;
    SECTION("after a read error") {}
    SECTION("after a complete read") { partial = false; }
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    // OUTER comes first and holds an INSERT of INNER, defined after it.
    const std::string blocks
        = dxfBlock("OUTER", dxfCircle("0.0", "0.0", "1.0") + dxfInsert("INNER", "10.0", "0.0"))
          + dxfBlock("INNER", dxfLine("0.0", "0.0", "2.0", "0.0"));
    const QString path = writeDxf(dir, "cut_after_nested_insert.dxf",
                                  kHeader + dxfSection("BLOCKS", blocks)
                                      + "0\nSECTION\n2\nENTITIES\n"
                                      + dxfInsert("OUTER", "100.0", "100.0")
                                      + (partial ? kCutEntity : "0\nENDSEC\n0\nEOF\n"));
    REQUIRE_FALSE(path.isEmpty());

    RS_Graphic graphic;
    const PartialOpen outcome = openKeepingPartial(graphic, path);
    REQUIRE(outcome.m_opened);
    CHECK(outcome.m_partial == partial);
    CHECK(outcome.m_callbacks == (partial ? 1 : 0));

    // The model-space INSERT shows the circle of OUTER and the line of INNER.
    RS_Insert* outer = findInsert(graphic, "OUTER");
    REQUIRE(outer != nullptr);
    CHECK(outer->count() == 2);
    CHECK(outer->getMin().x == Catch::Approx(99.0));
    CHECK(outer->getMin().y == Catch::Approx(99.0));
    CHECK(outer->getMax().x == Catch::Approx(112.0));
    CHECK(outer->getMax().y == Catch::Approx(101.0));

    // So does the INSERT inside the block definition, as after a complete read.
    RS_Block* outerBlock = graphic.findBlock("OUTER");
    REQUIRE(outerBlock != nullptr);
    RS_Insert* inner = findInsert(*outerBlock, "INNER");
    REQUIRE(inner != nullptr);
    CHECK(inner->count() == 1);
}

TEST_CASE("the INSERTs of a partly read block table are expanded as far as it was read",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    // ENTITIES comes before BLOCKS here, so that the drawing holds INSERTs when
    // the read stops inside the block table: the DWG reader is in that state
    // when it fails in a block after it has delivered model space. GOOD is
    // read whole, CUT has no ENDBLK, and ABSENT is never defined.
    const std::string entities = dxfInsert("GOOD", "0.0", "0.0") + dxfInsert("CUT", "20.0", "0.0")
                                 + dxfInsert("ABSENT", "40.0", "0.0");
    const std::string blocks = dxfBlock("GOOD", dxfLine("0.0", "0.0", "5.0", "5.0"))
                               + dxfBlockBegin("CUT") + dxfLine("0.0", "0.0", "5.0", "0.0")
                               + kCutEntity;
    const QString path = writeDxf(dir, "cut_in_block_table.dxf",
                                  kHeader + dxfSection("ENTITIES", entities)
                                      + "0\nSECTION\n2\nBLOCKS\n" + blocks);
    REQUIRE_FALSE(path.isEmpty());

    RS_Graphic graphic;
    const PartialOpen outcome = openKeepingPartial(graphic, path);
    REQUIRE(outcome.m_opened);
    CHECK(outcome.m_partial);
    CHECK(outcome.m_error == QObject::tr("error reading DXF/DWG blocks", "RS_FilterDXFRW"));
    REQUIRE(graphic.count() == 3);

    RS_Insert* good = findInsert(graphic, "GOOD");
    REQUIRE(good != nullptr);
    CHECK(good->count() == 1);

    // A block cut off before its ENDBLK shows what was read of it.
    RS_Insert* cut = findInsert(graphic, "CUT");
    REQUIRE(cut != nullptr);
    RS_Block* cutBlock = graphic.findBlock("CUT");
    REQUIRE(cutBlock != nullptr);
    CHECK(cut->count() == cutBlock->count());

    RS_Insert* absent = findInsert(graphic, "ABSENT");
    REQUIRE(absent != nullptr);
    CHECK(absent->getBlockForInsert() == nullptr);
    CHECK(absent->count() == 0);
}

TEST_CASE("a cyclic block does not prevent recovery of independent INSERTs",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const std::string blocks = dxfBlock("A", dxfInsert("B", "0.0", "0.0"))
                               + dxfBlock("B", dxfInsert("A", "0.0", "0.0")) + kSquare;
    const QString path = writeDxf(dir, "cut_after_cycle.dxf",
                                  kHeader + dxfSection("BLOCKS", blocks)
                                      + "0\nSECTION\n2\nENTITIES\n"
                                      + dxfInsert("A", "0.0", "0.0")
                                      + dxfInsert("SQUARE", "100.0", "50.0") + kCutEntity);
    REQUIRE_FALSE(path.isEmpty());
    RS_Graphic graphic;
    const PartialOpen outcome = openKeepingPartial(graphic, path);
    REQUIRE(outcome.m_opened);
    CHECK(outcome.m_partial);
    auto* cyclic = findInsert(graphic, "A");
    REQUIRE(cyclic != nullptr);
    CHECK(cyclic->count() == 0);
    auto* square = findInsert(graphic, "SQUARE");
    REQUIRE(square != nullptr);
    checkSquareAt(*square, 100.0, 50.0);
}

TEST_CASE("partial imports retain the INSERT nesting limit",
          "[dxf][filter][partial][insert]") {
    lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const std::size_t depth = RS_InsertExpansionBudget::DefaultMaxNestingDepth + 1;
    std::string blocks = kSquare;
    for (std::size_t i = 0; i < depth; ++i) {
        const std::string entities = i + 1 == depth
                                        ? dxfLine("0.0", "0.0", "1.0", "0.0")
                                        : dxfInsert("DEPTH" + std::to_string(i + 1), "0.0", "0.0");
        blocks += dxfBlock("DEPTH" + std::to_string(i), entities);
    }
    const QString path = writeDxf(dir, "cut_after_deep_insert.dxf",
                                  kHeader + dxfSection("BLOCKS", blocks)
                                      + "0\nSECTION\n2\nENTITIES\n"
                                      + dxfInsert("DEPTH0", "0.0", "0.0")
                                      + dxfInsert("SQUARE", "100.0", "50.0") + kCutEntity);
    REQUIRE_FALSE(path.isEmpty());
    RS_Graphic graphic;
    const PartialOpen outcome = openKeepingPartial(graphic, path);
    REQUIRE(outcome.m_opened);
    CHECK(outcome.m_partial);
    auto* deep = findInsert(graphic, "DEPTH0");
    REQUIRE(deep != nullptr);
    REQUIRE(deep->getBlockForInsert() != nullptr);
    CHECK(deep->count() == 0);
    auto* square = findInsert(graphic, "SQUARE");
    REQUIRE(square != nullptr);
    checkSquareAt(*square, 100.0, 50.0);
}
