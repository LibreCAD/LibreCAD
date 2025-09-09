/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2012 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actionorder.h"

#include "rs_debug.h"
#include "rs_document.h"
#include "rs_entity.h"

RS_ActionOrder::RS_ActionOrder(LC_ActionContext *actionContext, RS2::ActionType type)
    :LC_ActionPreSelectionAwareBase("Sort Entities", actionContext, type) ,targetEntity(nullptr){
}

void RS_ActionOrder::drawSnapper() {
    // delete snapper
}

void RS_ActionOrder::onSelectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    updateSelectionWidget();
    if (m_actionType == RS2::ActionOrderBottom || m_actionType ==  RS2::ActionOrderTop){
        trigger();
    }
    else {
        m_snapMode.restriction = RS2::RestrictNothing;
    }
}

void RS_ActionOrder::doTrigger(bool keepSelected) {
    // fixme - sand - review SELECTION STATE
    RS_DEBUG->print("RS_ActionOrder::trigger()");

    QList<RS_Entity*> entList;
    for (auto e : m_selectedEntities) {
        entList.append(e);
    }

    if (targetEntity != nullptr) {
        int index = -1;
        targetEntity->setHighlighted(false);

        switch (m_actionType) {
            case RS2::ActionOrderLower:
                index = m_document->findEntity(targetEntity);
                m_document->moveEntity(index, entList);
                break;
            case RS2::ActionOrderRaise:
                index = m_document->findEntity(targetEntity) + 1;
                m_document->moveEntity(index, entList);
                break;
            default:
                break;
        }
        targetEntity = nullptr;
    }
    else {
        switch (m_actionType) {
            case RS2::ActionOrderBottom:
                m_document->moveEntity(-1, entList);
                break;
            case RS2::ActionOrderTop:
                m_document->moveEntity(m_document->count() + 1, entList);
                break;
            default:
                break;
        }
    }
    // todo - sand - override mode with ctrl?
    if (!keepSelected) {
        deselectAll();
    }
    setStatus(getStatus() - 1);
}

void RS_ActionOrder::onMouseMoveEventSelected([[maybe_unused]]int status, LC_MouseEvent *e) {
    targetEntity = catchEntityByEvent(e);
    if (targetEntity != nullptr){
        highlightHover(targetEntity);
    }
}

void RS_ActionOrder::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, LC_MouseEvent *e) {
    targetEntity = catchEntityByEvent(e);
    if (targetEntity == nullptr) {
        commandMessage(tr("No Entity found."));
    } else {
        trigger();
    }
}

void RS_ActionOrder::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
     deletePreview();
     if (m_selectionComplete) {
         m_selectionComplete = false;
     }
     else{
         initPrevious(status);
     }
}

void RS_ActionOrder::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Choose entities (Enter to Complete)"), MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Order immediately after selection")));
}

void RS_ActionOrder::updateMouseButtonHintsForSelected([[maybe_unused]]int status) {
    updateMouseWidgetTRCancel(tr("Choose entity for order"));
}

RS2::CursorType RS_ActionOrder::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::SelectCursor;
}
