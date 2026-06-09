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

#include "lc_undoabledocumentmodificationaction.h"

#include "lc_undosection.h"
#include "rs_graphic.h"
#include "rs_preview.h"

void LC_UndoableDocumentModificationAction::doTrigger() {
    const bool result = m_document->undoableModify(m_viewport, [this](LC_DocumentModificationBatch& ctx)->bool {
       const bool success = doTriggerModifications(ctx);
       ctx.success = success;
       return success;
    },
    [this](const LC_DocumentModificationBatch& ctx, [[maybe_unused]]RS_Document* doc)->void {
        doTriggerSelections(ctx);
    });
    doTriggerCompletion(result);
}

void LC_UndoableDocumentModificationAction::previewEntitiesToAdd(LC_DocumentModificationBatch &ctx) const {
    for (const auto e: std::as_const(ctx.entitiesToAdd)) {
        const RS2::EntityType rtti = e->rtti();
        if (rtti == RS2::EntityInsert || RS2::isDimensionalEntity(rtti)) {
            e->update();
        }
        else{
            e->calculateBorders();
        }
        m_preview->addEntity(e);
    }
}

bool LC_SingleEntityCreationAction::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Entity* e = doTriggerCreateEntity();
    if (e == nullptr) {
        return false;
    }
    ctx += e;
    return true;
}
