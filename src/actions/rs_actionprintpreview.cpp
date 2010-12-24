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

#include "rs_actionprintpreview.h"
//Added by qt3to4:
#include <q3mimefactory.h>

/**
 * Constructor.
 */
RS_ActionPrintPreview::RS_ActionPrintPreview(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Print Preview",
                    container, graphicView) {
    showOptions();
}



RS_ActionPrintPreview::~RS_ActionPrintPreview() {
}


QAction* RS_ActionPrintPreview::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Print Preview")
	QAction* action = new QAction(tr("Print Pre&view"), NULL);
	action->setIcon(QIcon(":/actions/fileprintpreview.png"));
    //action->zetStatusTip(tr("Shows a preview of a print"));	
	return action;
}


void RS_ActionPrintPreview::init(int status) {
    RS_ActionInterface::init(status);
    showOptions();
}




void RS_ActionPrintPreview::trigger() {}



void RS_ActionPrintPreview::mouseMoveEvent(RS_MouseEvent* e) {
	switch (getStatus()) {
	case Moving:
		v2 = graphicView->toGraph(e->x(), e->y());
		if (graphic!=NULL) {
			RS_Vector pinsbase = graphic->getPaperInsertionBase();
			
			double scale = graphic->getPaperScale();
			
			graphic->setPaperInsertionBase(pinsbase-v2*scale+v1*scale);
		}
		v1 = v2;
			graphicView->redraw(RS2::RedrawGrid); // DRAW Grid also draws paper, background items
		break;

	default:
		break;
	}
}



void RS_ActionPrintPreview::mousePressEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        switch (getStatus()) {
        case Neutral:
            v1 = graphicView->toGraph(e->x(), e->y());
            setStatus(Moving);
            break;

        default:
            break;
        }
    }
}


void RS_ActionPrintPreview::mouseReleaseEvent(RS_MouseEvent* e) {
	switch (getStatus()) {
	case Moving:
		setStatus(Neutral);
		break;
		
    default:
        RS_DIALOGFACTORY->requestPreviousMenu();
        e->accept();
        break;
	}
}



void RS_ActionPrintPreview::coordinateEvent(RS_CoordinateEvent* ) {}



void RS_ActionPrintPreview::commandEvent(RS_CommandEvent* ) {}



RS_StringList RS_ActionPrintPreview::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}


void RS_ActionPrintPreview::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionPrintPreview::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionPrintPreview::updateMouseButtonHints() {}



void RS_ActionPrintPreview::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::MovingHandCursor);
}



void RS_ActionPrintPreview::updateToolBar() {}


void RS_ActionPrintPreview::center() {
    if (graphic!=NULL) {
        graphic->centerToPage();
        graphicView->redraw();
    }
}


void RS_ActionPrintPreview::fit() {
    if (graphic!=NULL) {
        graphic->fitToPage();
        graphicView->redraw();
    }
}


void RS_ActionPrintPreview::setScale(double f) {
    if (graphic!=NULL) {
		graphic->setPaperScale(f);
    	graphicView->redraw();
	}
}



double RS_ActionPrintPreview::getScale() {
	double ret = 1.0;
    if (graphic!=NULL) {
		ret = graphic->getPaperScale();
	}
	return ret;
}



void RS_ActionPrintPreview::setBlackWhite(bool bw) {
    if (bw) {
        graphicView->setDrawingMode(RS2::ModeBW);
    }
	else {
        graphicView->setDrawingMode(RS2::ModeFull);
	}
	graphicView->redraw();
}


RS2::Unit RS_ActionPrintPreview::getUnit() {
    if (graphic!=NULL) {
		return graphic->getUnit();
	}
	else {
		return RS2::None;
	}
}


// EOF
