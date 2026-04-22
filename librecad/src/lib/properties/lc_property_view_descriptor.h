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

#ifndef LC_PROPERTYVIEWDESCRIPTOR_H
#define LC_PROPERTYVIEWDESCRIPTOR_H

#include <QVariant>

// fixme - sand - no copy assignment operator!
struct LC_PropertyViewDescriptor {
    QByteArray viewName;
    using Attributes = QMap<QByteArray, QVariant>;
    Attributes attributes;

    LC_PropertyViewDescriptor() = default;
    LC_PropertyViewDescriptor(const LC_PropertyViewDescriptor& other);
    explicit LC_PropertyViewDescriptor(const QByteArray& name, const Attributes& attributes = Attributes());
    LC_PropertyViewDescriptor(const Attributes& attributes);

    QVariant& operator[](const QByteArray& idx) {
        return attributes[idx];
    }

    QVariant operator[](const QByteArray& idx) const {
        return attributes[idx];
    }

    template <typename T>
    T attr(const QByteArray& attrName, const T& defaultValue = T()) const {
        const auto it = attributes.find(attrName);
        if (it == attributes.end()) {
            return defaultValue;
        }
        return it.value().value<T>();
    }

    template <typename T>
    bool load(const QByteArray& attrName, T& destination) const {
        const auto it = attributes.find(attrName);
        if (it == attributes.end()) {
            return false;
        }
        destination = it.value().value<T>();
        return true;
    }

    template <typename OBJ_T, typename ATTR_T_RET, typename ATTR_T_ARG>
    void store(const QByteArray& attrName, OBJ_T* to, ATTR_T_RET (OBJ_T::*get)() const, void (OBJ_T::*set)(ATTR_T_ARG)) const {
        Q_ASSERT(to);
        (to->*set)(attr(attrName, (to->*get)()));
    }
};

#endif
