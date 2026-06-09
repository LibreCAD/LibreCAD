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

#include "lc_image_options_filler.h"

#include "lc_action_draw_image.h"

void LC_ImageOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawImage*>(m_action);

    addRawAngleDegrees({"a_angle", tr("Angle"), tr("Angle of image rotation around insertion point")}, [action]() {
                           return action->getUcsAngleDegrees();
                       }, [action](double val) {
                           action->setUcsAngleDegrees(val);
                       }, container);

    addLinearDistance({"a_factor", tr("Scale Factor"), tr("Scale factor of image")}, [action]() {
                          return action->getFactor();
                      }, [action](double val) {
                          action->setFactor(val);
                      }, container);

    addIntSpinbox({"a_factor", tr("DPI"), tr("DPI of Image")}, [action]() {
                      const double dpi = action->scaleToDpi(action->getFactor());
                      return dpi;
                  }, [action](double val) {
                      const double factor = action->dpiToScale(val);
                      action->setFactor(factor);
                  }, container);
}
