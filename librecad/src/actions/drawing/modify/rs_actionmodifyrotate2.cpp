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

#include "rs_actionmodifyrotate2.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_rotate2options.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_modification.h"
#include "rs_preview.h"

RS_ActionModifyRotate2::RS_ActionModifyRotate2(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("Rotate Entities around two centers",actionContext, RS2::ActionModifyRotate2)
    ,m_actionData(new RS_Rotate2Data()){
}

// fixme - The logic of Rotate2 action should be deeply reviewed... this is very old implementation, and for
// non-mirrored angles it delivery really uncontrolled and messy results.. So the generic purpose of this action
// is cumbersome. While it may by used for creation of entities moved over the circle and spiral, the overall
// practical use of the action seems to be not obvious...

RS_ActionModifyRotate2::~RS_ActionModifyRotate2() = default;

void RS_ActionModifyRotate2::init(int status) {
    LC_ActionModifyBase::init(status);
}

void RS_ActionModifyRotate2::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyRotate2::trigger()");
    RS_Modification m(*m_container, m_viewport);
    m.rotate2(*m_actionData, m_selectedEntities,false, keepSelected);
    finish(false);
}

void RS_ActionModifyRotate2::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
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
                RS_Modification m(*m_preview, m_viewport, false);
                m.rotate2(*m_actionData, m_selectedEntities, true, false);

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

void RS_ActionModifyRotate2::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetReferencePoint2: {
            snap = getSnapAngleAwarePoint(e, m_actionData->center1, snap, false);
            break;
        }
    }
    fireCoordinateEvent(snap);
}

void RS_ActionModifyRotate2::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
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

void RS_ActionModifyRotate2::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint1: {
            m_actionData->center1 = pos;
            moveRelativeZero(pos);
            setStatus(SetReferencePoint2);
            break;
        }
        case SetReferencePoint2: {
            m_actionData->center2 = pos;
//            setStatus(ShowDialog);
            doPerformTrigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate2::doPerformTrigger() {
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestRotate2Dialog(*m_actionData)) {
            updateOptions();
            trigger();
        }
        else{
            setStatus(SetReferencePoint2);
        }
    }
    else{
        trigger();
    }
}

void RS_ActionModifyRotate2::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select for two axis rotation  (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Rotate 2 Axis immediately after selection")));
}

void RS_ActionModifyRotate2::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint1:
            updateMouseWidgetTRCancel(tr("Specify absolute reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetReferencePoint2:
            updateMouseWidgetTRBack(tr("Specify relative reference point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyRotate2::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void RS_ActionModifyRotate2::previewRefPointsForMultipleCopies( [[maybe_unused]]const RS_Vector &mouse) {
    int numPoints = m_actionData->number;
    if (!m_actionData->multipleCopies) {
        numPoints = 1;
    }
    for (int i = 1; i <= numPoints; i++) {
        double angle1ForCopy = /*m_actionData->sameAngle1ForCopies ?  m_actionData->angle1 :*/ m_actionData->angle1 * i;
        double angle2ForCopy = m_actionData->sameAngle2ForCopies ? m_actionData->angle2 : m_actionData->angle2 * i;
        RS_Vector center2 = m_actionData->center2;
        center2.rotate(m_actionData->center1, angle1ForCopy);

        double angleSum = angle1ForCopy + angle2ForCopy;
        previewSnapAngleMark(center2, angleSum);
        if (i == 1) {
            previewRefLine(m_actionData->center1, center2);
            previewRefPoint(center2);
        }
    }
}

LC_ActionOptionsWidget *RS_ActionModifyRotate2::createOptionsWidget() {
    return new LC_Rotate2Options();
}

void RS_ActionModifyRotate2::setAngle2(double d) {
    m_actionData->angle2 = toWorldAngleFromUCSBasis(d);
}

void RS_ActionModifyRotate2::setAngle1(double d) {
    m_actionData->angle1 = toWorldAngleFromUCSBasis(d);
}

double RS_ActionModifyRotate2::getAngle1() {
    return toUCSBasisAngle(m_actionData->angle1);
}

double RS_ActionModifyRotate2::getAngle2() {
    return toUCSBasisAngle(m_actionData->angle2);
}

void RS_ActionModifyRotate2::setUseSameAngle2ForCopies(bool b) {
    m_actionData->sameAngle2ForCopies = b;
}

bool RS_ActionModifyRotate2::isUseSameAngle2ForCopies() {
    return m_actionData->sameAngle2ForCopies;
}

void RS_ActionModifyRotate2::setMirrorAngles(bool b) {
    m_actionData->mirrorAngles = b;
}

bool RS_ActionModifyRotate2::isMirrorAngles() {
    return m_actionData->mirrorAngles;
}

LC_ModifyOperationFlags *RS_ActionModifyRotate2::getModifyOperationFlags() {
    return m_actionData.get();
}

bool RS_ActionModifyRotate2::isAllowTriggerOnEmptySelection() {
    return false;
}
