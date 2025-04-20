/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_actionlayerscmd.h"

#include "rs_graphic.h"
#include "rs_layer.h"

LC_ActionLayersCmd::LC_ActionLayersCmd(LC_ActionContext* actionContext, RS2::ActionType actionType)
    :RS_ActionInterface("LayerCommand", actionContext, actionType){
}

LC_ActionLayersCmd::~LC_ActionLayersCmd() = default;

void LC_ActionLayersCmd::trigger() {
    switch (m_actionType) {
        case RS2::ActionLayersActivateCmd: {
            activateLayer();
            break;
        }
        case RS2::ActionLayersAddCmd: {
            createLayer();
            break;
        }
        default:
            break;
    }
    m_layerName = "";
    finish(true);
}

void LC_ActionLayersCmd::updateMouseButtonHints() {
    updateMouseWidgetTRCancel(tr("Enter layer name to %1").arg(m_actionType == RS2::ActionLayersActivateCmd? tr("activate") : tr("create")));
}

bool LC_ActionLayersCmd::doProcessCommand([[maybe_unused]]int status, const QString& command) {
    m_layerName = command;
    trigger();
    return true;
}

void LC_ActionLayersCmd::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e) {
    init(-1);
}

void LC_ActionLayersCmd::activateLayer() {
    if (m_graphic != nullptr) {
        m_graphic->activateLayer(m_layerName, true);
    }
}

void LC_ActionLayersCmd::createLayer() {
    if (m_graphic != nullptr) {
        auto layer = new RS_Layer(m_layerName);
        m_graphic->addLayer(layer);
        m_graphic->activateLayer(m_layerName, true);
    }
}
