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

#include "rs_actionmodifymoverotate.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionModifyMoveRotate::Points {
	RS_MoveRotateData data;
	RS_Vector targetPoint;
};

RS_ActionModifyMoveRotate::RS_ActionModifyMoveRotate(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Move and Rotate Entities",
						   container, graphicView)
		, pPoints(new Points())
{
	actionType=RS2::ActionModifyMoveRotate;
}

RS_ActionModifyMoveRotate::~RS_ActionModifyMoveRotate() = default;

void RS_ActionModifyMoveRotate::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionModifyMoveRotate::trigger() {

    RS_DEBUG->print("RS_ActionModifyMoveRotate::trigger()");

    RS_Modification m(*container, graphicView);
	m.moveRotate(pPoints->data);

	if (pPoints->data.number == 0) // move operation
		finish(false);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
}



void RS_ActionModifyMoveRotate::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent begin");

    if (getStatus()==SetReferencePoint ||
            getStatus()==SetTargetPoint) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetReferencePoint:
			pPoints->data.referencePoint = mouse;
            break;

        case SetTargetPoint:
			if (pPoints->data.referencePoint.valid) {
				pPoints->targetPoint = mouse;
				pPoints->data.amount = pPoints->targetPoint-pPoints->data.referencePoint;

				drawMovePreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyMoveRotate::mouseMoveEvent end");
}



void RS_ActionModifyMoveRotate::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
		finish(true);
    }
}



void RS_ActionModifyMoveRotate::coordinateEvent(RS_CoordinateEvent* e) {
	if (e==nullptr) return;

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetReferencePoint:
		pPoints->data.referencePoint = pos;
		graphicView->moveRelativeZero(pos);
		if (RS_DIALOGFACTORY->requestMoveRotateDialog(pPoints->data))
			setStatus(SetTargetPoint);
		else
			finish(true);
        break;

    case SetTargetPoint:
		pPoints->targetPoint = pos;
        pPoints->data.amount = pPoints->targetPoint - pPoints->data.referencePoint;
		trigger();        
        break;

    default:
        break;
    }
}


void RS_ActionModifyMoveRotate::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }
}


void RS_ActionModifyMoveRotate::showOptions() {
    //std::cout << "RS_ActionModifyMoveRotate::showOptions()\n";

    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyMoveRotate::hideOptions() {
    //std::cout << "RS_ActionModifyMoveRotate::hideOptions()\n";

    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionModifyMoveRotate::setAngle(double a) {
	pPoints->data.angle = a;
}
double RS_ActionModifyMoveRotate::getAngle() const{
	return pPoints->data.angle;
}

void RS_ActionModifyMoveRotate::drawMovePreview()
{
	RS_Modification m(*container, graphicView);
	QList<RS_Entity*> previewEntities;

	deletePreview();
	m.moveRotate(pPoints->data, &previewEntities);
	for (auto e : previewEntities) {
		preview->addEntity(e);
	}

	drawPreview();
}


void RS_ActionModifyMoveRotate::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
                                            tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionModifyMoveRotate::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
