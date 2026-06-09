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

#ifndef LC_PROPERTYVIEWREGISTRATOR_H
#define LC_PROPERTYVIEWREGISTRATOR_H

#include "lc_property_view_factory.h"

class LC_PropertyViewRegistrator {
public:
    explicit LC_PropertyViewRegistrator(LC_PropertyViewFactory& factory)
        : m_factory{factory} {
    }

    void registerViews();

protected:
    template <class Property, class PropertyViewClass>
    void defaultView(const QByteArray& viewName = PropertyViewClass::VIEW_NAME);
    template <class PropertyClass, class PropertyViewClass>
    void anotherView(const QByteArray& viewName = PropertyViewClass::VIEW_NAME);
    LC_PropertyViewFactory& m_factory;
};

#endif
