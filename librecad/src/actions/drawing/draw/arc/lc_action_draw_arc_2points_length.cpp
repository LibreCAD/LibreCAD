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

#include "lc_action_draw_arc_2points_length.h"

#include "lc_creation_arc.h"
#include "rs_circle.h"
#include "rs_information.h"

LC_ActionDrawArc2PointsLength::LC_ActionDrawArc2PointsLength(LC_ActionContext *actionContext)
    :LC_ActionDrawArc2PointsBase("ActionDrawArc2PLength",actionContext, RS2::ActionDrawArc2PLength) {
}

bool LC_ActionDrawArc2PointsLength::createArcData(RS_ArcData &data, [[maybe_unused]]int status, const RS_Vector& pos, const bool alternate, [[maybe_unused]] const bool reportErrors) {
    const bool result = LC_CreationArc::createFrom2PArcLength(m_startPoint, pos, m_parameterLen, m_reversed, alternate, data);
    if (!result) {
        if (reportErrors) {
            commandMessage(tr("The distance between the two points must be less than the arc length"));
        }
    }
    return result;
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
