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

#include "lc_arrow_integral.h"

#include "rs_painter.h"

LC_ArrowIntegral::LC_ArrowIntegral(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size)
    :LC_DimArrowPoly(container, point, dirAngle, size){
    createVertexes(size);
}

RS_Entity* LC_ArrowIntegral::clone() const {
    return new LC_ArrowIntegral(getParent(), getPosition(), getAngle(), getArrowSize());
}

void LC_ArrowIntegral::draw(RS_Painter* painter) {
    painter->drawLineWCS(getPosition(), getDimLinePoint());
    painter->drawPolygonWCS(getVertexes());
}

void LC_ArrowIntegral::createVertexes(double size) {
    initVertexes(7);

    double halfSize = size*0.5;

    // right leg
    setVertex(0,{halfSize,halfSize}); // rightmost topmost
    setVertex(1,{halfSize*0.5,halfSize*0.8659});
    setVertex(2,{halfSize*0.134 ,halfSize*0.4992});

    // center point
    setVertex(3,{0,0});

    // left Leg
    setVertex(4,{-halfSize*0.134 ,-halfSize*0.4992});
    setVertex(5,{-halfSize*0.5,-halfSize*0.8659});
    setVertex(6,{-halfSize,-halfSize});

    setDimLinePoint({-size,0});

    positionFromZero();
    calculateBorders();
}
