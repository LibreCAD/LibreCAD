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

#include "rs_arc.h"

#include "rs_constructionline.h"
#include "rs_linetypepattern.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "rs_painter.h"


/**
 * Default constructor.
 */
RS_Arc::RS_Arc(RS_EntityContainer* parent,
               const RS_ArcData& d)
        : RS_AtomicEntity(parent), data(d) {
    calculateEndpoints();
    calculateBorders();
}



/**
 * Creates this arc from 3 given points which define the arc line.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 * @param p3 3rd point.
 */
bool RS_Arc::createFrom3P(const RS_Vector& p1, const RS_Vector& p2,
                          const RS_Vector& p3) {
    if (p1.distanceTo(p2)>RS_TOLERANCE &&
            p2.distanceTo(p3)>RS_TOLERANCE &&
            p3.distanceTo(p1)>RS_TOLERANCE) {

        // middle points between 3 points:
        RS_Vector mp1, mp2;
        RS_Vector dir1, dir2;
        double a1, a2;

        // intersection of two middle lines
        mp1 = (p1 + p2)/2.0;
        a1 = p1.angleTo(p2) + M_PI/2.0;
        dir1.setPolar(100.0, a1);
        mp2 = (p2 + p3)/2.0;
        a2 = p2.angleTo(p3) + M_PI/2.0;
        dir2.setPolar(100.0, a2);

        RS_ConstructionLineData d1(mp1, mp1 + dir1);
        RS_ConstructionLineData d2(mp2, mp2 + dir2);
        RS_ConstructionLine midLine1(NULL, d1);
        RS_ConstructionLine midLine2(NULL, d2);

        RS_VectorSolutions sol =
            RS_Information::getIntersection(&midLine1, &midLine2);

        data.center = sol.get(0);
        data.radius = data.center.distanceTo(p3);
        data.angle1 = data.center.angleTo(p1);
        data.angle2 = data.center.angleTo(p3);
        data.reversed = RS_Math::isAngleBetween(data.center.angleTo(p2),
                                                data.angle1, data.angle2, true);

        if (sol.get(0).valid && data.radius<1.0e14 &&
                data.radius>RS_TOLERANCE) {
            calculateEndpoints();
            calculateBorders();
            return true;
        } else {
            RS_DEBUG->print("RS_Arc::createFrom3P(): "
                            "Cannot create an arc with inf radius.");
            return false;
        }
    } else {
        RS_DEBUG->print("RS_Arc::createFrom3P(): "
                        "Cannot create an arc with radius 0.0.");
        return false;
    }
}



/**
 * Creates an arc from its startpoint, endpoint, start direction (angle)
 * and radius.
 * 
 * @retval true Successfully created arc
 * @retval false Cannot creats arc (radius to small or endpoint to far away)
 */
bool RS_Arc::createFrom2PDirectionRadius(const RS_Vector& startPoint,
        const RS_Vector& endPoint,
        double direction1, double radius) {

    RS_Vector ortho;
    ortho.setPolar(radius, direction1 + M_PI/2.0);
    RS_Vector center1 = startPoint + ortho;
    RS_Vector center2 = startPoint - ortho;

    if (center1.distanceTo(endPoint) < center2.distanceTo(endPoint)) {
        data.center = center1;
    } else {
        data.center = center2;
    }

    data.radius = radius;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(endPoint);
    data.reversed = false;

    double diff = RS_Math::correctAngle(getDirection1()-direction1);
    if (fabs(diff-M_PI)<1.0e-1) {
        data.reversed = true;
    }

    calculateEndpoints();
    calculateBorders();

    return true;
}



/**
 * Creates an arc from its startpoint, endpoint and bulge.
 */
bool RS_Arc::createFrom2PBulge(const RS_Vector& startPoint, const RS_Vector& endPoint,
                               double bulge) {
    data.reversed = (bulge<0.0);
    double alpha = atan(bulge)*4.0;

    RS_Vector middle = (startPoint+endPoint)/2.0;
    double dist = startPoint.distanceTo(endPoint)/2.0;

    // alpha can't be 0.0 at this point
    data.radius = fabs(dist / sin(alpha/2.0));

    double wu = fabs(RS_Math::pow(data.radius, 2.0) - RS_Math::pow(dist, 2.0));
    double h = sqrt(wu);
    double angle = startPoint.angleTo(endPoint);

    if (bulge>0.0) {
        angle+=M_PI/2.0;
    } else {
        angle-=M_PI/2.0;
    }

    if (fabs(alpha)>M_PI) {
        h*=-1.0;
    }

    data.center.setPolar(h, angle);
    data.center+=middle;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(endPoint);

    calculateEndpoints();
    calculateBorders();

	return true;
}



/**
 * Recalculates the endpoints using the angles and the radius.
 */
void RS_Arc::calculateEndpoints() {
    startpoint.set(data.center.x + cos(data.angle1) * data.radius,
                   data.center.y + sin(data.angle1) * data.radius);
    endpoint.set(data.center.x + cos(data.angle2) * data.radius,
                 data.center.y + sin(data.angle2) * data.radius);
}


void RS_Arc::calculateBorders() {
    double minX = std::min(startpoint.x, endpoint.x);
    double minY = std::min(startpoint.y, endpoint.y);
    double maxX = std::max(startpoint.x, endpoint.x);
    double maxY = std::max(startpoint.y, endpoint.y);

    double a1 = !isReversed() ? data.angle1 : data.angle2;
    double a2 = !isReversed() ? data.angle2 : data.angle1;

    // check for left limit:
    if ((a1<M_PI && a2>M_PI) ||
            (a1>a2-1.0e-12 && a2>M_PI) ||
            (a1>a2-1.0e-12 && a1<M_PI) ) {

        minX = std::min(data.center.x - data.radius, minX);
    }

    // check for right limit:
    if (a1 > a2-1.0e-12) {
        maxX = std::max(data.center.x + data.radius, maxX);
    }

    // check for bottom limit:
    if ((a1<(M_PI_2*3) && a2>(M_PI_2*3)) ||
            (a1>a2-1.0e-12    && a2>(M_PI_2*3)) ||
            (a1>a2-1.0e-12    && a1<(M_PI_2*3)) ) {

        minY = std::min(data.center.y - data.radius, minY);
    }

    // check for top limit:
    if ((a1<M_PI_2 && a2>M_PI_2) ||
            (a1>a2-1.0e-12   && a2>M_PI_2) ||
            (a1>a2-1.0e-12   && a1<M_PI_2) ) {

        maxY = std::max(data.center.y + data.radius, maxY);
    }

    minV.set(minX, minY);
    maxV.set(maxX, maxY);
}



RS_VectorSolutions RS_Arc::getRefPoints() {
    RS_VectorSolutions ret(startpoint, endpoint, data.center);
    return ret;
}


RS_Vector RS_Arc::getNearestEndpoint(const RS_Vector& coord, double* dist) {
    double dist1, dist2;
    RS_Vector* nearerPoint;

    dist1 = startpoint.distanceTo(coord);
    dist2 = endpoint.distanceTo(coord);

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = dist2;
        }
        nearerPoint = &endpoint;
    } else {
        if (dist!=NULL) {
            *dist = dist1;
        }
        nearerPoint = &startpoint;
    }

    return *nearerPoint;
}



RS_Vector RS_Arc::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) {

    RS_Vector vec(false);
    if (entity!=NULL) {
        *entity = this;
    }

    double angle = (coord-data.center).angle();
    if (onEntity==false || RS_Math::isAngleBetween(angle,
            data.angle1, data.angle2, isReversed())) {
        vec.setPolar(data.radius, angle);
        vec+=data.center;
    }
    if (dist!=NULL) {
        *dist = fabs((vec-data.center).magnitude()-data.radius);
    }

    return vec;
}



RS_Vector RS_Arc::getNearestCenter(const RS_Vector& coord,
                                   double* dist) {
    if (dist!=NULL) {
        *dist = coord.distanceTo(data.center);
    }
    return data.center;
}



RS_Vector RS_Arc::getNearestMiddle(const RS_Vector& coord,
                                   double* dist) {

    RS_Vector ret = getMiddlepoint();

    if (dist!=NULL) {
        *dist = coord.distanceTo(ret);
    }
    return ret;
}



RS_Vector RS_Arc::getNearestDist(double distance,
                                 const RS_Vector& coord,
                                 double* dist) {

    if (data.radius<1.0e-6) {
        if (dist!=NULL) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }

    double aDist = distance / data.radius;
    if (isReversed()) aDist= -aDist;
    double a;
    if(coord.distanceTo(getStartpoint()) < coord.distanceTo(getEndpoint())) {
        a=getAngle1() + aDist;
    }else {
        a=getAngle2() - aDist;
    }


    RS_Vector ret;
    ret.setPolar(data.radius, a);
    ret += getCenter();

    return ret;
}




RS_Vector RS_Arc::getNearestDist(double distance,
                                 bool startp) {

    if (data.radius<1.0e-6) {
        return RS_Vector(false);
    }

    double a;
    RS_Vector p;
    double aDist = distance / data.radius;

    if (isReversed()) {
        if (startp) {
            a = data.angle1 - aDist;
        } else {
            a = data.angle2 + aDist;
        }
    } else {
        if (startp) {
            a = data.angle1 + aDist;
        } else {
            a = data.angle2 - aDist;
        }
    }

    p.setPolar(data.radius, a);
    p += data.center;

    return p;
}



double RS_Arc::getDistanceToPoint(const RS_Vector& coord,
                                  RS_Entity** entity,
                                  RS2::ResolveLevel,
                                  double) {
    if (entity!=NULL) {
        *entity = this;
    }

    // check endpoints first:
    double dist = coord.distanceTo(getStartpoint());
    if (dist<1.0e-4) {
        return dist;
    }
    dist = coord.distanceTo(getEndpoint());
    if (dist<1.0e-4) {
        return dist;
    }

    if (RS_Math::isAngleBetween(data.center.angleTo(coord),
                                data.angle1, data.angle2,
                                isReversed())) {

        // RVT 6 Jan 2011 : Added selection by center point of arc
        float dToEdge=fabs((coord-data.center).magnitude() - data.radius);
        float dToCenter=data.center.distanceTo(coord);

        if (dToEdge<dToCenter) {
            return dToEdge;
        } else {
            return dToCenter;
        }

    } else {
        return RS_MAXDOUBLE;
    }
}



void RS_Arc::moveStartpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
    //if (parent!=NULL && parent->rtti()==RS2::EntityPolyline) {
		double bulge = getBulge();
		createFrom2PBulge(pos, getEndpoint(), bulge);
    //}

	// normal arc: move angle1
	/*else {
    	data.angle1 = data.center.angleTo(pos);
    	calculateEndpoints();
    	calculateBorders();
	}*/
}



void RS_Arc::moveEndpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
    //if (parent!=NULL && parent->rtti()==RS2::EntityPolyline) {
		double bulge = getBulge();
		createFrom2PBulge(getStartpoint(), pos, bulge);
    //}

	// normal arc: move angle1
	/*else {
    	data.angle2 = data.center.angleTo(pos);
	    calculateEndpoints();
    	calculateBorders();
	}*/
}



void RS_Arc::trimStartpoint(const RS_Vector& pos) {
    	data.angle1 = data.center.angleTo(pos);
    	calculateEndpoints();
    	calculateBorders();
}



void RS_Arc::trimEndpoint(const RS_Vector& pos) {
    	data.angle2 = data.center.angleTo(pos);
	    calculateEndpoints();
    	calculateBorders();
}


RS2::Ending RS_Arc::getTrimPoint(const RS_Vector& coord,
                                 const RS_Vector& trimPoint) {

    double angEl = data.center.angleTo(trimPoint);
    double angM = data.center.angleTo(coord);

    if (RS_Math::getAngleDifference(angM, angEl)>M_PI) {
        if (data.reversed) {
            return RS2::EndingEnd;
        } else {
            return RS2::EndingStart;
        }
    } else {
        if (data.reversed) {
            return RS2::EndingStart;
        } else {
            return RS2::EndingEnd;
        }
    }
}


void RS_Arc::reverse() {
    double a = data.angle1;
    data.angle1 = data.angle2;
    data.angle2 = a;
    data.reversed = !data.reversed;
    calculateEndpoints();
    calculateBorders();
}


void RS_Arc::move(RS_Vector offset) {
    data.center.move(offset);
    calculateEndpoints();
    calculateBorders();
}



void RS_Arc::rotate(RS_Vector center, double angle) {
    RS_DEBUG->print("RS_Arc::rotate");
    data.center.rotate(center, angle);
    data.angle1 = RS_Math::correctAngle(data.angle1+angle);
    data.angle2 = RS_Math::correctAngle(data.angle2+angle);
    calculateEndpoints();
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}



void RS_Arc::scale(RS_Vector center, RS_Vector factor) {
    // negative scaling: mirroring
    if (factor.x<0.0) {
        mirror(data.center, data.center + RS_Vector(0.0, 1.0));
        //factor.x*=-1;
    }
    if (factor.y<0.0) {
        mirror(data.center, data.center + RS_Vector(1.0, 0.0));
        //factor.y*=-1;
    }

    data.center.scale(center, factor);
    data.radius *= factor.x;
    if (data.radius<0.0) {
        data.radius*=-1.0;
    }
    calculateEndpoints();
    calculateBorders();
}



void RS_Arc::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    data.center.mirror(axisPoint1, axisPoint2);
    data.reversed = (!data.reversed);
    /*
    startpoint.mirror(axisPoint1, axisPoint2);
    endpoint.mirror(axisPoint1, axisPoint2);

    data.angle1 = data.center.angleTo(startpoint);
    data.angle2 = data.center.angleTo(endpoint);
    */

    RS_Vector vec;
    vec.setPolar(1.0, data.angle1);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle1 = vec.angle();

    vec.setPolar(1.0, data.angle2);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle2 = vec.angle();

    calculateEndpoints();
    calculateBorders();
}



void RS_Arc::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(startpoint)<1.0e-4) {
        moveStartpoint(startpoint+offset);
    }
    if (ref.distanceTo(endpoint)<1.0e-4) {
        moveEndpoint(endpoint+offset);
    }
}



void RS_Arc::stretch(RS_Vector firstCorner,
                      RS_Vector secondCorner,
                      RS_Vector offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
	else {
	    if (getStartpoint().isInWindow(firstCorner,
    	                               secondCorner)) {
        	moveStartpoint(getStartpoint() + offset);
	    }
	    if (getEndpoint().isInWindow(firstCorner,
	                                 secondCorner)) {
	        moveEndpoint(getEndpoint() + offset);
	    }
	}
}



void RS_Arc::draw(RS_Painter* painter, RS_GraphicView* view,
                  double /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }

    //double styleFactor = getStyleFactor();

    // simple style-less lines
    if (getPen().getLineType()==RS2::SolidLine ||
            isSelected() ||
            view->getDrawingMode()==RS2::ModePreview) {

        painter->drawArc(view->toGui(getCenter()),
                         getRadius() * view->getFactor().x,
                         getAngle1(), getAngle2(),
                         isReversed());
    } else {
        double styleFactor = getStyleFactor(view);
		if (styleFactor<0.0) {
        	painter->drawArc(view->toGui(getCenter()),
                         getRadius() * view->getFactor().x,
                         getAngle1(), getAngle2(),
                         isReversed());
			return;
		}

        // Pattern:
        RS_LineTypePattern* pat;
        if (isSelected()) {
            pat = &patternSelected;
        } else {
            pat = view->getPattern(getPen().getLineType());
        }

        if (pat==NULL) {
            return;
        }

        if (getRadius()<1.0e-6) {
            return;
        }

        // Pen to draw pattern is always solid:
        RS_Pen pen = painter->getPen();
        pen.setLineType(RS2::SolidLine);
        painter->setPen(pen);

        double a1;
        double a2;
        if (data.reversed) {
            a2 = getAngle1();
            a1 = getAngle2();
        } else {
            a1 = getAngle1();
            a2 = getAngle2();
        }

        double* da;     // array of distances in x.
        int i;          // index counter

        double length = getAngleLength();

        // create scaled pattern:
        da = new double[pat->num];

        for (i=0; i<pat->num; ++i) {
            da[i] = fabs(pat->pattern[i] * styleFactor) / getRadius();
        }

        double tot=0.0;
        i=0;
        bool done = false;
        double curA = a1;
        //double cx = getCenter().x * factor.x + offsetX;
        //double cy = - a->getCenter().y * factor.y + getHeight() - offsetY;
        RS_Vector cp = view->toGui(getCenter());
        double r = getRadius() * view->getFactor().x;

        do {
            if (pat->pattern[i] > 0.0) {
                if (tot+da[i]<length) {
                    painter->drawArc(cp, r,
                                     curA,
                                     curA + da[i],
                                     false);
                } else {
                    painter->drawArc(cp, r,
                                     curA,
                                     a2,
                                     false);
                }
            }
            curA+=da[i];
            tot+=da[i];
            done=tot>length;

            i++;
            if (i>=pat->num) {
                i=0;
            }
        } while(!done);

        delete[] da;
    }
}



/**
 * @return Middle point of the entity. 
 */
RS_Vector RS_Arc::getMiddlepoint() const {
    double a;
    RS_Vector ret;

    if (isReversed()) {
        a = data.angle1 - getAngleLength()/2.0;
    } else {
        a = data.angle1 + getAngleLength()/2.0;
    }
    ret.setPolar(data.radius, a);
    ret+=data.center;

    return ret;
}



/**
 * @return Angle length in rad.
 */
double RS_Arc::getAngleLength() const {
    double ret = 0.0;
    
    if (isReversed()) {
        if (data.angle1<data.angle2) {
            ret = data.angle1+2*M_PI-data.angle2;
        } else {
            ret = data.angle1-data.angle2;
        }
    } else {
        if (data.angle2<data.angle1) {
            ret = data.angle2+2*M_PI-data.angle1;
        } else {
            ret = data.angle2-data.angle1;
        }
    }

    // full circle:
    if (fabs(ret)<1.0e-6) {
        ret = 2*M_PI;
    }
    
    return ret;
}



/**
 * @return Length of the arc.
 */
double RS_Arc::getLength() {
    return getAngleLength()*data.radius;
}



/**
 * Gets the arc's bulge (tangens of angle length divided by 4).
 */
double RS_Arc::getBulge() const {
    double bulge = tan(fabs(getAngleLength())/4.0);
    if (isReversed()) {
        bulge*=-1;
    }
    return bulge;
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Arc& a) {
    os << " Arc: " << a.data << "\n";
    return os;
}

