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

#include "rs_actionmodifyrotate.h"

#include <QKeyEvent>

#include "lc_actioninfomessagebuilder.h"
#include "lc_modifyrotateoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_modification.h"
#include "rs_preview.h"

// fixme - sand - add options for point selection mode (ref point/center)? Or just remove center selection first and have one way?
// fixme - sand - GENERAL: NAVIGATE TO OPTIONS WIDGET BY KEYBOARD (FOCUS from action, TABS) - so it should be possible not to move mouse to change some option

RS_ActionModifyRotate::RS_ActionModifyRotate(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("Rotate Entities",actionContext, m_actionType=RS2::ActionModifyRotate)
    ,m_rotateData(new RS_RotateData()){
}

RS_ActionModifyRotate::~RS_ActionModifyRotate() = default;

void RS_ActionModifyRotate::init(int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

void RS_ActionModifyRotate::onSelectionCompleted(bool singleEntity, bool fromInit) {
    LC_ActionModifyBase::onSelectionCompleted(singleEntity, fromInit);
    if (m_selectRefPointFirst){
        setStatus(SetReferencePoint);
    }
    else{
        setStatus(SetCenterPoint);
    }
}

void RS_ActionModifyRotate::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyRotate::trigger()");
    moveRelativeZero(m_rotateData->center);
    RS_Modification m(*m_container, m_viewport);
    m.rotate(*m_rotateData, m_selectedEntities, false, keepSelected);
}

void RS_ActionModifyRotate::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            if (m_selectRefPointFirst){
                if (e->isControl) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getRelZeroAwarePoint(e, mouse);
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(mouse);
                }

            } else {
                if (e->isControl) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getSnapAngleAwarePoint(e, m_rotateData->center, mouse, true);
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(m_rotateData->center, mouse);
                    previewRefPoint(m_rotateData->center);
                    previewRefSelectablePoint(mouse);
                }
                if (!m_freeAngle){
                    RS_RotateData tmpData = *m_rotateData;
                    tmpData.refPoint = mouse;
                    RS_Modification m(*m_preview, m_viewport, false);
                    m.rotate(tmpData, m_selectedEntities, true, false);
                    previewRotationCircleAndPoints(m_rotateData->center, mouse, m_rotateData->angle);
                }
            }
            break;
        }
        case SetCenterPoint: {
            if (m_selectRefPointFirst){
                if (e->isControl) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getSnapAngleAwarePoint(e, m_rotateData->refPoint, mouse, true);
                }
                RS_Vector originalReferencePoint = m_rotateData->refPoint;
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(originalReferencePoint, mouse);
                    previewRefPoint(originalReferencePoint);
                }
                if (!m_freeAngle) {
                    RS_RotateData tmpData = *m_rotateData;
                    tmpData.center = mouse;
                    RS_Modification m(*m_preview, m_viewport, false);
                    m.rotate(tmpData, m_selectedEntities, true, false);
                    previewRotationCircleAndPoints(mouse, m_rotateData->refPoint, m_rotateData->angle);

                    if (isInfoCursorForModificationEnabled()) {
                        RS_Vector offset = mouse - originalReferencePoint;
                        msg(tr("Rotation"))
                            .rawAngle(tr("Angle:"), adjustRelativeAngleSignByBasis(m_rotateData->angle))
                            .vector(tr("Reference Point:"), m_rotateData->refPoint)
                            .vector(tr("Center Point:"), mouse)
                            .string(tr("Offset:"))
                            .relative(offset)
                            .relativePolar(offset)
                            .toInfoCursorZone2(false);
                    }
                }
                else if (isInfoCursorForModificationEnabled()) {
                    RS_Vector offset = mouse - originalReferencePoint;

                    msg(tr("Rotation"))
                           .vector(tr("Reference Point:"), m_rotateData->refPoint)
                           .vector(tr("Center Point:"), mouse)
                           .string(tr("Offset:"))
                           .relative(offset)
                           .relativePolar(offset)
                           .toInfoCursorZone2(false);
                }
            } else {
                if (!trySnapToRelZeroCoordinateEvent(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    RS_Vector center = boundingForSelected.getCenter();
                    if (e->isControl) {
                        mouse = center;
                    }
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(center, mouse);
                    }
                }
            }
            break;
        }
        case SetTargetPoint: {
            RS_Vector &center = m_rotateData->center;
            RS_Vector &originalReferencePoint = m_rotateData->refPoint;
            mouse = getSnapAngleAwarePoint(e, center, mouse, true);
            double radius = center.distanceTo(originalReferencePoint);
            double wcsAngle = center.angleTo(mouse);
            double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
            RS_Vector newAxisPoint = center.relative(radius, wcsAngle);
            if (m_showRefEntitiesOnPreview) {
                RS_Vector xAxisPoint = center.relative(radius, toWorldAngleFromUCSBasis(0));
                previewSnapAngleMark(center, mouse);
                previewRefSelectablePoint(newAxisPoint);
                previewRefPoint(xAxisPoint);
                previewRefLine(center, xAxisPoint);
                previewRefPoint(originalReferencePoint);
                previewRefPoint(center);
                previewRefLine(mouse, center);
            }

            m_currentAngle = rotationAngle;
            updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::UPDATE_ANGLE);

            RS_RotateData tmpData = *m_rotateData;
            double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
            tmpData.angle = wcsRotationAngle;

            RS_Modification m(*m_preview, m_viewport, false);
            m.rotate(tmpData, m_selectedEntities, true, false);

            // todo - sand - we can temporarily add a copy of circle to the document, so intersection snap for target reference point will work.
            previewRotationCircleAndPoints(center, m_rotateData->refPoint, wcsRotationAngle);

            if (isInfoCursorForModificationEnabled()) {
                RS_Vector offset = newAxisPoint - originalReferencePoint;
                msg(tr("Rotation"))
                    .rawAngle(tr("Angle:"), rotationAngle)
                    .vector(tr("Source Point:"), originalReferencePoint)
                    .vector(tr("Target Point:"), newAxisPoint)
                    .string(tr("Offset:"))
                    .relative(offset)
                    .relativePolar(offset)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        case SetTargetPoint2ndRotation:{
            RS_Vector originalRefPoint = m_rotateData->refPoint;
            RS_Vector center = m_rotateData->center;
            RS_Vector newRefPoint =  originalRefPoint;
            newRefPoint.rotate(center, m_rotateData->angle);
            mouse = getSnapAngleAwarePoint(e, originalRefPoint, mouse, true);

            if (m_showRefEntitiesOnPreview) {
                previewSnapAngleMark(newRefPoint, mouse);
                previewRefPoint(newRefPoint);
                previewRefLine(newRefPoint, mouse);
                previewRefPoint(center);
            }
            previewRotationCircleAndPoints(center, originalRefPoint, m_rotateData->angle);

            double wcsAngle = newRefPoint.angleTo(mouse);
            double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));

            m_currentAngle2 = rotationAngle;
            updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::UPDATE_ANGLE2);

            RS_RotateData tmpData = *m_rotateData;
            tmpData.secondAngle = adjustRelativeAngleSignByBasis(rotationAngle);

            RS_Modification m(*m_preview, m_viewport, false);
            m.rotate(tmpData, m_selectedEntities, true, false);

            if (isInfoCursorForModificationEnabled()) {
                RS_Vector offset = newRefPoint - originalRefPoint;
                msg(tr("Rotation"))
                    .rawAngle(tr("Angle:"), m_rotateData->angle)
                    .vector("Source Point:", originalRefPoint)
                    .vector("Target Point:", newRefPoint)
                    .string(tr("Offset:"))
                    .relative(offset)
                    .relativePolar(offset)
                    .rawAngle(tr("Second Angle:"), m_currentAngle2)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

bool RS_ActionModifyRotate::doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) {
    if (tag == "angle") {
        setAngle(angleRad);
        return true;
    }
    if (tag == "angle2") {
        setRefPointAngle(angleRad);
        return true;
    }
    return false;
}

void RS_ActionModifyRotate::previewRotationCircleAndPoints(const RS_Vector &center, const RS_Vector &refPoint, double angle) {
    if (m_showRefEntitiesOnPreview) {
        double radius = center.distanceTo(refPoint);
        previewRefCircle(center, radius);
        int numberOfCopies = m_rotateData->obtainNumberOfCopies();
        for (int i = 1; i <= numberOfCopies; i++) {//
            RS_Vector rotatedRefPoint = refPoint;
            double rotationAngle = angle * i;
            rotatedRefPoint.rotate(center, rotationAngle);
            previewRefPoint(rotatedRefPoint);
        }
    }
}
// fixme - sand -  support for focus of options widgets by keyboard...this should be removed as it may potentially may conflict with some shortcut. We need separate action for this..
void RS_ActionModifyRotate::keyPressEvent(QKeyEvent *e) {
    int key = e->key();
    switch (key) {
        case Qt::Key_Space: {
            if (isControl(e)){
                m_optionWidget->setFocus();
            }
            break;
        }
        default:
            RS_ActionSelectBase::keyPressEvent(e);
    }
}

void RS_ActionModifyRotate::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    if (!pos.valid){
        return;
    }
    switch (status) {
        case SetReferencePoint: {
            if (m_selectRefPointFirst){
                m_rotateData->refPoint = pos;
                moveRelativeZero(pos);
                setStatus(SetCenterPoint);
            }
            else{
                m_rotateData->refPoint = pos;
                RS_Vector radius = pos - m_rotateData->center;
                bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint){
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else{
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::ENABLE_SECOND_ROTATION);
                }

                if (m_freeAngle) {
                    m_rotateData->angle = (m_rotateData->center - pos).angle();
                    moveRelativeZero(m_rotateData->center);
                    setStatus(SetTargetPoint);
                }
                else{
                    if (rotationOverSamePoint) { // rotation over center, no need for second rotation
                        tryTrigger();
                    }
                    else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()){
                        setStatus(SetTargetPoint2ndRotation);
                    }
                    else{
                        tryTrigger();
                    }
                }
                break;
            }
            break;
        }
        case SetCenterPoint:{
            if (m_selectRefPointFirst){
                RS_Vector radius = pos - m_rotateData->refPoint;
                bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint){
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else{
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::ENABLE_SECOND_ROTATION);
                }
                if (m_freeAngle) {
                    m_rotateData->center = pos;
                    m_rotateData->angle = (m_rotateData->refPoint - pos).angle();
                    moveRelativeZero(m_rotateData->center);
                    setStatus(SetTargetPoint);
                }
                else{
                    m_rotateData->center = pos;
                    if (rotationOverSamePoint) { // rotation over center, no need for second rotation
                        tryTrigger();
                    }
                    else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()){
                        setStatus(SetTargetPoint2ndRotation);
                    }
                    else{
                        tryTrigger();
                    }
                }
                break;
            }
            else{
                m_rotateData->center = pos;
                moveRelativeZero(m_rotateData->center);
                setStatus(SetReferencePoint);
                break;
            }
        }
        case SetTargetPoint: {
            double wcsAngle = m_rotateData->center.angleTo(pos);
            double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
            m_rotateData->angle = adjustRelativeAngleSignByBasis(rotationAngle);

            RS_Vector radius = m_rotateData->center - m_rotateData->refPoint;
            bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
            if (rotationOverSamePoint) { // rotation over center, no need for second rotation
                tryTrigger();
            }
            else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()){
                setStatus(SetTargetPoint2ndRotation);
                RS_Vector originalRefPoint = m_rotateData->refPoint;
                RS_Vector center = m_rotateData->center;
                RS_Vector newRefPoint =  originalRefPoint.rotate(center, m_rotateData->angle);
                moveRelativeZero(newRefPoint);
            }
            else{
                tryTrigger();
            }
            break;
        }
        case SetTargetPoint2ndRotation:{
            RS_Vector newRefPoint =  m_rotateData->refPoint;
            newRefPoint.rotate(m_rotateData->center, m_rotateData->angle);
            RS_Vector delta = pos - newRefPoint;

            double secondAngle;

            if (delta.squared() < RS_TOLERANCE2){
                secondAngle = 0.;//angle not well-defined
            } else {
                double wcsAngle = newRefPoint.angleTo(pos);
                double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
                secondAngle = adjustRelativeAngleSignByBasis(rotationAngle);
            }
            m_rotateData->secondAngle = secondAngle;
            tryTrigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate::tryTrigger(){
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestRotateDialog(*m_rotateData)) {
            updateOptions();
            trigger();
            finish(false);
        }
    }
    else{
        trigger();
        finish(false);
    }
}

void RS_ActionModifyRotate::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    switch (status){
        case SetReferencePoint:{
            if (m_selectRefPointFirst){
                if (e->isControl) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getRelZeroAwarePoint(e, snapped);
                }
            }
            else{
                if (e->isControl) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getSnapAngleAwarePoint(e, m_rotateData->center, snapped);
                }
            }
            break;
        }
        case SetCenterPoint:{
            if (e->isControl) {
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                snapped = boundingForSelected.getCenter();
            }
            else {
                snapped = getSnapAngleAwarePoint(e, m_rotateData->refPoint, snapped);
            }
            break;
        }
        case SetTargetPoint:{
            snapped = getSnapAngleAwarePoint(e, m_rotateData->center, snapped);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionModifyRotate::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    switch (status)    {
        case SetReferencePoint: {
            if (m_selectRefPointFirst){
                m_selectionComplete = false;
            } else {
                setStatus(SetCenterPoint);
            }
            break;
        }
        case SetCenterPoint: {
            if (m_selectRefPointFirst){
                setStatus(SetReferencePoint);

            } else {
                m_selectionComplete = false;
            }
            break;
        }
        case SetTargetPoint: {
            if (m_selectRefPointFirst) {
                setStatus(SetCenterPoint);

            } else {
                setStatus(SetReferencePoint);
            }
            break;
        }
        case SetTargetPoint2ndRotation:{
            setStatus(SetTargetPoint);
            moveRelativeZero(m_rotateData->center);
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate::setFreeAngle(bool enable) {
    if (m_freeAngle != enable) {
        int status = getStatus();
        switch (status){
            case SetReferencePoint:
            case SetCenterPoint:
                break;
            case SetTargetPoint:
                setStatus(SetCenterPoint);
                moveRelativeZero(m_rotateData->center);
                break;
            default:
                break;
        }
        m_freeAngle = enable;
    }
}

void RS_ActionModifyRotate::setFreeRefPointAngle(bool value) {
    m_freeRefPointAngle = value;
}

double RS_ActionModifyRotate::getCurrentAngleDegrees() {
    return RS_Math::rad2deg(m_currentAngle);
}

double RS_ActionModifyRotate::getCurrentAngle2Degrees() {
    return toUCSBasisAngleDegrees(m_currentAngle2);
}

double RS_ActionModifyRotate::getAngle() const{
    return adjustRelativeAngleSignByBasis(m_rotateData->angle);
}

void RS_ActionModifyRotate::setAngle(double angleRad) {
    m_rotateData->angle = adjustRelativeAngleSignByBasis(angleRad);
}

double RS_ActionModifyRotate::getRefPointAngle() {
    return adjustRelativeAngleSignByBasis(m_rotateData->secondAngle);
}

void RS_ActionModifyRotate::setRefPointAngle(double angle) {
    m_rotateData->secondAngle = adjustRelativeAngleSignByBasis(angle);
}

bool RS_ActionModifyRotate::isRotateAlsoAroundReferencePoint() {
    return m_rotateData->twoRotations;
}

void RS_ActionModifyRotate::setRotateAlsoAroundReferencePoint(bool value) {
    m_rotateData->twoRotations = value;
}

bool RS_ActionModifyRotate::isRefPointAngleAbsolute() {
    return m_rotateData->secondAngleIsAbsolute;
}

void RS_ActionModifyRotate::setRefPointAngleAbsolute(bool val) {
    m_rotateData->secondAngleIsAbsolute = val;
}

void RS_ActionModifyRotate::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to rotate (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Rotate immediately after selection")));
}

void RS_ActionModifyRotate::updateMouseButtonHintsForSelected(int status){
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRBack(tr("Specify reference point"),MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
        case SetCenterPoint:
            updateMouseWidgetTRBack(tr("Specify rotation center"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Snap to center of selection")));
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point to rotate to"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetTargetPoint2ndRotation:
            updateMouseWidgetTRBack(tr("Specify target point for rotation around reference point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyRotate::doGetMouseCursorSelected([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *RS_ActionModifyRotate::getModifyOperationFlags() {
    return m_rotateData.get();
}

LC_ActionOptionsWidget *RS_ActionModifyRotate::createOptionsWidget() {
    return new LC_ModifyRotateOptions();
}
