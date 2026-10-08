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

#ifndef LC_LINETYPELIST_H
#define LC_LINETYPELIST_H

#include <QList>

#include "lc_linetype.h"

/**
 * The line types of a drawing, the 35 built-ins first, one entry per key().
 * The list owns its entries: add() keeps or deletes its argument, like
 * RS_LayerList::add().
 */
class LC_LineTypeList {
public:
    LC_LineTypeList();
    ~LC_LineTypeList();
    LC_LineTypeList(const LC_LineTypeList&) = delete;
    LC_LineTypeList& operator=(const LC_LineTypeList&) = delete;
    /// frees every entry and seeds the built-ins again
    void clear();

    unsigned count() const {
        return m_entries.count();
    }

    LC_LineType* at(unsigned i) const {
        return m_entries.at(i);
    }

    /// by key(): blanks around the name, ASCII case and Unicode form do not matter
    LC_LineType* find(const QString& name) const;
    /// takes ownership; returns the entry that holds the key afterwards
    LC_LineType* add(LC_LineType* entry);

    bool isModified() const {
        return m_modified;
    }

    void setModified(bool m) {
        m_modified = m;
    }

private:
    void seedBuiltins();

    QList<LC_LineType*> m_entries;
    bool m_modified = false;
};

#endif
