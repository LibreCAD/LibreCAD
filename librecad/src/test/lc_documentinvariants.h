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

// What a drawing must satisfy after any operation, as a list of problems
// (empty when all hold). Test code only.

#ifndef LC_DOCUMENTINVARIANTS_H
#define LC_DOCUMENTINVARIANTS_H

#include <fstream>
#include <map>
#include <set>
#include <string>

#include <QString>
#include <QStringList>

#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

namespace lc::test {

inline QString describe(const RS_Entity* e) {
    return QStringLiteral("entity %1 (rtti %2)").arg(e->getId()).arg(static_cast<int>(e->rtti()));
}

/** Each child appears once, and an owning container is its children's parent, at every level. */
inline void collectContainerProblems(const RS_EntityContainer& container, QStringList& problems) {
    std::set<const RS_Entity*> seen;
    for (const RS_Entity* e : container) {
        if (e == nullptr) {
            continue;
        }
        if (!seen.insert(e).second) {
            problems << describe(e) + QStringLiteral(" appears twice in its container");
        }
        if (container.isOwner() && e->getParent() != &container) {
            problems << describe(e) + QStringLiteral(" is owned by one container but names another as parent");
        }
        if (e->isContainer()) {
            collectContainerProblems(*static_cast<const RS_EntityContainer*>(e), problems);
        }
    }
}

inline QStringList containerProblems(const RS_EntityContainer& container) {
    QStringList problems;
    collectContainerProblems(container, problems);
    return problems;
}

/**
 * containerProblems() over the model space and every block, plus: live
 * entities' layers belong to the drawing, live source handles are unique,
 * and live inserts name a block of the drawing.
 */
inline QStringList documentProblems(RS_Graphic& graphic) {
    QStringList problems = containerProblems(graphic);
    std::set<const RS_Layer*> layers;
    for (const RS_Layer* layer : *graphic.getLayerList()) {
        layers.insert(layer);
    }
    std::map<quint32, int> handles;
    auto checkTopLevel = [&](const RS_EntityContainer& space) {
        for (RS_Entity* e : space) {
            if (e == nullptr || e->isDeleted()) {
                continue;
            }
            const RS_Layer* layer = e->getLayer(false);
            if (layer != nullptr && layers.count(layer) == 0) {
                problems << describe(e) + QStringLiteral(" is on layer '%1', which is not one of the drawing's").arg(layer->getName());
            }
            if (e->sourceHandle() != 0 && ++handles[e->sourceHandle()] == 2) {
                problems << QStringLiteral("source handle 0x%1 is held by more than one live entity").arg(e->sourceHandle(), 0, 16);
            }
            if (e->rtti() == RS2::EntityInsert) {
                const auto* insert = static_cast<RS_Insert*>(e);
                if (graphic.findBlock(insert->getName()) == nullptr) {
                    problems << describe(e) + QStringLiteral(" inserts block '%1', which the drawing does not have").arg(insert->getName());
                }
            }
        }
    };
    checkTopLevel(graphic);
    for (unsigned i = 0; i < graphic.countBlocks(); ++i) {
        const RS_Block* block = graphic.blockAt(i);
        collectContainerProblems(*block, problems);
        checkTopLevel(*block);
    }
    return problems;
}

/** Owner (330) and hard-owner (360) references in a written DXF that name no handle (5, 105) of the file. */
inline QStringList danglingReferences(const QString& dxfPath) {
    std::ifstream in(dxfPath.toStdString());
    auto trim = [](std::string value) {
        const std::size_t first = value.find_first_not_of(" \t");
        const std::size_t last = value.find_last_not_of(" \t\r");
        return first == std::string::npos ? std::string{} : value.substr(first, last - first + 1);
    };
    std::set<std::string> defined;
    std::map<std::string, std::string> references; // handle -> the code that names it
    std::string code;
    std::string value;
    while (std::getline(in, code) && std::getline(in, value)) {
        code = trim(code);
        value = trim(value);
        if (code == "5" || code == "105") {
            defined.insert(value);
        }
        else if ((code == "330" || code == "360") && value != "0") {
            references.emplace(value, code);
        }
    }
    QStringList problems;
    for (const auto& [handle, byCode] : references) {
        if (defined.count(handle) == 0) {
            problems << QStringLiteral("%1 %2 names no object of the file")
                            .arg(QString::fromStdString(byCode), QString::fromStdString(handle));
        }
    }
    return problems;
}

} // namespace lc::test

#endif
