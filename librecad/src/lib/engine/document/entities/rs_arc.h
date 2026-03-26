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

#ifndef RS_ARC_H
#define RS_ARC_H

#include "lc_cachedlengthentity.h"
#include "rs_math.h"

class LC_Quadratic;

/**
 * Holds the data that defines an arc.
 */
struct RS_ArcData {
    RS_ArcData() = default;

    RS_ArcData(const RS_Vector& center, double radius, double angle1, double angle2, bool reversed);

    void reset();

    bool isValid() const;

    RS_Vector center;
    double radius = 0.;
    double angle1 = 0.;
    double angle2 = 0.;
    bool reversed = false;

    // cached value for draw()
    double startAngleDegrees = 0.;
    double otherAngleDegrees = 0.;
    double angularLength = 0.;

    double getDirection1() const {
        if (!reversed) {
            return RS_Math::correctAngle(angle1 + M_PI_2);
        }
        return RS_Math::correctAngle(angle1 - M_PI_2);
    }

    /**
     * @return Direction 2. The angle at which the arc starts at
     * the endpoint.
     */
    double getDirection2() const {
        if (!reversed) {
            return RS_Math::correctAngle(angle2 - M_PI_2);
        }
        return RS_Math::correctAngle(angle2 + M_PI_2);
    }

    /**
    * @return Angle length in rad.
    */
    double getAngleLength() const {
        double a = angle1;
        double b = angle2;

        if (reversed) {
            std::swap(a, b);
        }
        double ret = RS_Math::correctAngle(b - a);
        // full circle:
        if (std::abs(std::remainder(ret, 2. * M_PI)) < RS_TOLERANCE_ANGLE) {
            ret = 2 * M_PI;
        }

        return ret;
    }

    double getBulge() const {
        const double bulge = std::tan(std::abs(getAngleLength()) / 4.0);
        return reversed ? -bulge : bulge;
    }

    RS_Vector getEndpoint() const {
        return center.relative(radius, angle2);
    }
};

std::ostream& operator <<(std::ostream& os, const RS_ArcData& ad);

/**
 * Class for an arc entity. All angles are in Rad.
 *
 * @author Andrew Mustun
 */
class RS_Arc : public LC_CachedLengthEntity {
public:
    RS_Arc() = default;
    RS_Arc(RS_EntityContainer* parent, const RS_ArcData& d);
    explicit RS_Arc(const RS_ArcData& d);
    RS_Entity* clone() const override;

    /**	@return RS2::EntityArc */
    RS2::EntityType rtti() const override {
        return RS2::EntityArc;
    }

    /** @return true */
    bool isEdge() const override {
        return true;
    }

    /** @return Copy of data that defines the arc. **/
    const RS_ArcData& getData() const {
        return m_data;
    }

    RS_ArcData& getData() {
        return m_data;
    }

    RS_VectorSolutions getRefPoints() const override;

    /** Sets new arc parameters. **/
    void setData(const RS_ArcData& d) {
        m_data = d;
    }

    /** @return The center point (x) of this arc */
    RS_Vector getCenter() const override {
        return m_data.center;
    }

    /** Sets new center. */
    void setCenter(const RS_Vector& center);

    /** @return The radius of this arc */
    double getRadius() const override {
        return m_data.radius;
    }

    /** Sets new radius. */
    void setRadius(double radius) override;

    /** @return The start angle of this arc */
    double getAngle1() const {
        return m_data.angle1;
    }

    /** Sets new start angle. */
    void setAngle1(double a1);

    /** @return The end angle of this arc */
    double getAngle2() const {
        return m_data.angle2;
    }

    /** Sets new end angle. */
    void setAngle2(double a2);

    /** get angle relative arc center*/
    double getArcAngle(const RS_Vector& vp) const {
        return (vp - m_data.center).angle();
    }

    /**
     * @brief getPointAtParameter - get arc point at the given angle
     * @param angle - arc angle
     * @return RS_Vector - arc point, which may not be within the arc angular range
     */
    RS_Vector getPointAtParameter(const double angle) const {
        return getCenter() + RS_Vector::polar(getRadius(), angle);
    }

    /**
     * @return Direction 1. The angle at which the arc starts at
     * the startpoint.
     */
    double getDirection1() const override;
    /**
     * @return Direction 2. The angle at which the arc starts at
     * the endpoint.
     */
    double getDirection2() const override;

    /**
     * @retval true if the arc is reversed (clockwise),
     * @retval false otherwise
     */
    bool isReversed() const {
        return m_data.reversed;
    }

    /** sets the reversed status. */
    void setReversed(bool r);

    /** @return Start point of the entity. */
    RS_Vector getStartpoint() const override;
    /** @return End point of the entity. */
    RS_Vector getEndpoint() const override;
    std::vector<RS_Entity*> offsetTwoSides(double distance) const override;
    /**
          * implementations must revert the direction of an atomic entity
          */
    void revertDirection() override;
    void correctAngles(); //make sure angleLength() is not more than 2*M_PI
    void moveStartpoint(const RS_Vector& pos) override;
    void moveEndpoint(const RS_Vector& pos) override;
    bool offset(const RS_Vector& coord, double distance) override;

    void trimStartpoint(const RS_Vector& pos) override;
    void trimEndpoint(const RS_Vector& pos) override;

    RS2::Ending getTrimPoint(const RS_Vector& trimCoord, const RS_Vector& trimPoint) override;
    /** choose an intersection to trim to based on mouse point */
    RS_Vector prepareTrim(const RS_Vector& trimCoord, const RS_VectorSolutions& trimSol) override;
    void reverse() override;
    RS_Vector getMiddlePoint() const override;
    double getAngleLength() const {return m_data.getAngleLength();}
    double getBulge() const;
    double getSagitta() const;

    RS_Vector getNearestDistToEndpoint(double distance, bool startp) const override;
    RS_Vector getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, bool onEntity = false) const override;
    RS_Vector dualLineTangentPoint(const RS_Vector& line) const override;
    RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override; //find the tangential points seeing from given point
    RS_Vector getTangentDirection(const RS_Vector& point) const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    /**
     * @description:    Implementation of the Shear/Skew the entity
     *                  The shear transform is
     *                  1  k  0
     *                  0  1  0
     *                        1
     * @author          Dongxu Li
     * @param k the skew/shear parameter
     */
    RS_Entity& shear(double k) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) override;

    void draw(RS_Painter* painter) override;

    friend std::ostream& operator <<(std::ostream& os, const RS_Arc& a);

    void calculateBorders() override;
    /** return the equation of the entity
        for quadratic,
        return a vector contains:
        m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
    for linear:
       m0 x + m1 y + m2 =0
    **/
    LC_Quadratic getQuadratic() const override;
    /**
     * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * \oint x dy = c_x r \sin t + \frac{1}{4}r^2\sin 2t +  \frac{1}{2}r^2 t
     */
    double areaLineIntegral() const override;

    void updateMiddlePoint();

    double getChord() const {
        return m_startPoint.distanceTo(m_endPoint);
    }

protected:
    RS_ArcData m_data{};
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;

private:
    // cached values for performance
    RS_Vector m_middlePoint;
    RS_Vector m_startPoint;
    RS_Vector m_endPoint;

    void updateLength() override;
    void updatePaintingInfo();
    void moveMiddlePoint(const RS_Vector& vector);
};

#endif
