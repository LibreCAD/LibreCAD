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
        RS_Vector vra=p2 - p1;
        RS_Vector vrb=p3 - p1;
        double ra2=vra.squared()*0.5;
        double rb2=vrb.squared()*0.5;
        double crossp=vra.x * vrb.y - vra.y * vrb.x;
        if (fabs(crossp)< RS_TOLERANCE*RS_TOLERANCE) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Arc::createFrom3P(): "
                        "Cannot create a arc with radius 0.0.");
                return false;
        }
        crossp=1./crossp;
        data.center.set((ra2*vrb.y - rb2*vra.y)*crossp,(rb2*vra.x - ra2*vrb.x)*crossp);
        data.radius=data.center.magnitude();
        data.center += p1;
        data.angle1=data.center.angleTo(p1);
        data.angle2=data.center.angleTo(p3);
        data.reversed = RS_Math::isAngleBetween(data.center.angleTo(p2),
                                                data.angle1, data.angle2, true);
        return true;
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
 * Creates an arc from its startpoint, endpoint, start direction (angle)
 * and angle length.
 *
 * @retval true Successfully created arc
 * @retval false Cannot creats arc (radius to small or endpoint to far away)
 */
bool RS_Arc::createFrom2PDirectionAngle(const RS_Vector& startPoint,
                                        const RS_Vector& endPoint,
                                        double direction1, double angleLength) {
    if( fabs(remainder( angleLength, M_PI))<RS_TOLERANCE_ANGLE ) return false;
    data.radius=0.5*startPoint.distanceTo(endPoint)/sin(0.5*angleLength);

    RS_Vector ortho;
    ortho.setPolar(data.radius, direction1 + M_PI/2.0);
    RS_Vector center1 = startPoint + ortho;
    RS_Vector center2 = startPoint - ortho;

    if (center1.distanceTo(endPoint) < center2.distanceTo(endPoint)) {
        data.center = center1;
    } else {
        data.center = center2;
    }

    data.angle1 = data.center.angleTo(startPoint);
    data.reversed = false;

    double diff = RS_Math::correctAngle(getDirection1()-direction1);
    if (fabs(diff-M_PI)<1.0e-1) {
    data.angle2 = RS_Math::correctAngle(data.angle1 -angleLength);
        data.reversed = true;
    }else{
    data.angle2 = RS_Math::correctAngle(data.angle1 +angleLength);
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

    double a1 = isReversed() ? data.angle2 : data.angle1;
    double a2 = isReversed() ? data.angle1 : data.angle2;
    if ( RS_Math::isAngleBetween(0.5*M_PI,a1,a2,false) ) {
        maxY = data.center.y + data.radius;
    }
    if ( RS_Math::isAngleBetween(1.5*M_PI,a1,a2,false) ) {
        minY = data.center.y - data.radius;
    }
    if ( RS_Math::isAngleBetween(M_PI,a1,a2,false) ) {
        minX = data.center.x - data.radius;
    }
    if ( RS_Math::isAngleBetween(0.,a1,a2,false) ) {
        maxX = data.center.x + data.radius;
    }

    minV.set(minX, minY);
    maxV.set(maxX, maxY);
}



RS_VectorSolutions RS_Arc::getRefPoints() {
    RS_VectorSolutions ret(startpoint, endpoint, data.center);
    return ret;
}


RS_Vector RS_Arc::getNearestEndpoint(const RS_Vector& coord, double* dist) const{
    double dist1, dist2;

    dist1 = (startpoint-coord).squared();
    dist2 = (endpoint-coord).squared();

    if (dist2<dist1) {
        if (dist!=NULL) {
            *dist = sqrt(dist2);
        }
         return endpoint;
    } else {
        if (dist!=NULL) {
            *dist = sqrt(dist1);
        }
        return startpoint;
    }

}



RS_Vector RS_Arc::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity)const {

    RS_Vector vec(false);
    if (entity!=NULL) {
        *entity = const_cast<RS_Arc*>(this);
    }

    double angle = (coord-data.center).angle();
    if ( ! onEntity || RS_Math::isAngleBetween(angle,
            data.angle1, data.angle2, isReversed())) {
        vec.setPolar(data.radius, angle);
        vec+=data.center;
    } else {
            vec=getNearestEndpoint(coord, dist);
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

/*
 * get the nearest equidistant middle points
 * @coord, coordinate
 * @middlePoints, number of equidistant middle points
 *
 */

RS_Vector RS_Arc::getNearestMiddle(const RS_Vector& coord,
                                   double* dist,
                                   int middlePoints
                                   )const {
    RS_DEBUG->print("RS_Arc::getNearestMiddle(): begin\n");
        double amin=getAngle1();
        double amax=getAngle2();
        //std::cout<<"RS_Arc::getNearestMiddle(): middlePoints="<<middlePoints<<std::endl;
        if( !(std::isnormal(amin) || std::isnormal(amax))){
                //whole circle, no middle point
                if(dist != NULL) {
                        *dist=RS_MAXDOUBLE;
                }
                return RS_Vector(false);
        }
        if(isReversed()) {
                std::swap(amin,amax);
        }
        double da=fmod(amax-amin+2.*M_PI, 2.*M_PI);
        if ( da < RS_TOLERANCE ) {
                da= 2.*M_PI; // whole circle
        }
        RS_Vector vp(getNearestPointOnEntity(coord,true,dist));
        double angle=getCenter().angleTo(vp);
        int counts=middlePoints+1;
        int i( static_cast<int>(fmod(angle-amin+2.*M_PI,2.*M_PI)/da*counts+0.5));
        if(!i) i++; // remove end points
        if(i==counts) i--;
        angle=amin + da*(double(i)/double(counts));
        vp.setPolar(getRadius(), angle);
        vp.move(getCenter());

    if (dist!=NULL) {
        *dist = vp.distanceTo(coord);
    }
    RS_DEBUG->print("RS_Arc::getNearestMiddle(): end\n");
    return vp;
}

RS_Vector RS_Arc::getNearestDist(double distance,
                                 const RS_Vector& coord,
                                 double* dist) {

    if (data.radius<RS_TOLERANCE) {
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

    if (data.radius<RS_TOLERANCE) {
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


RS_Vector RS_Arc::getNearestOrthTan(const RS_Vector& coord,
                    const RS_Line& normal,
                    bool onEntity )
{
        if ( !coord.valid ) {
                return RS_Vector(false);
        }
        double angle=normal.getAngle1();
        RS_Vector vp;
        vp.setPolar(getRadius(),angle);
        QList<RS_Vector> sol;
        for(int i=0;i <= 1;i++){
                if(!onEntity ||
                   RS_Math::isAngleBetween(angle,getAngle1(),getAngle2(),isReversed())) {
                if(i){
                sol.append(- vp);
                }else {
                sol.append(vp);
                }
        }
                angle=RS_Math::correctAngle(angle+M_PI);
        }
        switch(sol.count()) {
                case 0:
                        return RS_Vector(false);
                case 2:
                        if( RS_Vector::dotP(sol[1],coord-getCenter())>0.) {
                                vp=sol[1];
                                break;
                        }
                default:
                        vp=sol[0];
        }
        return getCenter()+vp;
}


double RS_Arc::getDistanceToPoint(const RS_Vector& coord,
                                  RS_Entity** entity,
                                  RS2::ResolveLevel,
                                  double) const {
    if (entity!=NULL) {
        *entity = const_cast<RS_Arc*>(this);
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
        double dToEdge=fabs((coord-data.center).magnitude() - data.radius);
        double dToCenter=data.center.distanceTo(coord);

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
    correctAngles(); // make sure angleLength is no more than 2*M_PI
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
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    //}

    // normal arc: move angle1
    /*else {
        data.angle2 = data.center.angleTo(pos);
        calculateEndpoints();
        calculateBorders();
    }*/
}


/**
 * make sure angleLength() is not more than 2*M_PI
 */
void RS_Arc::correctAngles() {
        double *pa1= & data.angle1;
        double *pa2= & data.angle2;
        if (isReversed()) std::swap(pa1,pa2);
        *pa2 = *pa1 + fmod(*pa2 - *pa1, 2.*M_PI);
        if ( fabs(getAngleLength()) < RS_TOLERANCE_ANGLE ) *pa2 += 2.*M_PI;
}

void RS_Arc::trimStartpoint(const RS_Vector& pos) {
    data.angle1 = data.center.angleTo(pos);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateEndpoints();
    calculateBorders();
}



void RS_Arc::trimEndpoint(const RS_Vector& pos) {
    data.angle2 = data.center.angleTo(pos);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateEndpoints();
    calculateBorders();
}


RS2::Ending RS_Arc::getTrimPoint(const RS_Vector& trimCoord,
                                 const RS_Vector& /*trimPoint*/) {

    //double angEl = data.center.angleTo(trimPoint);
    double angM = data.center.angleTo(trimCoord);
    if (RS_Math::getAngleDifference(angM, data.angle1) > RS_Math::getAngleDifference(data.angle2,angM)) {
        return RS2::EndingStart;
    } else {
        return RS2::EndingEnd;
    }
}

RS_Vector RS_Arc::prepareTrim(const RS_Vector& trimCoord,
                              const RS_VectorSolutions& trimSol) {
//special trimming for ellipse arc
    if( ! trimSol.hasValid() ) return (RS_Vector(false));
    if( trimSol.getNumber() == 1 ) return (trimSol.get(0));
    double am=data.center.angleTo(trimCoord);
    QList<double> ias;
    double ia(0.),ia2(0.);
    RS_Vector is,is2;
    for(int ii=0; ii<trimSol.getNumber(); ii++) { //find closest according to arc angle
        ias.append(data.center.angleTo(trimSol.get(ii)));
        //std::cout<<"( "<<ias[ii]<<" ) ";
        if( !ii ||  fabs( remainder( ias[ii] - am, 2*M_PI)) < fabs( remainder( ia -am, 2*M_PI)) ) {
            ia = ias[ii];
            is = trimSol.get(ii);
        }
    }
    //std::cout<<std::endl;
    qSort(ias.begin(),ias.end());
    for(int ii=0; ii<trimSol.getNumber(); ii++) { //find segment to enclude trimCoord
        if ( ! RS_Math::isSameDirection(ia,ias[ii],RS_TOLERANCE)) continue;
        if( RS_Math::isAngleBetween(am,ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()],ia,isReversed()))  {
            ia2=ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()];
        } else {
            ia2=ias[(ii+1)% trimSol.getNumber()];
        }
        break;
    }
    if( RS_Math::isSameDirection(ia2,ia,RS_TOLERANCE) ) {
        is2=is;
    } else {
        for(int ii=0; ii<trimSol.getNumber(); ii++) { //find segment to enclude trimCoord
            if ( ! RS_Math::isSameDirection(ia2,data.center.angleTo(trimSol.get(ii)),RS_TOLERANCE) ) continue;
            is2=trimSol.get(ii);
            break;
        }
    }
    if(RS_Math::isSameDirection(getAngle1(),getAngle2(),RS_TOLERANCE_ANGLE)) {
        //whole circle
        if( RS_Math::isAngleBetween(am,ia,ia2,isReversed()) ) {
            setAngle1(ia);
            setAngle2(ia2);
        } else {
            setAngle2(ia);
            setAngle1(ia2);
        }
    } else {
        double dia=fabs(remainder(ia-am,2*M_PI));
        double dia2=fabs(remainder(ia2-am,2*M_PI));
        double ai_min=std::min(dia,dia2);
        double da1=fabs(remainder(getAngle1()-am,2*M_PI));
        double da2=fabs(remainder(getAngle2()-am,2*M_PI));
        double da_min=std::min(da1,da2);
        if( da_min < ai_min ) {
            //trimming one end of arc
            bool irev= RS_Math::isAngleBetween(am,ia2,ia, isReversed()) ;
            //std::cout<<"angle1="<<getAngle1()<<" angle2="<<getAngle2()<<" am="<< am<<" ia="<<ia<<" ia2="<<ia2<<" irev="<<irev<<std::endl;
            if ( RS_Math::isAngleBetween(ia,getAngle1(),getAngle2(), isReversed()) &&
                    RS_Math::isAngleBetween(ia2,getAngle1(),getAngle2(), isReversed()) ) { //
                if(irev) {
                    setAngle2(ia);
                    setAngle1(ia2);
                } else {
                    setAngle1(ia);
                    setAngle2(ia2);
                }
                da1=fabs(remainder(getAngle1()-am,2*M_PI));
                da2=fabs(remainder(getAngle2()-am,2*M_PI));
            }
            if( ((da1 < da2) && (RS_Math::isAngleBetween(ia2,ia,getAngle1(),isReversed()))) ||
                    ((da1 > da2) && (RS_Math::isAngleBetween(ia2,getAngle2(),ia,isReversed())))
              ) {
                RS_Math::swap(is,is2);
                // std::cout<<"reset: angle1="<<getAngle1()<<" angle2="<<getAngle2()<<" am="<< am<<" is="<<data.center.angleTo(is)<<" ia2="<<ia2<<std::endl;
            }
        } else {
            //choose intersection as new end
            if( dia > dia2) {
                RS_Math::swap(is,is2);
                RS_Math::swap(ia,ia2);
            }
            if(RS_Math::isAngleBetween(ia,getAngle1(),getAngle2(),isReversed())) {
                if(RS_Math::isAngleBetween(am,getAngle1(),ia,isReversed())) {
                    setAngle2(ia);
                } else {
                    setAngle1(ia);
                }
            }
        }
    }
    return is;
}



void RS_Arc::reverse() {
    std::swap(data.angle1,data.angle2);
    data.reversed = !data.reversed;
//    calculateEndpoints();
    std::swap(startpoint,endpoint);
    //reversing the order of start/end doesn't change position
//    calculateBorders();
}


void RS_Arc::move(const RS_Vector& offset) {
    data.center.move(offset);
    startpoint.move(offset);
    endpoint.move(offset);
    moveBorders(offset);
}



void RS_Arc::rotate(const RS_Vector& center, const double& angle) {
    RS_DEBUG->print("RS_Arc::rotate");
    data.center.rotate(center, angle);
    data.angle1 = RS_Math::correctAngle(data.angle1+angle);
    data.angle2 = RS_Math::correctAngle(data.angle2+angle);
    calculateEndpoints();
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}

void RS_Arc::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_DEBUG->print("RS_Arc::rotate");
    data.center.rotate(center, angleVector);
    double angle(angleVector.angle());
    data.angle1 = RS_Math::correctAngle(data.angle1+angle);
    data.angle2 = RS_Math::correctAngle(data.angle2+angle);
    calculateEndpoints();
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}



void RS_Arc::scale(const RS_Vector& center, const RS_Vector& factor) {
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
    data.radius = fabs( data.radius );
//    calculateEndpoints();
    //todo, does this handle negative factors properly?
    startpoint.scale(center,factor);
    endpoint.scale(center,factor);
    scaleBorders(center,factor);
//    calculateBorders();
}



void RS_Arc::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.center.mirror(axisPoint1, axisPoint2);
    setReversed( ! isReversed() );
    double a= (axisPoint2 - axisPoint1).angle()*2;
    setAngle1(RS_Math::correctAngle(a - getAngle1()));
    setAngle2(RS_Math::correctAngle(a - getAngle2()));
    correctAngles(); // make sure angleLength is no more than 2*M_PI
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
    correctAngles(); // make sure angleLength is no more than 2*M_PI
}



void RS_Arc::stretch(const RS_Vector& firstCorner,
                     const RS_Vector& secondCorner,
                     const RS_Vector& offset) {

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
    correctAngles(); // make sure angleLength is no more than 2*M_PI
}



void RS_Arc::draw(RS_Painter* painter, RS_GraphicView* view,
                  double& /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }
    RS_Vector cp=view->toGui(getCenter());
    double ra=getRadius()*view->getFactor().x;
    //double styleFactor = getStyleFactor();

    // simple style-less lines
    if ( !isSelected() && (
             getPen().getLineType()==RS2::SolidLine ||
             view->getDrawingMode()==RS2::ModePreview)) {

        painter->drawArc(cp,
                         ra,
                         getAngle1(), getAngle2(),
                         isReversed());
        return;
    }
//    double styleFactor = getStyleFactor(view);
    //        if (styleFactor<0.0) {
    //            painter->drawArc(cp,
    //                             ra,
    //                             getAngle1(), getAngle2(),
    //                             isReversed());
    //            return;
    //        }

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

    if (ra<1.){
        painter->drawArc(cp, ra,
                         getAngle1(),getAngle2(),
                         isReversed());
        return;
    }

    // Pen to draw pattern is always solid:
    RS_Pen pen = painter->getPen();
    pen.setLineType(RS2::SolidLine);
    painter->setPen(pen);

    double a1(getAngle1());
    double a2(getAngle2());
    if(isReversed()) std::swap(a1,a2);
    if(a2<a1+RS_TOLERANCE_ANGLE) a2 += 2.*M_PI;


    // create scaled pattern:
    double* da = new double[pat->num];
  // array of distances in x.
//    double k=2.;
//    double patternLength=0.;
    int i(0),j(0);          // index counter
    while(i<pat->num){
//        da[j] = pat->pattern[i++] * styleFactor;
        //fixme, stylefactor needed
        da[j] = pat->pattern[i++]/ra;
//        if(fabs(da[j])<RS_TOLERANCE) continue;
//        if(fabs(da[j]) > RS_TOLERANCE && fabs(da[j])<k) k=fabs(da[j]);
//        patternLength += fabs(da[j]);
        j++;
    }
    if(!j){
        //invalid pattern

        delete[] da;
        std::cout<<"RS_Arc::draw(): invalid pattern\n";
        painter->drawArc(cp,
                         ra,
                         getAngle1(), getAngle2(),
                         isReversed());
        return;
    }
//    k= 2./k;
//    patternLength *=k;
//    if (patternLength>80.) k*=80./patternLength;
//    for(i=0;i<j;i++) {
//        da[i]*=k/ra; //normalize pattern
//    }


//    bool done = false;
    double curA (a1);
    double a3;
    //double cx = getCenter().x * factor.x + offsetX;
    //double cy = - a->getCenter().y * factor.y + getHeight() - offsetY;

    for(i=0;;) {
        a3 = curA + fabs(da[i]);
        if (a3 < a2 ) {
            if (da[i] > 0.0) {
                painter->drawArc(cp, ra,
                                 curA,
                                 a3,
                                 false);
            }
        } else {
            if (da[i] > 0.0) {
                painter->drawArc(cp, ra,
                                 curA,
                                 a2,
                                 false);
            }
            break;
        }
        curA=a3;
        i=(i+1)%j;
    }

    delete[] da;
}



/**
 * @return Middle point of the entity.
 */
RS_Vector RS_Arc::getMiddlePoint() const {
    double a=getAngle1();
    double b=getAngle2();

    if (isReversed()) {
        a =b+ RS_Math::correctAngle(a-b)*0.5;
    }else{
        a += RS_Math::correctAngle(b-a)*0.5;
    }
    RS_Vector ret(a);
    return getCenter() + ret*getRadius();
}



/**
 * @return Angle length in rad.
 */
double RS_Arc::getAngleLength() const {
    double ret;
    double a=getAngle1();
    double b=getAngle2();

    if (isReversed()) std::swap(a,b);
    ret = RS_Math::correctAngle(b-a);
    // full circle:
    if (fabs(remainder(ret,2.*M_PI))<RS_TOLERANCE_ANGLE) {
        ret = 2*M_PI;
    }

    return ret;
}



/**
 * @return Length of the arc.
 */
double RS_Arc::getLength() const {
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

