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

#ifndef LC_PROPERTYENUM_H
#define LC_PROPERTYENUM_H

#include "lc_enum_descriptor.h"
#include "lc_property_single.h"

class LC_PropertyEnum : public LC_PropertySingle<LC_PropertyEnumValueType> {
    Q_OBJECT
public:
    explicit LC_PropertyEnum(QObject* parent, bool holdValue = true);
    LC_PropertyEnum(const LC_PropertyEnum& other) = delete;

    ~LC_PropertyEnum() override {
        if (m_ownDescriptor) {
            delete m_enumInfo;
        }
    }

    const LC_EnumDescriptor* getEnumDescriptor() const {
        return m_enumInfo;
    }

    void setEnumInfo(const LC_EnumDescriptor* enumInfo, const bool ownDescriptor = false) {
        m_enumInfo = enumInfo;
        m_ownDescriptor = ownDescriptor;
    }

protected:
    bool doAcceptValue(ValueType valueToAccept) override;

private:
    const LC_EnumDescriptor* m_enumInfo;
    bool m_ownDescriptor = false;
};

#endif
