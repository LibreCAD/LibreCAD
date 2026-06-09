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

#include "lc_action_modify_rotate_twice.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_rotate_2_options_filler.h"
#include "lc_rotate_2_options_widget.h"
#include "rs_modification.h"
#include "rs_preview.h"

LC_ActionModifyRotateTwice::LC_ActionModifyRotateTwice(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("ActionModifyRotateTwice",actionContext, RS2::ActionModifyRotateTwice)
    ,m_actionData(new RS_Rotate2Data()){
}

// fixme - The logic of Rotate2 action should be deeply reviewed... this is very old implementation, and for
// non-mirrored angles it delivery really uncontrolled and messy results.. So the generic purpose of this action
// is cumbersome. While it may by used for creation of entities moved over the circle and spiral, the overall
// practical use of the action seems to be not obvious...

LC_ActionModifyRotateTwice::~LC_ActionModifyRotateTwice() = default;

void LC_ActionModifyRotateTwice::doSaveOptions() {
    save("Angle1", getAngle1());
    save("Angle2", getAngle2());
    save("MirrorAngles", isMirrorAngles());
    save("SameAngleForCopies", isUseSameAngle2ForCopies());

    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());
}

void LC_ActionModifyRotateTwice::doLoadOptions() {
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

    const bool sameAngle = loadBool("SameAngleForCopies", false);
    setUseSameAngle2ForCopies(sameAngle);

    const bool mirrorAngles = loadBool("MirrorAngles", false);
    setMirrorAngles(mirrorAngles);

    const double angle2 = loadDouble("Angle2", 0.0);
    setAngle1(angle2);

    const double angle1 = loadDouble("Angle1", 0.0);
    setAngle1(angle1);
}

bool LC_ActionModifyRotateTwice::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint1) || (status == SetReferencePoint2);
}

void LC_ActionModifyRotateTwice::init(const int status) {
    LC_ActionModifyBase::init(status);
}

bool LC_ActionModifyRotateTwice::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    ctx.setActiveLayerAndPen(m_actionData->useCurrentLayer, m_actionData->useCurrentAttributes);
    return RS_Modification::rotate2(*m_actionData, m_selectedEntities,false, ctx);
}

void LC_ActionModifyRotateTwice::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (m_actionData->keepOriginals) {
        unselect(m_selectedEntities);
    }
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyRotateTwice::doTriggerCompletion([[maybe_unused]]bool success) {
    finish();
}

bool LC_ActionModifyRotateTwice::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngle1(angleRad);
        return true;
    }
    if (tag == "angle2") {
        setAngle2(angleRad);
        return true;
    }
    return false;
}

void LC_ActionModifyRotateTwice::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetReferencePoint2: {
            if (m_actionData->center1.valid){
                mouse = getSnapAngleAwarePoint(e, m_actionData->center1, mouse, true);
                m_actionData->center2 = mouse;
                LC_DocumentModificationBatch ctx;
                RS_Modification::rotate2(*m_actionData, m_selectedEntities, true, ctx);
                previewEntitiesToAdd(ctx);

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->center1);
                    previewRefLine(m_actionData->center1, mouse);
                    previewRefPointsForMultipleCopies(mouse);
                }

                if (isInfoCursorForModificationEnabled()){
                    msg(tr("Rotating Twice"))
                        .vector(tr("Center 1:"), m_actionData->center1)
                        .wcsAngle(tr("Angle 1:"), m_actionData->angle1)
                        .vector(tr("Center 2:"), m_actionData->center2)
                        .wcsAngle(tr("Angle 2:"), m_actionData->angle2)
                        .toInfoCursorZone2(false);
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyRotateTwice::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetReferencePoint2: {
            snap = getSnapAngleAwarePoint(e, m_actionData->center1, snap, false);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionModifyRotateTwice::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    switch (status) {
        case SetReferencePoint2: {
            setStatus(SetReferencePoint1);
            break;
        }
        case SetReferencePoint1: {
            m_selectionComplete = false;
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyRotateTwice::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint1: {
            m_actionData->center1 = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            setStatus(SetReferencePoint2);
            break;
        }
        case SetReferencePoint2: {
            addSnappedPointToVisualSnap(pos);
            m_actionData->center2 = pos;
            doPerformTrigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyRotateTwice::doPerformTrigger() {
    trigger();
}

void LC_ActionModifyRotateTwice::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select for two axis rotation") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Rotate 2 Axis immediately after selection")));
}

void LC_ActionModifyRotateTwice::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint1:
            updatePromptTRCancel(tr("Specify absolute reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetReferencePoint2:
            updatePromptTRBack(tr("Specify relative reference point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionModifyRotateTwice::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyRotateTwice::previewRefPointsForMultipleCopies( [[maybe_unused]]const RS_Vector &mouse) const {
    int numPoints = m_actionData->number;
    if (!m_actionData->multipleCopies) {
        numPoints = 1;
    }
    for (int i = 1; i <= numPoints; i++) {
        const double angle1ForCopy = /*m_actionData->sameAngle1ForCopies ?  m_actionData->angle1 :*/ m_actionData->angle1 * i;
        const double angle2ForCopy = m_actionData->sameAngle2ForCopies ? m_actionData->angle2 : m_actionData->angle2 * i;
        RS_Vector center2 = m_actionData->center2;
        center2.rotate(m_actionData->center1, angle1ForCopy);

        const double angleSum = angle1ForCopy + angle2ForCopy;
        previewSnapAngleMark(center2, angleSum);
        if (i == 1) {
            previewRefLine(m_actionData->center1, center2);
            previewRefPoint(center2);
        }
    }
}

LC_ActionOptionsWidget *LC_ActionModifyRotateTwice::createOptionsWidget() {
    return new LC_Rotate2OptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyRotateTwice::createOptionsFiller() {
    return new LC_Rotate2OptionsFiller();
}

void LC_ActionModifyRotateTwice::setAngle2(const double angleRad) const {
    m_actionData->angle2 = toWorldAngleFromUCSBasis(angleRad);
}

void LC_ActionModifyRotateTwice::setAngle1(const double angleRad) const {
    m_actionData->angle1 = toWorldAngleFromUCSBasis(angleRad);
}

double LC_ActionModifyRotateTwice::getAngle1() const {
    return toUCSBasisAngle(m_actionData->angle1);
}

double LC_ActionModifyRotateTwice::getAngle2() const {
    return toUCSBasisAngle(m_actionData->angle2);
}

void LC_ActionModifyRotateTwice::setUseSameAngle2ForCopies(const bool b) const {
    m_actionData->sameAngle2ForCopies = b;
}

bool LC_ActionModifyRotateTwice::isUseSameAngle2ForCopies() const {
    return m_actionData->sameAngle2ForCopies;
}

void LC_ActionModifyRotateTwice::setMirrorAngles(const bool b) const {
    m_actionData->mirrorAngles = b;
}

bool LC_ActionModifyRotateTwice::isMirrorAngles() const {
    return m_actionData->mirrorAngles;
}

LC_ModifyOperationFlags *LC_ActionModifyRotateTwice::getModifyOperationFlags() {
    return m_actionData.get();
}

bool LC_ActionModifyRotateTwice::isAllowTriggerOnEmptySelection() {
    return false;
}
