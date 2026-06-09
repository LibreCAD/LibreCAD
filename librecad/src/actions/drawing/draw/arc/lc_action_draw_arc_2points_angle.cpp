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

#include "lc_action_draw_arc_2points_angle.h"

#include "lc_creation_arc.h"
#include "rs_arc.h"

LC_ActionDrawArc2PointsAngle::LC_ActionDrawArc2PointsAngle(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("ActionDrawArc2PAngle",actionContext, RS2::ActionDrawArc2PAngle) {
}

bool LC_ActionDrawArc2PointsAngle::createArcData(RS_ArcData& data, [[maybe_unused]] int status, const RS_Vector& pos, const bool alternate, [[maybe_unused]] bool reportErrors) {
    return LC_CreationArc::createFrom2PAngle(m_startPoint, pos, m_parameterLen, m_reversed, alternate, data);
}

void LC_ActionDrawArc2PointsAngle::doPreviewOnPoint2Custom(RS_Arc *arc) {
    auto refArcData = RS_ArcData(arc->getData());
    const double radius = arc->getRadius() * 0.2;
    refArcData.radius = radius;
    previewRefArc(refArcData);

    const RS_Vector center = arc->getCenter();

    previewRefLine(center, m_startPoint);
    previewRefLine(center, arc->getEndpoint());
}

void LC_ActionDrawArc2PointsAngle::setParameterValue(const double r) {
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
