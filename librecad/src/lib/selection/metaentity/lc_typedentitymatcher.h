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


#ifndef LC_TYPEDENTITYMATCHER_H
#define LC_TYPEDENTITYMATCHER_H

#include "lc_entitymatcher.h"

template <typename MatchValueType, typename EntityType>
class LC_TypedEntityMatcher : public LC_ConvertingEntityMatcher<MatchValueType, MatchValueType, MatchValueType, EntityType> {
public:
    using FunValueAccessor = typename  std::function<MatchValueType(EntityType*)>;

    LC_TypedEntityMatcher(const LC_TypedPropertyMatchTypeDescriptor<MatchValueType>* propertyType, FunValueAccessor valueAccessor)
        : LC_ConvertingEntityMatcher<MatchValueType, MatchValueType, MatchValueType,EntityType>(propertyType, valueAccessor) {
    }

    void update(MatchValueType& valueToMatch, [[maybe_unused]] MatchValueType& tolerance, LC_PropertyMatchOperation operation) override {
        this->setupComparatorOperation(operation);

        // just to avoid additional check on selection - use different functions for matchAll and specific comparators
        if (operation == MATCH_OPERATION_ALL) {
            this->m_funMath = []([[maybe_unused]]RS_Entity* e)-> bool {
                return true;
            };
        }
        else {
            this->m_funMath = [this, valueToMatch, tolerance](RS_Entity* e)-> bool {
                EntityType* ent = static_cast<EntityType*>(e);
                MatchValueType entityPropertyValue = this->m_valueAccessor(ent);
                auto result = this->m_valueComparator(entityPropertyValue, valueToMatch, tolerance);
                return result;
            };
        }
    }
};

template <typename MatchValueType, typename EntityType>
class LC_TypedPropertyMatchDescriptor: public LC_GenericPropertyMatchDescriptor<MatchValueType, MatchValueType, MatchValueType, EntityType> {
public:
    using FunValueAccessor = std::function<MatchValueType(EntityType*)>;

    LC_TypedPropertyMatchDescriptor(const QString& name, const QString& displayName, const QString& description,
                                    const LC_TypedPropertyMatchTypeDescriptor<MatchValueType>& propertyDescriptor, FunValueAccessor funAccess)
        : LC_GenericPropertyMatchDescriptor<MatchValueType, MatchValueType, MatchValueType, EntityType>(
            name, displayName, description, propertyDescriptor, funAccess) {
    }

    ~LC_TypedPropertyMatchDescriptor() override = default;
    LC_EntityMatcher* createMatcher() override;
};

template <typename MatchValueType, typename EntityType>
class LC_DynamicChoicePropertyMatchDescriptor : public LC_TypedPropertyMatchDescriptor<MatchValueType, EntityType> {
public:
    using FunValueAccessor = std::function<MatchValueType(EntityType*)>;
    LC_DynamicChoicePropertyMatchDescriptor(const QString &name, const QString& displayName, const QString &description,
         const LC_TypedPropertyMatchTypeDescriptor<MatchValueType>& propertyDescriptor, FunValueAccessor funAccess)
    : LC_TypedPropertyMatchDescriptor<MatchValueType, EntityType>(name, displayName, description, propertyDescriptor, funAccess) {}

    void getChoiceValues(QList<QPair<QString, QVariant>>&values)  override {
        m_funListProvider(values);
    }

    void setFunListProvider(const std::function<void(QList<QPair<QString, QVariant>>&)>& funListValuesProvider) {
        this->m_funListProvider = funListValuesProvider;
        this->m_choice = true;
    }

protected:
    std::function<void(QList<QPair<QString, QVariant>>&)> m_funListProvider;
};

template <typename MatchValueType, typename EntityType>
LC_EntityMatcher* LC_TypedPropertyMatchDescriptor<MatchValueType, EntityType>::createMatcher() {
    LC_TypedPropertyMatchTypeDescriptor<MatchValueType>* propertyType = (LC_TypedPropertyMatchTypeDescriptor<MatchValueType>*)(this->m_type);
    auto matcher = new LC_TypedEntityMatcher<MatchValueType, EntityType>(propertyType,this->m_funAccess);
    return matcher;
}

#endif
