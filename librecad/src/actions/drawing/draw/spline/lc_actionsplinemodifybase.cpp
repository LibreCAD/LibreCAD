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

#include "rs_document.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_graphicview.h"
#include "lc_actionsplinemodifybase.h"

LC_ActionSplineModifyBase::LC_ActionSplineModifyBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView)
:RS_PreviewActionInterface(name, container, graphicView) {
}

void LC_ActionSplineModifyBase::doTrigger() {
    RS_Entity* createdEntity = createModifiedSplineEntity(entityToModify, vertexPoint, directionFromStart);
    if (createdEntity != nullptr){
        if (document) {
            createdEntity->setSelected(true);
            createdEntity->setLayer(entityToModify->getLayer());
            createdEntity->setPen(entityToModify->getPen(false));
            createdEntity->setParent(entityToModify->getParent());
            container->addEntity(createdEntity);
            doCompleteTrigger();
            undoCycleReplace(entityToModify, createdEntity);
        }
        entityToModify = createdEntity;
        vertexPoint = RS_Vector(false);
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
    if (entityToModify){
        entityToModify->setSelected(false);
    }
    deletePreview();
    graphicView->redraw();
}

void LC_ActionSplineModifyBase::mouseMoveEvent(QMouseEvent *e) {
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    deleteHighlights();
    deletePreview();
    onMouseMove(mouse, status, e);
    drawHighlights();
    drawPreview();
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
