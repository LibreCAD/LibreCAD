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

#include "lc_property_edit_context.h"

LC_PropertyEditContext::LC_PropertyEditContext()
    : m_property(nullptr) {
}

LC_PropertyEditContext::LC_PropertyEditContext(const LC_PropertyEditContext& other) {
    m_property = other.m_property;
    m_funCreateEditor = other.m_funCreateEditor;
}

QWidget* LC_PropertyEditContext::createEditor() const {
    if (m_funCreateEditor == nullptr) {
        return nullptr;
    }
    return m_funCreateEditor();
}

bool LC_PropertyEditContext::isValid() const {
    return m_property != nullptr && m_funCreateEditor != nullptr;
}

void LC_PropertyEditContext::setup(LC_Property* property, const FunCreateEditor& funCreateEditor) {
    Q_ASSERT(property);
    Q_ASSERT(funCreateEditor);
    m_property = property;
    m_funCreateEditor = funCreateEditor;
}
