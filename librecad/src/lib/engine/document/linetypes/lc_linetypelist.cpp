/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_linetypelist.h"

#include <QtAlgorithms>

#include "lc_linetypenames.h"

namespace {
// The row a built-in entry was seeded from, whatever spelling it holds now.
const LC_LineTypeNames::BuiltinMetric* seedOf(const LC_LineType& entry) {
    if (entry.origin != LC_LineType::Origin::BuiltIn) {
        return nullptr;
    }
    const RS2::LineType type = LC_LineTypeNames::nameToLineType(entry.key());
    for (const auto& row : LC_LineTypeNames::builtinMetrics()) {
        if (row.type == type) {
            return &row;
        }
    }
    return nullptr;
}

bool padded(const QString& name) {
    return name.trimmed() != name;
}

// A record RS_FilterDXFRW::writeLType() substitutes for a built-in: its name
// passes tableNameEquals() against the canonical one, so it is unpadded and
// ASCII (NFC folds U+212A KELVIN SIGN to K; the writer's byte compare does not).
bool substitutable(const QString& name) {
    if (padded(name)) {
        return false;
    }
    for (const QChar c : name) {
        if (c.unicode() >= 0x80) {
            return false;
        }
    }
    return true;
}

// Unpadded beats padded, then the smaller byte sequence (the archive's map
// order); an exact repeat replaces the earlier one.
bool outranks(const QString& candidate, const QString& holder) {
    if (padded(candidate) != padded(holder)) {
        return padded(holder);
    }
    return candidate.toStdString() <= holder.toStdString();
}
}

LC_LineTypeList::LC_LineTypeList() {
    seedBuiltins();
    setModified(false);
}

LC_LineTypeList::~LC_LineTypeList() {
    qDeleteAll(m_entries);
}

void LC_LineTypeList::seedBuiltins() {
    for (const auto& row : LC_LineTypeNames::builtinMetrics()) {
        auto* entry = new LC_LineType(LC_LineTypeNames::lineTypeToName(row.type));
        entry->description = QString::fromUtf8(row.description);
        entry->pattern = row.pattern;
        entry->origin = LC_LineType::Origin::BuiltIn;
        m_entries.append(entry);
    }
}

void LC_LineTypeList::clear() {
    qDeleteAll(m_entries);
    m_entries.clear();
    seedBuiltins();
    setModified(true);
}

LC_LineType* LC_LineTypeList::find(const QString& name) const {
    const QString key = LC_LineTypeNames::foldName(name.trimmed());
    for (LC_LineType* entry : m_entries) {
        if (entry->key() == key) {
            return entry;
        }
    }
    return nullptr;
}

LC_LineType* LC_LineTypeList::add(LC_LineType* entry) {
    if (entry == nullptr || m_entries.contains(entry)) {
        return entry;
    }
    LC_LineType* existing = find(entry->name);
    if (existing == nullptr) {
        m_entries.append(entry);
        setModified(true);
        return entry;
    }
    const auto* seed = seedOf(*existing);
    if (seed != nullptr && !substitutable(entry->name)) {
        // The DXF writer emits such a record as its own LTYPE and writes the
        // built-in from the literal, so it is no record for this entry.
        delete entry;
        return existing;
    }
    // a seed is no record: the first record for its name takes it over
    const bool untouchedSeed = seed != nullptr && !existing->hasImportedRecord;
    if (untouchedSeed || outranks(entry->name, existing->name)) {
        if (seed != nullptr && entry->pattern.empty() && !seed->pattern.empty()) {
            // a record that only names a built-in keeps its dashes (writeLType)
            entry->pattern = seed->pattern;
            if (entry->description.isEmpty()) {
                entry->description = QString::fromUtf8(seed->description);
            }
        }
        if (existing->name != entry->name || existing->pattern != entry->pattern ||
            existing->description != entry->description) {
            existing->name = entry->name;
            existing->description = entry->description;
            existing->pattern = entry->pattern;
            setModified(true);
        }
    }
    existing->hasImportedRecord |= entry->hasImportedRecord;
    delete entry;
    return existing;
}
