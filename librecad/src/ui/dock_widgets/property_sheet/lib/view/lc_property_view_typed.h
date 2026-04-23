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

#ifndef LC_PROPERTYVIEWTYPED_H
#define LC_PROPERTYVIEWTYPED_H

#include "lc_property_view_editable.h"

template <typename PropertyClass, typename ParentViewClass = LC_PropertyViewEditable>
class LC_PropertyViewTyped : public ParentViewClass, public QObject {
    Q_DISABLE_COPY(LC_PropertyViewTyped)

public:
    using ValueType = typename PropertyClass::ValueType;

    const PropertyClass& typedProperty() const {
        return *static_cast<const PropertyClass*>(this->getProperty());
    }

    PropertyClass& typedProperty() {
        return *static_cast<PropertyClass*>(this->getProperty());
    }

    ValueType propertyValue() const {
        return typedProperty().value();
    }

protected:
    explicit LC_PropertyViewTyped(PropertyClass* owner)
        : ParentViewClass(owner) {
    }
};

#endif
