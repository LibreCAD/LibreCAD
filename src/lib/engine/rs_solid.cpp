/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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


#include "rs_solid.h"

#include "rs_graphicview.h"
#include "rs_painter.h"


/**
 * Default constructor.
 */
RS_Solid::RS_Solid(RS_EntityContainer* parent,
                   const RS_SolidData& d)
        :RS_AtomicEntity(parent), data(d) {
    calculateBorders();
}



/**
 * @return Corner number 'num'. 
 */
RS_Vector RS_Solid::getCorner(int num) {
    if (num>=0 && num<4) {
        return data.corner[num];
    } else {
        RS_DEBUG->print("Illegal corner requested from Solid",
                        RS_Debug::D_WARNING);
        return RS_Vector(false);
    }
}



/**
 * Shapes this Solid into a standard arrow (used in dimensions).
 *
 * @param point The point the arrow points to.
 * @param angle Direction of the arrow.
 * @param arrowSize Size of arrow (length).
 */
void RS_Solid::shapeArrow(const RS_Vector& point,
                          double angle,
                          double arrowSize) {

    double cosv1, sinv1, cosv2, sinv2;
    double arrowSide = arrowSize/cos(0.165);

    cosv1 = cos(angle+0.165)*arrowSide;
    sinv1 = sin(angle+0.165)*arrowSide;
    cosv2 = cos(angle-0.165)*arrowSide;
    sinv2 = sin(angle-0.165)*arrowSide;

    data.corner[0] = point;
    data.corner[1] = RS_Vector(point.x - cosv1, point.y - sinv1);
    data.corner[2] = RS_Vector(point.x - cosv2, point.y - sinv2);
    data.corner[3] = RS_Vector(false);

    calculateBorders();
}



void RS_Solid::calculateBorders() {
    resetBorders();

    for (int i=0; i<4; ++i) {
        if (data.corner[i].valid) {
            minV = RS_Vector::minimum(minV, data.corner[i]);
            maxV = RS_Vector::maximum(maxV, data.corner[i]);
        }
    }
}



RS_Vector RS_Solid::getNearestEndpoint(const RS_Vector& coord, double* dist) {

    double minDist = RS_MAXDOUBLE;
    double curDist;
    RS_Vector ret;

    for (int i=0; i<4; ++i) {
        if (data.corner[i].valid) {
            curDist = data.corner[i].distanceTo(coord);
            if (curDist<minDist) {
                ret = data.corner[i];
                minDist = curDist;
            }
        }
    }

    if (dist!=NULL) {
        *dist = minDist;
    }

    return ret;
}



/**
 * @todo Implement this.
 */
RS_Vector RS_Solid::getNearestPointOnEntity(const RS_Vector& /*coord*/,
        bool /*onEntity*/, double* /*dist*/, RS_Entity** /*entity*/) {

    RS_Vector ret(false);
    return ret;
}



RS_Vector RS_Solid::getNearestCenter(const RS_Vector& /*coord*/,
                                     double* dist) {

    if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}



RS_Vector RS_Solid::getNearestMiddle(const RS_Vector& /*coord*/,
                                     double* dist) {
    if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}



RS_Vector RS_Solid::getNearestDist(double /*distance*/,
                                   const RS_Vector& /*coord*/,
                                   double* dist) {
    if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}



/**
 * @return Distance from one of the boundry lines of this solid to given point.
 *
 * @todo implement
 */
double RS_Solid::getDistanceToPoint(const RS_Vector& /*coord*/,
                                    RS_Entity** /*entity*/,
                                    RS2::ResolveLevel /*level*/,
								    double /*solidDist*/) {
    return RS_MAXDOUBLE;
}



void RS_Solid::move(RS_Vector offset) {
    for (int i=0; i<4; ++i) {
        data.corner[i].move(offset);
    }
    calculateBorders();
}



void RS_Solid::rotate(RS_Vector center, double angle) {
    for (int i=0; i<4; ++i) {
        data.corner[i].rotate(center, angle);
    }
    calculateBorders();
}



void RS_Solid::scale(RS_Vector center, RS_Vector factor) {
    for (int i=0; i<4; ++i) {
        data.corner[i].scale(center, factor);
    }
    calculateBorders();
}



void RS_Solid::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    for (int i=0; i<4; ++i) {
        data.corner[i].mirror(axisPoint1, axisPoint2);
    }
    calculateBorders();
}


void RS_Solid::draw(RS_Painter* painter, RS_GraphicView* view, 
	double /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }

    RS_SolidData d = getData();
    if (isTriangle()) {
        painter->fillTriangle(view->toGui(getCorner(0)),
                              view->toGui(getCorner(1)),
                              view->toGui(getCorner(2)));
    }

}


/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Solid& p) {
    os << " Solid: " << p.getData() << "\n";
    return os;
}

