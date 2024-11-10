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

#ifndef LC_UCS_H
#define LC_UCS_H

#include <QString>
#include "rs_vector.h"

/**
 * So far this is temporary data holder for UCS (if one is defined, say, in AutoCAD). The main idea - try to dont lose data in dxf on open/save cycle.
 *
 * However, later support of User Coordinate System may be added (which is actually by promising feature, especially considering scaling/rotation) -
 * and in this case, this class will be transformed to full-fledge entity
 */
class LC_UCS {
 public:
    LC_UCS();
    LC_UCS(QString name);

    void setOrigin(RS_Vector o);
    RS_Vector getOrigin() {return ucsOrigin;};

    void setElevation(double d);
    double getElevataion(){return ucsElevation;};

    void setXAxis(RS_Vector pos);
    RS_Vector getXAxis(){return ucsXAxis;}

    void setYAxis(RS_Vector axis);
    RS_Vector getYAxis(){return ucsYAxis;}

    void setOrthoType(int type);
    int getOrthoType(){return ucsOrthoType;};

protected:
    QString name;
    RS_Vector ucsOrigin; // UCS origin, 110, 120, 130
    RS_Vector ucsXAxis; // UCS X-axis, 111, 121, 131
    RS_Vector ucsYAxis; // UCS Y-axis, 112, 122, 132
    int ucsOrthoType; // Orthographic type of UCS, 0 = UCS is not orthographic, 1 = Top; 2 = Bottom, 3 = Front; 4 = Back, 5 = Left; 6 = Right, code 79
    double ucsElevation; // UCS elevation, code 146
    long         namedUCS_ID;// ID/handle of AcDbUCSTableRecord if UCS is a named UCS. If not present, then UCS is unnamed, code 345
    long         baseUCS_ID;// ID/handle of AcDbUCSTableRecord of base UCS if UCS is orthographic, If not present and 79 code is non-zero, then base UCS is taken to be WORLD, code 346
};

#endif // LC_UCS_H
