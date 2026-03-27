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

#include "lc_action_modify_rotate.h"

#include <QKeyEvent>

#include "lc_actioninfomessagebuilder.h"
#include "lc_formatter.h"
#include "lc_rotate_options_filler.h"
#include "lc_rotate_options_widget.h"
#include "rs_modification.h"
#include "rs_preview.h"

// fixme - sand - add options for point selection mode (ref point/center)? Or just remove center selection first and have one way?
// fixme - sand - GENERAL: NAVIGATE TO OPTIONS WIDGET BY KEYBOARD (FOCUS from action, TABS) - so it should be possible not to move mouse to change some option

LC_ActionModifyRotate::LC_ActionModifyRotate(LC_ActionContext* actionContext)
    : LC_ActionModifyBase("ActionModifyRotate", actionContext, m_actionType = RS2::ActionModifyRotate), m_rotateData(new RS_RotateData()) {
}

LC_ActionModifyRotate::~LC_ActionModifyRotate() = default;

void LC_ActionModifyRotate::doSaveOptions() {
    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());

    save("Angle", getAngle());
    save("AngleIsFree", isFreeAngle());
    save("AngleIsRelative", isRelativeAngle());
    save("CenterPointFirst", isCenterPointFirst());
    save("TwoRotations", isRotateAlsoAroundReferencePoint());
    save("AngleRef", getRefPointAngle());
    save("AngleRefIsFree", isFreeRefPointAngle());
    save("AngleRefIsAbsolute", isRefPointAngleAbsolute());
}

void LC_ActionModifyRotate::doLoadOptions() {
    const bool curLayer = loadBool("UseCurrentLayer", true);
    setUseCurrentLayer(curLayer);
    const bool curAttrs = loadBool("UseCurrentAttributes", true);
    setUseCurrentAttributes(curAttrs);
    const bool keepOriginals = loadBool("KeepOriginals", false);
    setKeepOriginals(keepOriginals);
    const bool multiCopy = loadBool("MultipleCopies", false);
    setUseMultipleCopies(multiCopy);
    const int copiesNum = loadInt("Copies", 1);
    setCopiesNumber(copiesNum);

    double angle = loadDouble("Angle", 0.0);
    setAngle(angle);

    bool freeAngle = loadBool("AngleIsFree", false);
    setFreeAngle(freeAngle);

    bool angleIsRelative = loadBool("AngleIsRelative", false);
    setRelativeAngle(angleIsRelative);

    bool centerFirst = loadBool("CenterPointFirst", true);
    setCenterPointFirst(centerFirst);

    bool rotate2 = loadBool("TwoRotations", false);
    setRotateAlsoAroundReferencePoint(rotate2);

    double angle2 = loadDouble("AngleRef", 0.0);
    setRefPointAngle(angle2);

    bool angle2free = loadBool("AngleRefIsFree", false);
    setFreeRefPointAngle(angle2free);

    bool angle2abs = loadBool("AngleRefIsAbsolute", false);
    setRefPointAngleAbsolute(angle2abs);
}

bool LC_ActionModifyRotate::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint) || (status == SetTargetPoint) || (status == SetCenterPoint) || (status == SetTargetPoint2ndRotation);
}

void LC_ActionModifyRotate::init(const int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

void LC_ActionModifyRotate::onSelectionCompleted(const bool singleEntity, const bool fromInit) {
    LC_ActionModifyBase::onSelectionCompleted(singleEntity, fromInit);
    if (m_selectRefPointFirst) {
        setStatus(SetReferencePoint);
    }
    else {
        setStatus(SetCenterPoint);
    }
}

bool LC_ActionModifyRotate::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    moveRelativeZero(m_rotateData->center);
    ctx.setActiveLayerAndPen(m_rotateData->useCurrentLayer, m_rotateData->useCurrentAttributes);
    return RS_Modification::rotate(*m_rotateData, m_selectedEntities, false, ctx);
}

void LC_ActionModifyRotate::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (m_rotateData->keepOriginals) {
        unselect(m_selectedEntities);
    }
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyRotate::doTriggerCompletion([[maybe_unused]] bool success) {
}

void LC_ActionModifyRotate::previewRotatedEntities(const RS_RotateData& rotateData) const {
    LC_DocumentModificationBatch ctx;
    RS_Modification::rotate(rotateData, m_selectedEntities, true, ctx);
    previewEntitiesToAdd(ctx);
}

void LC_ActionModifyRotate::onMouseMoveEventSelected(int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            if (m_selectRefPointFirst) {
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
            }
            else {
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
                if (!m_freeAngle) {
                    RS_RotateData tmpData = *m_rotateData;
                    tmpData.refPoint = mouse;
                    previewRotatedEntities(tmpData);
                    previewRotationCircleAndPoints(m_rotateData->center, mouse, m_rotateData->angle);
                }
            }
            break;
        }
        case SetCenterPoint: {
            if (m_selectRefPointFirst) {
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
                    previewRotatedEntities(tmpData);
                    previewRotationCircleAndPoints(mouse, m_rotateData->refPoint, m_rotateData->angle);

                    if (isInfoCursorForModificationEnabled()) {
                        RS_Vector offset = mouse - originalReferencePoint;
                        msg(tr("Rotation")).rawAngle(tr("Angle:"), adjustRelativeAngleSignByBasis(m_rotateData->angle)).
                                            vector(tr("Reference Point:"), m_rotateData->refPoint).vector(tr("Center Point:"), mouse).
                                            string(tr("Offset:")).relative(offset).relativePolar(offset).toInfoCursorZone2(false);
                    }
                }
                else if (isInfoCursorForModificationEnabled()) {
                    RS_Vector offset = mouse - originalReferencePoint;

                    msg(tr("Rotation")).vector(tr("Reference Point:"), m_rotateData->refPoint).vector(tr("Center Point:"), mouse).
                                        string(tr("Offset:")).relative(offset).relativePolar(offset).toInfoCursorZone2(false);
                }
            }
            else {
                if (!trySnapToRelZeroCoordinateEvent(e)) {
                    RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    RS_Vector center = boundingForSelected.getCenter();
                    if (e->isControl) {
                        mouse = center;
                    }
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(center, mouse);
                        previewRefSelectablePoint(mouse);
                        previewRefPoint(center);
                        if (!m_freeAngle) {
                            previewRotationCircleAndPoints(mouse, center, m_rotateData->angle);
                        }
                    }
                    if (!m_freeAngle) {
                        RS_RotateData tmpData = *m_rotateData;
                        tmpData.center = mouse;
                        previewRotatedEntities(tmpData);
                    }
                }
            }
            break;
        }
        case SetTargetPoint: {
            RS_Vector& center = m_rotateData->center;
            RS_Vector& originalReferencePoint = m_rotateData->refPoint;
            mouse = getSnapAngleAwarePoint(e, center, mouse, true);
            double radius = center.distanceTo(originalReferencePoint);
            double rotationAngle;
            double angleFromCenterToMouse = center.angleTo(mouse);
            double angleCenterToOriginalRefPpoint = center.angleTo(originalReferencePoint);
            if (m_relativeAngle) {
                rotationAngle = angleFromCenterToMouse - angleCenterToOriginalRefPpoint;
            }
            else {
                rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(angleFromCenterToMouse));
            }

            RS_Vector newAxisPoint = center.relative(radius, angleFromCenterToMouse);
            if (m_showRefEntitiesOnPreview) {
                RS_Vector xAxisPoint;
                if (m_relativeAngle) {
                    xAxisPoint = originalReferencePoint;
                    previewSnapAngleMark(center, angleFromCenterToMouse, angleCenterToOriginalRefPpoint,
                                         m_formatter->isAnglesCounterClockWise());
                }
                else {
                    xAxisPoint = center.relative(radius, toWorldAngleFromUCSBasis(0));
                    previewSnapAngleMark(center, mouse);
                }
                previewRefSelectablePoint(newAxisPoint);
                previewRefPoint(xAxisPoint);
                previewRefLine(center, xAxisPoint);
                previewRefPoint(originalReferencePoint);
                previewRefPoint(center);
                previewRefLine(mouse, center);
            }

            m_currentAngle = rotationAngle;
            updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::UPDATE_ANGLE);

            RS_RotateData tmpData = *m_rotateData;
            double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
            tmpData.angle = wcsRotationAngle;

            previewRotatedEntities(tmpData);

            // todo - sand - we can temporarily add a copy of circle to the document, so intersection snap for target reference point will work.
            previewRotationCircleAndPoints(center, m_rotateData->refPoint, wcsRotationAngle);

            if (isInfoCursorForModificationEnabled()) {
                RS_Vector offset = newAxisPoint - originalReferencePoint;
                msg(tr("Rotation")).rawAngle(tr("Angle:"), rotationAngle).vector(tr("Source Point:"), originalReferencePoint).
                                    vector(tr("Target Point:"), newAxisPoint).string(tr("Offset:")).relative(offset).relativePolar(offset).
                                    toInfoCursorZone2(false);
            }
            break;
        }
        case SetTargetPoint2ndRotation: {
            RS_Vector originalRefPoint = m_rotateData->refPoint;
            RS_Vector center = m_rotateData->center;
            RS_Vector newRefPoint = originalRefPoint;
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
            updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::UPDATE_ANGLE2);

            RS_RotateData tmpData = *m_rotateData;
            tmpData.secondAngle = adjustRelativeAngleSignByBasis(rotationAngle);

            previewRotatedEntities(tmpData);

            if (isInfoCursorForModificationEnabled()) {
                RS_Vector offset = newRefPoint - originalRefPoint;
                msg(tr("Rotation")).rawAngle(tr("Angle:"), m_rotateData->angle).vector(tr("Source Point:"), originalRefPoint).
                                    vector(tr("Target Point:"), newRefPoint).string(tr("Offset:")).relative(offset).relativePolar(offset).
                                    rawAngle(tr("Second Angle:"), m_currentAngle2).toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyRotate::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
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

void LC_ActionModifyRotate::previewRotationCircleAndPoints(const RS_Vector& center, const RS_Vector& refPoint, const double angle) const {
    if (m_showRefEntitiesOnPreview) {
        const double radius = center.distanceTo(refPoint);
        previewRefCircle(center, radius);
        const int numberOfCopies = m_rotateData->obtainNumberOfCopies();
        for (int i = 1; i <= numberOfCopies; i++) {
            //
            RS_Vector rotatedRefPoint = refPoint;
            const double rotationAngle = angle * i;
            rotatedRefPoint.rotate(center, rotationAngle);
            previewRefPoint(rotatedRefPoint);
        }
    }
}

// fixme - sand -  support for focus of options widgets by keyboard...this should be removed as it may potentially may conflict with some shortcut. We need separate action for this..
void LC_ActionModifyRotate::keyPressEvent(QKeyEvent* e) {
    const int key = e->key();
    switch (key) {
        case Qt::Key_Space: {
            if (isControl(e)) {
                // fixme - options- update UI
                // m_optionWidget->setFocus();
            }
            break;
        }
        default:
            RS_ActionSelectBase::keyPressEvent(e);
            break;
    }
}

void LC_ActionModifyRotate::setCenterPointFirst(bool val) {
    m_selectRefPointFirst = !val;
    if (m_selectionComplete) {
        if (m_selectRefPointFirst) {
            setStatus(SetReferencePoint);
        }
        else {
            setStatus(SetCenterPoint);
        }
    }
}

void LC_ActionModifyRotate::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    if (!pos.valid) {
        return;
    }
    switch (status) {
        case SetReferencePoint: {
            addSnappedPointToVisualSnap(pos);
            if (m_selectRefPointFirst) {
                m_rotateData->refPoint = pos;
                moveRelativeZero(pos);
                setStatus(SetCenterPoint);
            }
            else {
                m_rotateData->refPoint = pos;
                const RS_Vector radius = pos - m_rotateData->center;
                const bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint) {
                    updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else {
                    updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::ENABLE_SECOND_ROTATION);
                }

                if (m_freeAngle) {
                    m_rotateData->angle = (m_rotateData->center - pos).angle();
                    moveRelativeZero(m_rotateData->center);
                    setStatus(SetTargetPoint);
                }
                else {
                    if (rotationOverSamePoint) {
                        // rotation over center, no need for second rotation
                        tryTrigger();
                    }
                    else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()) {
                        setStatus(SetTargetPoint2ndRotation);
                    }
                    else {
                        tryTrigger();
                    }
                }
                break;
            }
            break;
        }
        case SetCenterPoint: {
            addSnappedPointToVisualSnap(pos);
            if (m_selectRefPointFirst) {
                const RS_Vector radius = pos - m_rotateData->refPoint;
                const bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
                if (rotationOverSamePoint) {
                    updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::DISABLE_SECOND_ROTATION);
                }
                else {
                    updateOptionsUI(LC_RotateOptionsWidget::UpdateMode::ENABLE_SECOND_ROTATION);
                }
                if (m_freeAngle) {
                    m_rotateData->center = pos;
                    m_rotateData->angle = (m_rotateData->refPoint - pos).angle();
                    moveRelativeZero(m_rotateData->center);
                    setStatus(SetTargetPoint);
                }
                else {
                    m_rotateData->center = pos;
                    if (rotationOverSamePoint) {
                        // rotation over center, no need for second rotation
                        tryTrigger();
                    }
                    else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()) {
                        setStatus(SetTargetPoint2ndRotation);
                    }
                    else {
                        tryTrigger();
                    }
                }
            }
            else {
                m_rotateData->center = pos;
                moveRelativeZero(m_rotateData->center);
                if (m_rotateData->twoRotations) {
                    setStatus(SetReferencePoint);
                }
                else {
                    if (m_freeAngle) {
                        setStatus(SetReferencePoint);
                    }
                    else {
                        tryTrigger();
                    }
                }
            }
            break;
        }
        case SetTargetPoint: {
            addSnappedPointToVisualSnap(pos);
            const double angleCenterToPosWCS = m_rotateData->center.angleTo(pos);
            double rotationAngle;
            if (m_relativeAngle) {
                const double angleCenterToRefPointWCS = m_rotateData->center.angleTo(m_rotateData->refPoint);
                rotationAngle = RS_Math::getAngleDifference(angleCenterToRefPointWCS, angleCenterToPosWCS);
            }
            else {
                double rotation = RS_Math::correctAngle(toUCSBasisAngle(angleCenterToPosWCS));
                rotationAngle = adjustRelativeAngleSignByBasis(rotation);
            }
            m_rotateData->angle = rotationAngle;

            const RS_Vector radius = m_rotateData->center - m_rotateData->refPoint;
            const bool rotationOverSamePoint = radius.squared() <= RS_TOLERANCE;
            if (rotationOverSamePoint) {
                // rotation over center, no need for second rotation
                tryTrigger();
            }
            else if (isRotateAlsoAroundReferencePoint() && isFreeRefPointAngle()) {
                setStatus(SetTargetPoint2ndRotation);
                RS_Vector originalRefPoint = m_rotateData->refPoint;
                const RS_Vector center = m_rotateData->center;
                const RS_Vector newRefPoint = originalRefPoint.rotate(center, m_rotateData->angle);
                moveRelativeZero(newRefPoint);
            }
            else {
                tryTrigger();
            }
            break;
        }
        case SetTargetPoint2ndRotation: {
            addSnappedPointToVisualSnap(pos);
            RS_Vector newRefPoint = m_rotateData->refPoint;
            newRefPoint.rotate(m_rotateData->center, m_rotateData->angle);
            const RS_Vector delta = pos - newRefPoint;

            double secondAngle;

            if (delta.squared() < RS_TOLERANCE2) {
                secondAngle = 0.; //angle not well-defined
            }
            else {
                const double wcsAngle = newRefPoint.angleTo(pos);
                const double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
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

void LC_ActionModifyRotate::tryTrigger() {
    trigger();
    finish();
}

void LC_ActionModifyRotate::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            if (m_selectRefPointFirst) {
                if (e->isControl) {
                    const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getRelZeroAwarePoint(e, snapped);
                }
            }
            else {
                if (e->isControl) {
                    const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    snapped = boundingForSelected.getCenter();
                }
                else {
                    snapped = getSnapAngleAwarePoint(e, m_rotateData->center, snapped);
                }
            }
            break;
        }
        case SetCenterPoint: {
            if (e->isControl) {
                const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                snapped = boundingForSelected.getCenter();
            }
            else {
                if (m_selectRefPointFirst) {
                    snapped = getSnapAngleAwarePoint(e, m_rotateData->refPoint, snapped);
                }
                else {
                    const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                    const RS_Vector selectionCenter = boundingForSelected.getCenter();
                    snapped = getSnapAngleAwarePoint(e, selectionCenter, snapped);
                }
            }
            break;
        }
        case SetTargetPoint: {
            snapped = getSnapAngleAwarePoint(e, m_rotateData->center, snapped);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionModifyRotate::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            if (m_selectRefPointFirst) {
                m_selectionComplete = false;
            }
            else {
                setStatus(SetCenterPoint);
            }
            break;
        }
        case SetCenterPoint: {
            if (m_selectRefPointFirst) {
                setStatus(SetReferencePoint);
            }
            else {
                m_selectionComplete = false;
            }
            break;
        }
        case SetTargetPoint: {
            if (m_selectRefPointFirst) {
                setStatus(SetCenterPoint);
            }
            else {
                setStatus(SetReferencePoint);
            }
            break;
        }
        case SetTargetPoint2ndRotation: {
            setStatus(SetTargetPoint);
            moveRelativeZero(m_rotateData->center);
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyRotate::setFreeAngle(const bool enable) {
    if (m_freeAngle != enable) {
        const int status = getStatus();
        switch (status) {
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

void LC_ActionModifyRotate::setRelativeAngle(bool enable) {
    m_relativeAngle = enable;
}

void LC_ActionModifyRotate::setFreeRefPointAngle(const bool value) {
    m_freeRefPointAngle = value;
}

double LC_ActionModifyRotate::getCurrentAngleDegrees() const {
    return RS_Math::rad2deg(m_currentAngle);
}

double LC_ActionModifyRotate::getCurrentAngle2Degrees() const {
    return toUCSBasisAngleDegrees(m_currentAngle2);
}

double LC_ActionModifyRotate::getAngle() const {
    return adjustRelativeAngleSignByBasis(m_rotateData->angle);
}

void LC_ActionModifyRotate::setAngle(const double angleRad) const {
    m_rotateData->angle = adjustRelativeAngleSignByBasis(angleRad);
}

double LC_ActionModifyRotate::getRefPointAngle() const {
    return adjustRelativeAngleSignByBasis(m_rotateData->secondAngle);
}

void LC_ActionModifyRotate::setRefPointAngle(const double angle) const {
    m_rotateData->secondAngle = adjustRelativeAngleSignByBasis(angle);
}

bool LC_ActionModifyRotate::isRotateAlsoAroundReferencePoint() const {
    return m_rotateData->twoRotations;
}

void LC_ActionModifyRotate::setRotateAlsoAroundReferencePoint(const bool value) const {
    m_rotateData->twoRotations = value;
}

bool LC_ActionModifyRotate::isRefPointAngleAbsolute() const {
    return m_rotateData->secondAngleIsAbsolute;
}

void LC_ActionModifyRotate::setRefPointAngleAbsolute(const bool val) const {
    m_rotateData->secondAngleIsAbsolute = val;
}

void LC_ActionModifyRotate::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to rotate") + getSelectionCompletionHintMsg(),
                               MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Rotate immediately after selection")));
}

void LC_ActionModifyRotate::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            updatePromptTRBack(tr("Specify reference point"),MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
        case SetCenterPoint:
            updatePromptTRBack(tr("Specify rotation center"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Snap to center of selection")));
            break;
        case SetTargetPoint:
            updatePromptTRBack(tr("Specify target point to rotate to"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetTargetPoint2ndRotation:
            updatePromptTRBack(tr("Specify target point for rotation around reference point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyRotate::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

LC_ModifyOperationFlags* LC_ActionModifyRotate::getModifyOperationFlags() {
    return m_rotateData.get();
}

LC_ActionOptionsWidget* LC_ActionModifyRotate::createOptionsWidget() {
    return new LC_RotateOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyRotate::createOptionsFiller() {
    return new LC_RotateOptionsFiller();
}
