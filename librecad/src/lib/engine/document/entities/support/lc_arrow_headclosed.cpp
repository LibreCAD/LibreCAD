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

#include "lc_arrow_headclosed.h"

#include "rs_painter.h"

LC_ArrowHeadClosed::LC_ArrowHeadClosed(RS_EntityContainer* container, const RS_Vector& point, double dirAngle,
                                       double size, double ownAngle, bool filled):
    LC_DimArrowPoly{container, point, dirAngle, size}, m_filled(filled), m_ownAngle(ownAngle) {
    createVertexes();
}

RS_Entity* LC_ArrowHeadClosed::clone() const {
    return new LC_ArrowHeadClosed(getParent(), getPosition(), getAngle(), getArrowSize(), m_ownAngle, m_filled);
}

void LC_ArrowHeadClosed::draw(RS_Painter* painter) {
    if (m_filled) {
        painter->drawFilledPolygonWCS(vertexAt(0), vertexAt(1), vertexAt(2), vertexAt(0), RS_Vector(false));
    }
    else {
        painter->drawPolygonWCS(vertexAt(0), vertexAt(1), vertexAt(2), vertexAt(0), RS_Vector(false));
        painter->drawLineWCS(vertexAt(1), getPosition());
    }
}

void LC_ArrowHeadClosed::createVertexes() {
    initVertexes(3);

    double cos1 = cos(m_ownAngle);
    double sin1 = sin(m_ownAngle);

    double arrowSize = getArrowSize();

    double arrowSide{arrowSize / cos1};
    double halfArrowHeight = sin1 * arrowSide;

    setVertex(0, -arrowSize, - halfArrowHeight);
    setVertex(1,0, 0);
    setVertex(2, -arrowSize, halfArrowHeight);

    positionFromZero();
    calculateBorders();
}
