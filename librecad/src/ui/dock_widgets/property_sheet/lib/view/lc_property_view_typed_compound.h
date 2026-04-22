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

#ifndef LC_PROPERTYVIEWTYPEDEX_H
#define LC_PROPERTYVIEWTYPEDEX_H

#include <deque>

#include "lc_property_view_typed.h"

template <typename PropertyClass, typename ParentViewClass = LC_PropertyViewEditable>
class LC_PropertyViewTypedCompound : public LC_PropertyViewTyped<PropertyClass, ParentViewClass> {
    Q_DISABLE_COPY(LC_PropertyViewTypedCompound)

protected:
    explicit LC_PropertyViewTypedCompound(PropertyClass& property)
        : LC_PropertyViewTyped<PropertyClass, ParentViewClass>(property) {
    }

    void addSubProperty(LC_Property* subProperty) {
        Q_ASSERT(subProperty != nullptr);

        m_subProperties.emplace_back(subProperty);
        subProperty->connectPrimaryState(this->getProperty());
    }

    int doGetSubPropertyCount() const override {
        return static_cast<int>(m_subProperties.size());
    }

    LC_Property* doGetSubProperty(const int index) override {
        return m_subProperties[index].get();
    }

    std::deque<std::unique_ptr<LC_Property>> m_subProperties;
};

#endif
