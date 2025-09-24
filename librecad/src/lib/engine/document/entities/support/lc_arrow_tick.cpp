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

#include "lc_arrow_tick.h"

#include "rs_painter.h"

LC_ArrowTick::LC_ArrowTick(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size,bool arc)
    :LC_DimArrowPoly(container, point, dirAngle, size), m_architectural{arc} {
    createVertexes(size);
}

RS_Entity* LC_ArrowTick::clone() const {
    return new LC_ArrowTick(getParent(), getPosition(), getAngle(), getArrowSize(), m_architectural);
}

void LC_ArrowTick::draw(RS_Painter* painter) {
    painter->drawLineWCS(getDimLinePoint(), vertexAt(1));

    if (m_architectural) {
        painter->drawLineWCSScaled(vertexAt(2), vertexAt(3), 2);
    }
    else {
        painter->drawLineWCS(vertexAt(2), vertexAt(3));
    }
    // fixme - sand - complete - add support of archTick.... decide how the width of the
    // tick should be devined.
}

void LC_ArrowTick::createVertexes(double size) {
    initVertexes(4);
    double halfSize = size * 0.5;

    setVertex(0, {0,0}); // dimline end point
    setVertex(1,{0,0}); // center point
    setVertex(2, {-halfSize, -halfSize}); //corner
    setVertex(3, {halfSize, halfSize}); // corner

    setDimLinePoint({-size,0});

    positionFromZero();
    calculateBorders();
}
