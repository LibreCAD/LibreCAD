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

#ifndef LC_ENUMVALUEDESCRIPTOR_H
#define LC_ENUMVALUEDESCRIPTOR_H

#include <QString>

using LC_PropertyEnumValueType = qint32;

enum LC_EnumValueStateFlag {
    EnumValueStateNone     = 0x0000,
    EnumValueStateHidden   = 0x0001,
    EnumValueStateObsolete = 0x0002,
    EnumValueStateInvalid  = 0x0004
};

Q_DECLARE_FLAGS(LC_EnumValueState, LC_EnumValueStateFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS (LC_EnumValueState)

class LC_EnumValueDescriptor {
public:
    LC_EnumValueDescriptor();
    LC_EnumValueDescriptor(LC_PropertyEnumValueType value, const QString& name, LC_EnumValueState state = EnumValueStateNone);
    LC_EnumValueDescriptor(LC_PropertyEnumValueType value, const QString& name, const QString& displayName,
                           LC_EnumValueState state = EnumValueStateNone);

    LC_PropertyEnumValueType getValue() const {
        return m_value;
    }

    void setValue(const LC_PropertyEnumValueType value) {
        m_value = value;
    }

    const QString& getName() const {
        return m_name;
    }

    const QString& getDisplayName() const {
        return m_displayName;
    }

    LC_EnumValueState getState() const {
        return m_state;
    }

private:
    LC_PropertyEnumValueType m_value;
    QString m_name;
    QString m_displayName;
    LC_EnumValueState m_state;
};

#endif
