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


#ifndef RS_POINT_H
#define RS_POINT_H

#include "rs_atomicentity.h"

/**
 * Holds the data that defines a point.
 */
struct RS_PointData {
    RS_PointData(const RS_Vector& pos) {
        this->pos = pos;
    }

    friend std::ostream& operator << (std::ostream& os, const RS_PointData& pd) {
        os << "(" << pd.pos << ")";
        return os;
    }

    RS_Vector pos;
};

/**
 * Class for a point entity.
 *
 * @author Andrew Mustun
 */
class RS_Point : public RS_AtomicEntity {
public:
    RS_Point(RS_EntityContainer* parent,
             const RS_PointData& d);

	virtual RS_Entity* clone() const;

    /**	@return RS_ENTITY_POINT */
    virtual RS2::EntityType rtti() const;

    /**
         * @return Start point of the entity.
         */
    virtual RS_Vector getStartpoint() const;
    /**
         * @return End point of the entity.
         */
    virtual RS_Vector getEndpoint() const;

        virtual void moveStartpoint(const RS_Vector& pos);

    /** @return Copy of data that defines the point. */
    RS_PointData getData() const;

	virtual RS_VectorSolutions getRefPoints() const;

    /** @return Position of the point */
    RS_Vector getPos() const;

    /** Sets a new position for this point. */
    void setPos(const RS_Vector& pos);
    virtual RS_Vector getCenter() const;
    virtual double getRadius() const;
    virtual bool isTangent(const RS_CircleData& circleData) const;

    virtual RS_Vector getMiddlePoint(void)const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = nullptr)const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = nullptr, RS_Entity** entity = nullptr)const;
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = nullptr)const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = nullptr,
                                       int middlePoints = 1)const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = nullptr)const;
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=nullptr,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                                                          double solidDist = RS_MAXDOUBLE)const;

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);

    friend std::ostream& operator << (std::ostream& os, const RS_Point& p);

    /** Recalculates the borders of this entity. */
    virtual void calculateBorders ();

protected:
    RS_PointData data;
    //RS_Vector point;
};
#endif
