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

#include "rs.h"
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
    virtual ~LC_UCS();
    LC_UCS* clone();
    void setOrigin(RS_Vector o);
    RS_Vector getOrigin() const {return m_ucsOrigin;}
    void setElevation(double d);
    double getElevation() const {return m_ucsElevation;}
    void setXAxis(RS_Vector pos);
    RS_Vector getXAxis(){return m_ucsXAxis;}
    void setYAxis(RS_Vector axis);
    RS_Vector getYAxis(){return m_ucsYAxis;}
    void setOrthoType(int type);
    int getOrthoType(){return m_ucsOrthoType;}
    void setName(const QString &name);
    const QString getName() const;
    long getNamedUcsId() const;
    long getBaseUcsId() const;
    bool isSameTo(LC_UCS* other);
    const RS_Vector getOrthoOrigin() const;
    void setOrthoOrigin(const RS_Vector &orthoOrigin);
    virtual bool isUCS() const {return true;}
    bool isTemporary() const {return m_temporary;}
    void setTemporary(bool temp){m_temporary = temp;}
    double getXAxisDirection(){
        return m_ucsXAxis.angle();
    }
    RS2::IsoGridViewType getIsoGridViewType();
    bool isIsometric();
    static bool isValidName(const QString &nameCandidate);

    enum UCSOrthoType{
        NON_ORTHO,
        TOP,
        BOTTOM,
        FRONT,
        BACK,
        LEFT,
        RIGHT
    };
private:
    bool m_temporary = false;
    QString m_name = "";
    RS_Vector m_ucsOrigin = RS_Vector(0,0,0); // UCS origin, 110, 120, 130
    RS_Vector m_ucsXAxis = RS_Vector(1,0,0); // UCS X-axis, 111, 121, 131
    RS_Vector m_ucsYAxis = RS_Vector(0,1,0); // UCS Y-axis, 112, 122, 132
    RS_Vector m_orthoOrigin = RS_Vector(0,0,0);
    int m_ucsOrthoType = 0; // Orthographic type of UCS, 0 = UCS is not orthographic, 1 = Top; 2 = Bottom, 3 = Front; 4 = Back, 5 = Left; 6 = Right, code 79
    double m_ucsElevation = 0.0; // UCS elevation, code 146
    long         namedUCS_ID;// ID/handle of AcDbUCSTableRecord if UCS is a named UCS. If not present, then UCS is unnamed, code 345
    long         baseUCS_ID;// ID/handle of AcDbUCSTableRecord of base UCS if UCS is orthographic, If not present and 79 code is non-zero, then base UCS is taken to be WORLD, code 346
};

class LC_WCS: public LC_UCS{
public:
    LC_WCS();
    bool isUCS() const override {return false;}

    static LC_UCS instance;
};

#endif // LC_UCS_H
