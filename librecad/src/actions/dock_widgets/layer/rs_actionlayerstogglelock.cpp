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
#include "rs_selection.h"

class RS_LayerList;

RS_ActionLayersToggleLock::RS_ActionLayersToggleLock(LC_ActionContext *actionContext,RS_Layer* layer)
    : RS_ActionInterface("Toggle Layer Visibility", actionContext, RS2::ActionLayersToggleLock)
    , m_layer(layer){
}

void RS_ActionLayersToggleLock::trigger() {
    RS_DEBUG->print("toggle layer");
    if (m_graphic != nullptr) {
        RS_LayerList* ll = m_graphic->getLayerList();
        bool noLayersToggled = true;
        // toggle selected layers
        for (const auto layer : *ll) {
            if (layer == nullptr || !layer->isVisibleInLayerList() || !layer->isSelectedInLayerList()) {
                continue;
            }
            m_graphic->toggleLayerLock(layer);
            deselectEntitiesOnLockedLayer(layer);
            noLayersToggled = false;
        }
        // if there wasn't selected layers, toggle active layer
        if (noLayersToggled) {
            m_graphic->toggleLayerLock(m_layer);
            deselectEntitiesOnLockedLayer(m_layer);
        }
    }
    redrawDrawing();
    finish();
}

void RS_ActionLayersToggleLock::init(const int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionLayersToggleLock::deselectEntitiesOnLockedLayer(RS_Layer* layer) const {
    if (layer == nullptr ||  !layer->isLocked()) {
        return;
    }
    RS_Selection::unselectLayer(m_document, m_graphicView->getViewPort(), layer);
}
