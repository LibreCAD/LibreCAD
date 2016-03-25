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

#include "rs_actiondrawpoint.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_point.h"
#include "rs_coordinateevent.h"

RS_ActionDrawPoint::RS_ActionDrawPoint(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Points",
						   container, graphicView)
		, pt(new RS_Vector{})
{
	actionType=RS2::ActionDrawPoint;
}

RS_ActionDrawPoint::~RS_ActionDrawPoint() = default;


void RS_ActionDrawPoint::trigger() {
	if (pt->valid) {
		RS_Point* point = new RS_Point(container, RS_PointData(*pt));
        container->addEntity(point);

        if (document) {
            document->startUndoCycle();
            document->addUndoable(point);
            document->endUndoCycle();
        }

		graphicView->moveRelativeZero(*pt);
		graphicView->redraw((RS2::RedrawMethod) (RS2::RedrawDrawing | RS2::RedrawOverlay));
    }
}



void RS_ActionDrawPoint::mouseMoveEvent(QMouseEvent* e) {
    snapPoint(e);
}



void RS_ActionDrawPoint::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionDrawPoint::coordinateEvent(RS_CoordinateEvent* e) {
	if (e==nullptr) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

	*pt = mouse;
    trigger();
}



void RS_ActionDrawPoint::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
		return;
	}
}



QStringList RS_ActionDrawPoint::getAvailableCommands() {
	return {};
}


void RS_ActionDrawPoint::updateMouseButtonHints() {
	switch (getStatus()) {
	case 0:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify location"), tr("Cancel"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
