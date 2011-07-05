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

#include "rs_actiondimangular.h"


#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_information.h"



RS_ActionDimAngular::RS_ActionDimAngular(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw Angular Dimensions",
                    container, graphicView) {
    reset();
}

QAction* RS_ActionDimAngular::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Angular"),
    QAction* action = new QAction(tr("&Angular"), NULL);
	action->setIcon(QIcon(":/extui/dimangular.png"));
    //action->zetStatusTip(tr("Angular Dimension"));
    return action;
}


void RS_ActionDimAngular::reset() {
    RS_ActionDimension::reset();

    edata = RS_DimAngularData(RS_Vector(false),
                              RS_Vector(false),
                              RS_Vector(false),
                              RS_Vector(false));
    line1 = NULL;
    line2 = NULL;
    center = RS_Vector(false);
    RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimAngular::trigger() {
    RS_PreviewActionInterface::trigger();

    if (line1!=NULL && line2!=NULL) {
        RS_DimAngular* newEntity = NULL;

        newEntity = new RS_DimAngular(container,
                                      data,
                                      edata);

        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        // upd. undo list:
        if (document!=NULL) {
            document->startUndoCycle();
            document->addUndoable(newEntity);
            document->endUndoCycle();
        }
        RS_Vector rz = graphicView->getRelativeZero();
		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);

    } else {
        RS_DEBUG->print("RS_ActionDimAngular::trigger:"
                        " Entity is NULL\n");
    }
}



void RS_ActionDimAngular::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimAngular::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
    case SetLine1:
        break;

    case SetLine2:
        break;

    case SetPos:
        if (line1!=NULL && line2!=NULL && center.valid) {
            RS_Vector mouse = snapPoint(e);
            edata.definitionPoint4 = mouse;

            RS_DimAngular* d = new RS_DimAngular(preview, data, edata);
            d->update();

            deletePreview();
            preview->addEntity(d);
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDimAngular::mouseMoveEvent end");
}



void RS_ActionDimAngular::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetLine1: {
                RS_Entity* en = catchEntity(e, RS2::ResolveAll);
                if (en!=NULL &&
                        en->rtti()==RS2::EntityLine) {
                    line1 = (RS_Line*)en;
                    setStatus(SetLine2);
                }
            }
            break;

        case SetLine2: {
                RS_Entity* en = catchEntity(e, RS2::ResolveAll);
                if (en!=NULL &&
                        en->rtti()==RS2::EntityLine) {
                    line2 = (RS_Line*)en;

                    RS_VectorSolutions sol =
                        RS_Information::getIntersectionLineLine(line1, line2);

                    if (sol.get(0).valid) {
                        center = sol.get(0);

                        if (center.distanceTo(line1->getStartpoint()) <
                                center.distanceTo(line1->getEndpoint())) {
                            edata.definitionPoint1 = line1->getStartpoint();
                            edata.definitionPoint2 = line1->getEndpoint();
                        } else {
                            edata.definitionPoint1 = line1->getEndpoint();
                            edata.definitionPoint2 = line1->getStartpoint();
                        }

                        if (center.distanceTo(line2->getStartpoint()) <
                                center.distanceTo(line2->getEndpoint())) {
                            edata.definitionPoint3 = line2->getStartpoint();
                            data.definitionPoint = line2->getEndpoint();
                        } else {
                            edata.definitionPoint3 = line2->getEndpoint();
                            data.definitionPoint = line2->getStartpoint();
                        }
                        graphicView->moveRelativeZero(center);
                        setStatus(SetPos);
                    }
                }
            }
            break;

        case SetPos: {
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



void RS_ActionDimAngular::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    switch (getStatus()) {
    case SetPos:
        edata.definitionPoint4 = e->getCoordinate();
        trigger();
        reset();
        setStatus(SetLine1);
        break;

    default:
        break;
    }
}


void RS_ActionDimAngular::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    // setting new text label:
    if (getStatus()==SetText) {
        setText(c);
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // command: text
    if (checkCommand("text", c)) {
        lastStatus = (Status)getStatus();
        graphicView->disableCoordinateInput();
        setStatus(SetText);
    }
}



QStringList RS_ActionDimAngular::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetLine1:
    case SetLine2:
    case SetPos:
        cmd += command("text");
        break;

    default:
        break;
    }

    return cmd;
}



void RS_ActionDimAngular::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDimAngular::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDimAngular::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetLine1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first line"),
                                            tr("Cancel"));
        break;
    case SetLine2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second line"),
                                            tr("Cancel"));
        break;
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify dimension arc line location"), tr("Cancel"));
        break;
    case SetText:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



// EOF
