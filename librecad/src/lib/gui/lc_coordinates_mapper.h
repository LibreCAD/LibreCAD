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

#ifndef LIBRECAD_LC_COORDINATES_MAPPER_H
#define LIBRECAD_LC_COORDINATES_MAPPER_H

#include "rs_vector.h"

class QPointF;

class LC_CoordinatesMapper {
public:
    LC_CoordinatesMapper();
    void toUCSDelta(const RS_Vector& worldDelta, double& ucsDX, double &ucsDY) const;
    RS_Vector toUCSDelta(const RS_Vector& worldDelta) const;
    RS_Vector toWorldDelta(const RS_Vector& worldDelta) const;
    double toWorldAngle(double ucsAngle) const;
    double toWorldAngleDegrees(double ucsAngle) const;
    double toUCSAngle(double wcsAngle) const;
    double toUCSAngleDegrees(double wcsAngle) const;
    RS_Vector toUCS(const RS_Vector& wcsPos) const;
    void toUCS(const RS_Vector& wcsPos, double& ucsX, double &ucsY) const;
    RS_Vector toWorld(double ucsX, double ucsY) const;
    RS_Vector toWorld(const RS_Vector& ucsPos) const;
    bool hasUCS() const {return m_hasUcs;}
    void ucsBoundingBox(const RS_Vector& wcsMin, const RS_Vector&wcsMax, RS_Vector& ucsMin, RS_Vector& ucsMax) const;
    void worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector &ucsMax, RS_Vector& worlMin, RS_Vector& worldMax) const;
    RS_Vector restrictHorizontal(const RS_Vector &baseWCSPoint, const RS_Vector& wcsCoord) const;
    RS_Vector restrictVertical(const RS_Vector &baseWCSPoint, const RS_Vector& wcsCoord) const;

    double toUCSBasisAngle(double ucsAbsAngle, double baseAngle, bool counterclockwise);
    double toUCSAbsAngle(double ucsBasisAngle, double baseAngle, bool conterclockwise);

    void apply(LC_CoordinatesMapper* other);

    const RS_Vector &getUcsOrigin() const;
    void setUcsOrigin(const RS_Vector& origin);
    double getXAxisAngle() const{return xAxisAngle;}
    const RS_Vector& getUcsRotation() const
    {
        return m_ucsRotation;
    }

protected:
    void setXAxisAngle(double angle);
    double toUCSAngleDegree(double angle) const;
    void doWCS2UCS(double worldX, double worldY, double &ucsX, double &ucsY) const;
    RS_Vector doWCS2UCS(const RS_Vector &worldCoordinate) const;
    RS_Vector doWCSDelta2UCSDelta(const RS_Vector &worldDelta) const;
    void doWCSDelta2UCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const;
    RS_Vector doUCSDelta2WCSDelta(const RS_Vector &ucsDelta) const;
    void doUCSDelta2WCSDelta(const RS_Vector &ucsDelta, double &wcsDX, double &wcsDY) const;
    RS_Vector doUCS2WCS(const RS_Vector &ucsCoordinate) const;
    void doUCS2WCS(double ucsX, double ucsY, double &worldX, double &worldY) const;
    void update(const RS_Vector& origin, double angle);
    void useUCS(bool isUcs)
    {
        m_hasUcs = isUcs;
    }

private:
    /**
     * Flag that defines whether ucs should be applied.
     * Potentially, it is possible to have 2 implementations of mapper - one for UCS and for WCS and
     * rely on virtual functions.
     * That will lead to cleaner code, of course, however - since mapping used extensively in rendering,
     * it's critical to have fast processing. That's the reason of such approach, as checking the flag
     * will be faster than virtual method call.
     */
    bool m_hasUcs = false;
    RS_Vector m_ucsOrigin{0., 0., 0.};
    double xAxisAngle = 0.0;
    double xAxisAngleDegrees = 0.0;
    RS_Vector m_ucsRotation;
    double& sinXAngle = m_ucsRotation.y;
    double& cosXAngle = m_ucsRotation.x;
    RS_Vector m_AxisNegRotation;
    double& sinNegativeXAngle = m_AxisNegRotation.y;
    double& cosNegativeXAngle = m_AxisNegRotation.x;
};


#endif //LIBRECAD_LC_COORDINATES_MAPPER_H
