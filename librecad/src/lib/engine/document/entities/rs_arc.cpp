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

#include <QPainterPath>

#include "rs_arc.h"

#include <QPainterPath>

#include "lc_creation_arc.h"
#include "lc_quadratic.h"
#include "lc_rect.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

RS_ArcData::RS_ArcData(const RS_Vector& center, const double radius, const double angle1, const double angle2, const bool reversed) :
    center(center), radius(radius), angle1(angle1), angle2(angle2), reversed(reversed) {
}

void RS_ArcData::reset() {
    center = RS_Vector(false);
    radius = 0.0;
    angle1 = 0.0;
    angle2 = 0.0;
    reversed = false;
}

void RS_Arc::setCenter(const RS_Vector& center) {
    m_data.center = center;
    calculateBorders();
}

void RS_Arc::setRadius(const double radius) {
    if (RS_Math::notEqual(m_data.radius, radius)) {
        m_data.radius = radius;
        calculateBorders();
    }
}

void RS_Arc::setAngle1(const double a1) {
    if (RS_Math::notEqual(m_data.angle1, a1)) {
        m_data.angle1 = RS_Math::correctAngle(a1);
        calculateBorders();
    }
}

/** Sets new end angle. */
void RS_Arc::setAngle2(const double a2) {
    if (RS_Math::notEqual(m_data.angle2, a2)) {
        m_data.angle2 = RS_Math::correctAngle(a2);
        calculateBorders();
    }
}

void RS_Arc::setReversed(const bool r) {
    if (m_data.reversed != r) {
        m_data.reversed = r;
        // fixme - sand - SHOULD we actually swap angles? That completely changes previous behavior of arc.
        // **********************************************************************************************************************
        // fixme - if angles swap is performed, the shape of arc remains the same, just start and end point are changed.
        // fixme - but! if there is no swap of angle - the SHAPE of arc changes (as it was before).
        // fixme - Support of changing angles on swap leads to changing the generic way of how arcs are built and processed
        // fixme - and that requires quite a big modifications. In addition, it breaks existing user's experience
        // fixme - Plus, swap of angles breaks modification actions, such as mirror (probably other too..)
        // **********************************************************************************************************************
        // std::swap(data.angle1, data.angle2);
        std::swap(m_startPoint, m_endPoint);
    }
}

bool RS_ArcData::isValid() const {
	return (center.valid
            && std::isfinite(center.x)
            && std::isfinite(center.y)
            && std::isfinite(center.z)
            && std::isfinite(radius)
            && std::isfinite(angle1)
            && std::isfinite(angle2)
            && radius>RS_TOLERANCE &&
			fabs(remainder(angle1-angle2, 2.*M_PI))>RS_TOLERANCE_ANGLE);
}

std::ostream& operator <<(std::ostream& os, const RS_ArcData& ad) {
    os << "(" << ad.center << "/" << ad.radius << " " << ad.angle1 << "," << ad.angle2 << ")";
    return os;
}

/**
 * Default constructor.
 */
RS_Arc::RS_Arc(RS_EntityContainer* parent, const RS_ArcData& d)
    : LC_CachedLengthEntity(parent), m_data(d) {
    RS_Arc::calculateBorders();
}

RS_Arc::RS_Arc(const RS_ArcData& d)
    : LC_CachedLengthEntity(nullptr), m_data(d) {
    RS_Arc::calculateBorders();
}

RS_Entity* RS_Arc::clone() const {
    const auto a = new RS_Arc(*this);
    return a;
}


void RS_Arc::calculateBorders() {
    m_startPoint = m_data.center.relative(m_data.radius, m_data.angle1);
    m_endPoint = m_data.center.relative(m_data.radius, m_data.angle2);
    const LC_Rect rect{m_startPoint, m_endPoint};

    double minX = rect.lowerLeftCorner().x;
    double minY = rect.lowerLeftCorner().y;
    double maxX = rect.upperRightCorner().x;
    double maxY = rect.upperRightCorner().y;

    const double a1 = isReversed() ? m_data.angle2 : m_data.angle1;
    const double a2 = isReversed() ? m_data.angle1 : m_data.angle2;
    if (RS_Math::isAngleBetween(0.5 * M_PI, a1, a2, false)) {
        maxY = m_data.center.y + m_data.radius;
    }
    if (RS_Math::isAngleBetween(1.5 * M_PI, a1, a2, false)) {
        minY = m_data.center.y - m_data.radius;
    }
    if (RS_Math::isAngleBetween(M_PI, a1, a2, false)) {
        minX = m_data.center.x - m_data.radius;
    }
    if (RS_Math::isAngleBetween(0., a1, a2, false)) {
        maxX = m_data.center.x + m_data.radius;
    }

    m_minV.set(minX, minY);
    m_maxV.set(maxX, maxY);
    updateMiddlePoint();

    updatePaintingInfo();
    updateLength();
}

void RS_Arc::updatePaintingInfo() {
    // angles in degrees
    m_data.startAngleDegrees = RS_Math::rad2deg(m_data.reversed ? m_data.angle2 : m_data.angle1);
    m_data.otherAngleDegrees = RS_Math::rad2deg(m_data.reversed ? m_data.angle1 : m_data.angle2);
    //    double endAngle = RS_Math::rad2deg(reversed ? a1 : a2);
    m_data.angularLength = RS_Math::rad2deg(RS_Math::getAngleDifference(m_data.angle1, m_data.angle2, m_data.reversed));
    // Issue #1896: zero angular length arc is not supported, assuming 360 degree arcs
    //    if (angularLength < RS_Math::rad2deg(RS_TOLERANCE_ANGLE))
    //        angularLength = 360.;
    //
    // brute fix for #1896
    if (std::abs(m_data.angularLength) < RS_TOLERANCE_ANGLE) {
        // check whether angles are via period
        if (RS_Math::getPeriodsCount(m_data.angle1, m_data.angle2, m_data.reversed) != 0) {
            m_data.angularLength = 360; // in degrees
        }
    }
}

RS_Vector RS_Arc::getStartpoint() const {
    return m_startPoint;
}

/** @return End point of the entity. */
RS_Vector RS_Arc::getEndpoint() const {
    return m_endPoint;
}

RS_VectorSolutions RS_Arc::getRefPoints() const {
    //order: start, end, center
    //order: start, center, middle, end
    return {{getStartpoint(), m_data.center, m_middlePoint, getEndpoint()}};
}

double RS_Arc::getDirection1() const {
    return m_data.getDirection1();
}

/**
 * @return Direction 2. The angle at which the arc starts at
 * the endpoint.
 */
double RS_Arc::getDirection2() const {
    return m_data.getDirection2();
}

RS_Vector RS_Arc::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    const auto startpoint = getStartpoint();
    const auto endpoint = getEndpoint();

    const double dist1 = coord.squaredTo(startpoint);
    const double dist2 = coord.squaredTo(endpoint);

    if (dist2 < dist1) {
        if (dist != nullptr) {
            *dist = sqrt(dist2);
        }
        if (entity != nullptr) {
            *entity = const_cast<RS_Arc*>(this);
        }
        return endpoint;
    }
    if (dist != nullptr) {
        *dist = sqrt(dist1);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Arc*>(this);
    }
    return startpoint;
}

/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  *Author: Dongxu Li
  */
RS_VectorSolutions RS_Arc::getTangentPoint(const RS_Vector& point) const {
    RS_VectorSolutions ret;
    const double radius = getRadius();
    const double r2(radius * radius);
    if (r2 < RS_TOLERANCE2) {
        return ret; //circle too small
    }
    RS_Vector vp(point - getCenter());
    const double c2(vp.squared());
    if (c2 < r2 - (radius * 2. * RS_TOLERANCE)) {
        //inside point, no tangential point
        return ret;
    }
    if (c2 > r2 + (radius * 2. * RS_TOLERANCE)) {
        //external point
        RS_Vector vp1(-vp.y, vp.x);
        vp1 *= radius * sqrt(c2 - r2) / c2;
        vp *= r2 / c2;
        vp += getCenter();
        if (vp1.squared() > RS_TOLERANCE2) {
            ret.push_back(vp + vp1);
            ret.push_back(vp - vp1);
            return ret;
        }
    }
    ret.push_back(point);
    return ret;
}

RS_Vector RS_Arc::getTangentDirection(const RS_Vector& point) const {
    RS_Vector vp = isReversed() ? getCenter() - point : point - getCenter();
    return {-vp.y, vp.x};
}

RS_Vector RS_Arc::doGetNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    RS_Vector vec(false);
    if (entity != nullptr) {
        *entity = const_cast<RS_Arc*>(this);
    }

    const double angle = (coord - m_data.center).angle();
    if (!onEntity || RS_Math::isAngleBetween(angle, m_data.angle1, m_data.angle2, isReversed())) {
        vec.setPolar(m_data.radius, angle);
        vec += m_data.center;
    }
    else {
        vec = getNearestEndpoint(coord, nullptr, dist);
        return vec;
    }
    if (dist != nullptr) {
        *dist = vec.distanceTo(coord);
        //        RS_DEBUG->print(RS_Debug::D_ERROR, "distance to (%g, %g)=%g\n", coord.x,coord.y,*dist);
    }

    return vec;
}

RS_Vector RS_Arc::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    if (dist != nullptr) {
        *dist = coord.distanceTo(m_data.center);
    }
    if (centerEntity != nullptr) {
        *centerEntity = const_cast<RS_Arc*>(this);
    }
    return m_data.center;
}

/*
 * get the nearest equidistant middle points
 * @coord, coordinate
 * @middlePoints, number of equidistant middle points
 *
 */

RS_Vector RS_Arc::doGetNearestMiddle(const RS_Vector& coord, double* dist, const int middlePoints) const {
#ifndef EMU_C99
    using std::isnormal;
#endif

    RS_DEBUG->print("RS_Arc::getNearestMiddle(): begin\n");
    double amin = getAngle1();
    double amax = getAngle2();
    //std::cout<<"RS_Arc::getNearestMiddle(): middlePoints="<<middlePoints<<std::endl;
    if (!(isnormal(amin) || isnormal(amax))) {
        //whole circle, no middle point
        if (dist != nullptr) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }
    if (isReversed()) {
        std::swap(amin, amax);
    }
    double da = fmod(amax - amin + (2. * M_PI), 2. * M_PI);
    if (da < RS_TOLERANCE) {
        da = 2. * M_PI; // whole circle
    }
    RS_Vector vp(getNearestPointOnEntity(coord, true, dist));
    double angle = getCenter().angleTo(vp);
    const int counts = middlePoints + 1;
    int i = static_cast<int>(fmod(angle - amin + 2. * M_PI, 2. * M_PI) / da * counts + 0.5);
    if (i == 0) {
        i++; // remove end points
    }
    if (i == counts) {
        i--;
    }
    const double doubleI = i;
    angle = amin + (da * (doubleI / counts));
    vp.setPolar(getRadius(), angle);
    vp.move(getCenter());

    if (dist != nullptr) {
        *dist = vp.distanceTo(coord);
    }
    RS_DEBUG->print("RS_Arc::getNearestMiddle(): end\n");
    return vp;
}

RS_Vector RS_Arc::doGetNearestDist(const double distance, const RS_Vector& coord, double* dist) const {
    if (m_data.radius < RS_TOLERANCE) {
        if (dist != nullptr) {
            *dist = RS_MAXDOUBLE;
        }
        return {};
    }

    double aDist = distance / m_data.radius;
    if (isReversed()) {
        aDist = -aDist;
    }

    double a;
    if (coord.distanceTo(getStartpoint()) < coord.distanceTo(getEndpoint())) {
        a = getAngle1() + aDist;
    }
    else {
        a = getAngle2() - aDist;
    }

    RS_Vector ret = RS_Vector::polar(m_data.radius, a);
    ret += getCenter();

    return ret;
}

RS_Vector RS_Arc::getNearestDistToEndpoint(const double distance, const bool startp) const {
    if (m_data.radius < RS_TOLERANCE) {
        return {};
    }

    const double aDist = distance / m_data.radius;
    double a;
    if (isReversed()) {
        if (startp) {
            a = m_data.angle1 - aDist;
        }
        else {
            a = m_data.angle2 + aDist;
        }
    }
    else {
        if (startp) {
            a = m_data.angle1 + aDist;
        }
        else {
            a = m_data.angle2 - aDist;
        }
    }

    RS_Vector p = RS_Vector::polar(m_data.radius, a);
    p += m_data.center;

    return p;
}

RS_Vector RS_Arc::getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, const bool onEntity) const {
    if (!coord.valid) {
        return RS_Vector(false);
    }
    double angle = normal.getAngle1();
    RS_Vector vp = RS_Vector::polar(getRadius(), angle);
    std::vector<RS_Vector> sol;
    for (int i = 0; i <= 1; i++) {
        if (!onEntity || RS_Math::isAngleBetween(angle, getAngle1(), getAngle2(), isReversed())) {
            if (i != 0) {
                sol.push_back(-vp);
            }
            else {
                sol.push_back(vp);
            }
        }
        angle = RS_Math::correctAngle(angle + M_PI);
    }
    switch (sol.size()) {
        case 0:
            return RS_Vector(false);
        case 2:
            if (RS_Vector::dotP(sol[1], coord - getCenter()) > 0.) {
                vp = sol[1];
                break;
            }
        // fall-through
        default:
            vp = sol[0];
            break;
    }
    return getCenter() + vp;
}

RS_Vector RS_Arc::dualLineTangentPoint(const RS_Vector& line) const {
    const RS_Vector dr = line.normalized() * m_data.radius;
    const RS_Vector vp0 = m_data.center + dr;
    const RS_Vector vp1 = m_data.center - dr;
    auto lineEqu = [&line](const RS_Vector& vp) {
        return std::abs(line.dotP(vp) + 1.);
    };
    return lineEqu(vp0) < lineEqu(vp1) ? vp0 : vp1;
}

void RS_Arc::moveStartpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
    //if (parent && parent->rtti()==RS2::EntityPolyline) {
    const double bulge = getBulge();
    if (fabs(bulge - M_PI_2) < RS_TOLERANCE_ANGLE) {
        return;
    }
    LC_CreationArc::createFrom2PBulge(pos, getEndpoint(), bulge, m_data);
    calculateBorders();
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    //}
}

void RS_Arc::moveEndpoint(const RS_Vector& pos) {
    // polyline arcs: move point not angle:
    //if (parent && parent->rtti()==RS2::EntityPolyline) {
    const double bulge = getBulge();
    LC_CreationArc::createFrom2PBulge(getStartpoint(), pos, bulge, m_data);
    calculateBorders();
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
bool RS_Arc::offset(const RS_Vector& coord, const double distance) {
    /*  bool increase = coord.x > 0;
      double newRadius;
      if (increase){
          newRadius = getRadius() + std::abs(distance);
      }
      else{
          newRadius = getRadius() - std::abs(distance);
          if(newRadius < RS_TOLERANCE) {
              return false;
          }
      }
      */
    const double dist(coord.distanceTo(getCenter()));
    double newRadius = NAN;
    if (dist > getRadius()) {
        //external
        newRadius = getRadius() + fabs(distance);
    }
    else {
        newRadius = getRadius() - fabs(distance);
        if (newRadius < RS_TOLERANCE) {
            return false;
        }
    }
    setRadius(newRadius);
    calculateBorders();
    return true;
}

std::vector<RS_Entity*> RS_Arc::offsetTwoSides(const double distance) const {
    std::vector<RS_Entity*> ret(0, nullptr);
    const double radius = getRadius();
    const double angle1 = getAngle1();
    const double angle2 = getAngle2();
    const bool reversed = isReversed();
    const auto center = getCenter();
    ret.push_back(new RS_Arc(nullptr, RS_ArcData(center, radius + distance, angle1, angle2, reversed)));
    if (radius > distance) {
        ret.push_back(new RS_Arc(nullptr, RS_ArcData(center, radius - distance, angle1, angle2, reversed)));
    }
    return ret;
}

/**
      * implementations must revert the direction of an atomic entity
      */
void RS_Arc::revertDirection() {
    std::swap(m_data.angle1, m_data.angle2);
    m_data.reversed = !m_data.reversed;
    std::swap(m_startPoint, m_endPoint);
}

/**
 * make sure angleLength() is not more than 2*M_PI
 */
void RS_Arc::correctAngles() {
    double* pa1 = &m_data.angle1;
    double* pa2 = &m_data.angle2;
    if (isReversed()) {
        std::swap(pa1, pa2);
    }
    *pa2 = *pa1 + fmod(*pa2 - *pa1, 2. * M_PI);
    if (fabs(getAngleLength()) < RS_TOLERANCE_ANGLE) {
        *pa2 += 2. * M_PI;
    }
}

void RS_Arc::trimStartpoint(const RS_Vector& pos) {
    m_data.angle1 = m_data.center.angleTo(pos);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

void RS_Arc::trimEndpoint(const RS_Vector& pos) {
    m_data.angle2 = m_data.center.angleTo(pos);
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

/**
  *@ trimCoord, mouse point
  *@  trimPoint, trim to this intersection point
  */
RS2::Ending RS_Arc::getTrimPoint(const RS_Vector& trimCoord, const RS_Vector& /*trimPoint*/) {
    //double angEl = data.center.angleTo(trimPoint);
    const double angMouse = m_data.center.angleTo(trimCoord);
    //    double angTrim = data.center.angleTo(trimPoint);
    if (fabs(remainder(angMouse - m_data.angle1, 2. * M_PI)) < fabs(remainder(angMouse - m_data.angle2, 2. * M_PI))) {
        return RS2::EndingStart;
    }
    return RS2::EndingEnd;

    //    if( RS_Math::isAngleBetween(angMouse , data.angle1, angTrim, isReversed())) {

    //        return RS2::EndingEnd;
    //    } else {

    //        return RS2::EndingStart;
    //    }
}

RS_Vector RS_Arc::prepareTrim(const RS_Vector& trimCoord, const RS_VectorSolutions& trimSol) {
    //special trimming for ellipse arc
    RS_DEBUG->print("RS_Arc::prepareTrim(): begin");
    for (auto&& intersection : trimSol) {
        LC_LOG << "RS_Arc::prepareTrim(): line " << __LINE__ << "intersection: angle=" << getArcAngle(intersection);
    }

    if (!trimSol.hasValid()) {
        return RS_Vector(false);
    }
    LC_LOG << "RS_Arc::prepareTrim(): line " << __LINE__ << "trimCoord: angle=" << getArcAngle(trimCoord);
    if (trimSol.getNumber() == 1) {
        return trimSol.get(0);
    }
    // The angle at trimCoord
    const double am = getArcAngle(trimCoord);
    std::vector<double> ias;
    double ia(0.);
    double ia2(0.);
    RS_Vector is;
    RS_Vector is2;
    //find the closest intersection to the trimCoord, according angular difference
    for (size_t ii = 0; ii < trimSol.getNumber(); ++ii) {
        ias.push_back(getArcAngle(trimSol.get(ii)));
        if ((ii == 0u) || fabs(remainder(ias[ii] - am, 2 * M_PI)) < fabs(remainder(ia - am, 2 * M_PI))) {
            ia = ias[ii];
            is = trimSol.get(ii);
        }
    }
    std::sort(ias.begin(), ias.end());
    //find segment to include trimCoord
    for (size_t ii = 0; ii < trimSol.getNumber(); ++ii) {
        if (!RS_Math::isSameDirection(ia, ias[ii],RS_TOLERANCE)) {
            continue;
        }
        if (RS_Math::isAngleBetween(am, ias[(ii + trimSol.getNumber() - 1) % trimSol.getNumber()], ia, false)) {
            ia2 = ias[(ii + trimSol.getNumber() - 1) % trimSol.getNumber()];
        }
        else {
            ia2 = ias[(ii + 1) % trimSol.getNumber()];
        }
        break;
    }
    LC_LOG << "RS_Arc::prepareTrim(): line " << __LINE__ << ": angle1=" << getAngle1() << " angle2=" << getAngle2() << " am=" << am <<
        " is=" << getArcAngle(is) << " ia2=" << ia2;
    //find segment to include trimCoord
    for (const RS_Vector& vp : trimSol) {
        if (!RS_Math::isSameDirection(ia2, getArcAngle(vp),RS_TOLERANCE)) {
            continue;
        }
        is2 = vp;
        break;
    }
    const double dia = fabs(remainder(ia - am, 2 * M_PI));
    const double dia2 = fabs(remainder(ia2 - am, 2 * M_PI));
    const double aiMin = std::min(dia, dia2);
    double da1 = fabs(remainder(getAngle1() - am, 2 * M_PI));
    double da2 = fabs(remainder(getAngle2() - am, 2 * M_PI));
    const double daMin = std::min(da1, da2);
    if (daMin < aiMin) {
        //trimming one end of arc
        const bool irev = RS_Math::isAngleBetween(am, ia2, ia, isReversed());
        if (RS_Math::isAngleBetween(ia, getAngle1(), getAngle2(), isReversed()) && RS_Math::isAngleBetween(
            ia2, getAngle1(), getAngle2(), isReversed())) {
            //
            if (irev) {
                setAngle2(ia);
                setAngle1(ia2);
                calculateBorders();
            }
            else {
                setAngle1(ia);
                setAngle2(ia2);
                calculateBorders();
            }
            da1 = fabs(remainder(getAngle1() - am, 2 * M_PI));
            da2 = fabs(remainder(getAngle2() - am, 2 * M_PI));
        }
        if (((da1 < da2 - RS_TOLERANCE_ANGLE) && (RS_Math::isAngleBetween(ia2, ia, getAngle1(), isReversed()))) || ((da1 > da2 -
            RS_TOLERANCE_ANGLE) && (RS_Math::isAngleBetween(ia2, getAngle2(), ia, isReversed())))) {
            std::swap(is, is2);
            LC_LOG << "reset: angle1=" << getAngle1() << " angle2=" << getAngle2() << " am=" << am << " is=" << getArcAngle(is) << " ia2="
                << ia2;
        }
    }
    else {
        //choose intersection as new end
        if (dia > dia2) {
            std::swap(is, is2);
            std::swap(ia, ia2);
        }
        if (RS_Math::isAngleBetween(ia, getAngle1(), getAngle2(), isReversed())) {
            if (std::abs(ia - getAngle1()) > RS_TOLERANCE_ANGLE && RS_Math::isAngleBetween(am, getAngle1(), ia, isReversed())) {
                setAngle2(ia);
            }
            else {
                setAngle1(ia);
            }
            calculateBorders();
        }
    }
    LC_LOG << "RS_Arc::prepareTrim(): line " << __LINE__ << ": angle1=" << getAngle1() << " angle2=" << getAngle2() << " am=" << am <<
        " is=" << getArcAngle(is) << " ia2=" << ia2;
    RS_DEBUG->print("RS_Arc::prepareTrim(): end");
    return is;
}

void RS_Arc::reverse() {
    std::swap(m_data.angle1, m_data.angle2);
    m_data.reversed = !m_data.reversed;
    calculateBorders();
}

void RS_Arc::move(const RS_Vector& offset) {
    m_data.center.move(offset);
    calculateBorders();
}

void RS_Arc::rotate(const RS_Vector& center, const double angle) {
    RS_DEBUG->print("RS_Arc::rotate");
    m_data.center.rotate(center, angle);
    m_data.angle1 = RS_Math::correctAngle(m_data.angle1 + angle);
    m_data.angle2 = RS_Math::correctAngle(m_data.angle2 + angle);
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}

void RS_Arc::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_DEBUG->print("RS_Arc::rotate");
    m_data.center.rotate(center, angleVector);
    const double angle(angleVector.angle());
    m_data.angle1 = RS_Math::correctAngle(m_data.angle1 + angle);
    m_data.angle2 = RS_Math::correctAngle(m_data.angle2 + angle);
    calculateBorders();
    RS_DEBUG->print("RS_Arc::rotate: OK");
}

void RS_Arc::scale(const RS_Vector& center, const RS_Vector& factor) {
    // negative scaling: mirroring
    if (factor.x < 0.0) {
        mirror(m_data.center, m_data.center + RS_Vector(0.0, 1.0));
        //factor.x*=-1;
    }
    if (factor.y < 0.0) {
        mirror(m_data.center, m_data.center + RS_Vector(1.0, 0.0));
        //factor.y*=-1;
    }

    m_data.center = m_data.center.scale(center, factor);
    m_data.radius *= factor.x;
    m_data.radius = fabs(m_data.radius);
    //todo, does this handle negative factors properly?
    calculateBorders();
}

/**
     * @description:    Implementation of the Shear/Skew the entity
     *                  The shear transform is
     *                  1  k  0
     *                  0  1  0
     *                        1
     * @author          Dongxu Li
     * @param k k the skew/shear parameter
     */
RS_Entity& RS_Arc::shear(const double k) {
    if (!std::isnormal(k)) {
        assert(!"shear(): cannot be called for arc");
    }
    return *this;
}

void RS_Arc::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.center.mirror(axisPoint1, axisPoint2);
    setReversed(!isReversed());
    const double a = (axisPoint2 - axisPoint1).angle() * 2;
    setAngle1(RS_Math::correctAngle(a - getAngle1()));
    setAngle2(RS_Math::correctAngle(a - getAngle2()));
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    updatePaintingInfo();
    calculateBorders();
}

void RS_Arc::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    //avoid moving start/end points for full circle arcs
    //as start/end points coincident
    if (fabs(fabs(getAngleLength() - M_PI) - M_PI) < RS_TOLERANCE_ANGLE) {
        move(offset);
        return;
    }
    const auto refs = getRefPoints();
    double dMin;
    size_t index;
    const RS_Vector vp = refs.getClosest(ref, &dMin, &index);
    if (dMin >= 1.0e-4) {
        return;
    }

    //reference points must be by the order: start, end, center
    //order: start, center, middle, end
    switch (index) {
        case 0: // start
            moveStartpoint(vp + offset);
            return;
        case 1: // center
            move(offset);
            return;
        case 2: // middlepoint
            moveMiddlePoint(vp + offset);
            return;
        case 3: // endpoint
            moveEndpoint(vp + offset);
            return;
        default:
            move(offset);
            break;
    }

    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

void RS_Arc::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    }
    else {
        if (getStartpoint().isInWindow(firstCorner, secondCorner)) {
            moveStartpoint(getStartpoint() + offset);
        }
        if (getEndpoint().isInWindow(firstCorner, secondCorner)) {
            moveEndpoint(getEndpoint() + offset);
        }
    }
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

void RS_Arc::createPainterPath(RS_Painter* painter, QPainterPath& path) const {
    const double baseAngle = getAngle1();
    const double fullAngleLength = isReversed() ? - getAngleLength() : getAngleLength();
    auto getParamFunc = [this](const RS_Vector& vp) { return getArcAngle(vp); };
    auto getPointFunc = [this](double param) { return getPointAtParameter(param); };
    painter->pathForEntity(path, this, baseAngle, fullAngleLength, getParamFunc, getPointFunc, getRadius());
}

void RS_Arc::draw(RS_Painter* painter) {
// fixme - sand - merge - runtime overhead and render performance degradatation. May be checked only once, not in EVERY DRAW!!!! Just a silly check
  if (painter == nullptr
      || !m_data.center.valid
      || !std::isfinite(m_data.center.x)
      || !std::isfinite(m_data.center.y)
      || !std::isfinite(m_data.center.z)
      || !std::isfinite(m_data.radius)
      || !std::isfinite(m_data.angle1)
      || !std::isfinite(m_data.angle2)
      || m_data.radius <= 0.) {
    return;
  }

  const double radiusUi = painter->toGuiDX(getRadius());
  if (!std::isfinite(radiusUi) || radiusUi <= 0.) {
    return;
  }

  if (radiusUi < RS_Painter::getMaximumArcNonErrorRadius()) {
    painter->drawEntityArc(this);
  } else {
    QPainterPath path;
    createPainterPath(painter, path);
    if (path.isEmpty()) {
      return;
    }

    painter->drawPath(path);
  }
}

/**
 * @return Middle point of the entity.
 */
RS_Vector RS_Arc::getMiddlePoint() const {
    return m_middlePoint;
}

/**
 * @return Length of the arc.
 */
void RS_Arc::updateLength() {
    m_cachedLength = getAngleLength() * m_data.radius;
}

/**
 * Gets the arc's bulge (tangens of angle length divided by 4).
 */
double RS_Arc::getBulge() const {
    return m_data.getBulge();
}

double RS_Arc::getSagitta() const {
    const double radius = m_data.radius;
    const double chord = m_endPoint.distanceTo(m_startPoint);
    const double result = radius - std::sqrt((radius * radius) - chord * chord / 4);
    return result;
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Arc::getQuadratic() const {
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[2] = 1.;
    ce[5] = -m_data.radius * m_data.radius;
    LC_Quadratic ret(ce);
    ret.move(m_data.center);
    return ret;
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 * \oint x dy = c_x r \sin t + \frac{1}{4}r^2\sin 2t +  \frac{1}{2}r^2 t
 */
double RS_Arc::areaLineIntegral() const {
  // Original implementation (kept unchanged)
  const double &r = m_data.radius;
  const double &a0 = m_data.angle1;
  const double &a1 = m_data.angle2;
  const double r2 = 0.25 * r * r;
  const double fStart = m_data.center.x * r * sin(a0) + r2 * sin(a0 + a0);
  const double fEnd = m_data.center.x * r * sin(a1) + r2 * sin(a1 + a1);
  if (isReversed()) {
      return fEnd - fStart - 2. * r2 * getAngleLength();
  }
  return fEnd - fStart + 2. * r2 * getAngleLength();
}

LC_FirstMoment RS_Arc::firstMomentLineIntegral() const {
    const double cx = m_data.center.x;
    const double cy = m_data.center.y;
    const double r  = m_data.radius;
    const double a0 = m_data.angle1;
    const double L  = getAngleLength();
    const double r2 = r * r;

    // mx = (1/2) ∮ x² dy = (r/2) ∫ (cx + r cos t)² cos t dt
    // Anti-derivative periodic part: F(t) = (r/2)[cx²·sin t + cx·r·sin(2t)/2 + r²·(sin t − sin³t/3)]
    // Linear coefficient: k_mx = cx·r²/2
    auto F_mx = [&](double t) -> double {
        const double st  = std::sin(t);
        const double s2t = std::sin(2.0 * t);
        return (r / 2.0) * (
            cx * cx * st
            + cx * r * s2t / 2.0
            + r2 * (st - st * st * st / 3.0)
        );
    };

    // my = −(1/2) ∮ y² dx = (r/2) ∫ (cy + r sin t)² sin t dt
    // Anti-derivative periodic part: G(t) = (r/2)[−cy²·cos t − cy·r·sin(2t)/2 + r²·(−cos t + cos³t/3)]
    // Linear coefficient: k_my = cy·r²/2
    auto G_my = [&](double t) -> double {
        const double ct  = std::cos(t);
        const double s2t = std::sin(2.0 * t);
        return (r / 2.0) * (
            -cy * cy * ct
            - cy * r * s2t / 2.0
            + r2 * (-ct + ct * ct * ct / 3.0)
        );
    };

    const double k_mx = cx * r2 / 2.0;
    const double k_my = cy * r2 / 2.0;
    const double sign = isReversed() ? -1.0 : 1.0;
    const double a1   = a0 + sign * L;

    return {
        F_mx(a1) - F_mx(a0) + sign * k_mx * L,
        G_my(a1) - G_my(a0) + sign * k_my * L
    };
}

LC_SecondMoment RS_Arc::secondMomentLineIntegral() const {
    const double cx = m_data.center.x;
    const double cy = m_data.center.y;
    const double r  = m_data.radius;
    const double a0 = m_data.angle1;
    const double a1 = m_data.angle2;
    const double L  = getAngleLength();
    const double r2 = r * r;
    const double r3 = r2 * r;
    const double r4 = r2 * r2;

    // ixx = (r/3) ∫ (cx + r cos t)³ cos t dt
    // Anti-derivative (periodic part only):
    //   F_nt(t) = (r/3)[(cx³+3cx·r²)sin t + (3cx²r/4+r³/4)sin 2t + r³/32 sin 4t - cx·r²·sin³t]
    // t-linear coefficient: k_ixx = cx²r²/2 + r⁴/8
    auto F_nt = [&](double t) -> double {
        const double st  = std::sin(t);
        const double s2t = std::sin(2.0*t);
        const double s4t = std::sin(4.0*t);
        return (r / 3.0) * (
            (cx*cx*cx + 3.0*cx*r2) * st
            + (3.0*cx*cx*r / 4.0 + r3 / 4.0) * s2t
            + r3 * s4t / 32.0
            - cx * r2 * st*st*st
        );
    };
    const double k_ixx = cx*cx*r2 / 2.0 + r4 / 8.0;

    // iyy = (r/3) ∫ (cy + r sin t)³ sin t dt
    // Anti-derivative (periodic part only):
    //   G_nt(t) = (r/3)[-(cy³+3cy·r²)cos t + cy·r²·cos³t - (3cy²r/4+r³/4)sin 2t + r³/32 sin 4t]
    // t-linear coefficient: k_iyy = cy²r²/2 + r⁴/8
    auto G_nt = [&](double t) -> double {
        const double ct  = std::cos(t);
        const double s2t = std::sin(2.0*t);
        const double s4t = std::sin(4.0*t);
        return (r / 3.0) * (
            -(cy*cy*cy + 3.0*cy*r2) * ct
            + cy * r2 * ct*ct*ct
            - (3.0*cy*cy*r / 4.0 + r3 / 4.0) * s2t
            + r3 * s4t / 32.0
        );
    };
    const double k_iyy = cy*cy*r2 / 2.0 + r4 / 8.0;

    // ixy = (r/2) ∫ (cx+r cos t)²(cy+r sin t) cos t dt
    // Anti-derivative (periodic part only):
    //   H_nt(t) = (r/2)[(cx²cy+r²cy)sin t + cx²r·sin²t/2 + cx·r·cy·sin 2t/2
    //                   - 2cx·r²·cos³t/3 - r²cy·sin³t/3 - r³·cos⁴t/4]
    // t-linear coefficient: k_ixy = cx·cy·r²/2
    auto H_nt = [&](double t) -> double {
        const double st  = std::sin(t);
        const double ct  = std::cos(t);
        const double s2t = std::sin(2.0*t);
        return (r / 2.0) * (
            (cx*cx*cy + r2*cy) * st
            + cx*cx*r * st*st / 2.0
            + cx*r*cy * s2t / 2.0
            - 2.0*cx*r2 * ct*ct*ct / 3.0
            - r2*cy * st*st*st / 3.0
            - r3 * ct*ct*ct*ct / 4.0
        );
    };
    const double k_ixy = cx*cy*r2 / 2.0;

    const double sign = isReversed() ? -1.0 : 1.0;
    return {
        F_nt(a1) - F_nt(a0) + sign * k_ixx * L,
        G_nt(a1) - G_nt(a0) + sign * k_iyy * L,
        H_nt(a1) - H_nt(a0) + sign * k_ixy * L
    };
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Arc& a) {
    os << " Arc: " << a.m_data << "\n";
    return os;
}

void RS_Arc::updateMiddlePoint() {
    double a = getAngle1();
    const double b = getAngle2();

    if (isReversed()) {
        a = b + (RS_Math::correctAngle(a - b) * 0.5);
    }
    else {
        a += RS_Math::correctAngle(b - a) * 0.5;
    }
    m_middlePoint = getCenter() + RS_Vector::polar(getRadius(), a);
}

void RS_Arc::moveMiddlePoint(const RS_Vector& vector) {
    RS_ArcData arcData;
    const bool success = LC_CreationArc::createFrom3P(m_startPoint, vector, m_endPoint, arcData);
    if (success) {
        m_data.center = arcData.center;
        m_data.radius = arcData.radius;
        m_data.angle1 = arcData.angle1;
        m_data.angle2 = arcData.angle2;
        m_data.reversed = arcData.reversed;
        calculateBorders();
    }
}
