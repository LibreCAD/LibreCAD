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

#include "rs_actionmodifymoverotate.h"

#include "lc_actioninfomessagebuilder.h"
#include "qg_moverotateoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyMoveRotate::MoveRotateActionData {
    RS_MoveRotateData data;
    RS_Vector targetPoint{false};
};

RS_ActionModifyMoveRotate::RS_ActionModifyMoveRotate(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("Move and Rotate Entities", actionContext, RS2::ActionModifyMoveRotate)
    , m_actionData(std::make_unique<MoveRotateActionData>()){
}

RS_ActionModifyMoveRotate::~RS_ActionModifyMoveRotate() = default;

void RS_ActionModifyMoveRotate::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyMoveRotate::trigger()");
    RS_Modification m(*m_container, m_viewport);
	   m.moveRotate(m_actionData->data, m_selectedEntities, false, keepSelected);
    m_actionData->targetPoint = RS_Vector(false);
    finish(false);
}

void RS_ActionModifyMoveRotate::onMouseMoveEventSelected(int status, LC_MouseEvent *e){
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            RS_Vector &originalRefPoint = m_actionData->data.referencePoint;
            if (originalRefPoint.valid) {
                mouse = getSnapAngleAwarePoint(e, originalRefPoint, mouse, true);
                m_actionData->data.offset = mouse - originalRefPoint;
                RS_Modification m(*m_preview, m_viewport);
                m.moveRotate(m_actionData->data, m_selectedEntities, true, false);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(originalRefPoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(originalRefPoint, mouse);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()){
                    msg(tr("Moving with rotation"))
                        .vector(tr("Source:"), m_actionData->data.referencePoint)
                        .vector(tr("Target:"), mouse)
                        .string(tr("Offset:"))
                        .relative(m_actionData->data.offset)
                        .relativePolar(m_actionData->data.offset)
                        .toInfoCursorZone2(false);
                }
            }
            break;
        }
        case SetAngle:{
            RS_Vector &targetPoint = m_actionData->targetPoint;
            RS_Vector &originalRefPoint = m_actionData->data.referencePoint;
            if (targetPoint.valid) {
                mouse = getSnapAngleAwarePoint(e, targetPoint, mouse, true);
                double wcsAngle = targetPoint.angleTo(mouse);
                double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
                double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
                m_actionData->data.angle = wcsRotationAngle;
                RS_Modification m(*m_preview, m_viewport);
                m.moveRotate(m_actionData->data, m_selectedEntities, true);
                if (m_showRefEntitiesOnPreview) {
                    previewSnapAngleMark(targetPoint, mouse);

                    previewRefPoint(originalRefPoint);
                    previewRefPoint(targetPoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(targetPoint, mouse);
                    previewRefLine(originalRefPoint, targetPoint);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()){
                    msg(tr("Moving with rotation"))
                        .vector(tr("Source:"), originalRefPoint)
                        .vector(tr("Target:"), targetPoint)
                        .string(tr("Offset:"))
                        .relative(m_actionData->data.offset)
                        .relativePolar(m_actionData->data.offset)
                        .rawAngle(tr("Angle:"), rotationAngle)
                        .toInfoCursorZone2(false);
                }
                updateOptions();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::previewRefPointsForMultipleCopies() {
    if (m_actionData->data.multipleCopies){
        int numPoints = m_actionData->data.number;
        if (numPoints > 1){
            for (int i = 1; i <= numPoints; i++){
                RS_Vector offset = m_actionData->data.offset * i;
                previewRefPoint(m_actionData->data.referencePoint + offset);
            }
        }
    }
}

void RS_ActionModifyMoveRotate::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    switch (status){
        case SetReferencePoint:{
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetTargetPoint: {
            RS_Vector snapped = e->snapPoint;
            snapped = getSnapAngleAwarePoint(e, m_actionData->data.referencePoint, snapped);
            fireCoordinateEvent(snapped);
            break;
        }
        case SetAngle: {
            if (m_actionData->targetPoint.valid) {
                RS_Vector snapped = e->snapPoint;
                snapped = getSnapAngleAwarePoint(e, m_actionData->targetPoint, snapped);
                fireCoordinateEvent(snapped);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            if (m_selectionComplete) {
                m_selectionComplete = false;
            } else {
                initPrevious(status);
            }
            break;
        }
        case SetTargetPoint: {
            setStatus(SetReferencePoint);
            m_actionData->targetPoint = RS_Vector(false);
            break;
        }
        case SetAngle: {
            m_actionData->targetPoint = RS_Vector(false);
            setStatus(m_lastStatus);
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMoveRotate::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = pos;
            moveRelativeZero(pos);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = pos;
            setStatus(ShowDialog);
            m_actionData->data.offset = pos - m_actionData->data.referencePoint;
            if (m_angleIsFixed) {
                doPerformTrigger();
            }
            else{
                moveRelativeZero(pos);
                setStatus(SetAngle);
                m_lastStatus = SetTargetPoint;
            }
            break;
        }
        case SetAngle: {
            if (m_actionData->targetPoint.valid){
//                double angle = pPoints->targetPoint.angleTo(pos);
                double wcsAngle = m_actionData->targetPoint.angleTo(pos);
                double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
                double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
                m_actionData->data.angle = wcsRotationAngle;
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
        if (RS_DIALOGFACTORY->requestMoveRotateDialog(m_actionData->data)) {
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
                m_lastStatus = (Status) status;
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
                // relative angle is used, no need to translate
                m_actionData->data.angle = adjustRelativeAngleSignByBasis(RS_Math::deg2rad(a));
                if (m_angleIsFixed) {
                    updateOptions();
                    setStatus(m_lastStatus);
                }
                else if (m_actionData->targetPoint.valid){
                    updateOptions();
                    doPerformTrigger();
                } else {
                    m_angleIsFixed = true;
                    updateOptions();
                    setStatus(m_lastStatus);
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
    m_actionData->data.angle = adjustRelativeAngleSignByBasis(a);
}

double RS_ActionModifyMoveRotate::getAngle() const{
    return adjustRelativeAngleSignByBasis(m_actionData->data.angle);
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
    return &m_actionData->data;
}

void RS_ActionModifyMoveRotate::setAngleIsFree(bool b) {
    m_angleIsFixed = !b;
    if (m_angleIsFixed && getStatus() == SetAngle){
        setStatus(SetTargetPoint);
    }
}

void RS_ActionModifyMoveRotate::setUseSameAngleForCopies(bool b) {
    m_actionData->data.sameAngleForCopies = b;
}

bool RS_ActionModifyMoveRotate::isUseSameAngleForCopies() {
    return m_actionData->data.sameAngleForCopies;
}

LC_ActionOptionsWidget* RS_ActionModifyMoveRotate::createOptionsWidget() {
    return new QG_MoveRotateOptions();
}
