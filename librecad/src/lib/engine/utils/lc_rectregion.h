/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_RECTREGION_H
#define LC_RECTREGION_H

#include "rs_vector.h"

struct LC_TransformData{
    RS_Vector scaleFactor;
    RS_Vector moveOffset;
};

/*
 * Utility class that defines rectangular region. Unlike to LC_Rect, this region may be rotated and be not parallel to X,Y axis
 */
class LC_RectRegion{
public:
    LC_RectRegion();
    void rotate(double angle);
    void move(const RS_Vector& offset);
    void scale(const RS_Vector& factor);
    void setCorners(const RS_Vector& lBottom, const RS_Vector& lTop, const RS_Vector& rTop, const RS_Vector& rBottom);

    RS_VectorSolutions getAllPoints() const {return m_allPoints;}
    RS_VectorSolutions getCorners() const {return m_corners;}

    RS_Vector getRotatedPoint(const RS_Vector& vect, double angle) const;
    LC_RectRegion* getRotated(double angle) const;

    RS_Vector getLeftBottomCorner() const {return m_leftBottom;}

    LC_TransformData determineTransformData(RS_Vector ref, RS_Vector newRef);

protected:
    RS_Vector m_leftBottom;
    RS_Vector m_leftTop;
    RS_Vector m_rightBottom;
    RS_Vector m_rightTop;

    RS_Vector m_midLeft;
    RS_Vector m_midRight;
    RS_Vector m_midBottom;
    RS_Vector m_midTop;
    RS_Vector m_center;

    RS_VectorSolutions m_allPoints{8};
    RS_VectorSolutions m_corners{4};

    void updateOther();
};

#endif
