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

#include "rs_actionmodifymove.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_moveoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyMove::MoveActionData {
	RS_MoveData data;
	RS_Vector referencePoint;
	RS_Vector targetPoint;
    bool createCopy {false};
};

RS_ActionModifyMove::RS_ActionModifyMove(LC_ActionContext *actionContext)
        :LC_ActionModifyBase("Move Entities", actionContext, RS2::ActionModifyMove)
        ,m_actionData(std::make_unique<MoveActionData>()){
}

RS_ActionModifyMove::~RS_ActionModifyMove() = default;

void RS_ActionModifyMove::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyMove::trigger()");
    RS_Modification m(*m_container, m_viewport);

    if (m_actionData->createCopy) {
        bool oldKeepOriginals = m_actionData->data.keepOriginals;
        m_actionData->data.keepOriginals = true;
        m.move(m_actionData->data, m_selectedEntities, false, keepSelected);
        m_actionData->data.keepOriginals = oldKeepOriginals;
    }
    else {
        m.move(m_actionData->data, m_selectedEntities, false, keepSelected);
        finish(false);
        moveRelativeZero(m_actionData->targetPoint);
    }
}

void RS_ActionModifyMove::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->referencePoint.valid){

                mouse = getSnapAngleAwarePoint(e, m_actionData->referencePoint, mouse, true);
                m_actionData->targetPoint = mouse;

                const RS_Vector &offset = m_actionData->targetPoint - m_actionData->referencePoint;
                m_actionData->data.offset = offset;

                RS_Modification m(*m_preview, m_viewport, false);
                m.move(m_actionData->data, m_selectedEntities, true, false);

                if (e->isShift){
                    previewLine(m_actionData->referencePoint, mouse);
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewRefPoint(m_actionData->referencePoint);
                    previewRefLine(m_actionData->referencePoint, mouse);

                    if (m_actionData->data.multipleCopies) {
                        int numCopies = m_actionData->data.number;
                        if (numCopies > 1) {
                            for (int i = 2; i <= numCopies; i++) {
                                previewRefPoint(m_actionData->referencePoint + offset * i);
                            }
                        }
                    }
                }
                if (isInfoCursorForModificationEnabled()){
                    msg(e->isControl ? tr("Copy Offset") : tr("Moving Offset"))
                        .relative(offset)
                        .relativePolar(offset)
                        .toInfoCursorZone2(false);
                }
            }
            break;
        }
        case ShowDialog:
            break;
        default:
            break;
    }
}

void RS_ActionModifyMove::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    if (status == SetTargetPoint){
        snapped = getSnapAngleAwarePoint(e, m_actionData->referencePoint, snapped);
        m_actionData->createCopy = e->isControl;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionModifyMove::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    if (status == SetReferencePoint){
        if (m_selectionComplete) {
            m_selectionComplete = false;
        }
        else{
            initPrevious(status);
        }
    }
    else{
        initPrevious(status);
    }
}

void RS_ActionModifyMove::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    if (!m_selectionComplete){
        return;
    }
    switch (status) {
        case SetReferencePoint: {
            m_actionData->referencePoint = pos;
            moveRelativeZero(m_actionData->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = pos;
            if (isShowModifyActionDialog()){
                setStatus(ShowDialog); // todo - hm... what for?
                if (RS_DIALOGFACTORY->requestMoveDialog(m_actionData->data)){
                    updateOptions();
                    m_actionData->data.offset = m_actionData->targetPoint - m_actionData->referencePoint;
                    trigger();
                    moveRelativeZero(m_actionData->targetPoint);
                }
                else{
                    setStatus(SetTargetPoint);
                }
            }
            else{
                m_actionData->data.offset = m_actionData->targetPoint - m_actionData->referencePoint;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMove::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Create a Copy")));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyMove::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to move (Enter to complete)"),
        MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Move immediately after selection")));
}

RS2::CursorType RS_ActionModifyMove::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionModifyMove::createOptionsWidget() {
    return new LC_MoveOptions();
}

LC_ModifyOperationFlags *RS_ActionModifyMove::getModifyOperationFlags() {
    return &m_actionData->data;
}
