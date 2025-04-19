/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_actionlayerstoggleconstruction.h"

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"

/**
 * whether a layer is a construction layer or not
 * construction layers doesn't appear on printout,
 * and have straight lines of infinite length
 *
 * @author Armin Stebich
 */
LC_ActionLayersToggleConstruction::LC_ActionLayersToggleConstruction(
    LC_ActionContext *actionContext,
    RS_Layer* layer)
        : RS_ActionInterface("Toggle Construction Layer", actionContext, RS2::ActionType::ActionLayersToggleConstruction)
        , m_layer(layer){
}


void LC_ActionLayersToggleConstruction::trigger() {
    RS_DEBUG->print("toggle layer construction");
    if (m_graphic) {
        RS_LayerList* ll = m_graphic->getLayerList();
        unsigned cnt = 0;
        // toggle selected layers
        for (auto layer: *ll) {
            if (!layer) continue;
            if (!layer->isVisibleInLayerList()) continue;
            if (!layer->isSelectedInLayerList()) continue;
            m_graphic->toggleLayerConstruction(layer);
            deselectEntities(layer);
            cnt++;
        }
        // if there wasn't selected layers, toggle active layer
        if (!cnt) {
            m_graphic->toggleLayerConstruction(m_layer);
            deselectEntities(m_layer);
        }
        redrawDrawing();
    }
    finish(false);
}

void LC_ActionLayersToggleConstruction::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void LC_ActionLayersToggleConstruction::deselectEntities(RS_Layer* layer){
    if (!layer) return;
    for(auto e: *m_container){ // fixme - sand -  iteration over all entities in container
        if (e && e->isVisible() && e->getLayer() == layer) {
            e->setSelected(false);
        }
    }
}
