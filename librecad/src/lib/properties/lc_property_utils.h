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

#ifndef LC_PROPERTYCREATIONUTILS_H
#define LC_PROPERTYCREATIONUTILS_H

#include "lc_property_container.h"
#include "lc_property_single.h"

namespace LC_PropertyCreationUtils {
    template <typename T>
    T* createProperty(LC_PropertyContainer* parent, QString name) {
        auto property = new T(parent);
        property->setName(name);
        if (parent != nullptr) {
            parent->addChildProperty(property);
        }
        return property;
    }

    template <typename T>
    T* createPropertySingle(LC_PropertyContainer* parent, const bool delegatedValue = false) {
        auto property = new T(parent, !delegatedValue);
        if (parent != nullptr) {
            parent->addChildProperty(property);
        }
        return property;
    }

    template <typename T>
    T* createProperty(LC_PropertyContainer* parent = nullptr) {
        auto property = new T(parent);
        if (parent != nullptr) {
            parent->addChildProperty(property);
        }
        return property;
    }
}

namespace LC_PropertyContainerUtils {
    void addChildPropertyToContainer(QObject* parent, LC_Property* child, bool moveOwnership);
    void removeChildPropertyFromContainer(QObject* parent, LC_Property* child);
    void gatherPropertiesToMultiSet(LC_PropertyContainer* target, LC_PropertyContainer* source, bool takeOwnership);
}

namespace LC_PropertyFieldUtils {
    template <typename ValueType, typename ClassType, typename SetterType>
    void setFieldValue(ClassType& to, SetterType setter, ValueType value,
                       typename std::enable_if<std::is_member_function_pointer<SetterType>::value>::type* = nullptr) {
        (to.*setter)(value);
    }

    template <typename ValueType, typename ClassType, typename SetterType>
    void setFieldValue(ClassType& to, SetterType field, ValueType value,
                       typename std::enable_if<std::is_member_object_pointer<SetterType>::value>::type* = nullptr) {
        to.*field = value;
    }

    template <typename ValueType, typename ClassType, typename GetterType>
    ValueType getFieldValue(ClassType from, GetterType getter,
                            typename std::enable_if<std::is_member_function_pointer<GetterType>::value>::type* = nullptr) {
        return (from.*getter)();
    }

    template <typename ValueType, typename ClassType, typename GetterType>
    ValueType getFieldValue(const ClassType& from, GetterType field,
                            typename std::enable_if<std::is_member_object_pointer<GetterType>::value>::type* = nullptr) {
        return from.*field;
    }

    inline QByteArray getViewNameAttr() {
        return QByteArrayLiteral("viewName");
    }

    template <typename FieldPropertyType, typename ValueType, typename GetterType, typename SetterType>
    FieldPropertyType* createFieldDelegatedProperty(LC_PropertySingle<ValueType>* property, GetterType getter, SetterType setter,
                                                    const QString& name = QString(), const QString& displayName = QString(),
                                                    const QString& descriptionFormat = QString(),
                                                    const QByteArray& viewName = QByteArray()) {
        using CallbackValueType = typename FieldPropertyType::ValueType;
        using CallbackValueTypeStore = typename FieldPropertyType::ValueTypeStore;
        using ValueTypeStore = typename LC_PropertySingle<ValueType>::ValueTypeStore;

        Q_ASSERT(property);

        auto result = new FieldPropertyType(nullptr, false);

        if (!displayName.isEmpty()) {
            result->setDisplayName(displayName);
        }
        if (!name.isEmpty()) {
            result->setName(name);
        }
        if (!descriptionFormat.isEmpty()) {
            result->setDescription(descriptionFormat.arg(property->getDisplayName()));
        }

        auto delegatedValue = result->getAsDelegateValue();
        Q_ASSERT(delegatedValue != nullptr);

        delegatedValue->setFunValueGet([property, getter]() -> CallbackValueTypeStore {
            return getFieldValue<CallbackValueTypeStore>(property->value(), getter);
        });
        delegatedValue->setFunValueSet([property, setter](CallbackValueType newValue, LC_PropertyChangeReason reason) {
            auto v = property->value();
            setFieldValue(v, setter, newValue);
            property->setValue(v, reason);
        }, property);

        auto funGetAttributes = [property, viewName]() -> LC_PropertyViewDescriptor {
            LC_PropertyViewDescriptor res;
            if (viewName.isEmpty()) {
                auto desc = property->getViewDescriptor();
                if (desc != nullptr) {
                    res.attributes = desc->attributes;
                    const auto it = res.attributes.find(getViewNameAttr());
                    if (it != res.attributes.end()) {
                        res.viewName = it.value().toByteArray();
                    }
                }
            }
            else {
                res.viewName = viewName;
                auto desc = property->getViewDescriptor();
                if (desc != nullptr) {
                    res.attributes = desc->attributes;
                }
            }

            return res;
        };
        result->setViewDescriptorProvider(funGetAttributes);
        return result;
    }
}

#endif
