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
// it adds in its place, and every other copy is new.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

#include "doc_plugin_interface.h"
#include "lc_actiontestsupport.h"
#include "lc_documentinvariants.h"
#include "rs_block.h"
#include "rs_filterdxfrw.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"

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
