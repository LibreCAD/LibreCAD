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

#include "rs_actiondrawlinepolygon.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLinePolygonCenCor::Points {
	/** Center of polygon */
	RS_Vector center;
	/** Edge */
	RS_Vector corner;
};

RS_ActionDrawLinePolygonCenCor::RS_ActionDrawLinePolygonCenCor(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Draw Polygons (Center,Corner)", container, graphicView)
		, pPoints(new Points{})
		,number(3)
		,lastStatus(SetCenter)
{
	actionType=RS2::ActionDrawLinePolygonCenCor;
}

RS_ActionDrawLinePolygonCenCor::~RS_ActionDrawLinePolygonCenCor() = default;

void RS_ActionDrawLinePolygonCenCor::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
	bool ok = creation.createPolygon(pPoints->center, pPoints->corner, number);

    if (!ok) {
        RS_DEBUG->print("RS_ActionDrawLinePolygon::trigger:"
                        " No polygon added\n");
    }
}



void RS_ActionDrawLinePolygonCenCor::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetCenter:
        break;

    case SetCorner:
		if (pPoints->center.valid) {
			pPoints->corner = mouse;
            deletePreview();

			RS_Creation creation(preview.get(), nullptr, false);
			creation.createPolygon(pPoints->center, pPoints->corner, number);

            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygonCenCor::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLinePolygonCenCor::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e)  return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
		pPoints->center = mouse;
        setStatus(SetCorner);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetCorner:
		pPoints->corner = mouse;
        trigger();
        break;

    default:
        break;
    }
}

void RS_ActionDrawLinePolygonCenCor::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCenter:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify center"),
											"");
		break;

	case SetCorner:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify a corner"), "");
		break;

	case SetNumber:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
		break;

	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawLinePolygonCenCor::showOptions() {
    RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionDrawLinePolygonCenCor::hideOptions() {
    RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawLinePolygonCenCor::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetCenter:
    case SetCorner:
        if (checkCommand("number", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetNumber);
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok) {
                e->accept();
                if (n>0 && n<10000) {
                    number = n;
				} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
														"Try 1..9999"));
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawLinePolygonCenCor::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetCenter:
    case SetCorner:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLinePolygonCenCor::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
