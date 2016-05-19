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
#include "rs_actiondrawcircle2p.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"

struct RS_ActionDrawCircle2P::Points {
	/**
	 * 1st point.
	 */
	RS_Vector point1;
	/**
	 * 2nd point.
	 */
	RS_Vector point2;
};

RS_ActionDrawCircle2P::RS_ActionDrawCircle2P(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles",
						   container, graphicView)
		, data(new RS_CircleData())
		, pPoints(new Points{})
{
	actionType=RS2::ActionDrawCircle2P;
    reset();
}

RS_ActionDrawCircle2P::~RS_ActionDrawCircle2P() = default;

void RS_ActionDrawCircle2P::reset() {
	data.reset(new RS_CircleData{});
	pPoints->point1 = {};
	pPoints->point2 = {};
}



void RS_ActionDrawCircle2P::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawCircle2P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
	if (data->isValid()) {
        RS_Circle* circle = new RS_Circle(container,
										  *data);
        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(circle);
            document->endUndoCycle();
        }

        RS_Vector rz = graphicView->getRelativeZero();
                graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);

        setStatus(SetPoint1);
        reset();
	} else
		RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid Circle data."));
}



void RS_ActionDrawCircle2P::preparePreview() {
	data.reset(new RS_CircleData{});
	if (pPoints->point1.valid && pPoints->point2.valid) {
		RS_Circle circle(nullptr, *data);
		bool suc = circle.createFrom2P(pPoints->point1, pPoints->point2);
        if (suc) {
			data.reset(new RS_CircleData(circle.getData()));
        }
    }
}


void RS_ActionDrawCircle2P::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetPoint1:
		pPoints->point1 = mouse;
        break;

    case SetPoint2:
		pPoints->point2 = mouse;
        preparePreview();
		if (data->isValid()) {
			RS_Circle* circle = new RS_Circle(preview.get(), *data);
            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircle2P::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircle2P::coordinateEvent(RS_CoordinateEvent* e) {
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
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircle2P::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
        return;
    }

}



QStringList RS_ActionDrawCircle2P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircle2P::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetPoint1:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify first point"), tr("Cancel"));
		break;
	case SetPoint2:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify second point"), tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawCircle2P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF

