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

#include "lc_actionpreselectionawarebase.h"
#include "rs_document.h"
#include "rs_debug.h"

LC_ActionPreSelectionAwareBase::LC_ActionPreSelectionAwareBase(
    const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
    const QList<RS2::EntityType> &entityTypeList, const bool countSelectionDeep)
    :RS_ActionSelectBase(name, container, graphicView, entityTypeList),
    countDeep(countSelectionDeep){}

LC_ActionPreSelectionAwareBase::~LC_ActionPreSelectionAwareBase() {
    selectedEntities.clear();
}

void LC_ActionPreSelectionAwareBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        selectedEntities.clear();
    }
    else{
        if (!selectionComplete) {
            unsigned int selectedCount = countSelectedEntities();
            if (selectedCount > 0) {
                selectionCompleted(false, true);
            }
        }
    }
}

unsigned int LC_ActionPreSelectionAwareBase::countSelectedEntities() {
    selectedEntities.clear();
    document->collectSelected(selectedEntities, countDeep, catchForSelectionEntityTypes);
    unsigned int selectedCount = selectedEntities.size();
//    LC_ERR << " Selected Count: " << selectedCount;
    return selectedCount;
}

void LC_ActionPreSelectionAwareBase::selectionFinishedByKey([[maybe_unused]]QKeyEvent *e, bool escape) {
    if (escape){
        selectedEntities.clear();
        finish(false);
    }
    else{
        if (!selectionComplete) {
            selectionCompleted(false,false);
        }
    }
}

void LC_ActionPreSelectionAwareBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    if (selectionComplete){
        mouseLeftButtonReleaseEventSelected(status, e);
    }
    else{
        entityToSelect = catchEntity(e, catchForSelectionEntityTypes);
        if (entityToSelect != nullptr){
            selectEntity();
            if (isControl(e)){
                selectionCompleted(true, false);
            }
        }
    }
}

void LC_ActionPreSelectionAwareBase::onMouseRightButtonRelease(int status, QMouseEvent *e) {
    if (selectionComplete) {
        mouseRightButtonReleaseEventSelected(status, e);
    }
    else{
        selectedEntities.clear();
        finish(false);
    }
}

void LC_ActionPreSelectionAwareBase::mouseMoveEvent(QMouseEvent *event) {
    if (selectionComplete){
        mouseMoveEventSelected(event);
    }
    else{
        selectionMouseMove(event);
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHints() {
    if (selectionComplete){
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        updateMouseButtonHintsForSelection();
    }
}

void LC_ActionPreSelectionAwareBase::selectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    if (selectionComplete) {
        trigger();
        if (singleEntity) {
            deselectAll();
            selectionComplete = false; // continue with selection, don't finish
        } else {
            setStatus(-1);
        }
        updateSelectionWidget();
    }
}

void LC_ActionPreSelectionAwareBase::setSelectionComplete(bool allowEmptySelection, bool fromInit) {
    unsigned int selectedCount;
    if (fromInit) {
       selectedCount = selectedEntities.size();
    }
    else{
        selectedCount = countSelectedEntities();
    }
    bool proceed = selectedCount > 0 || allowEmptySelection;
    if (proceed) {
        selectionComplete = true;
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        commandMessage(tr("No valid entities selected, select them first"));
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHintsForSelected([[maybe_unused]]int status) {
    updateMouseWidget();
}

void LC_ActionPreSelectionAwareBase::mouseLeftButtonReleaseEventSelected([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseRightButtonReleaseEventSelected([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseMoveEventSelected([[maybe_unused]]QMouseEvent *e) {}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursor(int status) {
    if (selectionComplete){
        return doGetMouseCursorSelected(status);
    }
    else {
        return RS_ActionSelectBase::doGetMouseCursor(status);
    }
}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
