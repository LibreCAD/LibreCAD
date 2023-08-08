/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "lc_rect.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_linetypepattern.h"
#include <rs_units.h>
#include <rs_graphic.h>
#include "rs_painter.h"
#include "rs_painterqt.h"
#include "lc_quadratic.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

std::ostream& operator << (std::ostream& os, const RS_LineData& ld) {
	os << "RS_LINE: ((" << ld.startpoint <<
		  ")(" << ld.endpoint <<
		  "))";
	return os;
}

/**
 * Constructor.
 */
RS_Line::RS_Line(RS_EntityContainer* parent,
                 const RS_LineData& d)
    :RS_AtomicEntity(parent), data(d) {
    calculateBorders();
}

////construct a line from two endpoints
RS_Line::RS_Line(RS_EntityContainer* parent, const RS_Vector& pStart, const RS_Vector& pEnd)
	:RS_AtomicEntity(parent), data({pStart,pEnd}) {
    calculateBorders();
}

////construct a line from two endpoints, to be used for construction
RS_Line::RS_Line(const RS_Vector& pStart, const RS_Vector& pEnd)
	:RS_AtomicEntity(nullptr), data({pStart,pEnd}) {
    calculateBorders();
}


RS_Entity* RS_Line::clone() const {
	RS_Line* l = new RS_Line(*this);
	l->initId();
	return l;
}



void RS_Line::calculateBorders() {
    minV = RS_Vector::minimum(data.startpoint, data.endpoint);
    maxV = RS_Vector::maximum(data.startpoint, data.endpoint);
}



RS_VectorSolutions RS_Line::getRefPoints() const
{
	return RS_VectorSolutions({data.startpoint, data.endpoint});
}


RS_Vector RS_Line::getNearestEndpoint(const RS_Vector& coord,
                                      double* dist)const {
    double dist1((data.startpoint-coord).squared());
    double dist2((data.endpoint-coord).squared());

    if (dist != nullptr)
        *dist = std::sqrt(std::min(dist1, dist2));
    return (dist1 < dist2) ? data.startpoint : data.endpoint;
}

/**
 *  This is similar to getNearestPointOnEntity, but only returns the value of
 *  the position of the projection.  The value may be negative, or greater than
 *  the length of the line since there are no bounds checking.  The absolute
 *  value of the return value represents a physical distance from the
 *  startpoint.
 *
 *  @param coord The point which will be projected onto the line.
 */
double RS_Line::getProjectionValueAlongLine(const RS_Vector& coord) const
{
    RS_Vector direction {data.endpoint - data.startpoint};
    RS_Vector vpc {coord - data.startpoint};
    double direction_magnitude {direction.magnitude()};
    double v = 0.0;

    if(direction_magnitude > RS_TOLERANCE2) {
        //find projection on line
        v = RS_Vector::dotP(vpc, direction) / direction_magnitude;
    }

    return v;
}

RS_Vector RS_Line::getNearestPointOnEntity(const RS_Vector& coord,
                                           bool onEntity,
                                           double* dist,
                                           RS_Entity** entity) const
{
    if (entity) {
        *entity = const_cast<RS_Line*>(this);
    }

    RS_Vector direction {data.endpoint - data.startpoint};
    RS_Vector vpc {coord - data.startpoint};
    double a {direction.squared()};

    if( a < RS_TOLERANCE2) {
        //line too short
        vpc = getMiddlePoint();
    }
    else {
        //find projection on line
        const double t {RS_Vector::dotP( vpc, direction) / a};
        if( !isConstruction()
            && onEntity
            && ( t <= -RS_TOLERANCE
                 || t >= 1. + RS_TOLERANCE ) ) {
            //projection point not within range, find the nearest endpoint
            return getNearestEndpoint( coord, dist);
        }

        vpc = data.startpoint + direction * t;
    }

    if (dist) {
        *dist = vpc.distanceTo( coord);
    }

    return vpc;
}

/*
RS_Vector RS_Line::getPointOnEntityAlongLine(const RS_Vector& coord,const double angle,
                                           bool onEntity,
                                           double* dist,
                                           RS_Entity** entity) const
{
    if (entity)
    {
        *entity = const_cast<RS_Line*>(this);
    }


    RS_Vector direction {data.endpoint - data.startpoint};
    RS_Vector vpc {coord - data.startpoint};
    double a {direction.squared()};

    if( a < RS_TOLERANCE2)
    {
        //line too short
        vpc = getMiddlePoint();
    }
    else
    {
        //find projection on line
        const double t {RS_Vector::dotP( vpc, direction) / a};
        if( !isConstruction()   && onEntity  && ( t <= -RS_TOLERANCE || t >= 1. + RS_TOLERANCE ) )
        {
            //projection point not within range, find the nearest endpoint
            return getNearestEndpoint( coord, dist);
        }

        vpc = data.startpoint + direction * t;
    }

    if (dist)
    {
        *dist = vpc.distanceTo( coord);
    }

    return vpc;
}
*/


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

		if (dist) {
            *dist=vp0.distanceTo(coord);
        }
//        RS_DEBUG->print("RS_Line::getNearestMiddle(): end\n");
        return vp0;
    }


//RS_Vector RS_Line::getNearestCenter(const RS_Vector& coord,
//                                    double* dist) {

//    RS_Vector p = (data.startpoint + data.endpoint)*0.5;

//    if (dist) {
//        *dist = p.distanceTo(coord);
//    }

//    return p;
//}


RS_Vector RS_Line::getNearestDist(double distance,
                                  const RS_Vector& coord,
								  double* dist) const{

	RS_Vector dv = RS_Vector::polar(distance, getAngle1());

    RS_Vector ret;
    //if(coord.distanceTo(getStartpoint()) < coord.distanceTo(getEndpoint())) {
    if( (coord-getStartpoint()).squared()<  (coord-getEndpoint()).squared() ) {
        ret = getStartpoint() + dv;
    }else{
        ret = getEndpoint() - dv;
    }
	if (dist)
        *dist=coord.distanceTo(ret);

    return ret;
}



RS_Vector RS_Line::getNearestDist(double distance,
								  bool startp) const{

    double a1 = getAngle1();

	RS_Vector dv = RS_Vector::polar(distance, a1);
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

	if (dist) {
        *dist = d;
    }

    return p;
}*/



/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Line::getQuadratic() const
{
    std::vector<double> ce(3,0.);
	auto dvp=data.endpoint - data.startpoint;
    RS_Vector normal(-dvp.y,dvp.x);
    ce[0]=normal.x;
    ce[1]=normal.y;
    ce[2]= -normal.dotP(data.endpoint);
    return LC_Quadratic(ce);
}

double RS_Line::areaLineIntegral() const
{
    return 0.5*(data.endpoint.y - data.startpoint.y)*(data.startpoint.x + data.endpoint.x);
}


RS_Vector  RS_Line::getTangentDirection(const RS_Vector& /*point*/)const{
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
	auto vp0=trimSol.getClosest(trimCoord, nullptr, 0);

    double dr2=trimCoord.squaredTo(vp0);
    //the trim point found is closer to mouse location (trimCoord) than both end points, return this trim point
    if(dr2 < trimCoord.squaredTo(getStartpoint()) && dr2 < trimCoord.squaredTo(getEndpoint())) return vp0;
    //the closer endpoint to trimCoord
    RS_Vector vp1=(trimCoord.squaredTo(getStartpoint()) <= trimCoord.squaredTo(getEndpoint()))?getStartpoint():getEndpoint();

    //searching for intersection in the direction of the closer end point
	auto dvp1=vp1 - trimCoord;
    RS_VectorSolutions sol1;
    for(size_t i=0; i<trimSol.size(); i++){
		auto dvp2=trimSol.at(i) - trimCoord;
		if( RS_Vector::dotP(dvp1, dvp2) > RS_TOLERANCE)
			sol1.push_back(trimSol.at(i));
    }
    //if found intersection in direction, return the closest to trimCoord from it
	if(sol1.size())
		return sol1.getClosest(trimCoord, nullptr, 0);

    //no intersection by direction, return previously found closest intersection
    return vp0;
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
	RS_Vector direction{getEndpoint()-getStartpoint()};
    double ds(direction.magnitude());
    if(ds< RS_TOLERANCE) return false;
    direction /= ds;
    RS_Vector vp(coord-getStartpoint());
//    RS_Vector vp1(getStartpoint() + direction*(RS_Vector::dotP(direction,vp))); //projection
    direction.set(-direction.y,direction.x); //rotate pi/2
    if(RS_Vector::dotP(direction,vp)<0.) {
        direction *= -1.;
    }
    direction*=distance;
    move(direction);
    return true;
}

bool RS_Line::isTangent(const RS_CircleData&  circleData) const{
    double d;
	getNearestPointOnEntity(circleData.center,false,&d);
    if(std::abs(d-circleData.radius)<20.*RS_TOLERANCE) return true;
    return false;
}

RS_Vector RS_Line::getNormalVector() const
{
    RS_Vector vp=data.endpoint  - data.startpoint; //direction vector
	double r=vp.magnitude();
	if (r< RS_TOLERANCE) return RS_Vector{false};
	return RS_Vector{-vp.y,vp.x}/r;
}

std::vector<RS_Entity* > RS_Line::offsetTwoSides(const double& distance) const
{
	std::vector<RS_Entity*> ret(0);
	RS_Vector const& vp=getNormalVector()*distance;
	ret.push_back(new RS_Line{data.startpoint+vp,data.endpoint+vp});
	ret.push_back(new RS_Line{data.startpoint-vp,data.endpoint-vp});
	return ret;
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
    moveBorders(offset);
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

	RS_Vector const vLow{std::min(firstCorner.x, secondCorner.x),
				std::min(firstCorner.y, secondCorner.y)
				  };
	RS_Vector const vHigh{std::max(firstCorner.x, secondCorner.x),
				std::max(firstCorner.y, secondCorner.y)
				   };

    if (getStartpoint().isInWindowOrdered(vLow, vHigh)) {
        moveStartpoint(getStartpoint() + offset);
    }
    if (getEndpoint().isInWindowOrdered(vLow, vHigh)) {
        moveEndpoint(getEndpoint() + offset);
    }
}



void RS_Line::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if(  std::abs(data.startpoint.x -ref.x)<1.0e-4 &&
         std::abs(data.startpoint.y -ref.y)<1.0e-4 ) {
        moveStartpoint(data.startpoint+offset);
    }
    if(  std::abs(data.endpoint.x -ref.x)<1.0e-4 &&
         std::abs(data.endpoint.y -ref.y)<1.0e-4 ) {
        moveEndpoint(data.endpoint+offset);
    }
}

void RS_Line::draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) {
    if (painter == nullptr && view == nullptr)
        return;
    if (isConstruction()) {

        // draw construction lines as infinite lines
        drawInfinite(*painter, *view);
    } else {

        // Adjust dash offset
        updateDashOffset(*painter, *view, patternOffset);
        painter->drawLine(view->toGui(getStartpoint()), view->toGui(getEndpoint()));
    }
}

void RS_Line::drawInfinite(RS_Painter& painter, RS_GraphicView& view)
{
    LC_Rect viewportRect = view.getViewRect();
    RS_VectorSolutions endPoints{{ getStartpoint(), getEndpoint()}};

    RS_EntityContainer borders(nullptr, true);
    borders.addRectangle(viewportRect.minP(), viewportRect.maxP());

    RS_Vector pStart{view.toGui(endPoints.at(0))};
    RS_Vector pEnd{view.toGui(endPoints.at(1))};
    //    std::cout<<"draw line: "<<pStart<<" to "<<pEnd<<std::endl;
    RS_Vector direction = pEnd-pStart;
    if (direction.squared() < RS_TOLERANCE2)
        return;

    RS_VectorSolutions vpIts;
    for(RS_Entity* border: borders) {
        auto const sol=RS_Information::getIntersection(this, border, false);
        for(const RS_Vector& vp: sol) {
            if (vpIts.getClosestDistance(vp) > RS_TOLERANCE * 10.)
                vpIts.push_back(vp);
        }
    }

    //draw construction lines up to viewport border
    switch (vpIts.size()) {
    case 2:
        // no need to sort intersections
        break;
    case 3:
    case 4: {
        // will use the inner two points
        size_t i{0};
        for (size_t j = 0; j < vpIts.size(); ++j)
            if (viewportRect.inArea(vpIts.at(j), RS_TOLERANCE * 10.))
                std::swap(vpIts[j], vpIts[i++]);

    }
        break;
    default:
        //should not happen
        return;
    }

    painter.drawLine(view.toGui(vpIts.get(0)), view.toGui(vpIts.get(1)));
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Line& l) {
    os << " Line: " << l.getData() << "\n";
    return os;
}


