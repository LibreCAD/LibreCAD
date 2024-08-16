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

#include <QMouseEvent>

#include "rs_actionmodifydeletequick.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"


RS_ActionModifyDeleteQuick::RS_ActionModifyDeleteQuick(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Quick Delete Entities",
					container, graphicView)
		,en(nullptr){
	actionType=RS2::ActionModifyDeleteQuick;
}

/**
 * Deletes all entities that were selected.
 */
void RS_ActionModifyDeleteQuick::trigger() {

    RS_DEBUG->print("RS_ActionModifyDeleteQuick::trigger()");

    if (en) {
        RS_DEBUG->print("Entity found");
        RS_EntityContainer* parent = en->getParent();
        if (parent) {
            en->setSelected(false);
            graphicView->deleteEntity(en);
            en->changeUndoState();

            if (document) {
                document->startUndoCycle();
                document->addUndoable(en);
                document->endUndoCycle();
            }
        }

        updateSelectionWidget();
    } else {
        RS_DEBUG->print("RS_ActionModifyDeleteQuick::mousePressEvent:"
                        " Entity is NULL\n");
    }
}

void RS_ActionModifyDeleteQuick::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    en = catchEntity(e);
    trigger();
}

void RS_ActionModifyDeleteQuick::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionModifyDeleteQuick::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        updateMouseWidgetTRCancel(tr("Pick entity to delete"));
        break;
    default:
        updateMouseWidget();
        break;
    }
}

RS2::CursorType RS_ActionModifyDeleteQuick::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::DelCursor;
}
