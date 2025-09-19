/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_textstylelist.h"

LC_TextStyleList::LC_TextStyleList() {
}

unsigned int LC_TextStyleList::count() const {
    return m_styles.count();
}

void LC_TextStyleList::clear() {
    m_styles.clear(); // fixme - sand - check whether items should be deleted!
}

LC_TextStyle* LC_TextStyleList::at(unsigned int i) {
    return m_styles.at(i);
}

void LC_TextStyleList::addStyle(LC_TextStyle* style) {
    if (style == nullptr) {
        return;
    }
    auto* s = find(style->getName());
    if (s == nullptr) {
        m_styles.append(style);
        setModified(true);
    }
    // fixme - notify it duplicate?
}

void LC_TextStyleList::remove(LC_TextStyle* style) {
    if (style != nullptr) {
        m_styles.removeOne(style);
        setModified(true);
    }
}

void LC_TextStyleList::remove(const QString& name) {
    auto* v = find(name);
    if (v != nullptr) {
        remove(v);
    }
}

LC_TextStyle* LC_TextStyleList::find(const QString& name) {
    // amount of styles should be small, so linear search should be fine
    for (auto v : m_styles) {
        if (v->getName() == name) {
            // fixme - case sensitivity?
            return v;
        }
    }
    return nullptr;
}

void LC_TextStyleList::replace(QList<LC_TextStyle*> newStylesList) {
    qDeleteAll(m_styles);
    m_styles.clear();
    m_styles.append(newStylesList);
    setModified(true);
}
