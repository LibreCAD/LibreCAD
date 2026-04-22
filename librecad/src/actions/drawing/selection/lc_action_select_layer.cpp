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

#include "lc_action_select_layer.h"

#include "rs_debug.h"
#include "rs_selection.h"

LC_ActionSelectLayer::LC_ActionSelectLayer(LC_ActionContext *actionContext)
    :RS_ActionSelectBase("ActionSelectLayer", actionContext,RS2::ActionSelectLayer) {
}

void LC_ActionSelectLayer::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* event) {
    deleteSnapper();
    const auto ent = catchAndDescribe(event);
    if (ent != nullptr){
        highlightHover(ent);
    }
}

void LC_ActionSelectLayer::doTrigger() {
    if (m_entity != nullptr){
        const RS_Selection s(m_document, m_viewport);
        s.selectLayer(m_entity);
    } else {
        RS_DEBUG->print("LC_ActionSelectLayer::trigger: Entity is NULL\n");
    }
}

void LC_ActionSelectLayer::selectionFinishedByKey([[maybe_unused]]QKeyEvent* e, [[maybe_unused]]bool escape) {
    finish();
}

void LC_ActionSelectLayer::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    m_entity = catchEntityByEvent(e);
    trigger();
    invalidateSnapSpot();
    if (e->isControl) {
        finish();
    }
}

void LC_ActionSelectLayer::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

RS2::CursorType LC_ActionSelectLayer::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void LC_ActionSelectLayer::updateActionPrompt() {
   updatePromptTRCancel(tr("Specify entity with desired layer") + " " + getSelectionCompletionHintMsg());
}
