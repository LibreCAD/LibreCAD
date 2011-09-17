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

#include "rs_actiondrawarctangential.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "ui_qg_arctangentialoptions.h"



RS_ActionDrawArcTangential::RS_ActionDrawArcTangential(RS_EntityContainer& container,
                                                       RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw arcs tangential",
                               container, graphicView) {
    reset();
}



RS_ActionDrawArcTangential::~RS_ActionDrawArcTangential() {}


QAction* RS_ActionDrawArcTangential::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    /*RVT_PORT    QAction* action = new QAction(tr("Arc: Tangential"),
                                  tr("&Tangential"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Arc: Tangential"), NULL);
    //action->zetStatusTip(tr("Draw arcs tangential to base entity"));
    return action;
}


void RS_ActionDrawArcTangential::reset() {
    baseEntity = NULL;
    isStartPoint = false;
    point = RS_Vector(false);
}



void RS_ActionDrawArcTangential::setByRadius(bool status) {
    byRadius=status;
}

void RS_ActionDrawArcTangential::init(int status) {
    RS_PreviewActionInterface::init(status);

    //reset();
}



void RS_ActionDrawArcTangential::trigger() {
    RS_PreviewActionInterface::trigger();

    if (point.valid==false || baseEntity==NULL) {
        RS_DEBUG->print("RS_ActionDrawArcTangential::trigger: "
                        "conditions not met");
        return;
    }

    preparePreview();
    RS_Arc* arc = new RS_Arc(container, data);
    arc->setLayerToActive();
    arc->setPenToActive();
    container->addEntity(arc);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(arc);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(arc->getCenter());

    setStatus(SetBaseEntity);
    reset();
}



void RS_ActionDrawArcTangential::preparePreview() {
    if (baseEntity!=NULL && point.valid) {
        RS_Vector startPoint;
        double direction;
        if (isStartPoint) {
            startPoint = baseEntity->getStartpoint();
            direction = RS_Math::correctAngle(baseEntity->getDirection1()+M_PI);
        } else {
            startPoint = baseEntity->getEndpoint();
            direction = RS_Math::correctAngle(baseEntity->getDirection2()+M_PI);
        }

        RS_Arc arc(NULL, RS_ArcData());
        bool suc;
        if (byRadius) {
            suc = arc.createFrom2PDirectionRadius(startPoint, point, direction, data.radius);
        } else {
            suc = arc.createFrom2PDirectionAngle(startPoint, point, direction, angleLength);
        }
        if (suc) {
            data = arc.getData();
            if(RS_DIALOGFACTORY != NULL) {
                if(byRadius){
                    RS_DIALOGFACTORY->updateArcTangentialOptions(arc.getAngleLength()*180./M_PI,true);
                }else{
                    RS_DIALOGFACTORY->updateArcTangentialOptions(arc.getRadius(),false);
                }
            }
        }
    }
}


void RS_ActionDrawArcTangential::mouseMoveEvent(QMouseEvent* e) {
    if(getStatus() == SetEndAngle) {
        point = snapPoint(e);
        preparePreview();
        if (data.isValid()) {
            RS_Arc* arc = new RS_Arc(preview, data);
            deletePreview();
            preview->addEntity(arc);
            drawPreview();
        }
    }
}



void RS_ActionDrawArcTangential::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {

        // set base entity:
        case SetBaseEntity: {
            RS_Vector coord = graphicView->toGraph(e->x(), e->y());
            RS_Entity* entity = catchEntity(coord, RS2::ResolveAll);
            if (entity!=NULL) {
                if (entity->isAtomic()) {
                    baseEntity = (RS_AtomicEntity*)entity;
                    if (baseEntity->getStartpoint().distanceTo(coord) <
                            baseEntity->getEndpoint().distanceTo(coord)) {
                        isStartPoint = true;
                    } else {
                        isStartPoint = false;
                    }
                    setStatus(SetEndAngle);
                    updateMouseButtonHints();
                } else {
                    // TODO: warning
                }
            }
            else {
            }
        }
            break;

            // set angle (point that defines the angle)
        case SetEndAngle: {
            RS_CoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawArcTangential::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetBaseEntity:
        break;

    case SetEndAngle:
        point = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawArcTangential::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }
}



QStringList RS_ActionDrawArcTangential::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}


void RS_ActionDrawArcTangential::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true);
    }
    updateMouseButtonHints();
}



void RS_ActionDrawArcTangential::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



void RS_ActionDrawArcTangential::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetBaseEntity:
        RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify base entity"),
                    tr("Cancel"));
        break;
    case SetEndAngle:
        if(byRadius) {
            RS_DIALOGFACTORY->updateMouseWidget(
                        tr("Specify end angle"), tr("Back"));
        } else {
            RS_DIALOGFACTORY->updateMouseWidget(
                        tr("Specify end point"), tr("Back"));
        }
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawArcTangential::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawArcTangential::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarArcs);
    }
}


// EOF
