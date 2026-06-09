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

#ifndef LC_ENTITYMATCHERBASE_H
#define LC_ENTITYMATCHERBASE_H

#include "lc_coordinates_mapper.h"
#include "lc_propertymatcher.h"
#include "lc_propertymatchtypedescriptor.h"

class LC_EntityMatcher {
public:
    virtual ~LC_EntityMatcher() = default;
    virtual bool isMatch([[maybe_unused]]RS_Entity* entity) {return false;}
};


class LC_EntityMatcherBase: public LC_EntityMatcher {
public:
    ~LC_EntityMatcherBase() override = default;

    using FunMatch = std::function<bool(RS_Entity*)>;

    bool isMatch(RS_Entity* entity) override {
        return m_funMath(entity);
    }

    void updateForOperationAll() {
        m_funMath = []([[maybe_unused]]RS_Entity* e)->bool {
            return true;
        };
    }
protected:
    FunMatch m_funMath;
};

template<typename MatchValueType>
class LC_GenericEntityMatcher : public LC_EntityMatcherBase{
public:
    ~LC_GenericEntityMatcher() override = default;

    virtual void update([[maybe_unused]] MatchValueType& valueToMatch, [[maybe_unused]] MatchValueType& tolerance,
                        [[maybe_unused]] LC_PropertyMatchOperation operation) {
    }

    virtual void updateWithMapper([[maybe_unused]] MatchValueType& valueToMatch, [[maybe_unused]] MatchValueType& tolerance,
                                  [[maybe_unused]] LC_PropertyMatchOperation operation, [[maybe_unused]] LC_CoordinatesMapper* mapper) {
    }
};

template <typename EntityPropertyValueType, typename ConvertedPropertyValueType, typename MatchValueType, typename EntityType>
class LC_ConvertingEntityMatcher: public LC_GenericEntityMatcher<MatchValueType> {
public:
    using PropertyType     =  LC_GenericPropertyMatchDescriptor<MatchValueType, EntityPropertyValueType, ConvertedPropertyValueType, EntityType>;
    using FunValueAccessor = std::function<EntityPropertyValueType(EntityType*)>;
    using FunValueCompareOperation = std::function<bool(ConvertedPropertyValueType&, const MatchValueType&, const MatchValueType&)>;

    LC_ConvertingEntityMatcher(const LC_ComparingPropertyMatchTypeDescriptor<ConvertedPropertyValueType, MatchValueType>* propertyType, const FunValueAccessor& valueAccessor)
        : propertyType{propertyType}, m_valueAccessor{valueAccessor} {
    }

    void setupComparatorOperation(const LC_PropertyMatchOperation operation) {
        switch (operation) {
            case MATCH_OPERATION_EQUALS: {
                m_valueComparator = propertyType->funEquals;
                break;
            }
            case MATCH_OPERATION_NOT_EQUALS: {
                m_valueComparator = propertyType->funNotEquals;
                break;
            }
            case MATCH_OPERATION_GREATER: {
                m_valueComparator = propertyType->funGreater;
                break;
            }
            case MATCH_OPERATION_LESS: {
                m_valueComparator = propertyType->funLess;
                break;
            }
            case MATCH_OPERATION_PATTERN_MATCH: {
                m_valueComparator = propertyType->funPatternMatch;
                break;
            }
            case MATCH_OPERATION_ALL: {
                m_valueComparator = propertyType->funAny;
                break;
            }
        }
    }

    const LC_ComparingPropertyMatchTypeDescriptor<ConvertedPropertyValueType, MatchValueType>* propertyType;
    FunValueAccessor m_valueAccessor;
    FunValueCompareOperation m_valueComparator;
};

#endif
