/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actioninfoinside.h"
#include "rs_information.h"
#include "rs_snapper.h"



RS_ActionInfoInside::RS_ActionInfoInside(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Info Inside",
                    container, graphicView) {

    contour = new RS_EntityContainer(NULL, false);

    for (RS_Entity* e=container.firstEntity(); e!=NULL;
            e=container.nextEntity()) {
        if (e->isSelected()) {
            contour->addEntity(e);
        }
    }
}


RS_ActionInfoInside::~RS_ActionInfoInside() {
    delete contour;
}


QAction* RS_ActionInfoInside::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Point inside contour"),
                                  tr("&Point inside contour"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Point inside contour"), NULL);
    action->setStatusTip(tr("Checks if a given point is inside the "
                            "selected contour"));

    return action;
}

void RS_ActionInfoInside::trigger() {
    bool onContour = false;
    if (RS_Information::isPointInsideContour(pt, contour, &onContour)) {
        RS_DIALOGFACTORY->commandMessage(tr("Point is inside selected contour."));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("Point is outside selected contour."));
    }
    finish();
}



void RS_ActionInfoInside::mouseMoveEvent(RS_MouseEvent* e) {
    //RS_Vector mouse = snapPoint(e);
    //bool onContour = false;
    /*if (RS_Information::isPointInsideContour(mouse, contour, &onContour)) {
    } else {
    }*/
}



void RS_ActionInfoInside::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    } else {
        pt = snapPoint(e);
        trigger();
    }
}



void RS_ActionInfoInside::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionInfoInside::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionInfoInside::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarInfo);
    }
}

// EOF
