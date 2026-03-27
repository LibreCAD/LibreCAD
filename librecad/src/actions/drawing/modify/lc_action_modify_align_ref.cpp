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

#include "lc_action_modify_align_ref.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_linemath.h"
#include "lc_align_ref_options_widget.h"
#include "lc_align_ref_options_filler.h"
#include "rs_document.h"

LC_ActionModifyAlignRef::LC_ActionModifyAlignRef(LC_ActionContext *actionContext)
  : LC_ActionModifyBase("ActionModifyAlignRef", actionContext, RS2::ActionModifyAlignRef) {
}

void LC_ActionModifyAlignRef::doSaveOptions() {
    save("Scale", isScale());
    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
}

void LC_ActionModifyAlignRef::doLoadOptions() {
    bool scale = loadBool("Scale", true);
    setScale(scale);
    bool curLayer = loadBool("UseCurrentLayer", true);
    setUseCurrentLayer(curLayer);
    bool curAttrs = loadBool("UseCurrentAttributes", true);
    setUseCurrentAttributes(curAttrs);
    bool keepOriginals = loadBool("KeepOriginals", false);
    setKeepOriginals(keepOriginals);
}

bool LC_ActionModifyAlignRef::isInVisualSnapStatus(int status) {
    return (status == SetRefPoint1) || (status == SetRefPoint2) || (status == SetTargetPoint1) || (status == SetTargetPoint2);
}

bool LC_ActionModifyAlignRef::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    prepareAlignRefData(m_actionData.targetPoint2);
    RS_Modification::alignRef(m_actionData.data, m_selectedEntities, false, ctx);
    ctx.setActiveLayerAndPen(false, false);
    return true;
}

void LC_ActionModifyAlignRef::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyAlignRef::doTriggerCompletion([[maybe_unused]]bool success) {
    finish();
}

void LC_ActionModifyAlignRef::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetRefPoint1:{
            snap = getRelZeroAwarePoint(e, snap);
            previewRefPoint(snap);
            break;
        }
        case SetTargetPoint1:{
            snap = getSnapAngleAwarePoint(e, m_actionData.referencePoint1, snap, true);
            previewRefPoint(m_actionData.referencePoint1);
            previewRefLine(snap, m_actionData.referencePoint1);
            previewRefSelectablePoint(snap);
            break;
        }
        case SetRefPoint2:{
            snap = getRelZeroAwarePoint(e, snap);
            previewRefPoint(m_actionData.referencePoint1);
            previewRefSelectablePoint(m_actionData.targetPoint1);
            previewRefLine(m_actionData.referencePoint1, m_actionData.targetPoint1);
            previewRefSelectablePoint(snap);
            break;
        }
        case SetTargetPoint2:{
            snap = getSnapAngleAwarePoint(e, m_actionData.targetPoint1, snap, true);
            previewRefPoint(m_actionData.referencePoint1);
            previewRefSelectablePoint(m_actionData.targetPoint1);
            previewRefPoint(m_actionData.referencePoint2);
            previewRefSelectablePoint(snap);
            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_actionData.referencePoint1, m_actionData.targetPoint1);
                previewRefLine(m_actionData.referencePoint2, snap);
                previewRefLine(m_actionData.targetPoint1, snap);
                previewRefLine(m_actionData.referencePoint1, m_actionData.referencePoint2);
            }

            prepareAlignRefData(snap);

            LC_DocumentModificationBatch ctx;
            RS_Modification::alignRef(m_actionData.data, m_selectedEntities, true, ctx);
            previewEntitiesToAdd(ctx);

            if (isInfoCursorForModificationEnabled()) {
                msg(tr("Align References"))
                .relative(tr("Offset:"),m_actionData.data.offset)
                .rawAngle(tr("Angle:"), m_actionData.data.rotationAngle)
                .linear(tr("Scale:"),m_actionData.data.scaleFactor)
                .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyAlignRef::prepareAlignRefData(const RS_Vector &snap) {
    m_actionData.data.offset = m_actionData.targetPoint1 - m_actionData.referencePoint1;
    m_actionData.data.rotationCenter = m_actionData.referencePoint1;

    const double angleOriginal = m_actionData.referencePoint1.angleTo(m_actionData.referencePoint2);
    const double angleNew = m_actionData.targetPoint1.angleTo(snap);

    const double rotationAngle = angleNew - angleOriginal;

    m_actionData.data.rotationAngle = rotationAngle;

    const double distanceOriginal = m_actionData.referencePoint1.distanceTo(m_actionData.referencePoint2);
    const double distanceNew = m_actionData.targetPoint1.distanceTo(snap);

    double scaleFactor = distanceNew / distanceOriginal;
    if (LC_LineMath::isNotMeaningful(scaleFactor)){
        scaleFactor = 1.0;
    }

    m_actionData.data.scaleFactor = scaleFactor;
}

void LC_ActionModifyAlignRef::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetRefPoint1:{
            snap = getRelZeroAwarePoint(e, snap);
            break;
        }
        case SetTargetPoint1:{
             break;
        }
        case SetRefPoint2:{
            snap = getRelZeroAwarePoint(e, snap);
            break;
        }
        case SetTargetPoint2:{
            snap = getSnapAngleAwarePoint(e, m_actionData.targetPoint1, snap, true);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionModifyAlignRef::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    if (status == SetRefPoint1){
        if (m_selectionComplete){
            m_selectionComplete = false;
            updateActionPrompt();
        }
        else{
            setStatus(-1);
        }
    }
    else {
        setStatus(status - 1);
    }
}

LC_ModifyOperationFlags *LC_ActionModifyAlignRef::getModifyOperationFlags() {
    return &m_actionData.data;
}

void LC_ActionModifyAlignRef::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetRefPoint1:{
            m_actionData.referencePoint1 = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            setStatus(SetTargetPoint1);
            break;
        }
        case SetTargetPoint1:{
            m_actionData.targetPoint1 = pos;
            addSnappedPointToVisualSnap(pos);
            setStatus(SetRefPoint2);
            break;
        }
        case SetRefPoint2:{
            if (LC_LineMath::isMeaningfulDistance(m_actionData.referencePoint1, pos)){
                m_actionData.referencePoint2 = pos;
                addSnappedPointToVisualSnap(pos);
                moveRelativeZero(m_actionData.targetPoint1);
                setStatus(SetTargetPoint2);
            }
            else {
                commandMessage("Second reference point is too close to the first one");
            }
            break;
        }
        case SetTargetPoint2:{
            if (LC_LineMath::isMeaningfulDistance(m_actionData.targetPoint1, pos)){
                m_actionData.targetPoint2 = pos;
                addSnappedPointToVisualSnap(pos);
                moveRelativeZero(pos);
                trigger();
            }
            else {
                commandMessage("Second target point is too close to the first one");
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyAlignRef::doProcessCommand(const int status, const QString &command) {
    return RS_ActionInterface::doProcessCommand(status, command);
}

void LC_ActionModifyAlignRef::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to align")+getSelectionCompletionHintMsg(),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Align immediately after selection")));
}

void LC_ActionModifyAlignRef::updateActionPromptForSelected(const int status) {
    switch (status){
        case SetRefPoint1:{
            updatePromptTRCancel(tr("Select first reference point")/*, MOD_CTRL(tr("Align immediately after selection"))*/);
            break;
        }
        case SetTargetPoint1:{
            updatePromptTRCancel(tr("Select first target point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetRefPoint2:{
            updatePromptTRCancel(tr("Select second reference point"));
            break;
        }
        case SetTargetPoint2:{
            updatePromptTRCancel(tr("Select second target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyAlignRef::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyAlignRef::setScale(const bool val) {
   m_actionData.data.scale = val;
}

bool LC_ActionModifyAlignRef::isScale() const {
    return m_actionData.data.scale;
}

bool LC_ActionModifyAlignRef::isAllowTriggerOnEmptySelection() {
    return false;
}

LC_ActionOptionsWidget *LC_ActionModifyAlignRef::createOptionsWidget() {
    return new LC_AlignRefOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyAlignRef::createOptionsFiller() {
    return new LC_AlignRefOptionsFiller();
}
