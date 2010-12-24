/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#include "rs_point.h"

#include "rs_graphicview.h"
#include "rs_painter.h"


/**
 * Default constructor.
 */
RS_Point::RS_Point(RS_EntityContainer* parent,
                   const RS_PointData& d)
        :RS_AtomicEntity(parent), data(d) {
    calculateBorders ();
}



void RS_Point::calculateBorders () {
    minV = maxV = data.pos;
}



RS_VectorSolutions RS_Point::getRefPoints() {
	RS_VectorSolutions ret(data.pos);
	return ret;
}



RS_Vector RS_Point::getNearestEndpoint(const RS_Vector& coord, double* dist) {

    if (dist!=NULL) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}



RS_Vector RS_Point::getNearestPointOnEntity(const RS_Vector& coord,
        bool /*onEntity*/, double* dist, RS_Entity** entity) {
    if (dist!=NULL) {
        *dist = data.pos.distanceTo(coord);
    }
    if (entity!=NULL) {
        *entity = this;
    }
    return data.pos;
}



RS_Vector RS_Point::getNearestCenter(const RS_Vector& coord, double* dist) {

    if (dist!=NULL) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}



RS_Vector RS_Point::getNearestMiddle(const RS_Vector& coord,
                                     double* dist) {
    if (dist!=NULL) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}



RS_Vector RS_Point::getNearestDist(double /*distance*/,
                                   const RS_Vector& /*coord*/,
                                   double* dist) {
    if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}



double RS_Point::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity,
                                    RS2::ResolveLevel /*level*/,
									double /*solidDist*/) {
    if (entity!=NULL) {
        *entity = this;
    }
    return data.pos.distanceTo(coord);
}



void RS_Point::moveStartpoint(const RS_Vector& pos) {
	data.pos = pos;
	calculateBorders();
}



void RS_Point::move(RS_Vector offset) {
    data.pos.move(offset);
    calculateBorders();
}



void RS_Point::rotate(RS_Vector center, double angle) {
    data.pos.rotate(center, angle);
    calculateBorders();
}



void RS_Point::scale(RS_Vector center, RS_Vector factor) {
    data.pos.scale(center, factor);
    calculateBorders();
}



void RS_Point::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    data.pos.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}


void RS_Point::draw(RS_Painter* painter,RS_GraphicView* view, double /*patternOffset*/) {
    if (painter==NULL || view==NULL) {
        return;
    }

    painter->drawPoint(view->toGui(getPos()));
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Point& p) {
    os << " Point: " << p.getData() << "\n";
    return os;
}


// EOF
