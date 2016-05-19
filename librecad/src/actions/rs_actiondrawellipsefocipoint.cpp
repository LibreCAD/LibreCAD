/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

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

#include "rs_actiondrawellipsefocipoint.h"
#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_ellipse.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawEllipseFociPoint::Points {
	// Foci of ellipse
	RS_Vector focus1,focus2;
	// A point on ellipse
	RS_Vector point;
	RS_Vector center,major;
	double c; //hold half of distance between foci
	double d; //hold half of distance
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseFociPoint::RS_ActionDrawEllipseFociPoint(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw ellipse by foci and a point",
						   container, graphicView)
		, pPoints(new Points{})
{
	actionType=RS2::ActionDrawEllipseFociPoint;
}

RS_ActionDrawEllipseFociPoint::~RS_ActionDrawEllipseFociPoint() = default;

void RS_ActionDrawEllipseFociPoint::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status==SetFocus1) {
		pPoints->focus1.valid=false;
    }
}

double RS_ActionDrawEllipseFociPoint::findRatio() const
{
	return 	sqrt(pPoints->d*pPoints->d-pPoints->c*pPoints->c)/pPoints->d;

}

void RS_ActionDrawEllipseFociPoint::trigger() {
    RS_PreviewActionInterface::trigger();

	RS_Ellipse* ellipse = new RS_Ellipse{container,
	{pPoints->center,
			pPoints->major*pPoints->d,
			findRatio(),
			0., 0.,false}
};
    ellipse->setLayerToActive();
    ellipse->setPenToActive();

    container->addEntity(ellipse);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(ellipse);
        document->endUndoCycle();
    }

//    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->moveRelativeZero(ellipse->getCenter());
    graphicView->redraw(RS2::RedrawDrawing);
    drawSnapper();

    setStatus(SetFocus1);

    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::trigger():"
                    " entity added: %d", ellipse->getId());
}



void RS_ActionDrawEllipseFociPoint::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {


    case SetPoint:
		pPoints->point=mouse;
		pPoints->d=0.5*(pPoints->focus1.distanceTo(pPoints->point) +
						pPoints->focus2.distanceTo(pPoints->point));
		if (pPoints->d > pPoints->c+ RS_TOLERANCE) {
			deletePreview();
			preview->addEntity(new RS_Ellipse{preview.get(),
											  {pPoints->center,
											  pPoints->major*pPoints->d,
											  findRatio(),
											  0., 0.,false}});
            drawPreview();
        }
        break;



    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::mouseMoveEvent end");
}



void RS_ActionDrawEllipseFociPoint::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }

    // Return to last status:
    else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionDrawEllipseFociPoint::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetFocus1:
        graphicView->moveRelativeZero(mouse);
		pPoints->focus1=mouse;
        setStatus(SetFocus2);
        break;

    case SetFocus2:
		pPoints->c = 0.5*pPoints->focus1.distanceTo(mouse);
		if(pPoints->c > RS_TOLERANCE){
            graphicView->moveRelativeZero(mouse);
			pPoints->focus2=mouse;
			pPoints->center=(pPoints->focus1+pPoints->focus2)*0.5;
			pPoints->major=pPoints->focus1-pPoints->center;
			pPoints->major /= pPoints->c ;
            setStatus(SetPoint);
        }
        break;
    case SetPoint:
		pPoints->point=mouse;
		pPoints->d=0.5*(pPoints->focus1.distanceTo(pPoints->point)+pPoints->focus2.distanceTo(pPoints->point));
		if (pPoints->d > pPoints->c+ RS_TOLERANCE) {
            graphicView->moveRelativeZero(mouse);
            trigger();
        }
        break;


    default:
        break;
    }
}

void RS_ActionDrawEllipseFociPoint::commandEvent(RS_CommandEvent* e) {
	QString cmd = e->getCommand().toLower();

	if (checkCommand("help", cmd)) {
			RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
											 + getAvailableCommands().join(": ")+
											 tr("specify a point on ellipse, or total distance to foci")
											 );
		e->accept();
		return;
	}

	if(getStatus()==SetPoint){
		bool ok;
		double a = RS_Math::eval(cmd, &ok);
		if (ok) {
			pPoints->d=0.5*fabs(a);
			if (pPoints->d > pPoints->c + RS_TOLERANCE) {
				trigger();
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Total distance %1 is smaller than distance between foci").arg(fabs(a)));
		} else
			RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));

	}
}


QStringList RS_ActionDrawEllipseFociPoint::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawEllipseFociPoint::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetFocus1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first focus of ellipse"),
											tr("Cancel"));
		break;

	case SetFocus2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second focus of ellipse"),
											tr("Back"));
		break;

	case SetPoint:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify a point on ellipse or total distance to foci"),
					tr("Back"));
		break;

	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionDrawEllipseFociPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
