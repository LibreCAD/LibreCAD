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

/**
 * Holds the data that defines a construction line (a line
 * which is not limited to both directions).
 */
struct RS_ConstructionLineData {
    /**
  * Default constructor
     */
    RS_ConstructionLineData() = default;

    RS_ConstructionLineData(const RS_Vector& point1, const RS_Vector& point2);

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
    RS_ConstructionLine() = default;
    RS_ConstructionLine(RS_EntityContainer* parent, const RS_ConstructionLineData& d);
    RS_ConstructionLine(const RS_Vector& point1, const RS_Vector& point2);
    RS_Entity* clone() const override;
    ~RS_ConstructionLine() override = default;

    /** @return RS2::EntityConstructionLine */
    RS2::EntityType rtti() const override {
        return RS2::EntityConstructionLine;
    }

    /** @return Copy of data that defines the line. */
    const RS_ConstructionLineData& getData() const;
    /** @return First definition point. */
    const RS_Vector& getPoint1() const;
    /** @return Second definition point. */
    const RS_Vector& getPoint2() const;
    /** @return Start point of the entity */
    RS_Vector getStartpoint() const override;
    /** @return End point of the entity */
    RS_Vector getEndpoint() const override;
    double getDirection1() const override;
    double getDirection2() const override;

    /** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    LC_Quadratic getQuadratic() const override;
    RS_Vector getMiddlePoint() const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;

    void draw(RS_Painter* painter) override;
    friend std::ostream& operator <<(std::ostream& os, const RS_ConstructionLine& l);
    void calculateBorders() override;
protected:
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
private:
    RS_ConstructionLineData m_data;
};

#endif
