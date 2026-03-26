/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

/**
 * Holds the data that defines a solid.
 */
struct RS_SolidData {
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_SolidData();

    /**
     * Constructor for a solid with 3 corners.
     */
    RS_SolidData(const RS_Vector& corner1, const RS_Vector& corner2, const RS_Vector& corner3);

    /**
     * Constructor for a solid with 4 corners.
     */
    RS_SolidData(const RS_Vector& corner1, const RS_Vector& corner2, const RS_Vector& corner3, const RS_Vector& corner4);

    enum Corners {
        FirstCorner = 0,
        Triangle    = 3,
        MaxCorners  = 4
    };

    std::array<RS_Vector, MaxCorners> corners;
};

std::ostream& operator <<(std::ostream& os, const RS_SolidData& pd);

/**
 * Class for a solid entity (e.g. dimension arrows).
 *
 * @author Andrew Mustun
 */
class RS_Solid : public RS_AtomicEntity {
public:
    RS_Solid(RS_EntityContainer* parent, const RS_SolidData& d);
    explicit RS_Solid(const RS_SolidData& d);

    RS_Entity* clone() const override;

    /** @return RS_ENTITY_POINT */
    RS2::EntityType rtti() const override {
        return RS2::EntitySolid;
    }

    /** @return Copy of data that defines the point. */
    const RS_SolidData& getData() const {
        return m_data;
    }

    /** @return true if this is a triangle. */
    bool isTriangle() const {
        return !m_data.corners[3].valid;
    }

    RS_Vector getCorner(int num) const;
    RS_Vector unsafeGetCorner(int num) const;

    void shapeArrow(const RS_Vector& point, double angle, double arrowSize);
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    void draw(RS_Painter* painter) override;

    friend std::ostream& operator <<(std::ostream& os, const RS_Solid& p);

    /** Recalculates the borders of this entity. */
    void calculateBorders() override;

    /** Check if is intersected by v1, v2 window.
    * @return true if is crossed false otherwise.
    **/
    bool isInCrossWindow(const RS_Vector& v1, const RS_Vector& v2) const;

protected:
    RS_SolidData m_data;
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
private:
    //helper method for getNearestPointOnEntity
    bool sign(const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3) const;
    void setDistPtr(double* dist, double value) const;
};

#endif
