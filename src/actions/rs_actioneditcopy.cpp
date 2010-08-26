/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actioneditcopy.h"
#include "rs_modification.h"
//Added by qt3to4:
#include <q3mimefactory.h>

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditCopy::RS_ActionEditCopy(bool copy,
                                     RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Copy",
                    container, graphicView) {

    this->copy = copy;
}



RS_ActionEditCopy::~RS_ActionEditCopy() {}



QAction* RS_ActionEditCopy::createGUIAction(RS2::ActionType type, QObject* parent) {
    QAction* action;

    if (type==RS2::ActionEditCopy) {
		// tr("Copy")
		action = new QAction(tr("&Copy"), parent);
		action->setIcon(QIcon(":/actions/editcopy2.png"));
		action->setShortcut(QKeySequence::Copy);
		action->setStatusTip(tr("Copies entities to the clipboard"));
    } else {
		// tr("Cut")
		action = new QAction(tr("Cu&t"), parent);
		action->setIcon(QIcon(":/actions/editcut2.png"));
		action->setShortcut(QKeySequence::Cut);
		action->setStatusTip(tr("Cuts entities to the clipboard"));
    }
    return action;
}



void RS_ActionEditCopy::init(int status) {
    RS_ActionInterface::init(status);
    //trigger();
}



void RS_ActionEditCopy::trigger() {

    RS_Modification m(*container, graphicView);
    m.copy(referencePoint, !copy);

    //graphicView->redraw();
    finish();
    graphicView->killSelectActions();
    //init(getStatus()-1);
    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}


void RS_ActionEditCopy::mouseMoveEvent(RS_MouseEvent* e) {
    snapPoint(e);
}



void RS_ActionEditCopy::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionEditCopy::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    referencePoint = e->getCoordinate();
    trigger();
}



void RS_ActionEditCopy::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionEditCopy::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionEditCopy::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    }
}


// EOF
