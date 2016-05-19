/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
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
#include<vector>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawcircleinscribe.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawCircleInscribe::Points {
	RS_CircleData cData;
	RS_Vector coord;
	std::vector<RS_Line*> lines;
};

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleInscribe::RS_ActionDrawCircleInscribe(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circle inscribed",
						   container, graphicView)
		, pPoints(new Points{})
		, valid(false)
{
	actionType=RS2::ActionDrawCircleInscribe;
}

RS_ActionDrawCircleInscribe::~RS_ActionDrawCircleInscribe() = default;

void RS_ActionDrawCircleInscribe::clearLines(bool checkStatus)
{
	while(pPoints->lines.size() ){
		if(checkStatus && (int) pPoints->lines.size()<=getStatus() )
			break;
		pPoints->lines.back()->setHighlighted(false);
		graphicView->drawEntity(pPoints->lines.back());
		pPoints->lines.pop_back();
	}
}


void RS_ActionDrawCircleInscribe::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status>=0) {
        RS_Snapper::suspend();
	}
	clearLines(true);
}

void RS_ActionDrawCircleInscribe::finish(bool updateTB){
	clearLines();
	RS_PreviewActionInterface::finish(updateTB);
}


void RS_ActionDrawCircleInscribe::trigger() {
    RS_PreviewActionInterface::trigger();


	RS_Circle* circle=new RS_Circle(container, pPoints->cData);

    deletePreview();
    container->addEntity(circle);

    // upd. undo list:
	if (document) {
        document->startUndoCycle();
        document->addUndoable(circle);
        document->endUndoCycle();
    }

	clearLines(false);

    setStatus(SetLine1);

    RS_DEBUG->print("RS_ActionDrawCircle4Line::trigger():"
                    " entity added: %d", circle->getId());
}



void RS_ActionDrawCircleInscribe::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent begin");

    if(getStatus() == SetLine3) {
        RS_Entity*  en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
		if(!en) return;
        if(!(en->isVisible() && en->rtti()== RS2::EntityLine)) return;
        for(int i=0;i<getStatus();i++) {
            if(en->getId() == pPoints->lines[i]->getId()) return; //do not pull in the same line again
        }
		if(en->getParent() && en->getParent()->ignoredOnModification())
			return;
		pPoints->coord= graphicView->toGraph(e->x(), e->y());
		deletePreview();
		while(pPoints->lines.size()==3){
			pPoints->lines.back()->setHighlighted(false);
			graphicView->drawEntity(pPoints->lines.back());
			pPoints->lines.pop_back();
		}
		en->setHighlighted(true);
		pPoints->lines.push_back(static_cast<RS_Line*>(en));
		graphicView->drawEntity(pPoints->lines.back());
        if(preparePreview()) {
			RS_Circle* e=new RS_Circle(preview.get(), pPoints->cData);
            preview->addEntity(e);
            drawPreview();
        }

    }
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent end");
}


bool RS_ActionDrawCircleInscribe::preparePreview(){
    valid=false;
    if(getStatus() == SetLine3) {
		RS_Circle c(preview.get(), pPoints->cData);
		valid= c.createInscribe(pPoints->coord, pPoints->lines);
        if(valid){
			pPoints->cData = c.getData();
        }
    }
    return valid;
}

void RS_ActionDrawCircleInscribe::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
		if (!e) {
            return;
        }
        RS_Entity*  en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
		if(!en) return;
        if(!(en->isVisible() && en->rtti()== RS2::EntityLine)) return;
        for(int i=0;i<getStatus();i++) {
            if(en->getId() == pPoints->lines[i]->getId()) return; //do not pull in the same line again
        }
		if(en->getParent()) {
			if ( en->getParent()->ignoredOnModification()) return;
        }
		while((int) pPoints->lines.size()>getStatus()){
			pPoints->lines.back()->setHighlighted(false);
			graphicView->drawEntity(pPoints->lines.back());
			pPoints->lines.pop_back();
		}
		pPoints->lines.push_back(static_cast<RS_Line*>(en));
		pPoints->coord= graphicView->toGraph(e->x(), e->y());
        switch (getStatus()) {
        case SetLine1:
        case SetLine2:
			en->setHighlighted(true);
			setStatus(getStatus()+1);
			graphicView->redraw(RS2::RedrawDrawing);
			break;
        case SetLine3:
            if( preparePreview()) {
                trigger();
            }

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        // Return to last status:
		if(getStatus()>0){
			clearLines(true);
			pPoints->lines.back()->setHighlighted(false);
			pPoints->lines.pop_back();
            graphicView->redraw(RS2::RedrawDrawing);
            deletePreview();
        }
        init(getStatus()-1);
    }
}


//void RS_ActionDrawCircleInscribe::coordinateEvent(RS_CoordinateEvent* e) {

//}

//fixme, support command line

/*
void RS_ActionDrawCircle4Line::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    default:
        break;
    }
}
*/


QStringList RS_ActionDrawCircleInscribe::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleInscribe::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetLine1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first line"),
											tr("Cancel"));
		break;

	case SetLine2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second line"),
											tr("Back"));
		break;

	case SetLine3:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the third line"),
											tr("Back"));
		break;

	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawCircleInscribe::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
