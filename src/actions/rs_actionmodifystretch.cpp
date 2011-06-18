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

#include "rs_actionmodifystretch.h"

#include "rs_snapper.h"



RS_ActionModifyStretch::RS_ActionModifyStretch(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Stretch Entities",
                           container, graphicView) {

    firstCorner = RS_Vector(false);
    secondCorner = RS_Vector(false);
    referencePoint = RS_Vector(false);
    targetPoint = RS_Vector(false);
}

QAction* RS_ActionModifyStretch::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Stretch")
    QAction* action = new QAction(tr("&Stretch"), NULL);
	action->setIcon(QIcon(":/extui/modifystretch.png"));
    //action->zetStatusTip(tr("Stretch Entities"));
    return action;
}


void RS_ActionModifyStretch::init(int status) {
    RS_ActionInterface::init(status);

}



void RS_ActionModifyStretch::trigger() {

    RS_DEBUG->print("RS_ActionModifyStretch::trigger()");

    deletePreview();

    RS_Modification m(*container, graphicView);
    m.stretch(firstCorner, secondCorner, targetPoint-referencePoint);

    setStatus(SetFirstCorner);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionModifyStretch::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetFirstCorner:
        break;

    case SetSecondCorner:
        if (firstCorner.valid) {
            secondCorner = snapPoint(e);
            deletePreview();
            preview->addEntity(
                new RS_Line(preview,
                            RS_LineData(RS_Vector(firstCorner.x,
                                                  firstCorner.y),
                                        RS_Vector(secondCorner.x,
                                                  firstCorner.y))));
            preview->addEntity(
                new RS_Line(preview,
                            RS_LineData(RS_Vector(secondCorner.x,
                                                  firstCorner.y),
                                        RS_Vector(secondCorner.x,
                                                  secondCorner.y))));
            preview->addEntity(
                new RS_Line(preview,
                            RS_LineData(RS_Vector(secondCorner.x,
                                                  secondCorner.y),
                                        RS_Vector(firstCorner.x,
                                                  secondCorner.y))));
            preview->addEntity(
                new RS_Line(preview,
                            RS_LineData(RS_Vector(firstCorner.x,
                                                  secondCorner.y),
                                        RS_Vector(firstCorner.x,
                                                  firstCorner.y))));
            drawPreview();
        }
        break;

    case SetReferencePoint:
        break;

    case SetTargetPoint:
        if (referencePoint.valid) {
            targetPoint = mouse;

            deletePreview();
            preview->addStretchablesFrom(*container, firstCorner, secondCorner);
            //preview->move(targetPoint-referencePoint);
            preview->stretch(firstCorner, secondCorner,
                             targetPoint-referencePoint);
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent end");
}



void RS_ActionModifyStretch::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyStretch::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetFirstCorner:
        firstCorner = mouse;
        setStatus(SetSecondCorner);
        break;

    case SetSecondCorner:
        secondCorner = mouse;
        deletePreview();
        setStatus(SetReferencePoint);
        break;

    case SetReferencePoint:
        referencePoint = mouse;
        graphicView->moveRelativeZero(referencePoint);
        setStatus(SetTargetPoint);
        break;

    case SetTargetPoint:
        targetPoint = mouse;
        graphicView->moveRelativeZero(targetPoint);
        trigger();
        //finish();
        break;

    default:
        break;
    }

}


void RS_ActionModifyStretch::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFirstCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first corner"),
                                            tr("Cancel"));
        break;
    case SetSecondCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second corner"),
                                            tr("Back"));
        break;
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Back"));
        break;
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyStretch::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyStretch::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
    }
}


// EOF
