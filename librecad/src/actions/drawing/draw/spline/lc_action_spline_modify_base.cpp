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

#include "lc_action_spline_modify_base.h"

#include "rs_document.h"
#include "rs_entity.h"
#include "rs_pen.h"

LC_ActionSplineModifyBase::LC_ActionSplineModifyBase(const QString& name, LC_ActionContext *actionContext, const RS2::ActionType actionType)
    :LC_UndoableDocumentModificationAction(name, actionContext, actionType) {
}

void LC_ActionSplineModifyBase::doInitWithContextEntity(RS_Entity* contextEntity,[[maybe_unused]] const RS_Vector& clickPos) {
    if (mayModifySplineEntity(contextEntity)) {
        setEntityToModify(contextEntity);
    }
}

bool LC_ActionSplineModifyBase::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Entity* splineClone = createModifiedSplineEntity(m_entityToModify, m_vertexPoint, m_directionFromStart);
    if (splineClone != nullptr) {
        splineClone->setLayer(m_entityToModify->getLayer());
        splineClone->setPen(m_entityToModify->getPen(false));

        doTriggerOther();

        ctx.replace(m_entityToModify, splineClone);

        m_entityToModify = splineClone;
        m_vertexPoint    = RS_Vector(false);
        return true;
    }
    doOnEntityNotCreated();
    return false;
}

void LC_ActionSplineModifyBase::doTriggerSelections(const LC_DocumentModificationBatch& ctx) {
    if (ctx.success) {
        if (m_entityToModify != nullptr) {
            select(m_entityToModify);
        }
    }
}

void LC_ActionSplineModifyBase::doTriggerOther() {}

void LC_ActionSplineModifyBase::finish() {
    clean();
    RS_PreviewActionInterface::finish();
}

void LC_ActionSplineModifyBase::clean() {
    if (m_entityToModify != nullptr){
        unselect(m_entityToModify);
    }
    deletePreview();
    redraw();
}

void LC_ActionSplineModifyBase::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    onMouseMove(mouse, status, e);
}

RS2::CursorType LC_ActionSplineModifyBase::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionSplineModifyBase::drawSnapper() {
    // completely disable snapper for action
}

// fixme - review this override... why version with QMouseEvent is used instead of LC_MouseEvent???
void LC_ActionSplineModifyBase::onMouseRightButtonRelease(const int status, [[maybe_unused]] QMouseEvent *e) {
    deleteSnapper();
    deletePreview();
    drawPreview();
    setStatus(status-1);
    if (getStatus() == SetEntity){
        clean();
    }

}

void LC_ActionSplineModifyBase::doOnEntityNotCreated() {
}
