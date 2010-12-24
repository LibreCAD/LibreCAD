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

#include "rs_actionmodifytrim.h"

#include "rs_snapper.h"


/**
 * @param both Trim both entities.
 */
RS_ActionModifyTrim::RS_ActionModifyTrim(RS_EntityContainer& container,
        RS_GraphicView& graphicView, bool both)
        :RS_PreviewActionInterface("Trim Entity",
                           container, graphicView) {

    trimEntity = NULL;
    trimCoord = RS_Vector(false);
    limitEntity = NULL;
    limitCoord = RS_Vector(false);
    this->both = both;
}

QAction* RS_ActionModifyTrim::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;
	
	switch (type) {
	default:
	case RS2::ActionModifyTrim:
		// (tr("Trim")
		action = new QAction(tr("&Trim"), NULL);
		action->setIcon(QIcon(":/extui/modifytrim.png"));
	    //action->zetStatusTip(tr("Trim Entities"));
		break;

	case RS2::ActionModifyTrim2:
		// tr("Trim Two")
		action = new QAction(tr("&Trim Two"), NULL);
		action->setIcon(QIcon(":/extui/modifytrim2.png"));
        //action->zetStatusTip(tr("Trim two Entities"));
		break;
	}
	
    return action;
}


void RS_ActionModifyTrim::init(int status) {
    RS_ActionInterface::init(status);

    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;

}



void RS_ActionModifyTrim::trigger() {

    RS_DEBUG->print("RS_ActionModifyTrim::trigger()");

    if (trimEntity!=NULL && trimEntity->isAtomic() &&
            limitEntity!=NULL /* && limitEntity->isAtomic()*/) {

        RS_Modification m(*container, graphicView);
        m.trim(trimCoord, (RS_AtomicEntity*)trimEntity,
               limitCoord, /*(RS_AtomicEntity*)*/limitEntity,
               both);

        trimEntity = NULL;
        if (both) {
            limitEntity->setHighlighted(false);
            graphicView->drawEntity(limitEntity);
            setStatus(ChooseLimitEntity);
        } else {
            setStatus(ChooseTrimEntity);
        }

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    }
}



void RS_ActionModifyTrim::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyTrim::mouseMoveEvent begin");

    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
    RS_Entity* se = catchEntity(e);

    switch (getStatus()) {
    case ChooseLimitEntity:
        limitCoord = mouse;
        limitEntity = se;
        break;

    case ChooseTrimEntity:
        trimCoord = mouse;
        trimEntity = se;
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyTrim::mouseMoveEvent end");
}



void RS_ActionModifyTrim::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {

        RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
        RS_Entity* se = catchEntity(e);

        switch (getStatus()) {
        case ChooseLimitEntity:
            limitCoord = mouse;
            limitEntity = se;
            if (limitEntity!=NULL /*&& limitEntity->isAtomic()*/) {
                limitEntity->setHighlighted(true);
                graphicView->drawEntity(limitEntity);
                setStatus(ChooseTrimEntity);
            }
            break;

        case ChooseTrimEntity:
            trimCoord = mouse;
            trimEntity = se;
            if (trimEntity!=NULL && trimEntity->isAtomic()) {
                trigger();
            }
            break;

        default:
            break;
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        if (limitEntity!=NULL) {
            limitEntity->setHighlighted(false);
            graphicView->drawEntity(limitEntity);
        }
        init(getStatus()-1);
    }
}



void RS_ActionModifyTrim::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseLimitEntity:
        if (both) {
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select first trim entity"),
                                                tr("Cancel"));
        } else {
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select limiting entity"),
                                                tr("Back"));
        }
        break;
    case ChooseTrimEntity:
        if (both) {
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select second trim entity"),
                                                tr("Cancel"));
        } else {
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select entity to trim"),
                                                tr("Back"));
        }
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyTrim::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyTrim::updateToolBar() {
    RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
}


