/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011-2015 Dongxu Li (dongxuli2011@gmail.com)
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

#include <QPainterPath>

#include "lc_creation_ellipse.h"
#include "lc_quadratic.h"
#include "lc_rect.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "lc_splinepoints.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif
// Workaround for Qt bug: https://bugreports.qt-project.org/browse/QTBUG-22829
// TODO: the Q_MOC_RUN detection shouldn't be necessary after this Qt bug is resolved
#ifndef Q_MOC_RUN
#include <boost/version.hpp>
#include <boost/math/special_functions/ellint_2.hpp>
#include <boost/math/tools/roots.hpp>
#if BOOST_VERSION > 104500
#include <boost/tuple/tuple.hpp>
#endif
#endif

namespace {
    //functor to solve for distance, used by snapDistance
    class EllipseDistanceFunctor {
    public:
        EllipseDistanceFunctor(const RS_Ellipse& ellipse, const double target) :
            m_distance{target}, m_e{ellipse}, m_ra{m_e.getMajorRadius()}, m_k2{1. - (m_e.getRatio() * m_e.getRatio())},
            m_k2Ra{m_k2 * m_ra} {
        }
#if BOOST_VERSION > 104500
        boost::tuples::tuple<double, double, double> operator()(const double z) const {
#else
            boost::fusion::tuple<double, double, double> operator()(const double& z) const {

#endif
            const double cz = std::cos(z);
            const double sz = std::sin(z);
            //delta amplitude
            const double d = std::sqrt(1 - (m_k2 * sz * sz));
            // return f(x), f'(x) and f''(x)
#if BOOST_VERSION > 104500
            return boost::tuples::make_tuple(
#else
                return boost::fusion::make_tuple(
#endif
                m_e.getEllipseLength(z) - m_distance, m_ra * d, m_k2Ra * sz * cz / d);
        }

    private:
        double m_distance;
        const RS_Ellipse& m_e;
        const double m_ra;
        const double m_k2;
        const double m_k2Ra;
    };

    /**
     * @brief getNearestDistHelper find end point after trimmed by amount
     * @param e ellipse which is not reversed, assume ratio (a/b) >= 1
     * @param trimAmount the length of the trimmed is increased by this amount
     * @param coord current mouse position
     * @param dist if this pointer is not nullptr, save the distance from the new
     * end point to mouse position coord
     * @return the new end point of the trimmed. Only one end of the entity is
     *  trimmed
     */
    RS_Vector getNearestDistHelper(const RS_Ellipse& e, const double trimAmount, const RS_Vector& coord, double* dist = nullptr) {
        const double x1 = e.getAngle1();

        const double guess = x1 + M_PI;
        constexpr int digits = std::numeric_limits<double>::digits;

        double trimmed = e.getLength() + trimAmount;

        // choose the end to trim by the mouse position coord
        const bool trimEnd = coord.squaredTo(e.getStartpoint()) <= coord.squaredTo(e.getEndpoint());

        if (trimEnd) {
            const double wholeLength = e.getEllipseLength(0, 0); // start/end angle 0 is used for whole ellipses
            trimmed = trimAmount > 0 ? wholeLength - trimAmount : -trimAmount;
        }

        //solve equation of the distance by second order newton_raphson
        const EllipseDistanceFunctor X{e, trimmed};
        using namespace boost::math::tools;
        const double sol = halley_iterate<EllipseDistanceFunctor, double>(X, guess, x1, x1 + (2 * M_PI) - RS_TOLERANCE_ANGLE, digits);

        const RS_Vector vp = e.getEllipsePoint(sol);
        if (dist != nullptr) {
            *dist = vp.distanceTo(coord);
        }
        return vp;
    }

    /**
     * @brief The ClosestElliptic class: find the closest point on an ellipse for a given point.
     * Intended for ellipses with small eccentricities.
     * Algorithm: Newton-Raphson
     * Added for issue #1653
     */
    class ClosestEllipticPoint {
    public:
        ClosestEllipticPoint(const double a, const double b, const RS_Vector& point) :
            m_point{point}, c2{(b * b) - (a * a)}, ax2{2. * a * point.x}, by2{2. * b * point.y} {
        }

        // The elliptic angle of the closest point on ellipse.
        double getTheta() const {
            double theta = std::atan2(m_point.y, m_point.x);
            // find the zero point of the first order derivative by Newton-Raphson
            // the convergence should be good: maximum 16 recursions
            for (short i = 0; i < 16; ++i) {
                // The first and second derivatives over theta
                const double d1 = ds2D1(theta);
                const double d2 = ds2D2(theta);
                if (std::abs(d2) < RS_TOLERANCE || std::abs(d1) < RS_TOLERANCE) {
                    break;
                }
                // Newton-Raphson
                theta -= d1 / d2;
            }
            return theta;
        }

    private:
        // The first order derivative of ds2=dx^2+dy^2 over theta
        double ds2D1(const double t) const {
            using namespace std;
            return (c2 * sin(2. * t)) + (ax2 * sin(t)) - (by2 * cos(t));
        }

        // The second order derivative of ds2=dx^2+dy^2 over theta
        double ds2D2(const double t) const {
            using namespace std;
            return (2. * c2 * cos(2. * t)) + (ax2 * cos(t)) + (by2 * sin(t));
        }

        RS_Vector m_point{};
        double c2 = 0.;
        double ax2 = 0.;
        double by2 = 0.;
    };

    /**
     * @brief The EllipseBorderHelper class a helper class to avoid infinite loop due to calculateBorders()
     *        The only difference from RS_Ellipse is a no-op calculateBorders() method
     */
    class EllipseBorderHelper : public RS_Ellipse {
    public:
        explicit EllipseBorderHelper(const RS_Ellipse& ellipse) : RS_Ellipse(ellipse) {
        }

        // No-op to avoid infinite loop in RS_Ellipse::calculateBorders()
        void calculateBorders() override {
        }
    };
} // anonymous namespace

std::ostream& operator <<(std::ostream& os, const RS_EllipseData& ed) {
    os << "(" << ed.center << " " << ed.majorP << " " << ed.ratio << " " << ed.angle1 << "," << ed.angle2 << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Ellipse::RS_Ellipse(RS_EntityContainer* parent, const RS_EllipseData& d)
    : LC_CachedLengthEntity(parent), m_data(d) {
    //calculateEndpoints();
    RS_Ellipse::calculateBorders();
}

RS_Entity* RS_Ellipse::clone() const {
    auto* e = new RS_Ellipse(*this);
    return e;
}

/**
 * Calculates the boundary box of this ellipse.
  * @author Dongxu Li
 */
void RS_Ellipse::calculateBorders() {
#ifndef EMU_C99
    using std::isnormal;
#endif
    if (std::abs(m_data.angle1) < RS_TOLERANCE_ANGLE && (std::abs(m_data.angle2) < RS_TOLERANCE_ANGLE)) {
        m_data.angle1 = 0;
        m_data.angle2 = 0;
    }
    m_data.isArc = isnormal(m_data.angle1) || isnormal(m_data.angle2);

    LC_Rect boundingBox = isEllipticArc() ? LC_Rect{getStartpoint(), getEndpoint()} : LC_Rect{};

    // x-range extremes are at this direction and its opposite, relative to the ellipse center
    const RS_Vector vpx{getMajorP().x, -getRatio() * getMajorP().y};
    mergeBoundingBox(boundingBox, vpx);

    // y-range extremes are at this direction and its opposite, relative to the ellipse center
    const RS_Vector vpy{getMajorP().y, getRatio() * getMajorP().x};
    mergeBoundingBox(boundingBox, vpy);

    m_minV = boundingBox.minP();
    m_maxV = boundingBox.maxP();

    m_data.angleDegrees = RS_Math::rad2deg(getAngle());
    m_data.startAngleDegrees = RS_Math::rad2deg(m_data.reversed ? m_data.angle2 : m_data.angle1);
    m_data.otherAngleDegrees = RS_Math::rad2deg(m_data.reversed ? m_data.angle1 : m_data.angle2);
    m_data.angularLength = RS_Math::rad2deg(RS_Math::getAngleDifference(m_data.angle1, m_data.angle2, m_data.reversed));
    if (std::abs(m_data.angularLength) < RS_TOLERANCE_ANGLE) {
        // check whether angles are via period
        if (RS_Math::getPeriodsCount(m_data.angle1, m_data.angle2, m_data.reversed) != 0) {
            m_data.angularLength = 360; // in degrees
        }
    }

    updateLength();
}

void RS_Ellipse::mergeBoundingBox(LC_Rect& boundingBox, const RS_Vector& direction) const {
    const double angle = direction.angle();
    // Test the given direction and its opposite
    for (const double a : {angle, angle + M_PI}) {
        if (RS_Math::isAngleBetween(a, getAngle1(), getAngle2(), isReversed())) {
            boundingBox = boundingBox.merge(getEllipsePoint(a));
        }
    }
}

/**
  * return the foci of ellipse
  *
  * @author Dongxu Li
  */

RS_VectorSolutions RS_Ellipse::getFoci() const {
    RS_Ellipse e = *this;
    if (getRatio() > 1.) {
        e.switchMajorMinor();
    }
    const RS_Vector vp(e.getMajorP() * sqrt(1. - (e.getRatio() * e.getRatio())));
    return RS_VectorSolutions({getCenter() + vp, getCenter() - vp});
}

RS_VectorSolutions RS_Ellipse::getRefPoints() const {
    RS_VectorSolutions ret;
    if (isEllipticArc()) {
        //no start/end point for whole ellipse
        ret.push_back(getStartpoint());
        ret.push_back(getEndpoint());
    }
    ret.push_back(m_data.center);
    ret.push_back(getFoci());
    ret.push_back(getMajorPoint());
    ret.push_back(getMinorPoint());
    return ret;
}

RS_Vector RS_Ellipse::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    if (!isEllipticArc()) {
        return RS_Vector{false};
    }

    const RS_Vector startpoint = getStartpoint();
    const RS_Vector endpoint = getEndpoint();

    const double dist1 = (startpoint - coord).squared();
    const double dist2 = (endpoint - coord).squared();

    if (dist2 < dist1) {
        if (dist != nullptr) {
            *dist = sqrt(dist2);
        }
        return endpoint;
    }
    if (dist != nullptr) {
        *dist = sqrt(dist1);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Ellipse*>(this);
    }
    return startpoint;
}

/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  * \author Dongxu Li
  */
RS_VectorSolutions RS_Ellipse::getTangentPoint(const RS_Vector& point) const {
    RS_Vector point2(point);
    point2.move(-getCenter());
    RS_Vector aV(-getAngle());
    point2.rotate(aV);
    RS_VectorSolutions sol;
    const double a = getMajorRadius();
    if (a < RS_TOLERANCE || getRatio() < RS_TOLERANCE) {
        return sol;
    }
    const RS_Circle c(nullptr, RS_CircleData(RS_Vector(0., 0.), a));
    point2.y /= getRatio();
    sol = c.getTangentPoint(point2);
    sol.scale(RS_Vector(1., getRatio()));
    aV.y *= -1.;
    sol.rotate(aV);
    sol.move(getCenter());
    return sol;
}

RS_Vector RS_Ellipse::getTangentDirection(const RS_Vector& point) const {
    RS_Vector vp = point - getCenter();
    RS_Vector aV{-getAngle()};
    vp.rotate(aV);
    vp.y /= getRatio();
    const double a = getMajorRadius();
    if (a < RS_TOLERANCE || getRatio() < RS_TOLERANCE) {
        return {};
    }
    const RS_Circle c(nullptr, RS_CircleData(RS_Vector(0., 0.), a));
    RS_Vector direction = c.getTangentDirection(vp);
    direction.y *= getRatio();
    aV.y *= -1.;
    direction.rotate(aV);
    return isReversed() ? -direction : direction;
}

/**
  * find total length of the ellipse (arc)
  *
  * \author: Dongxu Li
  */
void RS_Ellipse::updateLength() {
    // EllipseBorderHelper class has a no-op calculateBorders() method
    EllipseBorderHelper e{*this};

    //switch major/minor axis, because we need the ratio smaller than one in getEllipseLength()
    if (e.getRatio() > 1.) {
        e.switchMajorMinor();
    }

    // required to be not reversed in getEllipseLength()
    if (e.isReversed()) {
        std::swap(e.m_data.angle1, e.m_data.angle2);
        e.setReversed(false);
    }
    m_cachedLength = e.getEllipseLength(e.m_data.angle1, e.m_data.angle2);
}

/**
//Ellipse must have ratio<1, and not reversed
*@ x1, ellipse angle
*@ x2, ellipse angle
//@return the arc length between ellipse angle x1, x2
* \author Dongxu Li
**/
double RS_Ellipse::getEllipseLength(double angle1, double angle2) const {
    const double a(getMajorRadius());
    double k(getRatio());
    k = std::sqrt(1 - (k * k)); // elliptic modulus, or eccentricity
    //    std::cout<<"1, angle1="<<x1/M_PI<<" angle2="<<x2/M_PI<<std::endl;
    //    if(isReversed())  std::swap(x1,x2);
    angle1 = RS_Math::correctAngle(angle1);
    angle2 = RS_Math::correctAngle(angle2);
    //    std::cout<<"2, angle1="<<x1/M_PI<<" angle2="<<x2/M_PI<<std::endl;
    if (angle2 < angle1 + RS_TOLERANCE_ANGLE) {
        angle2 += 2. * M_PI;
    }
    double ret = 0.;
    //    std::cout<<"3, angle1="<<x1/M_PI<<" angle2="<<x2/M_PI<<std::endl;
    if (angle2 >= M_PI) {
        // the complete elliptic integral
        ret = (static_cast<int>((angle2 + RS_TOLERANCE_ANGLE) / M_PI) - (static_cast<int>((angle1 + RS_TOLERANCE_ANGLE) / M_PI))) * 2;
        //        std::cout<<"Adding "<<ret<<" of E("<<k<<")\n";
        ret *= boost::math::ellint_2<double>(k);
    }
    else {
        ret = 0.;
    }
    angle1 = std::fmod(angle1,M_PI);
    angle2 = std::fmod(angle2,M_PI);
    if (std::abs(angle2 - angle1) > RS_TOLERANCE_ANGLE) {
        ret += RS_Math::ellipticIntegral_2(k, angle2) - RS_Math::ellipticIntegral_2(k, angle1);
    }
    return a * ret;
}

/**
  * arc length from start point (angle1)
  */
double RS_Ellipse::getEllipseLength(const double angleLength) const {
    return getEllipseLength(getAngle1(), angleLength);
}

/**
  * get the point on the ellipse arc and with arc distance from the start point
  * the distance is expected to be within 0 and getLength()
  * using Newton-Raphson from boost
  *
  *@author: Dongxu Li
  */

RS_Vector RS_Ellipse::doGetNearestDist(const double distance, const RS_Vector& coord, double* dist) const {
    //    RS_DEBUG->print("RS_Ellipse::getNearestDist() begin\n");
    if (!isEllipticArc()) {
        // both angles being 0, whole ellipse
        // no end points for whole ellipse, therefore, no snap by distance from end points.
        return {};
    }
    RS_Ellipse e(nullptr, m_data);
    if (e.getRatio() > 1.) {
        e.switchMajorMinor();
    }
    if (e.isReversed()) {
        std::swap(e.m_data.angle1, e.m_data.angle2);
        e.setReversed(false);
    }

    if (e.getMajorRadius() < RS_TOLERANCE) {
        return {}; //ellipse too small
    }

    if (getRatio() < RS_TOLERANCE) {
        //treat the ellipse as a line
        const RS_Line line{e.m_minV, e.m_maxV};
        return line.getNearestDist(distance, coord, dist);
    }
    const double x1 = e.getAngle1();
    double x2 = e.getAngle2();
    if (x2 < x1 + RS_TOLERANCE_ANGLE) {
        x2 += 2 * M_PI;
    }
    const double l0 = e.getEllipseLength(x1, x2); // the getEllipseLength() function only defined for proper e
    //    distance=std::abs(distance);
    if (distance > l0 + RS_TOLERANCE) {
        return {}; // can not trim more than the current length
    }
    if (distance > l0 - RS_TOLERANCE) {
        return getNearestEndpoint(coord, nullptr, dist); // trim to zero length
    }

    return getNearestDistHelper(e, distance, coord, dist);
}

/**
  * switch the major/minor axis naming
  *
  * \author: Dongxu Li
  */
bool RS_Ellipse::switchMajorMinor() {
    //switch naming of major/minor, return true if success
    if (std::abs(m_data.ratio) < RS_TOLERANCE) {
        return false;
    }

    const RS_Vector vp = getMajorP();
    setMajorP(RS_Vector(-m_data.ratio * vp.y, m_data.ratio * vp.x)); //direction pi/2 relative to old MajorP;
    setRatio(1. / m_data.ratio);
    if (isEllipticArc()) {
        const RS_Vector vpStart = getStartpoint();
        const RS_Vector vpEnd = getEndpoint();
        //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
        setAngle1(getEllipseAngle(vpStart));
        setAngle2(getEllipseAngle(vpEnd));
    }
    calculateBorders();
    return true;
}

/**
 * @return Start point of the entity.
 */
RS_Vector RS_Ellipse::getStartpoint() const {
    return isEllipticArc() ? getEllipsePoint(m_data.angle1) : RS_Vector{false};
}

/**
 * @return End point of the entity.
 */
RS_Vector RS_Ellipse::getEndpoint() const {
    return isEllipticArc() ? getEllipsePoint(m_data.angle2) : RS_Vector{false};
}

/**
 * @return Ellipse point by ellipse angle
 */
RS_Vector RS_Ellipse::getEllipsePoint(const double a) const {
    RS_Vector point{a};
    const double ra = getMajorRadius();
    point.scale(RS_Vector(ra, ra * getRatio()));
    point.rotate(getAngle());
    point.move(getCenter());
    return point;
}

/** \brief implemented using an analytical algorithm
* find nearest point on ellipse to a given point
*
* @author Dongxu Li <dongxuli2011@gmail.com>
*/
RS_Vector RS_Ellipse::doGetNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    // RS_DEBUG->print("RS_Ellipse::getNearestPointOnEntity");
    RS_Vector ret(false);

    if (!coord.valid) {
        if (dist != nullptr) {
            *dist = RS_MAXDOUBLE;
        }
        return ret;
    }

    if (entity != nullptr) {
        *entity = const_cast<RS_Ellipse*>(this);
    }
    ret = coord;
    ret.move(-getCenter());
    ret.rotate(-getAngle());
    const double x = ret.x, y = ret.y;
    const double a = getMajorRadius();
    const double b = a * getRatio();
    //std::cout<<"(a= "<<a<<" b= "<<b<<" x= "<<x<<" y= "<<y<<" )\n";
    //std::cout<<"finding minimum for ("<<x<<"-"<<a<<"*cos(t))^2+("<<y<<"-"<<b<<"*sin(t))^2\n";
    const double twoa2b2 = 2 * (a * a - b * b);
    const double twoax = 2 * a * x;
    const double twoby = 2 * b * y;
    const double a0 = twoa2b2 * twoa2b2;
    std::vector<double> ce(4, 0.);
    std::vector<double> roots;

    //need to handle: a=b (i.e. a0=0); point close to the ellipse origin.
    if (a0 > RS_TOLERANCE && std::abs(getRatio() - 1.0) > RS_TOLERANCE && ret.squared() > RS_TOLERANCE2) {
        // a != b , ellipse
        ce[0] = -2. * twoax / twoa2b2;
        ce[1] = ((twoax * twoax + twoby * twoby) / a0) - 1.;
        ce[2] = -ce[0];
        ce[3] = -twoax * twoax / a0;
        //std::cout<<"1::find cosine, variable c, solve(c^4 +("<<ce[0]<<")*c^3+("<<ce[1]<<")*c^2+("<<ce[2]<<")*c+("<<ce[3]<<")=0,c)\n";
        roots = RS_Math::quarticSolver(ce);
    }
    else {
        // Issue #1653: approximately a=b, solve the equation of ds^2/d\theta = 0 by Newton-Raphson
        const double theta = ClosestEllipticPoint{a, b, ret}.getTheta();
        roots.push_back(std::cos(theta));
        // Just in case, the found solution is for the maximum distance. Then, the minimum is at the opposite
        roots.push_back(-roots.front());
    }
    if (roots.empty()) {
        //this should not happen
        // std::cout << "(a= " << a << " b= " << b << " x= " << x << " y= " << y << " )\n";
        std::cout << "finding minimum for (" << x << "-" << a << "*cos(t))^2+(" << y << "-" << b << "*sin(t))^2\n";
        std::cout << "2::find cosine, variable c, solve(c^4 +(" << ce[0] << ")*c^3+(" << ce[1] << ")*c^2+(" << ce[2] << ")*c+(" << ce[3] <<
            ")=0,c)\n";
        std::cout << ce[0] << ' ' << ce[1] << ' ' << ce[2] << ' ' << ce[3] << std::endl;
        std::cerr << "RS_Math::RS_Ellipse::getNearestPointOnEntity() finds no root from quartic, this should not happen\n";
        return RS_Vector(coord); // better not to return invalid: return RS_Vector(false);
    }

    //    RS_Vector vp2(false);
    double dDistance = RS_MAXDOUBLE * RS_MAXDOUBLE;
    //double ea;
    std::vector<std::pair<double, double>> directions;
    for (double cosTheta : roots) {
      // Skip spurious roots from squaring during the quartic derivation —
      // they have |cos| > 1 and do not correspond to any real angle on the
      // ellipse. Without this filter, plugging such a root into the
      // (cosTheta, sinTheta) → (a*cos, b*sin) mapping would yield an
      // off-ellipse point that could undercut the true minimum distance.
      if (std::abs(cosTheta) > 1.0 + RS_TOLERANCE)
        continue;
      double const c = std::clamp(cosTheta, -1.0, 1.0);  if (std::abs(twoax - (twoa2b2 * c)) > RS_TOLERANCE) {
            const double sinTheta = twoby * c / (twoax - twoa2b2 * c);
        directions.emplace_back(c, sinTheta);
        }
        else {
        directions.emplace_back(0., 1.);
        directions.emplace_back(0., -1.);
      }
    }
    // The quartic yields every critical point of the squared distance — both
    // minima and maxima. The global minimum is the critical point with the
    // smallest squared distance, so simply compare distances and keep the
    // smallest; no second-derivative test is needed.
    RS_Vector const query = ret;
    for (const auto &[cosTheta, sinTheta] : directions) {
      RS_Vector vp3{a * cosTheta, b * sinTheta};
      double d = (vp3 - query).squared();
      if (d >= dDistance)
        continue;
      ret = vp3;
      dDistance = d;
    }
    if (!ret.valid) {
        //this should not happen
        //        std::cout<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<' '<<ce[3]<<std::endl;
        //        std::cout<<"(x,y)=( "<<x<<" , "<<y<<" ) a= "<<a<<" b= "<<b<<" sine= "<<s<<" d2= "<<d2<<" dist= "<<d<<std::endl;
        //        std::cout<<"RS_Ellipse::getNearestPointOnEntity() finds no minimum, this should not happen\n";
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Ellipse::getNearestPointOnEntity() finds no minimum, this should not happen\n");
    }
    if (dist != nullptr) {
        *dist = std::sqrt(dDistance);
    }
    ret.rotate(getAngle());
    ret.move(getCenter());
    //    ret=vp2;
    if (onEntity) {
        if (!RS_Math::isAngleBetween(getEllipseAngle(ret), getAngle1(), getAngle2(), isReversed())) {
            // not on entity, use the nearest endpoint
            //std::cout<<"not on ellipse, ( "<<getAngle1()<<" "<<getEllipseAngle(ret)<<" "<<getAngle2()<<" ) reversed= "<<isReversed()<<"\n";
            ret = getNearestEndpoint(coord, nullptr,dist);
        }
    }

    //    if(! ret.valid) {
    //        std::cout<<"RS_Ellipse::getNearestOnEntity() returns invalid by mistake. This should not happen!"<<std::endl;
    //    }
    return ret;
}

/**
 * @param coord
 * @param tolerance Tolerance.
 *
 * @retval true if the given point is on this entity.
 * @retval false otherwise
 */
bool RS_Ellipse::doIsPointOnEntity(const RS_Vector& coord, const double tolerance) const {
    const double t = std::abs(tolerance);
    const double a = getMajorRadius();
    const double b = a * getRatio();
    RS_Vector vp((coord - getCenter()).rotate(-getAngle()));
    if (a < RS_TOLERANCE) {
        //radius treated as zero
        if (std::abs(vp.x) < RS_TOLERANCE && std::abs(vp.y) < b) {
            return true;
        }
        return false;
    }
    if (b < RS_TOLERANCE) {
        //radius treated as zero
        if (std::abs(vp.y) < RS_TOLERANCE && std::abs(vp.x) < a) {
            return true;
        }
        return false;
    }
    vp.scale(RS_Vector(1. / a, 1. / b));

    if (std::abs(vp.squared() - 1.) > t) {
        return false;
    }
    return RS_Math::isAngleBetween(vp.angle(), getAngle1(), getAngle2(), isReversed());
}

RS_Vector RS_Ellipse::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    RS_Vector vCenter = m_data.center;
    double distCenter = coord.distanceTo(m_data.center);

    const RS_VectorSolutions vsFoci = getFoci();
    if (2 == vsFoci.getNumber()) {
        const RS_Vector& vFocus1 = vsFoci.get(0);
        const RS_Vector& vFocus2 = vsFoci.get(1);

        const double distFocus1 = coord.distanceTo(vFocus1);
        const double distFocus2 = coord.distanceTo(vFocus2);

        /* if (distFocus1 < distCenter) is true
         * then (distFocus1 < distFocus2) must be true too
         * and vice versa
         * no need to check this */
        if (distFocus1 < distCenter) {
            vCenter = vFocus1;
            distCenter = distFocus1;
        }
        else if (distFocus2 < distCenter) {
            vCenter = vFocus2;
            distCenter = distFocus2;
        }
    }

    if (dist != nullptr) {
        *dist = distCenter;
    }
    if (centerEntity != nullptr) {
        *centerEntity = const_cast<RS_Ellipse*>(this);
    }
    return vCenter;
}


/**
 * a naive implementation of middle point
 * to accurately locate the middle point from arc length is possible by using elliptic integral to find the total arc length, then, using elliptic function to find the half length point
 */
RS_Vector RS_Ellipse::getMiddlePoint() const {
    return getNearestMiddle(getCenter());
}

/**
  * get Nearest equidistant point
  *
  *@author: Dongxu Li
  */
RS_Vector RS_Ellipse::doGetNearestMiddle(const RS_Vector& coord, double* dist, const int middlePoints) const {
    RS_DEBUG->print("RS_Ellpse::getNearestMiddle(): begin\n");
    if (!isEllipticArc()) {
        //no middle point for whole ellipse, angle1=angle2=0
        if (dist != nullptr) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }
    const double ra(getMajorRadius());
    const double rb(getRatio() * ra);
    if (ra < RS_TOLERANCE || rb < RS_TOLERANCE) {
        //zero radius, return the center
        const RS_Vector vp(getCenter());
        if (dist != nullptr) {
            *dist = vp.distanceTo(coord);
        }
        return vp;
    }
    double amin = getCenter().angleTo(getStartpoint());
    double amax = getCenter().angleTo(getEndpoint());
    if (isReversed()) {
        std::swap(amin, amax);
    }
    double da = std::fmod(amax - amin + (2. * M_PI), 2. * M_PI);
    if (da < RS_TOLERANCE) {
        da = 2. * M_PI; //whole ellipse
    }
    RS_Vector vp(getNearestPointOnEntity(coord, true, dist));
    double a = getCenter().angleTo(vp);
    const int counts(middlePoints + 1);
    int i = static_cast<int>((std::fmod(a - amin + (2. * M_PI), 2. * M_PI) / da * counts) + 0.5);
    // remove end points
    i = std::max(i, 1);
    i = std::min(i, counts - 1);
    const double doubleI = i;
    a = amin + (da * (doubleI / counts)) - getAngle();
    vp.set(a);
    RS_Vector vp2(vp);
    vp2.scale(RS_Vector(1. / ra, 1. / rb));
    vp.scale(1. / vp2.magnitude());
    vp.rotate(getAngle());
    vp.move(getCenter());

    if (dist != nullptr) {
        *dist = vp.distanceTo(coord);
    }
    //RS_DEBUG->print("RS_Ellipse::getNearestMiddle: angle1=%g, angle2=%g, middle=%g\n",amin,amax,a);
    RS_DEBUG->print("RS_Ellpse::getNearestMiddle(): end\n");
    return vp;
}

/**
  * get the tangential point of a tangential line orthogonal to a given line
  *@ normal, the given line
  *@ onEntity, should the tangential be required to on entity of the elliptic arc
  *@ coord, current cursor position
  *
  *@author: Dongxu Li
  */

RS_Vector RS_Ellipse::getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, const bool onEntity) const {
    if (!coord.valid) {
        return RS_Vector(false);
    }
    RS_Vector direction = normal.getEndpoint() - normal.getStartpoint();
    if (direction.squared() < RS_TOLERANCE15) {
        //undefined direction
        return RS_Vector(false);
    }
    //scale to ellipse angle
    RS_Vector aV(-getAngle());
    direction.rotate(aV);
    double angle = direction.scale(RS_Vector(1., getRatio())).angle();
    const double ra(getMajorRadius());
    direction.set(ra * cos(angle), getRatio() * ra * sin(angle)); //relative to center
    std::vector<RS_Vector> sol;
    for (int i = 0; i < 2; i++) {
        if (!onEntity || RS_Math::isAngleBetween(angle, getAngle1(), getAngle2(), isReversed())) {
            sol.push_back(i == 0 ? direction : -direction);
        }
        angle = RS_Math::correctAngle(angle + M_PI);
    }
    if (sol.empty()) {
        return RS_Vector(false);
    }
    aV.y *= -1.;
    for (auto& v : sol) {
        v.rotate(aV);
    }
    RS_Vector vp{};
    switch (sol.size()) {
        case 0: {
            return RS_Vector(false);
        }
        case 2: {
            if (RS_Vector::dotP(sol[1], coord - getCenter()) > 0.) {
                vp = sol[1];
                break;
            }
            // fall-through
            [[fallthrough]];
        }
        default: {
            vp = sol[0];
            break;
        }
    }
    return getCenter() + vp;
}

double RS_Ellipse::getBulge() const {
    const double bulge = std::tan(std::abs(getAngleLength()) / 4.0);
    return isReversed() ? -bulge : bulge;
}

RS_Vector RS_Ellipse::dualLineTangentPoint(const RS_Vector& line) const {
    // u x + v y = 1
    // coordinates : dual
    // rotate (-a) : rotate(a)
    const RS_Vector uv = RS_Vector{line}.rotate(-m_data.majorP.angle());
    // slope = -b c/ a s ( a s, - b c)
    // x a s - b c y =0 -> s/c = b y / a x
    // elliptical angle
    const double t = std::atan2(m_data.ratio * uv.y, uv.x);
    RS_Vector vp{m_data.majorP.magnitude() * std::cos(t), m_data.majorP.magnitude() * m_data.ratio * std::sin(t)};
    vp.rotate(m_data.majorP.angle());

    const RS_Vector vp0 = m_data.center + vp;
    const RS_Vector vp1 = m_data.center - vp;
    auto lineEqu = [&line](const RS_Vector& p) {
        return std::abs(line.dotP(p) + 1.);
    };
    return lineEqu(vp0) < lineEqu(vp1) ? vp0 : vp1;
}

void RS_Ellipse::move(const RS_Vector& offset) {
    m_data.move(offset);
    moveBorders(offset);
}

void RS_Ellipse::rotate(const RS_Vector& center, double angle) {
    RS_Vector angleVector(angle);
    m_data.center.rotate(center, angleVector);
    m_data.majorP.rotate(angleVector);
    calculateBorders();
}

void RS_Ellipse::revertDirection() {
    if (m_data.isArc) {
        std::swap(m_data.angle1, m_data.angle2);
        m_data.reversed = !m_data.reversed;
        calculateBorders();
    }
}

std::vector<RS_Entity*> RS_Ellipse::createOffset(const RS_Vector& coord,
                                                 const double& distance) const {
    const double a = getMajorRadius();
    const double b = getMinorRadius();
    if (a < RS_TOLERANCE || b < RS_TOLERANCE) {
        return {};
    }
    if (std::abs(distance) < RS_TOLERANCE) {
        return {};
    }

    // Sign convention: positive when `coord` is outside the ellipse.
    // Use the implicit form in the ellipse-aligned frame: q = (u/a)^2 + (v/b)^2.
    // q > 1 ⇒ outside, q < 1 ⇒ inside.
    const RS_Vector center = getCenter();
    const RS_Vector majorP = getMajorP();
    const double majorAngle = majorP.angle();
    RS_Vector local = coord - center;
    local.rotate(-majorAngle);
    const double q = (local.x / a) * (local.x / a)
                   + (local.y / b) * (local.y / b);
    const double signedD = (q >= 1.0) ? std::abs(distance) : -std::abs(distance);

    // Cusp / self-intersection rejection: inward offset cusps when |d| ≥ b²/a.
    // Reject with a 1% safety margin.
    if (signedD < 0.0) {
        const double cuspLimit = (b * b) / a;
        if (std::abs(signedD) >= 0.99 * cuspLimit) {
            return {};
        }
    }

    // Parameter range.
    const bool isArc = isEllipticArc();
    double angleStart;
    double angleEnd;
    if (isArc) {
        angleStart = getAngle1();
        angleEnd = getAngle2();
        if (isReversed()) {
            // Sweep from angle1 down to angle2 (CW). Normalize so end > start
            // by adding 2π if needed; we then walk negatively.
            if (angleEnd > angleStart) {
                angleEnd -= 2.0 * M_PI;
            }
        } else {
            if (angleEnd < angleStart) {
                angleEnd += 2.0 * M_PI;
            }
        }
    } else {
        angleStart = 0.0;
        angleEnd = 2.0 * M_PI;
    }
    const double sweep = angleEnd - angleStart;
    const double sweepLen = std::abs(sweep);
    if (sweepLen < RS_TOLERANCE_ANGLE) {
        return {};
    }
    const double dir = (sweep >= 0.0) ? 1.0 : -1.0;

    // Adaptive sagitta-based sampling on the offset curve.
    // Tolerance: 0.1% of major radius, with an absolute floor.
    const double epsilon = std::max(a * 1.0e-3, RS_TOLERANCE * 100.0);

    // Per-revolution caps (scaled to the actual sweep below).
    constexpr int kNMinFull = 16;
    constexpr int kNMaxFull = 512;
    const double sweepFraction = sweepLen / (2.0 * M_PI);
    const int nMin = std::max(2, static_cast<int>(std::ceil(kNMinFull * sweepFraction)));
    const int nMax = std::max(nMin + 1, static_cast<int>(std::ceil(kNMaxFull * sweepFraction)));

    // Sagitta-bounded step size in the ellipse parameter θ. For the ellipse
    // p(θ) = (a cosθ, b sinθ):
    //   |p_ell'(θ)|² = a²sin²θ + b²cos²θ ≡ T(θ)
    //   ρ_ell(θ) = T(θ)^(3/2) / (a·b)
    // Offsetting by signedD along the outward unit normal:
    //   ρ_off(θ) = ρ_ell(θ) + signedD      (signedD>0 outward, <0 inward)
    //   |p_off'(θ)| = (ρ_off / ρ_ell) · |p_ell'(θ)|     ← key correction
    // The offset curve's parametric speed differs from the ellipse's; for
    // inner offsets it slows down, vanishing at a cusp (ρ_off→0). Bounding
    // sagitta h ≈ chord_off²/(8 ρ_off) ≤ ε with chord_off = |p_off'|·Δθ:
    //   Δθ = (ρ_ell / √T) · √(8 ε / ρ_off) = (T / (a·b)) · √(8 ε / ρ_off)
    // Result: uniform sagitta along the offset curve regardless of offset
    // direction. Sample density on the offset is 1/√(8ε ρ_off) per unit arc
    // length — high at sharp regions, low at shallow ones, in both inner and
    // outer offsets.
    auto stepTheta = [&](double theta) {
        const double s = std::sin(theta);
        const double c = std::cos(theta);
        const double term = (a * s) * (a * s) + (b * c) * (b * c);
        const double rhoEll = std::pow(term, 1.5) / (a * b);
        const double rhoOff = std::max(rhoEll + signedD, RS_TOLERANCE);
        return (term / (a * b)) * std::sqrt(8.0 * epsilon / rhoOff);
    };

    // Hard caps on per-step Δθ regardless of curvature.
    const double dThetaMax = M_PI / 8.0;                // 22.5°
    const double dThetaMin = sweepLen / static_cast<double>(nMax);

    std::vector<RS_Vector> offsetPoints;
    offsetPoints.reserve(64);

    auto pushSample = [&](double theta) {
        // Work in the ellipse-aligned local frame, then transform once to world.
        // p_local(θ) = (a cosθ, b sinθ); tangent ∝ (-a sinθ, b cosθ); rotating
        // the tangent by -90° gives the outward normal (b cosθ, a sinθ), which
        // is always outward since dot(n, p_local) = a·b > 0.
        const double s = std::sin(theta);
        const double c = std::cos(theta);
        RS_Vector n(b * c, a * s);
        n.normalize();
        RS_Vector p = RS_Vector(a * c, b * s) + signedD * n;
        p.rotate(majorAngle);
        p += center;
        offsetPoints.push_back(p);
    };

    double theta = angleStart;
    pushSample(theta);
    while (true) {
        double dTheta = stepTheta(theta);
        if (!std::isfinite(dTheta) || dTheta < dThetaMin) {
            dTheta = dThetaMin;
        }
        if (dTheta > dThetaMax) {
            dTheta = dThetaMax;
        }

        const double remaining = std::abs(angleEnd - theta);
        if (remaining <= dTheta) {
            if (isArc) {
                // Open spline: include exact endpoint.
                theta = angleEnd;
                pushSample(theta);
            }
            // Closed spline: omit the duplicate endpoint (LC_SplinePoints
            // closes implicitly).
            break;
        }
        theta += dir * dTheta;
        pushSample(theta);
    }

    // Bounds: ensure at least nMin points (e.g. for very low-curvature
    // shallow arcs where the adaptive step is huge).
    if (static_cast<int>(offsetPoints.size()) < nMin) {
        offsetPoints.clear();
        const int n = nMin;
        for (int i = 0; i < n; ++i) {
            const double t = angleStart + sweep * (static_cast<double>(i) / (n - (isArc ? 1 : 0)));
            pushSample(t);
        }
        if (!isArc && !offsetPoints.empty()){
            offsetPoints.pop_back();  // drop duplicate wrap point
        }
    }

    if (offsetPoints.size() < 3) {
        return {};
    }

    LC_SplinePointsData spd(/*closed=*/!isArc, /*cut=*/false);
    spd.splinePoints = std::move(offsetPoints);
    auto* sp = new LC_SplinePoints(nullptr, spd);
    sp->setPen(getPen(false));
    sp->setLayer(getLayer());
    sp->calculateBorders();
    return { sp };
}

void RS_Ellipse::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_data.rotate(center, angleVector);
    //calculateEndpoints();
    calculateBorders();
}

void RS_Ellipse::rotate(const double angle) {
    //rotate around origin
    m_data.rotate(angle);
    calculateBorders();
}

void RS_Ellipse::rotate(const RS_Vector& angleVector) {
    //rotate around origin
    m_data.center.rotate(angleVector);
    m_data.majorP.rotate(angleVector);
    //calculateEndpoints();
    calculateBorders();
}

/**
 * make sure angleLength() is not more than 2*M_PI
 */
void RS_Ellipse::correctAngles() {
    double* pa1 = &m_data.angle1;
    double* pa2 = &m_data.angle2;
    if (isReversed()) {
        std::swap(pa1, pa2);
    }
    *pa2 = *pa1 + std::fmod(*pa2 - *pa1, 2. * M_PI);
    if (std::abs(m_data.angle1 - m_data.angle2) < RS_TOLERANCE_ANGLE && (std::abs(m_data.angle1) > RS_TOLERANCE_ANGLE)) {
        // we need this only if there are actual angles (arc). otherwise, adding 2pi will transform ellipse to
        // elliptic arc
        *pa2 += 2. * M_PI;
    }
}

void RS_Ellipse::moveStartpoint(const RS_Vector& pos) {
    m_data.angle1 = getEllipseAngle(pos);
    //data.angle1 = data.center.angleTo(pos);
    //calculateEndpoints();
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

void RS_Ellipse::moveEndpoint(const RS_Vector& pos) {
    m_data.angle2 = getEllipseAngle(pos);
    //data.angle2 = data.center.angleTo(pos);
    //calculateEndpoints();
    correctAngles(); // make sure angleLength is no more than 2*M_PI
    calculateBorders();
}

RS2::Ending RS_Ellipse::getTrimPoint(const RS_Vector& trimCoord, const RS_Vector& /*trimPoint*/) {
    //double angEl = getEllipseAngle(trimPoint);
    const double angM = getEllipseAngle(trimCoord);
    if (RS_Math::getAngleDifference(angM, m_data.angle1, isReversed()) > RS_Math::getAngleDifference(m_data.angle2, angM, isReversed())) {
        return RS2::EndingStart;
    }
    return RS2::EndingEnd;
}

RS_Vector RS_Ellipse::prepareTrim(const RS_Vector& trimCoord, const RS_VectorSolutions& trimSol) {
    //special trimming for ellipse arc
    RS_DEBUG->print("RS_Ellipse::prepareTrim()");
    if (!trimSol.hasValid()) {
        return RS_Vector{false};
    }
    if (trimSol.getNumber() == 1) {
        return trimSol.front();
    }
    const double am = getEllipseAngle(trimCoord);
    std::vector<double> ias;
    double ia(0.), ia2(0.);
    RS_Vector is, is2;
    for (size_t ii = 0; ii < trimSol.getNumber(); ++ii) {
        //find closest according ellipse angle
        ias.push_back(getEllipseAngle(trimSol.get(ii)));
        if (!ii || std::abs(remainder(ias[ii] - am, 2 * M_PI)) < std::abs(remainder(ia - am, 2 * M_PI))) {
            ia = ias[ii];
            is = trimSol.get(ii);
        }
    }
    std::sort(ias.begin(), ias.end());
    for (size_t ii = 0; ii < trimSol.getNumber(); ++ii) {
        //find segment to include trimCoord
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
    for (const RS_Vector& vp : trimSol) {
        //find segment to include trimCoord
        if (!RS_Math::isSameDirection(ia2, getEllipseAngle(vp),RS_TOLERANCE)) {
            continue;
        }
        is2 = vp;
        break;
    }
    if (RS_Math::isSameDirection(getAngle1(), getAngle2(),RS_TOLERANCE_ANGLE) || RS_Math::isSameDirection(ia2, ia,RS_TOLERANCE)) {
        //whole ellipse
        if (!RS_Math::isAngleBetween(am, ia, ia2, isReversed())) {
            std::swap(ia, ia2);
            std::swap(is, is2);
        }
        setAngle1(ia);
        setAngle2(ia2);
        const double da1 = std::abs(remainder(getAngle1() - am, 2 * M_PI));
        const double da2 = std::abs(remainder(getAngle2() - am, 2 * M_PI));
        if (da2 < da1) {
            std::swap(is, is2);
        }
    }
    else {
        const double dia = std::abs(remainder(ia - am, 2 * M_PI));
        const double dia2 = std::abs(remainder(ia2 - am, 2 * M_PI));
        const double aiMin = std::min(dia, dia2);
        double da1 = std::abs(remainder(getAngle1() - am, 2 * M_PI));
        double da2 = std::abs(remainder(getAngle2() - am, 2 * M_PI));
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
                }
                else {
                    setAngle1(ia);
                    setAngle2(ia2);
                }
                da1 = std::abs(remainder(getAngle1() - am, 2 * M_PI));
                da2 = std::abs(remainder(getAngle2() - am, 2 * M_PI));
            }
            if (((da1 < da2) && (RS_Math::isAngleBetween(ia2, ia, getAngle1(), isReversed()))) || ((da1 > da2) && (RS_Math::isAngleBetween(
                ia2, getAngle2(), ia, isReversed())))) {
                std::swap(is, is2);
                //std::cout<<"reset: angle1="<<getAngle1()<<" angle2="<<getAngle2()<<" am="<< am<<" is="<<getEllipseAngle(is)<<" ia2="<<ia2<<std::endl;
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
            }
        }
    }
    return is;
}

double RS_Ellipse::getEllipseAngle(const RS_Vector& pos) const {
    RS_Vector m = pos - m_data.center;
    m.rotate(-m_data.majorP.angle());
    m.x *= m_data.ratio;
    return m.angle();
}

const RS_EllipseData& RS_Ellipse::getData() const {
    return m_data;
}

/* Dongxu Li's Version, 19 Aug 2011
 * scale an ellipse
 * Find the eigen vectors and eigen values by optimization
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
    RS_Vector vpStart;
    RS_Vector vpEnd;
    if (isEllipticArc()) {
        //only handle start/end points for ellipse arc
        vpStart = getStartpoint().scale(center, factor);
        vpEnd = getEndpoint().scale(center, factor);
    }
    m_data.center.scale(center, factor);
    RS_Vector vp1(getMajorP());
    double a(vp1.magnitude());
    if (a < RS_TOLERANCE) {
        return; //ellipse too small
    }
    vp1 *= 1. / a;
    const double ct = vp1.x;
    const double ct2 = ct * ct; // cos^2 angle
    const double st = vp1.y;
    const double st2 = 1.0 - ct2; // sin^2 angle
    const double kx2 = factor.x * factor.x;
    const double ky2 = factor.y * factor.y;
    //    double a=getMajorRadius();
    double b = getRatio() * a;
    const double cA = 0.5 * a * a * (kx2 * ct2 + ky2 * st2);
    const double cB = 0.5 * b * b * (kx2 * st2 + ky2 * ct2);
    const double cC = a * b * ct * st * (ky2 - kx2);
    if (factor.x < 0) {
        setReversed(!isReversed());
    }
    if (factor.y < 0) {
        setReversed(!isReversed());
    }
    const RS_Vector vp(cA - cB, cC);
    vp1.set(a, b);
    vp1.scale(RS_Vector(0.5 * vp.angle()));
    vp1.rotate(RS_Vector(ct, st));
    vp1.scale(factor);
    setMajorP(vp1);
    a = cA + cB;
    b = vp.magnitude();
    setRatio(sqrt((a - b) / (a + b)));
    if (isEllipticArc()) {
        //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
        setAngle1(getEllipseAngle(vpStart));
        setAngle2(getEllipseAngle(vpEnd));
        correctAngles(); //avoid extra 2.*M_PI in angles
    }

    //calculateEndpoints();
    scaleBorders(center, factor);
    // calculateBorders();
}

/**
 * @author{Dongxu Li}
 */
RS_Entity& RS_Ellipse::shear(const double k) {
    RS_Ellipse e1 = *this;
    const auto quadratic = e1.getQuadratic().shear(k);
    LC_CreationEllipse::createEllipseFromQuadratic(quadratic, e1.m_data);
    if (isArc()) {
        e1.moveStartpoint(getStartpoint().shear(k));
        e1.moveEndpoint(getEndpoint().shear(k));
    }
    *this = e1;
    return *this;
}

double RS_Ellipse::computeLocalArea(double t1, double t2) const {
    auto F = [&](double t) {
        return (getMajorRadius() * getMinorRadius() / 2.0) * (t + 0.5 * std::sin(2.0 * t));
    };
    return F(t2) - F(t1);
}

/**
 * is the Ellipse an Arc
 * @return false, if both angle1/angle2 are zero
 *
 *@author: Dongxu Li
 */
bool RS_Ellipse::isEllipticArc() const {
    /*#ifndef EMU_C99
        using std::isnormal;
    #endif
        return *//*std::*//*isnormal(getAngle1()) || *//*std::*/ /*isnormal(getAngle2());*/
    return m_data.isArc;
}

/**
 * mirror by the axis of the line axisPoint1 and axisPoint2
 *
 *@author: Dongxu Li
 */
void RS_Ellipse::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Vector center = getCenter();
    RS_Vector majorp = center + getMajorP();
    RS_Vector startpoint, endpoint;
    const bool isArc = isEllipticArc();
    if (isArc) {
        startpoint = getStartpoint();
        endpoint = getEndpoint();
    }

    center.mirror(axisPoint1, axisPoint2);
    majorp.mirror(axisPoint1, axisPoint2);

    setCenter(center);
    setReversed(!isReversed());
    setMajorP(majorp - center);
    if (isArc) {
        //only reset start/end points for ellipse arcs, i.e., angle1 angle2 are not both zero
        startpoint.mirror(axisPoint1, axisPoint2);
        endpoint.mirror(axisPoint1, axisPoint2);
        setAngle1(getEllipseAngle(startpoint));
        setAngle2(getEllipseAngle(endpoint));
        correctAngles(); //avoid extra 2.*M_PI in angles
    }
    calculateBorders();
}

/**
  * get direction1 and direction2
  * get the tangent pointing outside at end points
  *
  *@author: Dongxu Li
  */
//getDirection1 for start point
double RS_Ellipse::getDirection1() const {
    RS_Vector vp;
    if (isReversed()) {
        vp.set(sin(getAngle1()), -getRatio() * cos(getAngle1()));
    }
    else {
        vp.set(-sin(getAngle1()), getRatio() * cos(getAngle1()));
    }
    return vp.angle() + getAngle();
}

//getDirection2 for end point
double RS_Ellipse::getDirection2() const {
    RS_Vector vp;
    if (isReversed()) {
        vp.set(-sin(getAngle2()), getRatio() * cos(getAngle2()));
    }
    else {
        vp.set(sin(getAngle2()), -getRatio() * cos(getAngle2()));
    }
    return vp.angle() + getAngle();
}

void RS_Ellipse::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (isEllipticArc()) {
        const RS_Vector startpoint = getStartpoint();
        const RS_Vector endpoint = getEndpoint();

        //    if (ref.distanceTo(startpoint)<1.0e-4) {
        //instead of
        if ((ref - startpoint).squared() < RS_TOLERANCE_ANGLE) {
            moveStartpoint(startpoint + offset);
            correctAngles();
            //avoid extra 2.*M_PI in angles // todo - is this call really necessary? It is called in moveStartpoint() already
            return;
        }
        if ((ref - endpoint).squared() < RS_TOLERANCE_ANGLE) {
            moveEndpoint(endpoint + offset);
            //            correctAngles();//avoid extra 2.*M_PI in angles // todo - is this call really necessary? It is called in moveStartpoint() already
            return;
        }
    }
    if ((ref - getCenter()).squared() < RS_TOLERANCE_ANGLE) {
        //move center
        setCenter(getCenter() + offset);
        calculateBorders();
        return;
    }

    if (m_data.ratio > 1.) {
        switchMajorMinor();
    }
    auto foci = getFoci();
    for (size_t i = 0; i < 2; i++) {
        if ((ref - foci.at(i)).squared() < RS_TOLERANCE_ANGLE) {
            const auto focusNew = foci.at(i) + offset;
            //move focus
            const auto center = getCenter() + offset * 0.5;
            RS_Vector majorP;
            if (getMajorP().dotP(foci.at(i) - getCenter()) >= 0.) {
                majorP = focusNew - center;
            }
            else {
                majorP = center - focusNew;
            }
            const double d = getMajorP().magnitude();
            const double c = 0.5 * focusNew.distanceTo(foci.at(1 - i));
            const double k = majorP.magnitude();
            if (k < RS_TOLERANCE2 || d < RS_TOLERANCE || c >= d - RS_TOLERANCE) {
                return;
            }
            //            DEBUG_HEADER
            //            std::cout<<__func__<<" : moving focus";
            majorP *= d / k;
            setCenter(center);
            setMajorP(majorP);
            setRatio(sqrt((d * d) - (c * c)) / d);
            correctAngles(); //avoid extra 2.*M_PI in angles
            if (m_data.ratio > 1.) {
                switchMajorMinor();
            }
            else {
                calculateBorders();
            }
            return;
        }
    }

    //move major/minor points
    if ((ref - getMajorPoint()).squared() < RS_TOLERANCE_ANGLE) {
        const RS_Vector majorP = getMajorP() + offset;
        const double r = majorP.magnitude();
        if (r < RS_TOLERANCE) {
            return;
        }
        const double ratio = getRatio() * getMajorRadius() / r;
        setMajorP(majorP);
        setRatio(ratio);
        if (m_data.ratio > 1.) {
            switchMajorMinor();
        }
        else {
            calculateBorders();
        }
        return;
    }
    if ((ref - getMinorPoint()).squared() < RS_TOLERANCE_ANGLE) {
        const RS_Vector minorP = getMinorPoint() + offset;
        const double r2 = getMajorP().squared();
        if (r2 < RS_TOLERANCE2) {
            return;
        }
        const RS_Vector projected = getCenter() + getMajorP() * getMajorP().dotP(minorP - getCenter()) / r2;
        const double r = (minorP - projected).magnitude();
        if (r < RS_TOLERANCE) {
            return;
        }
        const double ratio = getRatio() * r / getMinorRadius();
        setRatio(ratio);
        if (m_data.ratio > 1.) {
            switchMajorMinor();
        }
        else {
            calculateBorders();
        }
    }
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Ellipse::getQuadratic() const {
    std::vector<double> ce(6, 0.);
    ce[0] = m_data.majorP.squared();
    ce[2] = m_data.ratio * m_data.ratio * ce[0];
    if (ce[0] < RS_TOLERANCE2 || ce[2] < RS_TOLERANCE2) {
        return LC_Quadratic();
    }
    ce[0] = 1. / ce[0];
    ce[2] = 1. / ce[2];
    ce[5] = -1.;
    LC_Quadratic ret(ce);
    ret.rotate(getAngle());
    ret.move(m_data.center);
    return ret;
}

LC_SecondMoment RS_Ellipse::computeLocalSecondMoment(double t1, double t2) const {
    const double a = getMajorRadius();
    const double b = getMinorRadius();

    auto F_ixx = [&](double t) {
        const double s2 = std::sin(2.0 * t);
        const double s4 = std::sin(4.0 * t);
        return (a * a * a * b / 24.0) * (3.0 * t + 2.0 * s2 + 0.25 * s4);
    };

    auto F_iyy = [&](double t) {
        const double s2 = std::sin(2.0 * t);
        const double s4 = std::sin(4.0 * t);
        return (a * b * b * b / 24.0) * (3.0 * t - 2.0 * s2 + 0.25 * s4);
    };

    auto F_ixy = [&](double t) {
        return -(a * a * b * b / 8.0) * std::pow(std::cos(t), 4);
    };

    LC_SecondMoment m;
    m.ixx = F_ixx(t2) - F_ixx(t1);
    m.iyy = F_iyy(t2) - F_iyy(t1);
    m.ixy = F_ixy(t2) - F_ixy(t1);
    return m;
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 * \oint x dy = Cx y + \frac{1}{4}((a^{2}+b^{2})sin(2a)cos^{2}(t)-ab(2sin^{2}(a)sin(2t)-2t-sin(2t)))
 *@author Dongxu Li
 */
double RS_Ellipse::areaLineIntegral() const {
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (!isEllipticArc()) {
      return M_PI * a * b;
  }
  const double ab = a * b;
  const double r2 = (a * a) + (b * b);
  const double& cx = m_data.center.x;
  const double aE = getAngle();
  const double y_start = getStartpoint().y;
  const double y_end = getEndpoint().y;
  const double start_angle = m_data.angle1;
  const double end_angle = isReversed() ? start_angle - getAngleLength() : start_angle + getAngleLength();
  const double aE2 = aE + aE;
  const auto antiDerivative = [&cx, c1 = r2 * std::sin(aE2), c2 = std::cos(aE2), &ab](double t, double y) {
    const double t2 = t + t;
    return (cx * y) + ((c1 + c1 * std::cos(t2) + 2. * ab * (t2 + c2 * std::sin(t2))) / 8.);
  };

  return antiDerivative(end_angle, y_end) - antiDerivative(start_angle, y_start);
}

bool RS_Ellipse::isReversed() const {
    return m_data.reversed;
}

void RS_Ellipse::setReversed(const bool r) {
    m_data.reversed = r;
}

LC_FirstMoment RS_Ellipse::computeLocalFirstMoment(double t0, double t1) const {
    const double a = getMajorRadius();
    const double b = getMinorRadius();
    // mx = (a²b/2) ∫ cos³t dt = (a²b/2)[sin t − sin³t/3]
    // my = (ab²/2) ∫ sin³t dt = (ab²/2)[−cos t + cos³t/3]
    auto F_mx = [&](double t) {
        const double st = std::sin(t);
        return (a * a * b / 2.0) * (st - st * st * st / 3.0);
    };
    auto F_my = [&](double t) {
        const double ct = std::cos(t);
        return (a * b * b / 2.0) * (-ct + ct * ct * ct / 3.0);
    };
    return {F_mx(t1) - F_mx(t0), F_my(t1) - F_my(t0)};
}

/**
 * @brief firstMomentLineIntegral – exact 1st-order moments via Green's theorem
 *        (local aligned frame + rotation + parallel-axis shift)
 */
LC_FirstMoment RS_Ellipse::firstMomentLineIntegral() const {
    if (!isEllipticArc()) {
        const double area = M_PI * getMajorRadius() * getMinorRadius();
        return {m_data.center.x * area, m_data.center.y * area};
    }

    const double phi = getAngle();
    const double cx  = m_data.center.x;
    const double cy  = m_data.center.y;

    double t0 = m_data.angle1;
    double t1 = isReversed() ? t0 - getAngleLength() : t0 + getAngleLength();

    const auto local = computeLocalFirstMoment(t0, t1);
    const double area = computeLocalArea(t0, t1);

    return local.rotated(phi).shifted(-cx, -cy, area);
}

/**
 * @brief secondMomentLineIntegral – exact 2nd-order moments via Green's theorem
 *        (local aligned frame + rotation + parallel-axis shift)
 */
LC_SecondMoment RS_Ellipse::secondMomentLineIntegral() const {
    const double cx  = m_data.center.x;
    const double cy  = m_data.center.y;
    const double phi = getAngle();
    if (!isEllipticArc()) {
        // Full ellipse – exact closed-form (original fast formula kept)
        const double a   = getMajorRadius();
        const double b   = getMinorRadius();
        const double cosP = std::cos(phi);
        const double sinP = std::sin(phi);
        const double piab = M_PI * a * b;

        return {
            piab * (cx*cx + (a*a*cosP*cosP + b*b*sinP*sinP) / 4.0),
            piab * (cy*cy + (a*a*sinP*sinP + b*b*cosP*cosP) / 4.0),
            piab * (cx*cy + (a*a - b*b) * cosP * sinP / 4.0)
        };
    }

    double t0 = m_data.angle1;
    double t1 = isReversed() ? t0 - getAngleLength() : t0 + getAngleLength();

    const auto local = computeLocalSecondMoment(t0, t1);
    const double area = computeLocalArea(t0, t1);

    return local.rotated(phi).shifted(-cx, -cy, area);
}


double RS_Ellipse::getAngle() const {
    return m_data.majorP.angle();
}

double RS_Ellipse::getAngle1() const {
    return m_data.angle1;
}

void RS_Ellipse::setAngle1(const double a1) {
    m_data.setAngle1(a1);
}

double RS_Ellipse::getAngle2() const {
    return m_data.angle2;
}

void RS_Ellipse::setAngle2(const double a2) {
    m_data.setAngle2(a2);
}

RS_Vector RS_Ellipse::getCenter() const {
    return m_data.center;
}

void RS_Ellipse::setCenter(const RS_Vector& c) {
    m_data.center = c;
}

const RS_Vector& RS_Ellipse::getMajorP() const {
    return m_data.majorP;
}

void RS_Ellipse::setMajorP(const RS_Vector& p) {
    m_data.majorP = p;
}

double RS_Ellipse::getRatio() const {
    return m_data.ratio;
}

void RS_Ellipse::setRatio(const double r) {
    m_data.ratio = r;
}

double RS_Ellipse::getAngleLength() const {
    double a = getAngle1();
    double b = getAngle2();

    if (isReversed()) {
        std::swap(a, b);
    }
    double ret = RS_Math::correctAngle(b - a);
    // full ellipse:
    if (std::abs(std::remainder(ret, 2. * M_PI)) < RS_TOLERANCE_ANGLE) {
        ret = 2 * M_PI;
    }

    return ret;
}

double RS_Ellipse::getMajorRadius() const {
    return m_data.majorP.magnitude(); // fixme - renderperf - cache !!!!!
}

RS_Vector RS_Ellipse::getMajorPoint() const {
    return m_data.center + m_data.majorP;
}

RS_Vector RS_Ellipse::getMinorPoint() const {
    return m_data.center + RS_Vector(-m_data.majorP.y, m_data.majorP.x) * m_data.ratio;
}

double RS_Ellipse::getMinorRadius() const {
    return m_data.majorP.magnitude() * m_data.ratio;
}

void RS_Ellipse::draw(RS_Painter* painter) {
  if (painter == nullptr) {
      return;
  }

  const double uiRadius = painter->toGuiDX((getRatio() > 1.) ? getMajorRadius() : getMinorRadius());
  if (uiRadius <= double(RS_Painter::getMaximumArcNonErrorRadius())) {
    const double majorPDegrees = RS_Math::rad2deg(getMajorP().angle());
    if (isArc()) {
      painter->drawEllipseArcWCS(getCenter(), getMajorRadius(), getRatio(), majorPDegrees,
                                 RS_Math::rad2deg(getAngle1()),
                                 RS_Math::rad2deg(getAngleLength()),
                                 isReversed());
    } else {
      painter->drawEllipseWCS(getCenter(), getMajorRadius(), getRatio(), majorPDegrees);

    }
    return;
  }

  const RS_Vector startUi = painter->toGui(isArc() ? getStartpoint() : getCenter() + getMajorP());
  const QPointF startPos{startUi.x, startUi.y};
  QPainterPath path(startPos);
  path.moveTo(startPos);
  createPainterPath(painter, path);
  // A full ellipse is a closed contour; close the subpath so the stroke uses
  // the pen's join style at the closure point rather than its cap style.
  if (!isArc())
    path.closeSubpath();
  painter->drawPath(path);
}

void RS_Ellipse::createPainterPath(RS_Painter* painter, QPainterPath& path) const {
    const double baseAngle = getAngle1();
    double fullAngleLength = isArc() ? getAngleLength() : 2 * M_PI;
    if (isArc() && isReversed()) {
        fullAngleLength = - fullAngleLength;
    }
    auto getParamFunc = [this](const RS_Vector& vp) { return getEllipseAngle(vp); };
    auto getPointFunc = [this](double param) { return getEllipsePoint(param); };
    // pathForParametricCurve calibrates the step assuming an arc with constant
    // curvature radius equal to approxRadius and uniform-arc-length sampling.
    // We sample uniformly in the ellipse's angle parameter t, so combining the
    // local arc length |r'(t)| = sqrt(a^2 sin^2 t + b^2 cos^2 t) with the local
    // curvature radius rho(t) yields the per-segment error
    //     e(t) = (ab)^3 h^4 / (24 (a^2 sin^2 t + b^2 cos^2 t)^(5/2))
    // which is maximized at the SHARPER axis tip (smallest a^2 s^2 + b^2 c^2).
    // The bound is e_max = max(a,b)^3 h^4 / (24 min(a,b)^2), so passing
    // approxRadius = max^3 / min^2 keeps the calibration's 1-px target valid.
    const double a = getMajorRadius();
    const double b = getMinorRadius();
    const double maxSemi = std::max(a, b);
    const double minSemi = std::min(a, b);
    const double approxRadius =
        (minSemi > RS_TOLERANCE)
            ? (maxSemi * maxSemi * maxSemi) / (minSemi * minSemi)
            : maxSemi;
    painter->pathForEntity(path, this, baseAngle, fullAngleLength, getParamFunc,
                           getPointFunc, approxRadius);
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Ellipse& a) {
    os << " Ellipse: " << a.m_data << "\n";
    return os;
}
