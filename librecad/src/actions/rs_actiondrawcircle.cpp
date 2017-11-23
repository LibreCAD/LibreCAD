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
#include "rs_actiondrawcircle.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDrawCircle::RS_ActionDrawCircle(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles",
						   container, graphicView)
		,data(new RS_CircleData())
{
	actionType=RS2::ActionDrawCircle;
    reset();
}



RS_ActionDrawCircle::~RS_ActionDrawCircle() = default;

void RS_ActionDrawCircle::reset() {
	data.reset(new RS_CircleData{});
}



void RS_ActionDrawCircle::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawCircle::trigger() {
    RS_PreviewActionInterface::trigger();

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
        graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(circle->getCenter());

    setStatus(SetCenter);
    reset();

    RS_DEBUG->print("RS_ActionDrawCircle::trigger(): circle added: %d",
                    circle->getId());
}



void RS_ActionDrawCircle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircle::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetCenter:
		data->center = mouse;
        break;

    case SetRadius:
		if (data->center.valid) {
			data->radius = data->center.distanceTo(mouse);
            deletePreview();
			preview->addEntity(new RS_Circle(preview.get(),
											 *data));
            drawPreview();
        }
        break;
    }

    RS_DEBUG->print("RS_ActionDrawCircle::mouseMoveEvent end");
}



void RS_ActionDrawCircle::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircle::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e){
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
		data->center = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetRadius);
        break;

    case SetRadius:
		if (data->center.valid) {
            graphicView->moveRelativeZero(mouse);
			data->radius = data->center.distanceTo(mouse);
            trigger();
        }
        //setStatus(SetCenter);
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircle::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
		return;
	}

    switch (getStatus()) {

    case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok) {
				data->radius = r;
                e->accept();
                trigger();
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            //setStatus(SetCenter);
        }

    default:
        break;
    }
}



QStringList RS_ActionDrawCircle::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}


void RS_ActionDrawCircle::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCenter:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify center"), tr("Cancel"));
        break;
    case SetRadius:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify point on circle"), tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawCircle::showOptions() {
    RS_ActionInterface::showOptions();
}



void RS_ActionDrawCircle::hideOptions() {
    RS_ActionInterface::hideOptions();
}



void RS_ActionDrawCircle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF

