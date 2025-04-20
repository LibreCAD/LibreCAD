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
#include "rs_entitycontainer.h"
#include "rs_selection.h"

RS_ActionSelectBase::RS_ActionSelectBase(const char* name,LC_ActionContext *actionContext, RS2::ActionType actionType, QList<RS2::EntityType> entityTypeList)
        :LC_OverlayBoxAction(name, actionContext, actionType),
        m_catchForSelectionEntityTypes(std::move(entityTypeList)){
}

/**
 * Default behaviour of this method is triggering the predecesing
 * action and finishing this one when the enter key is pressed.
 */
void RS_ActionSelectBase::keyReleaseEvent(QKeyEvent* e) {
    if (e->key()==Qt::Key_Return && m_predecessor) {
        finish(false);
    }
}

void RS_ActionSelectBase::keyPressEvent(QKeyEvent *e){
    int key = e->key();
    switch (key){
        case Qt::Key_Escape:{
            selectionFinishedByKey(e, true);
            break;
        }
        case Qt::Key_Enter:{
            if (m_container->countSelected() > 0){
                selectionFinishedByKey(e, false);
            }
            break;
        }
        default:
            break;
    }
}

RS2::CursorType RS_ActionSelectBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

bool RS_ActionSelectBase::selectEntity(RS_Entity* entityToSelect, bool selectContour) {
    bool result = false;
    if (entityToSelect != nullptr){
        bool selectionAllowed = isEntityAllowedToSelect(entityToSelect);
        if (selectionAllowed){
            doSelectEntity(entityToSelect, selectContour);
            updateSelectionWidget();
            result = true;
        }
    }
    else {
        RS_DEBUG->print("RS_ActionSelectSingle::trigger: Entity is NULL\n");
    }
    return result;
//    entityToSelect = nullptr;
}

void RS_ActionSelectBase::doSelectEntity(RS_Entity* entityToSelect,  [[maybe_unused]]bool selectContour) const {
    RS_Selection s(*m_container, m_viewport);
    s.selectSingle(entityToSelect);
}

RS_Entity* RS_ActionSelectBase::selectionMouseMove(LC_MouseEvent *event) {
    RS_Entity* result = nullptr;
    auto ent = catchAndDescribe(event, m_catchForSelectionEntityTypes, RS2::ResolveNone);
    if (ent != nullptr){
        bool selectionAllowed = isEntityAllowedToSelect(ent);
        if (selectionAllowed){
            bool showRefPoints = isShowRefPointsOnHighlight();
            highlightHoverWithRefPoints(ent, showRefPoints);
            result = ent;
        }
    }
    return result;
}

bool RS_ActionSelectBase::isShowRefPointsOnHighlight() {
    return m_highlightEntitiesRefPointsOnHover;
}

void RS_ActionSelectBase::deselectAll(){
    RS_Selection s(*m_container, m_viewport);
    s.selectAll(false);
}
