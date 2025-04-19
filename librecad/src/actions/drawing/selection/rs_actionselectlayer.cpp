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

#include "rs_actionselectlayer.h"

#include "rs_debug.h"
#include "rs_selection.h"

RS_ActionSelectLayer::RS_ActionSelectLayer(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Select Layers", actionContext,RS2::ActionSelectLayer)
    , m_entity(nullptr){
}

void RS_ActionSelectLayer::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *event) {
    deleteSnapper();
    auto ent = catchAndDescribe(event);
    if (ent != nullptr){
        highlightHover(ent);
    }
}

void RS_ActionSelectLayer::doTrigger() {
    if (m_entity){
        RS_Selection s(*m_container, m_viewport);
        s.selectLayer(m_entity);
    } else {
        RS_DEBUG->print("RS_ActionSelectLayer::trigger: Entity is NULL\n");
    }
}

void RS_ActionSelectLayer::onMouseLeftButtonRelease([[maybe_unused]] int status, LC_MouseEvent *e) {
    m_entity = catchEntityByEvent(e);
    trigger();
    invalidateSnapSpot();
}

void RS_ActionSelectLayer::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    initPrevious(status);
}

RS2::CursorType RS_ActionSelectLayer::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionSelectLayer::updateMouseButtonHints() {
   updateMouseWidgetTRCancel(tr("Specify entity with desired layer"));
}
