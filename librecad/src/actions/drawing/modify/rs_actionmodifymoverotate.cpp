/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <QMouseEvent>

#include "rs_actionmodifymoverotate.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "qg_moverotateoptions.h"

struct RS_ActionModifyMoveRotate::Points {
    RS_MoveRotateData data;
    RS_Vector targetPoint{false};
};

RS_ActionModifyMoveRotate::RS_ActionModifyMoveRotate(RS_EntityContainer& container,
                                                     RS_GraphicView& graphicView)
    :LC_ActionModifyBase("Move and Rotate Entities",container, graphicView)
    , pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionModifyMoveRotate;
}

RS_ActionModifyMoveRotate::~RS_ActionModifyMoveRotate() = default;

void RS_ActionModifyMoveRotate::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyMoveRotate::trigger()");
    RS_Modification m(*container, graphicView);
	   m.moveRotate(pPoints->data, selectedEntities, false, keepSelected);
    pPoints->targetPoint = RS_Vector(false);
    finish(false);
}

void RS_ActionModifyMoveRotate::mouseMoveEventSelected(QMouseEvent *e) {
    deletePreview();
    RS_Vector mouse = snapPoint(e);
    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent begin");
    switch (getStatus()) {
        case SetReferencePoint: {
            pPoints->data.referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            if (pPoints->data.referencePoint.valid) {
                mouse = getSnapAngleAwarePoint(e, pPoints->data.referencePoint, mouse, true);
                pPoints->data.offset = mouse - pPoints->data.referencePoint;
                RS_Modification m(*preview, graphicView);
                m.moveRotate(pPoints->data, selectedEntities, true, false);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->data.referencePoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(pPoints->data.referencePoint, mouse);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()){
                    LC_InfoMessageBuilder msg(tr("Moving with rotation"));
                    msg.add(tr("Source:"), formatVector(pPoints->data.referencePoint));
                    msg.add(tr("Target:"), formatVector(mouse));
                    msg.add(tr("Offset:"));
                    msg.add(formatRelative(pPoints->data.offset));
                    msg.add(formatRelativePolar(pPoints->data.offset));
                    appendInfoCursorZoneMessage(msg.toString(), 2, false);
                }
            }
            break;
        }
        case SetAngle:{
            if (pPoints->targetPoint.valid) {
                mouse = getSnapAngleAwarePoint(e, pPoints->targetPoint, mouse, true);
                double angle = pPoints->targetPoint.angleTo(mouse);
                pPoints->data.angle = angle;
                RS_Modification m(*preview, graphicView);
                m.moveRotate(pPoints->data, selectedEntities, true);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->data.referencePoint);
                    previewRefPoint(pPoints->targetPoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(pPoints->targetPoint, mouse);
                    previewRefLine(pPoints->data.referencePoint, pPoints->targetPoint);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()){
                    LC_InfoMessageBuilder msg(tr("Moving with rotation"));
                    msg.add(tr("Source:"), formatVector(pPoints->data.referencePoint));
                    msg.add(tr("Target:"), formatVector(mouse));
                    msg.add(tr("Offset:"));
                    msg.add(formatRelative(pPoints->data.offset));
                    msg.add(formatRelativePolar(pPoints->data.offset));
                    msg.add(tr("Angle:"), formatAngle(pPoints->data.angle));
                    appendInfoCursorZoneMessage(msg.toString(), 2, false);
                }
                updateOptions();
            }
            break;
        }
        default:
            break;
    }
    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent end");
    drawPreview();
}

void RS_ActionModifyMoveRotate::previewRefPointsForMultipleCopies() {
    if (pPoints->data.multipleCopies){
        int numPoints = pPoints->data.number;
        if (numPoints > 1){
            for (int i = 1; i <= numPoints; i++){
                RS_Vector offset = pPoints->data.offset * i;
                previewRefPoint(pPoints->data.referencePoint + offset);
            }
        }
    }
}

void RS_ActionModifyMoveRotate::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    switch (status){
        case SetReferencePoint:{
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetTargetPoint: {
            RS_Vector snapped = snapPoint(e);
            snapped = getSnapAngleAwarePoint(e, pPoints->data.referencePoint, snapped);
            fireCoordinateEvent(snapped);
            break;
        }
        case SetAngle: {
            if (pPoints->targetPoint.valid) {
                RS_Vector snapped = snapPoint(e);
                snapped = getSnapAngleAwarePoint(e, pPoints->targetPoint, snapped);
                fireCoordinateEvent(snapped);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            if (selectionComplete) {
                selectionComplete = false;
            } else {
                initPrevious(status);
            }
            break;
        }
        case SetTargetPoint: {
            setStatus(SetReferencePoint);
            pPoints->targetPoint = RS_Vector(false);
            break;
        }
        case SetAngle: {
            pPoints->targetPoint = RS_Vector(false);
            setStatus(lastStatus);
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint: {
            pPoints->data.referencePoint = pos;
            moveRelativeZero(pos);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            pPoints->targetPoint = pos;
            setStatus(ShowDialog);
            pPoints->data.offset = pos - pPoints->data.referencePoint;
            if (angleIsFixed) {
                doPerformTrigger();
            }
            else{
                moveRelativeZero(pos);
                setStatus(SetAngle);
                lastStatus = SetTargetPoint;
            }
            break;
        }
        case SetAngle: {
            if (pPoints->targetPoint.valid){
                double angle = pPoints->targetPoint.angleTo(pos);
                pPoints->data.angle = angle;
                doPerformTrigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::doPerformTrigger() {
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestMoveRotateDialog(pPoints->data)) {
            updateOptions();
            trigger();
        }
        else{
            setStatus(SetTargetPoint);
        }
    }
    else{
        trigger();
    }
}

bool RS_ActionModifyMoveRotate::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetReferencePoint:
        case SetTargetPoint: {
            // RVT_PORT changed from if (c==checkCommand("angle", c)) {
            if (checkCommand("angle", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetAngle);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->data.angle = RS_Math::deg2rad(a);
                if (angleIsFixed) {
                    updateOptions();
                    setStatus(lastStatus);
                }
                else if (pPoints->targetPoint.valid){
                    updateOptions();
                    doPerformTrigger();
                } else {
                    angleIsFixed = true;
                    updateOptions();
                    setStatus(lastStatus);
                }
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionModifyMoveRotate::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetReferencePoint:
        case SetTargetPoint: {
            cmd += command("angle");
            break;
        }
        default:
            break;
    }
    return cmd;
}

void RS_ActionModifyMoveRotate::setAngle(double a){
    pPoints->data.angle = a;
}

double RS_ActionModifyMoveRotate::getAngle() const{
    return pPoints->data.angle;
}

void RS_ActionModifyMoveRotate::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point"));
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Enter rotation angle:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionModifyMoveRotate::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void RS_ActionModifyMoveRotate::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to move and rotate  (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Move and rotate immediately after selection")));
}

LC_ModifyOperationFlags *RS_ActionModifyMoveRotate::getModifyOperationFlags() {
    return &pPoints->data;
}

void RS_ActionModifyMoveRotate::setAngleIsFree(bool b) {
    angleIsFixed = !b;
    if (angleIsFixed && getStatus() == SetAngle){
        setStatus(SetTargetPoint);
    }
}

void RS_ActionModifyMoveRotate::setUseSameAngleForCopies(bool b) {
    pPoints->data.sameAngleForCopies = b;
}

bool RS_ActionModifyMoveRotate::isUseSameAngleForCopies() {
    return pPoints->data.sameAngleForCopies;
}

LC_ActionOptionsWidget* RS_ActionModifyMoveRotate::createOptionsWidget() {
    return new QG_MoveRotateOptions();
}
