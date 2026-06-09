/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_CROSSHAIR_H
#define LC_CROSSHAIR_H

#include "lc_overlayentity.h"
#include "rs_pen.h"
#include "rs_vector.h"

class LC_Crosshair:public LC_OverlayDrawable{
public:
    enum IndicatorShape{
        Circle,
        Point,
        Square,
        Gap,
        NoShape
    };

    enum LinesShape{
        Adaptive,
        Crosshair,
        Spiderweb,
        NoLines
    };

    LC_Crosshair(const RS_Vector &coord, int shapeType, int linesType, const RS_Pen& linesPen,int pointSize,
                 int pointType);
    void draw(RS_Painter *painter) override;
    void setLinesPen(const RS_Pen &pen);
    void setPointType(int type);
    void setPointSize(int size);
    void setShapesPen(const RS_Pen &p) {m_shapePen = p;}
protected:
    int m_linesShape;
    int m_indicatorShape;
    RS_Pen m_linesPen;
    RS_Pen m_shapePen;
    int m_pointType;
    int m_pointSize;
    RS_Vector m_wcsPos;

    double drawIndicator(RS_Painter *painter, const RS_Vector& uiPos) const;

    void drawCrosshairLines(
        RS_Painter *painter, const RS_Vector &guiCoord, double offset, const RS_Vector &p1, const RS_Vector &p2, const RS_Vector &p3,
        const RS_Vector &p4) const;
};

#endif
