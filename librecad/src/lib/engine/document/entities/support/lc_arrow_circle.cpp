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

#include "lc_arrow_circle.h"

#include "rs_painter.h"

LC_ArrowCircle::LC_ArrowCircle(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size, CircleArrowSubtype subType):
   LC_DimArrowPoly(container, point, dirAngle, size),
   m_subType{subType}{
   createVertexes(size);
}

RS_Entity* LC_ArrowCircle::clone() const {
    return new LC_ArrowCircle(getParent(), getPosition(), getAngle(), getArrowSize(), m_subType);
}

void LC_ArrowCircle::draw(RS_Painter* painter) {
    double arrowSize = getArrowSize();

    painter->drawCircleWCS(vertexAt(2), arrowSize*0.5);

    switch (m_subType) {
        case (dot): {
            painter->drawFilledCircleWCS(vertexAt(2), arrowSize*0.5);
            painter->drawLineWCS(vertexAt(0), vertexAt(1));
            break;
        }
        case dot_blank: {
            painter->drawLineWCS(vertexAt(0), vertexAt(1));
            break;
        }
        case origin_indicator: {
            // do nothing
            painter->drawLineWCS(vertexAt(0), vertexAt(2));
            break;
        }
        case origin_indicator2:{
            painter->drawLineWCS(vertexAt(0), vertexAt(1));
            painter->drawCircleWCS(vertexAt(2), arrowSize*0.25);
            break;
        }
    }
}

void LC_ArrowCircle::createVertexes(double size) {
    initVertexes(6);

    double halfSize = size / 2.0;

    setVertex(0, {-size,0}); // dimline end
    setVertex(1, {-halfSize,0}); // connection line
    setVertex(2, {0,0}); // center
    // just boundary points used for nearest point only
    setVertex(3, {0,halfSize});
    setVertex(4, {0,-halfSize});
    setVertex(5, {halfSize,0});

    positionFromZero();
    calculateBorders();
}
