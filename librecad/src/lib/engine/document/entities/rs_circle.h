/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_CIRCLE_H
#define RS_CIRCLE_H

#include "lc_cachedlengthentity.h"
#include "rs_vector.h"

class LC_Quadratic;
class QPainterPath;

/**
 * Holds the data that defines a circle.
 */
struct RS_CircleData {
    RS_CircleData() = default;
    RS_CircleData(const RS_Vector& center, double radius);
    bool isValid() const;
    bool operator ==(const RS_CircleData&) const;
    RS_Vector center;
    double radius = 0.;
};

std::ostream& operator <<(std::ostream& os, const RS_CircleData& ad);

/**
 * Class for a circle entity.
 *
 * @author Andrew Mustun
 */
class RS_Circle : public LC_CachedLengthEntity {
public:
    RS_Circle() = default;
    RS_Circle(RS_EntityContainer* parent, const RS_CircleData& d);
    explicit RS_Circle(const RS_CircleData& d);
    ~RS_Circle() override = default;

    RS_Entity* clone() const override;

    /**	@return RS2::EntityCircle */
    RS2::EntityType rtti() const override {
        return RS2::EntityCircle;
    }

    /** @return true */
    bool isEdge() const override {
        return true;
    }

    /** @return Copy of data that defines the circle. **/
    const RS_CircleData& getData() const {
        return m_data;
    }

    RS_VectorSolutions getRefPoints() const override;

    //no start/end point for whole circle
    //        RS_Vector getStartpoint() const {
    //                return data.center + RS_Vector(data.radius, 0.0);
    //        }
    //        RS_Vector getEndpoint() const {
    //                return data.center + RS_Vector(data.radius, 0.0);
    //        }
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

    /** @return The center point (x) of this arc */
    RS_Vector getCenter() const override;
    /** Sets new center. */
    void setCenter(const RS_Vector& c);
    /** @return The radius of this arc */
    double getRadius() const override;
    /** Sets new radius. */
    void setRadius(double r) override;
    double getAngleLength() const;
    bool isTangent(const RS_CircleData& circleData) const override;

    std::vector<RS_Entity*> offsetTwoSides(double distance) const override;

    RS_Vector getMiddlePoint() const override;
    RS_Vector getNearestDistToEndpoint(double distance, bool startp) const override;
    RS_Vector getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, bool onEntity = false) const override;

    RS_Vector dualLineTangentPoint(const RS_Vector& line) const override;

    bool offset(const RS_Vector& coord, double distance) override;
    RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override; //find the tangential points seeing from given point
    RS_Vector getTangentDirection(const RS_Vector& point) const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

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
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void draw(RS_Painter* painter) override;
    /** Creates QPainterPath representation (used for both drawing and solid fill contours) */
    void createPainterPath(RS_Painter* painter, QPainterPath& path) const;

    /** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    LC_Quadratic getQuadratic() const override;

    /**
    * @brief Returns area of full circle
    * Note: Circular arcs are handled separately by RS_Arc (areaLIneIntegral)
    * However, full ellipses and ellipse arcs are handled by RS_Ellipse
    * @return \pi r^2
    */
    double areaLineIntegral() const override;

    friend std::ostream& operator <<(std::ostream& os, const RS_Circle& a);

    void calculateBorders() override;

protected:
    RS_CircleData m_data;
    void updateLength() override;
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
};

#endif
