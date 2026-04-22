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

#ifndef LC_PROPERTYVIEWFACTORY_H
#define LC_PROPERTYVIEWFACTORY_H

#include <functional>

#include "lc_property_view.h"

class LC_PropertyViewFactory {
    Q_DISABLE_COPY(LC_PropertyViewFactory)

public:
    using FunCreateViewForProperty = std::function<LC_PropertyView * (LC_Property&)>;
    LC_PropertyViewFactory();

    LC_PropertyView* createView(LC_Property& property);
    void registerView(const QMetaObject* propertyMetaObject, const FunCreateViewForProperty& createFunction, const QByteArray& viewName);
    void registerViewDefault(const QMetaObject* propertyMetaObject, const FunCreateViewForProperty& createFunction,
                             const QByteArray& viewName = QByteArray());

    bool unregisterView(const QMetaObject* propertyMetaObject);
    bool unregisterView(const QMetaObject* propertyMetaObject, const QByteArray& viewName);

    static LC_PropertyViewFactory* staticInstance();

private:
    LC_PropertyView* createViewInternal(LC_Property& property);

    struct ViewFactoryInfo {
        FunCreateViewForProperty defaultCreatePropertyViewViewFunc;
        QMap<QByteArray, FunCreateViewForProperty> createFunctions;
    };

    QMap<const QMetaObject*, ViewFactoryInfo> m_createItems;
};


template <typename PropertyViewClass, typename PropertyClass>
LC_PropertyView* createViewForProperty(LC_Property& property) {
    auto theOwner = qobject_cast<PropertyClass*>(&property);
    if (theOwner == nullptr) {
        return nullptr;
    }

    return new PropertyViewClass(*theOwner);
}

#endif
