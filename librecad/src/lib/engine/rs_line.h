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


#ifndef RS_LINE_H
#define RS_LINE_H

#include "rs_atomicentity.h"

class LC_Quadratic;

/**
 * Holds the data that defines a line.
 */
struct RS_LineData {
    RS_Vector startpoint;
    RS_Vector endpoint;
};

std::ostream& operator << (std::ostream& os, const RS_LineData& ld);

/**
 * Class for a line entity.
 *
 * @author Andrew Mustun
 */
class RS_Line : public RS_AtomicEntity {
public:
	RS_Line() = default;
    RS_Line(RS_EntityContainer* parent,
            const RS_LineData& d);
    RS_Line(RS_EntityContainer* parent, const RS_Vector& pStart, const RS_Vector& pEnd);
    RS_Line(const RS_Vector& pStart, const RS_Vector& pEnd);

	virtual RS_Entity* clone() const;

	virtual ~RS_Line() = default;

    /**	@return RS2::EntityLine */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityLine;
    }
    /** @return true */
    virtual bool isEdge() const {
        return true;
    }

    /** @return Copy of data that defines the line. */
    RS_LineData getData() const {
        return data;
    }

	virtual RS_VectorSolutions getRefPoints() const;

    /** @return Start point of the entity */
    virtual RS_Vector getStartpoint() const {
        return data.startpoint;
    }
    /** @return End point of the entity */
    virtual RS_Vector getEndpoint() const {
        return data.endpoint;
    }
    /** Sets the startpoint */
    void setStartpoint(RS_Vector s) {
        data.startpoint = s;
        calculateBorders();
    }
    /** Sets the endpoint */
    void setEndpoint(RS_Vector e) {
        data.endpoint = e;
        calculateBorders();
    }
    /**
     * @return Direction 1. The angle at which the line starts at
     * the startpoint.
     */
    double getDirection1() const {
        return getAngle1();
    }
    /**
     * @return Direction 2. The angle at which the line starts at
     * the endpoint.
     */
    double getDirection2() const {
        return getAngle2();
    }
    virtual RS_Vector getTangentDirection(const RS_Vector& point)const;

    virtual void moveStartpoint(const RS_Vector& pos);
    virtual void moveEndpoint(const RS_Vector& pos);
    virtual RS2::Ending getTrimPoint(const RS_Vector& trimCoord,
                                     const RS_Vector& trimPoint);
    virtual RS_Vector prepareTrim(const RS_Vector& trimCoord,
                                  const RS_VectorSolutions& trimSol);
    virtual void reverse();
    /** Sets the y coordinate of the startpoint */
    void setStartpointY(double val) {
        data.startpoint.y = val;
        calculateBorders();
    }
    /** Sets the y coordinate of the endpoint */
    void setEndpointY(double val) {
        data.endpoint.y = val;
        calculateBorders();
    }
    virtual bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2);

    /**
     * @return The length of the line.
     */
    virtual double getLength() const {
        return data.startpoint.distanceTo(data.endpoint);
    }

    /**
     * @return The angle of the line (from start to endpoint).
     */
    virtual double getAngle1() const {
        return data.startpoint.angleTo(data.endpoint);
    }

    /**
     * @return The angle of the line (from end to startpoint).
     */
    virtual double getAngle2() const {
        return data.endpoint.angleTo(data.startpoint);
    }
	virtual bool isTangent(const RS_CircleData&  circleData) const;

/**
  * @return a perpendicular vector
  */
    RS_Vector getNormalVector() const;
    virtual RS_Vector getMiddlePoint()const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr)const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
			bool onEntity=true, double* dist = nullptr, RS_Entity** entity=nullptr)const;
//    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
//                                       double* dist = nullptr);
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
									   double* dist = nullptr,
                                       int middlePoints = 1
                                       )const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const;
    virtual RS_Vector getNearestDist(double distance,
									 bool startp)const;

    /**
          * implementations must revert the direction of an atomic entity
          */
    virtual void revertDirection();
	 virtual std::vector<RS_Entity* > offsetTwoSides(const double& distance) const;
    /**
      * the modify offset action
      */
    virtual bool offset(const RS_Vector& coord, const double& distance);
    virtual void move(const RS_Vector& offset);
    virtual void rotate(const double& angle);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& factor);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset);
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    /** whether the entity's bounding box intersects with visible portion of graphic view */
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);

    friend std::ostream& operator << (std::ostream& os, const RS_Line& l);

    virtual void calculateBorders();
	/** \brief getQuadratic() returns the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    virtual LC_Quadratic getQuadratic() const;
    /**
	 * @brief areaLineIntegral line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * \oint x dy = 0.5*(x0+x1)*(y1-y0)
     */
    virtual double areaLineIntegral() const;

protected:
	RS_LineData data;
};

#endif
