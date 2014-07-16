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

#include "rs_actiondrawsplinepoints.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"

RS_ActionDrawSplinePoints::RS_ActionDrawSplinePoints(RS_EntityContainer& container,
	RS_GraphicView& graphicView) : RS_PreviewActionInterface("Draw splines through points",
	container, graphicView), spline(NULL)
{
	spline = NULL;
	data = RS_SplinePointsData(false);
}

RS_ActionDrawSplinePoints::~RS_ActionDrawSplinePoints()
{
	clear();
}

QAction* RS_ActionDrawSplinePoints::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/)
{
	QAction* action = new QAction(tr("&Spline through points"),  NULL);
	action->setIcon(QIcon(":/extui/menuspline.png"));
	//action->zetStatusTip(tr("Draw splines"));
	return action;
}

void RS_ActionDrawSplinePoints::clear()
{
	if(spline) delete spline;
	spline = NULL;
}

void RS_ActionDrawSplinePoints::reset()
{
	clear();
	data = RS_SplinePointsData(false);
	undoBuffer.clear();
}

void RS_ActionDrawSplinePoints::init(int status)
{
	RS_PreviewActionInterface::init(status);
	reset();
}

void RS_ActionDrawSplinePoints::trigger()
{
	if(!spline) return;

	RS_SplinePoints *sp = new RS_SplinePoints(container, spline->data);

	deletePreview();
	spline = NULL;

	sp->setInsert(false);
	sp->setLayerToActive();
	sp->setPenToActive();
	sp->update();
//sp->setClosed(true);
	container->addEntity(sp);

	// upd. undo list:
	if(document != NULL)
	{
		document->startUndoCycle();
		document->addUndoable(sp);
		document->endUndoCycle();
	}

	// upd view
	RS_Vector r = graphicView->getRelativeZero();
	graphicView->redraw(RS2::RedrawDrawing);
	graphicView->moveRelativeZero(r);
	RS_DEBUG->print("RS_ActionDrawSplinePoints::trigger(): spline added: %d",
		sp->getId());

	reset();
	setStatus(SetStartPoint);
}

void RS_ActionDrawSplinePoints::mouseMoveEvent(QMouseEvent* e)
{
	RS_DEBUG->print("RS_ActionDrawSplinePoints::mouseMoveEvent begin");

	RS_Vector mouse = snapPoint(e);
	if(getStatus() == SetNextPoint && spline)
	{
		spline->dynamicPoint(mouse);
		drawPreview();
	}

	RS_DEBUG->print("RS_ActionDrawSplinePoints::mouseMoveEvent end");
}

void RS_ActionDrawSplinePoints::mouseReleaseEvent(QMouseEvent* e)
{
	if(e->button() == Qt::LeftButton)
	{
		RS_CoordinateEvent ce(snapPoint(e));
		coordinateEvent(&ce);
	}
	else if(e->button() == Qt::RightButton)
	{
		if(getStatus() == SetNextPoint && spline)
		{
			trigger();
		}
		init(getStatus() - 1);
	}
}

void RS_ActionDrawSplinePoints::coordinateEvent(RS_CoordinateEvent* e)
{
	if(e == NULL) return;

	RS_Vector mouse = e->getCoordinate();

	switch (getStatus())
	{
	case SetStartPoint:
		undoBuffer.clear();
		if(spline == NULL)
		{
			spline = new RS_SplinePoints(container, data);
			spline->setInsert(true);
			spline->addPoint(mouse);
			preview->addEntity(spline);
			preview->addEntity(new RS_Point(preview, RS_PointData(mouse)));
		}
		setStatus(SetNextPoint);
		graphicView->moveRelativeZero(mouse);
		updateMouseButtonHints();
		break;
	case SetNextPoint:
		graphicView->moveRelativeZero(mouse);
		if(spline != NULL)
		{
			if(spline->addPoint(mouse))
				preview->addEntity(new RS_Point(preview, RS_PointData(mouse)));
			drawPreview();
			drawSnapper();
		}
		updateMouseButtonHints();
		break;
	default:
		break;
	}
}

void RS_ActionDrawSplinePoints::commandEvent(RS_CommandEvent* e)
{
	QString c = e->getCommand().toLower();

	switch (getStatus())
	{
	case SetStartPoint:
		if(checkCommand("help", c))
		{
			RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
				+ getAvailableCommands().join(", "));
			return;
		}
		break;
	case SetNextPoint:
		if (checkCommand("undo", c))
		{
			undo();
			updateMouseButtonHints();
			return;
		}
		if (checkCommand("redo", c))
		{
			redo();
			updateMouseButtonHints();
			return;
		}
		break;
	default:
		break;
	}
}

QStringList RS_ActionDrawSplinePoints::getAvailableCommands()
{
	QStringList cmd;

	switch (getStatus())
	{
	case SetStartPoint:
		break;
	case SetNextPoint:
		if(data.splinePoints.count() > 0)
		{
			cmd += command("undo");
		}
		if(undoBuffer.count() > 0)
		{
			cmd += command("redo");
		}
		if(data.splinePoints.count() > 2)
		{
			cmd += command("close");
		}
		break;
	default:
		break;
	}

	return cmd;
}

void RS_ActionDrawSplinePoints::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SetStartPoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first control point"),
			tr("Cancel"));
		break;
	case SetNextPoint:
		{
		QString msg = "";

		if(data.splinePoints.count() > 2)
		{
			msg += RS_COMMANDS->command("close");
			msg += "/";
		}
		if(data.splinePoints.count() > 0)
		{
			msg += RS_COMMANDS->command("undo");
		}
		if(undoBuffer.count() > 0)
		{
			msg += RS_COMMANDS->command("redo");
		}

		if(data.splinePoints.count() > 0)
		{
			RS_DIALOGFACTORY->updateMouseWidget(
				tr("Specify next control point or [%1]").arg(msg),
				tr("Back"));
		}
		else
		{
			RS_DIALOGFACTORY->updateMouseWidget(
				tr("Specify next control point"),
				tr("Back"));
		}
		}
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget("", "");
		break;
	}
}

void RS_ActionDrawSplinePoints::showOptions()
{
	RS_ActionInterface::showOptions();
	RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionDrawSplinePoints::hideOptions()
{
	RS_ActionInterface::hideOptions();
	RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawSplinePoints::updateMouseCursor()
{
	graphicView->setMouseCursor(RS2::CadCursor);
}

//void RS_ActionDrawSplinePoints::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}


/*
void RS_ActionDrawSplinePoints::close() {
    if (history.count()>2 && start.valid) {
        //data.endpoint = start;
        //trigger();
                if (spline!=NULL) {
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

void RS_ActionDrawSplinePoints::undo()
{
	if(!spline)
	{
		RS_DIALOGFACTORY->commandMessage(
			tr("Cannot undo: Not enough entities defined yet."));
	}

	QList<RS_Vector> splinePts = data.splinePoints;

	int nPoints = splinePts.count();
	if(nPoints > 1)
	{
		RS_Vector v = splinePts.last();
		undoBuffer.append(v);
		spline->removeLastPoint();

		if(splinePts.isEmpty()) setStatus(SetStartPoint);
		else
		{
			v = splinePts.last();
			graphicView->moveRelativeZero(v);
		}
		graphicView->redraw(RS2::RedrawDrawing);
	}
	else
	{
		RS_DIALOGFACTORY->commandMessage(
			tr("Cannot undo: Not enough entities defined yet."));
	}
}

void RS_ActionDrawSplinePoints::redo()
{
	int iBufLen = undoBuffer.count();
	if(iBufLen > 1)
	{
		RS_Vector v = undoBuffer.last();
		spline->addPoint(v);
		undoBuffer.removeLast();

		setStatus(SetNextPoint);
		v = data.splinePoints.last();
		graphicView->moveRelativeZero(v);
		graphicView->redraw(RS2::RedrawDrawing);
	}
	else
	{
		RS_DIALOGFACTORY->commandMessage(
			tr("Cannot undo: Nothing could be redone."));
	}
}

// EOF

