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

#include "rs_actioninfoinside.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"



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
    //action->zetStatusTip(tr("Checks if a given point is inside the selected contour"));

    return action;
}

void RS_ActionInfoInside::trigger() {
    bool onContour = false;
    if (RS_Information::isPointInsideContour(pt, contour, &onContour)) {
        RS_DIALOGFACTORY->commandMessage(tr("Point is inside selected contour."));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("Point is outside selected contour."));
    }
    finish(false);
}



void RS_ActionInfoInside::mouseMoveEvent(QMouseEvent* /*e*/) {
    //RS_Vector mouse = snapPoint(e);
    //bool onContour = false;
    /*if (RS_Information::isPointInsideContour(mouse, contour, &onContour)) {
    } else {
    }*/
}



void RS_ActionInfoInside::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
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
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}
        // EOF
