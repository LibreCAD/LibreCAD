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

#include "rs_actiondimdiametric.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"


RS_ActionDimDiametric::RS_ActionDimDiametric(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw Diametric Dimensions",
                    container, graphicView) {
    reset();
}


QAction* RS_ActionDimDiametric::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Diametric")
    QAction* action = new QAction(tr("&Diametric"),  NULL);
	action->setIcon(QIcon(":/extui/dimdiametric.png"));
    //action->zetStatusTip(tr("Diametric Dimension"));
    return action;
}

void RS_ActionDimDiametric::reset() {
    RS_ActionDimension::reset();

    edata = RS_DimDiametricData(RS_Vector(false),
                                0.0);
    entity = NULL;
    pos = RS_Vector(false);
    RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimDiametric::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (entity!=NULL) {
        RS_DimDiametric* newEntity = NULL;

        newEntity = new RS_DimDiametric(container,
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
        RS_DEBUG->print("RS_ActionDimDiametric::trigger:"
                        " Entity is NULL\n");
    }
}



void RS_ActionDimDiametric::preparePreview() {
    if (entity!=NULL) {
        double radius=0.0;
        RS_Vector center = RS_Vector(false);
        if (entity->rtti()==RS2::EntityArc) {
            radius = ((RS_Arc*)entity)->getRadius();
            center = ((RS_Arc*)entity)->getCenter();
        } else if (entity->rtti()==RS2::EntityCircle) {
            radius = ((RS_Circle*)entity)->getRadius();
            center = ((RS_Circle*)entity)->getCenter();
        }
        double angle = center.angleTo(pos);

        data.definitionPoint.setPolar(radius, angle+M_PI);
        data.definitionPoint += center;

        edata.definitionPoint.setPolar(radius, angle);
        edata.definitionPoint += center;
    }
}



void RS_ActionDimDiametric::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimDiametric::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
    case SetEntity:
        entity = catchEntity(e, RS2::ResolveAll);
        break;

    case SetPos:
        if (entity!=NULL) {
            pos = snapPoint(e);

            preparePreview();
            RS_DimDiametric* d = new RS_DimDiametric(preview, data, edata);
            d->update();

            deletePreview();
            preview->addEntity(d);
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDimDiametric::mouseMoveEvent end");
}



void RS_ActionDimDiametric::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity: {
                RS_Entity* en = catchEntity(e, RS2::ResolveAll);
                if (en!=NULL) {
                    if (en->rtti()==RS2::EntityArc ||
                            en->rtti()==RS2::EntityCircle) {

                        entity = en;
                        RS_Vector center;
                        if (entity->rtti()==RS2::EntityArc) {
                            center =
                                ((RS_Arc*)entity)->getCenter();
                        } else if (entity->rtti()==RS2::EntityCircle) {
                            center =
                                ((RS_Circle*)entity)->getCenter();
                        }
                        graphicView->moveRelativeZero(center);
                        setStatus(SetPos);
                    } else {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a circle "
                                                            "or arc entity"));
                    }
                }
            }
            break;

        case SetPos: {
                RS_CoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }

}



void RS_ActionDimDiametric::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    switch (getStatus()) {
    case SetPos:
        pos = e->getCoordinate();
        trigger();
        reset();
        setStatus(SetEntity);
        break;

    default:
        break;
    }
}



void RS_ActionDimDiametric::commandEvent(RS_CommandEvent* e) {
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

    // setting angle
    if (getStatus()==SetPos) {
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok==true) {
            pos.setPolar(1.0, RS_Math::deg2rad(a));
            pos += data.definitionPoint;
            trigger();
            reset();
            setStatus(SetEntity);
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        return;
    }
}



QStringList RS_ActionDimDiametric::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetEntity:
    case SetPos:
        cmd += command("text");
        break;

    default:
        break;
    }

    return cmd;
}


void RS_ActionDimDiametric::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select arc or circle entity"),
                                            tr("Cancel"));
        break;
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify dimension line location"), tr("Cancel"));
        break;
    case SetText:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDimDiametric::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDimDiametric::hideOptions() {
    RS_ActionInterface::hideOptions();

    //RS_DIALOGFACTORY->requestDimDiametricOptions(edata, false);
    RS_DIALOGFACTORY->requestOptions(this, false);
}



// EOF
