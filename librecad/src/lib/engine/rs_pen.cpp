/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "rs_pen.h"

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lc_linetypenames.h"

namespace {
struct InternedLineType {
    QString       spelling;   // exactly as the caller gave it, after trim
    std::uint16_t foldId;     // 0 == "this name means its own enum"
};

// The interned spellings: built on first use, appended to for the life of the
// process and destroyed at exit. Ids are handed out from 1, id N is
// table()[N - 1], and the store grows from user input up to 65535 spellings.
std::vector<InternedLineType> &table() {
    static std::vector<InternedLineType> store;
    return store;
}

// Raw spelling -> spelling id.
std::unordered_map<QString, std::uint16_t> &bySpelling() {
    static std::unordered_map<QString, std::uint16_t> map;
    return map;
}

// Fold -> the id every spelling of that fold shares.
std::unordered_map<QString, std::uint16_t> &byFold() {
    static std::unordered_map<QString, std::uint16_t> map;
    return map;
}

// Interns trimmed, returning {spelling id, fold id}. Fold id 0 means the name is
// the canonical name of its own enum, the one rule this function computes. Runs
// once per distinct spelling; no lock, because the engine is single-threaded.
std::pair<std::uint16_t, std::uint16_t> internLineTypeName(const QString &trimmed,
                                                           const RS2::LineType lineType) {
    const auto known = bySpelling().find(trimmed);
    if (known != bySpelling().end()) {
        return {known->second, table()[known->second - 1].foldId};
    }
    // Exhausted: degrade to the enum's canonical name. No throw, no log.
    if (table().size() >= 65535) {
        return {0, 0};
    }

    // Both sides are folded: lineTypeToName() returns the mixed-case "ByLayer"
    // and "ByBlock", which a raw comparison would miss.
    const QString key = LC_LineTypeNames::foldName(trimmed);
    const QString canonical =
        LC_LineTypeNames::foldName(LC_LineTypeNames::lineTypeToName(lineType));
    const auto id = static_cast<std::uint16_t>(table().size() + 1);
    // A fold id is the id of the first spelling interned for that fold, so the
    // emplace both reads it and records this spelling as the first of a new one.
    const std::uint16_t foldId =
        (key == canonical) ? 0 : byFold().emplace(key, id).first->second;

    table().push_back({trimmed, foldId});
    bySpelling().emplace(trimmed, id);
    return {id, foldId};
}
} // namespace

RS_Pen::RS_Pen() :
    RS_Pen(RS_Color{Qt::black}, RS2::WidthByLayer, RS2::LineByLayer)
{}

void RS_Pen::setLineTypeName(const QString &name) {
    // nameToLineType("") is LineByLayer but nameToLineType("   ") is SolidLine,
    // so the trim belongs here rather than in the fold.
    const QString trimmed = name.trimmed();
    m_lineType = LC_LineTypeNames::nameToLineType(trimmed);
    if (trimmed.isEmpty()) {
        m_lineTypeId = 0;
        m_lineTypeFoldId = 0;
        return;
    }
    const auto ids = internLineTypeName(trimmed, m_lineType);
    m_lineTypeId = ids.first;
    m_lineTypeFoldId = ids.second;
}

QString RS_Pen::getLineTypeName() const {
    if (m_lineTypeId != 0) {
        return table()[m_lineTypeId - 1].spelling;
    }
    return LC_LineTypeNames::lineTypeToName(m_lineType);
}

std::ostream& operator << (std::ostream& os, const RS_Pen& p) {
    //os << "style: " << p.style << std::endl;
    os << " pen color: " << p.getColor()
    << " pen width: " << p.getWidth()
    << " pen screen width: " << p.getScreenWidth()
    << " pen line type: " << p.getLineType()
    << " flags: " << (p.getFlag(RS2::FlagInvalid) ? "INVALID" : "")
    << std::endl;
    return os;
}
