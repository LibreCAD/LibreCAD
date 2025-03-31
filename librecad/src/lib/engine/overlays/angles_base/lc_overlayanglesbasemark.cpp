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

#include "lc_overlayanglesbasemark.h"
#include "rs_settings.h"
#include "rs_painter.h"

void LC_AnglesBaseMarkOptions::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_showAnglesBaseMark =  LC_GET_BOOL("AnglesBasisMarkEnabled", true);
        m_displayPolicy = LC_GET_INT("AnglesBasisMarkPolicy", SHOW_ALWAYS);

        int zeroMarkerSize = LC_GET_INT("ZeroMarkerSize", 30);
        m_markerRadius = zeroMarkerSize / 2;
    }
    LC_GROUP_GUARD("Colors");
    {
        m_colorAnglePointer = QColor(LC_GET_STR("angles_basis_angleray", RS_Settings::anglesBasisAngleRay));
        m_colorDirectionType = QColor(LC_GET_STR("angles_basis_direction", RS_Settings::anglesBasisDirection));
        // m_colorRadius = QColor(LC_GET_STR("colorAnglesBaseRadius", RS_Settings::anglesBasisDirection));
        m_colorRadius = m_colorDirectionType;
    }
}

LC_OverlayAnglesBaseMark::LC_OverlayAnglesBaseMark(RS_Vector uiOrigin, double ucsAngle, bool counterClockWise, LC_AnglesBaseMarkOptions *opt)
   :origin{uiOrigin}
    , baseAngle{ucsAngle}
    , dirCounterClockWise{counterClockWise}
    , options{opt}
{}

LC_OverlayAnglesBaseMark::LC_OverlayAnglesBaseMark(LC_AnglesBaseMarkOptions *options):
    options{options}
{
}

void LC_OverlayAnglesBaseMark::update(const RS_Vector &uiOrigin, double angle, bool counterclock) {
  baseAngle = angle;
  dirCounterClockWise = counterclock;
  origin = uiOrigin;
}

void LC_OverlayAnglesBaseMark::draw(RS_Painter *painter) {
    RS_Pen penRadius(options->m_colorRadius, RS2::Width00, RS2::SolidLine);
    penRadius.setScreenWidth(0);
    painter->setPen(penRadius);

    int radius = options->m_markerRadius;
    painter->drawCircleUIDirect(origin, radius);

    RS_Pen penAngle(options->m_colorAnglePointer, RS2::Width00, RS2::SolidLine);
    penAngle.setScreenWidth(0);
    painter->setPen(penAngle);

    RS_Vector angleEnd = origin.relative(radius, -baseAngle);
    painter->drawLineUISimple(origin, angleEnd);

    RS_Pen penDirection(options->m_colorRadius, RS2::Width00, RS2::SolidLine);
    penDirection.setScreenWidth(2);
    painter->setPen(penDirection);

    RS_Vector p1, p2, p3;

    double arrowSize = radius * 0.8;
    double arrowAngle = 0;

    RS_Vector arrowPoint;

    if (dirCounterClockWise){
        arrowPoint = RS_Vector(origin.x, origin.y + radius);
    }
    else{
        arrowPoint = RS_Vector(origin.x, origin.y - radius);
    }

    createArrowShape(arrowPoint, arrowAngle, arrowSize, p1, p2, p3);

    painter->fillTriangleUI(p1, p2, p3);
}

void LC_OverlayAnglesBaseMark::createArrowShape(const RS_Vector& point,double angle,double arrowSize, RS_Vector& p1, RS_Vector &p2, RS_Vector &p3){
    double arrowSide {arrowSize / cos(0.165)};
    double cosv1 {cos( angle + 0.165) * arrowSide};
    double sinv1 {sin( angle + 0.165) * arrowSide};
    double cosv2 {cos( angle - 0.165) * arrowSide};
    double sinv2 {sin( angle - 0.165) * arrowSide};

    p1 = point;
    p2 = RS_Vector(point.x - cosv1, point.y - sinv1);
    p3 = RS_Vector(point.x - cosv2, point.y - sinv2);
}
