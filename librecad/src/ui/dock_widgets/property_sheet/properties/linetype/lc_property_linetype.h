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

#ifndef LC_PROPERTYLINETYPE_H
#define LC_PROPERTYLINETYPE_H

#include "lc_property_single.h"
#include "rs.h"

class LC_LineTypeList;
class RS_Pen;

// The enum alone loses drawing-defined names; a name alone loses values such
// as NoPen that have no DXF name. Preserve both components in the editor.
struct LC_LineTypeIdentity {
    RS2::LineType type = RS2::LineByLayer;
    QString name;
    bool hasName = false;
    static LC_LineTypeIdentity fromPen(const RS_Pen& pen);
    void applyTo(RS_Pen& pen) const;
    bool operator==(const LC_LineTypeIdentity& other) const {
        return type == other.type && hasName == other.hasName &&
               (!hasName || name == other.name);
    }
};
Q_DECLARE_METATYPE(LC_LineTypeIdentity)

class LC_PropertyLineType : public LC_PropertySingle<RS2::LineType> {
    Q_OBJECT

public:
    using ValueType = typename RS2::LineType;
    LC_PropertyLineType(const LC_PropertyLineType& other) = delete;

    explicit LC_PropertyLineType(QObject* parent, bool holdValue = true)
        : LC_PropertySingle(parent, holdValue) {
    }
};

// Entity pens retain drawing-defined names in addition to the legacy enum.
class LC_PropertyNamedLineType : public LC_PropertySingle<LC_LineTypeIdentity> {
    Q_OBJECT
public:
    using ValueType = LC_LineTypeIdentity;
    explicit LC_PropertyNamedLineType(QObject* parent, bool holdValue = true)
        : LC_PropertySingle(parent, holdValue) {}
    void setLineTypeList(const LC_LineTypeList* list) { m_list = list; }
    const LC_LineTypeList* lineTypeList() const { return m_list; }
private:
    const LC_LineTypeList* m_list = nullptr; // owned by the active drawing
};

#endif
