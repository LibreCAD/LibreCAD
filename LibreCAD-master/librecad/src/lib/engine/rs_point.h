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
    RS_PointData(const RS_Vector& pos): pos(pos) {}

    friend std::ostream& operator << (std::ostream& os, const RS_PointData& pd);

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

	RS_Entity* clone() const override;

    /**	@return RS_ENTITY_POINT */
	RS2::EntityType rtti() const override;

    /**
         * @return Start point of the entity.
         */
	RS_Vector getStartpoint() const override;
    /**
         * @return End point of the entity.
         */
	RS_Vector getEndpoint() const override;

		void moveStartpoint(const RS_Vector& pos) override;

    /** @return Copy of data that defines the point. */
    RS_PointData getData() const;

	RS_VectorSolutions getRefPoints() const override;

    /** @return Position of the point */
    RS_Vector getPos() const;

    /** Sets a new position for this point. */
    void setPos(const RS_Vector& pos);
	RS_Vector getCenter() const override;
	double getRadius() const override;
	bool isTangent(const RS_CircleData& circleData) const override;

	RS_Vector getMiddlePoint(void)const override;
	RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr)const override;
	RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
			bool onEntity = true, double* dist = nullptr, RS_Entity** entity = nullptr)const override;
	RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = nullptr)const override;
	RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = nullptr,
									   int middlePoints = 1)const override;
	RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const override;
	double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=nullptr,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
							  double solidDist = RS_MAXDOUBLE)const override;

	void move(const RS_Vector& offset) override;
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

	void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

    friend std::ostream& operator << (std::ostream& os, const RS_Point& p);

    /** Recalculates the borders of this entity. */
	void calculateBorders () override;

protected:
    RS_PointData data;
    //RS_Vector point;
};
#endif
