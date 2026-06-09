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

#ifndef LC_ENTITYPROPERTYMETATYPES_H
#define LC_ENTITYPROPERTYMETATYPES_H

#include "lc_propertymatchtypedescriptor.h"
#include "rs.h"
#include "rs_color.h"

class RS_Layer;
class LC_DimStyle;
class RS_Color;

struct LC_PropertyMatcherTypes {
    using TCOLOR = LC_TypedPropertyMatchTypeDescriptor<RS_Color>;
    using TLAYER = LC_TypedPropertyMatchTypeDescriptor<RS_Layer*>;
    using TLINE_WIDTH = LC_TypedPropertyMatchTypeDescriptor<RS2::LineWidth>;
    using TLINE_TYPE = LC_TypedPropertyMatchTypeDescriptor<RS2::LineType>;
    using TDOUBLE = LC_TypedPropertyMatchTypeDescriptor<double>;
    using TDOUBLE_QLIST = LC_ComparingPropertyMatchTypeDescriptor<QList<double>, double>;
    using TDOUBLE_IN_VECT =  LC_ComparingPropertyMatchTypeDescriptor<std::vector<double>, double>;
    using TINT =   LC_TypedPropertyMatchTypeDescriptor<int>;
    using TBOOL =  LC_TypedPropertyMatchTypeDescriptor<bool>;
    using TSTRING =   LC_TypedPropertyMatchTypeDescriptor<QString>;
    using TDIMSTYLE =   LC_TypedPropertyMatchTypeDescriptor<LC_DimStyle*>;

    static const TCOLOR COLOR;
    static const TCOLOR COLOR_RESOLVED;
    static const TLAYER LAYER;
    static const TLINE_WIDTH LINE_WIDTH;
    static const TLINE_WIDTH LINE_WIDTH_RESOLVED;
    static const TLINE_TYPE LINE_TYPE;
    static const TLINE_TYPE LINE_TYPE_RESOLVED;
    static const TDOUBLE COORD_X;
    static const TDOUBLE_QLIST COORD_X_IN_QLIST;
    static const TDOUBLE  COORD_Y;
    static const TDOUBLE_QLIST  COORD_Y_IN_QLIST;
    static const TDOUBLE DOUBLE;
    static const TDOUBLE LENGTH;
    static const TDOUBLE ANGLE;
    static const TDOUBLE INCLINATION;
    static const TINT INT;
    static const TINT INT_CHOICE;
    static const TBOOL BOOL;
    static const TSTRING STRING;
    static const TSTRING STRING_CHOICE;
    static const TDIMSTYLE DIM_STYLE;
    static const TDOUBLE_IN_VECT COORD_X_IN_VECTOR;
    static const TDOUBLE_IN_VECT COORD_Y_IN_VECTOR;
};

#endif
