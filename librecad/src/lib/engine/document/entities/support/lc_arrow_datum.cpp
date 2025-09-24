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

#include "lc_arrow_datum.h"
#include "rs_painter.h"

LC_ArrowDatum::LC_ArrowDatum(RS_EntityContainer* container, const RS_Vector& pos, double dirAngle, double size,
                             bool filled):LC_DimArrowPoly(container, pos, dirAngle, size), m_filled{filled} {
    createVertexes(size);
}

RS_Entity* LC_ArrowDatum::clone() const {
    return new LC_ArrowDatum(getParent(), getPosition(), getAngle(), getArrowSize(), m_filled);
}

void LC_ArrowDatum::draw(RS_Painter* painter) {
    if (m_filled) {
        painter->drawFilledPolygonWCS(vertexAt(0), vertexAt(1), vertexAt(2), vertexAt(0),RS_Vector(false));
    }
    else {
        painter->drawPolygonWCS(vertexAt(0), vertexAt(1), vertexAt(2), vertexAt(0),RS_Vector(false));
    }
}

void LC_ArrowDatum::createVertexes(double size) {
    initVertexes(4);

    double halfSize = size * 0.5;

    setVertex(0,{0, halfSize});
    setVertex(1, {0,-halfSize});
    setVertex(2, {-size, 0});

    positionFromZero();
    calculateBorders();
}
