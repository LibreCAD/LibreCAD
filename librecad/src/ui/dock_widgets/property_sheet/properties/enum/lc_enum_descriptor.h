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

#ifndef LC_ENUMDESCRIPTOR_H
#define LC_ENUMDESCRIPTOR_H

#include <qmetaobject.h>

#include "lc_enum_value_descriptor.h"

class LC_EnumDescriptor {
public:
    LC_EnumDescriptor();
    explicit LC_EnumDescriptor(const QString& name);
    LC_EnumDescriptor(const QString& name, QVector<LC_EnumValueDescriptor>& staticValues);
    LC_EnumDescriptor(const QString& name, const QVector<LC_EnumValueDescriptor>& staticValues);

    inline bool isValid() const;
    inline const QString& getName() const;

    template <typename FunProcessValue>
    bool doProcessEachEnumValue(FunProcessValue pred) const {
        for (const auto& value : m_values) {
            if (!pred(value)) {
                return false;
            }
        }

        return true;
    }

    const LC_EnumValueDescriptor* findByValue(LC_PropertyEnumValueType value) const;
    const LC_EnumValueDescriptor* findByName(const QString& name) const;
    const LC_EnumValueDescriptor* findByDisplayName(const QString& displayName, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    bool toString(const LC_EnumValueDescriptor* value, QString& str) const;
    bool toString(LC_PropertyEnumValueType value, QString& str) const;

private:
    Qt::CaseSensitivity m_caseSensitivity;
    QString m_name;
    QVector<LC_EnumValueDescriptor> m_values;
};

bool LC_EnumDescriptor::isValid() const {
    return !m_name.isEmpty() && !m_values.isEmpty();
}

const QString& LC_EnumDescriptor::getName() const {
    return m_name;
}

#endif
