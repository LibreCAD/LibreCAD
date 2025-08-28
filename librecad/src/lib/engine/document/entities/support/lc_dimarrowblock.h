/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_DIMARROWBLOCK_H
#define LC_DIMARROWBLOCK_H

#include "rs_atomicentity.h"

class LC_DimArrow: public RS_AtomicEntity{
public:
    LC_DimArrow(RS_EntityContainer* parent, const RS_Vector &pos, double angle, double size)
        : RS_AtomicEntity{parent},
          m_position{pos},
          m_angle{angle},
          m_arrowSize{size} {
    }
    RS2::EntityType rtti() const override {return RS2::EntityDimArrowBlock;}

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector getNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
    void calculateBorders() override;
    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level,
                              double solidDist) const override;
    double getAngle() const {return m_angle;}
    void setAngle(double angle) {m_angle = angle;}
    double getArrowSize() const {return m_arrowSize;}
    RS_Vector getPosition() const {return m_position;}
    RS_Vector getDimLinePoint () const {return m_dimLinePoint;}
protected:
    void setDistPtr(double* dist, double value) const;
    virtual void doMove(const RS_Vector& offset);
    virtual void doRotate(const RS_Vector& center, RS_Vector angleVector);
    virtual void doScale(const RS_Vector& center, const RS_Vector& factor);
    virtual void doMirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void doCalculateBorders();
    void setDimLinePoint(const RS_Vector& pos);
    void positionDimLinePointFromZero(const RS_Vector &angleVector);
private:    
    RS_Vector m_position {};
    double m_angle {0.0};
    double m_arrowSize {0.0};
    RS_Vector m_dimLinePoint {};
};

#endif // LC_DIMARROWBLOCK_H
