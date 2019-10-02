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

#include "rs_actionmodifymove.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionModifyMove::Points {
	RS_MoveData data;
	RS_Vector referencePoint;
	RS_Vector targetPoint;
};

RS_ActionModifyMove::RS_ActionModifyMove(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Move Entities",
						   container, graphicView)
		, pPoints(new Points{})
{
	actionType=RS2::ActionModifyMove;
}

RS_ActionModifyMove::~RS_ActionModifyMove() = default;

void RS_ActionModifyMove::trigger() {

    RS_DEBUG->print("RS_ActionModifyMove::trigger()");

    RS_Modification m(*container, graphicView);
	m.move(pPoints->data);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
	if (pPoints->data.number == 0) // move operation
		finish(false);
}



void RS_ActionModifyMove::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyMove::mouseMoveEvent begin");

    if (getStatus()==SetReferencePoint ||
            getStatus()==SetTargetPoint) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetReferencePoint:
			pPoints->referencePoint = mouse;
            break;

        case SetTargetPoint:
			if (pPoints->referencePoint.valid) {
				pPoints->targetPoint = mouse;
				pPoints->data.amount = mouse - pPoints->referencePoint;
				drawMovePreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyMove::mouseMoveEvent end");
}



void RS_ActionModifyMove::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        finish(true);
    }
}

void RS_ActionModifyMove::coordinateEvent(RS_CoordinateEvent* e) {

    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetReferencePoint:
		pPoints->referencePoint = pos;
		graphicView->moveRelativeZero(pPoints->referencePoint);
		if (RS_DIALOGFACTORY->requestMoveDialog(pPoints->data)) {
			if (pPoints->data.number < 0) {
				pPoints->data.number = abs(pPoints->data.number);
				RS_DIALOGFACTORY->commandMessage(tr("Invalid number of copies, use %1 ").arg(pPoints->data.number));
			}
		}
        setStatus(SetTargetPoint);
        break;

    case SetTargetPoint:
		graphicView->moveRelativeZero(pos);
		pPoints->targetPoint = pos;
		pPoints->data.amount = pos - pPoints->referencePoint;
		trigger();
        break;

    default:
        break;
    }
}


void RS_ActionModifyMove::updateMouseButtonHints() {
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



void RS_ActionModifyMove::updateMouseCursor() {
        if(graphicView != NULL){
    graphicView->setMouseCursor(RS2::CadCursor);
        }
}

void RS_ActionModifyMove::drawMovePreview()
{
	RS_Modification m(*container, graphicView);
	QList<RS_Entity*> previewEntities;

	deletePreview();
	m.move(pPoints->data, &previewEntities);
	for (auto e : previewEntities) {
		preview->addEntity(e);
	}

	drawPreview();
}

// EOF
