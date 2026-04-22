/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_PROPERTYFIELDUTILS_H
#define LC_PROPERTYFIELDUTILS_H

#include "lc_property_single.h"
#include "lc_property_utils.h"

template <typename ValueType, typename FieldPropertyType>
class LC_PropertyStructBase : public LC_PropertySingle<ValueType> {
public:
    using Inherited = LC_PropertySingle<ValueType>;
    using ParentClass = LC_PropertyStructBase;

    template <typename GetterType, typename SetterType>
    FieldPropertyType* createFieldProperty(GetterType getter, SetterType setter, const QString& name = QString(),
                                           const QString& displayName = QString(), const QString& descFormat = QString(),
                                           const QByteArray& viewName = QByteArray()) {
        return LC_PropertyFieldUtils::createFieldDelegatedProperty<FieldPropertyType>(
            this, getter, setter, name, displayName, descFormat, viewName);
    }

protected:
    explicit LC_PropertyStructBase(QObject* parent, bool holdValue = true)
        : Inherited(parent, holdValue) {
    }
};

#endif
