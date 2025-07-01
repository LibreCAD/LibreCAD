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

#include "lc_arrow_dot.h"

#include "rs_painter.h"

LC_ArrowDot::LC_ArrowDot(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size,
                         DotArrowSubtype subType):
    LC_DimArrowPoly(container, point, dirAngle, size),
    m_subType{subType}{
    createVertexes(size);
}

RS_Entity* LC_ArrowDot::clone() const {
    return new LC_ArrowDot(getParent(), getPosition(), getAngle(), getArrowSize(), m_subType);
}

void LC_ArrowDot::draw(RS_Painter* painter) {
    double arrowSize = getArrowSize();

    switch (m_subType) {
        case (blank): {
            painter->drawCircleWCS(vertexAt(2), arrowSize*0.25);
            break;
        }
        case small: {
            painter->drawFilledCircleWCS(vertexAt(2), arrowSize*0.1);
            break;
        }
    }
    painter->drawLineWCS(vertexAt(0), vertexAt(1));
}

void LC_ArrowDot::createVertexes(double size) {
    initVertexes(6);

    double radius = (m_subType == blank) ? size * 0.25 : size * 0.1;

    setVertex(0, {-size,0}); // dimline end
    setVertex(1, {-radius,0}); // connection line
    setVertex(2, {0,0}); // center
    // just boundary points used for nearest point only
    setVertex(3, {-size + radius+radius,radius});
    setVertex(4, {-size +radius+radius,-radius});
    setVertex(5, {radius,0});

    positionFromZero();
    calculateBorders();
}
