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


#ifndef RS_CONSTRUCTIONLINE_H
#define RS_CONSTRUCTIONLINE_H

#include "rs_atomicentity.h"
#include "rs_vector.h"

/**
 * Holds the data that defines a construction line (a line
 * which is not limited to both directions).
 */
class RS_ConstructionLineData {
public:
    /**
	 * Default constructor
     */
	RS_ConstructionLineData();

    RS_ConstructionLineData(const RS_Vector& point1,
							const RS_Vector& point2);

    friend class RS_ConstructionLine;

    friend std::ostream& operator << (std::ostream& os,
									  const RS_ConstructionLineData& ld);

private:
    RS_Vector point1;
    RS_Vector point2;
};


/**
 * Class for a construction line entity.
 *
 * @author Andrew Mustun
 */
class RS_ConstructionLine : public RS_AtomicEntity {
public:
	RS_ConstructionLine()=default;
    RS_ConstructionLine(RS_EntityContainer* parent,
                        const RS_ConstructionLineData& d);

	virtual RS_Entity* clone() const;

	virtual ~RS_ConstructionLine()=default;

    /**	@return RS2::EntityConstructionLine */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityConstructionLine;
    }

    /** @return Copy of data that defines the line. */
	RS_ConstructionLineData const& getData() const;

    /** @return First definition point. */
	RS_Vector const& getPoint1() const;
    /** @return Second definition point. */
	RS_Vector const& getPoint2() const;

    /** @return Start point of the entity */
    RS_Vector getStartpoint() const override;
    /** @return End point of the entity */
    RS_Vector getEndpoint() const override;
    double getDirection1(void) const override;
    double getDirection2(void) const override;

    /** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    virtual LC_Quadratic getQuadratic() const;
    virtual RS_Vector getMiddlePoint(void) const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL)const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = NULL, RS_Entity** entity=NULL)const;
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = NULL)const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
                                       int middlePoints = 1)const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = NULL)const;
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                                                          double solidDist = RS_MAXDOUBLE) const;

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

    virtual void draw(RS_Painter* /*painter*/, RS_GraphicView* /*view*/,
                double& /*patternOffset*/) {}

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_ConstructionLine& l);

    virtual void calculateBorders();

protected:
    RS_ConstructionLineData data;
};

#endif
