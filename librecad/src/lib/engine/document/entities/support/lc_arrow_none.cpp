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

#include "lc_arrow_none.h"
#include "rs_painter.h"

LC_ArrowNone::LC_ArrowNone(RS_EntityContainer* container, const RS_Vector& point, double dirAngle, double size)
    :LC_DimArrowPoly(container, point, dirAngle, size){
    createVertexes(size);
}

RS_Entity* LC_ArrowNone::clone() const {
    return new LC_ArrowNone(getParent(), getPosition(), getAngle(), getArrowSize());
}

void LC_ArrowNone::draw(RS_Painter* painter) {
    painter->drawLineWCS(getDimLinePoint(), getPosition());
}

void LC_ArrowNone::createVertexes(double size) {
    initVertexes(2);

    setVertex(0, {-size,0}); // dimline end point
    setVertex(1,{0,0}); // center point

    setDimLinePoint({-size,0});
    positionFromZero();
    calculateBorders();
}
