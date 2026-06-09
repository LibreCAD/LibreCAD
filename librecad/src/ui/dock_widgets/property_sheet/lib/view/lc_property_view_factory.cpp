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

#include "lc_property_view_factory.h"

#include "lc_property_atomic.h"
#include "lc_property_view_error.h"
#include "rs_debug.h"

LC_PropertyViewFactory::LC_PropertyViewFactory(){
}

LC_PropertyView* LC_PropertyViewFactory::createView(LC_Property* property) {
    const auto result = createViewInternal(property);
    if (result != nullptr) {
        result->setFactory(this);
        result->init();
        return result;
    }

    const auto desc = property->getViewDescriptor();
    QByteArray viewName;

    if (desc != nullptr) {
        viewName = desc->viewName;
    }

    if (viewName.isEmpty()) {
        LC_ERR << "Cannot find default view for property" << property->getName();
        LC_ERR << "Did you forget to register view for " << property->metaObject()->className() << "type?";
    }
    else {
        LC_ERR << "Cannot find view with name" << viewName << "for property" << property->getName();
        LC_ERR << "Did you forget to register" << viewName << "view for" << property->metaObject()->className() << "type?";
    }

    return LC_PropertyViewError::createErrorView(property, QString("View <%1> unknown").arg(QString::fromLatin1(viewName)));
}

LC_PropertyView* LC_PropertyViewFactory::createViewInternal(LC_Property* property) {
    const QMetaObject* metaObject = property->metaObject();
    if (property->asAtomic() != nullptr && property->asContainer() != nullptr) {
        return createViewInternal(property->asAtomic());
    }

    FunCreateViewForProperty createFunction = nullptr;
    while (metaObject != nullptr && !createFunction) {
        // try to find view factory by class name
        auto it = m_createItems.find(metaObject);

        if (it != m_createItems.end()) {
            // try to find view factory by name
            const ViewFactoryInfo& createItem = it.value();
            const auto desc = property->getViewDescriptor();
            QByteArray viewName;

            if (desc != nullptr) {
                viewName = desc->viewName;
            }

            if (viewName.isEmpty()) {
                createFunction = createItem.defaultCreatePropertyViewViewFunc;
            }
            else {
                auto jt = createItem.createFunctions.find(viewName);
                if (jt != createItem.createFunctions.end()) {
                    createFunction = jt.value();
                }
            }
        }
        metaObject = metaObject->superClass();
    }
    if (createFunction == nullptr) {
        return nullptr;
    }
    return createFunction(property);
}

void LC_PropertyViewFactory::registerView(const QMetaObject* propertyMetaObject, const FunCreateViewForProperty& createFunction,
                                          const QByteArray& viewName) {
    Q_ASSERT(propertyMetaObject);
    Q_ASSERT(createFunction);
    Q_ASSERT(!viewName.isEmpty());

    ViewFactoryInfo& createItem = m_createItems[propertyMetaObject];
    createItem.createFunctions[viewName] = createFunction;
}

void LC_PropertyViewFactory::registerViewDefault(const QMetaObject* propertyMetaObject, const FunCreateViewForProperty& createFunction,
                                                 const QByteArray& viewName) {
    Q_ASSERT(propertyMetaObject);
    Q_ASSERT(createFunction);

    ViewFactoryInfo& createItem = m_createItems[propertyMetaObject];
    createItem.defaultCreatePropertyViewViewFunc = createFunction;

    if (!viewName.isEmpty()) {
        registerView(propertyMetaObject, createFunction, viewName);
    }
}

bool LC_PropertyViewFactory::unregisterView(const QMetaObject* propertyMetaObject) {
    Q_ASSERT(propertyMetaObject);
    const auto it = m_createItems.find(propertyMetaObject);
    if (it == m_createItems.end()) {
        return false;
    }
    m_createItems.erase(it);
    return true;
}

bool LC_PropertyViewFactory::unregisterView(const QMetaObject* propertyMetaObject, const QByteArray& viewName) {
    Q_ASSERT(propertyMetaObject);
    Q_ASSERT(!viewName.isEmpty());

    const auto it = m_createItems.find(propertyMetaObject);
    if (it == m_createItems.end()) {
        return false;
    }

    auto& createFunctions = it->createFunctions;
    const auto it2 = createFunctions.find(viewName);
    if (it2 == createFunctions.end()) {
        return false;
    }

    createFunctions.erase(it2);
    return true;
}

static std::unique_ptr<LC_PropertyViewFactory> m_factoryInstance;

LC_PropertyViewFactory* LC_PropertyViewFactory::staticInstance() {
    if (m_factoryInstance == nullptr) {
        m_factoryInstance.reset(new LC_PropertyViewFactory);
    }
    return m_factoryInstance.get();
}
