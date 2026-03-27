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

#include "lc_action_modify_move_rotate.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_move_rotate_options_widget.h"
#include "lc_move_rotate_options_filler.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct LC_ActionModifyMoveRotate::MoveRotateActionData {
    RS_MoveRotateData data;
    RS_Vector targetPoint{false};
};

LC_ActionModifyMoveRotate::LC_ActionModifyMoveRotate(LC_ActionContext* actionContext)
    : LC_ActionModifyBase("ActionModifyMoveRotate", actionContext, RS2::ActionModifyMoveRotate),
      m_actionData(std::make_unique<MoveRotateActionData>()) {
}

LC_ActionModifyMoveRotate::~LC_ActionModifyMoveRotate() = default;

void LC_ActionModifyMoveRotate::doSaveOptions() {
    save("Angle", getAngle());
    save("UseCurrentLayer", isUseSameAngleForCopies());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());
    save("FreeAngle", isAngleFree());
    save("SameAngleForCopies", isUseSameAngleForCopies());
}

void LC_ActionModifyMoveRotate::doLoadOptions() {
    const double angle = loadDouble("Angle", 0.0);
    setAngle(angle);
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
    const bool freeAngle = loadBool("FreeAngle", true);
    setAngleIsFree(freeAngle);
    const bool sameAngle = loadBool("SameAngleForCopies", false);
    setUseSameAngleForCopies(sameAngle);
}

bool LC_ActionModifyMoveRotate::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint) || (status == SetTargetPoint)  || (status == SetAngle);
}

bool LC_ActionModifyMoveRotate::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    ctx.setActiveLayerAndPen(m_actionData->data.useCurrentLayer, m_actionData->data.useCurrentAttributes);
    RS_Modification::moveRotate(m_actionData->data, m_selectedEntities, false, ctx);
    m_actionData->targetPoint = RS_Vector(false);
    return true;
}

void LC_ActionModifyMoveRotate::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (m_actionData->data.keepOriginals) {
        unselect(m_selectedEntities);
    }
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyMoveRotate::doTriggerCompletion([[maybe_unused]] bool success) {
    finish();
}

void LC_ActionModifyMoveRotate::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            const RS_Vector& originalRefPoint = m_actionData->data.referencePoint;
            if (originalRefPoint.valid) {
                mouse = getSnapAngleAwarePoint(e, originalRefPoint, mouse, true);
                m_actionData->data.offset = mouse - originalRefPoint;
                LC_DocumentModificationBatch ctx;
                RS_Modification::moveRotate(m_actionData->data, m_selectedEntities, true, ctx);
                previewEntitiesToAdd(ctx);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(originalRefPoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(originalRefPoint, mouse);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Moving with rotation")).vector(tr("Source:"), m_actionData->data.referencePoint).vector(tr("Target:"), mouse).
                                                    string(tr("Offset:")).relative(m_actionData->data.offset).relativePolar(
                                                        m_actionData->data.offset).toInfoCursorZone2(false);
                }
            }
            break;
        }
        case SetAngle: {
            const RS_Vector& targetPoint = m_actionData->targetPoint;
            const RS_Vector& originalRefPoint = m_actionData->data.referencePoint;
            if (targetPoint.valid) {
                mouse = getSnapAngleAwarePoint(e, targetPoint, mouse, true);
                const double wcsAngle = targetPoint.angleTo(mouse);
                const double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
                const double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
                m_actionData->data.angle = wcsRotationAngle;
                LC_DocumentModificationBatch ctx;
                RS_Modification::moveRotate(m_actionData->data, m_selectedEntities, true, ctx);
                previewEntitiesToAdd(ctx);
                if (m_showRefEntitiesOnPreview) {
                    previewSnapAngleMark(targetPoint, mouse);
                    previewRefPoint(originalRefPoint);
                    previewRefPoint(targetPoint);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(targetPoint, mouse);
                    previewRefLine(originalRefPoint, targetPoint);
                    previewRefPointsForMultipleCopies();
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Moving with rotation")).vector(tr("Source:"), originalRefPoint).vector(tr("Target:"), targetPoint).
                                                    string(tr("Offset:")).relative(m_actionData->data.offset).relativePolar(
                                                        m_actionData->data.offset).rawAngle(tr("Angle:"), rotationAngle).toInfoCursorZone2(
                                                        false);
                }
                updateOptions();
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyMoveRotate::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngle(angleRad);
        return true;
    }
    return false;
}

void LC_ActionModifyMoveRotate::previewRefPointsForMultipleCopies() const {
    const auto& data = m_actionData->data;
    if (data.multipleCopies) {
        const int numPoints = data.number;
        if (numPoints > 1) {
            for (int i = 1; i <= numPoints; i++) {
                RS_Vector offset = data.offset * i;
                previewRefPoint(data.referencePoint + offset);
            }
        }
    }
}

void LC_ActionModifyMoveRotate::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetReferencePoint: {
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

void LC_ActionModifyMoveRotate::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            if (m_selectionComplete) {
                m_selectionComplete = false;
            }
            else {
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

void LC_ActionModifyMoveRotate::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = pos;
            addSnappedPointToVisualSnap(pos);
            setStatus(ShowDialog);
            m_actionData->data.offset = pos - m_actionData->data.referencePoint;
            if (m_angleIsFixed) {
                doPerformTrigger();
            }
            else {
                moveRelativeZero(pos);
                setStatus(SetAngle);
                m_lastStatus = SetTargetPoint;
            }
            break;
        }
        case SetAngle: {
            if (m_actionData->targetPoint.valid) {
                //                double angle = pPoints->targetPoint.angleTo(pos);
                const double wcsAngle = m_actionData->targetPoint.angleTo(pos);
                const double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
                const double wcsRotationAngle = adjustRelativeAngleSignByBasis(rotationAngle);
                m_actionData->data.angle = wcsRotationAngle;
                doPerformTrigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyMoveRotate::doPerformTrigger() {
    trigger();
}

bool LC_ActionModifyMoveRotate::doProcessCommand(int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetReferencePoint:
        case SetTargetPoint: {
            if (checkCommand("angle", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetAngle);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            const double a = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                // relative angle is used, no need to translate
                m_actionData->data.angle = adjustRelativeAngleSignByBasis(RS_Math::deg2rad(a));
                if (m_angleIsFixed) {
                    updateOptions();
                    setStatus(m_lastStatus);
                }
                else if (m_actionData->targetPoint.valid) {
                    updateOptions();
                    doPerformTrigger();
                }
                else {
                    m_angleIsFixed = true;
                    updateOptions();
                    setStatus(m_lastStatus);
                }
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionModifyMoveRotate::getAvailableCommands() {
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

void LC_ActionModifyMoveRotate::setAngle(const double angleRad) const {
    m_actionData->data.angle = adjustRelativeAngleSignByBasis(angleRad);
}

double LC_ActionModifyMoveRotate::getAngle() const {
    return adjustRelativeAngleSignByBasis(m_actionData->data.angle);
}

void LC_ActionModifyMoveRotate::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            updatePromptTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updatePromptTRBack(tr("Specify target point"));
            break;
        case SetAngle:
            updatePromptTRBack(tr("Enter rotation angle:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyMoveRotate::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyMoveRotate::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to move and rotate") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Move and rotate immediately after selection")));
}

LC_ModifyOperationFlags* LC_ActionModifyMoveRotate::getModifyOperationFlags() {
    return &m_actionData->data;
}

void LC_ActionModifyMoveRotate::setAngleIsFree(const bool b) {
    m_angleIsFixed = !b;
    if (m_angleIsFixed && getStatus() == SetAngle) {
        setStatus(SetTargetPoint);
    }
}

void LC_ActionModifyMoveRotate::setUseSameAngleForCopies(const bool b) const {
    m_actionData->data.sameAngleForCopies = b;
}

bool LC_ActionModifyMoveRotate::isUseSameAngleForCopies() const {
    return m_actionData->data.sameAngleForCopies;
}

LC_ActionOptionsWidget* LC_ActionModifyMoveRotate::createOptionsWidget() {
    return new LC_MoveRotateOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyMoveRotate::createOptionsFiller() {
    return new LC_MoveRotateOptionsFiller();
}
