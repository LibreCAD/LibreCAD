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

#include "lc_actiondrawarc2pointsangle.h"

#include "rs_arc.h"

LC_ActionDrawArc2PointsAngle::LC_ActionDrawArc2PointsAngle(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("DrawArc2Angle",actionContext, RS2::ActionDrawArc2PAngle) {
}

bool LC_ActionDrawArc2PointsAngle::createArcData(RS_ArcData &data, [[maybe_unused]]int status, RS_Vector pos, bool alternate, [[maybe_unused]]bool reportErrors) {

    double chordAngle = m_startPoint.angleTo(pos);
    double chordAngleNormal = chordAngle + M_PI_2;
    double chordAngleNormalAlt = chordAngle - M_PI_2;

    double chordLen = pos.distanceTo(m_startPoint);
    double chordLenHalf = chordLen * 0.5;

    double angle = m_parameterLen;

    double angleHalf = angle*0.5;

    double distanceFromChordCenterToCenter = chordLenHalf / tan(angleHalf);

    RS_Vector chordLenHalfPont = (m_startPoint + pos) * 0.5;

    bool reverseArc = m_reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    double angleToCenter = /*reverseArc*/ m_reversed ? chordAngleNormalAlt : chordAngleNormal;
    RS_Vector center = chordLenHalfPont.relative(distanceFromChordCenterToCenter, angleToCenter);

    double radius = center.distanceTo(m_startPoint);
    double angle1 = center.angleTo(m_startPoint);
    double angle2 = center.angleTo(pos);

    // double secondLegAngle = M_PI - angleHalf;
    // double radiusLeg = chordLenHalf / cos(secondLegAngle);
    // double radiusDelta = radius - radiusLeg;
    // LC_ERR << radiusDelta;
    
    data.angle1 = angle1;
    data.angle2 = angle2;
    data.reversed = reverseArc;
    data.radius = radius;
    data.center = center;
    return true;
}

void LC_ActionDrawArc2PointsAngle::doPreviewOnPoint2Custom(RS_Arc *arc) {
    auto refArcData = RS_ArcData(arc->getData());
    double radius = arc->getRadius() * 0.2;
    refArcData.radius = radius;
    previewRefArc(refArcData);

    RS_Vector center = arc->getCenter();

    previewRefLine(center, m_startPoint);
    previewRefLine(center, arc->getEndpoint());
}

void LC_ActionDrawArc2PointsAngle::setParameterValue(double r) {
    m_parameterLen = RS_Math::deg2rad(r);
}

QString LC_ActionDrawArc2PointsAngle::getParameterCommand() {
    return "angle";
}

QString LC_ActionDrawArc2PointsAngle::getParameterPromptValue() const {
    return tr("Enter value of central angle");
}

QString LC_ActionDrawArc2PointsAngle::getAlternativePoint2Prompt() const {
    return tr("Alternate angle to outer arc");
}
