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

#ifndef LC_TEXTSTYLELIST_H
#define LC_TEXTSTYLELIST_H
#include <QList>

#include "lc_textstyle.h"

class LC_TextStyleList{
public:
    LC_TextStyleList();
    virtual ~LC_TextStyleList() = default;
    void clear();
    unsigned int count() const;
    LC_TextStyle* at(unsigned int i);
    void addStyle(LC_TextStyle* style);
    void remove(LC_TextStyle* style);
    void remove(const QString& name);
    LC_TextStyle* find(const QString& name);
    void replace(QList<LC_TextStyle*> newStylesList);
    const QList<LC_TextStyle*>* getStyles() const {return &m_styles;}
    LC_TextStyle* getActiveStyle() const {return m_activeStyle;}
    bool isModified() const {return m_modified;}
    void setModified(bool modified) {m_modified = modified;}
private:
    QList<LC_TextStyle*> m_styles;
    LC_TextStyle *m_activeStyle = nullptr;
    bool m_modified = false;
};

#endif // LC_TEXTSTYLELIST_H
