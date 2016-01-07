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
#include<iostream>
#include<cmath>
#include "rs_point.h"
#include "rs_circle.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

RS_Point::RS_Point(RS_EntityContainer* parent,
                   const RS_PointData& d)
        :RS_AtomicEntity(parent), data(d) {
    calculateBorders ();
}

RS_Entity* RS_Point::clone() const {
	RS_Point* p = new RS_Point(*this);
	p->initId();
	return p;
}

RS2::EntityType RS_Point::rtti() const
{
    return RS2::EntityPoint;
}

void RS_Point::calculateBorders () {
    minV = maxV = data.pos;
}

RS_VectorSolutions RS_Point::getRefPoints() const
{
	return RS_VectorSolutions{data.pos};
}

RS_Vector RS_Point::getStartpoint() const
{
    return data.pos;
}

RS_Vector RS_Point::getEndpoint() const
{
    return data.pos;
}

RS_PointData RS_Point::getData() const
{
    return data;
}

RS_Vector RS_Point::getPos() const
{
    return data.pos;
}

RS_Vector RS_Point::getCenter() const
{
    return data.pos;
}

double RS_Point::getRadius() const
{
    return 0.;
}

bool RS_Point::isTangent(const RS_CircleData& circleData) const
{
    double const dist=data.pos.distanceTo(circleData.center);
    return fabs(dist - fabs(circleData.radius))<50.*RS_TOLERANCE;
}

void RS_Point::setPos(const RS_Vector& pos)
{
    data.pos = pos;
}

RS_Vector RS_Point::getNearestEndpoint(const RS_Vector& coord, double* dist)const {

    if (dist) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}

RS_Vector RS_Point::getNearestPointOnEntity(const RS_Vector& coord,
        bool /*onEntity*/, double* dist, RS_Entity** entity) const{
    if (dist) {
        *dist = data.pos.distanceTo(coord);
    }
    if (entity) {
        *entity = const_cast<RS_Point*>(this);
    }
    return data.pos;
}

RS_Vector RS_Point::getNearestCenter(const RS_Vector& coord, double* dist) const{

    if (dist) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}

RS_Vector RS_Point::getMiddlePoint()const{
    return data.pos;
}

RS_Vector RS_Point::getNearestMiddle(const RS_Vector& coord,
                                     double* dist,
                                     const int /*middlePoints*/)const {
    if (dist) {
        *dist = data.pos.distanceTo(coord);
    }

    return data.pos;
}

RS_Vector RS_Point::getNearestDist(double /*distance*/,
                                   const RS_Vector& /*coord*/,
								   double* dist) const{
    if (dist) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

double RS_Point::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity,
                                    RS2::ResolveLevel /*level*/,
                                                                        double /*solidDist*/)const {
    if (entity) {
        *entity = const_cast<RS_Point*>(this);
    }
    return data.pos.distanceTo(coord);
}

void RS_Point::moveStartpoint(const RS_Vector& pos) {
        data.pos = pos;
        calculateBorders();
}

void RS_Point::move(const RS_Vector& offset) {
    data.pos.move(offset);
    calculateBorders();
}

void RS_Point::rotate(const RS_Vector& center, const double& angle) {
    data.pos.rotate(center, angle);
    calculateBorders();
}

void RS_Point::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.pos.rotate(center, angleVector);
    calculateBorders();
}

void RS_Point::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.pos.scale(center, factor);
    calculateBorders();
}

void RS_Point::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.pos.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

void RS_Point::draw(RS_Painter* painter,RS_GraphicView* view, double& /*patternOffset*/) {
    if (painter==NULL || view==NULL) {
        return;
    }

    painter->drawPoint(view->toGui(getPos()));
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Point& p) {
    os << " Point: " << p.getData() << "\n";
    return os;
}

std::ostream& operator << (std::ostream& os, const RS_PointData& pd)
{
        os << "(" << pd.pos << ")";
        return os;
}

// EOF
