/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)
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


#include <vector>

#include "lc_quadratic.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"


namespace {
// tangent condition tolerance
// two circles are considered tangent, if the distance is within this factor of the radii
constexpr double Tangent_Tolerance_Factor = 1e-6;

/**
 * @brief isCollinearXY whether the 2x3 matrix has degenerate columns
 * @param mat - a 2x3 linear equation to solve an Appollonius
 * @return  true, if the matrix is degenerate, i.e. the 3 input circle centers have identical
 *                x or y-coordinates
 */
bool identicalXOrY(const std::vector<std::vector<double>>& mat) {
    // matrix must be 2x3 in dimension
    assert(mat.size() >= 2 && mat.front().size() >= 3);
    const auto isDegenerateCol = [&mat] (size_t column) {
        return RS_Math::equal(std::max(std::abs(mat[0][column]), std::abs(mat[1][column])), 0., RS_TOLERANCE);
    };
    // first(x) or second(y) column
    return isDegenerateCol(0) || isDegenerateCol(1);
}
}

RS_CircleData::RS_CircleData(RS_Vector const& center, double radius):
	center(center)
	, radius(radius)
{
}

bool RS_CircleData::isValid() const {
	return (center.valid && radius>RS_TOLERANCE);
}

bool RS_CircleData::operator == (RS_CircleData const& rhs) const
{
    if (!(center.valid && rhs.center.valid))
      return false;
    if (center.squaredTo(rhs.center) > RS_TOLERANCE2)
      return false;
    return std::abs(radius - rhs.radius) < RS_TOLERANCE;
}

std::ostream& operator << (std::ostream& os, const RS_CircleData& ad)
{
	os << "(" << ad.center <<
		  "/" << ad.radius <<
		  ")";
	return os;
}


/**
 * constructor.
 */
RS_Circle::RS_Circle(RS_EntityContainer* parent,
                     const RS_CircleData& d)
    :LC_CachedLengthEntity(parent), data(d) {
    calculateBorders();
}

RS_Entity* RS_Circle::clone() const {
    RS_Circle* c = new RS_Circle(*this);
    return c;
}

void RS_Circle::calculateBorders() {
    RS_Vector r{data.radius, data.radius};
    minV = data.center - r;
    maxV = data.center + r;
    updateLength();
}

/** @return The center point (x) of this arc */
RS_Vector RS_Circle::getCenter() const {
    return data.center;
}
/** Sets new center. */
void RS_Circle::setCenter(const RS_Vector& c) {
    data.center = c;
}
/** @return The radius of this arc */
double RS_Circle::getRadius() const {
    return data.radius;
}

/** Sets new radius. */
void RS_Circle::setRadius(double r) {
    data.radius = r;
}

/**
 * @return Angle length in rad.
 */
double RS_Circle::getAngleLength() const {
    return 2*M_PI;
}

/**
 * @return Length of the circle which is the circumference.
 */
void RS_Circle::updateLength() {
    cachedLength = 2.0 * M_PI * data.radius;
}

bool RS_Circle::isTangent(const RS_CircleData&  circleData) const{
    const double d=circleData.center.distanceTo(data.center);
    double r0=std::abs(circleData.radius);
    double r1=std::abs(data.radius);
    if (r0 < r1)
        std::swap(r0, r1);
    const double tol = Tangent_Tolerance_Factor * r0;
    if (r1 < tol || d < tol)
        return false;

    const double tangentTol = std::max(200.*RS_TOLERANCE, tol);
    // Internal or external tangency
    bool ret = std::abs(d-r0+r1)<tangentTol ||
            std::abs(d-r0-r1)<tangentTol;
    return ret;
}


/**
 * Creates this circle from a center point and a radius.
 *
 * @param c Center.
 * @param r Radius
 */
bool RS_Circle::createFromCR(const RS_Vector& c, double r) {
    if (std::abs(r)>RS_TOLERANCE && c.valid ) {
        data.radius = std::abs(r);
		data.center = c;
        return true;
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFromCR(): "
                        "Cannot create a circle with radius 0.0.");
        return false;
    }
}



/**
 * Creates this circle from two opposite points.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 */
bool RS_Circle::createFrom2P(const RS_Vector& p1, const RS_Vector& p2) {
        double r=0.5*p1.distanceTo(p2);
    if (r>RS_TOLERANCE) {
		data.radius = r;
		data.center = (p1+p2)*0.5;
        return true;
    } else {
//        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom2P(): "
//                        "Cannot create a circle with radius 0.0.");
        return false;
    }
}



/**
 * Creates this circle from 3 given points which define the circle line.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 * @param p3 3rd point.
 */
bool RS_Circle::createFrom3P(const RS_Vector& p1, const RS_Vector& p2,
                             const RS_Vector& p3) {
        RS_Vector vra=p2 - p1;
        RS_Vector vrb=p3 - p1;
        double ra2=vra.squared()*0.5;
        double rb2=vrb.squared()*0.5;
        double crossp=vra.x * vrb.y - vra.y * vrb.x;
        if (std::abs(crossp)< RS_TOLERANCE2) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): "
                        "Cannot create a circle with radius 0.0.");
                return false;
        }
        crossp=1./crossp;
		data.center.set((ra2*vrb.y - rb2*vra.y)*crossp,(rb2*vra.x - ra2*vrb.x)*crossp);
		data.radius=data.center.magnitude();
		data.center += p1;
        return true;
}
//*create Circle from 3 points
//Author: Dongxu Li
bool RS_Circle::createFrom3P(const RS_VectorSolutions& sol) {
    if(sol.getNumber() < 2) return false;
    if(sol.getNumber() == 2) return createFrom2P(sol.get(0),sol.get(1));
    if((sol.get(1)-sol.get(2)).squared() < RS_TOLERANCE2)
        return createFrom2P(sol.get(0),sol.get(1));
    RS_Vector vra(sol.get(1) - sol.get(0));
    RS_Vector vrb(sol.get(2) - sol.get(0));
    double ra2=vra.squared()*0.5;
    double rb2=vrb.squared()*0.5;
    double crossp=vra.x * vrb.y - vra.y * vrb.x;
    if (std::abs(crossp)< RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): "
                        "Cannot create a circle with radius 0.0.");
        return false;
    }
    crossp=1./crossp;
	data.center.set((ra2*vrb.y - rb2*vra.y)*crossp,(rb2*vra.x - ra2*vrb.x)*crossp);
	data.radius=data.center.magnitude();
	data.center += sol.get(0);
    return true;
}

/**
  *create circle inscribled in a triangle
  *
  *Author: Dongxu Li
  */
bool RS_Circle::createInscribe(const RS_Vector& coord, const std::vector<RS_Line*>& lines){
    if(lines.size()<3) return false;
	std::vector<RS_Line*> tri(lines);
    RS_VectorSolutions sol=RS_Information::getIntersectionLineLine(tri[0],tri[1]);
    if(sol.getNumber() == 0 ) {//move parallel to opposite
        std::swap(tri[1],tri[2]);
        sol=RS_Information::getIntersectionLineLine(tri[0],tri[1]);
    }
    if(sol.getNumber() == 0 ) return false;
    RS_Vector vp0(sol.get(0));
    sol=RS_Information::getIntersectionLineLine(tri[2],tri[1]);
    if(sol.getNumber() == 0 ) return false;
    RS_Vector vp1(sol.get(0));
    RS_Vector dvp(vp1-vp0);
    double a(dvp.squared());
    if( a< RS_TOLERANCE2) return false; //three lines share a common intersecting point
    RS_Vector vp(coord - vp0);
    vp -= dvp*(RS_Vector::dotP(dvp,vp)/a); //normal component
    RS_Vector vl0(tri[0]->getEndpoint() - tri[0]->getStartpoint());
    a=dvp.angle();
    double angle0(0.5*(vl0.angle() + a));
    if( RS_Vector::dotP(vp,vl0) <0.) {
        angle0 += 0.5*M_PI;
    }

    RS_Line line0(vp0, vp0+RS_Vector(angle0));//first bisecting line
    vl0=(tri[2]->getEndpoint() - tri[2]->getStartpoint());
    angle0=0.5*(vl0.angle() + a+M_PI);
    if( RS_Vector::dotP(vp,vl0) <0.) {
        angle0 += 0.5*M_PI;
    }
    RS_Line line1(vp1, vp1+RS_Vector(angle0));//second bisection line
    sol=RS_Information::getIntersectionLineLine(&line0,&line1);
    if(sol.getNumber() == 0 ) return false;

	bool ret=createFromCR(sol.get(0),tri[1]->getDistanceToPoint(sol.get(0)));
	if(!ret) return false;
	for(auto p: lines){
		if(! p->isTangent(data)) return false;
	}
	return true;
}

std::vector<RS_Entity* > RS_Circle::offsetTwoSides(const double& distance) const
{
	std::vector<RS_Entity*> ret(0,nullptr);
	ret.push_back(new RS_Circle(nullptr, {getCenter(),getRadius()+distance}));
    if(std::abs(getRadius()-distance)>RS_TOLERANCE)
    ret.push_back(new RS_Circle(nullptr, {getCenter(),std::abs(getRadius()-distance)}));
    return ret;
}

RS_VectorSolutions RS_Circle::createTan1_2P(const RS_AtomicEntity* circle, const std::vector<RS_Vector>& points)
{
    RS_VectorSolutions ret;
	if (!circle||points.size()<2) return ret;
    return LC_Quadratic::getIntersection(
                LC_Quadratic(circle,points[0]),
                LC_Quadratic(circle,points[1])
                );
}

/**
  * create a circle of radius r and tangential to two given entities
  */
RS_VectorSolutions RS_Circle::createTan2(const std::vector<RS_AtomicEntity*>& circles, const double& r)
{
    if(circles.size()<2) return false;
	auto e0=circles[0]->offsetTwoSides(r);
	auto e1=circles[1]->offsetTwoSides(r);
    RS_VectorSolutions centers;
	if(e0.size() && e1.size()) {
        for(auto it0=e0.begin();it0!=e0.end();it0++){
            for(auto it1=e1.begin();it1!=e1.end();it1++){
                centers.push_back(RS_Information::getIntersection(*it0,*it1));
            }
        }
    }
    for(auto it0=e0.begin();it0!=e0.end();it0++){
        delete *it0;
    }
    for(auto it0=e1.begin();it0!=e1.end();it0++){
        delete *it0;
	}
    return centers;

}

std::vector<RS_Circle> RS_Circle::createTan3(const std::vector<RS_AtomicEntity*>& circles)
{
	std::vector<RS_Circle> ret;
    if(circles.size()!=3)
        return ret;
	 std::vector<RS_Circle> cs;
     for(RS_AtomicEntity* c: circles){
		 cs.emplace_back(RS_Circle(nullptr, {c->getCenter(),c->getRadius()}));
	 }
    unsigned short flags=0;
    do{
        for(unsigned short j=0u;j<3u;++j){
            if(flags & (1u<<j)) {
                cs[j].setRadius( - std::abs(cs[j].getRadius()));
            }else{
                cs[j].setRadius( std::abs(cs[j].getRadius()));
            }
        }
//        RS_DEBUG->print(RS_Debug::D_ERROR, "flags=%d\n",flags);
        std::vector<RS_Circle> list = solveAolloniusSingle(cs);
        if (list.empty())
            list = solveApolloniusHyperbola(cs);
        if(list.size()>=1){
            for(RS_Circle& c0: list){
                bool addNew=true;
                for(RS_Circle& c: ret){
                    if((c0.getCenter()-c.getCenter()).squared()<RS_TOLERANCE15 && std::abs(c0.getRadius() - c.getRadius())<RS_TOLERANCE){
                        addNew=false;
                        break;
                    }
                }
                if(addNew)
                    ret.push_back(c0);
            }
        }


    }while(++flags<8u);
//    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//    std::cout<<"before testing, ret.size()="<<ret.size()<<std::endl;
    auto it = std::remove_if(ret.begin(), ret.end(), [&circles](const RS_Circle& circle) {
        return !circle.testTan3(circles);
    });
    ret.erase(it, ret.end());
//        DEBUG_HEADER
//    std::cout<<"after testing, ret.size()="<<ret.size()<<std::endl;
    return ret;
}

bool RS_Circle::testTan3(const std::vector<RS_AtomicEntity*>& circles) const
{
    if(circles.size()!=3)
        return false;

    for(auto const& c: circles){
        if (!c->isTangent(data))
            return false;
    }
    return true;
}

/** solve one of the eight Appollonius Equations
| Cx - Ci|^2=(Rx+Ri)^2
with Cx the center of the common tangent circle, Rx the radius. Ci and Ri are the Center and radius of the i-th existing circle
**/
std::vector<RS_Circle> RS_Circle::solveAolloniusSingle(const std::vector<RS_Circle>& circles)
{
//          std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//          for(int i=0;i<circles.size();i++){
//std::cout<<"i="<<i<<"\t center="<<circles[i].getCenter()<<"\tr="<<circles[i].getRadius()<<std::endl;
//          }

	std::vector<RS_Vector> centers;
    std::vector<double> radii;

    for(const RS_Circle& c: circles){
        if(!c.getCenter().valid)
            return {};
		centers.push_back(c.getCenter());
		radii.push_back(c.getRadius());
	}
//              for(int i=0;i<circles.size();i++){
//    std::cout<<"i="<<i<<"\t center="<<circles[i].getCenter()<<"\tr="<<radii.at(i)<<std::endl;
//              }
/** form the linear equation to solve center in radius **/
    std::vector<std::vector<double> > mat(2, std::vector<double>(3,0.));
    mat[0][0]=centers[2].x - centers[0].x;
    mat[0][1]=centers[2].y - centers[0].y;
    mat[1][0]=centers[2].x - centers[1].x;
    mat[1][1]=centers[2].y - centers[1].y;

    // Issue #2160: this algebraic algorithm fails when input circle centers are identical in
    // x-coordinates or y-coordinates
    LC_LOG<<__func__<<"(): identicalXOrY="<<identicalXOrY(mat);
    if(identicalXOrY(mat) || std::abs(mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0])<RS_TOLERANCE15) {
        return {};
    }
    // r^0 term
    mat[0][2]=0.5*(centers[2].squared()-centers[0].squared()+radii[0]*radii[0]-radii[2]*radii[2]);
    mat[1][2]=0.5*(centers[2].squared()-centers[1].squared()+radii[1]*radii[1]-radii[2]*radii[2]);
//    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//    for(unsigned short i=0;i<=1;i++){
//        std::cout<<"eqs P:"<<i<<" : "<<mat[i][0]<<"*x + "<<mat[i][1]<<"*y = "<<mat[i][2]<<std::endl;
//    }
//    std::vector<std::vector<double> > sm(2,std::vector<double>(2,0.));
	std::vector<double> sm(2,0.);
    if(RS_Math::linearSolver(mat,sm)==false){
        return {};
    }

    RS_Vector vp(sm[0],sm[1]);
//      std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//      std::cout<<"vp="<<vp<<std::endl;

    // r term
    mat[0][2]= radii[0]-radii[2];
    mat[1][2]= radii[1]-radii[2];
//    for(unsigned short i=0;i<=1;i++){
//        std::cout<<"eqs Q:"<<i<<" : "<<mat[i][0]<<"*x + "<<mat[i][1]<<"*y = "<<mat[i][2]<<std::endl;
//    }
    if(RS_Math::linearSolver(mat,sm)==false){
        return {};
    }
    RS_Vector vq(sm[0],sm[1]);
//      std::cout<<"vq="<<vq<<std::endl;
    //form quadratic equation for r
    RS_Vector dcp=vp-centers[0];
    double a=vq.squared()-1.;
    if(std::abs(a)<RS_TOLERANCE*1e-4) {
        return {};
    }
    std::vector<double> ce(0,0.);
    ce.push_back(2.*(dcp.dotP(vq)-radii[0])/a);
    ce.push_back((dcp.squared()-radii[0]*radii[0])/a);
    std::vector<double> vr=RS_Math::quadraticSolver(ce);
    std::vector<RS_Circle> ret;
    for(double dist: vr) {
        if(dist >= RS_TOLERANCE)
            ret.emplace_back(RS_Circle(nullptr, {vp+vq*dist, std::abs(dist)}));
    }
//    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//    std::cout<<"Found "<<ret.size()<<" solutions"<<std::endl;

    return ret;
}

std::vector<RS_Circle> RS_Circle::solveApolloniusHyperbola(const std::vector<RS_Circle>& circles)
{
    assert(circles.size() == 3);
    std::vector<RS_Vector> centers;
    std::vector<double> radii;

    for(const RS_Circle& c: circles){
        if(!c.getCenter().valid)
            return {};
        centers.push_back(c.getCenter());
        radii.push_back(c.getRadius());
    }

    size_t i0 = ( centers[0] == centers[1]||  centers[0] == centers[2]) ? 1 : 0;

    std::vector<RS_Circle> ret;
    LC_Quadratic lc0(& (circles[i0]), & (circles[(i0+1)%3]));
    LC_Quadratic lc1(& (circles[i0]), & (circles[(i0+2)%3]));
    RS_VectorSolutions c0 = LC_Quadratic::getIntersection(lc0, lc1);
    for(size_t i=0; i<c0.size(); i++){
        const double dc =  c0[i].distanceTo(centers[i0]);
        ret.push_back(RS_Circle(nullptr, {c0[i], std::abs(dc - radii[i0])}));
        if( dc > radii[i0]) {
            ret.push_back(RS_Circle(nullptr, {c0[i], dc + radii[i0]}));
        }
    }
    return ret;
}


RS_VectorSolutions RS_Circle::getRefPoints() const
{
	RS_Vector v1(data.radius, 0.0);
	RS_Vector v2(0.0, data.radius);

	return RS_VectorSolutions ({data.center,
						   data.center+v1, data.center+v2,
						   data.center-v1, data.center-v2});
}


/**
 * @brief compute nearest endpoint, intersection with X/Y axis at 0, 90, 180 and 270 degree
 *
 * Use getNearestMiddle() method to compute the nearest circle quadrant endpoints
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @return the nearest intersection of the circle with X/Y axis.
 */
RS_Vector RS_Circle::getNearestEndpoint(const RS_Vector& coord, double* dist /*= nullptr*/) const
{
    return getNearestMiddle( coord, dist, 0);
}


RS_Vector RS_Circle::getNearestPointOnEntity(const RS_Vector& coord,
        bool /*onEntity*/, double* dist, RS_Entity** entity)const {

	if (entity) {
        *entity = const_cast<RS_Circle*>(this);
    }
	RS_Vector vp(coord - data.center);
    double d(vp.magnitude());
    if( d < RS_TOLERANCE ) return RS_Vector(false);
	vp =data.center+vp*(data.radius/d);
//    RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)\n",data.center.x,data.center.y,coord.x,coord.y);

	if(dist){
        *dist=coord.distanceTo(vp);
//        RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)=%g\n",data.center.x,data.center.y,coord.x,coord.y,*dist);
    }
    return vp;
}


/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  *Author: Dongxu Li
  */
RS_VectorSolutions RS_Circle::getTangentPoint(const RS_Vector& point) const {
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
RS_Vector RS_Circle::getTangentDirection(const RS_Vector& point) const {
    RS_Vector vp(point-getCenter());
//    double c2(vp.squared());
//    if(c2<r2-getRadius()*2.*RS_TOLERANCE) {
//        //inside point, no tangential point
//        return RS_Vector(false);
//    }
    return RS_Vector(-vp.y,vp.x);

}

RS_Vector RS_Circle::getNearestCenter(const RS_Vector& coord,
									  double* dist) const{
	if (dist) {
		*dist = coord.distanceTo(data.center);
    }
	return data.center;
}



RS_Vector RS_Circle::getMiddlePoint(void)const
{
    return RS_Vector(false);
}

/**
 * @brief compute middlePoints for each quadrant of a circle
 *
 * 0 middlePoints snaps to axis intersection at 0, 90, 180 and 270 degree (getNearestEndpoint) \n
 * 1 middlePoints snaps to 45, 135, 225 and 315 degree \n
 * 2 middlePoints snaps to 30, 60, 120, 150, 210, 240, 300 and 330 degree \n
 * and so on
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @param middlePoints number of middle points to compute per quadrant (0 for endpoints)
 * @return the nearest of equidistant middle points of the circles quadrants.
 */
RS_Vector RS_Circle::getNearestMiddle(const RS_Vector& coord,
                                      double* dist /*= nullptr*/,
                                      const int middlePoints /*= 1*/) const
{
	if( data.radius <= RS_TOLERANCE) {
        //circle too short
        if ( nullptr != dist) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }

    RS_Vector vPoint( getNearestPointOnEntity( coord, true, dist));
    int iCounts = middlePoints + 1;
	double dAngleSteps = M_PI_2 / iCounts;
	double dAngleToPoint = data.center.angleTo(vPoint);
    int iStepCount = static_cast<int>((dAngleToPoint + 0.5 * dAngleSteps) / dAngleSteps);
    if( 0 < middlePoints) {
        // for nearest middle eliminate start/endpoints
        int iQuadrant = static_cast<int>(dAngleToPoint / 0.5 / M_PI);
        int iQuadrantStep = iStepCount - iQuadrant * iCounts;
        if( 0 == iQuadrantStep) {
            ++iStepCount;
        }
        else if( iCounts == iQuadrantStep) {
            --iStepCount;
        }
    }

	vPoint.setPolar( data.radius, dAngleSteps * iStepCount);
	vPoint.move( data.center);

	if(dist) {
        *dist = vPoint.distanceTo( coord);
    }

    return vPoint;
}


RS_Vector RS_Circle::getNearestDist(double /*distance*/,
                                    const RS_Vector& /*coord*/,
									double* dist) const{

	if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_Circle::getNearestDist(double /*distance*/,
									bool /*startp*/) const{

    return RS_Vector(false);
}


RS_Vector RS_Circle::getNearestOrthTan(const RS_Vector& coord,
                    const RS_Line& normal,
					bool /*onEntity = false*/) const
{
        if ( !coord.valid) {
                return RS_Vector(false);
        }
        RS_Vector vp0(coord-getCenter());
        RS_Vector vp1(normal.getAngle1());
        double d=RS_Vector::dotP(vp0,vp1);
        if(d >= 0. ) {
                return getCenter() + vp1*getRadius();
        }else{
                return getCenter() - vp1*getRadius();
        }
}

RS_Vector RS_Circle::dualLineTangentPoint(const RS_Vector& line) const
{
    RS_Vector dr = line.normalized() * data.radius;
    RS_Vector vp0 = data.center + dr;
    RS_Vector vp1 = data.center - dr;
    auto lineEqu = [&line](const RS_Vector& vp) {
        return std::abs(line.dotP(vp) + 1.);
    };
    return lineEqu(vp0) < lineEqu(vp1) ? vp0 : vp1;
}

void RS_Circle::move(const RS_Vector& offset) {
	data.center.move(offset);
    moveBorders(offset);
//    calculateBorders();
}

/**
  * this function creates offset
  *@coord, position indicates the direction of offset
  *@distance, distance of offset
  * return true, if success, otherwise, false
  *
  *Author: Dongxu Li
  */
bool RS_Circle::offset(const RS_Vector& coord, const double& distance) {
   /* bool increase = coord.x > 0;
    double newRadius;
    if (increase){
        newRadius = getRadius() + std::abs(distance);
    }
    else{
        newRadius = getRadius() - std::abs(distance);
        if(newRadius < RS_TOLERANCE) {
            return false;
        }
    }*/

    double dist(coord.distanceTo(getCenter()));
    double newRadius;
    if(dist > getRadius()){
        //external
        newRadius = getRadius()+ fabs(distance);
    }else{
        newRadius = getRadius()- fabs(distance);
        if(newRadius<RS_TOLERANCE) {
            return false;
        }
    }
    setRadius(newRadius);
    calculateBorders();
    setRadius(newRadius);
    calculateBorders();
    return true;
}

void RS_Circle::rotate(const RS_Vector& center, double angle) {
	data.center.rotate(center, angle);
    calculateBorders();
}

void RS_Circle::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
	data.center.rotate(center, angleVector);
    calculateBorders();
}

void RS_Circle::scale(const RS_Vector& center, const RS_Vector& factor) {
	data.center.scale(center, factor);
    //radius always is positive
    data.radius *= std::abs(factor.x);
    scaleBorders(center,factor);
}

double RS_Circle::getDirection1() const{
		return M_PI_2;
}

double RS_Circle::getDirection2() const {
    return M_PI_2 * 3.0;
}

void RS_Circle::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
	data.center.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}
RS_Entity& RS_Circle::shear(double k)
{
    if (!std::isnormal(k))
        assert(!"shear() should not not be called for circle");
    return *this;
}


void RS_Circle::draw(RS_Painter* painter) {
    painter->drawEntityCircle(this);
}

void RS_Circle::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if(ref.distanceTo(data.center)<1.0e-4){
        data.center += offset;
        calculateBorders();
        return;
    }
    RS_Vector v1(data.radius, 0.0);
    RS_VectorSolutions sol;
    sol.push_back(data.center + v1);
    sol.push_back(data.center - v1);
    v1.set(0., data.radius);
    sol.push_back(data.center + v1);
    sol.push_back(data.center - v1);
    double dist;
    v1=sol.getClosest(ref,&dist);
    if(dist>1.0e-4) {
        calculateBorders();
        return;
    }
    else {
        data.radius = data.center.distanceTo(v1 + offset);
        calculateBorders();
    }
}


/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Circle::getQuadratic() const
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
* @brief Returns area of full circle
* Note: Circular arcs are handled separately by RS_Arc (areaLIneIntegral) 
* However, full ellipses and ellipse arcs are handled by RS_Ellipse
* @return \pi r^2
*/
double RS_Circle::areaLineIntegral() const
{
	const double r = getRadius();
	
	return M_PI*r*r;
}


/**
 * Dumps the circle's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Circle& a) {
    os << " Circle: " << a.data << "\n";
    return os;
}
