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


#include "rs_constructionline.h"

#include "rs_debug.h"
#include "lc_quadratic.h"
#include "rs_math.h"

RS_ConstructionLineData::RS_ConstructionLineData():
	point1(false),
	point2(false)
{}

RS_ConstructionLineData::RS_ConstructionLineData(const RS_Vector& point1,
						const RS_Vector& point2):
	point1(point1)
	,point2(point2)
{
}

std::ostream& operator << (std::ostream& os,
								  const RS_ConstructionLineData& ld)
{
	os << "(" << ld.point1 <<
	"/" << ld.point2 <<
	")";
	return os;
}

/**
 * Constructor.
 */
RS_ConstructionLine::RS_ConstructionLine(RS_EntityContainer* parent,
        const RS_ConstructionLineData& d)
        :RS_AtomicEntity(parent), data(d) {

    calculateBorders();
}

RS_Entity* RS_ConstructionLine::clone() const {
    RS_ConstructionLine* c = new RS_ConstructionLine(*this);
    c->initId();
    return c;
}

void RS_ConstructionLine::calculateBorders() {
    minV = RS_Vector::minimum(data.point1, data.point2);
    maxV = RS_Vector::maximum(data.point1, data.point2);
}

RS_Vector RS_ConstructionLine::getNearestEndpoint(const RS_Vector& coord,
        double* dist) const{
    double dist1, dist2;

    dist1 = (data.point1-coord).squared();
    dist2 = (data.point2-coord).squared();

    if (dist2<dist1) {
		if (dist) {
            *dist = sqrt(dist2);
        }
        return data.point2;
    } else {
		if (dist) {
            *dist = sqrt(dist1);
        }
        return data.point1;
    }
}

RS_Vector RS_ConstructionLine::getNearestPointOnEntity(const RS_Vector& coord,
        bool /*onEntity*/, double* /*dist*/, RS_Entity** entity) const{

	if (entity) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }

    RS_Vector ae = data.point2-data.point1;
    RS_Vector ea = data.point1-data.point2;
    RS_Vector ap = coord-data.point1;
//    RS_Vector ep = coord-data.point2;

        if (ae.magnitude()<1.0e-6 || ea.magnitude()<1.0e-6) {
                return RS_Vector(false);
        }

    // Orthogonal projection from both sides:
    RS_Vector ba = ae * RS_Vector::dotP(ae, ap)
                   / (ae.magnitude()*ae.magnitude());
//    RS_Vector be = ea * RS_Vector::dotP(ea, ep)
//                   / (ea.magnitude()*ea.magnitude());

    return data.point1+ba;
}

RS_Vector RS_ConstructionLine::getNearestCenter(const RS_Vector& /*coord*/,
		double* dist) const{

	if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}

/** @return Copy of data that defines the line. */
RS_ConstructionLineData const& RS_ConstructionLine::getData() const {
	return data;
}

/** @return First definition point. */
RS_Vector const& RS_ConstructionLine::getPoint1() const {
	return data.point1;
}
/** @return Second definition point. */
RS_Vector const& RS_ConstructionLine::getPoint2() const {
	return data.point2;
}

/** @return Start point of the entity */
RS_Vector RS_ConstructionLine::getStartpoint() const
{
    return data.point1;
}

/** @return End point of the entity */
RS_Vector RS_ConstructionLine::getEndpoint() const
{
    return data.point2;
}

/**
 * @return Direction 1. The angle at which the arc starts at
 * the startpoint.
 */
double RS_ConstructionLine::getDirection1(void) const
{
    return RS_Math::correctAngle( data.point1.angleTo( data.point2));
}

/**
 * @return Direction 2. The angle at which the arc starts at
 * the endpoint.
 */
double RS_ConstructionLine::getDirection2(void) const
{
    return RS_Math::correctAngle( data.point2.angleTo( data.point1));
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_ConstructionLine::getQuadratic() const
{
    std::vector<double> ce(3,0.);
	auto dvp=data.point2 - data.point1;
    RS_Vector normal(-dvp.y,dvp.x);
    ce[0]=normal.x;
    ce[1]=normal.y;
    ce[2]= -normal.dotP(data.point2);
    return LC_Quadratic(ce);
}

RS_Vector RS_ConstructionLine::getMiddlePoint() const{
    return RS_Vector(false);
}
RS_Vector RS_ConstructionLine::getNearestMiddle(const RS_Vector& /*coord*/,
        double* dist, const int /*middlePoints*/)const {
	if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}



RS_Vector RS_ConstructionLine::getNearestDist(double /*distance*/,
        const RS_Vector& /*coord*/,
		double* dist) const{
	if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}


double RS_ConstructionLine::getDistanceToPoint(const RS_Vector& coord,
        RS_Entity** entity,
        RS2::ResolveLevel /*level*/, double /*solidDist*/) const {

    RS_DEBUG->print("RS_ConstructionLine::getDistanceToPoint");

	if (entity) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }
    //double dist = RS_MAXDOUBLE;
    RS_Vector se = data.point2-data.point1;
    double d(se.magnitude());
    if(d<RS_TOLERANCE) {
        //line too short
        return RS_MAXDOUBLE;
    }
    se.set( se.x/d,-se.y/d); //normalized
    RS_Vector vpc= coord - data.point1;
    vpc.rotate(se); // rotate to use the line as x-axis, and the distance is fabs(y)
    return ( fabs(vpc.y) );
}



void RS_ConstructionLine::move(const RS_Vector& offset) {
    data.point1.move(offset);
    data.point2.move(offset);
    //calculateBorders();
}



void RS_ConstructionLine::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    data.point1.rotate(center, angleVector);
    data.point2.rotate(center, angleVector);
    //calculateBorders();
}

void RS_ConstructionLine::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.point1.rotate(center, angleVector);
    data.point2.rotate(center, angleVector);
    //calculateBorders();
}

void RS_ConstructionLine::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.point1.scale(center, factor);
    data.point2.scale(center, factor);
    //calculateBorders();
}



void RS_ConstructionLine::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
        data.point1.mirror(axisPoint1, axisPoint2);
        data.point2.mirror(axisPoint1, axisPoint2);
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_ConstructionLine& l) {
    os << " ConstructionLine: " << l.getData() << "\n";
    return os;
}


