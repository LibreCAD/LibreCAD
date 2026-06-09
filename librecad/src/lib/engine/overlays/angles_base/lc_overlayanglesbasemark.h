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

#ifndef LC_OVERLAYANGLESBASEMARK_H
#define LC_OVERLAYANGLESBASEMARK_H

#include "lc_overlayentity.h"
#include "rs_color.h"
#include "rs_vector.h"

struct LC_AnglesBaseMarkOptions{
    enum {
        SHOW_ALWAYS,
        SHOW_IF_NON_DEFAULTS
    };
    /** coordinate origin marker */
    bool showAnglesBaseMark = true;
    int displayPolicy = SHOW_ALWAYS;
    RS_Color colorDirectionType = Qt::blue;
    int markerRadius = 30;
    RS_Color colorAnglePointer = Qt::blue;
    RS_Color colorRadius = Qt::blue;
    void loadSettings();
};

class LC_OverlayAnglesBaseMark:public LC_OverlayDrawable{
public:
    LC_OverlayAnglesBaseMark(const RS_Vector& uiOrigin, double baseAngle, bool counterClockWise, LC_AnglesBaseMarkOptions *options);
    explicit LC_OverlayAnglesBaseMark(LC_AnglesBaseMarkOptions *options);
    void draw(RS_Painter *painter) override;
    void update(const RS_Vector &uiOrigin, double angle, bool counterclockwise);
protected:
    RS_Vector m_origin = RS_Vector(false);
    double m_baseAngle = 0.0;
    bool m_dirCounterClockWise = true;
    LC_AnglesBaseMarkOptions* m_options = nullptr;
    void createArrowShape(const RS_Vector &point, double angle, double arrowSize, RS_Vector &p1, RS_Vector &p2, RS_Vector &p3);
};

#endif
