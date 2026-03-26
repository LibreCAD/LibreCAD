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

#include "lc_polyline_options_filler.h"

#include "lc_action_draw_polyline.h"

void LC_PolylineOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawPolyline*>(m_action);

    static LC_EnumDescriptor modeDescriptor = {
        "modeDescriptor",
        {
            {LC_ActionDrawPolyline::SegmentMode::Line, tr("Line")},
            {LC_ActionDrawPolyline::SegmentMode::Tangential, tr("Arc tangental")},
            {LC_ActionDrawPolyline::SegmentMode::TangentalArcFixedRadius, tr("Arc radius")},
            {LC_ActionDrawPolyline::SegmentMode::TangentalArcFixedAngle, tr("Arc angle")},
            {LC_ActionDrawPolyline::SegmentMode::ArcFixedAngle, tr("Arc")}
        }
    };
    // fixme - probably it will be more efficient for the drawing use 5 checkboxes instead of one combobox? that will save 1 click for switch
    addEnum({"a_segmentType", tr("Segment type"), tr("Defines type of polyline's segment to be created)")}, &modeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                auto mode = static_cast<LC_ActionDrawPolyline::SegmentMode>(v);
                action->setMode(mode);
            }, container);

    addLinearDistance({"a_radius", tr("Radius"), tr("Radius of arc")}, [action]() {
                          return action->getRadius();
                      }, [action](double val) {
                          action->setRadius(val);
                      }, container, [action]([[maybe_unused]]LC_PropertyViewDescriptor& d) {
                          return action->getMode() != LC_ActionDrawPolyline::SegmentMode::TangentalArcFixedRadius;
                      });

    addRawAngleDegrees({"a_angle", tr("Angle"), tr("Angle of arc")}, [action]() {
                           return action->getAngleDegrees();
                       }, [action](double val) {
                           action->setAngleDegrees(val);
                       }, container, [action]([[maybe_unused]]LC_PropertyViewDescriptor& d) {
                           int mode = action->getMode();
                           return !((mode == LC_ActionDrawPolyline::SegmentMode::TangentalArcFixedAngle) || (mode ==
                               LC_ActionDrawPolyline::SegmentMode::ArcFixedAngle));
                       });

    addBoolean({"a_reversed", tr("Reversed"), tr("If selected, arc will be clockwise, otherwise - counterclockwise")}, [action]()-> bool {
                   return action->isReversed();
               }, [action](bool val)-> void {
                   action->setReversed(val);
               }, container, [action](LC_PropertyViewDescriptor&) {
                   int mode = action->getMode();
                   return mode != LC_ActionDrawPolyline::SegmentMode::ArcFixedAngle;
               });

    createCommandsLine(container, "a_commands2", tr("Close"), tr("Connects endpoints of the polyline so closed contour is created"), tr("Undo"),
                       tr("Undo for previous vertex"), [action](int linkIndex)-> void {
                           if (linkIndex == 0) {
                               action->close();
                           }
                           else {
                               action->undo();
                           }
                       }, tr("Generic commands for polyline")/*, action->mayUndo(), action->mayRedo()*/);
}
