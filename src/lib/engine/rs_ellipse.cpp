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
 */
void RS_Ellipse::calculateBorders() {
    RS_DEBUG->print("RS_Ellipse::calculateBorders");

    double radius1 = getMajorRadius();
    double radius2 = getMinorRadius();
    double angle = getAngle();
    //double a1 = ((!isReversed()) ? data.angle1 : data.angle2);
    //double a2 = ((!isReversed()) ? data.angle2 : data.angle1);
    RS_Vector startpoint = getStartpoint();
    RS_Vector endpoint = getEndpoint();

    double minX = std::min(startpoint.x, endpoint.x);
    double minY = std::min(startpoint.y, endpoint.y);
    double maxX = std::max(startpoint.x, endpoint.x);
    double maxY = std::max(startpoint.y, endpoint.y);

    RS_Vector vp;
    // kind of a brute force. TODO: exact calculation
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
    double amin,amax;
//      x range
    vp.set(radius1*cos(angle),radius2*sin(angle));

    amin=RS_Math::correctAngle(getAngle1()+vp.angle()); // to the range of 0 to 2*M_PI
    amax=RS_Math::correctAngle(getAngle2()+vp.angle()); // to the range of 0 to 2*M_PI
    if( RS_Math::isAngleBetween(M_PI,amin,amax,isReversed()) ) {
        //if( (amin<=M_PI && delta_a >= M_PI - amin) || (amin > M_PI && delta_a >= 3*M_PI - amin)) {
        minX= data.center.x-vp.magnitude();
    }
    //    else

//       minX=data.center.x +vp.magnitude()*std::min(cos(amin),cos(amin+delta_a));
    if( RS_Math::isAngleBetween(2.*M_PI,amin,amax,isReversed()) ) {
        //if( delta_a >= 2*M_PI - amin ) {
        maxX= data.center.x+vp.magnitude();
    }//    else
//       maxX= data.center.x+vp.magnitude()*std::max(cos(amin),cos(amin+delta_a));
//      y range
    vp.set(radius1*sin(angle),-radius2*cos(angle));
    amin=RS_Math::correctAngle(getAngle1()+vp.angle()); // to the range of 0 to 2*M_PI
    amax=RS_Math::correctAngle(getAngle2()+vp.angle()); // to the range of 0 to 2*M_PI
    if( RS_Math::isAngleBetween(M_PI,amin,amax,isReversed()) ) {
        //if( (amin<=M_PI &&delta_a >= M_PI - amin) || (amin > M_PI && delta_a >= 3*M_PI - amin)) {
        minY= data.center.y-vp.magnitude();
    }//    else
//        minY=data.center.y +vp.magnitude()*std::min(cos(amin),cos(amin+delta_a));
    if( RS_Math::isAngleBetween(2.*M_PI,amin,amax,isReversed()) ) {
        //if( delta_a >= 2*M_PI - amin ) {
        maxY= data.center.y+vp.magnitude();
    }
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

bool RS_Ellipse::switchMajorMinor(void)
//switch naming of major/minor, return true if success
{
    if (fabs(data.ratio) < RS_TOLERANCE) return false;
    RS_Vector vp_start=getStartpoint();
    RS_Vector vp_end=getStartpoint();
    RS_Vector vp=getMajorP();
    setMajorP(RS_Vector(- data.ratio*vp.y, data.ratio*vp.x)); //direction pi/2 relative to old MajorP;
    setRatio(1./data.ratio);
    if(   std::isnormal(getAngle1()) || std::isnormal(getAngle2() ) )  {
    //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
    setAngle1(getEllipseAngle(vp_start));
    setAngle2(getEllipseAngle(vp_end));
    }
    return true;
}

//implemented using an analytical aglorithm
// find nearest point on ellipse to a given point
//
// @author Dongxu Li <dongxuli2011@gmail.com>
//

RS_Vector RS_Ellipse::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity)
{

    RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity");
    RS_Vector ret(false);

    if( ! coord.valid ) {
        if ( dist != NULL ) *dist=RS_MAXDOUBLE;
        return ret;

    }

    if (entity!=NULL) {
        *entity = this;
    }
    ret=coord;
    ret.move(-getCenter());
    ret.rotate(-getAngle());
    double x=ret.x,y=ret.y;
    double a=getMajorRadius();
    double b=getMinorRadius();
    //std::cout<<"(a= "<<a<<" b= "<<b<<" x= "<<x<<" y= "<<y<<" )\n";
    //std::cout<<"finding minimum for ("<<x<<"-"<<a<<"*cos(t))^2+("<<y<<"-"<<b<<"*sin(t))^2\n";
    double twoa2b2=2*(a*a-b*b);
    double twoax=2*a*x;
    double twoby=2*b*y;
    double a0=twoa2b2*twoa2b2;
    double ce[4];
    double roots[4];
    unsigned int counts=0;
    //need to handle a=b
    if(a0 > RS_TOLERANCE*RS_TOLERANCE ) { // a != b , ellipse
        ce[0]=-2.*twoax/twoa2b2;
        ce[1]= (twoax*twoax+twoby*twoby)/a0-1.;
        ce[2]= - ce[0];
        ce[3]= -twoax*twoax/a0;
        //std::cout<<"1::find cosine, variable c, solve(c^4 +("<<ce[0]<<")*c^3+("<<ce[1]<<")*c^2+("<<ce[2]<<")*c+("<<ce[3]<<")=0,c)\n";
        counts=RS_Math::quarticSolver(ce,roots);
    } else {//a=b, quadratic equation for circle
        counts=2;
        a0=twoby/twoax;
        roots[0]=sqrt(1./(1.+a0*a0));
        roots[1]=-roots[0];
    }
    if(!counts) {
        //this should not happen
        std::cout<<"(a= "<<a<<" b= "<<b<<" x= "<<x<<" y= "<<y<<" )\n";
        std::cout<<"finding minimum for ("<<x<<"-"<<a<<"*cos(t))^2+("<<y<<"-"<<b<<"*sin(t))^2\n";
        std::cout<<"2::find cosine, variable c, solve(c^4 +("<<ce[0]<<")*c^3+("<<ce[1]<<")*c^2+("<<ce[2]<<")*c+("<<ce[3]<<")=0,c)\n";
        std::cout<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<' '<<ce[3]<<std::endl;
        std::cerr<<"RS_Math::RS_Ellipse::getNearestPointOnEntity() finds no root from quartic, this should not happen\n";
        return RS_Vector(coord); // better not to return invalid: return RS_Vector(false);
    }

    RS_Vector vp2(false);
    double d(RS_MAXDOUBLE),d2,s,dDistance(RS_MAXDOUBLE);
    //double ea;
    for(unsigned int i=0; i<counts; i++) {
        //I don't understand the reason yet, but I can do without checking whether sine/cosine are valid
        //if ( fabs(roots[i])>1.) continue;
        s=twoby*roots[i]/(twoax-twoa2b2*roots[i]); //sine
        //if (fabs(s) > 1. ) continue;
        d2=twoa2b2+(twoax-2.*roots[i]*twoa2b2)*roots[i]+twoby*s;
        if (d2<0) continue; // fartherest
        RS_Vector vp3;
        vp3.set(a*roots[i],b*s);
        d=vp3.distanceTo(ret);
//        std::cout<<i<<" Checking: cos= "<<roots[i]<<" sin= "<<s<<" angle= "<<atan2(roots[i],s)<<" ds2= "<<d<<" d="<<d2<<std::endl;
        if( vp2.valid && d>dDistance) continue;
        vp2=vp3;
        dDistance=d;
//			ea=atan2(roots[i],s);
    }
    if( ! vp2.valid ) {
        //this should not happen
        std::cout<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<' '<<ce[3]<<std::endl;
        std::cout<<"(x,y)=( "<<x<<" , "<<y<<" ) a= "<<a<<" b= "<<b<<" sine= "<<s<<" d2= "<<d2<<" dist= "<<d<<std::endl;
        std::cout<<"RS_Ellipse::getNearestPointOnEntity() finds no minimum, this should not happen\n";
    }
    if (dist!=NULL) {
        *dist = dDistance;
    }
    vp2.rotate(getAngle());
    vp2.move(getCenter());
    ret=vp2;
    if (onEntity) {
        if (!RS_Math::isAngleBetween(getEllipseAngle(ret), getAngle1(), getAngle2(), isReversed())) { // not on entity, use the nearest endpoint
               //std::cout<<"not on ellipse, ( "<<getAngle1()<<" "<<getEllipseAngle(ret)<<" "<<getAngle2()<<" ) reversed= "<<isReversed()<<"\n";
            ret=getNearestEndpoint(coord,dist);
        }
    }

    if(! ret.valid) {
        std::cout<<"RS_Ellipse::getNearestOnEntity() returns invalid by mistake. This should not happen!"<<std::endl;
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
                                 double tolerance) const {
    double t=fabs(tolerance);
    double a=getMajorRadius();
    double b=a*getRatio();

    RS_Vector vp(coord - getCenter());
    vp.rotate(-getAngle());
    if ( a<t ) {
        //radius treated as zero
        if(fabs(vp.x)<t && fabs(vp.y) < b) return true;
        return false;
    }
    if ( b<t ) {
        //radius treated as zero
        if (fabs(vp.y)<t && fabs(vp.x) < a) return true;
        return false;
    }
    vp.scale(1./a,1./b);
    if (fabs(vp.squared()-1.) < t) return true;
    return false;

//    if ( getCenter().distanceTo(coord) < tolerance ) {
//            if (getMajorRadius() < tolerance || getMinorRadius() < tolerance ) {
//                    return true;
//            } else {
//                    return false;
//            }
//    }
//    double dist = getDistanceToPoint(coord, NULL, RS2::ResolveNone);
//    return (dist<=tolerance);
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
RS_Vector RS_Ellipse::getMiddlePoint(){
        return getNearestMiddle(getCenter());
}

RS_Vector RS_Ellipse::getNearestMiddle(const RS_Vector& coord,
                                       double* dist,
                                       int middlePoints
                                       ) {
    RS_DEBUG->print("RS_Ellpse::getNearestMiddle(): begin\n");
    if ( ! ( std::isnormal(getAngle1()) || std::isnormal(getAngle2()))) {
            //no middle point for whole ellipse, angle1=angle2=0
        if (dist!=NULL) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }
    double ra(getMajorRadius());
    double rb(getMinorRadius());
    if ( ra < RS_TOLERANCE || rb < RS_TOLERANCE ) {
            //zero radius, return the center
            RS_Vector vp(getCenter());
        if (dist!=NULL) {
            *dist = vp.distanceTo(coord);
        }
        return vp;
    }
    double amin=getCenter().angleTo(getStartpoint());
    double amax=getCenter().angleTo(getEndpoint());
    if(isReversed()) {
            std::swap(amin,amax);
    }
    double da=fmod(amax-amin+2.*M_PI, 2.*M_PI);
    if ( da < RS_TOLERANCE ) {
            da = 2.*M_PI; //whole ellipse
    }
    RS_Vector vp(getNearestPointOnEntity(coord,true,dist));
    double a=getCenter().angleTo(vp);
    int counts(middlePoints + 1);
    int i( static_cast<int>(fmod(a-amin+2.*M_PI,2.*M_PI)/da*counts+0.5));
    if(!i) i++; // remove end points
    if(i==counts) i--;
    a=amin + da*(double(i)/double(counts))-getAngle();
    vp.set(a);
    RS_Vector vp2(vp);
    vp2.scale( RS_Vector(1./ra,1./rb));
    vp.scale(1./vp2.magnitude());
    vp.rotate(getAngle());
    vp.move(getCenter());

    if (dist!=NULL) {
        *dist = vp.distanceTo(coord);
    }
    //RS_DEBUG->print("RS_Ellipse::getNearestMiddle: angle1=%g, angle2=%g, middle=%g\n",amin,amax,a);
    RS_DEBUG->print("RS_Ellpse::getNearestMiddle(): end\n");
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
/**
  * get the tangential point of a tangential line orthogonal to a given line
  *@ normal, the given line
  *@ onEntity, should the tangential be required to on entity of the elliptic arc
  *@ coord, current cursor position
  */

RS_Vector RS_Ellipse::getNearestOrthTan(const RS_Vector& coord,
                    const RS_Line& normal,
                    bool onEntity )
{
        if ( !coord.valid ) {
                return RS_Vector(false);
        }
        RS_Vector direction=normal.getEndpoint() - normal.getStartpoint();
        if (direction.squared()< RS_TOLERANCE*RS_TOLERANCE) {
                //undefined direction
                return RS_Vector(false);
        }
        //scale to ellipse angle
        direction.rotate(-getAngle());
        double angle=direction.scale(RS_Vector(1.,getRatio())).angle();
        direction.set(getMajorRadius()*cos(angle),getMinorRadius()*sin(angle));//relative to center
        QList<RS_Vector> sol;
        for(int i=0;i<2;i++){
                if(!onEntity ||
                   RS_Math::isAngleBetween(angle,getAngle1(),getAngle2(),isReversed())) {
                if(i){
                sol.append(- direction);
                }else{
                sol.append(direction);
                }
                }
                angle=RS_Math::correctAngle(angle+M_PI);
        }
        RS_Vector vp;
        switch(sol.count()) {
                case 0:
                        return RS_Vector(false);
                case 2:
                        if( RS_Vector::dotP(sol[1]-getCenter(),coord-getCenter())>0.) {
                                vp=sol[1];
                                break;
                        }
                default:
                        vp=sol[0];
        }
        return getCenter() + vp.rotate(getAngle());
}



double RS_Ellipse::getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity,
                                      RS2::ResolveLevel, double /*solidDist*/) {
    double dToEntity = RS_MAXDOUBLE;
    getNearestPointOnEntity(coord, true, &dToEntity, entity);

    // RVT 6 Jan 2011 : Add selection by center point
    double dToCenter=data.center.distanceTo(coord);
    return std::min(dToEntity,dToCenter);
}



void RS_Ellipse::move(const RS_Vector& offset) {
    data.center.move(offset);
    //calculateEndpoints();
    minV.move(offset);
    maxV.move(offset);
}



void RS_Ellipse::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    data.center.rotate(center, angleVector);
    data.majorP.rotate(angleVector);
    //calculateEndpoints();
    calculateBorders();
}
void RS_Ellipse::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.center.rotate(center, angleVector);
    data.majorP.rotate(angleVector);
    //calculateEndpoints();
    calculateBorders();
}

void RS_Ellipse::rotate( const double& angle) {
    RS_Vector angleVector(angle);
    data.center.rotate(angleVector);
    data.majorP.rotate(angleVector);
    //calculateEndpoints();
    calculateBorders();
}

/**
 * make sure angleLength() is not more than 2*M_PI
 */
void RS_Ellipse::correctAngles() {
        double *pa1= & data.angle1;
        double *pa2= & data.angle2;
        if (isReversed()) std::swap(pa1,pa2);
        *pa2 = *pa1 + fmod(*pa2 - *pa1, 2.*M_PI);
        if ( fabs(data.angle1 - data.angle2) < RS_TOLERANCE_ANGLE ) *pa2 += 2.*M_PI;
}

void RS_Ellipse::moveStartpoint(const RS_Vector& pos) {
    data.angle1 = getEllipseAngle(pos);
    //data.angle1 = data.center.angleTo(pos);
    //calculateEndpoints();
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}



void RS_Ellipse::moveEndpoint(const RS_Vector& pos) {
    data.angle2 = getEllipseAngle(pos);
    //data.angle2 = data.center.angleTo(pos);
    //calculateEndpoints();
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}


RS2::Ending RS_Ellipse::getTrimPoint(const RS_Vector& trimCoord,
                                     const RS_Vector& /*trimPoint*/) {

    //double angEl = getEllipseAngle(trimPoint);
    double angM = getEllipseAngle(trimCoord);
    if (RS_Math::getAngleDifference(angM, data.angle1) > RS_Math::getAngleDifference(data.angle2,angM)) {
        return RS2::EndingStart;
    } else {
        return RS2::EndingEnd;
    }
}

RS_Vector RS_Ellipse::prepareTrim(const RS_Vector& trimCoord,
                                  const RS_VectorSolutions& trimSol) {
//special trimming for ellipse arc
        RS_DEBUG->print("RS_Ellipse::prepareTrim()");
    if( ! trimSol.hasValid() ) return (RS_Vector(false));
    if( trimSol.getNumber() == 1 ) return (trimSol.get(0));
    double am=getEllipseAngle(trimCoord);
    QList<double> ias;
    double ia(0.),ia2(0.);
    RS_Vector is,is2;
    for(int ii=0; ii<trimSol.getNumber(); ii++) { //find closest according ellipse angle
        ias.append(getEllipseAngle(trimSol.get(ii)));
        if( !ii ||  fabs( remainder( ias[ii] - am, 2*M_PI)) < fabs( remainder( ia -am, 2*M_PI)) ) {
            ia = ias[ii];
            is = trimSol.get(ii);
        }
    }
    qSort(ias.begin(),ias.end());
    for(int ii=0; ii<trimSol.getNumber(); ii++) { //find segment to enclude trimCoord
        if ( ! RS_Math::isSameDirection(ia,ias[ii],RS_TOLERANCE)) continue;
        if( RS_Math::isAngleBetween(am,ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()],ia,false))  {
            ia2=ias[(ii+trimSol.getNumber()-1)% trimSol.getNumber()];
        } else {
            ia2=ias[(ii+1)% trimSol.getNumber()];
        }
        break;
    }
    for(int ii=0; ii<trimSol.getNumber(); ii++) { //find segment to enclude trimCoord
        if ( ! RS_Math::isSameDirection(ia2,getEllipseAngle(trimSol.get(ii)),RS_TOLERANCE)) continue;
        is2=trimSol.get(ii);
        break;
    }
    if(RS_Math::isSameDirection(getAngle1(),getAngle2(),RS_TOLERANCE_ANGLE)
            ||  RS_Math::isSameDirection(ia2,ia,RS_TOLERANCE) ) {
        //whole ellipse
        if( !RS_Math::isAngleBetween(am,ia,ia2,isReversed())) {
            std::swap(ia,ia2);
            std::swap(is,is2);
        }
        setAngle1(ia);
        setAngle2(ia2);
        double da1=fabs(remainder(getAngle1()-am,2*M_PI));
        double da2=fabs(remainder(getAngle2()-am,2*M_PI));
        if(da2<da1) {
            std::swap(is,is2);
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
                //std::cout<<"reset: angle1="<<getAngle1()<<" angle2="<<getAngle2()<<" am="<< am<<" is="<<getEllipseAngle(is)<<" ia2="<<ia2<<std::endl;
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
    }
    return is;
}

double RS_Ellipse::getEllipseAngle(const RS_Vector& pos) const {
    RS_Vector m = pos-data.center;
    m.rotate(-data.majorP.angle());
    m.scale(RS_Vector(data.ratio, 1.0));
    return m.angle();
}


/* Dongxu Li's Version, 19 Aug 2011
 * scale an ellipse
 * Find the eigen vactors and eigen values by optimization
 * original ellipse equation,
 * x= a cos t
 * y= b sin t
 * rotated by angle,
 *
 * x = a cos t cos (angle) - b sin t sin(angle)
 * y = a cos t sin (angle) + b sin t cos(angle)
 * scaled by ( kx, ky),
 * x *= kx
 * y *= ky
 * find the maximum and minimum of x^2 + y^2,
 */
void RS_Ellipse::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.center.scale(center, factor);
    RS_Vector vpStart=getStartpoint().scale(getCenter(),factor);
    RS_Vector vpEnd=getEndpoint().scale(getCenter(),factor);;
    double ct=cos(getAngle());
    double ct2 = ct*ct; // cos^2 angle
    double st=sin(getAngle());
    double st2=1.0 - ct2; // sin^2 angle
    double kx2= factor.x * factor.x;
    double ky2= factor.y * factor.y;
    double a=getMajorRadius();
    double b=getMinorRadius();
    double cA=0.5*a*a*(kx2*ct2+ky2*st2);
    double cB=0.5*b*b*(kx2*st2+ky2*ct2);
    double cC=a*b*ct*st*(ky2-kx2);
    RS_Vector vp(cA-cB,cC);
    setMajorP(RS_Vector(a,b).scale(RS_Vector(vp.angle())).rotate(RS_Vector(ct,st)).scale(factor));
    a=cA+cB;
    b=vp.magnitude();
    setRatio( sqrt((a - b)/(a + b) ));
    if(   std::isnormal(getAngle1()) || std::isnormal(getAngle2() ) )  {
    //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
    setAngle1(getEllipseAngle(vpStart));
    setAngle2(getEllipseAngle(vpEnd));
    }
    correctAngles();//avoid extra 2.*M_PI in angles
    //calculateEndpoints();
    scaleBorders(center,factor);
//    calculateBorders();
}


/**
 * mirror by the axis defined by axisPoint1 and axisPoint2
 */
void RS_Ellipse::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Vector center=getCenter();
    RS_Vector mp = center + getMajorP();
    RS_Vector startpoint = getStartpoint();
    RS_Vector endpoint = getEndpoint();

    center.mirror(axisPoint1, axisPoint2);
    mp.mirror(axisPoint1, axisPoint2);
    startpoint.mirror(axisPoint1, axisPoint2);
    endpoint.mirror(axisPoint1, axisPoint2);

    setCenter(center);
    setReversed(!isReversed());
    setMajorP(mp - center);
    if(   std::isnormal(getAngle1()) || std::isnormal(getAngle2() ) )  {
    //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
    setAngle1( getEllipseAngle(startpoint));
    setAngle2( getEllipseAngle(endpoint));
    }
/*  old version
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
*/
    //calculateEndpoints();
    correctAngles();//avoid extra 2.*M_PI in angles
    calculateBorders();
}

/**
  * get direction1 and direction2
  * get the tangent pointing outside at end points
  *
  * Author: Dongxu Li
  */
//getDirection1 for start point
double RS_Ellipse::getDirection1() const {
    RS_Vector vp;
    if (isReversed()){
        vp.set(sin(getAngle1()), -getRatio()*cos(getAngle1()));
    } else {
        vp.set(-sin(getAngle1()), getRatio()*cos(getAngle1()));
    }
    return vp.angle()+getAngle();
}

//getDirection2 for end point
double RS_Ellipse::getDirection2() const {
    RS_Vector vp;
    if (isReversed()){
        vp.set(-sin(getAngle2()), getRatio()*cos(getAngle2()));
    } else {
        vp.set(sin(getAngle2()), -getRatio()*cos(getAngle2()));
    }
    return vp.angle()+getAngle();
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
    correctAngles();//avoid extra 2.*M_PI in angles
}


void RS_Ellipse::draw(RS_Painter* painter, RS_GraphicView* view, double /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }


    if (getPen().getLineType()==RS2::SolidLine ||
            ! isSelected() ||
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

