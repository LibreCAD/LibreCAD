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

#include "rs_actiondrawlinerectangle.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLineRectangle::Points {
	/**
	 * 1st corner.
	 */
	RS_Vector corner1;
	/**
	 * 2nd corner.
	 */
	RS_Vector corner2;
};

RS_ActionDrawLineRectangle::RS_ActionDrawLineRectangle(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw rectangles",
						   container, graphicView)
		, pPoints(new Points{})
{
	actionType=RS2::ActionDrawLineRectangle;
}

RS_ActionDrawLineRectangle::~RS_ActionDrawLineRectangle() = default;


void RS_ActionDrawLineRectangle::trigger() {
	RS_PreviewActionInterface::trigger();

	RS_Polyline* polyline = new RS_Polyline(container);

	// create and add rectangle:
	polyline->addVertex(pPoints->corner1);
	polyline->setLayerToActive();
	polyline->setPenToActive();
	polyline->addVertex({pPoints->corner2.x, pPoints->corner1.y});
	polyline->addVertex(pPoints->corner2);
	polyline->addVertex({pPoints->corner1.x, pPoints->corner2.y});
	polyline->setClosed(true);
	polyline->endPolyline();
	container->addEntity(polyline);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
		document->addUndoable(polyline);
        document->endUndoCycle();
    }

    // upd. view
	graphicView->redraw(RS2::RedrawDrawing);
	graphicView->moveRelativeZero(pPoints->corner2);
}



void RS_ActionDrawLineRectangle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
	if (getStatus()==SetCorner2 && pPoints->corner1.valid) {
		pPoints->corner2 = mouse;
        deletePreview();
		preview->addRectangle(pPoints->corner1, pPoints->corner2);
		drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent end");
}


void RS_ActionDrawLineRectangle::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLineRectangle::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCorner1:
		pPoints->corner1 = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetCorner2);
        break;

    case SetCorner2:
		pPoints->corner2 = mouse;
        trigger();
        setStatus(SetCorner1);
        break;

    default:
        break;
    }
}

void RS_ActionDrawLineRectangle::commandEvent(RS_CommandEvent* e) {
	QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }
}

void RS_ActionDrawLineRectangle::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCorner1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first corner"),
											tr("Cancel"));
		break;
	case SetCorner2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second corner"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}


void RS_ActionDrawLineRectangle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
