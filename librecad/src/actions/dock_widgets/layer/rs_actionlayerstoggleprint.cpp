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
    if (m_graphic) {
        RS_LayerList* ll = m_graphic->getLayerList();
        unsigned cnt = 0;
        // toggle selected layers
        for (auto layer: *ll) {
            if (!layer) continue;
            if (!layer->isVisibleInLayerList()) continue;
            if (!layer->isSelectedInLayerList()) continue;
            m_graphic->toggleLayerPrint(layer);
            deselectEntities(layer);
            cnt++;
        }
        // if there wasn't selected layers, toggle active layer
        if (!cnt) {
            m_graphic->toggleLayerPrint(m_layer);
            deselectEntities(m_layer);
        }
    }
    redrawDrawing();
    finish(false);
}

void RS_ActionLayersTogglePrint::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionLayersTogglePrint::deselectEntities(RS_Layer* layer)
{
    if (!layer) return;

    for(auto e: *m_container){ // // fixme - sand -  iteration over all entities in container
        if (e && e->isVisible() && e->getLayer() == layer) {
            e->setSelected(false);
        }
    }
}
