/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_RELATIVE_POSITIONEVALUATOR_H
#define LC_RELATIVE_POSITIONEVALUATOR_H

#include "lc_graphicviewport.h"
#include "lc_relative_point_data.h"
#include "rs.h"

class RS_Vector;

class LC_RelativePositionEvaluator {
public:
    void update(const RS_Vector& wcsPos, const RS_Vector& baseWCSPoint);
    void setPositionParam(RS2::RelativePointParam paramType, double value, bool fromInteractiveInput);
    void setViewport(LC_GraphicViewport* viewport){m_viewport = viewport;}
    const LC_RelativePositionData* getRelativePositionData() const {return &m_relativeInputData;}
protected:
    void recalculateDeltas(const RS_Vector& wcsEndPoint);
    void recalculateLength(const RS_Vector& wcsBasePoint);
    void recalculateAngle(const RS_Vector& wcsBasePoint);
    void recalculateLengthAndAngle(const RS_Vector& wcsBasePoint);
    void calculatePointForGivenXKeepingLength(double length, double wcsAngle, RS_Vector& ucsProjectedPoint);
    void calculatePointForGivenYKeepingLength(double length, double wcsAngle, RS_Vector& ucsProjectedPoint);
    void calculatePointForGivenXKeepingAngle(RS_Vector& ucsProjectedPoint);
    void calculatePointForGivenYKeepingAngle(RS_Vector& ucsProjectedPoint);
    void calculateForX(RS_Vector wcsBasePoint, double length, double wcsAngle, RS_Vector ucsProjectedPoint);
    void calculateForY(RS_Vector wcsBasePoint, double length, double wcsAngle, RS_Vector ucsProjectedPoint);

private:
    LC_RelativePositionData m_relativeInputData;
    LC_GraphicViewport* m_viewport {nullptr};
};

#endif
