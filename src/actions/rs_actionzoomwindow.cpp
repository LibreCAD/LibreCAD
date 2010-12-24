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

#include "rs_actionzoomwindow.h"

#include "rs.h"
#include "rs_snapper.h"
#include "rs_point.h"
//Added by qt3to4:
#include <q3mimefactory.h>


/**
 * Default constructor.
 *
 * @param keepAspectRatio Keep the aspect ratio. true: the factors 
 *          in x and y will stay the same. false Exactly the chosen 
 *          area will be fit to the viewport.
 */
RS_ActionZoomWindow::RS_ActionZoomWindow(RS_EntityContainer& container,
        RS_GraphicView& graphicView, bool keepAspectRatio)
        : RS_PreviewActionInterface("Zoom Window",
                            container, graphicView) {

    this->keepAspectRatio = keepAspectRatio;
}


QAction* RS_ActionZoomWindow::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Window Zoom")
	QAction* action = new QAction(tr("&Window Zoom"), NULL);
	action->setIcon(QIcon(":/actions/zoomwindow.png"));
	//action->zetStatusTip(tr("Zooms in a window"));
	
    return action;
}


void RS_ActionZoomWindow::init(int status) {
    RS_DEBUG->print("RS_ActionZoomWindow::init()");

    RS_PreviewActionInterface::init(status);
    v1 = v2 = RS_Vector(false);
    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
}



void RS_ActionZoomWindow::trigger() {
    RS_DEBUG->print("RS_ActionZoomWindow::trigger()");

    RS_PreviewActionInterface::trigger();

    if (v1.valid && v2.valid) {
        deletePreview();
        if (graphicView->toGuiDX(v1.distanceTo(v2))>5) {
            graphicView->zoomWindow(v1, v2, keepAspectRatio);
            init();
        }
    }
}



void RS_ActionZoomWindow::mouseMoveEvent(RS_MouseEvent* e) {
    if (getStatus()==1 && v1.valid) {
        v2 = snapPoint(e);
        deletePreview();
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(RS_Vector(v1.x, v1.y),
                                                   RS_Vector(v2.x, v1.y))));
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(RS_Vector(v2.x, v1.y),
                                                   RS_Vector(v2.x, v2.y))));
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(RS_Vector(v2.x, v2.y),
                                                   RS_Vector(v1.x, v2.y))));
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(RS_Vector(v1.x, v2.y),
                                                   RS_Vector(v1.x, v1.y))));
        drawPreview();
    }
}



void RS_ActionZoomWindow::mousePressEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        switch (getStatus()) {
        case 0:
            v1 = snapPoint(e);
            setStatus(1);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionZoomWindow::mousePressEvent(): %f %f",
                    v1.x, v1.y);
}



void RS_ActionZoomWindow::mouseReleaseEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");

    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        if (getStatus()==1) {
            deletePreview();
        }
        init(getStatus()-1);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        if (getStatus()==1) {
            v2 = snapPoint(e);
            trigger();
        }
    }
}



void RS_ActionZoomWindow::updateMouseButtonHints() {
    RS_DEBUG->print("RS_ActionZoomWindow::updateMouseButtonHints()");

    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first edge"), tr("Cancel"));
        break;
    case 1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second edge"), tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionZoomWindow::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::MagnifierCursor);
}


// EOF
