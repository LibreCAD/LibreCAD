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

#ifndef LC_ARROW_DOT_H
#define LC_ARROW_DOT_H
#include "lc_dimarrowblockpoly.h"

class LC_ArrowDot: public LC_DimArrowPoly{
public:
    enum DotArrowSubtype {
        small, blank
    };
    LC_ArrowDot(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size, DotArrowSubtype subType);
    RS_Entity* clone() const override;
    void draw(RS_Painter* painter) override;
    DotArrowSubtype getSubType() const {return m_subType;}
protected:
    void createVertexes(double size);
private:
    DotArrowSubtype m_subType;
};

#endif // LC_ARROW_DOT_H
