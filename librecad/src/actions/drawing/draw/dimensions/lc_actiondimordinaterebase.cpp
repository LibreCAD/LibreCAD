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

#include "lc_actiondimordinaterebase.h"

#include "lc_dimordinate.h"
#include "lc_graphicviewport.h"
#include "rs_entity.h"
#include "rs_selection.h"

class LC_DimOrdinate;

LC_ActionDimOrdinateRebase::LC_ActionDimOrdinateRebase(LC_ActionContext* actionContext)
    :LC_ActionPreSelectionAwareBase("DimOrdinateRebase", actionContext, RS2::ActionDimOrdRebase) {
}

void LC_ActionDimOrdinateRebase::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    LC_ActionPreSelectionAwareBase::doInitWithContextEntity(contextEntity, clickPos);
}

void LC_ActionDimOrdinateRebase::updateActionPromptForSelection() {
    updatePromptTRCancel("Select Ordinate dimension to rebase (Enter - to complete)", MOD_CTRL(tr("Select and rebase")));
}

bool LC_ActionDimOrdinateRebase::isAllowTriggerOnEmptySelection() {
    return false;
}

bool LC_ActionDimOrdinateRebase::doTriggerModifications(LC_DocumentModificationBatch& modificationData) {
    double horizontalDirection = 0;
    RS_Vector origin{false};
    m_viewport->fillCurrentUCSInfo(origin, horizontalDirection);

    for (const auto e : std::as_const(m_selectedEntities)) {
        const auto* dimOrdinate = dynamic_cast<LC_DimOrdinate*>(e);
        if (dimOrdinate != nullptr) {
            const auto clone = static_cast<LC_DimOrdinate*>(dimOrdinate->clone());
            clone->setHDir(horizontalDirection);
            clone->setDefinitionPoint(origin);
            clone->clearSelectionFlag();
            clone->update();

            modificationData.replace(e, clone);
        }
    }
    return true;
}

void LC_ActionDimOrdinateRebase::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        RS_Selection::selectEntitiesList(m_document, m_viewport, ctx.entitiesToAdd, true);
    }
}

void LC_ActionDimOrdinateRebase::doTriggerCompletion([[maybe_unused]]bool success) {
}
