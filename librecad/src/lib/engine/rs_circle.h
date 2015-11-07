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

#include <vector>
#include "rs_atomicentity.h"

class LC_Quadratic;

/**
 * Holds the data that defines a circle.
 */
struct RS_CircleData {
	RS_CircleData() = default;
	RS_CircleData(RS_Vector const& center, double radius);
	bool isValid() const;
	bool operator == (RS_CircleData const&) const;
	RS_Vector center;
	double radius;
};

std::ostream& operator << (std::ostream& os, const RS_CircleData& ad);

/**
 * Class for a circle entity.
 *
 * @author Andrew Mustun
 */
class RS_Circle : public RS_AtomicEntity {
public:
	RS_Circle()=default;
    RS_Circle (RS_EntityContainer* parent,
               const RS_CircleData& d);
	~RS_Circle() = default;

	virtual RS_Entity* clone() const;

    /**	@return RS2::EntityCircle */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityCircle;
    }
    /** @return true */
    virtual bool isEdge() const {
        return true;
    }

    /** @return Copy of data that defines the circle. **/
	const RS_CircleData& getData() const {
        return data;
    }

	virtual RS_VectorSolutions getRefPoints() const;

    //no start/end point for whole circle
    //        virtual RS_Vector getStartpoint() const {
    //                return data.center + RS_Vector(data.radius, 0.0);
    //        }
    //        virtual RS_Vector getEndpoint() const {
    //                return data.center + RS_Vector(data.radius, 0.0);
    //        }
    /**
         * @return Direction 1. The angle at which the arc starts at
         * the startpoint.
         */
    double getDirection1() const; 
    /**
         * @return Direction 2. The angle at which the arc starts at
         * the endpoint.
         */
    double getDirection2() const;

    /** @return The center point (x) of this arc */
	virtual RS_Vector getCenter() const;
    /** Sets new center. */
	void setCenter(const RS_Vector& c);
    /** @return The radius of this arc */
	virtual double getRadius() const;
    /** Sets new radius. */
	void setRadius(double r);
    double getAngleLength() const;
    virtual double getLength() const;
	virtual bool isTangent(const RS_CircleData&  circleData) const;

    bool createFromCR(const RS_Vector& c, double r);
    bool createFrom2P(const RS_Vector& p1, const RS_Vector& p2);
    bool createFrom3P(const RS_Vector& p1, const RS_Vector& p2,
                      const RS_Vector& p3);
    bool createFrom3P(const RS_VectorSolutions& sol);
	bool createInscribe(const RS_Vector& coord, const std::vector<RS_Line*>& lines);
	virtual std::vector<RS_Entity* > offsetTwoSides(const double& distance) const;
	RS_VectorSolutions createTan1_2P(const RS_AtomicEntity* circle, const std::vector<RS_Vector> points);
	static RS_VectorSolutions createTan2(const std::vector<RS_AtomicEntity*>& circles, const double& r);
    /** solve one of the eight Appollonius Equations
| Cx - Ci|^2=(Rx+Ri)^2
with Cx the center of the common tangent circle, Rx the radius. Ci and Ri are the Center and radius of the i-th existing circle
**/
	static std::vector<RS_Circle> solveAppolloniusSingle(const std::vector<RS_Circle>& circles);

	std::vector<RS_Circle> createTan3(const std::vector<RS_AtomicEntity*>& circles);
	bool testTan3(const std::vector<RS_AtomicEntity*>& circles);
    virtual RS_Vector getMiddlePoint(void)const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = nullptr) const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
                                              bool onEntity = true, double* dist = NULL, RS_Entity** entity=NULL)const;
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = NULL)const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = nullptr,
                                       int middlePoints = 1 ) const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = NULL)const;
    virtual RS_Vector getNearestDist(double distance,
									 bool startp)const;
    virtual RS_Vector getNearestOrthTan(const RS_Vector& coord,
                                        const RS_Line& normal,
										bool onEntity = false) const;

    virtual bool offset(const RS_Vector& coord, const double& distance);
    virtual RS_VectorSolutions getTangentPoint(const RS_Vector& point) const;//find the tangential points seeing from given point
    virtual RS_Vector getTangentDirection(const RS_Vector& point)const;
    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
    /** whether the entity's bounding box intersects with visible portion of graphic view */
    virtual bool isVisibleInWindow(RS_GraphicView* view) const;
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
    /** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    virtual LC_Quadratic getQuadratic() const;
    
/**
* @brief Returns area of full circle
* Note: Circular arcs are handled separately by RS_Arc (areaLIneIntegral) 
* However, full ellipses and ellipse arcs are handled by RS_Ellipse
* @return \pi r^2
*/
    virtual double areaLineIntegral() const;

    friend std::ostream& operator << (std::ostream& os, const RS_Circle& a);

    virtual void calculateBorders();

protected:
    RS_CircleData data;
};

#endif
