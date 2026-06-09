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

#include "lc_arc_2_points_options_widget_filler.h"

#include "lc_action_draw_arc_2points_base.h"

void LC_Arc2PointsOptionsWidgetFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawArc2PointsBase*>(m_action);

    const RS2::ActionType actionType = action->rtti();

    QString paramLabel;
    QString paramDescription;

    bool valueIsLength = true;

    switch (actionType) {
        case RS2::ActionDrawArc2PRadius: {
            paramDescription = tr("Radius of arc to create");
            paramLabel = tr("Radius");
            break;
        }
        case RS2::ActionDrawArc2PHeight: {
            paramDescription = tr("Height of arc to create");
            paramLabel = tr("Height");
            break;
        }
        case RS2::ActionDrawArc2PLength: {
            paramDescription = tr("Length (circumference) of arc to create");
            paramLabel = tr("Length");
            break;
        }
        case RS2::ActionDrawArc2PAngle: {
            paramDescription = tr("Central angle of arc to create");
            paramLabel = tr("Angle");
            valueIsLength = false;
            break;
        }
        default:
            break;
    }
    if (valueIsLength) {
        addLinearDistance({"a_valueParam", paramLabel, paramDescription}, [action]() {
                              return action->getParameter();
                          }, [action](double val) {
                              action->setParameter(val);
                          }, container);
    }
    else {
        addRawAngle({"a_valueParam", paramLabel, paramDescription}, [action]() {
                               return action->getParameter();
                           }, [action](double val) {
                               action->setParameter(val);
                           }, container);
    }

    addBoolean({"a_reveresed", tr("Reversed"), tr("If selected, arc will be clockwise, otherwise - counterclockwise")}, [action]()-> bool {
                   return action->isReversed();
               }, [action](bool val)-> void {
                   action->setReversed(val);
               }, container);
}
