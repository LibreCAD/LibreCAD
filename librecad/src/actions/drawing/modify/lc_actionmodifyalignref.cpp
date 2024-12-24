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
#include "lc_modifyalignrefoptions.h"
#include "rs_modification.h"
#include "lc_linemath.h"
#include "rs_preview.h"
#include "rs_graphicview.h"


LC_ActionModifyAlignRef::LC_ActionModifyAlignRef(RS_EntityContainer &container, RS_GraphicView &graphicView)
  : LC_ActionModifyBase("ModifyAlignRef", container, graphicView) {
    actionType = RS2::ActionModifyAlignRef;
}

void LC_ActionModifyAlignRef::doTrigger(bool keepSelected) {
    prepareAlignRefData(pPoints.targetPoint2);
    RS_Modification m(*container, graphicView);
    m.alignRef(pPoints.data, selectedEntities, false, keepSelected);
    finish(false);
}

void LC_ActionModifyAlignRef::mouseMoveEventSelected(QMouseEvent *e) {
    deletePreview();
    deleteHighlights();
    RS_Vector snap = snapPoint(e);
    switch (getStatus()){
        case SetRefPoint1:{
            snap = getRelZeroAwarePoint(e, snap);
            previewRefPoint(snap);
            break;
        }
        case SetTargetPoint1:{
            snap = getSnapAngleAwarePoint(e, pPoints.referencePoint1, snap, true);
            previewRefPoint(pPoints.referencePoint1);
            previewRefLine(snap, pPoints.referencePoint1);
            previewRefSelectablePoint(snap);
            break;
        }
        case SetRefPoint2:{
            snap = getRelZeroAwarePoint(e, snap);
            previewRefPoint(pPoints.referencePoint1);
            previewRefSelectablePoint(pPoints.targetPoint1);
            previewRefLine(pPoints.referencePoint1, pPoints.targetPoint1);
            break;
        }
        case SetTargetPoint2:{
            snap = getSnapAngleAwarePoint(e, pPoints.targetPoint1, snap, true);
            previewRefPoint(pPoints.referencePoint1);
            previewRefSelectablePoint(pPoints.targetPoint1);
            previewRefPoint(pPoints.referencePoint2);
            previewRefSelectablePoint(snap);
            if (showRefEntitiesOnPreview) {
                previewRefLine(pPoints.referencePoint1, pPoints.targetPoint1);
                previewRefLine(pPoints.referencePoint2, snap);
                previewRefLine(pPoints.targetPoint1, snap);
                previewRefLine(pPoints.referencePoint1, pPoints.referencePoint2);
            }

            prepareAlignRefData(snap);

            RS_Modification m(*preview, graphicView, false);
            m.alignRef(pPoints.data, selectedEntities, true, true);

            if (isInfoCursorForModificationEnabled()) {
                LC_InfoMessageBuilder msg(tr("Align References"));
                msg.add(tr("Offset:"),formatRelative(pPoints.data.offset));
                msg.add(tr("Angle:"),formatAngle(pPoints.data.rotationAngle));
                msg.add(tr("Scale:"),formatLinear(pPoints.data.scaleFactor));
                appendInfoCursorZoneMessage(msg.toString(), 2, false);
            }

            break;
        }
        default:
            break;
    }
    drawPreview();
    drawHighlights();
}

void LC_ActionModifyAlignRef::prepareAlignRefData(const RS_Vector &snap) {
    pPoints.data.offset = pPoints.targetPoint1 - pPoints.referencePoint1;
    pPoints.data.rotationCenter = pPoints.referencePoint1;

    double angleOriginal = pPoints.referencePoint1.angleTo(pPoints.referencePoint2);
    double angleNew = pPoints.targetPoint1.angleTo(snap);

    double rotationAngle = angleNew - angleOriginal;

    pPoints.data.rotationAngle = rotationAngle;

    double distanceOriginal = pPoints.referencePoint1.distanceTo(pPoints.referencePoint2);
    double distanceNew = pPoints.targetPoint1.distanceTo(snap);

    double scaleFactor = distanceNew / distanceOriginal;
    if (LC_LineMath::isNotMeaningful(scaleFactor)){
        scaleFactor = 1.0;
    }

    pPoints.data.scaleFactor = scaleFactor;
}

void LC_ActionModifyAlignRef::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
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
            snap = getSnapAngleAwarePoint(e, pPoints.targetPoint1, snap, true);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionModifyAlignRef::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *pEvent) {
    if (status == SetRefPoint1){
        if (selectionComplete){
            selectionComplete = false;
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
    return &pPoints.data;
}

void LC_ActionModifyAlignRef::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetRefPoint1:{
            pPoints.referencePoint1 = pos;
            moveRelativeZero(pos);
            setStatus(SetTargetPoint1);
            break;
        }
        case SetTargetPoint1:{
            pPoints.targetPoint1 = pos;
            setStatus(SetRefPoint2);
            break;
        }
        case SetRefPoint2:{
            if (LC_LineMath::isMeaningfulDistance(pPoints.referencePoint1, pos)){
                pPoints.referencePoint2 = pos;
                moveRelativeZero(pPoints.targetPoint1);
                setStatus(SetTargetPoint2);
            }
            else {
                commandMessage("Second reference point is too close to the first one");
            }
            break;
        }
        case SetTargetPoint2:{
            if (LC_LineMath::isMeaningfulDistance(pPoints.targetPoint1, pos)){
                pPoints.targetPoint2 = pos;
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
   pPoints.data.scale = val;
}

bool LC_ActionModifyAlignRef::isScale() {
    return pPoints.data.scale;
}

bool LC_ActionModifyAlignRef::isAllowTriggerOnEmptySelection() {
    return false;
}

LC_ActionOptionsWidget *LC_ActionModifyAlignRef::createOptionsWidget() {
    return new LC_ModifyAlignRefOptions();
}
