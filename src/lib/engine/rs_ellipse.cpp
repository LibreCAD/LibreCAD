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


#include "rs_ellipse.h"

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_information.h"
#include "rs_linetypepattern.h"

/**
 * Constructor.
 */
RS_Ellipse::RS_Ellipse(RS_EntityContainer* parent,
                       const RS_EllipseData& d)
    :RS_AtomicEntity(parent), data(d) {

    //calculateEndpoints();
    calculateBorders();
}


/**
 * Recalculates the endpoints using the angles and the radius.
 */
/*
void RS_Ellipse::calculateEndpoints() {
   double angle = data.majorP.angle();
   double radius1 = getMajorRadius();
   double radius2 = getMinorRadius();

   startpoint.set(data.center.x + cos(data.angle1) * radius1,
                  data.center.y + sin(data.angle1) * radius2);
   startpoint.rotate(data.center, angle);
   endpoint.set(data.center.x + cos(data.angle2) * radius1,
                data.center.y + sin(data.angle2) * radius2);
   endpoint.rotate(data.center, angle);
}
*/


/**
 * Calculates the boundary box of this ellipse.
 *
 * @todo Fix that - the algorithm used is really bad / slow.
 */
void RS_Ellipse::calculateBorders() {
    RS_DEBUG->print("RS_Ellipse::calculateBorders");

    double radius1 = getMajorRadius();
    double radius2 = getMinorRadius();
    double angle = getAngle();
    double a1 = ((!isReversed()) ? data.angle1 : data.angle2);
    double a2 = ((!isReversed()) ? data.angle2 : data.angle1);
    RS_Vector startpoint = getStartpoint();
    RS_Vector endpoint = getEndpoint();

    double minX = std::min(startpoint.x, endpoint.x);
    double minY = std::min(startpoint.y, endpoint.y);
    double maxX = std::max(startpoint.x, endpoint.x);
    double maxY = std::max(startpoint.y, endpoint.y);

    // kind of a brute force. TODO: exact calculation
    RS_Vector vp;
//    double a = a1;

//    do {
//        vp.set(data.center.x + radius1 * cos(a),
//               data.center.y + radius2 * sin(a));
//        vp.rotate(data.center, angle);
//
//        minX = std::min(minX, vp.x);
//        minY = std::min(minY, vp.y);
//        maxX = std::max(maxX, vp.x);
//        maxY = std::max(maxY, vp.y);
//
//        a += 0.03;
//    } while (RS_Math::isAngleBetween(RS_Math::correctAngle(a), a1, a2, false) &&
//             a<4*M_PI);
//    std::cout<<"a1="<<a1<<"\ta2="<<a2<<std::endl<<"Old algorithm:\nminX="<<minX<<"\tmaxX="<<maxX<<"\nminY="<<minY<<"\tmaxY="<<maxY<<std::endl;

    // Exact algorithm, based on rotation:
    // ( r1*cos(a), r2*sin(a)) rotated by angle to
    // (r1*cos(a)*cos(angle)-r2*sin(a)*sin(angle),r1*cos(a)*sin(angle)+r2*sin(a)*cos(angle))
    // both coordinates can be further reorganized to the form rr*cos(a+ theta),
    // with rr and theta angle defined by the coordinates given above
    double amin,delta_a;
//      x range
    vp.set(radius1*cos(angle),radius2*sin(angle));

    amin=fmod(2*M_PI+a1+vp.angle(),2*M_PI); // to the range of 0 to 2*M_PI
    delta_a=fmod(4*M_PI+a2-a1,2*M_PI);

    if( (amin<=M_PI && delta_a >= M_PI - amin) || (amin > M_PI && delta_a >= 3*M_PI - amin))
        minX= data.center.x-vp.magnitude();
//    else
//       minX=data.center.x +vp.magnitude()*std::min(cos(amin),cos(amin+delta_a));
    if( delta_a >= 2*M_PI - amin )
        maxX= data.center.x+vp.magnitude();
//    else
//       maxX= data.center.x+vp.magnitude()*std::max(cos(amin),cos(amin+delta_a));
//      y range
    vp.set(radius1*sin(angle),-1*radius2*cos(angle));
    amin=fmod(2*M_PI+a1+vp.angle(),2*M_PI); // to the range of 0 to 2*M_PI
    if( (amin<=M_PI &&delta_a >= M_PI - amin) || (amin > M_PI && delta_a >= 3*M_PI - amin))
        minY= data.center.y-vp.magnitude();
//    else
//        minY=data.center.y +vp.magnitude()*std::min(cos(amin),cos(amin+delta_a));
    if( delta_a >= 2*M_PI - amin )
        maxY= data.center.y+vp.magnitude();
//    else
//        maxY= data.center.y+vp.magnitude()*std::max(cos(amin),cos(amin+delta_a));

//std::cout<<"New algorithm:\nminX="<<minX<<"\tmaxX="<<maxX<<"\nminY="<<minY<<"\tmaxY="<<maxY<<std::endl;


    minV.set(minX, minY);
    maxV.set(maxX, maxY);
    RS_DEBUG->print("RS_Ellipse::calculateBorders: OK");
}



RS_VectorSolutions RS_Ellipse::getRefPoints() {
    RS_VectorSolutions ret(getStartpoint(), getEndpoint(), data.center);
    return ret;
}



RS_Vector RS_Ellipse::getNearestEndpoint(const RS_Vector& coord, double* dist) {
    double dist1, dist2;
    RS_Vector nearerPoint;
    RS_Vector startpoint = getStartpoint();
    RS_Vector endpoint = getEndpoint();

    dist1 = startpoint.distanceTo(coord);
    dist2 = endpoint.distanceTo(coord);

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = dist2;
        }
        nearerPoint = endpoint;
    } else {
        if (dist!=NULL) {
            *dist = dist1;
        }
        nearerPoint = startpoint;
    }

    return nearerPoint;
}



RS_Vector RS_Ellipse::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) {

    RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity");

    RS_Vector ret(false);

    if (entity!=NULL) {
        *entity = this;
    }
    double ang = getAngle();

    RS_Vector normalized = (coord - data.center).rotate(-ang);

    double dU = normalized.x;
    double dV = normalized.y;
    double dA = getMajorRadius();
    double dB = getMinorRadius();
    double dEpsilon = 1.0e-8;
    int iMax = 32;
    int riIFinal = 0;
    double rdX = 0.0;
    double rdY = 0.0;
    double dDistance;
    bool swap = false;
    bool majorSwap = false;

    if (dA<dB) {
        double dum = dA;
        dA = dB;
        dB = dum;
        dum = dU;
        dU = dV;
        dV = dum;
        majorSwap = true;
    }

    if (dV<0.0) {
        dV*=-1.0;
        swap = true;
    }

    // initial guess
    double dT = dB*(dV - dB);

    // Newton s method
    int i;
    for (i = 0; i < iMax; i++) {
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: i: %d", i);
        double dTpASqr = dT + dA*dA;
        double dTpBSqr = dT + dB*dB;
        double dInvTpASqr = 1.0/dTpASqr;
        double dInvTpBSqr = 1.0/dTpBSqr;
        double dXDivA = dA*dU*dInvTpASqr;
        double dYDivB = dB*dV*dInvTpBSqr;
        double dXDivASqr = dXDivA*dXDivA;
        double dYDivBSqr = dYDivB*dYDivB;
        double dF = dXDivASqr + dYDivBSqr - 1.0;
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: dF: %f", dF);
        if ( fabs(dF) < dEpsilon ) {
            // F(t0) is close enough to zero, terminate the iteration:
            rdX = dXDivA*dA;
            rdY = dYDivB*dB;
            riIFinal = i;
            RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: rdX,rdY 1: %f,%f", rdX, rdY);
            break;
        }
        double dFDer = 2.0*(dXDivASqr*dInvTpASqr + dYDivBSqr*dInvTpBSqr);
        double dRatio = dF/dFDer;
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: dRatio: %f", dRatio);
        if ( fabs(dRatio) < dEpsilon ) {
            // t1-t0 is close enough to zero, terminate the iteration:
            rdX = dXDivA*dA;
            rdY = dYDivB*dB;
            riIFinal = i;
            RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: rdX,rdY 2: %f,%f", rdX, rdY);
            break;
        }
        dT += dRatio;
    }
    if ( i == iMax ) {
        // failed to converge:
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: failed");
        dDistance = RS_MAXDOUBLE;
    }
    else {
        double dDelta0 = rdX - dU;
        double dDelta1 = rdY - dV;
        dDistance = sqrt(dDelta0*dDelta0 + dDelta1*dDelta1);
        ret = RS_Vector(rdX, rdY);
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: rdX,rdY 2: %f,%f", rdX, rdY);
        RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity: ret: %f,%f", ret.x, ret.y);
    }

    if (dist!=NULL) {
        if (ret.valid) {
            *dist = dDistance;
        } else {
            *dist = RS_MAXDOUBLE;
        }
    }

    if (ret.valid) {
        if (swap) {
            ret.y*=-1.0;
        }
        if (majorSwap) {
            double dum = ret.x;
            ret.x = ret.y;
            ret.y = dum;
        }
        ret = (ret.rotate(ang) + data.center);

        if (onEntity) {
            double a1 = data.center.angleTo(getStartpoint());
            double a2 = data.center.angleTo(getEndpoint());
            double a = data.center.angleTo(ret);
            if (!RS_Math::isAngleBetween(a, a1, a2, data.reversed)) {
                ret = RS_Vector(false);
            }
        }
    }

    return ret;
}



/**
 * @param tolerance Tolerance.
 *
 * @retval true if the given point is on this entity.
 * @retval false otherwise
 */
bool RS_Ellipse::isPointOnEntity(const RS_Vector& coord,
                                 double tolerance) {
    double dist = getDistanceToPoint(coord, NULL, RS2::ResolveNone);
    return (dist<=tolerance);
}



RS_Vector RS_Ellipse::getNearestCenter(const RS_Vector& coord,
                                       double* dist) {
    if (dist!=NULL) {
        *dist = coord.distanceTo(data.center);
    }
    return data.center;
}



/**
 * a naive implementation of middle point
 * to accurately locate the middle point from arc length is possible by using elliptic integral to find the total arc length, then, using elliptic function to find the half length point
 */
RS_Vector RS_Ellipse::getNearestMiddle(const RS_Vector& coord,
                                       double* dist) {
    //std::cout<<"angle1="<<data.angle1<<"\tdata.angle2="<<data.angle2<<std::endl;
    if ( fmod( fabs( data.angle1 - data.angle2),2*M_PI) < 1e-6) { // no middle point for whole ellipse
        if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);

    }
        double amin,amax;
        RS_Vector vp;
        vp.setPolar(1.0,data.angle1);
        vp.scale(RS_Vector(1.0,data.ratio));
        amin=vp.angle();
        vp.setPolar(1.0,data.angle2);
        vp.scale(RS_Vector(1.0,data.ratio));
        amax=vp.angle();
        amin=0.5*(amin+amax);
        if (data.reversed ^ amin > amax) amin += M_PI; // condition to adjust one end by 2*M_PI, therefore, the middle point by M_PI

    //std::cout<<"middle="<<amin<<std::endl;
    vp.set(getMajorRadius()*cos(amin),getMinorRadius()*sin(amin));
    vp.rotate(getAngle());
    vp.move(data.center);
    if (dist!=NULL) {
        *dist = coord.distanceTo(vp);
    }
    return vp;
}



RS_Vector RS_Ellipse::getNearestDist(double /*distance*/,
                                     const RS_Vector& /*coord*/,
                                     double* dist) {
    if (dist!=NULL) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}



double RS_Ellipse::getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity,
                                      RS2::ResolveLevel, double /*solidDist*/) {
    double dist = RS_MAXDOUBLE;
    getNearestPointOnEntity(coord, true, &dist, entity);

    // RVT 6 Jan 2011 : Add selection by center point
    float dToCenter=data.center.distanceTo(coord);

    if (dist<dToCenter) {
        return dist;
    } else {
        return dToCenter;
    }

}



void RS_Ellipse::move(RS_Vector offset) {
    data.center.move(offset);
    //calculateEndpoints();
    calculateBorders();
}



void RS_Ellipse::rotate(RS_Vector center, double angle) {
    data.center.rotate(center, angle);
    data.majorP.rotate(angle);
    //calculateEndpoints();
    calculateBorders();
}



void RS_Ellipse::moveStartpoint(const RS_Vector& pos) {
    data.angle1 = getEllipseAngle(pos);
    //data.angle1 = data.center.angleTo(pos);
    //calculateEndpoints();
    calculateBorders();
}



void RS_Ellipse::moveEndpoint(const RS_Vector& pos) {
    data.angle2 = getEllipseAngle(pos);
    //data.angle2 = data.center.angleTo(pos);
    //calculateEndpoints();
    calculateBorders();
}


RS2::Ending RS_Ellipse::getTrimPoint(const RS_Vector& coord,
                                     const RS_Vector& trimPoint) {

    double angEl = getEllipseAngle(trimPoint);
    double angM = getEllipseAngle(coord);

    if (RS_Math::getAngleDifference(angM, angEl)>M_PI) {
        //if (data.reversed) {
        //	return RS2::EndingEnd;
        //}
        //else {
        return RS2::EndingStart;
        //}
    }
    else {
        //if (data.reversed) {
        //	return RS2::EndingStart;
        //}
        //else {
        return RS2::EndingEnd;
        //}
    }
}

double RS_Ellipse::getEllipseAngle(const RS_Vector& pos) {
    RS_Vector m = pos-data.center;
    m.rotate(-data.majorP.angle());
    m.scale(RS_Vector(data.ratio, 1.0));
    return m.angle();
}



void RS_Ellipse::scale(RS_Vector center, RS_Vector factor) {
    data.center.scale(center, factor);
    data.majorP.scale(factor);
    //calculateEndpoints();
    calculateBorders();
}


/**
 * @todo deal with angles correctly
 */
void RS_Ellipse::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Vector mp = data.center + data.majorP;

    data.center.mirror(axisPoint1, axisPoint2);
    mp.mirror(axisPoint1, axisPoint2);

    data.majorP = mp - data.center;

    double a = axisPoint1.angleTo(axisPoint2);

    RS_Vector vec;
    vec.setPolar(1.0, data.angle1);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle1 = vec.angle() - 2*a;

    vec.setPolar(1.0, data.angle2);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    data.angle2 = vec.angle() - 2*a;

    data.reversed = (!data.reversed);

    //calculateEndpoints();
    calculateBorders();
}



void RS_Ellipse::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    RS_Vector startpoint = getStartpoint();
    RS_Vector endpoint = getEndpoint();

    if (ref.distanceTo(startpoint)<1.0e-4) {
        moveStartpoint(startpoint+offset);
    }
    if (ref.distanceTo(endpoint)<1.0e-4) {
        moveEndpoint(endpoint+offset);
    }
}


void RS_Ellipse::draw(RS_Painter* painter, RS_GraphicView* view, double /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }


    if (getPen().getLineType()==RS2::SolidLine ||
            isSelected() ||
            view->getDrawingMode()==RS2::ModePreview) {

        painter->drawEllipse(view->toGui(getCenter()),
                             getMajorRadius() * view->getFactor().x,
                             getMinorRadius() * view->getFactor().x,
                             getAngle(),
                             getAngle1(), getAngle2(),
                             isReversed());
    } else {
        double styleFactor = getStyleFactor(view);
        if (styleFactor<0.0) {
            painter->drawEllipse(view->toGui(getCenter()),
                                 getMajorRadius() * view->getFactor().x,
                                 getMinorRadius() * view->getFactor().x,
                                 getAngle(),
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

        // Pen to draw pattern is always solid:
        RS_Pen pen = painter->getPen();
        pen.setLineType(RS2::SolidLine);
        painter->setPen(pen);

        double* da;     // array of distances in x.
        int i;          // index counter

        double length = getAngleLength();

        // create pattern:
        da = new double[pat->num];

        double tot=0.0;
        i=0;
        bool done = false;
        double curA = getAngle1();
        double curR;
        RS_Vector cp = view->toGui(getCenter());
        double r1 = getMajorRadius() * view->getFactor().x;
        double r2 = getMinorRadius() * view->getFactor().x;

        do {
            curR = sqrt(RS_Math::pow(getMinorRadius()*cos(curA), 2.0)
                        + RS_Math::pow(getMajorRadius()*sin(curA), 2.0));

            if (curR>1.0e-6) {
                da[i] = fabs(pat->pattern[i] * styleFactor) / curR;
                if (pat->pattern[i] * styleFactor > 0.0) {

                    if (tot+fabs(da[i])<length) {
                        painter->drawEllipse(cp,
                                             r1, r2,
                                             getAngle(),
                                             curA,
                                             curA + da[i],
                                             false);
                    } else {
                        painter->drawEllipse(cp,
                                             r1, r2,
                                             getAngle(),
                                             curA,
                                             getAngle2(),
                                             false);
                    }
                }
            }
            curA+=da[i];
            tot+=fabs(da[i]);
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
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Ellipse& a) {
    os << " Ellipse: " << a.data << "\n";
    return os;
}

