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

#include "lc_text_options_filler.h"

#include "lc_action_draw_text.h"

void LC_TextOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionDrawText*>(m_action);

    addString({"a_text", tr("Text"), tr("Text to insert")}, [action]() {
                   return action->getText();
               }, [action](const QString& val) {
                   action->setText(val);
               }, container);

    addRawAngleDegrees({"a_angle", tr("Angle"), tr("Text rotation angle over insertion point")}, [action]() {
                   return action->getUcsAngleDegrees();
               }, [action](double val) {
                   action->setUcsAngleDegrees(val);
               }, container);
}
