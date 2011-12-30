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


#include "rs_line.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"
#include "rs_linetypepattern.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/**
 * Constructor.
 */
RS_Line::RS_Line(RS_EntityContainer* parent,
                 const RS_LineData& d)
    :RS_AtomicEntity(parent), data(d) {
    calculateBorders();
}

////construct a line from two endpoints, to be used for construction
RS_Line::RS_Line(const RS_Vector& pStart, const RS_Vector& pEnd)
    :RS_AtomicEntity(NULL), data(pStart,pEnd) {
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
                                      double* dist)const {
    double dist1((data.startpoint-coord).squared());
    double dist2((data.endpoint-coord).squared());

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = sqrt(dist2);
        }
        return data.endpoint;
    } else {
        if (dist!=NULL) {
            *dist = sqrt(dist1);
        }
        return data.startpoint;
    }

}



RS_Vector RS_Line::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity)const {

    if (entity!=NULL) {
        *entity = const_cast<RS_Line*>(this);
    }
//std::cout<<"RS_Line::getNearestPointOnEntity():"<<coord<<std::endl;
    RS_Vector direction = data.endpoint-data.startpoint;
    RS_Vector vpc=coord-data.startpoint;
    double a=direction.squared();
    if( a < RS_TOLERANCE*RS_TOLERANCE) {
        //line too short
        vpc=getMiddlePoint();
    }else{
        //find projection on line
        vpc = data.startpoint + direction*RS_Vector::dotP(vpc,direction)/a;
        if( onEntity &&
                ! vpc.isInWindowOrdered(minV,maxV) ){
//                !( vpc.x>= minV.x && vpc.x <= maxV.x && vpc.y>= minV.y && vpc.y<=maxV.y) ) {
            //projection point not within range, find the nearest endpoint
//            std::cout<<"not within window, returning endpoints\n";
            return getNearestEndpoint(coord,dist);
        }
    }

    if (dist!=NULL) {
        *dist = vpc.distanceTo(coord);
    }
    return vpc;
}

    RS_Vector RS_Line::getMiddlePoint()const
{
        return (getStartpoint() + getEndpoint())*0.5;
}
    /** @return the nearest of equidistant middle points of the line. */
    RS_Vector RS_Line::getNearestMiddle(const RS_Vector& coord,
                                        double* dist,
                                        int middlePoints
                                        )const {
//        RS_DEBUG->print("RS_Line::getNearestMiddle(): begin\n");
        RS_Vector dvp(getEndpoint() - getStartpoint());
        double l=dvp.magnitude();
        if( l<= RS_TOLERANCE) {
            //line too short
            return const_cast<RS_Line*>(this)->getNearestCenter(coord, dist);
        }
        RS_Vector vp0(getNearestPointOnEntity(coord,true,dist));
        int counts=middlePoints+1;
        int i( static_cast<int>(vp0.distanceTo(getStartpoint())/l*counts+0.5));
        if(!i) i++; // remove end points
        if(i==counts) i--;
        vp0=getStartpoint() + dvp*(double(i)/double(counts));

        if(dist != NULL) {
            *dist=vp0.distanceTo(coord);
        }
//        RS_DEBUG->print("RS_Line::getNearestMiddle(): end\n");
        return vp0;
    }


RS_Vector RS_Line::getNearestCenter(const RS_Vector& coord,
                                    double* dist) {

    RS_Vector p = (data.startpoint + data.endpoint)*0.5;

    if (dist!=NULL) {
        *dist = p.distanceTo(coord);
    }

    return p;
}


RS_Vector RS_Line::getNearestDist(double distance,
                                  const RS_Vector& coord,
                                  double* dist) {

    RS_Vector dv;
    dv.setPolar(distance, getAngle1());

    RS_Vector ret;
    //if(coord.distanceTo(getStartpoint()) < coord.distanceTo(getEndpoint())) {
    if( (coord-getStartpoint()).squared()<  (coord-getEndpoint()).squared() ) {
        ret = getStartpoint() + dv;
    }else{
        ret = getEndpoint() - dv;
    }
    if(dist != NULL) {
        *dist=coord.distanceTo(ret);
    }

    return ret;
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
                                   double /*solidDist*/)const {

//    RS_DEBUG->print("RS_Line::getDistanceToPoint");

    if (entity!=NULL) {
        *entity = const_cast<RS_Line*>(this);
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

RS_Vector  RS_Line::getTangentDirection(const RS_Vector& point)const{
        return getEndpoint() - getStartpoint();
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
    std::swap(data.startpoint, data.endpoint);
}



bool RS_Line::hasEndpointsWithinWindow(const RS_Vector& firstCorner, const RS_Vector& secondCorner) {
    RS_Vector vLow( std::min(firstCorner.x, secondCorner.x), std::min(firstCorner.y, secondCorner.y));
    RS_Vector vHigh( std::max(firstCorner.x, secondCorner.x), std::max(firstCorner.y, secondCorner.y));

    return data.startpoint.isInWindowOrdered(vLow, vHigh)
            || data.endpoint.isInWindowOrdered(vLow, vHigh);

}

/**
  * this function creates offset
  *@coord, position indicates the direction of offset
  *@distance, distance of offset
  * return true, if success, otherwise, false
  *
  *Author: Dongxu Li
  */
bool RS_Line::offset(const RS_Vector& coord, const double& distance) {
    RS_Vector direction(getEndpoint()-getStartpoint());
    double ds(direction.magnitude());
    if(ds< RS_TOLERANCE) return false;
    direction /= ds;
    RS_Vector vp(coord-getStartpoint());
    RS_Vector vp1(getStartpoint() + direction*(RS_Vector::dotP(direction,vp))); //projection
    direction.set(-direction.y,direction.x); //rotate pi/2
    if(RS_Vector::dotP(direction,vp)<0.) {
        direction *= -1.;
    }
    direction*=distance;
    move(direction);
    moveBorders(direction);
    return true;
}

/**
  * revert the direction of line
  */
void RS_Line::revertDirection(){
    std::swap(data.startpoint,data.endpoint);
}

void RS_Line::move(const RS_Vector& offset) {
//    RS_DEBUG->print("RS_Line::move1: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);

//    RS_DEBUG->print("RS_Line::move1: offset: %f/%f", offset.x, offset.y);

    data.startpoint.move(offset);
    data.endpoint.move(offset);
    minV += offset;
    maxV += offset;
//    RS_DEBUG->print("RS_Line::move2: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
}

void RS_Line::rotate(const double& angle) {
//    RS_DEBUG->print("RS_Line::rotate");
//    RS_DEBUG->print("RS_Line::rotate1: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    RS_Vector rvp(angle);
    data.startpoint.rotate(rvp);
    data.endpoint.rotate(rvp);
//    RS_DEBUG->print("RS_Line::rotate2: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
//    RS_DEBUG->print("RS_Line::rotate: OK");
}



void RS_Line::rotate(const RS_Vector& center, const double& angle) {
//    RS_DEBUG->print("RS_Line::rotate");
//    RS_DEBUG->print("RS_Line::rotate1: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    RS_Vector rvp(angle);
    data.startpoint.rotate(center, rvp);
    data.endpoint.rotate(center, rvp);
//    RS_DEBUG->print("RS_Line::rotate2: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
//    RS_DEBUG->print("RS_Line::rotate: OK");
}

void RS_Line::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.startpoint.rotate(center, angleVector);
    data.endpoint.rotate(center, angleVector);
    calculateBorders();
}

/*scale the line around origin (0,0)
  *
  */
void RS_Line::scale(const RS_Vector& factor) {
//    RS_DEBUG->print("RS_Line::scale1: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    data.startpoint.scale(factor);
    data.endpoint.scale(factor);
//    RS_DEBUG->print("RS_Line::scale2: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
}


void RS_Line::scale(const RS_Vector& center, const RS_Vector& factor) {
//    RS_DEBUG->print("RS_Line::scale1: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    data.startpoint.scale(center, factor);
    data.endpoint.scale(center, factor);
//    RS_DEBUG->print("RS_Line::scale2: sp: %f/%f, ep: %f/%f",
//                    data.startpoint.x, data.startpoint.y,
//                    data.endpoint.x, data.endpoint.y);
    calculateBorders();
}



void RS_Line::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.startpoint.mirror(axisPoint1, axisPoint2);
    data.endpoint.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}


/**
 * Stretches the given range of the entity by the given offset.
 */
void RS_Line::stretch(const RS_Vector& firstCorner,
                      const RS_Vector& secondCorner,
                      const RS_Vector& offset) {

    RS_Vector vLow( std::min(firstCorner.x, secondCorner.x), std::min(firstCorner.y, secondCorner.y));
    RS_Vector vHigh( std::max(firstCorner.x, secondCorner.x), std::max(firstCorner.y, secondCorner.y));

    if (getStartpoint().isInWindowOrdered(vLow, vHigh)) {
        moveStartpoint(getStartpoint() + offset);
    }
    if (getEndpoint().isInWindowOrdered(vLow, vHigh)) {
        moveEndpoint(getEndpoint() + offset);
    }
}



void RS_Line::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if(  fabs(data.startpoint.x -ref.x)<1.0e-4 &&
         fabs(data.startpoint.y -ref.y)<1.0e-4 ) {
        moveStartpoint(data.startpoint+offset);
    }
    if(  fabs(data.endpoint.x -ref.x)<1.0e-4 &&
         fabs(data.endpoint.y -ref.y)<1.0e-4 ) {
        moveEndpoint(data.endpoint+offset);
    }
}




void RS_Line::draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) {
    if (painter==NULL || view==NULL) {
        return;
    }
    RS_Vector pStart(view->toGui(getStartpoint()));
    RS_Vector pEnd(view->toGui(getEndpoint()));
    //    std::cout<<"draw line: "<<pStart<<" to "<<pEnd<<std::endl;
    RS_Vector direction=pEnd-pStart;
    double  length=direction.magnitude();
    patternOffset -= length;
    if (( !isSelected() && (
              getPen().getLineType()==RS2::SolidLine ||
              view->getDrawingMode()==RS2::ModePreview)) ) {
        //if length is too small, attempt to draw the line, could be a potential bug
        painter->drawLine(pStart,pEnd);
        return;
    }
    //    double styleFactor = getStyleFactor(view);


    // Pattern:
    RS_LineTypePattern* pat;
    if (isSelected()) {
//        styleFactor=1.;
        pat = &patternSelected;
    } else {
        pat = view->getPattern(getPen().getLineType());
    }
    if (pat==NULL) {
//        patternOffset -= length;
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Line::draw: Invalid line pattern");
        painter->drawLine(pStart,pEnd);
        return;
    }
//    patternOffset = remainder(patternOffset - length-0.5*pat->totalLength,pat->totalLength)+0.5*pat->totalLength;
    if(length<=RS_TOLERANCE){
        painter->drawLine(pStart,pEnd);
        return; //avoid division by zero
    }
    direction/=length; //cos(angle), sin(angle)
    // Pen to draw pattern is always solid:
    RS_Pen pen = painter->getPen();

    pen.setLineType(RS2::SolidLine);
    painter->setPen(pen);

    // index counter
    int i;

    // pattern segment length:
    double patternSegmentLength = pat->totalLength;

    // create pattern:
    RS_Vector* dp=new RS_Vector[pat->num > 0?pat->num:0];
    double* ds=new double[pat->num > 0?pat->num:0];
    if (pat->num >0 ){
        for (i=0; i<pat->num; ++i) {
            //        ds[j]=pat->pattern[i] * styleFactor;
            //fixme, styleFactor support needed
            ds[i]=pat->pattern[i] ;
            dp[i] = direction*fabs(ds[i]);
        }
    }else {
        delete[] dp;
        delete[] ds;
        RS_DEBUG->print(RS_Debug::D_WARNING,"invalid line pattern for line, draw solid line instread");
        painter->drawLine(view->toGui(getStartpoint()),
                          view->toGui(getEndpoint()));
        return;
    }
    double total= remainder(patternOffset-0.5*patternSegmentLength,patternSegmentLength) -0.5*patternSegmentLength;
    //    double total= patternOffset-patternSegmentLength;

    RS_Vector p1,p2,p3;
    RS_Vector curP(pStart+direction*total);
    double t2;
    for(int j=0;total<length;j=(j+1)%i) {

        // line segment (otherwise space segment)
        t2=total+fabs(ds[j]);
        p3=curP+dp[j];
        if (ds[j]>0.0 && t2 > 0.0) {
            // drop the whole pattern segment line, for ds[i]<0:
            // trim end points of pattern segment line to line
            p1 =(total > -0.5)? curP:pStart;
            p2 =(t2<length+0.5)?p3:pEnd;
            painter->drawLine(p1,p2);
        }
        total=t2;
        curP=p3;
    }
    delete[] dp;
    delete[] ds;

}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Line& l) {
    os << " Line: " << l.getData() << "\n";
    return os;
}


