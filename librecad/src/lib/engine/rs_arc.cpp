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

#include <cmath>
#include "rs_arc.h"

#include "rs_line.h"
#include "rs_constructionline.h"
#include "rs_linetypepattern.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "lc_quadratic.h"
#include "rs_painterqt.h"
#include "rs_debug.h"
#include "lc_rect.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

RS_ArcData::RS_ArcData(const RS_Vector& _center,
					   double _radius,
					   double _angle1, double _angle2,
					   bool _reversed):
	center(_center)
  ,radius(_radius)
  ,angle1(_angle1)
  ,angle2(_angle2)
  ,reversed(_reversed)
{
}

void RS_ArcData::reset() {
	center = RS_Vector(false);
	radius = 0.0;
	angle1 = 0.0;
	angle2 = 0.0;
	reversed = false;
}

bool RS_ArcData::isValid() const{
	return (center.valid && radius>RS_TOLERANCE &&
			fabs(remainder(angle1-angle2, 2.*M_PI))>RS_TOLERANCE_ANGLE);
}

std::ostream& operator << (std::ostream& os, const RS_ArcData& ad) {
	os << "(" << ad.center <<
		  "/" << ad.radius <<
		  " " << ad.angle1 <<
		  "," << ad.angle2 <<
		  ")";
	return os;
}
/**
 * Default constructor.
 */
RS_Arc::RS_Arc(RS_EntityContainer* parent,
               const RS_ArcData& d)
    : RS_AtomicEntity(parent), data(d) {
    calculateBorders();
}

RS_Entity* RS_Arc::clone() const {
	RS_Arc* a = new RS_Arc(*this);
	a->initId();
	return a;
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
        if (fabs(crossp)< RS_TOLERANCE2) {
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
 * @retval false Cannot create arc (radius to small or endpoint to far away)
 */
bool RS_Arc::createFrom2PDirectionRadius(const RS_Vector& startPoint,
        const RS_Vector& endPoint,
        double direction1, double radius) {

	RS_Vector ortho = RS_Vector::polar(radius, direction1 + M_PI_2);
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
    calculateBorders();

    return true;
}

/**
 * Creates an arc from its startpoint, endpoint, start direction (angle)
 * and angle length.
 *
 * @retval true Successfully created arc
 * @retval false Cannot create arc (radius to small or endpoint to far away)
 */
bool RS_Arc::createFrom2PDirectionAngle(const RS_Vector& startPoint,
                                        const RS_Vector& endPoint,
										double direction1, double angleLength) {
	if (angleLength <= RS_TOLERANCE_ANGLE || angleLength > 2. * M_PI - RS_TOLERANCE_ANGLE) return false;
	RS_Line l0 {nullptr, startPoint, startPoint - RS_Vector{direction1}};
	double const halfA = 0.5 * angleLength;
	l0.rotate(startPoint, halfA);

	double d0;
	RS_Vector vEnd0 = l0.getNearestPointOnEntity(endPoint, false, &d0);
	RS_Line l1 = l0;
	l1.rotate(startPoint, -angleLength);
	double d1;
	RS_Vector vEnd1 = l1.getNearestPointOnEntity(endPoint, false, &d1);
	if (d1 < d0) {
		vEnd0 = vEnd1;
		l0 = l1;
	}

	l0.rotate((startPoint + vEnd0) * 0.5, M_PI_2);

	l1 = RS_Line{nullptr, startPoint, startPoint + RS_Vector{direction1 + M_PI_2}};

	auto const sol = RS_Information::getIntersection(&l0, &l1, false);
	if (sol.size()==0) return false;

	data.center = sol.at(0);

	data.radius = data.center.distanceTo(startPoint);
    data.angle1 = data.center.angleTo(startPoint);
    data.reversed = false;

    double diff = RS_Math::correctAngle(getDirection1()-direction1);
    if (fabs(diff-M_PI)<1.0e-1) {
    data.angle2 = RS_Math::correctAngle(data.angle1 -angleLength);
        data.reversed = true;
    }else{
    data.angle2 = RS_Math::correctAngle(data.angle1 +angleLength);
    }
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
		angle+=M_PI_2;
    } else {
		angle-=M_PI_2;
    }

    if (fabs(alpha)>M_PI) {
        h*=-1.0;
    }

    data.center.setPolar(h, angle);
    data.center+=middle;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(endPoint);

    calculateBorders();

    return true;
}

void RS_Arc::calculateBorders() {
	RS_Vector const startpoint = data.center + RS_Vector::polar(data.radius, data.angle1);
	RS_Vector const endpoint = data.center + RS_Vector::polar(data.radius, data.angle2);
	LC_Rect const rect{startpoint, endpoint};

	double minX = rect.lowerLeftCorner().x;
	double minY = rect.lowerLeftCorner().y;
	double maxX = rect.upperRightCorner().x;
	double maxY = rect.upperRightCorner().y;

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


RS_Vector RS_Arc::getStartpoint() const
{
	return data.center + RS_Vector::polar(data.radius, data.angle1);
}

/** @return End point of the entity. */
RS_Vector RS_Arc::getEndpoint() const
{
	return data.center + RS_Vector::polar(data.radius, data.angle2);
}

RS_VectorSolutions RS_Arc::getRefPoints() const
{
	//order: start, end, center
	return {getStartpoint(), getEndpoint(), data.center};
}

double RS_Arc::getDirection1() const {
	if (!data.reversed) {
		return RS_Math::correctAngle(data.angle1+M_PI_2);
	}
	else {
		return RS_Math::correctAngle(data.angle1-M_PI_2);
	}
}
/**
 * @return Direction 2. The angle at which the arc starts at
 * the endpoint.
 */
double RS_Arc::getDirection2() const {
	if (!data.reversed) {
		return RS_Math::correctAngle(data.angle2-M_PI_2);
	}
	else {
		return RS_Math::correctAngle(data.angle2+M_PI_2);
	}
}

RS_Vector RS_Arc::getNearestEndpoint(const RS_Vector& coord, double* dist) const{
    double dist1, dist2;

	auto const startpoint = getStartpoint();
	auto const endpoint = getEndpoint();

	dist1 = coord.squaredTo(startpoint);
	dist2 = coord.squaredTo(endpoint);

    if (dist2<dist1) {
		if (dist)
            *dist = sqrt(dist2);

         return endpoint;
    } else {
		if (dist)
            *dist = sqrt(dist1);

        return startpoint;
    }

}


/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  *Author: Dongxu Li
  */
RS_VectorSolutions RS_Arc::getTangentPoint(const RS_Vector& point) const {
    RS_VectorSolutions ret;
    double r2(getRadius()*getRadius());
    if(r2<RS_TOLERANCE2) return ret; //circle too small
    RS_Vector vp(point-getCenter());
    double c2(vp.squared());
    if(c2<r2-getRadius()*2.*RS_TOLERANCE) {
        //inside point, no tangential point
        return ret;
    }
    if(c2>r2+getRadius()*2.*RS_TOLERANCE) {
        //external point
        RS_Vector vp1(-vp.y,vp.x);
        vp1*=getRadius()*sqrt(c2-r2)/c2;
        vp *= r2/c2;
        vp += getCenter();
        if(vp1.squared()>RS_TOLERANCE2) {
            ret.push_back(vp+vp1);
            ret.push_back(vp-vp1);
            return ret;
        }
    }
    ret.push_back(point);
    return ret;
}

RS_Vector RS_Arc::getTangentDirection(const RS_Vector& point) const {
    RS_Vector vp(point-getCenter());
//    double c2(vp.squared());
//    if(c2<r2-getRadius()*2.*RS_TOLERANCE) {
//        //inside point, no tangential point
//        return RS_Vector(false);
//    }
    return RS_Vector(-vp.y,vp.x);

}

RS_Vector RS_Arc::getNearestPointOnEntity(const RS_Vector& coord,
		bool onEntity, double* dist, RS_Entity** entity) const{

    RS_Vector vec(false);
	if (entity) {
        *entity = const_cast<RS_Arc*>(this);
    }

    double angle = (coord-data.center).angle();
    if ( ! onEntity || RS_Math::isAngleBetween(angle,
            data.angle1, data.angle2, isReversed())) {
        vec.setPolar(data.radius, angle);
        vec+=data.center;
    } else {
            return vec=getNearestEndpoint(coord, dist);
    }
	if (dist) {
        *dist = vec.distanceTo(coord);
//        RS_DEBUG->print(RS_Debug::D_ERROR, "distance to (%g, %g)=%g\n", coord.x,coord.y,*dist);
    }

    return vec;
}



RS_Vector RS_Arc::getNearestCenter(const RS_Vector& coord,
								   double* dist) const{
	if (dist) {
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
#ifndef EMU_C99
    using std::isnormal;
#endif

    RS_DEBUG->print("RS_Arc::getNearestMiddle(): begin\n");
        double amin=getAngle1();
        double amax=getAngle2();
        //std::cout<<"RS_Arc::getNearestMiddle(): middlePoints="<<middlePoints<<std::endl;
		if( !(isnormal(amin) || isnormal(amax))){
                //whole circle, no middle point
				if(dist) {
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

	if (dist) {
        *dist = vp.distanceTo(coord);
    }
    RS_DEBUG->print("RS_Arc::getNearestMiddle(): end\n");
    return vp;
}

RS_Vector RS_Arc::getNearestDist(double distance,
                                 const RS_Vector& coord,
								 double* dist) const{

    if (data.radius<RS_TOLERANCE) {
		if (dist)
            *dist = RS_MAXDOUBLE;

		return {};
    }

    double aDist = distance / data.radius;
    if (isReversed()) aDist= -aDist;
    double a;

	if(coord.distanceTo(getStartpoint()) < coord.distanceTo(getEndpoint()))
        a=getAngle1() + aDist;
	else
        a=getAngle2() - aDist;

	RS_Vector ret = RS_Vector::polar(data.radius, a);
    ret += getCenter();

    return ret;
}




RS_Vector RS_Arc::getNearestDist(double distance,
								 bool startp) const{

    if (data.radius<RS_TOLERANCE) {
		return {};
    }

    double a;
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

	RS_Vector p = RS_Vector::polar(data.radius, a);
    p += data.center;

    return p;
}


RS_Vector RS_Arc::getNearestOrthTan(const RS_Vector& coord,
                    const RS_Line& normal,
					bool onEntity ) const
{
        if ( !coord.valid ) {
                return RS_Vector(false);
        }
        double angle=normal.getAngle1();
		RS_Vector vp = RS_Vector::polar(getRadius(),angle);
		std::vector<RS_Vector> sol;
		for(int i=0; i <= 1; i++){
			if(!onEntity ||
					RS_Math::isAngleBetween(angle,getAngle1(),getAngle2(),isReversed())) {
				if (i)
					sol.push_back(- vp);
				else
					sol.push_back(vp);
			}
			angle=RS_Math::correctAngle(angle+M_PI);
		}
		switch(sol.size()) {
                case 0:
                        return RS_Vector(false);
                case 2:
                        if( RS_Vector::dotP(sol[1],coord-getCenter())>0.) {
                                vp=sol[1];
                                break;
                        }
                        // fall-through
                default:
                        vp=sol[0];
                        break;
        }
        return getCenter()+vp;
}

void RS_Arc::moveStartpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
	//if (parent && parent->rtti()==RS2::EntityPolyline) {
    double bulge = getBulge();
	if(fabs(bulge - M_PI_2)<RS_TOLERANCE_ANGLE) return;

    createFrom2PBulge(pos, getEndpoint(), bulge);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    //}
}



void RS_Arc::moveEndpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
	//if (parent && parent->rtti()==RS2::EntityPolyline) {
    double bulge = getBulge();
    createFrom2PBulge(getStartpoint(), pos, bulge);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    //}
}

/**
  * this function creates offset
  *@coord, position indicates the direction of offset
  *@distance, distance of offset
  * return true, if success, otherwise, false
  *
  *Author: Dongxu Li
  */
bool RS_Arc::offset(const RS_Vector& coord, const double& distance) {
    double r0(coord.distanceTo(getCenter()));
    if(r0 > getRadius()){
        //external
        r0 = getRadius()+ fabs(distance);
    }else{
        r0 = getRadius()- fabs(distance);
        if(r0<RS_TOLERANCE) {
            return false;
        }
    }
    setRadius(r0);
    calculateBorders();
    return true;
}
std::vector<RS_Entity* > RS_Arc::offsetTwoSides(const double& distance) const
{
	std::vector<RS_Entity*> ret(0,nullptr);
	ret.push_back(new RS_Arc(nullptr,RS_ArcData(getCenter(),getRadius()+distance,getAngle1(),getAngle2(),isReversed())));
    if(getRadius()>distance)
	ret.push_back(new RS_Arc(nullptr,RS_ArcData(getCenter(),getRadius()-distance,getAngle1(),getAngle2(),isReversed())));
    return ret;
}

/**
      * implementations must revert the direction of an atomic entity
      */
void RS_Arc::revertDirection(){
    std::swap(data.angle1,data.angle2);
    data.reversed = ! data.reversed;
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
    calculateBorders();
}



void RS_Arc::trimEndpoint(const RS_Vector& pos) {
    data.angle2 = data.center.angleTo(pos);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

/**
  *@ trimCoord, mouse point
  *@  trimPoint, trim to this intersection point
  */
RS2::Ending RS_Arc::getTrimPoint(const RS_Vector& trimCoord,
                                 const RS_Vector& /*trimPoint*/) {

    //double angEl = data.center.angleTo(trimPoint);
    double angMouse = data.center.angleTo(trimCoord);
//    double angTrim = data.center.angleTo(trimPoint);
    if( fabs(remainder(angMouse-data.angle1, 2.*M_PI))< fabs(remainder(angMouse-data.angle2, 2.*M_PI)))
        return RS2::EndingStart;
    else
        return RS2::EndingEnd;

//    if( RS_Math::isAngleBetween(angMouse , data.angle1, angTrim, isReversed())) {

//        return RS2::EndingEnd;
//    } else {

//        return RS2::EndingStart;
//    }
}

RS_Vector RS_Arc::prepareTrim(const RS_Vector& trimCoord,
                              const RS_VectorSolutions& trimSol) {
    //special trimming for ellipse arc
            RS_DEBUG->print("RS_Ellipse::prepareTrim()");
        if( ! trimSol.hasValid() ) return (RS_Vector(false));
        if( trimSol.getNumber() == 1 ) return (trimSol.get(0));
        double am=getArcAngle(trimCoord);
		std::vector<double> ias;
        double ia(0.),ia2(0.);
        RS_Vector is,is2;
		for(size_t ii=0; ii<trimSol.getNumber(); ++ii) { //find closest according ellipse angle
			ias.push_back(getArcAngle(trimSol.get(ii)));
            if( !ii ||  fabs( remainder( ias[ii] - am, 2*M_PI)) < fabs( remainder( ia -am, 2*M_PI)) ) {
                ia = ias[ii];
                is = trimSol.get(ii);
            }
        }
        std::sort(ias.begin(),ias.end());
		for(size_t ii=0; ii<trimSol.getNumber(); ++ii) { //find segment to enclude trimCoord
            if ( ! RS_Math::isSameDirection(ia,ias[ii],RS_TOLERANCE)) continue;
            if( RS_Math::isAngleBetween(am,ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()],ia,false))  {
                ia2=ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()];
            } else {
                ia2=ias[(ii+1)% trimSol.getNumber()];
            }
            break;
        }
		for(const RS_Vector& vp: trimSol) { //find segment to enclude trimCoord
			if ( ! RS_Math::isSameDirection(ia2,getArcAngle(vp),RS_TOLERANCE)) continue;
			is2=vp;
            break;
        }
//        if(RS_Math::isSameDirection(getAngle1(),getAngle2(),RS_TOLERANCE_ANGLE)
//                ||  RS_Math::isSameDirection(ia2,ia,RS_TOLERANCE) ) {
//            //whole ellipse
//            if( !RS_Math::isAngleBetween(am,ia,ia2,isReversed())) {
//                std::swap(ia,ia2);
//                std::swap(is,is2);
//            }
//            setAngle1(ia);
//            setAngle2(ia2);
//            double da1=fabs(remainder(getAngle1()-am,2*M_PI));
//            double da2=fabs(remainder(getAngle2()-am,2*M_PI));
//            if(da2<da1) {
//                std::swap(is,is2);
//            }

//        } else {
            double dia=fabs(remainder(ia-am,2*M_PI));
            double dia2=fabs(remainder(ia2-am,2*M_PI));
            double ai_min=std::min(dia,dia2);
            double da1=fabs(remainder(getAngle1()-am,2*M_PI));
            double da2=fabs(remainder(getAngle2()-am,2*M_PI));
            double da_min=std::min(da1,da2);
            if( da_min < ai_min ) {
                //trimming one end of arc
                bool irev= RS_Math::isAngleBetween(am,ia2,ia, isReversed()) ;
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
                    std::swap(is,is2);
                    //std::cout<<"reset: angle1="<<getAngle1()<<" angle2="<<getAngle2()<<" am="<< am<<" is="<<getArcAngle(is)<<" ia2="<<ia2<<std::endl;
                }
            } else {
                //choose intersection as new end
                if( dia > dia2) {
                    std::swap(is,is2);
                    std::swap(ia,ia2);
                }
                if(RS_Math::isAngleBetween(ia,getAngle1(),getAngle2(),isReversed())) {
                    if(RS_Math::isAngleBetween(am,getAngle1(),ia,isReversed())) {
                        setAngle2(ia);
                    } else {
                        setAngle1(ia);
                    }
                }
            }
//        }
        return is;
}



void RS_Arc::reverse() {
    std::swap(data.angle1,data.angle2);
    data.reversed = !data.reversed;
//    calculateBorders();
}


void RS_Arc::move(const RS_Vector& offset) {
    data.center.move(offset);
    moveBorders(offset);
}



void RS_Arc::rotate(const RS_Vector& center, const double& angle) {
    RS_DEBUG->print("RS_Arc::rotate");
    data.center.rotate(center, angle);
    data.angle1 = RS_Math::correctAngle(data.angle1+angle);
    data.angle2 = RS_Math::correctAngle(data.angle2+angle);
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}

void RS_Arc::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_DEBUG->print("RS_Arc::rotate");
    data.center.rotate(center, angleVector);
    double angle(angleVector.angle());
    data.angle1 = RS_Math::correctAngle(data.angle1+angle);
    data.angle2 = RS_Math::correctAngle(data.angle2+angle);
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
    //todo, does this handle negative factors properly?
	calculateBorders();
}



void RS_Arc::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.center.mirror(axisPoint1, axisPoint2);
    setReversed( ! isReversed() );
    double a= (axisPoint2 - axisPoint1).angle()*2;
    setAngle1(RS_Math::correctAngle(a - getAngle1()));
    setAngle2(RS_Math::correctAngle(a - getAngle2()));
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}



void RS_Arc::moveRef(const RS_Vector& ref, const RS_Vector& offset)
{
	//avoid moving start/end points for full circle arcs
	//as start/end points coincident
	if (fabs(fabs(getAngleLength()-M_PI)-M_PI) < RS_TOLERANCE_ANGLE){
        move(offset);
        return;
	}
	auto const refs = getRefPoints();
	double dMin;
	size_t index;
	RS_Vector const vp = refs.getClosest(ref, &dMin, &index);
	if (dMin >= 1.0e-4)
		return;

	//reference points must be by the order: start, end, center
	switch (index) {
	case 0:
		moveStartpoint(vp + offset);
		return;
	case 1:
		moveEndpoint(vp + offset);
		return;
	default:
		move(offset);
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

/** find the visible part of the arc, and call drawVisible() to draw */
void RS_Arc::draw(RS_Painter* painter, RS_GraphicView* view,
                  double& patternOffset) {
	if (!( painter && view)) return;

    //only draw the visible portion of line
    RS_Vector vpMin(view->toGraph(0,view->getHeight()));
    RS_Vector vpMax(view->toGraph(view->getWidth(),0));
    QPolygonF visualBox(QRectF(vpMin.x,vpMin.y,vpMax.x-vpMin.x, vpMax.y-vpMin.y));

    RS_Vector vpStart(isReversed()?getEndpoint():getStartpoint());
    RS_Vector vpEnd(isReversed()?getStartpoint():getEndpoint());

	std::vector<RS_Vector> vertex(0);
    for(unsigned short i=0;i<4;i++){
        const QPointF& vp(visualBox.at(i));
		vertex.push_back(RS_Vector(vp.x(),vp.y()));
    }
    /** angles at cross points */
	std::vector<double> crossPoints(0);

    double baseAngle=isReversed()?getAngle2():getAngle1();
    for(unsigned short i=0;i<4;i++){
		RS_Line line{vertex.at(i),vertex.at((i+1)%4)};
		auto vpIts=RS_Information::getIntersection(
                    static_cast<RS_Entity*>(this),
                    &line,
                    true);
        if( vpIts.size()==0) continue;
		for(const RS_Vector& vp: vpIts){
			auto ap1=getTangentDirection(vp).angle();
			auto ap2=line.getTangentDirection(vp).angle();
            //ignore tangent points, because the arc doesn't cross over
            if( fabs( remainder(ap2 - ap1, M_PI) ) < RS_TOLERANCE_ANGLE) continue;
            crossPoints.push_back(
                        RS_Math::getAngleDifference(baseAngle, getCenter().angleTo(vp))
                        );
        }
    }
    if(vpStart.isInWindowOrdered(vpMin, vpMax)) crossPoints.push_back(0.);
    if(vpEnd.isInWindowOrdered(vpMin, vpMax)) crossPoints.push_back(getAngleLength());

    //sorting
    std::sort(crossPoints.begin(),crossPoints.end());
    //draw visible
    RS_Arc arc(*this);
    arc.setPen(getPen());
    arc.setSelected(isSelected());
    arc.setReversed(false);
	for(size_t i=1;i<crossPoints.size();i+=2){
		arc.setAngle1(baseAngle+crossPoints[i-1]);
		arc.setAngle2(baseAngle+crossPoints[i]);
        arc.drawVisible(painter,view,patternOffset);
    }

}

/** directly draw the arc, assuming the whole arc is within visible window */
void RS_Arc::drawVisible(RS_Painter* painter, RS_GraphicView* view,
                  double& patternOffset) {

	if (!( painter && view)) return;
    //visible in graphic view
    if(isVisibleInWindow(view)==false) return;

    RS_Vector cp=view->toGui(getCenter());
    double ra=getRadius()*view->getFactor().x;
    double length=getLength()*view->getFactor().x;
    //double styleFactor = getStyleFactor();
    patternOffset -= length;

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
    const RS_LineTypePattern* pat;
    if (isSelected()) {
        pat = &RS_LineTypePattern::patternSelected;
    } else {
        pat = view->getPattern(getPen().getLineType());
    }

	if (!pat || ra<0.5) {//avoid division by zero from small ra
		RS_DEBUG->print("%s: Invalid line pattern or radius too small, drawing arc using solid line", __func__);
        painter->drawArc(cp, ra,
                         getAngle1(),getAngle2(),
                         isReversed());
        return;
    }

//    patternOffset=remainder(patternOffset - length -0.5*pat->totalLength,pat->totalLength)+0.5*pat->totalLength;

    if (ra<RS_TOLERANCE_ANGLE){
        return;
    }

    // Pen to draw pattern is always solid:
    RS_Pen pen = painter->getPen();
    pen.setLineType(RS2::SolidLine);
    painter->setPen(pen);



    // create scaled pattern:
	if(pat->num<=0) { //invalid pattern
		RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Arc::draw(): invalid line pattern\n");
		painter->drawArc(cp,
						 ra,
						 getAngle1(), getAngle2(),
						 isReversed());
		return;
	}
	std::vector<double> da(pat->num);
    double patternSegmentLength(pat->totalLength);
	double ira=1./ra;
	double dpmm=static_cast<RS_PainterQt*>(painter)->getDpmm();
	for (size_t i=0; i<pat->num; i++){
		//        da[j] = pat->pattern[i++] * styleFactor;
		//fixme, stylefactor needed
		da[i] =dpmm*(isReversed() ? -fabs(pat->pattern[i]):fabs(pat->pattern[i]));
		if ( fabs(da[i]) < 1.) da[i] = copysign(1., da[i]);
		da[i] *= ira;
	}

    //    bool done = false;
    double total=remainder(patternOffset-0.5*patternSegmentLength,patternSegmentLength)-0.5*patternSegmentLength;

	double a1{RS_Math::correctAngle(getAngle1())};
	double a2{RS_Math::correctAngle(getAngle2())};

    if(isReversed()) {//always draw from a1 to a2, so, patternOffset is automatic
        if(a1<a2+RS_TOLERANCE_ANGLE) a2 -= 2.*M_PI;
        total = a1 - total*ira; //in angle
    }else{
        if(a2<a1+RS_TOLERANCE_ANGLE) a2 += 2.*M_PI;
        total = a1 + total*ira; //in angle
    }
    double limit(fabs(a1-a2));
    double t2;

	for(int j=0; fabs(total-a1) < limit; j=(j+1)%pat->num) {
		t2=total+da[j];

		if(pat->pattern[j] > 0.0 && fabs(t2-a2) < limit) {
			double a11=(fabs(total-a2) < limit)?total:a1;
			double a21=(fabs(t2-a1) < limit)?t2:a2;
			painter->drawArc(cp, ra, a11, a21, isReversed());
		}

		total=t2;
	}
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

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Arc::getQuadratic() const
{
    std::vector<double> ce(6,0.);
    ce[0]=1.;
    ce[2]=1.;
    ce[5]=-data.radius*data.radius;
    LC_Quadratic ret(ce);
    ret.move(data.center);
    return ret;
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 * \oint x dy = c_x r \sin t + \frac{1}{4}r^2\sin 2t +  \frac{1}{2}r^2 t
 */
double RS_Arc::areaLineIntegral() const
{
    const double& r=data.radius;
    const double& a0=data.angle1;
    const double& a1=data.angle2;
    const double r2=0.25*r*r;
    const double fStart=data.center.x*r*sin(a0)+r2*sin(a0+a0);
    const double fEnd=data.center.x*r*sin(a1)+r2*sin(a1+a1);
    return (isReversed()?fStart-fEnd:fEnd-fStart) + 2.*r2*getAngleLength();
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Arc& a) {
    os << " Arc: " << a.data << "\n";
    return os;
}

