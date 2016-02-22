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
#include "rs_actiondrawlineangle.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_settings.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLineAngle::Points {
	/**
	 * Line data defined so far.
	 */
	RS_LineData data;
	/**
	 * Position.
	 */
	RS_Vector pos;
	/**
	 * Line angle.
	 */
	double angle;
	/**
	 * Line length.
	 */
	double length{1.};
	/**
	 * Is the angle fixed?
	 */
	bool fixedAngle;
	/**
	 * Snap point (start, middle, end).
	 */
	int snpPoint{0};
};

RS_ActionDrawLineAngle::RS_ActionDrawLineAngle(RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        double angle,
        bool fixedAngle, RS2::ActionType actionType)
        :RS_PreviewActionInterface("Draw lines with given angle",
						   container, graphicView)
		, pPoints(new Points{})
{

    this->actionType=actionType;
	pPoints->angle = angle;
	pPoints->fixedAngle = fixedAngle;

    RS_DIALOGFACTORY->requestOptions(this, true,false);
    reset();
}



RS_ActionDrawLineAngle::~RS_ActionDrawLineAngle() {
    RS_SETTINGS->beginGroup("/Draw");
    if (!hasFixedAngle()) {
        RS_SETTINGS->writeEntry("/LineAngleAngle", RS_Math::rad2deg(getAngle()));
    }
    RS_SETTINGS->writeEntry("/LineAngleLength", getLength());
    RS_SETTINGS->writeEntry("/LineAngleSnapPoint", getSnapPoint());
    RS_SETTINGS->endGroup();
}


void RS_ActionDrawLineAngle::reset() {
	pPoints->data = {{}, {}};
}

void RS_ActionDrawLineAngle::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}

void RS_ActionDrawLineAngle::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
	RS_Line* line = new RS_Line{container, pPoints->data};
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }

	graphicView->moveRelativeZero(pPoints->data.startpoint);
        graphicView->redraw(RS2::RedrawDrawing);
    RS_DEBUG->print("RS_ActionDrawLineAngle::trigger(): line added: %d",
                    line->getId());
}

void RS_ActionDrawLineAngle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent begin");

    if (getStatus()==SetPos) {
		pPoints->pos = snapPoint(e);
        deletePreview();
        preparePreview();
		preview->addEntity(new RS_Line(preview.get(),
									   pPoints->data));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent end");
}

void RS_ActionDrawLineAngle::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        if (getStatus()==SetPos) {
            RS_CoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionDrawLineAngle::preparePreview() {
    RS_Vector p1, p2;
    // End:
	if (pPoints->snpPoint == 2) {
		p2.setPolar(-pPoints->length, pPoints->angle);
    } else {
		p2.setPolar(pPoints->length, pPoints->angle);
    }

    // Middle:
	if (pPoints->snpPoint == 1) {
		p1 = pPoints->pos - (p2 / 2);
    } else {
		p1 = pPoints->pos;
    }

    p2 += p1;
	pPoints->data = {p1, p2};
}

void RS_ActionDrawLineAngle::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

    switch (getStatus()) {
    case SetPos:
		pPoints->pos = e->getCoordinate();
        trigger();
        break;

    default:
        break;
    }
}

void RS_ActionDrawLineAngle::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetPos:
		if (!pPoints->fixedAngle && checkCommand("angle", c)) {
            deletePreview();
            setStatus(SetAngle);
        } else if (checkCommand("length", c)) {
            deletePreview();
            setStatus(SetLength);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
				pPoints->angle = RS_Math::deg2rad(a);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
				pPoints->length = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    default:
        break;
    }
}

void RS_ActionDrawLineAngle::setSnapPoint(int sp) {
	pPoints->snpPoint = sp;
}

int RS_ActionDrawLineAngle::getSnapPoint() const{
	return pPoints->snpPoint;
}

void RS_ActionDrawLineAngle::setAngle(double a) {
	pPoints->angle = a;
}

double RS_ActionDrawLineAngle::getAngle() const{
	return pPoints->angle;
}

void RS_ActionDrawLineAngle::setLength(double l) {
	pPoints->length = l;
}

double RS_ActionDrawLineAngle::getLength() const{
	return pPoints->length;
}

bool RS_ActionDrawLineAngle::hasFixedAngle() const{
	switch(rtti()){
	case RS2::ActionDrawLineHorizontal:
	case RS2::ActionDrawLineVertical:
		return true;
	default:
		return false;
	}
}

QStringList RS_ActionDrawLineAngle::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetPos:
		if (!pPoints->fixedAngle) {
            cmd += command("angle");
        }
        cmd += command("length");
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionDrawLineAngle::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify position"),
                                            tr("Cancel"));
        break;

    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter angle:"), tr("Back"));
        break;

    case SetLength:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length:"), tr("Back"));
        break;

    default:
        break;
    }
}

void RS_ActionDrawLineAngle::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true,true);
}

void RS_ActionDrawLineAngle::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawLineAngle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
