/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_atomicentity.h"
#include "rs_vector.h"


/**
 * Holds the data that defines a solid.
 */
class RS_SolidData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_SolidData() {
        for (int i=0; i<4; ++i) {
            corner[i] = RS_Vector(false);
        }
    }

    /**
     * Constructor for a solid with 3 corners.
     */
    RS_SolidData(const RS_Vector& corner1,
                 const RS_Vector& corner2,
                 const RS_Vector& corner3) {

        corner[0] = corner1;
        corner[1] = corner2;
        corner[2] = corner3;
        corner[3] = RS_Vector(false);
    }

    /**
     * Constructor for a solid with 4 corners.
     */
    RS_SolidData(const RS_Vector& corner1,
                 const RS_Vector& corner2,
                 const RS_Vector& corner3,
                 const RS_Vector& corner4) {

        corner[0] = corner1;
        corner[1] = corner2;
        corner[2] = corner3;
        corner[3] = corner4;
    }

    friend class RS_Solid;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_SolidData& pd) {
        os << "(";
        for (int i=0; i<4; i++) {
            os << pd.corner[i];
        }
        os << ")";
        return os;
    }

private:
    RS_Vector corner[4];
};



/**
 * Class for a solid entity (e.g. dimension arrows).
 *
 * @author Andrew Mustun
 */
class RS_Solid : public RS_AtomicEntity {
public:
    RS_Solid(RS_EntityContainer* parent,
             const RS_SolidData& d);

    virtual RS_Entity* clone() {
        RS_Solid* s = new RS_Solid(*this);
        s->initId();
        return s;
    }

    /**	@return RS_ENTITY_POINT */
    virtual RS2::EntityType rtti() const {
        return RS2::EntitySolid;
    }
	
    /** 
	 * @return Start point of the entity. 
	 */
    virtual RS_Vector getStartpoint() const {
        return RS_Vector(false);
    }
    /** 
	 * @return End point of the entity. 
	 */
    virtual RS_Vector getEndpoint() const {
        return RS_Vector(false);
    }


    /** @return Copy of data that defines the point. */
    RS_SolidData getData() const {
        return data;
    }

    /** @return true if this is a triangle. */
    bool isTriangle() {
        return !data.corner[3].valid;
    }

    RS_Vector getCorner(int num);

    void shapeArrow(const RS_Vector& point,
                    double angle,
                    double arrowSize);

    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL);
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = NULL, RS_Entity** entity = NULL);
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = NULL);
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL);
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = NULL);

    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
									  double solidDist = RS_MAXDOUBLE);

    virtual void move(RS_Vector offset);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double patternOffset=0.0);

    friend std::ostream& operator << (std::ostream& os, const RS_Solid& p);

    /** Recalculates the borders of this entity. */
    virtual void calculateBorders ();

protected:
    RS_SolidData data;
    //RS_Vector point;
}
;

#endif
