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

#include "lc_enum_value_descriptor.h"

LC_EnumValueDescriptor::LC_EnumValueDescriptor()
    : m_value(0), m_state(EnumValueStateInvalid) {
}

LC_EnumValueDescriptor::LC_EnumValueDescriptor(const LC_PropertyEnumValueType value, const QString& name, const LC_EnumValueState state)
    : LC_EnumValueDescriptor(value, name, name, state) {
}

LC_EnumValueDescriptor::LC_EnumValueDescriptor(const LC_PropertyEnumValueType value, const QString& name, const QString& displayName,
                                               const LC_EnumValueState state)
    : m_value(value), m_name(name), m_displayName(displayName), m_state(state) {
    if (displayName.isEmpty()) {
        m_displayName = name;
    }
}
