/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include<QAction>
#include <QMouseEvent>
#include "rs_actiondrawlineorthtan.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_selection.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace{
auto circleList={RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse}; //this holds a list of entity types which supports tangent
}

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", container, graphicView)
	,normal(nullptr)
	,tangent(nullptr)
	,circle(nullptr)
{
	actionType=RS2::ActionDrawLineOrthTan;
}


void RS_ActionDrawLineOrthTan::finish(bool updateTB){
	clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineOrthTan::trigger() {
	if(!tangent) return;
    RS_PreviewActionInterface::trigger();

	deletePreview();
	if(circle)
		circle->setHighlighted(false);
	circle=nullptr;
	graphicView->redraw(RS2::RedrawDrawing);
	RS_Entity* newEntity = new RS_Line(container,
									   tangent->getData());
	newEntity->setLayerToActive();
	newEntity->setPenToActive();
	container->addEntity(newEntity);

	// upd. undo list:
	if (document) {
		document->startUndoCycle();
		document->addUndoable(newEntity);
		document->endUndoCycle();
	}

	graphicView->redraw(RS2::RedrawDrawing);

	setStatus(SetCircle);

}



void RS_ActionDrawLineOrthTan::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent begin");
	e->accept();
	RS_Vector mouse(graphicView->toGraphX(e->x()),
					graphicView->toGraphY(e->y()));

	switch(getStatus()){
	case SetLine:
		return;
	case SetCircle:{

		RS_Entity* en = catchEntity(e, circleList, RS2::ResolveAll);
		if(!en) return;
		deletePreview();
		if(circle)
			circle->setHighlighted(false);
		circle = en;
		circle->setHighlighted(true);
		graphicView->redraw(RS2::RedrawDrawing);
		deletePreview();
		RS_Creation creation(preview.get(), graphicView, false);
		tangent = creation.createLineOrthTan(mouse,
											 normal,
											 circle);
		preview->addEntity(tangent);
		drawPreview();

	}
	default:
		break;
	}
	RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent end");
}


void RS_ActionDrawLineOrthTan::clearLines()
{
	for(RS_Entity* p: {(RS_Entity*) normal, circle}){
		if(p){
			p->setHighlighted(false);
			graphicView->drawEntity(p);
		}
	}
	if(circle) circle=nullptr;
	deletePreview();
}

void RS_ActionDrawLineOrthTan::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
		clearLines();
        if (getStatus() == SetLine) {
				finish(true);
        }else{
                init(getStatus()-1);
        }
    } else {
        switch (getStatus()) {
        case SetLine: {
            RS_Entity* en=catchEntity(e,RS2::EntityLine);
			if(en){
                if (en->getLength() < RS_TOLERANCE) {
                    //ignore lines not long enough
                    break;
                }
				if(normal) {
                    normal->setHighlighted(false);
                    graphicView->drawEntity(normal);
                }
                normal=static_cast<RS_Line*>(en);
                normal->setHighlighted(true);
                graphicView->drawEntity(normal);
                setStatus(SetCircle);
            }
        }
            break;

        case SetCircle:
			if(tangent){
				trigger();
			}
			break;

        default:
            break;
        }
    }

}



void RS_ActionDrawLineOrthTan::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetLine:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select a line"),
											tr("Cancel"));
		break;
	case SetCircle:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawLineOrthTan::updateMouseCursor() {
        if(isFinished()) {
    graphicView->setMouseCursor(RS2::ArrowCursor);
        }else{
    graphicView->setMouseCursor(RS2::SelectCursor);
        }
}

// EOF
