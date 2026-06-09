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

#include "lc_ucs_create_options_filler.h"

#include "lc_actionucscreate.h"
#include "lc_property_double_interactivepick_view.h"

void LC_UCSCreateOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionUCSCreate*>(m_action);

    addBoolean({
                      "a_angleFree",
                      tr("Free Angle"),
                      tr("Specifies whether X-Axis angle is defined by entered value or by mouse position")
                  }, [action]()-> bool {
                      return !action->isFixedAngle();
                  }, [action](bool val)-> void {
                      action->setFixedAngle(!val);
                  }, container);

    addRawAngle({"a_angle", tr("Angle"), tr("Angle X-axis of new coordinate system")}, [action]() {
                          return action->getAngle();
                      }, [action](double val) {
                          action->setAngle(val);
                      }, container, [action](LC_PropertyViewDescriptor& d) {
                          d.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
                          return !action->isFixedAngle();
                      });

}
