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

#ifndef LC_UNDOABLEDOCUMENTMODIFICATIONACTION_H
#define LC_UNDOABLEDOCUMENTMODIFICATIONACTION_H

#include "rs_previewactioninterface.h"

struct LC_DocumentModificationBatch;

class LC_UndoableDocumentModificationAction: public RS_PreviewActionInterface {
protected:
    LC_UndoableDocumentModificationAction(const QString& name,LC_ActionContext *actionContext, const RS2::ActionType actionType = RS2::ActionNone):
     RS_PreviewActionInterface(name, actionContext, actionType){}
    ~LC_UndoableDocumentModificationAction() override = default;
    void previewEntitiesToAdd(LC_DocumentModificationBatch& ctx) const;
    virtual bool doTriggerModifications(LC_DocumentModificationBatch& ctx)  = 0;
    virtual void doTriggerCompletion([[maybe_unused]]bool success) {}
    virtual void doTriggerSelections([[maybe_unused]]const LC_DocumentModificationBatch& ctx){}
    void doTrigger() override;
};

class LC_SingleEntityCreationAction: public LC_UndoableDocumentModificationAction {
protected:
    LC_SingleEntityCreationAction(const QString& name,LC_ActionContext *actionContext, const RS2::ActionType actionType = RS2::ActionNone):
    LC_UndoableDocumentModificationAction(name, actionContext, actionType){}
    ~LC_SingleEntityCreationAction() override = default;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    virtual RS_Entity* doTriggerCreateEntity() = 0;
};
#endif
