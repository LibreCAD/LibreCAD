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

#ifndef LC_METAINFOPROVIDER_H
#define LC_METAINFOPROVIDER_H

#include "lc_propertymatchertypes.h"
#include "lc_propertymatchtypedescriptor.h"
#include "lc_typedentitymatcher.h"
#include "lc_vectorentitymatcher.h"
#include "lc_vectorlistentitymatcher.h"
#include "rs.h"
#include "rs_font.h"
#include "rs_fontlist.h"

class RS_Entity;

class LC_EntityMatchDescriptor {
public:
    LC_EntityMatchDescriptor(const QString &name, const RS2::EntityType entityType)
       : m_name{name}, m_entityType{entityType} {
    }
    void collectPropertiesInfo(QList<QPair<QString, QString>> &propertyNames);
    LC_PropertyMatchDescriptor* findPropertyDescriptor(const QString& propertyName);
    RS2::EntityType getEntityType() const {return m_entityType;}
    void fillPropertiesInfo(const std::function<void(QString&, QString&, QString&)>& fillFunction);
protected:
    QString m_name;
    RS2::EntityType m_entityType;
    QList<LC_PropertyMatchDescriptor*> m_entityPropertyDescriptors;
};

inline void LC_EntityMatchDescriptor::fillPropertiesInfo(const std::function<void(QString &, QString&, QString&)>& fillFunction){
    for (const auto p: std::as_const(m_entityPropertyDescriptors)) {
        QString name = p->getName();
        QString displayName = p->getDisplayName();
        QString description = p->getDescription();
        fillFunction(name, displayName, description);
    }
}

inline LC_PropertyMatchDescriptor* LC_EntityMatchDescriptor::findPropertyDescriptor(const QString& propertyName) {
    for (const auto p: std::as_const(m_entityPropertyDescriptors)) {
        if (p->getName() == propertyName) {
            return p;
        }
    }
    return nullptr;
}

template <typename EntityType> class LC_TypedEntityMatchDescriptor: public LC_EntityMatchDescriptor {
public:
    LC_TypedEntityMatchDescriptor(const QString& name, const RS2::EntityType entityType):LC_EntityMatchDescriptor(name, entityType) {}
    template <typename ValueType>
    void add(const QString& name, std::function<ValueType(EntityType*)> funAccess, const QString& displayName, const QString& description,
             const LC_TypedPropertyMatchTypeDescriptor<ValueType>& type);

    template <typename ValueType>
    void add(const QString& name, std::function<ValueType(EntityType*)> funAccess, const QString& displayName, const QString& description,
             const LC_TypedPropertyMatchTypeDescriptor<ValueType>& type, const QList<QPair<QString, QVariant>>& choicesList);

    void addIntChoice(const QString& name, std::function<int(EntityType*)> funAccess, const QString& displayName,
                      const QString& description, const std::initializer_list<QPair<QString, int>>& choicesList);

    void addStringList(const QString& name, std::function<QString(EntityType*)> funAccess, const QString& displayName,
                       const QString& description, std::function<void(QList<QPair<QString, QVariant>>&)> funChoicesProvider);

    void addVector(const QString& name, std::function<RS_Vector(EntityType*)> funAccess, const QString& displayName,
                   const QString& description, const LC_TypedPropertyMatchTypeDescriptor<double>& type);

    void addVectorX(const QString& name, std::function<RS_Vector(EntityType*)> funAccess, const QString& displayName,
                    const QString& description);
    void addVectorY(const QString& name, std::function<RS_Vector(EntityType*)> funAccess, const QString& displayName,
                    const QString& description);
    void addLength(const QString& name, std::function<double(EntityType*)> funAccess, const QString& displayName,
                   const QString& description);
    void addDouble(const QString& name, std::function<double(EntityType*)> funAccess, const QString& displayName,
                   const QString& description);
    void addAngle(const QString& name, std::function<double(EntityType*)> funAccess, const QString& displayName,
                  const QString& description);
    void addBoolean(const QString& name, std::function<bool(EntityType*)> funAccess, const QString& displayName,
                    const QString& description);
    void addInt(const QString& name, std::function<int(EntityType*)> funAccess, const QString& displayName, const QString& description);

    void addString(const QString& name, std::function<QString(EntityType*)> funAccess, const QString& displayName,
                   const QString& description);

    template <class ListType>
    void addContains(const QString& name, std::function<ListType(EntityType*)> funAccess, const QString& displayName, const QString& description,
                     const LC_ComparingPropertyMatchTypeDescriptor<QList<double>, double>& type);

    template <class ListType>
    void addContainsXInList(const QString& name, std::function<ListType(EntityType*)> funAccess, const QString& displayName,
                            const QString& description);

    template <class ListType>
    void addContainsYInList(const QString& name, std::function<ListType(EntityType*)> funAccess, const QString& displayName,
                            const QString& description);

    void addFontStringList(const QString& name, std::function<QString(EntityType*)> funAccess, const QString& displayName, const QString& description);
};



template <typename EntityType>
template <typename ValueType>
void LC_TypedEntityMatchDescriptor<EntityType>::add(const QString&name,
    std::function<ValueType(EntityType*)> funAccess, const QString& displayName, const QString& description, const LC_TypedPropertyMatchTypeDescriptor<ValueType>& type) {
    auto propertyMatchDescriptor = new LC_TypedPropertyMatchDescriptor<ValueType, EntityType>(name, displayName, description, type, funAccess);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}


template <typename EntityType>
template <typename ValueType>
void LC_TypedEntityMatchDescriptor<EntityType>::add(const QString &name,
    std::function<ValueType(EntityType*)> funAccess, const QString &displayName, const QString& description, const LC_TypedPropertyMatchTypeDescriptor<ValueType>& type,
    const QList<QPair<QString, QVariant>> & choicesList) {
    auto propertyMatchDescriptor = new LC_TypedPropertyMatchDescriptor<ValueType, EntityType>(name, displayName, description, type, funAccess);
    propertyMatchDescriptor->setChoiceValues(choicesList);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addIntChoice(const QString &name,
    std::function<int(EntityType*)> funAccess, const QString &displayName, const QString& description,
    const std::initializer_list<QPair<QString, int>> & choicesList) {
    auto propertyMatchDescriptor = new LC_TypedPropertyMatchDescriptor<int, EntityType>(name, displayName, description,  LC_PropertyMatcherTypes::INT_CHOICE, funAccess);
    QList<QPair<QString, QVariant>> choices;
    for (const auto & [fst, snd]: choicesList) {
        choices.push_back(QPair<QString, QVariant>(fst, QVariant(snd)));
    }
    propertyMatchDescriptor->setChoiceValues(choices);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addStringList(const QString &name,
    std::function<QString(EntityType*)> funAccess, const QString &displayName, const QString& description,
    std::function<void(QList<QPair<QString, QVariant>>&)> funChoicesProvider) {

    auto propertyMatchDescriptor = new LC_DynamicChoicePropertyMatchDescriptor<QString, EntityType>(name, displayName, description, LC_PropertyMatcherTypes::STRING_CHOICE, funAccess);
    propertyMatchDescriptor->setFunListProvider(funChoicesProvider);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addFontStringList(const QString& name, std::function<QString(EntityType*)> funAccess,
    const QString& displayName, const QString& description) {
    addStringList(name, funAccess, displayName, description, [](QList<std::pair<QString, QVariant>>& listValues) -> void {
        QStringList fonts;
        for (const auto& f : *RS_FONTLIST) {
            auto fontName = f->getFileName();
            if (fonts.contains(fontName)) {
                continue;
            }
            fonts.append(fontName);
        }
        for (const QString& f : fonts) {
            listValues.push_back(std::pair<QString, QVariant>(f, f));
        }
    });
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addVector(const QString &name,
    std::function<RS_Vector(EntityType*)> funAccess, const QString &displayName, const QString& description,  const LC_TypedPropertyMatchTypeDescriptor<double>& type) {
    auto propertyMatchDescriptor = new LC_RS_VectorPropertyMatchDescriptor<EntityType>(name, displayName, description, type, funAccess);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addVectorX(const QString &name,
    std::function<RS_Vector(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    addVector(name, funAccess, displayName, description, LC_PropertyMatcherTypes::COORD_X);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addVectorY(const QString &name,
    std::function<RS_Vector(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    addVector(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::COORD_Y);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addLength(const QString &name,
    std::function<double(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<double>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::LENGTH);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addDouble(const QString &name,
    std::function<double(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<double>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::DOUBLE);
}


template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addAngle(const QString &name,
    std::function<double(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<double>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::ANGLE);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addBoolean(const QString &name,
    std::function<bool(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<bool>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::BOOL);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addInt(const QString &name,
    std::function<int(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<int>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::INT);
}

template <typename EntityType>
void LC_TypedEntityMatchDescriptor<EntityType>::addString(const QString &name,
    std::function<QString(EntityType*)> funAccess, const QString &displayName, const QString& description) {
    add<QString>(name, funAccess, displayName, description,  LC_PropertyMatcherTypes::STRING);
}



template <typename EntityType>
template <typename ListType>
void LC_TypedEntityMatchDescriptor<EntityType>::addContains(const QString& name, std::function<ListType(EntityType*)> funAccess,
    const QString& displayName, const QString& description, const LC_ComparingPropertyMatchTypeDescriptor<QList<double>,double>& type) {
    auto propertyMatchDescriptor = new LC_RS_VectorListPropertyMatchDescriptor<EntityType, ListType>(name, displayName, description, type, funAccess);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
template <typename ListType>
void LC_TypedEntityMatchDescriptor<EntityType>::addContainsXInList(const QString& name, std::function<ListType(EntityType*)> funAccess,
    const QString& displayName, const QString& description) {
    auto propertyMatchDescriptor = new LC_RS_VectorListPropertyMatchDescriptor<EntityType, ListType>(name, displayName, description, LC_PropertyMatcherTypes::COORD_X_IN_QLIST, funAccess);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}

template <typename EntityType>
template <typename ListType>
void LC_TypedEntityMatchDescriptor<EntityType>::addContainsYInList(const QString& name, std::function<ListType(EntityType*)> funAccess,
    const QString& displayName, const QString& description) {
    auto propertyMatchDescriptor = new LC_RS_VectorListPropertyMatchDescriptor<EntityType, ListType>(name, displayName, description, LC_PropertyMatcherTypes::COORD_Y_IN_QLIST, funAccess);
    m_entityPropertyDescriptors.push_back(propertyMatchDescriptor);
}


#endif
