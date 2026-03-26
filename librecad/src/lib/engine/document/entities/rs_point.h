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
    RS_PointData() = default;

    explicit RS_PointData(const RS_Vector& pos) : pos(pos) {
    }

    friend std::ostream& operator <<(std::ostream& os, const RS_PointData& pd);
    RS_Vector pos;
};

/**
 * Class for a point entity.
 *
 * @author Andrew Mustun
 */
class RS_Point : public RS_AtomicEntity {
public:
    RS_Point(RS_EntityContainer* parent, const RS_Vector& c);
    RS_Point(RS_EntityContainer* parent, const RS_PointData& d);
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
    RS_Vector getMiddlePoint() const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;
    friend std::ostream& operator<<(std::ostream& os, const RS_Point& p);
    /** Recalculates the borders of this entity. */
    void calculateBorders() override;

    //RS_Vector point;
    void draw(RS_Painter* painter) override;
protected:
    RS_PointData m_data;
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
};
#endif
