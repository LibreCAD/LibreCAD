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
#include "rs_actiondrawcirclecr.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

/**
 * Constructor.
 */
RS_ActionDrawCircleCR::RS_ActionDrawCircleCR(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circles CR",
						   container, graphicView)
		,data(new RS_CircleData())
{
	actionType=RS2::ActionDrawCircleCR;

    reset();
}



RS_ActionDrawCircleCR::~RS_ActionDrawCircleCR() = default;


void RS_ActionDrawCircleCR::reset() {
	data.reset(new RS_CircleData{RS_Vector{false}, 0.0});
}



void RS_ActionDrawCircleCR::init(int status) {
    RS_PreviewActionInterface::init(status);
}



void RS_ActionDrawCircleCR::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Circle* circle = new RS_Circle(container,
									  *data);
    circle->setLayerToActive();
    circle->setPenToActive();

    switch(getStatus()) {
    	case SetCenter:
    		container->addEntity(circle);
		graphicView->moveRelativeZero(circle->getCenter());
		break;
	case SetRadius:
		break;
    }

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(circle);
        document->endUndoCycle();
    }
        graphicView->redraw(RS2::RedrawDrawing);

    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawCircleCR::trigger(): circle added: %d",
                    circle->getId());
}

void RS_ActionDrawCircleCR::setRadius(double r)
{
    if(r>RS_TOLERANCE){
		data->radius=r;
    }else{
        RS_DIALOGFACTORY->commandMessage(tr("radius=%1 is invalid").arg(r));
    }
}


void RS_ActionDrawCircleCR::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircleCR::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetCenter:
		data->center = mouse;
        deletePreview();
		preview->addEntity(new RS_Circle(preview.get(),
										 *data));
        drawPreview();
        break;
    }

    RS_DEBUG->print("RS_ActionDrawCircleCR::mouseMoveEvent end");
}



void RS_ActionDrawCircleCR::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawCircleCR::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
		data->center = mouse;
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawCircleCR::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetCenter:
        if (checkCommand("radius", c)) {
            deletePreview();
            setStatus(SetRadius);
        }
        break;

    case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
			if (ok) {
				data->radius = r;
                e->accept();
                trigger();
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawCircleCR::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetCenter:
        cmd += command("radius");
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawCircleCR::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCenter:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify circle center"),
                                            tr("Cancel"));
        break;
    case SetRadius:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify circle radius"),
                                            tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawCircleCR::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawCircleCR::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawCircleCR::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


double RS_ActionDrawCircleCR::getRadius() const{
	return data->radius;
}
// EOF

