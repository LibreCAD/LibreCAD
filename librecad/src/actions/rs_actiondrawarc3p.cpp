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
#include "rs_actiondrawarc3p.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_actiondrawarc.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"

struct RS_ActionDrawArc3P::Points {
RS_ArcData data;
/**
 * 1st point.
 */
RS_Vector point1;
/**
 * 2nd point.
 */
RS_Vector point2;
/**
 * 3nd point.
 */
RS_Vector point3;
};

RS_ActionDrawArc3P::RS_ActionDrawArc3P(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw arcs 3P",
						   container, graphicView)
		, pPoints(new Points())
{
	actionType=RS2::ActionDrawArc3P;
    reset();
}



RS_ActionDrawArc3P::~RS_ActionDrawArc3P() = default;


void RS_ActionDrawArc3P::reset() {
	pPoints.reset(new Points{});
}



void RS_ActionDrawArc3P::init(int status) {
    RS_PreviewActionInterface::init(status);

    //reset();
}



void RS_ActionDrawArc3P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
	if (pPoints->data.isValid()) {
		RS_Arc* arc = new RS_Arc{container, pPoints->data};
        arc->setLayerToActive();
        arc->setPenToActive();
        container->addEntity(arc);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(arc);
            document->endUndoCycle();
        }

                graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(arc->getEndpoint());

        setStatus(SetPoint1);
        reset();
    } else {
        //RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid arc data."));
        RS_DIALOGFACTORY->commandMessage(tr("Invalid arc data."));
    }
}



void RS_ActionDrawArc3P::preparePreview() {
	pPoints->data = {};
	if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid) {
		RS_Arc arc(NULL, pPoints->data);
		bool suc = arc.createFrom3P(pPoints->point1, pPoints->point2, pPoints->point3);
        if (suc) {
			pPoints->data = arc.getData();
        }
    }
}


void RS_ActionDrawArc3P::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetPoint1:
		pPoints->point1 = mouse;
        break;

    case SetPoint2:
		pPoints->point2 = mouse;
		if (pPoints->point1.valid) {
			RS_Line* line = new RS_Line{preview.get(), pPoints->point1, pPoints->point2};

            deletePreview();
            preview->addEntity(line);
            drawPreview();
        }
        break;

    case SetPoint3:
		pPoints->point3 = mouse;
        preparePreview();
		if (pPoints->data.isValid()) {
			RS_Arc* arc = new RS_Arc(preview.get(), pPoints->data);

            deletePreview();
            preview->addEntity(arc);
            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawArc3P::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawArc3P::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetPoint1:
		pPoints->point1 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetPoint2);
        break;

    case SetPoint2:
		pPoints->point2 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetPoint3);
        break;

    case SetPoint3:
		pPoints->point3 = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawArc3P::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    if (RS_COMMANDS->checkCommand("center", c, rtti())) {
        finish(false);
        graphicView->setCurrentAction(
            new RS_ActionDrawArc(*container, *graphicView));
    }
}



QStringList RS_ActionDrawArc3P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawArc3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify startpoint or [center]"),
            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify second point"), tr("Back"));
        break;
    case SetPoint3:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify endpoint"), tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawArc3P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF

