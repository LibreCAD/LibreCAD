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

#include "lc_actiondrawarc2pointsheight.h"

#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_information.h"

LC_ActionDrawArc2PointsHeight::LC_ActionDrawArc2PointsHeight(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("DrawArc2Angle",actionContext, RS2::ActionDrawArc2PHeight) {
}

bool LC_ActionDrawArc2PointsHeight::createArcData(RS_ArcData &data, [[maybe_unused]]int status, RS_Vector pos, bool alternate, [[maybe_unused]]bool reportErrors) {

    double chordLen = m_startPoint.distanceTo(pos);
    double arcHeight = m_parameterLen;

    double radius = (arcHeight / 2) + ((chordLen * chordLen) / (8 * arcHeight));


    RS_Circle circle1 = RS_Circle(nullptr, RS_CircleData(m_startPoint, radius));
    RS_Circle circle2 = RS_Circle(nullptr, RS_CircleData(pos, radius));

    const RS_VectorSolutions &intersections = RS_Information::getIntersection(&circle1, &circle2);

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
        bool heightLargeThanHalfChord = arcHeight > (chordLen / 2);

        if (m_reversed) {
            center = heightLargeThanHalfChord ? ipLeft : ipRight;
        } else {
            center = heightLargeThanHalfChord ? ipRight : ipLeft;
        }
    } else {
        RS_Vector v = RS_Vector();
        v.setPolar(radius, m_startPoint.angleTo(pos));
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

void LC_ActionDrawArc2PointsHeight::doPreviewOnPoint2Custom(RS_Arc *arc) {
    const RS_Vector &startPoint = arc->getStartpoint();
    const RS_Vector &endPoint = arc->getEndpoint();
    RS_Vector arcChordMiddle = (startPoint + endPoint) * 0.5;
    previewRefPoint(arcChordMiddle);
    RS_Vector arcMiddlePoint = arc->getMiddlePoint();
    previewRefPoint(arcMiddlePoint);
    previewRefLine(arcChordMiddle, arcMiddlePoint);
    previewRefLine(startPoint, endPoint);
}

QString LC_ActionDrawArc2PointsHeight::getParameterCommand() {
    return "height";
}

QString LC_ActionDrawArc2PointsHeight::getParameterPromptValue() const {
    return tr("Enter height of arc");
}

QString LC_ActionDrawArc2PointsHeight::getAlternativePoint2Prompt() const {
    return tr("Alternative arc where diameter is reduced by height");
}
