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

#include "rs_point.h"

class LC_Crosshair:public RS_Point{
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

    LC_Crosshair(RS_EntityContainer *parent, const RS_Vector &coord, int shapeType, int linesType, const RS_Pen& linesPen,int pointSize,
                 int pointType);
    void draw(RS_Painter *painter, RS_GraphicView *view, double &patternOffset) override;
    void setLinesPen(const RS_Pen &linesPen);

    void setPointType(int pointType);

    void setPointSize(int pointSize);

protected:
    int linesShape;
    int indicatorShape;
    RS_Pen linesPen;
    int pointType;
    int pointSize;

    double drawIndicator(RS_Painter *painter, RS_GraphicView *view, const RS_Vector &guiPos);

    void drawCrosshairLines(
        RS_Painter *painter, const RS_Vector &guiCoord, double offset, const RS_Vector &p1, const RS_Vector &p2, const RS_Vector &p3,
        const RS_Vector &p4) const;
};

#endif // LC_CROSSHAIR_H
