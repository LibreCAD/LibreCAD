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

#include "rs_actiondrawlineparallelthrough.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_commandevent.h"



RS_ActionDrawLineParallelThrough::RS_ActionDrawLineParallelThrough(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Parallels", container, graphicView) {

    parallel = NULL;
    entity = NULL;
    distance = 1.0;
    number = 1;
    coord = RS_Vector(false);
}


QAction* RS_ActionDrawLineParallelThrough::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Par&allel through point"),
    QAction* action = new QAction(tr("Parallel through point"), NULL);
	action->setIcon(QIcon(":/extui/linesparathrough.png"));
    //action->zetStatusTip(tr("Draw parallel through a given point"));
    return action;
}

void RS_ActionDrawLineParallelThrough::trigger() {
    RS_PreviewActionInterface::trigger();

    if (entity!=NULL) {
        RS_Creation creation(container, graphicView);
        RS_Entity* e = creation.createParallelThrough(coord,
                       number,
                       entity);

        if (e==NULL) {
            RS_DEBUG->print("RS_ActionDrawLineParallelThrough::trigger:"
                            " No parallels added\n");
        }
    }
}



void RS_ActionDrawLineParallelThrough::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineParallelThrough::mouseMoveEvent begin");


    switch (getStatus()) {
    case SetEntity: {
            entity = catchEntity(e, RS2::ResolveAll);
            /*deletePreview();

            RS_Creation creation(preview, NULL, false);
            creation.createParallelThrough(coord,
                                           number,
                                           entity);

            drawPreview();*/
        }
        break;

    case SetPos: {
            coord = snapPoint(e);
            if(coord.valid==false) return;
            //RS_Vector(graphicView->toGraphX(e->x()),
            //                  graphicView->toGraphY(e->y()));
            deletePreview();

            RS_Creation creation(preview, NULL, false);
            creation.createParallelThrough(coord,
                                           number,
                                           entity);

            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineParallelThrough::mouseMoveEvent end");
}



void RS_ActionDrawLineParallelThrough::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity:
            entity = catchEntity(e, RS2::ResolveAll);
            if (entity!=NULL) {
                entity->setHighlighted(true);
                graphicView->drawEntity(entity);
                setStatus(SetPos);
            }
            break;
        case SetPos: {
            RS_Vector vp=snapPoint(e);
            if(vp.valid==false) return;
            RS_CoordinateEvent ce(vp);
                coordinateEvent(&ce);
            }
            break;
        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        if (entity!=NULL) {
            entity->setHighlighted(false);
            graphicView->drawEntity(entity);
            entity=NULL;
        }
        init(getStatus()-1);
    }
}



void RS_ActionDrawLineParallelThrough::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetPos:
        coord = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawLineParallelThrough::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select entity"), tr("Cancel"));
        break;

    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify through point"),
                                            tr("Back"));
        break;

    case SetNumber:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Number:"), tr("Back"));
        break;

    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawLineParallelThrough::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
    updateMouseButtonHints();
}



void RS_ActionDrawLineParallelThrough::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawLineParallelThrough::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetEntity:
    case SetPos: {
            if (checkCommand("number", c)) {
                deletePreview();
                lastStatus = (Status)getStatus();
                setStatus(SetNumber);
            }
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok==true) {
                if (n>0 && n<100) {
                    number = n;
                } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                        "Try 1..99"));
                }
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawLineParallelThrough::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetEntity:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLineParallelThrough::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLineParallelThrough::updateToolBar() {
    if (getStatus()==SetPos && !isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}


// EOF
