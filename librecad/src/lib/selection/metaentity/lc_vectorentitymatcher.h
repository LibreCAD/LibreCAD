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

#ifndef LC_VECTORENTITYMATCHER_H
#define LC_VECTORENTITYMATCHER_H

#include "lc_entitymatcher.h"

template <typename EntityType>
class LC_RS_VectorEntityMatcher : public LC_ConvertingEntityMatcher<RS_Vector, double, double, EntityType> {
public:
    using FunValueAccessor = std::function<RS_Vector(EntityType*)>;

    LC_RS_VectorEntityMatcher(const LC_TypedPropertyMatchTypeDescriptor<double>* propertyType, FunValueAccessor& valueAccessor)
        : LC_ConvertingEntityMatcher<RS_Vector, double, double, EntityType>(propertyType, valueAccessor) {
    }

    void updateWithMapper(double& valueToMatch, double& tolerance, LC_PropertyMatchOperation operation, LC_CoordinatesMapper* mapper) override {
        this->setupComparatorOperation(operation);
        // just to avoid additional check on selection - use different functions for matchAll and specific comparators
        if (operation == MATCH_OPERATION_ALL) {
            this->m_funMath = []([[maybe_unused]]RS_Entity* e)-> bool {
                return true;
            };
        }
        else {
            auto matchType = this->propertyType->getType();
            auto coordX = matchType == LC_PropertyMatchTypeEnum::ENTITY_PROPERTY_COORD_X;
            this->m_funMath = [this, mapper, coordX, valueToMatch, tolerance](RS_Entity* e) -> bool {
                auto* ent = static_cast<EntityType*>(e);
                const RS_Vector wcsCoord = this->m_valueAccessor(ent);
                const RS_Vector ucsCoord = mapper->toUCS(wcsCoord);
                double entityPropertyValue = coordX ? ucsCoord.getX() : ucsCoord.getY();
                return this->m_valueComparator(entityPropertyValue, valueToMatch, tolerance);
            };
        }
    }
};

template <typename EntityType>
class LC_RS_VectorPropertyMatchDescriptor: public LC_GenericPropertyMatchDescriptor<RS_Vector, double, double, EntityType>{
public:
    LC_RS_VectorPropertyMatchDescriptor(const QString& name, const QString& displayName, const QString& description, const LC_TypedPropertyMatchTypeDescriptor<double>& propertyDescriptor,
        std::function<RS_Vector(EntityType*)> funAccess)
        : LC_GenericPropertyMatchDescriptor<RS_Vector, double,  double, EntityType>(name, displayName, description, propertyDescriptor, funAccess) {}

    LC_EntityMatcher* createMatcher() override;
};

template <typename EntityType>
LC_EntityMatcher* LC_RS_VectorPropertyMatchDescriptor<EntityType>::createMatcher() {
    auto* propertyType = (LC_TypedPropertyMatchTypeDescriptor<double>*)(this->m_type);
    auto* matcher = new LC_RS_VectorEntityMatcher<EntityType>(propertyType, this->m_funAccess);
    return matcher;
}

#endif
