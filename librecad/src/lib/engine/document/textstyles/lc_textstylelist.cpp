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

#include <utility>

LC_TextStyleList::LC_TextStyleList() {
}

LC_TextStyleList::LC_TextStyleList(const LC_TextStyleList &other) {
    try {
        for (auto *style : other.m_styles) {
            auto copy = std::make_unique<LC_TextStyle>(*style);
            m_styles.append(copy.get());
            if (style == other.m_activeStyle) m_activeStyle = copy.get();
            copy.release();
        }
        m_modified = other.m_modified;
    } catch (...) {
        qDeleteAll(m_styles);
        throw;
    }
}

LC_TextStyleList &LC_TextStyleList::operator=(const LC_TextStyleList &other) {
    if (this != &other) {
        LC_TextStyleList incoming(other);
        swapState(incoming);
    }
    return *this;
}

LC_TextStyleList::~LC_TextStyleList() {
    qDeleteAll(m_styles);
}

void LC_TextStyleList::swapState(LC_TextStyleList &other) noexcept {
    m_styles.swap(other.m_styles);
    std::swap(m_activeStyle, other.m_activeStyle);
    std::swap(m_modified, other.m_modified);
}

unsigned int LC_TextStyleList::count() const {
    return m_styles.count();
}

void LC_TextStyleList::clear() {
    qDeleteAll(m_styles);
    m_styles.clear();
    m_activeStyle = nullptr;
    m_modified = false;
}

LC_TextStyle* LC_TextStyleList::at(const unsigned int i) const {
    return m_styles.at(i);
}

void LC_TextStyleList::addStyle(LC_TextStyle* style) {
    if (style == nullptr) {
        return;
    }
    if (m_styles.contains(style)) return;
    std::unique_ptr<LC_TextStyle> owned(style);
    if (find(style->getName()) != nullptr) return;
    m_styles.append(style);
    if (m_activeStyle == nullptr) m_activeStyle = style;
    owned.release();
    setModified(true);
}

LC_TextStyle *LC_TextStyleList::upsertStyle(std::unique_ptr<LC_TextStyle> style) {
    if (!style) return nullptr;
    // A mistakenly re-wrapped owned pointer is never a transfer, even when
    // another renamed record now has the same name earlier in the list.
    if (m_styles.contains(style.get())) return style.release();
    auto *existing = find(style->getName());
    if (existing) {
        const auto index = m_styles.indexOf(existing);
        m_styles[index] = style.get();
        if (m_activeStyle == existing) m_activeStyle = style.get();
        delete existing;
    } else {
        m_styles.append(style.get());
        if (!m_activeStyle) m_activeStyle = style.get();
    }
    setModified(true);
    return style.release();
}

void LC_TextStyleList::remove(LC_TextStyle* style) {
    if (style != nullptr && m_styles.removeOne(style)) {
        if (m_activeStyle == style) m_activeStyle = m_styles.isEmpty() ? nullptr : m_styles.front();
        delete style;
        setModified(true);
    }
}

void LC_TextStyleList::remove(const QString& name) {
    auto* v = find(name);
    if (v != nullptr) {
        remove(v);
    }
}

LC_TextStyle* LC_TextStyleList::find(const QString& name) const {
    // fixme - sand - merge -> caching normalized?
    // amount of styles should be small, so linear search should be fine.
    // NFC-normalize both sides for robust matching of CJK / accented names.
    const QString k = name.normalized(QString::NormalizationForm_C);
    for (auto v : m_styles) {
        if (v->getName().normalized(QString::NormalizationForm_C) == k) {
            // fixme - case sensitivity?
            return v;
        }
    }
    return nullptr;
}

void LC_TextStyleList::replace(const QList<LC_TextStyle*>& newStylesList) {
    // Prepare first: newStylesList may itself be getStyles(), and may contain
    // reused current pointers. Delete only objects absent from the final list.
    QList<LC_TextStyle *> incoming;
    QList<LC_TextStyle *> rejected;
    for (auto *style : newStylesList) {
        if (!style || incoming.contains(style) || rejected.contains(style)) continue;
        bool duplicate = false;
        for (auto *present : incoming)
            if (present->getName().normalized(QString::NormalizationForm_C) ==
                style->getName().normalized(QString::NormalizationForm_C)) {duplicate = true; break;}
        (duplicate ? rejected : incoming).append(style);
    }
    auto old = m_styles;
    m_styles.swap(incoming);
    m_activeStyle = m_styles.contains(m_activeStyle) ? m_activeStyle :
        (m_styles.isEmpty() ? nullptr : m_styles.front());
    for (auto *style : old) if (!m_styles.contains(style)) delete style;
    for (auto *style : rejected) if (!old.contains(style)) delete style;
    setModified(true);
}
