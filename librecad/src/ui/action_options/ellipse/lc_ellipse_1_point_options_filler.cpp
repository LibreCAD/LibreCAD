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

#include "lc_ellipse_1_point_options_filler.h"

#include "lc_action_draw_ellipse_1point.h"

void LC_Ellipse1PointOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawEllipse1Point*>(m_action);

    addLinearDistance({"a_radiusMajor", tr("Major Radius"), tr("Length of major axis of ellipse")}, [action]() {
                          return action->getMajorRadius();
                      }, [action](double val) {
                          action->setMajorRadius(val);
                      }, container);

    addLinearDistance({"a_radiusMinor", tr("Minor Radius"), tr("Length of minor axis of ellipse")}, [action]() {
                          return action->getMinorRadius();
                      }, [action](double val) {
                          action->setMinorRadius(val);
                      }, container);

    addBoolean({"a_hasAngle", tr("Major Axis rotated"), tr("If checked, angle for major axis may be specified")}, [action]()-> bool {
                   return action->hasAngle();
               }, [action](bool val)-> void {
                   action->setHasAngle(val);
               }, container);

    addBoolean(
        {"a_freeAngle", tr("Free Angle"), tr("If checked, major radius angle is specified by mouse")},
        [action]() -> bool { return action->isAngleFree(); }, [action](bool val) { action->setAngleFree(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasAngle(); });

    addRawAngleDegrees(
        {"a_angle", tr("Angle"), tr("Value of angle for major radius")}, [action]() { return action->getUcsMajorAngleDegrees(); },
        [action](double val) { action->setUcsMajorAngleDegrees(val); }, container,
        [action](LC_PropertyViewDescriptor&) -> bool { return !action->hasAngle() || action->isAngleFree(); });

    if (action->rtti() == RS2::ActionDrawEllipseArc1Point) {
        addBoolean({"a_reversed", tr("Reversed"), tr("If selected, arc will be clockwise, otherwise - counterclockwise")}, [action]()-> bool {
                  return action->isReversed();
              }, [action](bool val)-> void {
                  action->setReversed(val);
              }, container);
    }
}
