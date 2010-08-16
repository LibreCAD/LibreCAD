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

#include "rs_actionzoomin.h"
//Added by qt3to4:
#include <q3mimefactory.h>


/**
 * Default constructor.
 *
 * @param direction In for zooming in, Out for zooming out.
 * @param axis Axis that are affected by the zoom (OnlyX, OnlyY or Both)
 */
RS_ActionZoomIn::RS_ActionZoomIn(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView,
                                 RS2::ZoomDirection direction,
                                 RS2::Axis axis,
                                 const RS_Vector& center)
        :RS_ActionInterface("Zoom in", container, graphicView) {

    this->direction = direction;
    this->axis = axis;
    this->center = center;
}

QAction* RS_ActionZoomIn::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;

    if (type==RS2::ActionZoomIn) {
/* RVT_PORT        action = new QAction(tr("Zoom in"), qPixmapFromMimeSource("zoomin.png"),
                             tr("Zoom &In"), QKeySequence(), NULL); */
        action = new QAction( qPixmapFromMimeSource("zoomin.png"),tr("Zoom in"),NULL);
        action->setStatusTip(tr("Zooms in"));
    } else {
  /* RVT_PORT       action = new QAction(tr("Zoom out"), qPixmapFromMimeSource("zoomout.png"),
                             tr("Zoom &Out"), QKeySequence(), NULL); */
        action = new QAction( qPixmapFromMimeSource("zoomout.png"),tr("Zoom out"),NULL);
        action->setStatusTip(tr("Zooms out"));
    }
    return action;
}


void RS_ActionZoomIn::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionZoomIn::trigger() {
    switch (axis) {
    case RS2::OnlyX:
        if (direction==RS2::In) {
            graphicView->zoomInX();
        } else {
            graphicView->zoomOutX();
        }
        break;

    case RS2::OnlyY:
        if (direction==RS2::In) {
            graphicView->zoomInY();
        } else {
            graphicView->zoomOutY();
        }
        break;

    case RS2::Both:
        if (direction==RS2::In) {
            graphicView->zoomIn(1.25, center);
        } else {
            graphicView->zoomOut(1.25, center);
        }
        break;
    }
    finish();
}




// EOF
