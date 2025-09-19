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

#include "lc_arrow_box.h"

#include "rs_painter.h"

LC_ArrowBox::LC_ArrowBox(RS_EntityContainer* container, const RS_Vector& pos, double dirAngle, double size, bool filled)
   :LC_DimArrowPoly(container, pos, dirAngle, size), m_filled{filled} {
    createVertexes(size);
}

RS_Entity* LC_ArrowBox::clone() const {
    return new LC_ArrowBox(getParent(), getPosition(), getAngle(), getArrowSize(), m_filled);
}

void LC_ArrowBox::draw(RS_Painter* painter) {
    if (m_filled) {
        painter->drawFilledPolygonWCS(vertexAt(1), vertexAt(2), vertexAt(3), vertexAt(4), vertexAt(1));
    }
    else {
        painter->drawPolygonWCS(vertexAt(1), vertexAt(2), vertexAt(3), vertexAt(4), vertexAt(1));
    }

    painter->drawLineWCS(getDimLinePoint(), getPosition());
}

void LC_ArrowBox::createVertexes(double size) {
    initVertexes(5);
    double halfSize = size * 0.5;

    setVertex(0, {halfSize,0});

    setVertex(1,{-halfSize, halfSize});
    setVertex(2, {-halfSize, -halfSize});
    setVertex(3, {halfSize, -halfSize});
    setVertex(4, {halfSize, halfSize});

    setDimLinePoint({-size,0});

    positionFromZero();
    calculateBorders();
}
