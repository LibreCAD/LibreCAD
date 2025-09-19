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

#include "rs_actionselectcontour.h"

#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_selection.h"

RS_ActionSelectContour::RS_ActionSelectContour(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Select Contours", actionContext, RS2::ActionSelectContour)
	,m_entity(nullptr){
}

void RS_ActionSelectContour::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (contextEntity->isAtomic()) {
        m_entity = contextEntity;
        trigger();
        redrawDrawing();
    }
}

void RS_ActionSelectContour::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *event) {
    auto ent = catchAndDescribe(event);
    if (ent != nullptr){
        // fixme - proper highlighting of planned selection - yet after fixing underlying logic!
//        RS_Selection s(*container, graphicView);
//        s.selectContour(en);
        // fixme - temporarily highlight only caught entity only
        highlightHover(ent);
    }
}

void RS_ActionSelectContour::doTrigger() {
    if (m_entity != nullptr) {
        if (m_entity->isAtomic()){ // fixme - why it is so??? why it's not suitable to select, say, polyline here too?
            RS_Selection s(*m_container, m_viewport);
            s.selectContour(m_entity);
        } else
           commandMessage(tr("Entity must be an Atomic Entity."));
    } else
        RS_DEBUG->print("RS_ActionSelectContour::trigger: Entity is NULL\n");
}

void RS_ActionSelectContour::onMouseLeftButtonRelease([[maybe_unused]] int status, LC_MouseEvent *e) {
    m_entity = catchEntityByEvent(e);
    trigger();
}

void RS_ActionSelectContour::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    initPrevious(status);
}

RS2::CursorType RS_ActionSelectContour::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionSelectContour::updateMouseButtonHints() {
     updateMouseWidgetTRCancel(tr("Specify entity to select"));
}
