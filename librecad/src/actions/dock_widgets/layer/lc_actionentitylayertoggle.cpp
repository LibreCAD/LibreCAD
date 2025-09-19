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

#include "lc_actionentitylayertoggle.h"

#include "rs_graphic.h"
#include "rs_layer.h"

LC_ActionLayerToggle::LC_ActionLayerToggle(LC_ActionContext* context, RS2::ActionType actionType):
    LC_ActionLayerBase{"Layer of Entity Toggle Action", context, actionType}{
}

void LC_ActionLayerToggle::doWithLayer(RS_Graphic* graphic, RS_Layer* layer) {
    switch (m_actionType) {
        case RS2::ActionLayerEntityActivate: {
            graphic->activateLayer(layer, true);
            break;
        }
        case RS2::ActionLayerEntityToggleLock: {
            graphic->toggleLayerLock(layer);
            break;
        }
        case RS2::ActionLayerEntityToggleView: {
            graphic->toggleLayer(layer);
            break;
        }
        case RS2::ActionLayerEntityToggleConstruction: {
            graphic->toggleLayerConstruction(layer);
            break;
        }
        case RS2::ActionLayerEntityTogglePrint: {
            graphic->toggleLayerPrint(layer);
            break;
        }
        case RS2::ActionLayerEntityHideOthers: {
            auto layersList = graphic->getLayerList();
            for (RS_Layer* l: *layersList) {
                if (l != layer) {
                    l->freeze(true);
                }
            }
            layersList->fireLayerToggled();
            break;
        }
        default:
            break;
    }
}

void LC_ActionLayerToggle::updateMouseButtonHintsForSelection() {
    switch (m_actionType) {
        case RS2::ActionLayerEntityActivate: {
            updateMouseWidgetTRCancel("Select entity to activate layer");
            break;
        }
        case RS2::ActionLayerEntityToggleLock: {
            updateMouseWidgetTRCancel("Select entity to toggle layer lock");
            break;
        }
        case RS2::ActionLayerEntityHideOthers: {
            updateMouseWidgetTRCancel("Select entity to hide other layers");
            break;
        }
        case RS2::ActionLayerEntityToggleView: {
            updateMouseWidgetTRCancel("Select entity to toggle layer visibility");
            break;
        }
        case RS2::ActionLayerEntityToggleConstruction: {
            updateMouseWidgetTRCancel("Select entity to toggle layer print");
            break;
        }
        case RS2::ActionLayerEntityTogglePrint: {
            updateMouseWidgetTRCancel("Select entity to toggle layer construction");
            break;
        }
        default:
            break;
    }

}
