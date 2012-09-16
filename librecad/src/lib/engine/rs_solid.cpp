/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#include "rs_information.h"


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



RS_Vector RS_Solid::getNearestEndpoint(const RS_Vector& coord, double* dist)const {

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

bool RS_Solid::isInCrossWindow(const RS_Vector& v1,const RS_Vector& v2)const {
//    bool sol = false;
    RS_Vector vBL, vTR;
    RS_VectorSolutions sol;
//sort imput vectors to BottomLeft & TopRight
    if (v1.x<v2.x) {
        vBL.x = v1.x;
        vTR.x = v2.x;
    } else {
        vBL.x = v2.x;
        vTR.x = v1.x;
    }
    if (v1.y<v2.y) {
        vBL.y = v1.y;
        vTR.y = v2.y;
    } else {
        vBL.y = v2.y;
        vTR.y = v1.y;
    }
    //Check if is out of window
    if ( getMin().x > vTR.x || getMax().x < vBL.x
           || getMin().y > vTR.y || getMax().y < vBL.y) {
        return false;
    }
    int totalV = 3;
    if (data.corner[3].valid)
        totalV = 4;
    RS_Line l[totalV];
    l[0] = RS_Line(NULL, RS_LineData(data.corner[0], data.corner[1]));
    l[1] = RS_Line(NULL, RS_LineData(data.corner[1], data.corner[2]));
    if (data.corner[3].valid) {
        l[2] = RS_Line(NULL, RS_LineData(data.corner[2], data.corner[3]));
        l[3] = RS_Line(NULL, RS_LineData(data.corner[3], data.corner[0]));
    } else {
        l[2] = RS_Line(NULL, RS_LineData(data.corner[2], data.corner[0]));
    }
    //Find crossing edge
    if (getMax().x > vBL.x && getMin().x < vBL.x) {//left
        RS_Line edge = RS_Line(NULL, RS_LineData(vBL, RS_Vector(vBL.x, vTR.y)));
        for (int i=0; i<totalV; ++i) {
            sol = RS_Information::getIntersection(&edge, &l[i], true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().x < vTR.x && getMin().x > vTR.x) {//right
        RS_Line edge = RS_Line(NULL, RS_LineData(RS_Vector(vTR.x, vBL.y), vTR));
        for (int i=0; i<totalV; ++i) {
            sol = RS_Information::getIntersection(&edge, &l[i], true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().y > vBL.y && getMin().y < vBL.y) {//bottom
        RS_Line edge = RS_Line(NULL, RS_LineData(vBL, RS_Vector(vTR.x, vBL.y)));
        for (int i=0; i<totalV; ++i) {
            sol = RS_Information::getIntersection(&edge, &l[i], true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if(getMax().y < vBL.y && getMin().y > vBL.y) {//top
        RS_Line edge = RS_Line(NULL, RS_LineData(RS_Vector(vBL.x, vTR.y), vTR));
        for (int i=0; i<totalV; ++i) {
            sol = RS_Information::getIntersection(&edge, &l[i], true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    return false;
}

/**
*
* @return true if positive o zero, false if negative.
*/
bool RS_Solid::sign (const RS_Vector v1, const RS_Vector v2, const RS_Vector v3)const {
    double res = (v1.x-v3.x)*(v2.y-v3.y)-(v2.x-v3.x)*(v1.y-v3.y);
    return (res>=0.0);
}

/**
 * @todo Implement this.
 */
RS_Vector RS_Solid::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity)const {
//first check if point is inside solid
    bool s1 = sign(data.corner[0], data.corner[1], coord);
    bool s2 = sign(data.corner[1], data.corner[2], coord);
    bool s3 = sign(data.corner[2], data.corner[0], coord);
    if ( (s1 == s2) && (s2 == s3) ) {
        if (dist!=NULL)
            *dist = 0.0;
        return coord;
    }
    if (data.corner[3].valid) {
        s1 = sign(data.corner[0], data.corner[2], coord);
        s2 = sign(data.corner[2], data.corner[3], coord);
        s3 = sign(data.corner[3], data.corner[0], coord);
        if ( (s1 == s2) && (s2 == s3) ) {
            if (dist!=NULL)
                *dist = 0.0;
            return coord;
        }
    }
    //not inside of solid
    RS_Vector ret(false);
    double currDist = RS_MAXDOUBLE;
    double tmpDist;
    if (entity!=NULL) {
        *entity = const_cast<RS_Solid*>(this);
    }
    //Find nearest distance from each edge
    int totalV = 3;
    if (data.corner[3].valid)
        totalV = 4;
    for (int i=0; i<=totalV; ++i) {
        int next =i+1;
        //closing edge
        if (next == totalV) next =0;

        RS_Vector direction = data.corner[next]-data.corner[i];
        RS_Vector vpc=coord-data.corner[i];
        double a=direction.squared();
        if( a < RS_TOLERANCE*RS_TOLERANCE) {
            //line too short
            vpc=data.corner[i];
        }else{
            //find projection on line
            vpc = data.corner[i] + direction*RS_Vector::dotP(vpc,direction)/a;
        }
        tmpDist = vpc.distanceTo(coord);
        if (tmpDist < currDist) {
            currDist = tmpDist;
            ret = vpc;
        }
    }
    //verify this part
    if( onEntity && !ret.isInWindowOrdered(minV,maxV) ){
        //projection point not within range, find the nearest endpoint
        ret = getNearestEndpoint(coord,dist);
        currDist = ret.distanceTo(coord);
    }

    if (dist!=NULL) {
        *dist = currDist;
    }

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
                                     double* dist,
                                     const int /*middlePoints*/)const {
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
 */
double RS_Solid::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity,
                                    RS2::ResolveLevel /*level*/,
                                    double /*solidDist*/)const {
    if (entity!=NULL) {
        *entity = const_cast<RS_Solid*>(this);
    }
    double ret;
    getNearestPointOnEntity(coord,true,&ret,entity);
    return ret;
}



void RS_Solid::move(const RS_Vector& offset) {
    for (int i=0; i<4; ++i) {
        data.corner[i].move(offset);
    }
    calculateBorders();
}



//rotation
void RS_Solid::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    for (int i=0; i<4; ++i) {
        data.corner[i].rotate(center, angleVector);
    }
    calculateBorders();
}

void RS_Solid::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (int i=0; i<4; ++i) {
        data.corner[i].rotate(center, angleVector);
    }
    calculateBorders();
}



void RS_Solid::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (int i=0; i<4; ++i) {
        data.corner[i].scale(center, factor);
    }
    calculateBorders();
}



void RS_Solid::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (int i=0; i<4; ++i) {
        data.corner[i].mirror(axisPoint1, axisPoint2);
    }
    calculateBorders();
}


void RS_Solid::draw(RS_Painter* painter, RS_GraphicView* view,
        double& /*patternOffset*/) {

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

