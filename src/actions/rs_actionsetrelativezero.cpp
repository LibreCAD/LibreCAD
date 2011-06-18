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

#include "rs_actionsetrelativezero.h"

#include "rs_snapper.h"
#include "rs_point.h"



RS_ActionSetRelativeZero::RS_ActionSetRelativeZero(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Set the relative Zero",
                           container, graphicView) {}


QAction* RS_ActionSetRelativeZero::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Set Relative Zero"), tr("&Set Relative Zero"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Set Relative Zero"), NULL);
    //action->zetStatusTip(tr("Set position of the Relative Zero point"));
	action->setIcon(QIcon(":/extui/relzeromove.png"));
    return action;
}


void RS_ActionSetRelativeZero::trigger() {
    bool wasLocked = graphicView->isRelativeZeroLocked();
    if (pt.valid) {
        graphicView->lockRelativeZero(false);
        graphicView->moveRelativeZero(pt);
        graphicView->lockRelativeZero(wasLocked);
    }
    finish();
}



void RS_ActionSetRelativeZero::mouseMoveEvent(RS_MouseEvent* e) {
    snapPoint(e);
}



void RS_ActionSetRelativeZero::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
}



void RS_ActionSetRelativeZero::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    pt = e->getCoordinate();
    trigger();
}



void RS_ActionSetRelativeZero::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Set relative Zero"), tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionSetRelativeZero::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionSetRelativeZero::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    }
}

// EOF
