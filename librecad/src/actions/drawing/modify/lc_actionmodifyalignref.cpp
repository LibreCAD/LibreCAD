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

#include "lc_actionmodifyalignref.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_linemath.h"
#include "lc_modifyalignrefoptions.h"
#include "rs_preview.h"

LC_ActionModifyAlignRef::LC_ActionModifyAlignRef(LC_ActionContext *actionContext)
  : LC_ActionModifyBase("ModifyAlignRef", actionContext, RS2::ActionModifyAlignRef) {
}

void LC_ActionModifyAlignRef::doTrigger(bool keepSelected) {
    prepareAlignRefData(m_actionData.targetPoint2);
    RS_Modification m(*m_container, m_viewport);
    m.alignRef(m_actionData.data, m_selectedEntities, false, keepSelected);
    finish(false);
}

void LC_ActionModifyAlignRef::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
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

            RS_Modification m(*m_preview, m_viewport, false);
            m.alignRef(m_actionData.data, m_selectedEntities, true, true);

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

    double angleOriginal = m_actionData.referencePoint1.angleTo(m_actionData.referencePoint2);
    double angleNew = m_actionData.targetPoint1.angleTo(snap);

    double rotationAngle = angleNew - angleOriginal;

    m_actionData.data.rotationAngle = rotationAngle;

    double distanceOriginal = m_actionData.referencePoint1.distanceTo(m_actionData.referencePoint2);
    double distanceNew = m_actionData.targetPoint1.distanceTo(snap);

    double scaleFactor = distanceNew / distanceOriginal;
    if (LC_LineMath::isNotMeaningful(scaleFactor)){
        scaleFactor = 1.0;
    }

    m_actionData.data.scaleFactor = scaleFactor;
}

void LC_ActionModifyAlignRef::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
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

void LC_ActionModifyAlignRef::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *pEvent) {
    if (status == SetRefPoint1){
        if (m_selectionComplete){
            m_selectionComplete = false;
            updateMouseButtonHints();
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

void LC_ActionModifyAlignRef::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetRefPoint1:{
            m_actionData.referencePoint1 = pos;
            moveRelativeZero(pos);
            setStatus(SetTargetPoint1);
            break;
        }
        case SetTargetPoint1:{
            m_actionData.targetPoint1 = pos;
            setStatus(SetRefPoint2);
            break;
        }
        case SetRefPoint2:{
            if (LC_LineMath::isMeaningfulDistance(m_actionData.referencePoint1, pos)){
                m_actionData.referencePoint2 = pos;
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

bool LC_ActionModifyAlignRef::doProcessCommand(int status, const QString &command) {
    return RS_ActionInterface::doProcessCommand(status, command);
}

void LC_ActionModifyAlignRef::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to align (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Align immediately after selection")));
}

void LC_ActionModifyAlignRef::updateMouseButtonHintsForSelected(int status) {
    switch (status){
        case SetRefPoint1:{
            updateMouseWidgetTRCancel(tr("Select first reference point")/*, MOD_CTRL(tr("Align immediately after selection"))*/);
            break;
        }
        case SetTargetPoint1:{
            updateMouseWidgetTRCancel(tr("Select first target point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetRefPoint2:{
            updateMouseWidgetTRCancel(tr("Select second reference point"));
            break;
        }
        case SetTargetPoint2:{
            updateMouseWidgetTRCancel(tr("Select second target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            updateMouseWidget();
    }
}

RS2::CursorType LC_ActionModifyAlignRef::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyAlignRef::setScale(bool val) {
   m_actionData.data.scale = val;
}

bool LC_ActionModifyAlignRef::isScale() {
    return m_actionData.data.scale;
}

bool LC_ActionModifyAlignRef::isAllowTriggerOnEmptySelection() {
    return false;
}

LC_ActionOptionsWidget *LC_ActionModifyAlignRef::createOptionsWidget() {
    return new LC_ModifyAlignRefOptions();
}
