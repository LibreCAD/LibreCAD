/****************************************************************************
** $Id: rs_actiondrawpolyline.cpp,v 1.6 2004/07/21 22:44:34 andrew Exp $
**
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2009 Jani Frilander.
** Copyright (C) 2010 R. van Twisk
**
** This file is part of the qcadlib Library project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**********************************************************************/
// New drawing modes added by Jani Frilander 2009

#include "rs_actiondrawpolyline.h"
#include "rs_actioneditundo.h"
#include "rs_snapper.h"



RS_ActionDrawPolyline::RS_ActionDrawPolyline(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw polylines",
                           container, graphicView) {
    Reversed=1;
    reset();
    history.setAutoDelete(true);
    bHistory.setAutoDelete(true);
}



RS_ActionDrawPolyline::~RS_ActionDrawPolyline() {}


QAction* RS_ActionDrawPolyline::createGUIAction(RS2::ActionType /*type*/, 
				QObject* /*parent*/) {
	// (tr("Polyline")
	QAction* action = new QAction(tr("&Polyline"),  NULL);	
    action->setStatusTip(tr("Draw polylines"));
    return action;
}



void RS_ActionDrawPolyline::reset() {
	polyline = NULL;
    data = RS_PolylineData(RS_Vector(false), RS_Vector(false), false);
    start = RS_Vector(false);
    history.clear();
    bHistory.clear();
}



void RS_ActionDrawPolyline::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawPolyline::trigger() {
    RS_PreviewActionInterface::trigger();

	if (polyline==NULL) {
		return;
	}

	// add the entity
    //RS_Polyline* polyline = new RS_Polyline(container, data);
    //polyline->setLayerToActive();
    //polyline->setPenToActive();
    //container->addEntity(polyline);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(polyline);
        document->endUndoCycle();
    }

	// upd view
    deleteSnapper();
    graphicView->moveRelativeZero(RS_Vector(0.0,0.0));
    graphicView->drawEntity(polyline);
    graphicView->moveRelativeZero(polyline->getEndpoint());
    drawSnapper();
    RS_DEBUG->print("RS_ActionDrawLinePolyline::trigger(): polyline added: %d",
                    polyline->getId());

	polyline = NULL;
}



void RS_ActionDrawPolyline::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    double bulge=solveBulge(mouse);
    if (getStatus()==SetNextPoint && point.valid) {
        deletePreview();
        // clearPreview();
		
		//RS_Polyline* p = polyline->clone();
		//p->reparent(preview);
		//preview->addEntity(p);
        if (fabs(bulge)<RS_TOLERANCE || Mode==Line) {
	    preview->addEntity(new RS_Line(preview,
                                       RS_LineData(point, mouse)));
	} else
	    preview->addEntity(new RS_Arc(preview,arc_data));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent end");
}



void RS_ActionDrawPolyline::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
		if (getStatus()==SetNextPoint) {
			trigger();
		}
        deletePreview();
        // clearPreview();
        deleteSnapper();
        init(getStatus()-1);
    }
}

double RS_ActionDrawPolyline::solveBulge(RS_Vector mouse) {

    double b;
    bool suc;
    RS_Arc arc(NULL, RS_ArcData());
    RS_Line line(NULL,RS_LineData());
    double direction,direction2,delta;
    RS_AtomicEntity* lastentity;
    calculatedSegment=false;

    switch (Mode){
     case Line:
        b=0.0;
        break;
     case Tangential:
	if (polyline!=NULL){
            lastentity = (RS_AtomicEntity*)polyline->lastEntity();
            direction = RS_Math::correctAngle(
                lastentity->getDirection2()+M_PI);
	    line.setStartpoint(point);
	    line.setEndpoint(mouse);
	    direction2=RS_Math::correctAngle(line.getDirection2()+M_PI);
	    delta=fabs(direction-direction2);
	    if(delta<RS_TOLERANCE_ANGLE ||
		(delta<M_PI+RS_TOLERANCE_ANGLE &&
		delta>M_PI-RS_TOLERANCE_ANGLE))
		b=0;
	    else{
		b=tan((direction2-direction)/2);
	        suc = arc.createFrom2PBulge(point,mouse,b);
        	if (suc)
	            arc_data = arc.getData();
        	else
          	    b=0;
	    }
	}
	else
	    b=0;
        break;
     case TanRad:
        if (polyline!=NULL){
	    lastentity = (RS_AtomicEntity*)polyline->lastEntity();
	    direction = RS_Math::correctAngle(
		lastentity->getDirection2()+M_PI);
	    suc = arc.createFrom2PDirectionRadius(point, mouse,
		direction,Radius);
	    if (suc){
        	arc_data = arc.getData();
		b=arc.getBulge();
		calculatedEndpoint = arc.getEndpoint();
		calculatedSegment=true;
	    
	    }
	    else
		b=0;
	}
        else
          b=0;
	break;
/*     case TanAng:
	b=tan(Reversed*Angle*M_PI/720.0);
	break;
     case TanRadAng:
	b=tan(Reversed*Angle*M_PI/720.0);
	break;*/
     case Ang:
        b=tan(Reversed*Angle*M_PI/720.0);
        suc = arc.createFrom2PBulge(point,mouse,b);
        if (suc)
          arc_data = arc.getData();
        else
          b=0;
	break;
/*     case RadAngEndp:
        b=tan(Reversed*Angle*M_PI/720.0);
        break;
     case RadAngCenp:
	b=tan(Reversed*Angle*M_PI/720.0);*/
    }
    return b;
}

void RS_ActionDrawPolyline::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    double bulge=solveBulge(mouse);
    if (calculatedSegment)
	mouse=calculatedEndpoint;

    switch (getStatus()) {
    case SetStartpoint:
	//	data.startpoint = mouse;
        //printf ("SetStartpoint\n");
	point = mouse;
        history.clear();
        history.append(new RS_Vector(mouse));
        bHistory.clear();
        bHistory.append(new double(0.0));
        start = point;
        setStatus(SetNextPoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetNextPoint:
    	graphicView->moveRelativeZero(mouse);
        point = mouse;
        history.append(new RS_Vector(mouse));
        bHistory.append(new double(bulge));
		if (polyline==NULL) {
			//printf("polyline==NULL\n");
			polyline = new RS_Polyline(container, data);
			polyline->addVertex(start, 0.0);
		}
		if (polyline!=NULL) {
			polyline->setNextBulge(bulge);
			polyline->addVertex(mouse, 0.0);
			polyline->setEndpoint(mouse);
			if (polyline->count()==1) {
    			polyline->setLayerToActive();
    			polyline->setPenToActive();
				container->addEntity(polyline);
			}
			deletePreview();
			// clearPreview();
			deleteSnapper();
			graphicView->drawEntity(polyline);
			drawSnapper();
		}
        //trigger();
        //data.startpoint = data.endpoint;
        updateMouseButtonHints();
        //graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
}



void RS_ActionDrawPolyline::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
            return;
        }
        break;

    case SetNextPoint:
        if (checkCommand("close", c)) {
            close();
            updateMouseButtonHints();
            return;
        }

        if (checkCommand("undo", c)) {
            undo();
            updateMouseButtonHints();
            return;
        }
        break;

    default:
        break;
    }
}



RS_StringList RS_ActionDrawPolyline::getAvailableCommands() {
    RS_StringList cmd;

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetNextPoint:
        if (history.count()>=2) {
            cmd += command("undo");
        }
        if (history.count()>=3) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawPolyline::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetNextPoint: {
            RS_String msg = "";

            if (history.count()>=3) {
                msg += RS_COMMANDS->command("close");
                msg += "/";
            }
            if (history.count()>=2) {
                msg += RS_COMMANDS->command("undo");
            }

            if (history.count()>=2) {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point or [%1]").arg(msg),
                    tr("Back"));
            } else {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point"),
                    tr("Back"));
            }
        }
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}


void RS_ActionDrawPolyline::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawPolyline::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawPolyline::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


void RS_ActionDrawPolyline::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}

void RS_ActionDrawPolyline::close() {
    if (history.count()>2 && start.valid) {
        //data.endpoint = start;
        //trigger();
		if (polyline!=NULL) {
			if (Mode==TanRad)
				Mode=Line;
			RS_CoordinateEvent e(polyline->getStartpoint());
			coordinateEvent(&e);
		}
		trigger();
        setStatus(SetStartpoint);
        graphicView->moveRelativeZero(start);
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot close sequence of lines: "
               "Not enough entities defined yet."));
    }
}

void RS_ActionDrawPolyline::undo() {
    if (history.count()>1) {
        if (history.count()>2){
	history.removeLast();
        bHistory.removeLast();
        deletePreview();
        // clearPreview();
        //graphicView->setCurrentAction(
        //    new RS_ActionEditUndo(true, *container, *graphicView));
		//if (history.last()!=NULL) {
        	point = *history.last();
		//}
		if (polyline!=NULL) {
			polyline->removeLastVertex();
        	graphicView->moveRelativeZero(polyline->getStartpoint());
			graphicView->redraw();
		}
        //if (history.count()==1) {
          //polyline->clear();
	  //delete polyline;
          //polyline = NULL;
        //}	
	}
	else
	RS_DIALOGFACTORY->commandMessage(
		tr("Undo disallowed due a fatal bug somewhere. Sorry."));
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot undo: "
               "Not enough entities defined yet."));
    }
}

// EOF
