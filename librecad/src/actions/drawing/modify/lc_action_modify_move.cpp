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

#include "lc_action_modify_move.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_move_options_filler.h"
#include "lc_move_options_widget.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct LC_ActionModifyMove::MoveActionData {
    RS_MoveData data;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    bool createCopy{false};
};

LC_ActionModifyMove::LC_ActionModifyMove(LC_ActionContext* actionContext)
    : LC_ActionModifyBase("ActionModifyMove", actionContext, RS2::ActionModifyMove), m_actionData(std::make_unique<MoveActionData>()) {
}

LC_ActionModifyMove::~LC_ActionModifyMove() = default;

void LC_ActionModifyMove::doSaveOptions() {
    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());
}

void LC_ActionModifyMove::doLoadOptions() {
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
}

bool LC_ActionModifyMove::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint) || (status == SetTargetPoint);
}

bool LC_ActionModifyMove::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    auto& moveData = m_actionData->data;
    if (m_actionData->createCopy) {
        const bool oldKeepOriginals = moveData.keepOriginals;
        moveData.keepOriginals = true;
        RS_Modification::move(moveData, m_selectedEntities, false, ctx);
        moveData.keepOriginals = oldKeepOriginals;
    }
    else {
        RS_Modification::move(moveData, m_selectedEntities, false, ctx);
        moveRelativeZero(m_actionData->targetPoint);
    }
    ctx.setActiveLayerAndPen(moveData.useCurrentLayer, moveData.useCurrentAttributes);
    return true;
}

void LC_ActionModifyMove::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        if (m_actionData->createCopy || m_actionData->data.keepOriginals) {
            unselect(m_selectedEntities);
        }
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyMove::doTriggerCompletion([[maybe_unused]] bool success) {
    finish();
}

void LC_ActionModifyMove::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->referencePoint.valid) {
                mouse = getSnapAngleAwarePoint(e, m_actionData->referencePoint, mouse, true);
                m_actionData->targetPoint = mouse;

                const RS_Vector& offset = m_actionData->targetPoint - m_actionData->referencePoint;
                m_actionData->data.offset = offset;

                LC_DocumentModificationBatch ctx;
                RS_Modification::move(m_actionData->data, m_selectedEntities, true, ctx);
                m_preview->addAllFromList(ctx.entitiesToAdd);

                if (e->isShift) {
                    previewLine(m_actionData->referencePoint, mouse);
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewRefPoint(m_actionData->referencePoint);
                    previewRefLine(m_actionData->referencePoint, mouse);

                    if (m_actionData->data.multipleCopies) {
                        const int numCopies = m_actionData->data.number;
                        if (numCopies > 1) {
                            for (int i = 2; i <= numCopies; i++) {
                                previewRefPoint(m_actionData->referencePoint + offset * i);
                            }
                        }
                    }
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(e->isControl ? tr("Copy Offset") : tr("Moving Offset")).relative(offset).relativePolar(offset).
                                                                                toInfoCursorZone2(false);
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

void LC_ActionModifyMove::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    if (status == SetTargetPoint) {
        snapped = getSnapAngleAwarePoint(e, m_actionData->referencePoint, snapped);
        m_actionData->createCopy = e->isControl;
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionModifyMove::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    if (status == SetReferencePoint) {
        if (m_selectionComplete) {
            m_selectionComplete = false;
        }
        else {
            initPrevious(status);
        }
    }
    else {
        initPrevious(status);
    }
}

void LC_ActionModifyMove::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    if (!m_selectionComplete) {
        return;
    }
    switch (status) {
        case SetReferencePoint: {
            m_actionData->referencePoint = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(m_actionData->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = pos;
            addSnappedPointToVisualSnap(pos);
            m_actionData->data.offset = m_actionData->targetPoint - m_actionData->referencePoint;
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyMove::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            updatePromptTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updatePromptTRBack(tr("Specify target point"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Create a Copy")));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyMove::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to move") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Move immediately after selection")));
}

RS2::CursorType LC_ActionModifyMove::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionModifyMove::createOptionsWidget() {
    return new LC_MoveOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyMove::createOptionsFiller() {
    return new LC_MoveOptionsFiller();
}

LC_ModifyOperationFlags* LC_ActionModifyMove::getModifyOperationFlags() {
    return &m_actionData->data;
}
