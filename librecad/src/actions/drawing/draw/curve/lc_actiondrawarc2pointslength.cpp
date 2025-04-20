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

#include "lc_actiondrawarc2pointslength.h"

#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_information.h"

LC_ActionDrawArc2PointsLength::LC_ActionDrawArc2PointsLength(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("DrawArc2Angle",actionContext, RS2::ActionDrawArc2PLength) {
}

bool LC_ActionDrawArc2PointsLength::createArcData(RS_ArcData &data, [[maybe_unused]]int status, RS_Vector pos, bool alternate, [[maybe_unused]]bool reportErrors) {

    double chordLen = m_startPoint.distanceTo(pos);
    double chordAngle = m_startPoint.angleTo(pos);
    double arcLen = m_parameterLen;

    if (chordLen >= arcLen) {
        if (reportErrors) {
            commandMessage(tr("The distance between the two points must be less than the arc length"));
        }
        return false;
    }

    double angle = determineArcAngleByLenAndChord(arcLen, chordLen);
    double radius = chordLen/(2.0 * (std::sin(angle/2.0)));

    RS_Circle circle1 = RS_Circle(nullptr, RS_CircleData(m_startPoint, radius));
    RS_Circle circle2 = RS_Circle(nullptr, RS_CircleData(pos, radius));

    const RS_VectorSolutions &intersections = RS_Information::getIntersection(&circle2, &circle1, false);

    RS_Vector center;

    bool reverseArc = m_reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    if (intersections.size() == 2) {
        RS_Vector ipRight, ipLeft;
        int pointPosition = LC_LineMath::getPointPosition(m_startPoint, pos, intersections[0]);
        if (pointPosition == LC_LineMath::PointToLinePosition::RIGHT) {
            ipRight = intersections[0];
            ipLeft = intersections[1];
        }
        else {
            ipLeft = intersections[0];
            ipRight = intersections[1];
        }
        bool angleLessPI = angle < M_PI;

        if (m_reversed) {
            center = angleLessPI ? ipRight : ipLeft;
        } else {
            center = angleLessPI ? ipLeft : ipRight;
        }
    } else {
        const auto v = RS_Vector::polar(radius, chordAngle);
        center = m_startPoint + v;
        pos = m_startPoint + v*2.0;
    }

    data.center = center;
    data.reversed = reverseArc;
    data.radius = radius;
    data.angle1 = data.center.angleTo(m_startPoint);
    data.angle2 = data.center.angleTo(pos);
    return true;
}

void LC_ActionDrawArc2PointsLength::doPreviewOnPoint2Custom([[maybe_unused]]RS_Arc *pArc) {

}

QString LC_ActionDrawArc2PointsLength::getParameterCommand() {
    return "length";
}

QString LC_ActionDrawArc2PointsLength::getParameterPromptValue() const {
    return tr("Enter length of arc");
}

QString LC_ActionDrawArc2PointsLength::getAlternativePoint2Prompt() const {
    return tr("Alternate solutions");
}


double LC_ActionDrawArc2PointsLength::determineArcAngleByLenAndChord(double arcLen, double chordLen) {
    double k = chordLen / arcLen;
    double x = std::sqrt(6 - (6 * k));

    for (int i = 0; i < 6; i++) {
        x = x - ((std::sin(x) - (k * x)) / (std::cos(x) - k));
    }
    return (2 * x);
};
