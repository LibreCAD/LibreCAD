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


#include "rs_line.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"
#include "rs_linetypepattern.h"

/**
 * Constructor.
 */
RS_Line::RS_Line(RS_EntityContainer* parent,
                 const RS_LineData& d)
    :RS_AtomicEntity(parent), data(d) {
    calculateBorders();
}



/**
 * Destructor.
 */
RS_Line::~RS_Line() {}




RS_Entity* RS_Line::clone() {
    RS_Line* l = new RS_Line(*this);
    l->initId();
    return l;
}



void RS_Line::calculateBorders() {
    minV = RS_Vector::minimum(data.startpoint, data.endpoint);
    maxV = RS_Vector::maximum(data.startpoint, data.endpoint);
}



RS_VectorSolutions RS_Line::getRefPoints() {
    RS_VectorSolutions ret(data.startpoint, data.endpoint);
    return ret;
}


RS_Vector RS_Line::getNearestEndpoint(const RS_Vector& coord,
                                      double* dist) {
    double dist1, dist2;
    RS_Vector* nearerPoint;

    dist1 = data.startpoint.distanceTo(coord);
    dist2 = data.endpoint.distanceTo(coord);

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = dist2;
        }
        nearerPoint = &data.endpoint;
    } else {
        if (dist!=NULL) {
            *dist = dist1;
        }
        nearerPoint = &data.startpoint;
    }

    return *nearerPoint;
}



RS_Vector RS_Line::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) {

    if (entity!=NULL) {
        *entity = this;
    }
    RS_Vector ret;

    RS_Vector vpl = data.endpoint-data.startpoint;
    double angle=vpl.angle();
    double r=vpl.magnitude();
    RS_Vector vpc=coord-data.startpoint;
    vpc.rotate(-angle); // rotate to use the line direction as x-axis
    if ( (vpc.x >= 0. && vpc.x <= r) || ! onEntity ) { //use the projection
    	ret=RS_Vector(vpc.x,0.);
	ret.rotate(angle);
	ret += data.startpoint;
    } else {// onEntity=true and projection not within range, only have to check the endpoints
    	ret=getNearestEndpoint(coord,dist);
    }

    if (dist!=NULL) {
        *dist = ret.distanceTo(coord);
	}
    return ret;
}



RS_Vector RS_Line::getNearestCenter(const RS_Vector& coord,
                                    double* dist) {

    RS_Vector p = (data.startpoint + data.endpoint)/2.0;

    if (dist!=NULL) {
        *dist = p.distanceTo(coord);
    }

    return p;
}



RS_Vector RS_Line::getNearestMiddle(const RS_Vector& coord,
                                    double* dist) {
    return getNearestCenter(coord, dist);
}



RS_Vector RS_Line::getNearestDist(double distance,
                                  const RS_Vector& coord,
                                  double* dist) {

    double a1 = getAngle1();

    RS_Vector dv;
    dv.setPolar(distance, a1);

    RS_Vector p1 = data.startpoint + dv;
    RS_Vector p2 = data.endpoint - dv;

    double dist1, dist2;
    RS_Vector* nearerPoint;

    dist1 = p1.distanceTo(coord);
    dist2 = p2.distanceTo(coord);

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = dist2;
        }
        nearerPoint = &p2;
    } else {
        if (dist!=NULL) {
            *dist = dist1;
        }
        nearerPoint = &p1;
    }

    return *nearerPoint;
}



RS_Vector RS_Line::getNearestDist(double distance,
                                  bool startp) {

    double a1 = getAngle1();

    RS_Vector dv;
    dv.setPolar(distance, a1);
    RS_Vector ret;

    if (startp) {
        ret = data.startpoint + dv;
    }
    else {
        ret = data.endpoint - dv;
    }

    return ret;

}


/*RS_Vector RS_Line::getNearestRef(const RS_Vector& coord,
                                 double* dist) {
    double d1, d2, d;
    RS_Vector p;
    RS_Vector p1 = getNearestEndpoint(coord, &d1);
    RS_Vector p2 = getNearestMiddle(coord, &d2);

    if (d1<d2) {
        d = d1;
        p = p1;
    } else {
        d = d2;
        p = p2;
    }

    if (dist!=NULL) {
        *dist = d;
    }

    return p;
}*/


double RS_Line::getDistanceToPoint(const RS_Vector& coord,
                                   RS_Entity** entity,
                                   RS2::ResolveLevel /*level*/,
                                   double /*solidDist*/) {

    RS_DEBUG->print("RS_Line::getDistanceToPoint");

    if (entity!=NULL) {
        *entity = this;
    }
    double ret;
    getNearestPointOnEntity(coord,true,&ret,entity);
    //std::cout<<"rs_line::getDistanceToPoint(): new algorithm dist= "<<ret<<std::endl;
    return ret;
//    // check endpoints first:
//    double dist = coord.distanceTo(getStartpoint());
//    if (dist<1.0e-4) {
//        RS_DEBUG->print("RS_Line::getDistanceToPoint: OK1");
//        return dist;
//    }
//    dist = coord.distanceTo(getEndpoint());
//    if (dist<1.0e-4) {
//        RS_DEBUG->print("RS_Line::getDistanceToPoint: OK2");
//        return dist;
//    }
//
//    dist = RS_MAXDOUBLE;
//    RS_Vector ae = data.endpoint-data.startpoint;
//    RS_Vector ea = data.startpoint-data.endpoint;
//    RS_Vector ap = coord-data.startpoint;
//    RS_Vector ep = coord-data.endpoint;
//
//    if (ae.magnitude()<1.0e-6 || ea.magnitude()<1.0e-6) {
//        RS_DEBUG->print("RS_Line::getDistanceToPoint: OK2a");
//        return dist;
//    }
//
//    // Orthogonal projection from both sides:
//    RS_Vector ba = ae * RS_Vector::dotP(ae, ap) /
//                   RS_Math::pow(ae.magnitude(), 2);
//    RS_Vector be = ea * RS_Vector::dotP(ea, ep) /
//                   RS_Math::pow(ea.magnitude(), 2);
//
//    // Check if the projection is outside this line:
//    if (ba.magnitude()>ae.magnitude() || be.magnitude()>ea.magnitude()) {
//        // return distance to endpoint
//        getNearestEndpoint(coord, &dist);
//        RS_DEBUG->print("RS_Line::getDistanceToPoint: OK3");
//        return dist;
//    }
//    //RS_DEBUG->print("ba: %f", ba.magnitude());
//    //RS_DEBUG->print("ae: %f", ae.magnitude());
//
//    RS_Vector cp = RS_Vector::crossP(ap, ae);
//    dist = cp.magnitude() / ae.magnitude();
//
//    RS_DEBUG->print("RS_Line::getDistanceToPoint: OK4");
//    std::cout<<"rs_line::getDistanceToPoint(): Old algorithm dist= "<<ret<<std::endl;
//
//    return dist;
}



void RS_Line::moveStartpoint(const RS_Vector& pos) {
    data.startpoint = pos;
    calculateBorders();
}



void RS_Line::moveEndpoint(const RS_Vector& pos) {
    data.endpoint = pos;
    calculateBorders();
}



RS_Vector RS_Line::prepareTrim(const RS_Vector& trimCoord,
                               const RS_VectorSolutions& trimSol) {
//prepare trimming for multiple intersections
    if ( ! trimSol.hasValid()) return(RS_Vector(false));
    if ( trimSol.getNumber() == 1 ) return(trimSol.get(0));
    return trimSol.getClosest(trimCoord,NULL,0);
}

RS2::Ending RS_Line::getTrimPoint(const RS_Vector& trimCoord,
                                  const RS_Vector& trimPoint) {
    RS_Vector vp1=getStartpoint() - trimCoord;
    RS_Vector vp2=trimPoint - trimCoord;
    if ( RS_Vector::dotP(vp1,vp2) < 0 ) {
        return RS2::EndingEnd;
    } else {
        return RS2::EndingStart;
    }
}


void RS_Line::reverse() {
    RS_Vector v = data.startpoint;
    data.startpoint = data.endpoint;
    data.endpoint = v;
}



bool RS_Line::hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) {
    if (data.startpoint.isInWindow(v1, v2) ||
            data.endpoint.isInWindow(v1, v2)) {
        return true;
    } else {
        return false;
    }
}


void RS_Line::move(RS_Vector offset) {
    RS_DEBUG->print("RS_Line::move1: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);

    RS_DEBUG->print("RS_Line::move1: offset: %f/%f", offset.x, offset.y);

    data.startpoint.move(offset);
    data.endpoint.move(offset);
    calculateBorders();
    RS_DEBUG->print("RS_Line::move2: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
}

void RS_Line::rotate(double angle) {
    RS_DEBUG->print("RS_Line::rotate");
    RS_DEBUG->print("RS_Line::rotate1: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    data.startpoint.rotate(angle);
    data.endpoint.rotate(angle);
    RS_DEBUG->print("RS_Line::rotate2: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
    RS_DEBUG->print("RS_Line::rotate: OK");
}



void RS_Line::rotate(RS_Vector center, double angle) {
    RS_DEBUG->print("RS_Line::rotate");
    RS_DEBUG->print("RS_Line::rotate1: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    data.startpoint.rotate(center, angle);
    data.endpoint.rotate(center, angle);
    RS_DEBUG->print("RS_Line::rotate2: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
    RS_DEBUG->print("RS_Line::rotate: OK");
}



void RS_Line::scale(RS_Vector center, RS_Vector factor) {
    RS_DEBUG->print("RS_Line::scale1: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
    RS_DEBUG->print("RS_Line::scale2: sp: %f/%f, ep: %f/%f",
                    data.startpoint.x, data.startpoint.y,
                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
}



void RS_Line::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}


/**
 * Stretches the given range of the entity by the given offset.
 */
void RS_Line::stretch(RS_Vector firstCorner,
                      RS_Vector secondCorner,
                      RS_Vector offset) {

    if (getStartpoint().isInWindow(firstCorner,
                                   secondCorner)) {
        moveStartpoint(getStartpoint() + offset);
    }
    if (getEndpoint().isInWindow(firstCorner,
                                 secondCorner)) {
        moveEndpoint(getEndpoint() + offset);
    }
}



void RS_Line::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(data.startpoint)<1.0e-4) {
        moveStartpoint(data.startpoint+offset);
    }
    if (ref.distanceTo(data.endpoint)<1.0e-4) {
        moveEndpoint(data.endpoint+offset);
    }
}




void RS_Line::draw(RS_Painter* painter, RS_GraphicView* view, double patternOffset) {
    if (painter==NULL || view==NULL) {
        return;
    }

    if (getPen().getLineType()==RS2::SolidLine ||
            isSelected() ||
            view->getDrawingMode()==RS2::ModePreview) {

        painter->drawLine(view->toGui(getStartpoint()),
                          view->toGui(getEndpoint()));
    } else {
        double styleFactor = getStyleFactor(view);
        if (styleFactor<0.0) {
            painter->drawLine(view->toGui(getStartpoint()),
                              view->toGui(getEndpoint()));
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
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Line::draw: Invalid line pattern");
            return;
        }

        // Pen to draw pattern is always solid:
        RS_Pen pen = painter->getPen();

        pen.setLineType(RS2::SolidLine);
        painter->setPen(pen);

        // index counter
        int i;

        // line data:
        double length = getLength();
        double angle = getAngle1();

        // pattern segment length:
        double patternSegmentLength = 0.0;

        // create pattern:
        RS_Vector* dp = new RS_Vector[pat->num];

        for (i=0; i<pat->num; ++i) {
            dp[i] = RS_Vector(cos(angle) * fabs(pat->pattern[i] * styleFactor),
                              sin(angle) * fabs(pat->pattern[i] * styleFactor));

            patternSegmentLength += fabs(pat->pattern[i] * styleFactor);
        }

        // handle pattern offset:
        int m;
        if (patternOffset<0.0) {
            m = (int)ceil(patternOffset / patternSegmentLength);
        }
        else {
            m = (int)floor(patternOffset / patternSegmentLength);
        }

        patternOffset -= (m*patternSegmentLength);
        //if (patternOffset<0.0) {
        //	patternOffset+=patternSegmentLength;
        //}
        //RS_DEBUG->print("pattern. offset: %f", patternOffset);
        RS_Vector patternOffsetVec;
        patternOffsetVec.setPolar(patternOffset, angle);

        double tot=patternOffset;
        i=0;
        bool done = false;
        bool cutStartpoint, cutEndpoint, drop;
        RS_Vector curP=getStartpoint()+patternOffsetVec;

        do {
            cutStartpoint = false;
            cutEndpoint = false;
            drop = false;

            // line segment (otherwise space segment)
            if (pat->pattern[i]>0.0) {
                // drop the whole pattern segment line:
                if (tot+pat->pattern[i]*styleFactor < 0.0) {
                    drop = true;
                }
                else {
                    // trim startpoint of pattern segment line to line startpoint
                    if (tot < 0.0) {
                        cutStartpoint = true;
                    }
                    // trim endpoint of pattern segment line to line endpoint
                    if (tot+pat->pattern[i]*styleFactor > length) {
                        cutEndpoint = true;
                    }
                }

                if (drop) {
                    // do nothing
                }
                else {
                    RS_Vector p1 = curP;
                    RS_Vector p2 = curP + dp[i];

                    if (cutStartpoint) {
                        p1 = getStartpoint();
                    }
                    if (cutEndpoint) {
                        p2 = getEndpoint();
                    }

                    painter->drawLine(view->toGui(p1),
                                      view->toGui(p2));
                }
            }
            curP+=dp[i];
            tot+=fabs(pat->pattern[i]*styleFactor);
            //RS_DEBUG->print("pattern. tot: %f", tot);
            done=tot>length;

            i++;
            if (i>=pat->num) {
                i=0;
            }
        } while(!done);

        delete[] dp;

    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Line& l) {
    os << " Line: " << l.getData() << "\n";
    return os;
}


