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

#include "rs_actionzoompan.h"
#include "rs_snapper.h"
#include "rs_point.h"
//Added by qt3to4:
#include <q3mimefactory.h>


RS_ActionZoomPan::RS_ActionZoomPan(RS_EntityContainer& container,
                                   RS_GraphicView& graphicView)
        :RS_ActionInterface("Zoom Pan", container, graphicView) {}


QAction* RS_ActionZoomPan::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Pan Zoom"), qPixmapFromMimeSource("zoompan.png"),
                                  tr("&Pan Zoom"), QKeySequence(), NULL); */
    QAction* action = new QAction(qPixmapFromMimeSource("zoompan.png"),tr("Pan Zoom"), NULL);
    action->setStatusTip(tr("Realtime Panning"));
    return action;
}


void RS_ActionZoomPan::init(int status) {
    RS_ActionInterface::init(status);
    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
    //v1 = v2 = RS_Vector(false);
    x1 = y1 = x2 = y2 = -1;
    //graphicView->saveView();
}



void RS_ActionZoomPan::trigger() {
    /*if (v1.valid && v2.valid) {
        graphicView->zoomPan(v2-v1);
        v1 = v2;
}*/
    if (x1>=0) {
        graphicView->zoomPan(x2-x1, y2-y1);
        x1 = x2;
        y1 = y2;
    }
}



void RS_ActionZoomPan::mouseMoveEvent(RS_MouseEvent* e) {
    //v2 = snapPoint(e);
    x2 = e->x();
    y2 = e->y();
    //if (getStatus()==1 && graphicView->toGuiDX((v2-v1).magnitude())>10) {
    if (getStatus()==1 && (abs(x2-x1)>7 || abs(y2-y1)>7)) {
        trigger();
    }
}



void RS_ActionZoomPan::mousePressEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::MidButton ||
            RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        //v1 = snapPoint(e);
        x1 = e->x();
        y1 = e->y();
        setStatus(1);
    }
}



void RS_ActionZoomPan::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::MidButton) {
        init(-1);
    } else {
        setStatus(0);
    }

    //RS_DEBUG->print("RS_ActionZoomPan::mousePressEvent(): %f %f", v1.x, v1.y);
}



void RS_ActionZoomPan::updateMouseCursor() {
#ifndef __APPLE__
    graphicView->setMouseCursor(RS2::SizeAllCursor);
#endif
}


// EOF
