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

#ifndef LC_PROPERTYEDITCONTEXT_H
#define LC_PROPERTYEDITCONTEXT_H

#include <QWidget>

#include "lc_property.h"

// fixme - sand - no copy assignment operator!
class LC_PropertyEditContext {
public:
    using FunCreateEditor = std::function<QWidget *()>;
    LC_PropertyEditContext();
    LC_PropertyEditContext(const LC_PropertyEditContext& other);
    LC_Property* getProperty() const;
    QWidget* createEditor() const;
    bool isValid() const;
    void setup(LC_Property* property, const FunCreateEditor& funCreateEditor);

private:
    LC_Property* m_property;
    FunCreateEditor m_funCreateEditor;
};

inline LC_Property* LC_PropertyEditContext::getProperty() const {
    return m_property;
}

#endif
