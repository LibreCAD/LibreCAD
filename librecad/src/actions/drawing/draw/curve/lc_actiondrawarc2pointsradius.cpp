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

#include "lc_actiondrawarc2pointsradius.h"

#include "rs_arc.h"

LC_ActionDrawArc2PointsRadius::LC_ActionDrawArc2PointsRadius(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_ActionDrawArc2PointsBase("DrawArc2PRadius",container, graphicView) {
    actionType = RS2::ActionDrawArc2PRadius;
}

bool LC_ActionDrawArc2PointsRadius::createArcData(RS_ArcData &data, [[maybe_unused]]int status, RS_Vector pos, bool alternate, [[maybe_unused]]bool reportErrors) {

    double chordLen = startPoint.distanceTo(pos);
    double chordHalf = chordLen * 0.5;
    
    double chordAngle = startPoint.angleTo(pos);

    RS_Vector chordLenHalfPont = (startPoint + pos) * 0.5;

    double radius = parameterLen;

    if (chordHalf >= radius) {
        chordLen = radius * 2;
        chordHalf = radius;
        pos = startPoint.relative(chordLen, chordAngle);
        chordLenHalfPont = (startPoint + pos) * 0.5;
    }

    double distanceFromChordCenterToCenter = 0.0;
    if ((radius - chordHalf) > RS_TOLERANCE) {
        double triangleLegSquared = radius * radius - chordHalf * chordHalf;
        if (triangleLegSquared > 0) {
            distanceFromChordCenterToCenter = sqrt(triangleLegSquared);
        }
    }

    
    double chordAngleNormal = chordAngle + M_PI_2;
    double chordAngleNormalAlt = chordAngle - M_PI_2;

    bool reverseArc = reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    double angleToCenter = /*reverseArc*/ reversed ? chordAngleNormalAlt : chordAngleNormal;
    RS_Vector center = chordLenHalfPont.relative(distanceFromChordCenterToCenter, angleToCenter);

    data.center = center;
    data.reversed = reverseArc;
    data.radius = radius;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(pos);
    return true;
}


void LC_ActionDrawArc2PointsRadius::doPreviewOnPoint2Custom(RS_Arc *arc) {
    RS_Vector arcMiddlePoint = arc->getMiddlePoint();
    const RS_Vector &center = arc->getCenter();
    const RS_Vector &startPoint = arc->getStartpoint();
    const RS_Vector &endPoint = arc->getEndpoint();

    previewRefPoint(arcMiddlePoint);
    previewRefLine(center, arcMiddlePoint);
    previewRefLine(startPoint, endPoint);
    previewRefLine(center, startPoint);
    previewRefLine(center, endPoint);
}

QString LC_ActionDrawArc2PointsRadius::getParameterCommand() {
    return "radius";
}

QString LC_ActionDrawArc2PointsRadius::getParameterPromptValue() const {
    return tr("Enter radius of arc");
}

QString LC_ActionDrawArc2PointsRadius::getAlternativePoint2Prompt() const {
    return tr("Create outer arc");
}
