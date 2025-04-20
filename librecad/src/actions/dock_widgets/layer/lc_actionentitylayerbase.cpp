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

#include "lc_actionentitylayerbase.h"

#include "lc_actioncontext.h"
#include "rs_entity.h"
#include "rs_graphicview.h"

LC_ActionLayerBase::LC_ActionLayerBase(const char* name, LC_ActionContext* actionContext, RS2::ActionType actionType)
       : LC_ActionPreSelectionAwareBase{name, actionContext, actionType} {
}

void LC_ActionLayerBase::onSelectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    if (m_selectionComplete) {
        trigger();
    }
}

void LC_ActionLayerBase::doTrigger([[maybe_unused]]bool keepSelected) {
    if (!m_selectedEntities.empty()) {
        auto firstEntity = m_selectedEntities.front();
        RS_Layer* layer = firstEntity->getLayer();

        if (layer != nullptr) {
            doProceedLayer(layer);
            deselectAll();
            init(-1);
        }
    }
    else {
        // hmm.. what to do? notify the user?
    }
}

void LC_ActionLayerBase::applyBoxSelectionModeIfNeeded([[maybe_unused]]RS_Vector mouse) {
    m_inBoxSelectionMode = false;
}

void LC_ActionLayerBase::proceedSelectedEntity([[maybe_unused]]LC_MouseEvent* e) {
    onSelectionCompleted(true, false);
}

void LC_ActionLayerBase::doProceedLayer(RS_Layer* layer) {
    auto graphicView = m_actionContext->getGraphicView();
    auto graphic = graphicView->getGraphic();
    if (graphic != nullptr) {
        doWithLayer(graphic, layer);
    }
}
