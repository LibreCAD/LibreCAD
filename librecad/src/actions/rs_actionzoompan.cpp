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

#include "rs_actionzoompan.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_debug.h"

RS_ActionZoomPan::RS_ActionZoomPan(RS_EntityContainer& container,
                                   RS_GraphicView& graphicView)
        :RS_ActionInterface("Zoom Panning", container, graphicView) {}

void RS_ActionZoomPan::init(int status) {
    RS_ActionInterface::init(status);
    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
    //v1 = v2 = RS_Vector(false);
    x1 = y1 = x2 = y2 = -1;
    //graphicView->saveView();
    setStatus(SetPanStart);
    updateMouseButtonHints();
}



void RS_ActionZoomPan::trigger() {
    /*if (v1.valid && v2.valid) {
        graphicView->zoomPan(v2-v1);
        v1 = v2;
}*/
    if (getStatus()==SetPanning && (abs(x2-x1)>7 || abs(y2-y1)>7)) {
        graphicView->zoomPan(x2-x1, y2-y1);
        x1 = x2;
        y1 = y2;
    }
    if(getStatus()==SetPanEnd)
    {
        finish(false);
        graphicView->setPanning(false);
        graphicView->redraw();
    }
}



void RS_ActionZoomPan::mouseMoveEvent(QMouseEvent* e) {
    //v2 = snapPoint(e);
    x2 = e->x();
    y2 = e->y();
    //if (getStatus()==1 && graphicView->toGuiDX((v2-v1).magnitude())>10) {
    if (getStatus()==SetPanning ) {
            if (abs(x2-x1)>7 || abs(y2-y1)>7) {
        trigger();
            }
    }
}


void RS_ActionZoomPan::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::MiddleButton ||
            e->button()==Qt::LeftButton) {
        //v1 = snapPoint(e);
        x1 = e->x();
        y1 = e->y();
        setStatus(SetPanning);
        graphicView->setPanning(true);
    }
}



void RS_ActionZoomPan::mouseReleaseEvent(QMouseEvent* /*e*/) {
        setStatus(SetPanEnd);
        trigger();
    //RS_DEBUG->print("RS_ActionZoomPan::mousePressEvent(): %f %f", v1.x, v1.y);
}

void RS_ActionZoomPan::updateMouseButtonHints()
{
    switch (getStatus()) {
            case SetPanStart:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Click and drag to pan zoom"),
											tr("Cancel"));
                break;
            case SetPanning:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Zoom panning"),
											tr("Cancel"));
                break;
           default:
		RS_DIALOGFACTORY->updateMouseWidget();
    }
}


void RS_ActionZoomPan::updateMouseCursor() {
    switch (getStatus()){
    case SetPanStart:
        graphicView->setMouseCursor(RS2::OpenHandCursor);
        break;
    case SetPanning:
        graphicView->setMouseCursor(RS2::ClosedHandCursor);
        break;
    default:
        break;
    }
}


// EOF
