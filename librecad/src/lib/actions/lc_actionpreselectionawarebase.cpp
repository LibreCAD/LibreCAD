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

#include <QMouseEvent>

#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "rs_document.h"
#include "rs_selection.h"
#include "rs_settings.h"

LC_ActionPreSelectionAwareBase::LC_ActionPreSelectionAwareBase(
    const char *name, LC_ActionContext *actionContext, RS2::ActionType actionType,
    const QList<RS2::EntityType> &entityTypeList, const bool countSelectionDeep)
    :RS_ActionSelectBase(name, actionContext, actionType, entityTypeList),
    m_countDeep(countSelectionDeep){}

void LC_ActionPreSelectionAwareBase::doTrigger() {
    bool keepSelected = LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true);
    doTrigger(keepSelected);
    // updateMouseButtonHints(); // todo - is it really necessary??
}

LC_ActionPreSelectionAwareBase::~LC_ActionPreSelectionAwareBase() {
    m_selectedEntities.clear();
}

void LC_ActionPreSelectionAwareBase::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (isForceSelectContextEntity()) {
        contextEntity->setSelected(true);
        redrawDrawing();
    }
    m_selectedEntities.push_back(contextEntity);
    onSelectionCompleted(true, true);
}

void LC_ActionPreSelectionAwareBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        m_selectedEntities.clear();
    }
    else{
        if (!m_selectionComplete) {
            unsigned int selectedCount = countSelectedEntities();
            if (selectedCount > 0) {
                onSelectionCompleted(false, true);
            }
        }
    }
}

unsigned int LC_ActionPreSelectionAwareBase::countSelectedEntities() {
    m_selectedEntities.clear();
    m_document->collectSelected(m_selectedEntities, m_countDeep, m_catchForSelectionEntityTypes);
    unsigned int selectedCount = m_selectedEntities.size();
//    LC_ERR << " Selected Count: " << selectedCount;
    return selectedCount;
}

void LC_ActionPreSelectionAwareBase::selectionFinishedByKey([[maybe_unused]]QKeyEvent *e, bool escape) {
    if (escape){
        m_selectedEntities.clear();
        finish(false);
    }
    else{
        if (!m_selectionComplete) {
            onSelectionCompleted(false,false);
        }
    }
}

void LC_ActionPreSelectionAwareBase::mousePressEvent(QMouseEvent * e) {
    if (!m_selectionComplete){
        if (e->button() == Qt::LeftButton){
            m_selectionCorner1 = toGraph(e);
        }
    }
}

void LC_ActionPreSelectionAwareBase::proceedSelectedEntity(LC_MouseEvent* e) {
    if (e->isControl) {
        onSelectionCompleted(true, false);
    }
}

void LC_ActionPreSelectionAwareBase::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    if (m_selectionComplete){
        onMouseLeftButtonReleaseSelected(status, e);
    }
    else{
        if (m_inBoxSelectionMode){
            RS_Vector mouse = e->graphPoint;
            // Issue #2299: also delete the overlay box for selection
            deletePreviewAndHighlights();

            // restore selection box to ucs
            RS_Vector ucsP1 = toUCS(m_selectionCorner1);
            RS_Vector ucsP2 = toUCS(mouse);

            bool selectIntersecting = (ucsP1.x > ucsP2.x);

            RS_Selection s(*m_container, m_viewport);
            bool performSelection = !e->isShift;
            bool alterSelectIntersecting = e->isControl;
            if (alterSelectIntersecting) {
                selectIntersecting = !selectIntersecting;
            }

            // expand selection wcs to ensure that selection box in ucs is full within bounding rect in wcs
            RS_Vector wcsP1, wcsP2;
            m_viewport->worldBoundingBox(ucsP1, ucsP2, wcsP1, wcsP2);

            if (m_catchForSelectionEntityTypes.isEmpty()){
                s.selectWindow(RS2::EntityUnknown, wcsP1, wcsP2, performSelection, selectIntersecting);
            }
            else {
                s.selectWindow(m_catchForSelectionEntityTypes, wcsP1, wcsP2, performSelection, selectIntersecting);
            }
            updateSelectionWidget();
            deletePreviewAndHighlights();
        }
        else{
            RS_Entity* entityToSelect = catchEntityByEvent(e, m_catchForSelectionEntityTypes);
            bool selectContour = e->isShift;
            if (selectEntity(entityToSelect, selectContour)) {
                proceedSelectedEntity(e);
            }
        }
        m_inBoxSelectionMode = false;
        m_selectionCorner1.valid = false;
        invalidateSnapSpot();
    }
}

void LC_ActionPreSelectionAwareBase::onMouseRightButtonRelease(int status, LC_MouseEvent *e) {
    if (m_selectionComplete) {
        onMouseRightButtonReleaseSelected(status, e);
    }
    else{
        m_selectedEntities.clear();
        finish(false);
    }
}

void LC_ActionPreSelectionAwareBase::applyBoxSelectionModeIfNeeded(RS_Vector mouse) {
    if (m_selectionCorner1.valid && (m_viewport->toGuiDX(m_selectionCorner1.distanceTo(mouse)) > 10.0)){
        m_inBoxSelectionMode = true;
    }
}

void LC_ActionPreSelectionAwareBase::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    if (m_selectionComplete){
        onMouseMoveEventSelected(status, e);
    }
    else{
        RS_Vector mouse = e->graphPoint;
        applyBoxSelectionModeIfNeeded(mouse);
        if (m_inBoxSelectionMode){
            drawOverlayBox(m_selectionCorner1, mouse);
            if (m_infoCursorOverlayPrefs->enabled) {
                // restore selection box to ucs
                RS_Vector ucsP1 = toUCS(m_selectionCorner1);
                RS_Vector ucsP2 = toUCS(mouse);
                bool selectIntersecting = (ucsP1.x > ucsP2.x);
                bool deselect = e->isShift;
                bool alterSelectIntersecting = e->isControl;
                if (alterSelectIntersecting) {
                    selectIntersecting = !selectIntersecting;
                }
                QString msg = deselect ? tr("De-Selecting") : tr("Selecting");
                msg.append(tr(" entities "));
                msg.append(selectIntersecting? tr("that intersect with box") : tr("that are within box"));
                m_infoCursorOverlayData->setZone2(msg);
                forceUpdateInfoCursor(mouse);
            }
        }
        else {
            selectionMouseMove(e);
            finishMouseMoveOnSelection(e);
        }
    }
}

void LC_ActionPreSelectionAwareBase::drawSnapper() {
    if (m_selectionComplete) {
        RS_Snapper::drawSnapper();
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHints() {
    if (m_selectionComplete){
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        if (m_inBoxSelectionMode){
            updateMouseWidgetTRBack(tr("Choose second edge"), MOD_SHIFT_AND_CTRL(tr("Select/Deselect entities"), tr("Select Intersecting")));
        }
        else {
            updateMouseButtonHintsForSelection();
        }
    }
}

void LC_ActionPreSelectionAwareBase::onSelectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    if (m_selectionComplete) {
        trigger();
        if (singleEntity) {
            deselectAll();
            m_selectionComplete = false; // continue with selection, don't finish
        } else {
            setStatus(-1);
        }
        updateSelectionWidget();
    }
}

void LC_ActionPreSelectionAwareBase::setSelectionComplete(bool allowEmptySelection, bool fromInit) {
    unsigned int selectedCount;
    if (fromInit) {
       selectedCount = m_selectedEntities.size();
    }
    else{
        selectedCount = countSelectedEntities();
    }
    bool proceed = selectedCount > 0 || allowEmptySelection;
    if (proceed) {
        m_selectionComplete = true;
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        commandMessage(tr("No valid entities selected, select them first"));
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHintsForSelected([[maybe_unused]]int status) {
    updateMouseWidget();
}

void LC_ActionPreSelectionAwareBase::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::onMouseRightButtonReleaseSelected([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::onMouseMoveEventSelected([[maybe_unused]] int status, [[maybe_unused]]LC_MouseEvent *event) {}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursor(int status) {
    if (m_selectionComplete){
        return doGetMouseCursorSelected(status);
    }
    else {
        return RS_ActionSelectBase::doGetMouseCursor(status);
    }
}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionPreSelectionAwareBase::finishMouseMoveOnSelection([[maybe_unused]]LC_MouseEvent *event) {

}

void LC_ActionPreSelectionAwareBase::doSelectEntity(RS_Entity *entityToSelect, bool selectContour) const {
    if (entityToSelect != nullptr){
        RS_Selection s(*m_container, m_viewport);
        // try to minimize selection clicks - and select contour based on selected entity. May be optional, but what for?
        if (entityToSelect->isAtomic() && selectContour) {
            s.selectContour(entityToSelect);
        }
        else{
            s.selectSingle(entityToSelect);
        }
    }
}
