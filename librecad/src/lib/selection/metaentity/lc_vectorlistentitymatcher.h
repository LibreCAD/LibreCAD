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

#ifndef LC_VERCTORLISTENTITYMATCHER_H
#define LC_VERCTORLISTENTITYMATCHER_H

#include <QList>

#include "lc_entitymatcher.h"

template <typename EntityType, typename VectorsListType>
class LC_RS_VectorListEntityMatcher : public LC_ConvertingEntityMatcher<VectorsListType, QList<double>, double, EntityType> {
public:
    using FunValueAccessor = std::function<VectorsListType(EntityType*)>;

    LC_RS_VectorListEntityMatcher(const LC_ComparingPropertyMatchTypeDescriptor<QList<double>, double>* propertyType, FunValueAccessor& valueAccessor)
        : LC_ConvertingEntityMatcher<VectorsListType, QList<double>,double, EntityType>(propertyType, valueAccessor) {
    }

    void updateWithMapper(double& valueToMatch, [[maybe_unused]] double& tolerance, LC_PropertyMatchOperation operation, LC_CoordinatesMapper* mapper) override {
        this->setupComparatorOperation(operation);
        // just to avoid additional check on selection - use different functions for matchAll and specific comparators
        if (operation == MATCH_OPERATION_ALL) {
            this->m_funMath = []([[maybe_unused]]RS_Entity* e)-> bool {
                return true;
            };
        }
        else {
            auto type = this->propertyType->getType();
            auto coordX = type == LC_PropertyMatchTypeEnum::ENTITY_PROPERTY_COORD_X;
            this->m_funMath = [this, mapper, coordX, valueToMatch, tolerance](RS_Entity* e) -> bool {
                auto* ent = static_cast<EntityType*>(e);
                VectorsListType wcsCoordinates = this->m_valueAccessor(ent);
                QList<double> ucsCoords;

                for (const auto wcs : wcsCoordinates) {
                    RS_Vector ucsCoord = mapper->toUCS(wcs);
                    const double entityPropertyValue = coordX ? ucsCoord.getX() : ucsCoord.getY();
                    ucsCoords.push_back(entityPropertyValue);
                }

                return this->m_valueComparator(ucsCoords, valueToMatch, tolerance);
            };
        }
    }
};

template <typename EntityType, typename VectorListType>
class LC_RS_VectorListPropertyMatchDescriptor: public LC_GenericPropertyMatchDescriptor<VectorListType, QList<double>,double, EntityType>{
public:
    LC_RS_VectorListPropertyMatchDescriptor(const QString& name, const QString& displayName, const QString& description,
                                            const LC_ComparingPropertyMatchTypeDescriptor<QList<double>, double>& propertyDescriptor,
                                            std::function<VectorListType(EntityType*)> funAccess)
        : LC_GenericPropertyMatchDescriptor<VectorListType, QList<double>, double, EntityType>(
            name, displayName, description, propertyDescriptor, funAccess) {
    }

    LC_EntityMatcher* createMatcher() override;
};

template <typename EntityType, typename VectorListType>
LC_EntityMatcher* LC_RS_VectorListPropertyMatchDescriptor<EntityType, VectorListType>::createMatcher() {
    auto matcher = new LC_RS_VectorListEntityMatcher<EntityType,VectorListType>(this->m_type, this->m_funAccess);
    return matcher;
}


#endif
