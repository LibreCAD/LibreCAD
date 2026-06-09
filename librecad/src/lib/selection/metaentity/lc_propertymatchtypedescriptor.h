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

#ifndef LC_PROPERTYMATCHTYPEDESCRIPTOR_H
#define LC_PROPERTYMATCHTYPEDESCRIPTOR_H

#include <QString>
#include <functional>

class RS_Entity;

enum LC_PropertyMatchOperation {
    MATCH_OPERATION_EQUALS        = 1,
    MATCH_OPERATION_NOT_EQUALS    = 2,
    MATCH_OPERATION_GREATER       = 4,
    MATCH_OPERATION_LESS          = 8,
    MATCH_OPERATION_PATTERN_MATCH = 16,
    MATCH_OPERATION_ALL           = 32
};

Q_DECLARE_FLAGS(LC_PropertyComparingOperations, LC_PropertyMatchOperation);


enum LC_PropertyMatchTypeEnum {
    ENTITY_PROPERTY_INT,
    ENTITY_PROPERTY_INT_CHOICE,
    ENTITY_PROPERTY_BOOL,
    ENTITY_PROPERTY_COORD_X,
    ENTITY_PROPERTY_COORD_X_CONTAINS,
    ENTITY_PROPERTY_COORD_Y,
    ENTITY_PROPERTY_COORD_Y_CONTAINS,
    ENTITY_PROPERTY_DOUBLE,
    ENTITY_PROPERTY_LENGTH,
    ENTITY_PROPERTY_ANGLE,
    ENTITY_PROPERTY_LINETYPE,
    ENTITY_PROPERTY_LINETYPE_RESOLVED,
    ENTITY_PROPERTY_LINEWIDTH,
    ENTITY_PROPERTY_LINEWIDTH_RESOLVED,
    ENTITY_PROPERTY_COLOR,
    ENTITY_PROPERTY_COLOR_RESOLVED,
    ENTITY_PROPERTY_LAYER,
    ENTITY_PROPERTY_DIM_STYLE,
    ENTITY_PROPERTY_STRING,
    ENTITY_PROPERTY_STRING_CHOICE

    // todo - arrow type as type?
};

struct LC_PropertyMatchTypeDescriptor {
    explicit LC_PropertyMatchTypeDescriptor(const LC_PropertyMatchTypeEnum type)
        : type(type) {
    }
    LC_PropertyMatchTypeEnum getType() const {return type;}

    void hasAll() {
        supportedOperations.setFlag(MATCH_OPERATION_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_NOT_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_GREATER);
        supportedOperations.setFlag(MATCH_OPERATION_LESS);
        supportedOperations.setFlag(MATCH_OPERATION_PATTERN_MATCH);
        supportedOperations.setFlag(MATCH_OPERATION_ALL);
    }

    void hasAllExceptPattern() {
        supportedOperations.setFlag(MATCH_OPERATION_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_NOT_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_GREATER);
        supportedOperations.setFlag(MATCH_OPERATION_LESS);
        supportedOperations.setFlag(MATCH_OPERATION_PATTERN_MATCH, false);
        supportedOperations.setFlag(MATCH_OPERATION_ALL);
    }

    void hasBasic() {
        supportedOperations.setFlag(MATCH_OPERATION_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_NOT_EQUALS);
        supportedOperations.setFlag(MATCH_OPERATION_GREATER, false);
        supportedOperations.setFlag(MATCH_OPERATION_LESS, false);
        supportedOperations.setFlag(MATCH_OPERATION_PATTERN_MATCH, false);
        supportedOperations.setFlag(MATCH_OPERATION_ALL);
    }

    LC_PropertyComparingOperations supportedOperations;
    LC_PropertyMatchTypeEnum type;
};

template <typename PropertyType, typename MatchValueType>
class LC_ComparingPropertyMatchTypeDescriptor: public LC_PropertyMatchTypeDescriptor{
public:
    using FunValueComparisonOperation = std::function<bool(PropertyType&,  const MatchValueType&,  const MatchValueType&)>;

    explicit LC_ComparingPropertyMatchTypeDescriptor(const LC_PropertyMatchTypeEnum type)
        : LC_PropertyMatchTypeDescriptor(type) {
    }

    LC_ComparingPropertyMatchTypeDescriptor() = default;

    FunValueComparisonOperation funEquals;
    FunValueComparisonOperation funNotEquals;
    FunValueComparisonOperation funGreater;
    FunValueComparisonOperation funLess;
    FunValueComparisonOperation funAny;
    FunValueComparisonOperation funPatternMatch;
};

template <typename PropertyType>
class LC_TypedPropertyMatchTypeDescriptor: public LC_ComparingPropertyMatchTypeDescriptor<PropertyType, PropertyType>{
public:
    explicit LC_TypedPropertyMatchTypeDescriptor(LC_PropertyMatchTypeEnum type)
        : LC_ComparingPropertyMatchTypeDescriptor<PropertyType, PropertyType>(type) {
    }
    LC_TypedPropertyMatchTypeDescriptor() = default;
};
#endif
