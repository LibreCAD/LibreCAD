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

#ifndef LC_DIMENSIONSBUILDER_H
#define LC_DIMENSIONSBUILDER_H

#include "lc_dimarrowregistry.h"

class RS_Pen;
class RS_Line;
class LC_DimOrdinate;
class RS_DimRadial;
class RS_Leader;
class LC_DimArc;
class RS_DimAngular;
class RS_DimDiametric;
class RS_DimAligned;
class RS_DimLinear;
class RS_Dimension;
class LC_DimStyle;
class RS_Graphic;

class LC_DimensionsBuilder{
public:
    explicit LC_DimensionsBuilder(RS_Graphic* graphic, RS_Dimension* dimension);
    ~LC_DimensionsBuilder() = default;
    void build();
private:
    void buildLinear(RS_DimLinear* dimLinear);
    void buildAligned(RS_DimAligned* dimLinear);
    void buildDiametric(RS_DimDiametric* dimLinear);
    void buildRadial(RS_DimRadial* dimLinear);
    void buildOrdinate(LC_DimOrdinate* dimLinear);
    RS_Line* addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen& pen);
    RS_Pen getPenForText();
    RS_Pen getPenExtensionLine();
    RS_Pen getPenDimensionLine();
    void buildAngular(RS_DimAngular* dimLinear);
    void buildArc(LC_DimArc* dimLinear);
    void buildLeader(RS_Leader* dimLinear);
    std::unique_ptr<LC_DimArrowRegistry> m_arrowsRegistry;
    RS_Graphic* m_graphic;
    RS_Dimension *m_dimension;
    LC_DimStyle *m_dimStyle;
};

#endif // LC_DIMENSIONSBUILDER_H
