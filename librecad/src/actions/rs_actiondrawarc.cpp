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
#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawarc.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDrawArc::RS_ActionDrawArc(RS_EntityContainer& container,
                                   RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw arcs",
						   container, graphicView)
		,data(new RS_ArcData())
{
	actionType= RS2::ActionDrawArc;

    reset();
}

RS_ActionDrawArc::~RS_ActionDrawArc() = default;

void RS_ActionDrawArc::reset() {

	if (data->reversed) {
		data.reset(new RS_ArcData(RS_Vector(false),
                          0.0,
						  2.*M_PI, 0.0,
						  true));
    } else {
		data.reset(new RS_ArcData(RS_Vector(false),
                          0.0,
						  0.0, 2.*M_PI,
						  false));
    }
}



void RS_ActionDrawArc::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}



void RS_ActionDrawArc::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Arc* arc = new RS_Arc(container,
							 *data);
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
    graphicView->moveRelativeZero(arc->getCenter());

    setStatus(SetCenter);
    reset();

    RS_DEBUG->print("RS_ActionDrawArc::trigger(): arc added: %d",
                    arc->getId());
}



void RS_ActionDrawArc::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawArc::mouseMoveEvent begin");

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
			{data->center, data->radius}));
            drawPreview();
        }
        break;

    case SetAngle1:
		data->angle1 = data->center.angleTo(mouse);
		if (data->reversed) {
			data->angle2 = RS_Math::correctAngle(data->angle1-M_PI/3);
        } else {
			data->angle2 = RS_Math::correctAngle(data->angle1+M_PI/3);
        }
        deletePreview();
		preview->addEntity(new RS_Arc(preview.get(),
									  *data));
        drawPreview();
        break;

    case SetAngle2:
		data->angle2 = data->center.angleTo(mouse);
        deletePreview();
		preview->addEntity(new RS_Arc(preview.get(),
									  *data));
        drawPreview();
        break;

    case SetIncAngle:
		data->angle2 = data->angle1 + data->center.angleTo(mouse);
        deletePreview();
		preview->addEntity(new RS_Arc(preview.get(),
									  *data));
        drawPreview();
        break;

    case SetChordLength: {
			double x = data->center.distanceTo(mouse);
			if (fabs(x/(2*data->radius))<=1.0) {
				data->angle2 = data->angle1 + asin(x/(2*data->radius)) * 2;
                deletePreview();
				preview->addEntity(new RS_Arc(preview.get(),
											  *data));
                drawPreview();
            }
        }
        break;

    default:
        break;

    }

    RS_DEBUG->print("RS_ActionDrawArc::mouseMoveEvent end");
}



void RS_ActionDrawArc::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawArc::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
		data->center = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetRadius);
        break;

    case SetRadius:
		if (data->center.valid) {
			data->radius = data->center.distanceTo(mouse);
        }
        setStatus(SetAngle1);
        break;

    case SetAngle1:
		data->angle1 = data->center.angleTo(mouse);
        setStatus(SetAngle2);
        break;

    case SetAngle2:
		data->angle2 = data->center.angleTo(mouse);
        trigger();
        break;

    case SetIncAngle:
		data->angle2 = data->angle1 + data->center.angleTo(mouse);
        trigger();
        break;

    case SetChordLength: {
			double x = data->center.distanceTo(mouse);
			if (fabs(x/(2*data->radius))<=1.0) {
				data->angle2 = data->angle1 + asin(x/(2*data->radius)) * 2;
                trigger();
            }
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawArc::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (RS_COMMANDS->checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
        return;
    }

    if (RS_COMMANDS->checkCommand("reversed", c)) {
        e->accept();
		setReversed(!isReversed());

		RS_DIALOGFACTORY->requestOptions(this, true, true);
		return;
	}

    switch (getStatus()) {

    case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok) {
				data->radius = r;
                setStatus(SetAngle1);
                e->accept();
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
				data->angle1 = RS_Math::deg2rad(a);
                e->accept();
                setStatus(SetAngle2);
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            if (RS_COMMANDS->checkCommand("angle", c)) {
                setStatus(SetIncAngle);
            } else if (RS_COMMANDS->checkCommand("chord length", c)) {
                setStatus(SetChordLength);
            } else {
                bool ok;
                double a = RS_Math::eval(c, &ok);
                if (ok) {
					data->angle2 = RS_Math::deg2rad(a);
                    e->accept();
                    trigger();
				} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetIncAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
				data->angle2 = data->angle1 + RS_Math::deg2rad(a);
                e->accept();
                trigger();
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetChordLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
				if (fabs(l/(2*data->radius))<=1.0) {
					data->angle2 = data->angle1 + asin(l/(2*data->radius)) * 2;
                    trigger();
				} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid chord length"));
				e->accept();
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawArc::getAvailableCommands() {
    QStringList cmd;
    cmd += RS_COMMANDS->command("reversed");
    return cmd;
}


void RS_ActionDrawArc::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCenter:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify center"), tr("Cancel"));
		break;
	case SetRadius:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify radius"), tr("Back"));
		break;
	case SetAngle1:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify start angle:"), tr("Back"));
		break;
	case SetAngle2:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify end angle or [angle/chord length]"),
					tr("Back"));
		break;
	case SetIncAngle:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify included angle:"),
											tr("Back"));
		break;
	case SetChordLength:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify chord length:"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawArc::showOptions() {
    RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawArc::hideOptions() {
    RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawArc::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


bool RS_ActionDrawArc::isReversed() const{
	return data->reversed;
}

void RS_ActionDrawArc::setReversed(bool r) const{
	data->reversed = r;
}

// EOF

