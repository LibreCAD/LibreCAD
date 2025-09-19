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

#include "lc_actionsplinemodifybase.h"

#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_pen.h"

LC_ActionSplineModifyBase::LC_ActionSplineModifyBase(const char* name, LC_ActionContext *actionContext, RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, actionContext, actionType) {
}

void LC_ActionSplineModifyBase::doInitWithContextEntity(RS_Entity* contextEntity,[[maybe_unused]] const RS_Vector& clickPos) {
    if (mayModifySplineEntity(contextEntity)) {
        setEntityToModify(contextEntity);
    }
}

void LC_ActionSplineModifyBase::doTrigger() {
    RS_Entity* createdEntity = createModifiedSplineEntity(m_entityToModify, m_vertexPoint, m_directionFromStart);
    if (createdEntity != nullptr){
        if (m_document) {
            createdEntity->setSelected(true);
            createdEntity->setLayer(m_entityToModify->getLayer());
            createdEntity->setPen(m_entityToModify->getPen(false));
            createdEntity->setParent(m_entityToModify->getParent());
            m_container->addEntity(createdEntity);
            doCompleteTrigger();
            undoCycleReplace(m_entityToModify, createdEntity);
        }
        m_entityToModify = createdEntity;
        m_vertexPoint = RS_Vector(false);
        doAfterTrigger();
    }
    else{
        doOnEntityNotCreated();
    }
}

void LC_ActionSplineModifyBase::doCompleteTrigger() {}
void LC_ActionSplineModifyBase::doAfterTrigger() {}


void LC_ActionSplineModifyBase::finish(bool updateTB) {
    clean();
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionSplineModifyBase::clean() {
    if (m_entityToModify){
        m_entityToModify->setSelected(false);
    }
    deletePreview();
    redraw();
}

void LC_ActionSplineModifyBase::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    onMouseMove(mouse, status, e);
}

RS2::CursorType LC_ActionSplineModifyBase::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionSplineModifyBase::drawSnapper() {
    // completely disable snapper for action
}

void LC_ActionSplineModifyBase::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
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
