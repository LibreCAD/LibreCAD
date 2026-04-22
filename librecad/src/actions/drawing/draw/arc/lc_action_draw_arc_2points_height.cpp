/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_action_draw_arc_2points_height.h"

#include "lc_creation_arc.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_information.h"

LC_ActionDrawArc2PointsHeight::LC_ActionDrawArc2PointsHeight(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("ActionDrawArc2PHeight",actionContext, RS2::ActionDrawArc2PHeight) {
}

bool LC_ActionDrawArc2PointsHeight::createArcData(RS_ArcData &data, [[maybe_unused]]int status, const RS_Vector& pos, bool alternate, [[maybe_unused]]bool reportErrors) {
    return LC_CreationArc::createFrom2PHeight(m_startPoint, pos, m_parameterLen, m_reversed, alternate, data);
}

void LC_ActionDrawArc2PointsHeight::doPreviewOnPoint2Custom(RS_Arc *arc) {
    const RS_Vector &startPoint = arc->getStartpoint();
    const RS_Vector &endPoint = arc->getEndpoint();
    const RS_Vector arcChordMiddle = (startPoint + endPoint) * 0.5;
    previewRefPoint(arcChordMiddle);
    const RS_Vector arcMiddlePoint = arc->getMiddlePoint();
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
