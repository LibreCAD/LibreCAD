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

#ifndef LC_PROPERTYMATCHER_H
#define LC_PROPERTYMATCHER_H

#include <QVariant>
#include <functional>

#include "lc_propertymatchtypedescriptor.h"

class LC_EntityMatcher;

class LC_PropertyMatchDescriptor {
public:
    LC_PropertyMatchDescriptor(const QString &name, const QString& displayName, const QString &description)
     :m_name{name}, m_displayName{displayName}, m_description{description} {}
    virtual ~LC_PropertyMatchDescriptor() = default;
    QString getName() {return m_name;}
    QString getDisplayName() {return m_displayName;}
    QString getDescription() {return m_description;}
    virtual bool isSupportedOperation(LC_PropertyMatchOperation type) = 0;
    LC_PropertyMatchTypeEnum getPropertyType() const {return m_propertyType;}
    bool isChoice() const {return m_choice;}
    void setChoiceValues(const QList<QPair<QString, QVariant>>& values) {
        m_choice = true;
        m_choiceValues = values;
    }
    virtual void getChoiceValues(QList<QPair<QString, QVariant>>&values) {return values.append(m_choiceValues);}
    virtual LC_EntityMatcher* createMatcher() = 0;
protected:
    QString m_name;
    QString m_displayName;
    QString m_description;
    bool m_choice = false;
    QList<QPair<QString, QVariant>> m_choiceValues;
    LC_PropertyMatchTypeEnum m_propertyType {LC_PropertyMatchTypeEnum::ENTITY_PROPERTY_DOUBLE};
};

template <typename EntityPropertyValueType, typename ConvertedPropertValueType, typename MatchValueType, typename EntityType>
class LC_GenericPropertyMatchDescriptor: public LC_PropertyMatchDescriptor {
public:
    using FunValueAccessor = std::function<EntityPropertyValueType(EntityType*)>;

    LC_GenericPropertyMatchDescriptor(const QString &name, const QString& displayName, const QString& description,
                                    const LC_ComparingPropertyMatchTypeDescriptor<ConvertedPropertValueType, MatchValueType>& propertyDescriptor,
                                    FunValueAccessor funAccess) : LC_PropertyMatchDescriptor(name, displayName, description),
                                                                                       m_type{&propertyDescriptor}, m_funAccess{funAccess} {
        m_propertyType = m_type->getType();
    }

    bool isSupportedOperation(LC_PropertyMatchOperation type) override {return m_type->supportedOperations.testFlag(type);}

protected:
    const LC_ComparingPropertyMatchTypeDescriptor<ConvertedPropertValueType, MatchValueType>* m_type;
    FunValueAccessor m_funAccess;
};
#endif
