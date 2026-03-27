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

#include "lc_action_modify_order.h"

#include "rs_debug.h"
#include "rs_document.h"
#include "rs_entity.h"

LC_ActionOrder::LC_ActionOrder(LC_ActionContext *actionContext, const RS2::ActionType type)
    :LC_ActionPreSelectionAwareBase("ActionOrder", actionContext, type) {
    // todo - type may be one of the following, update name if needed
    // RS2::ActionOrderBottom
    // RS2::ActionOrderLower
    // RS2::ActionOrderRaise
    // RS2::ActionOrderTop
}

void LC_ActionOrder::drawSnapper() {
    // delete snapper
}

void LC_ActionOrder::onSelectionCompleted([[maybe_unused]]bool singleEntity, const bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateActionPrompt();
    if (m_actionType == RS2::ActionOrderBottom || m_actionType ==  RS2::ActionOrderTop){
        trigger();
    }
    else {
        m_snapMode.restriction = RS2::RestrictNothing;
    }
}

void LC_ActionOrder::doTrigger() {
    // fixme - sand - review SELECTION STATE
    RS_DEBUG->print("RS_ActionOrder::trigger()");

    QList<RS_Entity*> entList;
    for (const auto e : m_selectedEntities) {
        entList.append(e);
    }

    if (m_targetEntity != nullptr) {
        m_targetEntity->setHighlighted(false);

        switch (m_actionType) {
            case RS2::ActionOrderLower: {
                const int index = m_document->findEntity(m_targetEntity);
                m_document->moveEntity(index, entList);
                break;
            }
            case RS2::ActionOrderRaise: {
                const int index = m_document->findEntity(m_targetEntity) + 1;
                m_document->moveEntity(index, entList);
                break;
            }
            default:
                break;
        }
        m_targetEntity = nullptr;
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
    const bool keepSelected = isKeepModifiedEntitiesSelected();
    if (!keepSelected) {
        deselectAll();
    }
    setStatus(getStatus() - 1);
}

void LC_ActionOrder::onMouseMoveEventSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    m_targetEntity = catchEntityByEvent(e);
    if (m_targetEntity != nullptr){
        highlightHover(m_targetEntity);
    }
}

void LC_ActionOrder::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    m_targetEntity = catchEntityByEvent(e);
    if (m_targetEntity == nullptr) {
        commandMessage(tr("No Entity found."));
    } else {
        trigger();
    }
}

void LC_ActionOrder::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
     deletePreview();
     if (m_selectionComplete) {
         m_selectionComplete = false;
     }
     else{
         initPrevious(status);
     }
}

void LC_ActionOrder::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Choose entities") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Order immediately after selection")));
}

void LC_ActionOrder::updateActionPromptForSelected([[maybe_unused]]int status) {
    updatePromptTRCancel(tr("Choose entity for order"));
}

RS2::CursorType LC_ActionOrder::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::SelectCursor;
}
