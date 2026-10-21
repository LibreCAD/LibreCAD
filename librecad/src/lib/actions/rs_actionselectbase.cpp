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

#include "rs_actionselectbase.h"

#include <QKeyEvent>

#include "rs_debug.h"
#include "rs_document.h"
#include "rs_selection.h"

RS_ActionSelectBase::RS_ActionSelectBase(const QString& name, LC_ActionContext* actionContext, const RS2::ActionType actionType,
                                         QList<RS2::EntityType> entityTypeList)
    : LC_OverlayBoxAction(name, actionContext, actionType), m_catchForSelectionEntityTypes(std::move(entityTypeList)) {
}

void RS_ActionSelectBase::keyPressEvent(QKeyEvent* e) {
    const int key = e->key();
    switch (key) {
        case Qt::Key_Escape: {
            selectionFinishedByKey(e, true);
            break;
        }
        // The main keyboard sends Return; only the keypad sends Enter. Both
        // finish here, on the press, so the release cannot reach whichever
        // action resumes next and finish that one too.
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            if (m_document->hasSelection()
                || isAllowSelectionFinishByEnterForEmptySelection()) {
                selectionFinishedByKey(e, false);
            }
            else if (m_predecessor != nullptr) {
                // With nothing selected there is no selection to complete, so
                // hand control back to the action that asked for one - what
                // the key release used to do.
                finish();
            }
            break;
        }
        default:
            break;
    }
}

RS2::CursorType RS_ActionSelectBase::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

bool RS_ActionSelectBase::selectEntity(RS_Entity* entityToSelect, const bool selectContour) const {
    bool result = false;
    if (entityToSelect != nullptr) {
        const bool selectionAllowed = isEntityAllowedToSelect(entityToSelect);
        if (selectionAllowed) {
            doSelectEntity(entityToSelect, selectContour);
            result = true;
        }
    }
    else {
        RS_DEBUG->print("RS_ActionSelectSingle::trigger: Entity is NULL\n");
    }
    return result;
    //    entityToSelect = nullptr;
}

void RS_ActionSelectBase::doSelectEntity(RS_Entity* entityToSelect, [[maybe_unused]] bool selectContour) const {
    const RS_Selection s(m_document, m_viewport);
    s.selectSingle(entityToSelect);
}

RS_Entity* RS_ActionSelectBase::selectionMouseMove(const LC_MouseEvent* event) {
    RS_Entity* result = nullptr;
    const auto ent = catchAndDescribe(event, m_catchForSelectionEntityTypes, RS2::ResolveNone);
    if (ent != nullptr) {
        const bool selectionAllowed = isEntityAllowedToSelect(ent);
        if (selectionAllowed) {
            const bool showRefPoints = isShowRefPointsOnHighlight();
            highlightHoverWithRefPoints(ent, showRefPoints);
            result = ent;
        }
    }
    return result;
}

bool RS_ActionSelectBase::isShowRefPointsOnHighlight() {
    return m_highlightEntitiesRefPointsOnHover;
}

void RS_ActionSelectBase::deselectAll() const {
    const RS_Selection s(m_document, m_viewport);
    s.selectAll(false);
}
