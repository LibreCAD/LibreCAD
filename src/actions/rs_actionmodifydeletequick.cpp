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

#include "rs_actionmodifydeletequick.h"

#include "rs_actionselectsingle.h"
#include "rs_snapper.h"
#include "rs_point.h"



RS_ActionModifyDeleteQuick::RS_ActionModifyDeleteQuick(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Quick Delete Entities",
                    container, graphicView) {

    en = NULL;
}


QAction* RS_ActionModifyDeleteQuick::createGUIAction(RS2::ActionType /*type*/, QObject* parent) {
	// (tr("Delete selected"
	QAction* action = new QAction(tr("&Delete selected"), parent);
	action->setIcon(QIcon(":/extui/modifydelete.png"));
	//action->zetStatusTip(tr("Delete selected entities"));
	action->setShortcut(QKeySequence::Delete);
	return action;
}


/**
 * Deletes all entities that were selected.
 */
void RS_ActionModifyDeleteQuick::trigger() {

    RS_DEBUG->print("RS_ActionModifyDeleteQuick::trigger()");

    if (en!=NULL) {
        RS_DEBUG->print("Entity found");
        RS_EntityContainer* parent = en->getParent();
        if (parent!=NULL) {
            en->setSelected(false);
            graphicView->deleteEntity(en);
            en->changeUndoState();

            if (document) {
                document->startUndoCycle();
                document->addUndoable(en);
                document->endUndoCycle();
            }
        }

    	RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    } else {
        RS_DEBUG->print("RS_ActionModifyDeleteQuick::mousePressEvent:"
                        " Entity is NULL\n");
    }
}



void RS_ActionModifyDeleteQuick::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}



void RS_ActionModifyDeleteQuick::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Pick entity to delete"),
                                       tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyDeleteQuick::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::DelCursor);
}

// EOF
