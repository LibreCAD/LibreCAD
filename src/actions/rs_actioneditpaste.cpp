/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_actioneditpaste.h"

#include "rs_clipboard.h"
#include "rs_modification.h"
//Added by qt3to4:
#include <q3mimefactory.h>

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditPaste::RS_ActionEditPaste( RS_EntityContainer& container,
                                        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Edit Paste",
                           container, graphicView) {}



RS_ActionEditPaste::~RS_ActionEditPaste() {}


QAction* RS_ActionEditPaste::createGUIAction(RS2::ActionType /*type*/, QObject* parent) {
	// tr("Paste")
	QAction* action = new QAction(tr("&Paste"), parent);
	action->setIcon(QIcon(":/actions/editpaste2.png"));
	action->setShortcut(QKeySequence::Paste);
	//action->zetStatusTip(tr("Pastes the clipboard contents"));
	
	
    return action;
}

void RS_ActionEditPaste::init(int status) {
    RS_PreviewActionInterface::init(status);
    //trigger();
}



void RS_ActionEditPaste::trigger() {
    deletePreview();

    RS_Modification m(*container, graphicView);
    m.paste(RS_PasteData(targetPoint, 1.0, 0.0, false, ""));
    //std::cout << *RS_Clipboard::instance();

	graphicView->redraw(RS2::RedrawDrawing); 

    finish();
}


void RS_ActionEditPaste::mouseMoveEvent(RS_MouseEvent* e) {
    switch (getStatus()) {
    case SetTargetPoint:
        targetPoint = snapPoint(e);

        deletePreview();
        preview->addAllFrom(*RS_CLIPBOARD->getGraphic());
        preview->move(targetPoint);

		if (graphic!=NULL) {
			RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
			RS2::Unit targetUnit = graphic->getUnit();
			double f = RS_Units::convert(1.0, sourceUnit, targetUnit);
        	preview->scale(targetPoint, RS_Vector(f,f));
		}
        drawPreview();
        break;

    default:
        break;
    }
}



void RS_ActionEditPaste::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionEditPaste::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    targetPoint = e->getCoordinate();
    trigger();
}



void RS_ActionEditPaste::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Set reference point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionEditPaste::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionEditPaste::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    }
}


// EOF
