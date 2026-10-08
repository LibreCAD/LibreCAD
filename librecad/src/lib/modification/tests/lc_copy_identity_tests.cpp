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

// A DXF/DWG identity (source handle, extension dictionary, reactors) has one
// live holder: an operation hands it from what it deletes to the first entity
// it adds in its place, and every other copy is new. Paste puts entities on
// the destination's own layers and blocks, and keeps table handles only in
// the drawing that defines them.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include "doc_plugin_interface.h"
#include "lc_actiontestsupport.h"
#include "lc_copyutils.h"
#include "lc_documentinvariants.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_linetype.h"
#include "lc_linetypenames.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_clipboard.h"
#include "rs_filterdxfrw.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_text.h"

namespace {

struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;

    Drawing() {
        m_graphic.initForNewDocument();
        m_graphic.onLoadingCompleted();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }

    template <typename T>
    T* add(T* entity, const quint32 handle = 0) {
        entity->setSourceHandle(handle);
        m_graphic.addEntity(entity);
        return entity;
    }

    RS_Line* addLine(const double y, const quint32 handle = 0) {
        return add(new RS_Line(&m_graphic, RS_LineData(RS_Vector{0, y}, RS_Vector{10, y})), handle);
    }

    RS_Layer* addLayer(const QString& name) {
        auto* layer = new RS_Layer(name);
        layer->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width05, RS2::DashLine));
        m_graphic.addLayer(layer);
        return m_graphic.findLayer(name);
    }

    RS_Block* addBlock(const QString& name, const std::function<void(RS_Block&)>& fill) {
        auto* block = new RS_Block(&m_graphic, RS_BlockData(name, RS_Vector{0, 0}, false));
        m_graphic.addBlock(block);
        fill(*block);
        return block;
    }

    RS_Insert* addInsert(const QString& name, const RS_Vector& at, const quint32 handle = 0) {
        auto* insert = add(new RS_Insert(&m_graphic, RS_InsertData(name, at, RS_Vector{1, 1}, 0, 1, 1, RS_Vector{0, 0})),
                           handle);
        insert->update();
        return insert;
    }

    void modify(const std::function<void(LC_DocumentModificationBatch&)>& operation) {
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) {
            operation(ctx);
            return true;
        });
    }

    QList<RS_Entity*> live(const RS2::EntityType type = RS2::EntityUnknown) const {
        QList<RS_Entity*> entities;
        for (RS_Entity* e : m_graphic) {
            if (e != nullptr && !e->isDeleted() && (type == RS2::EntityUnknown || e->rtti() == type)) {
                entities << e;
            }
        }
        return entities;
    }

    void copy(QList<RS_Entity*> entities) {
        m_graphic.select(entities, true);
        LC_CopyUtils::copy(RS_Vector{0, 0}, entities, &m_graphic);
        m_graphic.select(entities, false);
    }

    void paste(const RS_Vector& at) {
        modify([&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(at), &m_graphic, ctx);
            ctx.dontSetActiveLayerAndPen();
        });
    }
};

void giveIdentity(RS_Entity* e) {
    e->setXDictHandle(e->sourceHandle() + 1);
    e->setReactorHandles({e->sourceHandle() + 2});
}

void checkIdentity(const RS_Entity* e, const quint32 handle) {
    CHECK(e->sourceHandle() == handle);
    CHECK(e->xDictHandle() == (handle == 0 ? 0 : handle + 1));
    CHECK(e->reactorHandles() == (handle == 0 ? std::vector<quint32>{} : std::vector<quint32>{handle + 2}));
}

RS_MoveData moveBy(const RS_Vector& offset, const bool keepOriginals, const int copies = 1) {
    RS_MoveData data;
    data.offset = offset;
    data.keepOriginals = keepOriginals;
    data.multipleCopies = copies > 1;
    data.number = copies;
    return data;
}

RS_Entity* at(const QList<RS_Entity*>& entities, const double y) {
    const auto found = std::find_if(entities.cbegin(), entities.cend(), [y](const RS_Entity* e) {
        return std::abs(e->getStartpoint().y - y) < 1e-9;
    });
    return found == entities.cend() ? nullptr : *found;
}

// Two lines in a GROUP; the first also has an extension dictionary holding an XRECORD.
constexpr const char* groupedLinesDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\nA1\n330\n1F\n102\n{ACAD_REACTORS\n330\n90\n102\n}\n102\n{ACAD_XDICTIONARY\n360\nA5\n102\n}\n"
    "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
    "0\nLINE\n5\nA2\n330\n1F\n102\n{ACAD_REACTORS\n330\n90\n102\n}\n"
    "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n5\n30\n0\n11\n10\n21\n5\n31\n0\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nOBJECTS\n"
    "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n3\nACAD_GROUP\n350\nD\n"
    "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nPAIR\n350\n90\n"
    "0\nGROUP\n5\n90\n102\n{ACAD_REACTORS\n330\nD\n102\n}\n330\nD\n100\nAcDbGroup\n300\npair\n70\n0\n71\n1\n"
    "340\nA1\n340\nA2\n"
    "0\nDICTIONARY\n5\nA5\n330\nA1\n100\nAcDbDictionary\n281\n1\n3\nMYDATA\n350\nA6\n"
    "0\nXRECORD\n5\nA6\n330\nA5\n100\nAcDbXrecord\n280\n1\n1\nhello\n"
    "0\nENDSEC\n0\nEOF\n";

std::filesystem::path tempFile(const std::string& name) {
    return std::filesystem::temp_directory_path() / ("lc_copy_identity_" + name);
}

void importGroupedLines(RS_Graphic& graphic) {
    const auto path = tempFile("grouped_lines.dxf");
    std::ofstream(path) << groupedLinesDxf;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(path.string()), RS2::FormatDXFRW));
    std::filesystem::remove(path);
}

bool save(RS_Graphic& graphic, const std::filesystem::path& path, const RS2::FormatType format) {
    std::filesystem::remove(path);
    RS_FilterDXFRW filter;
    return filter.fileExport(graphic, QString::fromStdString(path.string()), format);
}

// Values of one group code in every record of one type, in file order.
std::vector<std::string> values(const std::filesystem::path& path, const std::string& record, const std::string& code) {
    std::ifstream in(path);
    std::vector<std::string> found;
    std::string c;
    std::string v;
    bool inRecord = false;
    auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t\r") + 1);
        return s;
    };
    while (std::getline(in, c) && std::getline(in, v)) {
        c = trim(c);
        v = trim(v);
        if (c == "0") {
            inRecord = v == record;
        }
        else if (inRecord && c == code) {
            found.push_back(v);
        }
    }
    return found;
}

// The dangling owner references in a save of the drawing that a save of the file as imported does not have.
// (The writer gives the GROUP a new handle but leaves its members' reactors naming the old one.)
QStringList addedDangling(const std::filesystem::path& path) {
    static const QStringList imported = [] {
        Drawing d;
        importGroupedLines(d.m_graphic);
        const auto plain = tempFile("imported.dxf");
        REQUIRE(save(d.m_graphic, plain, RS2::FormatDXFRW));
        QStringList problems = lc::test::danglingReferences(QString::fromStdString(plain.string()));
        std::filesystem::remove(plain);
        return problems;
    }();
    QStringList added = lc::test::danglingReferences(QString::fromStdString(path.string()));
    for (const QString& problem : imported) {
        added.removeAll(problem);
    }
    return added;
}

// A block reference and a text that belongs to it, as the DXF filter reads a block attribute: the text is
// a sibling of the block reference and names it by lineage.
struct OwnedText {
    RS_Insert* m_insert = nullptr;
    RS_Text* m_text = nullptr;
};

OwnedText addOwnedText(Drawing& d) {
    d.addBlock("PART", [](RS_Block& b) {
        b.addEntity(new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
    });
    OwnedText pair;
    pair.m_insert = d.addInsert("PART", RS_Vector{0, 0});
    pair.m_text = d.add(new RS_Text(&d.m_graphic,
                                    RS_TextData(RS_Vector{0, 2}, RS_Vector{0, 2}, 1.0, 1.0, RS_TextData::VABaseline,
                                                RS_TextData::HALeft, RS_TextData::None, "value", "standard", 0.0)));
    pair.m_text->setOwnerLineage(pair.m_insert->lineage());
    return pair;
}

// The live entity of a type whose insertion point has this x.
template <typename T>
T* atX(const Drawing& d, const RS2::EntityType type, const double x) {
    T* found = nullptr;
    for (RS_Entity* e : d.live(type)) {
        auto* candidate = static_cast<T*>(e);
        if (std::abs(candidate->getInsertionPoint().x - x) < 1e-9) {
            REQUIRE(found == nullptr);
            found = candidate;
        }
    }
    REQUIRE(found != nullptr);
    return found;
}

} // namespace

TEST_CASE("Moving without keeping the original keeps its identity", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, false), {line}, false, ctx);
    });

    const auto lines = d.live();
    REQUIRE(lines.size() == 1);
    CHECK(lines.front() != line);
    checkIdentity(lines.front(), 0x40);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("Moving into several copies keeps the identity with the first", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, false, 3), {line}, false, ctx);
    });

    const auto lines = d.live();
    REQUIRE(lines.size() == 3);
    checkIdentity(at(lines, 5), 0x40);
    checkIdentity(at(lines, 10), 0);
    checkIdentity(at(lines, 15), 0);
}

TEST_CASE("Copies that keep their originals are new entities", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, true, 2), {line}, false, ctx);
    });

    const auto lines = d.live();
    REQUIRE(lines.size() == 3);
    checkIdentity(at(lines, 0), 0x40);
    checkIdentity(at(lines, 5), 0);
    checkIdentity(at(lines, 10), 0);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("A new container made of copies holds none of their identities", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    auto* container = new RS_EntityContainer(&d.m_graphic);
    RS_Entity* copy = line->clone();
    copy->setParent(container);
    container->addEntity(copy);
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        ctx += container;
    });

    checkIdentity(line, 0x40);
    checkIdentity(copy, 0);
}

TEST_CASE("A replacement takes over the identity of what it replaces", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    RS_Entity* trimmed = line->clone();
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        ctx.replace(line, trimmed);
    });

    REQUIRE(d.live() == QList<RS_Entity*>{trimmed});
    checkIdentity(trimmed, 0x40);
}

TEST_CASE("Cutting an entity in two keeps its identity with the first piece", "[copy][identity]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        REQUIRE(RS_Modification::cut(RS_Vector{4, 0}, line, ctx));
    });

    const auto pieces = d.live();
    REQUIRE(pieces.size() == 2);
    checkIdentity(pieces.at(0), 0x40);
    checkIdentity(pieces.at(1), 0);
}

TEST_CASE("Exploding an insert gives its pieces new identities", "[copy][identity]") {
    Drawing d;
    RS_Line* inBlock = nullptr;
    const RS_Block* block = d.addBlock("PART", [&](RS_Block& b) {
        inBlock = new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
        inBlock->setSourceHandle(0x200);
        giveIdentity(inBlock);
        b.addEntity(inBlock);
    });
    RS_Insert* insert = d.addInsert("PART", RS_Vector{5, 5}, 0x300);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        REQUIRE(RS_Modification::explode({insert}, ctx));
    });

    const auto pieces = d.live();
    REQUIRE(pieces.size() == 1);
    checkIdentity(pieces.front(), 0);
    CHECK(block->count() == 1);
    checkIdentity(inBlock, 0x200);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("An entity on a locked layer keeps its identity when a move leaves it in place", "[copy][identity]") {
    Drawing d;
    RS_Layer* locked = d.addLayer("LOCKED");
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);
    line->setLayer(locked);
    locked->lock(true);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, false), {line}, false, ctx);
    });

    const auto lines = d.live();
    REQUIRE(lines.size() == 2);
    checkIdentity(line, 0x40);
    checkIdentity(at(lines, 5), 0);
}

TEST_CASE("A plugin edit keeps the identity only when it deletes the original", "[copy][identity][plugins]") {
    for (const auto how : {DPI::DELETE_ORIGINAL, DPI::KEEP_ORIGINAL}) {
        Drawing d;
        RS_Line* line = d.addLine(0, 0x40);
        giveIdentity(line);
        RS_Entity* moved = line->clone();
        moved->move(RS_Vector{0, 5});

        const Doc_plugin_interface plugin(&d.m_context, nullptr);
        REQUIRE(plugin.addToUndo(line, moved, how));

        checkIdentity(moved, how == DPI::DELETE_ORIGINAL ? 0x40 : 0);
        checkIdentity(line, 0x40);
        CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
    }
}

TEST_CASE("Undo and redo leave identities alone", "[copy][identity][undo]") {
    Drawing d;
    RS_Line* line = d.addLine(0, 0x40);
    giveIdentity(line);
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, false), {line}, false, ctx);
    });
    RS_Entity* moved = d.live().front();

    REQUIRE(d.m_graphic.undo());
    REQUIRE(d.live() == QList<RS_Entity*>{line});
    checkIdentity(line, 0x40);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());

    REQUIRE(d.m_graphic.redo());
    REQUIRE(d.live() == QList<RS_Entity*>{moved});
    checkIdentity(moved, 0x40);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("GROUP membership and the extension dictionary follow a moved entity", "[copy][identity][dxf]") {
    Drawing d;
    importGroupedLines(d.m_graphic);
    const auto lines = d.live();
    REQUIRE(lines.size() == 2);
    RS_Entity* first = at(lines, 0);
    REQUIRE(first != nullptr);
    REQUIRE(first->xDictHandle() == 0xA5);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 20}, false), {first}, false, ctx);
    });

    const auto path = tempFile("moved.dxf");
    REQUIRE(save(d.m_graphic, path, RS2::FormatDXFRW));
    const auto lineHandles = values(path, "LINE", "5");
    REQUIRE(lineHandles.size() == 2);
    auto members = values(path, "GROUP", "340");
    std::sort(members.begin(), members.end());
    auto sortedLines = lineHandles;
    std::sort(sortedLines.begin(), sortedLines.end());
    CHECK(members == sortedLines);
    CHECK(values(path, "LINE", "360").size() == 1);
    CHECK(addedDangling(path).isEmpty());

    RS_Graphic reread;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(reread, QString::fromStdString(path.string()), RS2::FormatDXFRW));
    }
    const auto& groups = reread.dwgAdvancedMetadata().groups();
    REQUIRE(groups.size() == 1);
    CHECK(groups.front().entityHandles.size() == 2);
    int withDictionary = 0;
    for (const RS_Entity* e : reread) {
        if (e->rtti() == RS2::EntityLine && e->xDictHandle() != 0) {
            ++withDictionary;
            CHECK(e->getStartpoint().y == 20);
        }
    }
    CHECK(withDictionary == 1);
    std::filesystem::remove(path);
}

TEST_CASE("A copy that keeps its original joins no GROUP and owns no dictionary", "[copy][identity][dxf]") {
    Drawing d;
    importGroupedLines(d.m_graphic);
    RS_Entity* first = at(d.live(), 0);
    REQUIRE(first != nullptr);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 20}, true), {first}, false, ctx);
    });
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());

    const auto path = tempFile("copied.dxf");
    REQUIRE(save(d.m_graphic, path, RS2::FormatDXFRW));
    CHECK(values(path, "LINE", "5").size() == 3);
    CHECK(values(path, "GROUP", "340").size() == 2);
    CHECK(values(path, "LINE", "360").size() == 1);
    CHECK(addedDangling(path).isEmpty());
    std::filesystem::remove(path);
}

TEST_CASE("Pasting into the same drawing uses its layers and blocks and keeps its table handles", "[copy][paste]") {
    Drawing d;
    RS_Layer* walls = d.addLayer("WALLS");
    d.addBlock("DOOR", [](RS_Block& b) {
        b.addEntity(new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
    });
    RS_Line* line = d.addLine(0, 0x40);
    line->setLayer(walls);
    line->setMaterialHandle(0xC0);
    RS_Insert* insert = d.addInsert("DOOR", RS_Vector{5, 5}, 0x300);
    insert->setLayer(walls);
    const unsigned layers = d.m_graphic.countLayers();
    const unsigned blocks = d.m_graphic.countBlocks();

    d.copy({line, insert});
    d.paste(RS_Vector{0, 100});

    CHECK(d.m_graphic.countLayers() == layers);
    CHECK(d.m_graphic.countBlocks() == blocks);
    const auto pastedLines = d.live(RS2::EntityLine);
    REQUIRE(pastedLines.size() == 2);
    const RS_Entity* pastedLine = at(pastedLines, 100);
    REQUIRE(pastedLine != nullptr);
    CHECK(pastedLine->getLayer(false) == walls);
    CHECK(pastedLine->sourceHandle() == 0);
    CHECK(pastedLine->materialHandle() == 0xC0);
    const auto inserts = d.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 2);
    const auto* pastedInsert = static_cast<RS_Insert*>(inserts.back());
    CHECK(pastedInsert->sourceHandle() == 0);
    CHECK(pastedInsert->getLayer(false) == walls);
    CHECK(pastedInsert->getBlockForInsert() == d.m_graphic.findBlock("DOOR"));
    CHECK(pastedInsert->count() == 1);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("Pasting into another drawing brings the layers and blocks it lacks", "[copy][paste]") {
    auto source = std::make_unique<Drawing>();
    RS_Layer* walls = source->addLayer("WALLS");
    source->addBlock("KNOB", [](RS_Block& b) {
        b.addEntity(new RS_Circle(&b, RS_CircleData(RS_Vector{0, 0}, 0.1)));
    });
    source->addBlock("DOOR", [&](RS_Block& b) {
        auto* edge = new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
        edge->setLayer(walls);
        edge->setSourceHandle(0x210);
        edge->setMaterialHandle(0xC0);
        b.addEntity(edge);
        auto* knob = new RS_Insert(&b, RS_InsertData("KNOB", RS_Vector{1, 0}, RS_Vector{1, 1}, 0, 1, 1, RS_Vector{0, 0}));
        b.addEntity(knob);
    });
    RS_Line* line = source->addLine(0, 0x40);
    line->setLayer(walls);
    line->setMaterialHandle(0xC0);
    RS_Insert* insert = source->addInsert("DOOR", RS_Vector{5, 5}, 0x300);
    insert->setLayer(walls);
    source->copy({line, insert});

    Drawing destination;
    destination.paste(RS_Vector{0, 100});
    source.reset(); // nothing pasted or on the clipboard may depend on the source drawing
    CHECK(RS_CLIPBOARD->hasBlock("DOOR"));

    RS_Layer* pastedWalls = destination.m_graphic.findLayer("WALLS");
    REQUIRE(pastedWalls != nullptr);
    CHECK(pastedWalls->getPen().getColor() == RS_Color(255, 0, 0));
    const RS_Block* door = destination.m_graphic.findBlock("DOOR");
    REQUIRE(door != nullptr);
    REQUIRE(destination.m_graphic.findBlock("KNOB") != nullptr);
    const RS_Entity* edge = door->firstEntity();
    CHECK(edge->getLayer(false) == pastedWalls);
    CHECK(edge->sourceHandle() == 0);
    CHECK(edge->materialHandle() == 0);

    const auto lines = destination.live(RS2::EntityLine);
    REQUIRE(lines.size() == 1);
    CHECK(lines.front()->getLayer(false) == pastedWalls);
    CHECK(lines.front()->sourceHandle() == 0);
    CHECK(lines.front()->materialHandle() == 0);
    const auto inserts = destination.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 1);
    auto* pastedInsert = static_cast<RS_Insert*>(inserts.front());
    CHECK(pastedInsert->getBlockForInsert() == door);
    RS_CLIPBOARD->clear(); // nor on the clipboard
    pastedInsert->update();
    CHECK(pastedInsert->count() == 2);
    CHECK(lc::test::documentProblems(destination.m_graphic).isEmpty());
}

TEST_CASE("Pasting keeps the destination's block of the same name", "[copy][paste]") {
    Drawing source;
    source.addBlock("DOOR", [](RS_Block& b) {
        b.addEntity(new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
    });
    source.copy({source.addInsert("DOOR", RS_Vector{0, 0})});

    Drawing destination;
    const RS_Block* door = destination.addBlock("DOOR", [](RS_Block& b) {
        b.addEntity(new RS_Circle(&b, RS_CircleData(RS_Vector{0, 0}, 1)));
        b.addEntity(new RS_Circle(&b, RS_CircleData(RS_Vector{0, 0}, 2)));
    });
    destination.paste(RS_Vector{0, 0});

    CHECK(destination.m_graphic.findBlock("DOOR") == door);
    CHECK(door->count() == 2);
    CHECK(door->firstEntity()->rtti() == RS2::EntityCircle);
    const auto inserts = destination.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 1);
    CHECK(static_cast<RS_Insert*>(inserts.front())->getBlockForInsert() == door);
}

TEST_CASE("Pasting a drawing into itself leaves its GROUP and dictionary with the originals", "[copy][paste][dxf]") {
    Drawing d;
    importGroupedLines(d.m_graphic);
    d.copy(d.live());
    d.paste(RS_Vector{0, 50});
    d.paste(RS_Vector{0, 100});

    CHECK(d.live().size() == 6);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
    const auto path = tempFile("pasted.dxf");
    REQUIRE(save(d.m_graphic, path, RS2::FormatDXFRW));
    CHECK(values(path, "LINE", "5").size() == 6);
    CHECK(values(path, "GROUP", "340").size() == 2);
    CHECK(values(path, "LINE", "360").size() == 1);
    CHECK(addedDangling(path).isEmpty());
    std::filesystem::remove(path);
}

TEST_CASE("A drawing pasted twice into a new one saves as DWG", "[copy][paste][dwg]") {
    auto source = std::make_unique<Drawing>();
    importGroupedLines(source->m_graphic);
    source->copy(source->live());
    source.reset();
    Drawing d;
    d.paste(RS_Vector{0, 0});
    d.paste(RS_Vector{0, 50});

    CHECK(d.live().size() == 4);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
    const auto dxf = tempFile("new.dxf");
    REQUIRE(save(d.m_graphic, dxf, RS2::FormatDXFRW));
    CHECK(lc::test::danglingReferences(QString::fromStdString(dxf.string())).isEmpty());
    std::filesystem::remove(dxf);
    const auto dwg = tempFile("new.dwg");
    CHECK(save(d.m_graphic, dwg, RS2::FormatDWG));
    std::filesystem::remove(dwg);
}

TEST_CASE("Copy and paste across the sample corpus keep every drawing consistent", "[.slow][copy][paste][corpus]") {
    QStringList files;
    for (const QString& dir : {QStringLiteral("dev/dwg_samples"), QStringLiteral("doc/dwg"), QStringLiteral("doc/dwg2")}) {
        QDirIterator it(QDir::home().filePath(dir), {QStringLiteral("*.dwg"), QStringLiteral("*.dxf")}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            if (QFileInfo(file).size() < 5 * 1024 * 1024) {
                files << file;
            }
        }
    }
    files.sort();
    if (files.isEmpty()) {
        SKIP("no sample drawings under ~/dev/dwg_samples, ~/doc/dwg or ~/doc/dwg2");
    }
    // Inserts of blocks the source file lacks are the file's problem, not the copy's.
    auto structural = [](RS_Graphic& graphic) {
        QStringList problems = lc::test::documentProblems(graphic);
        problems.erase(std::remove_if(problems.begin(), problems.end(), [](const QString& problem) {
            return problem.contains(QStringLiteral("which the drawing does not have"));
        }), problems.end());
        return problems;
    };
    int pasted = 0;
    for (const QString& file : files) {
        CAPTURE(file.toStdString());
        auto source = std::make_unique<Drawing>();
        RS_FilterDXFRW filter;
        const RS2::FormatType format = file.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive) ? RS2::FormatDWG
                                                                                                    : RS2::FormatDXFRW;
        if (!filter.fileImport(source->m_graphic, file, format) || source->live().isEmpty()) {
            continue;
        }
        const qsizetype before = structural(source->m_graphic).size();
        source->copy(source->live());
        source->paste(RS_Vector{0, 0});
        CHECK(structural(source->m_graphic).size() == before);

        Drawing destination;
        destination.paste(RS_Vector{0, 0});
        for (const RS_Entity* e : destination.live(RS2::EntityInsert)) {
            const QString name = static_cast<const RS_Insert*>(e)->getName();
            if (destination.m_graphic.findBlock(name) == nullptr) {
                CHECK(source->m_graphic.findBlock(name) == nullptr);
            }
        }
        source.reset();
        for (int i = 0; i < RS_CLIPBOARD->countBlocks(); ++i) {
            CHECK(RS_CLIPBOARD->hasBlock(RS_CLIPBOARD->blockAt(i)->getName()));
        }
        destination.m_graphic.updateInserts();
        CHECK(structural(destination.m_graphic).isEmpty());
        const auto dxf = tempFile("corpus.dxf");
        if (save(destination.m_graphic, dxf, RS2::FormatDXFRW)) {
            CHECK(lc::test::danglingReferences(QString::fromStdString(dxf.string())).isEmpty());
        }
        std::filesystem::remove(dxf);
        ++pasted;
    }
    RS_CLIPBOARD->clear();
    CHECK(pasted > 0);
}

TEST_CASE("A library insert brings its nested blocks onto the destination's layers as new objects", "[copy][library]") {
    auto source = std::make_unique<Drawing>();
    RS_Layer* walls = source->addLayer("WALLS");
    source->addBlock("KNOB", [&](RS_Block& b) {
        auto* circle = new RS_Circle(&b, RS_CircleData(RS_Vector{0, 0}, 0.1));
        circle->setLayer(walls);
        circle->setSourceHandle(0x220);
        b.addEntity(circle);
    });
    source->addBlock("DOOR", [](RS_Block& b) {
        b.addEntity(new RS_Insert(&b, RS_InsertData("KNOB", RS_Vector{1, 0}, RS_Vector{1, 1}, 0, 1, 1, RS_Vector{0, 0})));
    });
    source->addInsert("DOOR", RS_Vector{5, 5}, 0x300)->setLayer(walls);
    RS_Line* line = source->addLine(0, 0x40);
    line->setLayer(walls);
    line->setMaterialHandle(0xC0);

    Drawing destination;
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::libraryInsert(LC_LibraryInsertData(RS_Vector{0, 0}, 1, 0, "PART", &source->m_graphic),
                                       &destination.m_graphic, ctx);
    });
    source.reset(); // nothing inserted may depend on the library drawing

    RS_Layer* ownWalls = destination.m_graphic.findLayer("WALLS");
    REQUIRE(ownWalls != nullptr);
    const RS_Block* knob = destination.m_graphic.findBlock("KNOB");
    REQUIRE(knob != nullptr);
    CHECK(knob->firstEntity()->getLayer(false) == ownWalls);
    CHECK(knob->firstEntity()->sourceHandle() == 0);
    REQUIRE(destination.m_graphic.findBlock("DOOR") != nullptr);
    const RS_Block* part = destination.m_graphic.findBlock("PART");
    REQUIRE(part != nullptr);
    for (const RS_Entity* e : *part) {
        CHECK(e->getLayer(false) == ownWalls);
        CHECK(e->sourceHandle() == 0);
        CHECK(e->materialHandle() == 0);
    }
    const auto inserts = destination.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 1);
    inserts.front()->update();
    CHECK(static_cast<RS_Insert*>(inserts.front())->count() == 2);
    CHECK(lc::test::documentProblems(destination.m_graphic).isEmpty());
}

TEST_CASE("A plugin's block from disk comes onto the drawing's layers as new objects", "[copy][plugins]") {
    const auto path = tempFile("from_disk.dxf");
    std::ofstream(path) << groupedLinesDxf;
    Drawing d;
    const unsigned layers = d.m_graphic.countLayers();

    Doc_plugin_interface plugin(&d.m_context, nullptr);
    const QString name = plugin.addBlockfromFromdisk(QString::fromStdString(path.string()));
    std::filesystem::remove(path);

    const RS_Block* block = d.m_graphic.findBlock(name);
    REQUIRE(block != nullptr);
    CHECK(block->count() == 2);
    for (const RS_Entity* e : *block) {
        CHECK(e->getLayer(false) == d.m_graphic.findLayer("0"));
        CHECK(e->sourceHandle() == 0);
        CHECK(e->xDictHandle() == 0);
        CHECK(e->reactorHandles().empty());
    }
    CHECK(d.m_graphic.countLayers() == layers);
    CHECK(lc::test::documentProblems(d.m_graphic).isEmpty());
}

TEST_CASE("Pasting a block does not resurrect an entity the block editor deleted", "[copy][paste][block]") {
    auto source = std::make_unique<Drawing>();
    source->addBlock("DOOR", [](RS_Block& b) {
        b.addEntity(new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
        auto* trimmedAway = new RS_Line(&b, RS_LineData(RS_Vector{0, 1}, RS_Vector{1, 1}));
        b.addEntity(trimmedAway);
        // The block editor edits the block itself as its own document:
        // undoableDelete only flags an entity deleted and keeps it in the
        // block's list, as undo history (RS_Document::undoableDelete).
        trimmedAway->setFlag(RS2::FlagDeleted);
    });
    source->copy({source->addInsert("DOOR", RS_Vector{0, 0})});

    Drawing destination;
    destination.paste(RS_Vector{0, 100});
    source.reset(); // nothing pasted or on the clipboard may depend on the source drawing

    const RS_Block* door = destination.m_graphic.findBlock("DOOR");
    REQUIRE(door != nullptr);
    int liveLines = 0;
    for (const RS_Entity* e : *door) {
        if (e != nullptr && !e->isDeleted()) {
            ++liveLines;
        }
    }
    CHECK(liveLines == 1);
}

TEST_CASE("Copying an insert of a block that inserts another block on a since-deleted layer reads no freed layer",
          "[copy][paste][layer]") {
    // RS_Graphic::removeLayer() only sweeps top-level entities and direct
    // children of blocks; it never reaches the expansion children of a
    // nested INSERT two blocks deep. LEAF's own line still names the freed
    // layer by pointer.
    Drawing source;
    RS_Layer* removable = source.addLayer("REMOVABLE");
    source.addBlock("LEAF", [&](RS_Block& b) {
        auto* line = new RS_Line(&b, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
        line->setLayer(removable);
        b.addEntity(line);
    });
    source.addBlock("MIDDLE", [](RS_Block& b) {
        b.addEntity(new RS_Insert(&b, RS_InsertData("LEAF", RS_Vector{0, 0}, RS_Vector{1, 1}, 0, 1, 1, RS_Vector{0, 0})));
    });
    RS_Insert* topInsert = source.addInsert("MIDDLE", RS_Vector{0, 0});

    source.m_graphic.removeLayer(removable); // frees `removable`

    source.copy({topInsert}); // must not dereference the freed layer (ASan)

    Drawing destination;
    destination.paste(RS_Vector{0, 100});
    CHECK(lc::test::documentProblems(destination.m_graphic).isEmpty());
}

TEST_CASE("Pasting onto the points it removes in the same batch does not take over their identity",
          "[copy][paste][undo]") {
    Drawing source;
    RS_Line* line = source.addLine(0, 0x2A);
    giveIdentity(line);
    source.copy({line});

    Drawing destination;
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        // Mimics LC_ActionPasteToPoints: paste, then delete in the same
        // section, so the undo section's own handle-takeover rule alone
        // would let the pasted clone claim the deleted point's identity.
        auto* point = new RS_Line(&destination.m_graphic, RS_LineData(RS_Vector{0, 100}, RS_Vector{0, 100}));
        point->setSourceHandle(0x2A);
        destination.m_graphic.addEntity(point);
        LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector{0, 100}), &destination.m_graphic, ctx);
        ctx.dontSetActiveLayerAndPen();
        ctx.remove(point);
    });

    const auto lines = destination.live(RS2::EntityLine);
    REQUIRE(lines.size() == 1);
    checkIdentity(lines.front(), 0);
}

TEST_CASE("A replacement keeps the lineage and the owner of what it replaces", "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();
    REQUIRE(insertLineage != 0);
    REQUIRE(textLineage != 0);
    REQUIRE(textLineage != insertLineage);
    CHECK(pair.m_insert->ownerLineage() == 0);

    // A clone is what it was made from until an undo section says otherwise.
    RS_Entity* movedInsert = pair.m_insert->clone();
    movedInsert->move(RS_Vector{50, 0});
    CHECK(movedInsert->lineage() == insertLineage);
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        ctx.replace(pair.m_insert, movedInsert);
    });
    CHECK(movedInsert->lineage() == insertLineage);

    RS_Entity* editedText = pair.m_text->clone();
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        ctx.replace(pair.m_text, editedText);
    });
    CHECK(editedText->lineage() == textLineage);
    CHECK(editedText->ownerLineage() == insertLineage);

    // What was replaced keeps its own too, so an undo brings the same pair back.
    REQUIRE(d.m_graphic.undo());
    REQUIRE(d.m_graphic.undo());
    REQUIRE(d.live() == QList<RS_Entity*>{pair.m_insert, pair.m_text});
    CHECK(pair.m_insert->lineage() == insertLineage);
    CHECK(pair.m_text->lineage() == textLineage);
    CHECK(pair.m_text->ownerLineage() == insertLineage);
    REQUIRE(d.m_graphic.redo());
    REQUIRE(d.m_graphic.redo());
    REQUIRE(d.live() == QList<RS_Entity*>{movedInsert, editedText});
    CHECK(editedText->ownerLineage() == movedInsert->lineage());
}

TEST_CASE("Moving into several copies keeps the lineage with the first", "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{50, 0}, false, 3), {pair.m_insert, pair.m_text}, false, ctx);
    });

    REQUIRE(d.live(RS2::EntityInsert).size() == 3);
    REQUIRE(d.live(RS2::EntityText).size() == 3);
    CHECK(atX<RS_Insert>(d, RS2::EntityInsert, 50)->lineage() == insertLineage);
    CHECK(atX<RS_Text>(d, RS2::EntityText, 50)->lineage() == textLineage);
    CHECK(atX<RS_Text>(d, RS2::EntityText, 50)->ownerLineage() == insertLineage);
    // The other two are copies: new lineages, and each text belongs to the block reference of its own copy.
    QSet<unsigned long long> lineages{insertLineage, textLineage};
    for (const double x : {100.0, 150.0}) {
        const RS_Insert* insert = atX<RS_Insert>(d, RS2::EntityInsert, x);
        const RS_Text* text = atX<RS_Text>(d, RS2::EntityText, x);
        CHECK_FALSE(lineages.contains(insert->lineage()));
        lineages.insert(insert->lineage());
        CHECK_FALSE(lineages.contains(text->lineage()));
        lineages.insert(text->lineage());
        CHECK(insert->ownerLineage() == 0);
        CHECK(text->ownerLineage() == insert->lineage());
    }
}

TEST_CASE("Copies that keep their originals get lineages of their own and stay paired", "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{50, 0}, true, 2), {pair.m_insert, pair.m_text}, false, ctx);
    });

    REQUIRE(d.live(RS2::EntityInsert).size() == 3);
    REQUIRE(d.live(RS2::EntityText).size() == 3);
    CHECK(pair.m_insert->lineage() == insertLineage);
    CHECK(pair.m_text->lineage() == textLineage);
    CHECK(pair.m_text->ownerLineage() == insertLineage);
    QSet<unsigned long long> lineages{insertLineage, textLineage};
    for (const double x : {50.0, 100.0}) {
        const RS_Insert* insert = atX<RS_Insert>(d, RS2::EntityInsert, x);
        const RS_Text* text = atX<RS_Text>(d, RS2::EntityText, x);
        CHECK_FALSE(lineages.contains(insert->lineage()));
        lineages.insert(insert->lineage());
        CHECK_FALSE(lineages.contains(text->lineage()));
        lineages.insert(text->lineage());
        CHECK(text->ownerLineage() == insert->lineage());
    }

    // A copy of a copy is a pair of its own again, in a later undo section too.
    RS_Insert* copiedInsert = atX<RS_Insert>(d, RS2::EntityInsert, 50);
    RS_Text* copiedText = atX<RS_Text>(d, RS2::EntityText, 50);
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 30}, true), {copiedInsert, copiedText}, false, ctx);
    });
    REQUIRE(d.live(RS2::EntityInsert).size() == 4);
    REQUIRE(d.live(RS2::EntityText).size() == 4);
    const RS_Entity* newestInsert = d.live(RS2::EntityInsert).back();
    const RS_Entity* newestText = d.live(RS2::EntityText).back();
    CHECK_FALSE(lineages.contains(newestInsert->lineage()));
    CHECK_FALSE(lineages.contains(newestText->lineage()));
    CHECK(newestText->ownerLineage() == newestInsert->lineage());
    CHECK(copiedText->ownerLineage() == copiedInsert->lineage());

    // A text copied without its block reference belongs to none that exists.
    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, -30}, true), {pair.m_text}, false, ctx);
    });
    const RS_Entity* loneText = d.live(RS2::EntityText).back();
    CHECK(loneText->ownerLineage() != 0);
    for (const RS_Entity* insert : d.live(RS2::EntityInsert)) {
        CHECK(insert->lineage() != loneText->ownerLineage());
    }
    CHECK(pair.m_text->ownerLineage() == insertLineage);
}

TEST_CASE("An entity that is no block reference and belongs to none keeps its lineage when copied", "[copy][lineage]") {
    Drawing d;
    RS_Line* line = d.addLine(0);
    const auto lineage = line->lineage();

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{0, 5}, true, 2), {line}, false, ctx);
    });

    // Nothing tells such entities apart by lineage, so no undo section spends anything on them.
    const auto lines = d.live();
    REQUIRE(lines.size() == 3);
    for (const RS_Entity* e : lines) {
        CHECK(e->lineage() == lineage);
        CHECK(e->ownerLineage() == 0);
    }
}

TEST_CASE("A text on a locked layer stays with its moved block reference and its copy belongs to none",
          "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();
    RS_Layer* locked = d.addLayer("LOCKED");
    pair.m_text->setLayer(locked);
    locked->lock(true);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{50, 0}, false), {pair.m_insert, pair.m_text}, false, ctx);
    });

    // The block reference was replaced; the text could not be, so its clone is one more text.
    REQUIRE(d.live(RS2::EntityInsert).size() == 1);
    REQUIRE(d.live(RS2::EntityText).size() == 2);
    const RS_Insert* moved = atX<RS_Insert>(d, RS2::EntityInsert, 50);
    CHECK(moved->lineage() == insertLineage);
    CHECK_FALSE(pair.m_text->isDeleted());
    CHECK(pair.m_text->lineage() == textLineage);
    CHECK(pair.m_text->ownerLineage() == insertLineage);
    const RS_Text* copy = atX<RS_Text>(d, RS2::EntityText, 50);
    CHECK(copy->lineage() != textLineage);
    CHECK(copy->ownerLineage() != 0);
    CHECK(copy->ownerLineage() != insertLineage);
}

TEST_CASE("Copies of a block reference and of its text on a locked layer are pairs copy by copy", "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();
    RS_Layer* locked = d.addLayer("LOCKED");
    pair.m_text->setLayer(locked);
    locked->lock(true);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{50, 0}, false, 2), {pair.m_insert, pair.m_text}, false, ctx);
    });

    // The block reference was replaced by its first clone; the text could not be, so both its clones are
    // texts more.
    REQUIRE(d.live(RS2::EntityInsert).size() == 2);
    REQUIRE(d.live(RS2::EntityText).size() == 3);
    CHECK(atX<RS_Insert>(d, RS2::EntityInsert, 50)->lineage() == insertLineage);
    CHECK_FALSE(pair.m_text->isDeleted());
    CHECK(pair.m_text->lineage() == textLineage);
    CHECK(pair.m_text->ownerLineage() == insertLineage);

    // The second clone of each is the second copy of the pair.
    const RS_Insert* secondInsert = atX<RS_Insert>(d, RS2::EntityInsert, 100);
    const RS_Text* secondText = atX<RS_Text>(d, RS2::EntityText, 100);
    CHECK(secondInsert->lineage() != insertLineage);
    CHECK(secondText->lineage() != textLineage);
    CHECK(secondText->ownerLineage() == secondInsert->lineage());

    // The first clone of the text lies next to the block reference that has the original: it belongs to none.
    const RS_Text* firstText = atX<RS_Text>(d, RS2::EntityText, 50);
    CHECK(firstText->lineage() != textLineage);
    CHECK(firstText->ownerLineage() != 0);
    for (const RS_Entity* insert : d.live(RS2::EntityInsert)) {
        CHECK(insert->lineage() != firstText->ownerLineage());
    }
}

TEST_CASE("Copies of a block reference on a locked layer and of its text are pairs copy by copy", "[copy][lineage]") {
    Drawing d;
    const OwnedText pair = addOwnedText(d);
    const auto insertLineage = pair.m_insert->lineage();
    const auto textLineage = pair.m_text->lineage();
    RS_Layer* locked = d.addLayer("LOCKED");
    pair.m_insert->setLayer(locked);
    locked->lock(true);

    d.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(moveBy(RS_Vector{50, 0}, false, 2), {pair.m_insert, pair.m_text}, false, ctx);
    });

    // The text was replaced by its first clone, which still belongs to the block reference that stayed.
    REQUIRE(d.live(RS2::EntityInsert).size() == 3);
    REQUIRE(d.live(RS2::EntityText).size() == 2);
    CHECK_FALSE(pair.m_insert->isDeleted());
    CHECK(pair.m_insert->lineage() == insertLineage);
    const RS_Text* firstText = atX<RS_Text>(d, RS2::EntityText, 50);
    CHECK(firstText->lineage() == textLineage);
    CHECK(firstText->ownerLineage() == insertLineage);

    // The second clone of each is the second copy of the pair, and the first clone of the block reference
    // has no text.
    const RS_Insert* firstInsert = atX<RS_Insert>(d, RS2::EntityInsert, 50);
    const RS_Insert* secondInsert = atX<RS_Insert>(d, RS2::EntityInsert, 100);
    const RS_Text* secondText = atX<RS_Text>(d, RS2::EntityText, 100);
    CHECK(firstInsert->lineage() != insertLineage);
    CHECK(secondInsert->lineage() != insertLineage);
    CHECK(secondInsert->lineage() != firstInsert->lineage());
    CHECK(secondText->ownerLineage() == secondInsert->lineage());
    for (const RS_Entity* text : d.live(RS2::EntityText)) {
        CHECK(text->ownerLineage() != firstInsert->lineage());
    }
}

namespace {

std::string ltypeRecordDxf(const std::string& name, const std::string& description, const std::vector<double>& dashes) {
    double length = 0;
    for (const double dash : dashes) {
        length += std::abs(dash);
    }
    std::string record = "0\nLTYPE\n2\n" + name + "\n70\n0\n3\n" + description + "\n72\n65\n73\n"
        + std::to_string(dashes.size()) + "\n40\n" + std::to_string(length) + "\n";
    for (const double dash : dashes) {
        record += "49\n" + std::to_string(dash) + "\n";
    }
    return record;
}

// An R12 library drawing: VENDOR_X, named by a layer, a block member and a
// top-level line; VENDOR_LIB, named by nothing; a lower-case HIDDEN record
// and a padded DASHED one, which the library's list keeps or drops by its
// seed rules (the seed takes the first, add() deletes the second).
std::string libraryDxf() {
    return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
           "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n4\n"
           + ltypeRecordDxf("VENDOR_X", "Vendor x", {2, -2})
           + ltypeRecordDxf("VENDOR_LIB", "Vendor library", {4, -1})
           + ltypeRecordDxf("hidden", "Vendor hidden", {9, -9})
           + ltypeRecordDxf(" DASHED", "Vendor dashed", {9, -9})
           + "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n2\n"
             "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
             "0\nLAYER\n2\nL_X\n70\n0\n62\n7\n6\nVENDOR_X\n"
             "0\nENDTAB\n0\nENDSEC\n"
             "0\nSECTION\n2\nBLOCKS\n"
             "0\nBLOCK\n8\n0\n2\nKNOB\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n3\nKNOB\n1\n\n"
             "0\nLINE\n8\nL_X\n6\nVENDOR_X\n10\n0.0\n20\n0.0\n11\n1.0\n21\n0.0\n"
             "0\nENDBLK\n8\n0\n0\nENDSEC\n"
             "0\nSECTION\n2\nENTITIES\n"
             "0\nLINE\n8\nL_X\n6\nVENDOR_X\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
             "0\nINSERT\n8\n0\n2\nKNOB\n10\n5.0\n20\n5.0\n30\n0.0\n"
             "0\nENDSEC\n0\nEOF\n";
}

// A drawing of its own: one LTYPE record and a line naming it.
std::string drawingWithLType(const std::string& name, const std::string& description, const std::vector<double>& dashes) {
    return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
           "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n1\n"
           + ltypeRecordDxf(name, description, dashes)
           + "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n1\n"
             "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
             "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
             "0\nLINE\n8\n0\n6\n" + name + "\n10\n0.0\n20\n50.0\n11\n10.0\n21\n50.0\n"
             "0\nENDSEC\n0\nEOF\n";
}

void importText(RS_Graphic& graphic, const std::string& text, const std::string& name) {
    const auto path = tempFile(name);
    std::ofstream(path) << text;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(path.string()), RS2::FormatDXFRW));
    std::filesystem::remove(path);
}

// The values of one group code in each LTYPE record spelt `name` to the
// byte, one vector per record: values() cannot tell the records apart, nor
// a padded name from the plain one.
std::vector<std::vector<std::string>> ltypeRecords(const std::filesystem::path& path, const std::string& name,
                                                   const std::string& code) {
    std::ifstream in(path);
    std::vector<std::vector<std::string>> records;
    std::string c;
    std::string v;
    bool inLtype = false;
    bool named = false;
    auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t\r") + 1);
        return s;
    };
    while (std::getline(in, c) && std::getline(in, v)) {
        c = trim(c);
        if (!v.empty() && v.back() == '\r') {
            v.pop_back();
        }
        if (c == "0") {
            inLtype = trim(v) == "LTYPE";
            named = false;
        }
        else if (inLtype && c == "2") {
            named = v == name;
            if (named) {
                records.emplace_back();
            }
        }
        else if (named && c == code) {
            records.back().push_back(trim(v));
        }
    }
    return records;
}

using Dashes = std::vector<std::vector<double>>;

Dashes ltypeDashes(const std::filesystem::path& path, const std::string& name) {
    Dashes dashes;
    for (const auto& record : ltypeRecords(path, name, "49")) {
        dashes.emplace_back();
        for (const std::string& value : record) {
            dashes.back().push_back(std::stod(value));
        }
    }
    return dashes;
}

std::unique_ptr<Drawing> libraryDrawing() {
    auto source = std::make_unique<Drawing>();
    importText(source->m_graphic, libraryDxf(), "library.dxf");
    // The seeds, VENDOR_X and VENDOR_LIB: HIDDEN took the record spelt
    // `hidden`, the padded DASHED record made no entry.
    REQUIRE(source->m_graphic.countLineTypes() == 37);
    return source;
}

void insertLibrary(Drawing& destination, std::unique_ptr<Drawing>& source) {
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::libraryInsert(LC_LibraryInsertData(RS_Vector{0, 0}, 1, 0, "PART", &source->m_graphic),
                                       &destination.m_graphic, ctx);
    });
    source.reset(); // nothing inserted may depend on the library drawing
}

void importBack(RS_Graphic& graphic, const std::filesystem::path& path, const RS2::FormatType format) {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(path.string()), format));
}

} // namespace

// The library's line types come as fresh copies, the whole table as every
// layer does; its built-ins never do.
TEST_CASE("A library insert brings the library drawing's line types as new entries", "[copy][library][linetype]") {
    auto source = libraryDrawing();
    const LC_LineType* sourceX = source->m_graphic.findLineType("VENDOR_X");
    REQUIRE(sourceX != nullptr);
    REQUIRE(sourceX->hasImportedRecord);

    Drawing destination;
    destination.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::libraryInsert(LC_LibraryInsertData(RS_Vector{0, 0}, 1, 0, "PART", &source->m_graphic),
                                       &destination.m_graphic, ctx);
    });
    // A copy of the library's entry; the library drawing keeps its own.
    const LC_LineType* x = destination.m_graphic.findLineType("VENDOR_X");
    REQUIRE(x != nullptr);
    CHECK(x != sourceX);
    CHECK(source->m_graphic.countLineTypes() == 37);
    CHECK(source->m_graphic.findLineType("VENDOR_X") == sourceX);
    source.reset(); // nothing inserted may depend on the library drawing

    CHECK(x->name == "VENDOR_X");
    CHECK(x->description == "Vendor x");
    CHECK(x->pattern == std::vector<double>{2, -2});
    CHECK(x->origin == LC_LineType::Origin::Imported);
    // No LTYPE record of this drawing backs it: the writer makes one from the entry.
    CHECK_FALSE(x->hasImportedRecord);
    CHECK(destination.m_graphic.dwgAdvancedMetadata().findLineTypeTableEntryByName("VENDOR_X") == nullptr);
    // The whole table comes, named by something or not.
    const LC_LineType* lib = destination.m_graphic.findLineType("VENDOR_LIB");
    REQUIRE(lib != nullptr);
    CHECK(lib->pattern == std::vector<double>{4, -1});
    CHECK(destination.m_graphic.countLineTypes() == 37);
    // The built-ins stay this drawing's own, however the library spelt or dashed them.
    const LC_LineType* hidden = destination.m_graphic.findLineType("HIDDEN");
    REQUIRE(hidden != nullptr);
    CHECK(hidden == destination.m_graphic.lineTypeAt(11));
    CHECK(hidden->name == "HIDDEN");
    CHECK(hidden->pattern == LC_LineTypeNames::builtinMetrics()[11].pattern);
    CHECK(hidden->origin == LC_LineType::Origin::BuiltIn);
    CHECK_FALSE(hidden->hasImportedRecord);
    CHECK(destination.m_graphic.findLineType(" DASHED") == destination.m_graphic.lineTypeAt(7));
    CHECK(destination.m_graphic.lineTypeAt(7)->name == "DASHED");
    CHECK(lc::test::documentProblems(destination.m_graphic).isEmpty());
}

// The pens are what they were, and the file gets one record
// per entry with the entry's dashes, in DXF and DWG, and nothing of the
// library's archive.
TEST_CASE("A library insert's line types reach the destination's pens and file", "[copy][library][linetype]") {
    auto source = libraryDrawing();
    Drawing destination;
    insertLibrary(destination, source);

    // The cloned layer's pen and the block member's. (The part block's own
    // members are ByBlock, as addByBlockEntity has always made them.)
    RS_Layer* lx = destination.m_graphic.findLayer("L_X");
    REQUIRE(lx != nullptr);
    CHECK(lx->getPen().getLineTypeName() == "VENDOR_X");
    const RS_Block* knob = destination.m_graphic.findBlock("KNOB");
    REQUIRE(knob != nullptr);
    REQUIRE(knob->firstEntity() != nullptr);
    CHECK(knob->firstEntity()->getPen(false).getLineTypeName() == "VENDOR_X");

    for (const RS2::FormatType format : {RS2::FormatDXFRW, RS2::FormatDXFRW12}) {
        INFO("format " << static_cast<int>(format));
        const auto dxf = tempFile("library_linetype.dxf");
        REQUIRE(save(destination.m_graphic, dxf, format));
        // One record per entry, with the entry's dashes and description...
        CHECK(ltypeDashes(dxf, "VENDOR_X") == Dashes{{2, -2}});
        CHECK(ltypeDashes(dxf, "VENDOR_LIB") == Dashes{{4, -1}});
        if (format != RS2::FormatDXFRW12) {
            CHECK(ltypeRecords(dxf, "VENDOR_X", "3") == std::vector<std::vector<std::string>>{{"Vendor x"}});
        }
        // ...and nothing of the library's archive: its HIDDEN and DASHED
        // records stayed there. The head's 35, VENDOR_X and VENDOR_LIB.
        CHECK(ltypeDashes(dxf, "hidden").empty());
        CHECK(ltypeDashes(dxf, " DASHED").empty());
        CHECK(values(dxf, "LTYPE", "2").size() == 37);
        // Read back, each entry is a record's.
        RS_Graphic again;
        importBack(again, dxf, RS2::FormatDXFRW);
        const LC_LineType* x = again.findLineType("VENDOR_X");
        REQUIRE(x != nullptr);
        CHECK(x->pattern == std::vector<double>{2, -2});
        CHECK(x->hasImportedRecord);
        CHECK(again.findLineType("VENDOR_LIB") != nullptr);
        CHECK(again.countLineTypes() == 37);
        std::filesystem::remove(dxf);
    }

    // DWG keeps group 40 as the double it is given: the dashes and their sum.
    const auto dwg = tempFile("library_linetype.dwg");
    REQUIRE(save(destination.m_graphic, dwg, RS2::FormatDWG2004));
    RS_Graphic fromDwg;
    importBack(fromDwg, dwg, RS2::FormatDWG);
    const auto& meta = fromDwg.dwgAdvancedMetadata();
    const DRW_LType* libRecord = meta.findLineTypeTableEntryByName("VENDOR_LIB");
    REQUIRE(libRecord != nullptr);
    CHECK(libRecord->path == std::vector<double>{4, -1});
    CHECK(libRecord->length == 5.0);
    CHECK(libRecord->desc == "Vendor library");
    const DRW_LType* xRecord = meta.findLineTypeTableEntryByName("VENDOR_X");
    REQUIRE(xRecord != nullptr);
    CHECK(xRecord->path == std::vector<double>{2, -2});
    CHECK(xRecord->length == 4.0);
    CHECK(fromDwg.countLineTypes() == 37);
    std::filesystem::remove(dwg);
}

// A line type the destination defines stays as it is, spelling, dashes and
// description, as a layer or a block of the same name does.
TEST_CASE("A library insert leaves a line type the destination already defines alone", "[copy][library][linetype]") {
    auto source = libraryDrawing();
    Drawing destination;
    importText(destination.m_graphic, drawingWithLType("Vendor_x", "Own x", {5, -5}), "own_x.dxf");
    REQUIRE(destination.m_graphic.countLineTypes() == 36);
    const LC_LineType* own = destination.m_graphic.findLineType("VENDOR_X");
    REQUIRE(own != nullptr);
    insertLibrary(destination, source);

    CHECK(destination.m_graphic.findLineType("VENDOR_X") == own);
    CHECK(own->name == "Vendor_x");
    CHECK(own->pattern == std::vector<double>{5, -5});
    CHECK(own->description == "Own x");
    CHECK(own->hasImportedRecord);
    CHECK(destination.m_graphic.countLineTypes() == 37);
    // The inserted pens keep the library's spelling; the file has the one
    // record, the destination's, which that spelling resolves to.
    const RS_Block* knob = destination.m_graphic.findBlock("KNOB");
    REQUIRE(knob != nullptr);
    REQUIRE(knob->firstEntity() != nullptr);
    CHECK(knob->firstEntity()->getPen(false).getLineTypeName() == "VENDOR_X");
    const auto dxf = tempFile("own_x_out.dxf");
    REQUIRE(save(destination.m_graphic, dxf, RS2::FormatDXFRW));
    CHECK(ltypeDashes(dxf, "Vendor_x") == Dashes{{5, -5}});
    CHECK(ltypeDashes(dxf, "VENDOR_X").empty());
    CHECK(values(dxf, "LTYPE", "2").size() == 37);
    std::filesystem::remove(dxf);
}

// The merge sits beside the layer clones, outside the undo section: undo
// removes the insert and keeps the entries, as it keeps the layers.
TEST_CASE("Undoing a library insert keeps its line types, as it keeps its layers", "[copy][library][linetype][undo]") {
    auto source = libraryDrawing();
    Drawing destination;
    insertLibrary(destination, source);

    REQUIRE(destination.m_graphic.undo());
    CHECK(destination.live(RS2::EntityInsert).isEmpty());
    CHECK(destination.m_graphic.findLayer("L_X") != nullptr);
    const LC_LineType* x = destination.m_graphic.findLineType("VENDOR_X");
    REQUIRE(x != nullptr);
    CHECK(x->pattern == std::vector<double>{2, -2});
    REQUIRE(destination.m_graphic.redo());
    CHECK(destination.m_graphic.findLineType("VENDOR_X") == x);
    CHECK(destination.m_graphic.countLineTypes() == 37);
}

// A record that only names a line type does not keep a definition out: the
// entry keeps the destination's spelling, description and record and takes
// the library's dashes, and the file gets that record with them, once.
TEST_CASE("A record that only names a line type takes the library's dashes", "[copy][library][linetype]") {
    auto source = libraryDrawing();
    Drawing destination;
    importText(destination.m_graphic, drawingWithLType("VENDOR_X", "Name only", {}), "name_only.dxf");
    const LC_LineType* own = destination.m_graphic.findLineType("VENDOR_X");
    REQUIRE(own != nullptr);
    REQUIRE(own->pattern.empty());
    REQUIRE(own->hasImportedRecord);
    insertLibrary(destination, source);

    CHECK(destination.m_graphic.findLineType("VENDOR_X") == own);
    CHECK(own->pattern == std::vector<double>{2, -2});
    CHECK(own->name == "VENDOR_X");
    CHECK(own->description == "Name only");
    CHECK(own->hasImportedRecord);
    CHECK(destination.m_graphic.countLineTypes() == 37);

    for (const RS2::FormatType format : {RS2::FormatDXFRW, RS2::FormatDXFRW12}) {
        INFO("format " << static_cast<int>(format));
        const auto dxf = tempFile("name_only_out.dxf");
        REQUIRE(save(destination.m_graphic, dxf, format));
        CHECK(ltypeDashes(dxf, "VENDOR_X") == Dashes{{2, -2}});
        if (format != RS2::FormatDXFRW12) {
            CHECK(ltypeRecords(dxf, "VENDOR_X", "3") == std::vector<std::vector<std::string>>{{"Name only"}});
        }
        CHECK(values(dxf, "LTYPE", "2").size() == 37);
        std::filesystem::remove(dxf);
    }
    const auto dwg = tempFile("name_only_out.dwg");
    REQUIRE(save(destination.m_graphic, dwg, RS2::FormatDWG2004));
    RS_Graphic fromDwg;
    importBack(fromDwg, dwg, RS2::FormatDWG);
    const DRW_LType* record = fromDwg.dwgAdvancedMetadata().findLineTypeTableEntryByName("VENDOR_X");
    REQUIRE(record != nullptr);
    CHECK(record->path == std::vector<double>{2, -2});
    CHECK(record->length == 4.0);
    std::filesystem::remove(dwg);
}
