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

#include "lc_enum_descriptor.h"

#include <QCoreApplication>

LC_EnumDescriptor::LC_EnumDescriptor()
    : m_caseSensitivity(Qt::CaseInsensitive) {
}

LC_EnumDescriptor::LC_EnumDescriptor(const QString& name)
    : m_caseSensitivity(Qt::CaseInsensitive), m_name(name) {
}

LC_EnumDescriptor::LC_EnumDescriptor(const QString& name, QVector<LC_EnumValueDescriptor>& staticValues)
    : m_caseSensitivity(Qt::CaseInsensitive), m_name(name) {
    m_values.swap(staticValues);
}

LC_EnumDescriptor::LC_EnumDescriptor(const QString& name, const QVector<LC_EnumValueDescriptor>& staticValues)
    : m_caseSensitivity(Qt::CaseSensitive), m_name(name), m_values(staticValues) {
}

const LC_EnumValueDescriptor* LC_EnumDescriptor::findByName(const QString& name) const {
    const LC_EnumValueDescriptor* result = nullptr;
    doProcessEachEnumValue([&result, &name, this](const LC_EnumValueDescriptor& enumValue) -> bool {
        if (QString::compare(enumValue.getName(), name, m_caseSensitivity) == 0) {
            result = &enumValue;
            return false;
        }
        return true;
    });

    return result;
}

const LC_EnumValueDescriptor* LC_EnumDescriptor::findByDisplayName(const QString& displayName, Qt::CaseSensitivity cs) const {
    const LC_EnumValueDescriptor* result = nullptr;

    doProcessEachEnumValue([&result, &displayName, cs](const LC_EnumValueDescriptor& enumValue) -> bool {
        if (QString::compare(enumValue.getDisplayName(), displayName, cs) == 0) {
            result = &enumValue;
            return false;
        }
        return true;
    });
    return result;
}

const LC_EnumValueDescriptor* LC_EnumDescriptor::findByValue(LC_PropertyEnumValueType value) const {
    const LC_EnumValueDescriptor* result = nullptr;
    doProcessEachEnumValue([&result, value](const LC_EnumValueDescriptor& enumValue) -> bool {
        if (enumValue.getValue() == value) {
            result = &enumValue;
            return false;
        }
        return true;
    });
    return result;
}

bool LC_EnumDescriptor::toString(const LC_EnumValueDescriptor* value, QString& str) const {
    if (value == nullptr) {
        return false;
    }

    str = value->getName();
    return true;
}

bool LC_EnumDescriptor::toString(const LC_PropertyEnumValueType value, QString& str) const {
    return toString(findByValue(value), str);
}
