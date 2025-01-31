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

class RS_Vector;
class QPointF;

class LC_CoordinatesMapper {
public:
//    RS_Vector toGui(RS_Vector v) const;
//    void toGui(const RS_Vector &v, double& x, double& y) const;
//    RS_Vector toGui(double x, double y) const;
//    RS_Vector toGuiFromUCS(const RS_Vector &ucs) const;
//    RS_Vector toGuiFromUCS(double x, double y) const;
//    RS_Vector toGuiD(RS_Vector v) const;
//    double toGuiX(double x) const;
//    double toGuiY(double y) const;
//    double toGuiDX(double d) const;
//    double toGuiDY(double d) const;
//    RS_Vector toUCSFromGui(const QPointF& pos) const;
//    RS_Vector toUCSFromGui(double x, double y) const;
//    RS_Vector toGraph(const RS_Vector &v) const;
//    RS_Vector toGraph(const QPointF &v) const;
//    RS_Vector toGraph(int x, int y) const;
//    double toGraphX(int x) const;
//    double toGraphY(int y) const;
//    double toGraphDX(int uiDX) const {return 0.0;};// FIXME - ucs - complete
//    double toGraphDY(int d) const;
//    RS_Vector toGraphD(int d, int y) const;
//    RS_Vector toUCS(const QPointF &position) const;

    LC_CoordinatesMapper();


    double toUCSAngle(double a) const;
    double toUCSAngleDegrees(double a) const;
    void toUCSDelta(const RS_Vector& worldDelta, double& ucsDX, double &ucsDY) const;
    RS_Vector toUCSDelta(const RS_Vector& worldDelta) const;
    double toWorldAngle(double angle) const;
    double toWorldAngleDegrees(double angle) const;
    RS_Vector toUCS(const RS_Vector& v) const;
    void toUCS(const RS_Vector& v, double& ucsX, double &ucsY) const;
    RS_Vector toWorld(double ucsX, double ucsY) const;
    RS_Vector toWorld(const RS_Vector& ucsPos) const;
    bool hasUCS() const {return m_hasUcs;};
    void ucsBoundingBox(const RS_Vector& wcsMin, const RS_Vector&wcsMax, RS_Vector& ucsMin, RS_Vector& ucsMax) const;
    void worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector &ucsMax, RS_Vector& worlMin, RS_Vector& worldMax) const;
    RS_Vector restrictHorizontal(const RS_Vector &baseWCSPoint, const RS_Vector& wcsCoord) const;
    RS_Vector restrictVertical(const RS_Vector &baseWCSPoint, const RS_Vector& wcsCoord) const;

    double toUCSBasisAngle(double ucsAbsAngle, double baseAngle, bool counterclockwise);
    double toUCSAbsAngle(double ucsBasisAngle, double baseAngle, bool conterclockwise);

    RS_Vector getUcsOrigin(){
        return ucs.getUcsOrigin();
    }

    double getXAxisAngle(){
        return ucs.getXAxisAngle();
    }

protected:
    /**
     * Flag that defines whether ucs should be applied.
     * Potentially, it is possible to have 2 implementations of mapper - one for UCS and for WCS and
     * rely on virtual functions.
     * That will lead to cleaner code, of course, however - since mapping used extensively in rendering,
     * it's critical to have fast processing. That's the reason of such approach, as checking the flag
     * will be faster than virtual method call.
     */
    bool m_hasUcs = false;

    // ucs support

    class UserCoordinateSystem{
    public:
        UserCoordinateSystem(){ ucsOrigin = RS_Vector(0, 0, 0); setXAxisAngle(0.0);}
        double toWorldAngle(double angle) const;
        double toWorldAngleDegrees(double angle) const;
        double toUCSAngle(double angle) const;
        double toUCSAngleDegree(double angle) const;
        void toUCS(double worldX, double worldY, double &ucsX, double &ucsY) const;
        void toUCS(const RS_Vector &worldCoordinate, RS_Vector& ucsCoordinate) const;
        void toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const;
        void toWorld(const RS_Vector &ucsCoordinate, RS_Vector& worldCoordinate) const;
        void toWorld(double ucsX, double ucsY, double &worldX, double &worldY) const;
        void update(const RS_Vector& origin, double angle);
        void rotate(double &x, double &y) const;
        void rotateBack(double &x, double &y) const;
        const RS_Vector &getUcsOrigin() const;
        double getXAxisAngle() const;
    protected:
        RS_Vector ucsOrigin = RS_Vector(0, 0, 0);
        double xAxisAngle = 0.0;
        double xAxisAngleDegrees = 0.0;
        double sinXAngle = 0.0;
        double cosXAngle = 0.0;
        double sinNegativeXAngle = 0.0;
        double cosNegativeXAngle = 0.0;
        void setXAxisAngle(double angle);
    };

    UserCoordinateSystem ucs = UserCoordinateSystem();
};


#endif //LIBRECAD_LC_COORDINATES_MAPPER_H
