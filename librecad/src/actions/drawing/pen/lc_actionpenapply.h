/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 sand1024

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
#ifndef LC_ACTIONPENAPPLY_H
#define LC_ACTIONPENAPPLY_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_pen.h"
#include "rs_previewactioninterface.h"

/**
 * Action that applies pen (either from pen toolbar or from selected entity) to
 * entity selected by the user.
 */
class LC_ActionPenApply:public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    // statuses of action
    enum {
        SelectEntity = InitialActionStatus,
        ApplyToEntity
    };
    LC_ActionPenApply(LC_ActionContext *actionContext, bool copy);
    void init(int status) override;
    void finish() override;
private:
    // entity that might be used as source for pen applying
    RS_Entity* m_srcEntity {nullptr};
    // controls whether pen should be copied from source entity or applied from pen toolbar
    bool m_copyMode;
    RS_Entity* m_entityToApply {nullptr};
    bool m_updateInserts {false};
    RS_Pen m_penToApply;
protected:
    bool mayInitWithContextEntity(int status) override;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void updateActionPrompt() override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
};

#endif
