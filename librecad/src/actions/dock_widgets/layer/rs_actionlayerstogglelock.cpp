/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_actionlayerstogglelock.h"

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"

class RS_LayerList;

RS_ActionLayersToggleLock::RS_ActionLayersToggleLock(LC_ActionContext *actionContext,RS_Layer* layer)
    : RS_ActionInterface("Toggle Layer Visibility", actionContext, RS2::ActionLayersToggleLock)
    , m_layer(layer){
}

void RS_ActionLayersToggleLock::trigger() {
    RS_DEBUG->print("toggle layer");
    if (m_graphic) {
        RS_LayerList* ll = m_graphic->getLayerList();
        unsigned cnt = 0;
        // toggle selected layers
        for (auto layer: *ll) {
            if (!layer) continue;
            if (!layer->isVisibleInLayerList()) continue;
            if (!layer->isSelectedInLayerList()) continue;
            m_graphic->toggleLayerLock(layer);
            deselectEntitiesOnLockedLayer(layer);
            cnt++;
        }
        // if there wasn't selected layers, toggle active layer
        if (!cnt) {
            m_graphic->toggleLayerLock(m_layer);
            deselectEntitiesOnLockedLayer(m_layer);
        }
    }
    redrawDrawing();
    finish(false);
}

void RS_ActionLayersToggleLock::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionLayersToggleLock::deselectEntitiesOnLockedLayer(RS_Layer* layer)
{
    if (!layer) return;
    if (!layer->isLocked()) return;

    for(auto e: *m_container){ // fixme - sand -  interation over all entities in container
        if (e && e->isVisible() && e->getLayer() == layer) {
            e->setSelected(false);
        }
    }
}
