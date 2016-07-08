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

#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawline.h"

#include "rs_actioneditundo.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLine::Points {
	/**
	* Line data defined so far.
	*/
	RS_LineData data;
	/**
		 * Start point of the series of lines. Used for close function.
		 */
	RS_Vector start;
	/**
		 * Point history (for undo)
		 */
	int historyIndex{-1};
	std::vector<RS_Vector> history;
};

RS_ActionDrawLine::RS_ActionDrawLine(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw lines",
							   container, graphicView)
	, pPoints(new Points{})
{
    RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine");
	actionType=RS2::ActionDrawLine;
    reset();
    RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine: OK");
}

RS_ActionDrawLine::~RS_ActionDrawLine() = default;

void RS_ActionDrawLine::reset() {
	RS_DEBUG->print("RS_ActionDrawLine::reset");
	pPoints.reset(new Points{});
    RS_DEBUG->print("RS_ActionDrawLine::reset: OK");
}

void RS_ActionDrawLine::init(int status) {
    RS_DEBUG->print("RS_ActionDrawLine::init");
    RS_PreviewActionInterface::init(status);

    reset();
    drawSnapper();
    RS_DEBUG->print("RS_ActionDrawLine::init: OK");
}

void RS_ActionDrawLine::trigger() {
    RS_PreviewActionInterface::trigger();

	RS_Line* line = new RS_Line(container, pPoints->data);
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);
	graphicView->moveRelativeZero(pPoints->history.at(pPoints->historyIndex));
    //    graphicView->moveRelativeZero(line->getEndpoint());
    RS_DEBUG->print("RS_ActionDrawLine::trigger(): line added: %d",
                    line->getId());
}

RS_Vector RS_ActionDrawLine::snapToAngle(const RS_Vector &currentCoord)
{
    if(getStatus() != SetEndpoint)
    {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Trying to snap to angle when not setting EndPoint!");
        return currentCoord;
    }
    if(snapMode.restriction != RS2::RestrictNothing ||
            snapMode.snapGrid) {
        return currentCoord;
    }
	double angle = pPoints->data.startpoint.angleTo(currentCoord)*180.0/M_PI;
    /*Snapping to angle(15*) if shift key is pressed*/
    const double angularResolution=15.;
    angle -= remainder(angle,angularResolution);
    angle *= M_PI/180.;
	RS_Vector res = RS_Vector::polar(pPoints->data.startpoint.distanceTo(currentCoord),
                 angle);
	res += pPoints->data.startpoint;
    snapPoint(res, true);
    return res;
}



void RS_ActionDrawLine::mouseMoveEvent(QMouseEvent* e)
{
    RS_Vector mouse = snapPoint(e);
	if (getStatus()==SetEndpoint && pPoints->data.startpoint.valid) {

        /*Snapping to angle(15*) if shift key is pressed*/
        if(e->modifiers() & Qt::ShiftModifier)
            mouse = snapToAngle(mouse);

        deletePreview();
        auto line = new RS_Line(pPoints->data.startpoint, mouse);
        preview->addEntity(line);
        line->setLayerToActive();
        line->setPenToActive();
        drawPreview();
    }
}

void RS_ActionDrawLine::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_Vector snapped = snapPoint(e);

        /*Snapping to angle(15*) if shift key is pressed*/
        if((e->modifiers() & Qt::ShiftModifier) && getStatus() == SetEndpoint )
            snapped = snapToAngle(snapped);
        RS_CoordinateEvent ce(snapped);
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLine::coordinateEvent(RS_CoordinateEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent");
    if (e==NULL) {
        RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: event was NULL");
        return;
    }

    RS_Vector mouse = e->getCoordinate();
	if(pPoints->data.startpoint.valid == false && getStatus()==SetEndpoint) setStatus(SetStartpoint);
    switch (getStatus()) {
    case SetStartpoint:
		pPoints->data.startpoint = mouse;
        addHistory(mouse);

		pPoints->start = pPoints->data.startpoint;
        setStatus(SetEndpoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetEndpoint:
		if((mouse-pPoints->data.startpoint).squared() > RS_TOLERANCE2) {
            //refuse zero length lines
			pPoints->data.endpoint = mouse;
            addHistory(mouse);
            trigger();
			pPoints->data.startpoint = pPoints->data.endpoint;
			if(pPoints->history.size()>=2) updateMouseButtonHints();
        }
        //graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: OK");
}

void RS_ActionDrawLine::commandEvent(RS_CommandEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent");
    QString c = e->getCommand().toLower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
			e->accept();
			return;
		}
		break;

    case SetEndpoint:
        if (checkCommand("close", c)) {
            close();
			e->accept();
			updateMouseButtonHints();
			return;
        }

        if (checkCommand("undo", c)) {
            undo();
			e->accept();
			updateMouseButtonHints();
			return;
        }
		break;

    default:
		return;
    }
    if (checkCommand("redo", c)) {
        redo();
		e->accept();
		updateMouseButtonHints();
	}
	//    RS_DEBUG->print("RS_ActionDrawLine::commandEvent: OK");
}

QStringList RS_ActionDrawLine::getAvailableCommands() {
    QStringList cmd;
	if(pPoints->historyIndex+1<(int) pPoints->history.size()) {
        cmd += command("redo");
    }

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetEndpoint:
		if (pPoints->historyIndex>=1) {
            cmd += command("undo");
        }
		if (pPoints->historyIndex>=2) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawLine::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetStartpoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
											tr("Cancel"));
		break;
	case SetEndpoint: {
		QString msg = "";

		if (pPoints->historyIndex>=2) {
			msg += RS_COMMANDS->command("close");
		}
		if(pPoints->historyIndex+1<(int) pPoints->history.size()) {
			if(msg.size()>0)  msg += "/";
			msg += RS_COMMANDS->command("redo");
		}
		if (pPoints->historyIndex>=1) {
			if(msg.size()>0)  msg += "/";
			msg += RS_COMMANDS->command("undo");
		}

		if (pPoints->historyIndex>=1) {
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
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
    }
}

void RS_ActionDrawLine::showOptions() {
	RS_DEBUG->print("RS_ActionDrawLine::showOptions");
	RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
	RS_DEBUG->print("RS_ActionDrawLine::showOptions: OK");
}

void RS_ActionDrawLine::hideOptions() {
	RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawLine::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

void RS_ActionDrawLine::close() {
	if (pPoints->historyIndex>2 && pPoints->start.valid && (pPoints->data.startpoint - pPoints->start).squared() > RS_TOLERANCE2 ) {
		pPoints->data.endpoint = pPoints->start;
		addHistory(pPoints->data.endpoint);
        trigger();
        setStatus(SetStartpoint);
		//        graphicView->moveRelativeZero(start);
	} else {
		RS_DIALOGFACTORY->commandMessage(
					tr("Cannot close sequence of lines: "
					   "Not enough entities defined yet, or already closed."));
	}
}

void RS_ActionDrawLine::addHistory(const RS_Vector& v){
	if(pPoints->historyIndex<-1) pPoints->historyIndex=-1;
	pPoints->history.erase(pPoints->history.begin()+pPoints->historyIndex+1,pPoints->history.end());
	pPoints->history.push_back(v);
	pPoints->historyIndex=pPoints->history.size() - 1;
}
void RS_ActionDrawLine::undo() {
	if (pPoints->historyIndex>0) {
		pPoints->historyIndex--;
        //        history.removeLast();
        deletePreview();
        graphicView->setCurrentAction(
                    new RS_ActionEditUndo(true, *container, *graphicView));
		pPoints->data.startpoint = pPoints->history.at(pPoints->historyIndex);
		graphicView->moveRelativeZero(pPoints->data.startpoint);
    } else {
        RS_DIALOGFACTORY->commandMessage(
                    tr("Cannot undo: "
                       "Not enough entities defined yet."));
    }
	if(pPoints->historyIndex>=1) {
        setStatus(SetEndpoint);
    }else{
        setStatus(SetStartpoint);
    }
}

void RS_ActionDrawLine::redo() {
	if ((int) pPoints->history.size()>pPoints->historyIndex+1) {
		pPoints->historyIndex++;
        //        history.removeLast();
        deletePreview();
        graphicView->setCurrentAction(
                    new RS_ActionEditUndo(false, *container, *graphicView));
		pPoints->data.startpoint = pPoints->history.at(pPoints->historyIndex);
		graphicView->moveRelativeZero(pPoints->data.startpoint);
    } else {
        RS_DIALOGFACTORY->commandMessage(
                    tr("Cannot redo: "
                       "Not previous line segment defined."));
    }
    setStatus(SetEndpoint);
}

// EOF
