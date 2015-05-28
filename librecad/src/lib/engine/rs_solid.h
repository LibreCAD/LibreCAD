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
#ifndef RS_SOLID_H
#define RS_SOLID_H

#include <array>
#include "rs_atomicentity.h"
#include "rs_vector.h"


/**
 * Holds the data that defines a solid.
 */
struct RS_SolidData {
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
	RS_SolidData();
	~RS_SolidData() = default;

    /**
     * Constructor for a solid with 3 corners.
     */
    RS_SolidData(const RS_Vector& corner1,
                 const RS_Vector& corner2,
				 const RS_Vector& corner3);

    /**
     * Constructor for a solid with 4 corners.
     */
    RS_SolidData(const RS_Vector& corner1,
                 const RS_Vector& corner2,
                 const RS_Vector& corner3,
				 const RS_Vector& corner4);


	std::array<RS_Vector, 4> corner;
};


std::ostream& operator << (std::ostream& os, const RS_SolidData& pd);

/**
 * Class for a solid entity (e.g. dimension arrows).
 *
 * @author Andrew Mustun
 */
class RS_Solid : public RS_AtomicEntity {
public:
    RS_Solid(RS_EntityContainer* parent,
             const RS_SolidData& d);
	~RS_Solid() = default;

	virtual RS_Entity* clone() const;

    /**	@return RS_ENTITY_POINT */
    virtual RS2::EntityType rtti() const {
        return RS2::EntitySolid;
    }

    /** @return Copy of data that defines the point. */
	RS_SolidData const& getData() const {
        return data;
    }

    /** @return true if this is a triangle. */
	bool isTriangle() const{
        return !data.corner[3].valid;
    }

	RS_Vector getCorner(int num) const;

    void shapeArrow(const RS_Vector& point,
                    double angle,
                    double arrowSize);

    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL)const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = NULL, RS_Entity** entity = NULL)const;
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = NULL) const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
                                       int middlePoints = 1)const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = NULL)const;

    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                                                          double solidDist = RS_MAXDOUBLE)const;

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);

    friend std::ostream& operator << (std::ostream& os, const RS_Solid& p);

    /** Recalculates the borders of this entity. */
    virtual void calculateBorders ();

    /** Check if is intersected by v1, v2 window.
    * @return true if is crossed false otherwise.
    **/
    bool isInCrossWindow(const RS_Vector& v1,const RS_Vector& v2)const;

protected:
    RS_SolidData data;
    //RS_Vector point;

private:
    //helper metod for getNearestPointOnEntity
    bool sign (const RS_Vector v1, const RS_Vector v2, const RS_Vector v3) const;

}
;

#endif
