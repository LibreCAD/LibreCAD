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
#include "rs_actiondrawcircle3p.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"

struct RS_ActionDrawCircle3P::Points {
	RS_CircleData data;
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


RS_ActionDrawCircle3P::RS_ActionDrawCircle3P(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles",
						   container, graphicView)
		,pPoints(new Points{})
{
	actionType=RS2::ActionDrawCircle3P;
}

RS_ActionDrawCircle3P::~RS_ActionDrawCircle3P() = default;

void RS_ActionDrawCircle3P::init(int status) {
    RS_PreviewActionInterface::init(status);
	pPoints.reset(new Points{});
}



void RS_ActionDrawCircle3P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
	if (pPoints->data.isValid()) {
		RS_Circle* circle = new RS_Circle{container, pPoints->data};
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
        drawSnapper();

        setStatus(SetPoint1);
		pPoints.reset(new Points{});
	} else
		RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid circle data."));
}


void RS_ActionDrawCircle3P::preparePreview() {
	pPoints->data = RS_CircleData{};
	if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid) {
		RS_Circle circle{nullptr, pPoints->data};
		bool suc = circle.createFrom3P(pPoints->point1,
									   pPoints->point2,
									   pPoints->point3);
        if (suc) {
			pPoints->data = circle.getData();
        }
    }
}



void RS_ActionDrawCircle3P::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetPoint1:
		pPoints->point1 = mouse;
        break;

    case SetPoint2:
		pPoints->point2 = mouse;
        break;

    case SetPoint3:
		pPoints->point3 = mouse;
        preparePreview();
		if (pPoints->data.isValid()) {
			RS_Circle* circle = new RS_Circle{preview.get(), pPoints->data};

            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }
        break;
    }
}



void RS_ActionDrawCircle3P::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircle3P::coordinateEvent(RS_CoordinateEvent* e) {
	if (e==nullptr) {
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



void RS_ActionDrawCircle3P::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }
}



QStringList RS_ActionDrawCircle3P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}


void RS_ActionDrawCircle3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second point"),
                                            tr("Back"));
        break;
    case SetPoint3:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify third point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawCircle3P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF

