/****************************************************************************
**
 * Toggle whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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
**********************************************************************/

#include "rs_actionlayerstoggleprint.h"

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_selection.h"

class RS_LayerList;
/**
 * whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)
 *
 * @author Dongxu Li
 */
RS_ActionLayersTogglePrint::RS_ActionLayersTogglePrint(LC_ActionContext *actionContext, RS_Layer* layer)
        : RS_ActionInterface("Toggle Layer Printing", actionContext, RS2::ActionLayersTogglePrint)
        , m_layer(layer)
{}

void RS_ActionLayersTogglePrint::trigger() {
    RS_DEBUG->print("toggle layer printing");
    if (m_graphic != nullptr) {
        RS_LayerList* ll = m_graphic->getLayerList();
        bool noLayersToggled = true;
        // toggle selected layers
        for (const auto layer : *ll) {
            if (layer == nullptr || !layer->isVisibleInLayerList() || !layer->isSelectedInLayerList()) {
                continue;
            }
            m_graphic->toggleLayerPrint(layer);
            deselectEntities(layer);
            noLayersToggled = false;
        }
        // if there wasn't selected layers, toggle active layer
        if (noLayersToggled) {
            m_graphic->toggleLayerPrint(m_layer);
            deselectEntities(m_layer);
        }
    }
    redrawDrawing();
    finish();
}

void RS_ActionLayersTogglePrint::init(const int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionLayersTogglePrint::deselectEntities(RS_Layer* layer) const {
    if (layer == nullptr) {
        return;
    }
    RS_Selection::unselectLayer(m_document, m_viewport, layer);
}
