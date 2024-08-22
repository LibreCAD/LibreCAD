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
#include <QApplication>

#include "rs_actionmodifyrotate.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_arc.h"
#include "lc_modifyrotateoptions.h"

// fixme - sand - add options for point selection mode (ref point/center)? Or just remove center selection first and have one way?
// fixme - sand - GENERAL: NAVIGATE TO OPTIONS WIDGET BY KEYBOARD (FOCUS from action, TABS) - so it should be possible not to move mouse to change some option

RS_ActionModifyRotate::RS_ActionModifyRotate(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :LC_ActionModifyBase("Rotate Entities",container, graphicView)
	,data(new RS_RotateData()){
    actionType=RS2::ActionModifyRotate;
}

RS_ActionModifyRotate::~RS_ActionModifyRotate() = default;

void RS_ActionModifyRotate::init(int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

void RS_ActionModifyRotate::selectionCompleted(bool singleEntity, bool fromInit) {
    LC_ActionModifyBase::selectionCompleted(singleEntity, fromInit);
    if (selectRefPointFirst){
        setStatus(SetReferencePoint);
    }
    else{
        setStatus(SetCenterPoint);
    }
}

void RS_ActionModifyRotate::trigger(){
    RS_DEBUG->print("RS_ActionModifyRotate::trigger()");

    RS_Modification m(*container, graphicView);
    m.rotate(*data, selectedEntities, false);
    moveRelativeZero(data->center);
    updateSelectionWidget();
}

void RS_ActionModifyRotate::mouseMoveEventSelected(QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    switch (getStatus()) {
        case SetReferencePoint: {
            if (selectRefPointFirst){
                if (isControl(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getRelZeroAwarePoint(e, mouse);
                }
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(mouse);
                }

            } else {
                if (isControl(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);
                }
                if (showRefEntitiesOnPreview) {
                    previewRefLine(data->center, mouse);
                    previewRefPoint(data->center);
                    previewRefSelectablePoint(mouse);
                }
                if (!freeAngle){
                    RS_RotateData tmpData = *data;
                    tmpData.refPoint = mouse;
                    RS_Modification m(*preview, graphicView, false);
                    m.rotate(tmpData, selectedEntities, true);
                    previewRotationCircleAndPoints(data->center, mouse, data->angle);
                }
            }
            break;
        }
        case SetCenterPoint: {
            if (selectRefPointFirst){
                if (isControl(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    mouse = boundingForSelected.getCenter();
                }
                else {
                    mouse = getSnapAngleAwarePoint(e, data->refPoint, mouse, true);
                }
                RS_Vector originalReferencePoint = data->refPoint;
                if (showRefEntitiesOnPreview) {
                    previewRefLine(originalReferencePoint, mouse);
                    previewRefPoint(originalReferencePoint);
                }
                if (!freeAngle){
                    RS_RotateData tmpData = *data;
                    tmpData.center = mouse;
                    RS_Modification m(*preview, graphicView, false);
                    m.rotate(tmpData, selectedEntities, true);
                    previewRotationCircleAndPoints(mouse, data->refPoint, data->angle);
                }
            } else {
                if (!trySnapToRelZeroCoordinateEvent(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    RS_Vector center = boundingForSelected.getCenter();
                    if (isControl(e)) {
                        mouse = center;
                    }
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(center, mouse);
                    }
                }
            }
            break;
        }
        case SetTargetPoint: {
            mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);

            double radius = data->center.distanceTo(data->refPoint);

            double rotationAngle = data->center.angleTo(mouse);

            if (showRefEntitiesOnPreview) {
                RS_Vector originalReferencePoint = data->refPoint;
                RS_Vector xAxisPoint = data->center.relative(radius, 0);
                RS_Vector circlePoint = data->center.relative(radius, rotationAngle);
                previewRefSelectablePoint(circlePoint);

                previewRefPoint(xAxisPoint);
                previewRefLine(data->center, xAxisPoint);

                previewRefPoint(originalReferencePoint);
                //            previewRefSelectablePoint(newReferencePoint);
                previewRefPoint(data->center);
                //            previewRefLine(originalReferencePoint, data->center);
                previewRefLine(mouse, data->center);
            }

            currentAngle = rotationAngle;
            updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::UPDATE_ANGLE);

            RS_RotateData tmpData = *data;
            tmpData.angle = rotationAngle;

            RS_Modification m(*preview, graphicView, false);
            m.rotate(tmpData, selectedEntities, true);

            // todo - sand - we can temporarily add a copy of circle to the document, so intersection snap for target reference point will work.
            previewRotationCircleAndPoints(data->center, data->refPoint, rotationAngle);
            break;
        }
        case SetTargetPoint2ndRotation:{
            mouse = getSnapAngleAwarePoint(e, data->refPoint, mouse, true);

            if (showRefEntitiesOnPreview) {
                RS_Vector originalReferencePoint = data->refPoint;
                previewRefPoint(originalReferencePoint);
                previewRefLine(originalReferencePoint, mouse);
                previewRefPoint(data->center);
            }
            previewRotationCircleAndPoints(data->center, data->refPoint, data->angle);

            double secondRotationAngle = /*RS_Math::correctAngle((mouse - data->refPoint).angle() - data->secondAngle);*/(mouse - data->refPoint).angle();

            RS_RotateData tmpData = *data;
            tmpData.secondAngle = secondRotationAngle;

            RS_Modification m(*preview, graphicView, false);
            m.rotate(tmpData, selectedEntities, true);

            currentAngle2 = secondRotationAngle;
            updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::UPDATE_ANGLE2);

            break;
        }

        default:
            break;
    }
    drawPreview();
    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent end");
}

void RS_ActionModifyRotate::previewRotationCircleAndPoints(const RS_Vector &center, const RS_Vector &refPoint, double angle) {
    if (showRefEntitiesOnPreview) {
        double radius = center.distanceTo(refPoint);
        previewRefCircle(center, radius);
        int numberOfCopies = data->obtainNumberOfCopies();
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
            if (selectRefPointFirst){
                data->refPoint = pos;
                moveRelativeZero(pos);
                setStatus(SetCenterPoint);
            }
            else{
                data->refPoint = pos;
                RS_Vector radius = pos - data->center;
                bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint){
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else{
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::ENABLE_SECOND_ROTATION);
                }

                if (freeAngle) {
                    data->angle = (data->center - pos).angle();
                    moveRelativeZero(data->center);
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
            if (selectRefPointFirst){
                RS_Vector radius = pos - data->refPoint;
                bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint){
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else{
                    updateOptionsUI(LC_ModifyRotateOptions::UpdateMode::ENABLE_SECOND_ROTATION);
                }
                if (freeAngle) {
                    data->center = pos;
                    data->angle = (data->refPoint - pos).angle();
                    moveRelativeZero(data->center);
                    setStatus(SetTargetPoint);
                }
                else{
                    data->center = pos;
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
                data->center = pos;
                moveRelativeZero(data->center);
                setStatus(SetReferencePoint);
                break;
            }
        }
        case SetTargetPoint: {
//            pos -= data->center;
//            if (pos.squared() < RS_TOLERANCE2){
//                data->angle = 0.;//angle not well defined
//            } else {
//                data->angle = RS_Math::correctAngle(pos.angle() - data->angle);
//            }

            data->angle = data->center.angleTo(pos);


            RS_Vector radius = data->center - data->refPoint;
            bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
            if (rotationOverSamePoint) { // rotation over center, no need for second rotation
                tryTrigger();
            }
            else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()){
                setStatus(SetTargetPoint2ndRotation);
                moveRelativeZero(data->refPoint);
            }
            else{
                tryTrigger();
            }
            break;
        }
        case SetTargetPoint2ndRotation:{
            RS_Vector coord = pos;
            coord-=data->refPoint;
            if (coord.squared() < RS_TOLERANCE2){
                data->secondAngle = 0.;//angle not well-defined
            } else {
                data->secondAngle = RS_Math::correctAngle(coord.angle() - data->secondAngle);
            }
            tryTrigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate::tryTrigger(){
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestRotateDialog(*data)) {
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

void RS_ActionModifyRotate::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    switch (status){
        case SetReferencePoint:{
            if (selectRefPointFirst){
                if (isControl(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getRelZeroAwarePoint(e, snapped);
                }
            }
            else{
                if (isControl(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getSnapAngleAwarePoint(e, data->center, snapped);
                }
            }
            break;
        }
        case SetCenterPoint:{
            if (isControl(e)) {
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                snapped = boundingForSelected.getCenter();
            }
            else {
                snapped = getSnapAngleAwarePoint(e, data->refPoint, snapped);
            }
            break;
        }
        case SetTargetPoint:{
            snapped = getSnapAngleAwarePoint(e, data->center, snapped);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionModifyRotate::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    switch (status)    {
        case SetReferencePoint: {
            if (selectRefPointFirst){
                selectionComplete = false;
            } else {
                setStatus(SetCenterPoint);
            }
            break;
        }
        case SetCenterPoint: {
            if (selectRefPointFirst){
                setStatus(SetReferencePoint);

            } else {
                selectionComplete = false;
            }
            break;
        }
        case SetTargetPoint: {
            if (selectRefPointFirst) {
                setStatus(SetCenterPoint);

            } else {
                setStatus(SetReferencePoint);
            }
            break;
        }
        case SetTargetPoint2ndRotation:{
            setStatus(SetTargetPoint);
            moveRelativeZero(data->center);
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate::setFreeAngle(bool enable) {
    if (freeAngle != enable) {
        int status = getStatus();
        switch (status){
            case SetReferencePoint:
            case SetCenterPoint:
                break;
            case SetTargetPoint:
                setStatus(SetCenterPoint);
                moveRelativeZero(data->center);
                break;
            default:
                break;
        }
        freeAngle = enable;
    }
}

void RS_ActionModifyRotate::setFreeRefPointAngle(bool value) {
    freeRefPointAngle = value;
}

double RS_ActionModifyRotate::getAngle() {
    return data->angle;
}

void RS_ActionModifyRotate::setAngle(double angle) {
    data->angle = angle;
}

double RS_ActionModifyRotate::getRefPointAngle() {
    return data->secondAngle;
}

void RS_ActionModifyRotate::setRefPointAngle(double angle) {
    data->secondAngle = angle;
}

bool RS_ActionModifyRotate::isRotateAlsoAroundReferencePoint() {
    return data->twoRotations;
}

void RS_ActionModifyRotate::setRotateAlsoAroundReferencePoint(bool value) {
    data->twoRotations = value;
}

bool RS_ActionModifyRotate::isRefPointAngleAbsolute() {
    return data->secondAngleIsAbsolute;
}

void RS_ActionModifyRotate::setRefPointAngleAbsolute(bool val) {
    data->secondAngleIsAbsolute = val;
}

void RS_ActionModifyRotate::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to rotate"), MOD_CTRL(tr("Rotate immediately after selection")));
}

void RS_ActionModifyRotate::updateMouseButtonHintsForSelected(int status){
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRBack(tr("Specify reference point"),MOD_SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
        case SetCenterPoint:
            updateMouseWidgetTRBack(tr("Specify rotation center"), MOD_SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_ANGLE_SNAP, tr("Snap to center of selection")));
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
    return data.get();
}

LC_ActionOptionsWidget *RS_ActionModifyRotate::createOptionsWidget() {
    return new LC_ModifyRotateOptions();
}
