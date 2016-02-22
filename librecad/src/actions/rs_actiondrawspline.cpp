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

#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawspline.h"

#include "rs_spline.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_point.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawSpline::Points {

	/**
	 * Spline data defined so far.
	 */
	RS_SplineData data;
	/**
	 * Polyline entity we're working on.
	 */
	RS_Spline* spline{nullptr};
	/**
	 * Point history (for undo)
	 */
		QList<RS_Vector> history;

	/**
	 * Bulge history (for undo)
	 */
		//QList<double> bHistory;
};

RS_ActionDrawSpline::RS_ActionDrawSpline(RS_EntityContainer& container,
										 RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw splines",
							   container, graphicView)
	,pPoints(new Points{})
{
	actionType=RS2::ActionDrawSpline;
	reset();
}

RS_ActionDrawSpline::~RS_ActionDrawSpline() = default;

void RS_ActionDrawSpline::reset() {
	pPoints->spline = nullptr;
	pPoints->history.clear();
}



void RS_ActionDrawSpline::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawSpline::trigger() {
    RS_PreviewActionInterface::trigger();

		if (!pPoints->spline) {
                return;
        }

        // add the entity
    //RS_Spline* spline = new RS_Spline(container, data);
	pPoints->spline->setLayerToActive();
	pPoints->spline->setPenToActive();
	pPoints->spline->update();
	container->addEntity(pPoints->spline);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
		document->addUndoable(pPoints->spline);
        document->endUndoCycle();
    }

        // upd view
        RS_Vector r = graphicView->getRelativeZero();
        graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(r);
    RS_DEBUG->print("RS_ActionDrawSpline::trigger(): spline added: %d",
					pPoints->spline->getId());

		pPoints->spline = nullptr;
    //history.clear();
}

void RS_ActionDrawSpline::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawSpline::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
	if (getStatus()==SetNextPoint && pPoints->spline /*&& point.valid*/) {
        deletePreview();

				RS_Spline* tmpSpline = static_cast<RS_Spline*>(pPoints->spline->clone());
                tmpSpline->addControlPoint(mouse);
                tmpSpline->update();
                preview->addEntity(tmpSpline);

				auto cpts = tmpSpline->getControlPoints();
				for (const RS_Vector& vp: cpts) {
						preview->addEntity(new RS_Point(preview.get(), RS_PointData(vp)));
                }
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawSpline::mouseMoveEvent end");
}



void RS_ActionDrawSpline::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
				if (getStatus()==SetNextPoint &&
						pPoints->spline &&
						pPoints->spline->getNumberOfControlPoints()>=pPoints->spline->getDegree()+1) {
                        trigger();
                }
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawSpline::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetStartpoint:
                //data.startpoint = mouse;
        //point = mouse;
		pPoints->history.clear();
		pPoints->history.append(mouse);
				if (!pPoints->spline) {
						pPoints->spline = new RS_Spline(container, pPoints->data);
						pPoints->spline->addControlPoint(mouse);
                }
        //bHistory.clear();
        //bHistory.append(new double(0.0));
        //start = mouse;
        setStatus(SetNextPoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetNextPoint:
        graphicView->moveRelativeZero(mouse);
        //point = mouse;
		pPoints->history.append(mouse);
        //bHistory.append(new double(0.0));
				if (pPoints->spline) {
                        //graphicView->deleteEntity(spline);
						pPoints->spline->addControlPoint(mouse);
                        //spline->setEndpoint(mouse);
                        //if (spline->count()==1) {
                        //spline->setLayerToActive();
                        //spline->setPenToActive();
                                //container->addEntity(spline);
                        //}
                        deletePreview();
                        //graphicView->drawEntity(spline);
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



void RS_ActionDrawSpline::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
            return;
        }
        break;

    case SetNextPoint:
        /*if (checkCommand("close", c)) {
            close();
            updateMouseButtonHints();
            return;
        }*/

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



QStringList RS_ActionDrawSpline::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetNextPoint:
		if (pPoints->history.size()>=2) {
            cmd += command("undo");
		}else if (pPoints->history.size()>=3) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawSpline::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first control point"),
                                            tr("Cancel"));
        break;
    case SetNextPoint: {
            QString msg = "";

			if (pPoints->history.size()>=3) {
                msg += RS_COMMANDS->command("close");
                msg += "/";
            }
			if (pPoints->history.size()>=2) {
				msg += RS_COMMANDS->command("undo");
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point or [%1]").arg(msg),
                    tr("Back"));
            } else {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next control point"),
                    tr("Back"));
            }
        }
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}


void RS_ActionDrawSpline::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawSpline::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawSpline::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

/*
void RS_ActionDrawSpline::close() {
    if (history.count()>2 && start.valid) {
        //data.endpoint = start;
        //trigger();
                if (spline) {
                        RS_CoordinateEvent e(spline->getStartpoint());
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
*/

void RS_ActionDrawSpline::undo() {
	if (pPoints->history.size()>1) {
		pPoints->history.removeLast();
        //bHistory.removeLast();
        deletePreview();
        //graphicView->setCurrentAction(
        //    new RS_ActionEditUndo(true, *container, *graphicView));
				if (!pPoints->history.isEmpty()) {
                //point = *history.last();
                }
				if (pPoints->spline) {
						pPoints->spline->removeLastControlPoint();
						if (!pPoints->history.isEmpty()) {
							RS_Vector v = pPoints->history.last();
                            graphicView->moveRelativeZero(v);
                        }
                        graphicView->redraw(RS2::RedrawDrawing);

                }
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot undo: "
               "Not enough entities defined yet."));
    }
}


void RS_ActionDrawSpline::setDegree(int deg) {
		pPoints->data.degree = deg;
		if (pPoints->spline) {
				pPoints->spline->setDegree(deg);
        }
}

int RS_ActionDrawSpline::getDegree() {
		return pPoints->data.degree;
}

void RS_ActionDrawSpline::setClosed(bool c) {
		pPoints->data.closed = c;
		if (pPoints->spline) {
				pPoints->spline->setClosed(c);
        }
}

bool RS_ActionDrawSpline::isClosed() {
		return pPoints->data.closed;
}

// EOF
