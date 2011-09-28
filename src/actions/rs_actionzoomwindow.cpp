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

#include "rs_actionzoomwindow.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"


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
#if QT_VERSION >= 0x040600
        action->setIcon(QIcon::fromTheme("zoom-select", QIcon(":/actions/zoomwindow.png")));
#else
        action->setIcon(QIcon(":/actions/zoomwindow.png"));
#endif
        //action->zetStatusTip(tr("Zooms in a window"));

    return action;
}


void RS_ActionZoomWindow::init(int status) {
    RS_DEBUG->print("RS_ActionZoomWindow::init()");

    RS_PreviewActionInterface::init(status);
    v1 = v2 = RS_Vector(false);
   // snapMode.clear();
   // snapMode.restriction = RS2::RestrictNothing;
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



void RS_ActionZoomWindow::mouseMoveEvent(QMouseEvent* e) {
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



void RS_ActionZoomWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetFirstCorner:
            v1 = snapFree(e);
            setStatus(SetSecondCorner);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionZoomWindow::mousePressEvent(): %f %f",
                    v1.x, v1.y);
}



void RS_ActionZoomWindow::mouseReleaseEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");

    if (e->button()==Qt::RightButton) {
        if (getStatus()==SetSecondCorner) {
            deletePreview();
        }
        init(getStatus()-1);
    } else if (e->button()==Qt::LeftButton) {
        if (getStatus()==SetSecondCorner) {
            v2 = snapFree(e);
            trigger();
        }
    }
}



void RS_ActionZoomWindow::updateMouseButtonHints() {
    RS_DEBUG->print("RS_ActionZoomWindow::updateMouseButtonHints()");

    switch (getStatus()) {
    case SetFirstCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first edge"), tr("Cancel"),false);
        break;
    case SetSecondCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second edge"), tr("Back"),false);
        break;
    default:
        RS_DIALOGFACTORY->restoreMouseWidget();
        break;
    }
}



void RS_ActionZoomWindow::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::MagnifierCursor);
}


// EOF
