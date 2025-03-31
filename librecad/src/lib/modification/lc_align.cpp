/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_align.h"
#include "rs_entity.h"
#include "rs_pen.h"

RS_Entity* LC_Align::createCloneMovedToOffset(const RS_Entity *e, const RS_Vector &offset, bool updateAttributes) {
    RS_Entity *clone = e->clone();
    clone->move(offset);
    if (updateAttributes){
        clone->setLayer(e->getLayer());
        clone->setPen(e->getPen(false));
    }
    return clone;
}

RS_Entity *LC_Align::createCloneMovedToTarget(const RS_Entity *e, const RS_Vector &targetPoint, bool updateAttributes, int horizontalAlign, int verticalAlign) {
    RS_Vector entityRefPoint = getReferencePoint(e->getMin(), e->getMax(), horizontalAlign, verticalAlign);
    RS_Vector offset = targetPoint - entityRefPoint;
    RS_Entity* clone = createCloneMovedToOffset(e, offset, updateAttributes);
    return clone;
}

RS_Vector LC_Align::getReferencePoint(const RS_Vector &min, const RS_Vector &max, int horizontalAlign, int verticalAlign) {
    double x = 0;
    double y = 0;

    switch (horizontalAlign) {
        case LEFT_TOP:
            x = min.x;
            break;
        case MIDDLE:
            x = (min.x + max.x) / 2;
            break;
        case RIGHT_BOTTOM:
            x = max.x;
            break;
        default:
            break;
    }

    switch (verticalAlign) {
        case LEFT_TOP:
            y = max.y;
            break;
        case MIDDLE:
            y = (min.y + max.y) / 2;
            break;
        case RIGHT_BOTTOM:
            y = min.y;
            break;
        default:
            break;
    }

    return RS_Vector(x, y);
}

void LC_Align::collectSelectionBounds( std::vector<RS_Entity*> selectedEntities, RS_Vector &boxMin, RS_Vector &boxMax) {
    RS_Vector minV;
    RS_Vector maxV;
    for (auto e: selectedEntities) {
        minV = RS_Vector::minimum(e->getMin(), minV);
        maxV = RS_Vector::maximum(e->getMax(), maxV);
    }
    boxMin = minV;
    boxMax = maxV;
}

bool LC_Align::getVerticalRefCoordinate(const RS_Vector &min, const RS_Vector &max, int verticalAlign, double &refCoordinate) {
    bool hasRefPoint = true;
    switch (verticalAlign) {
        case LC_Align::LEFT_TOP: {
            refCoordinate = min.x;
            break;
        }
        case LC_Align::MIDDLE: {
            refCoordinate = (min.x + max.x) / 2.0;
            break;
        }
        case LC_Align::RIGHT_BOTTOM: {
            refCoordinate = max.x;
            break;
        }
        default: {
            hasRefPoint = false;
        }
    }
    return hasRefPoint;
}

bool LC_Align::getHorizontalRefCoordinate(const RS_Vector &min, const RS_Vector &max, int verticalAlign, double &refCoordinate) {
    bool hasRefPoint = true;
    switch (verticalAlign) {
        case LC_Align::LEFT_TOP: {
            refCoordinate = max.y;
            break;
        }
        case LC_Align::MIDDLE: {
            refCoordinate = (min.y + max.y) / 2.0;
            break;
        }
        case LC_Align::RIGHT_BOTTOM: {
            refCoordinate = min.y;
            break;
        }
        default: {
            hasRefPoint = false;
        }
    }
    return hasRefPoint;
}
