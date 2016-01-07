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
#include "rs_actionmodifystretch.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionModifyStretch::Points {
	RS_Vector firstCorner;
	RS_Vector secondCorner;
	RS_Vector referencePoint;
	RS_Vector targetPoint;
};

RS_ActionModifyStretch::RS_ActionModifyStretch(RS_EntityContainer& container,
											   RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Stretch Entities",
							   container, graphicView)
	, pPoints(new Points{})
{
	actionType=RS2::ActionModifyStretch;
}

void RS_ActionModifyStretch::init(int status) {
    RS_ActionInterface::init(status);
}

RS_ActionModifyStretch::~RS_ActionModifyStretch() = default;


void RS_ActionModifyStretch::trigger() {

    RS_DEBUG->print("RS_ActionModifyStretch::trigger()");

    deletePreview();

    RS_Modification m(*container, graphicView);
	m.stretch(pPoints->firstCorner,
			  pPoints->secondCorner,
			  pPoints->targetPoint - pPoints->referencePoint);

    setStatus(SetFirstCorner);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
}



void RS_ActionModifyStretch::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case SetFirstCorner:
        break;

    case SetSecondCorner:
		if (pPoints->firstCorner.valid) {
			pPoints->secondCorner = snapPoint(e);
			deletePreview();
			preview->addRectangle(pPoints->firstCorner, pPoints->secondCorner);
            drawPreview();
        }
        break;

    case SetReferencePoint:
        break;

    case SetTargetPoint:
		if (pPoints->referencePoint.valid) {
			pPoints->targetPoint = mouse;

            deletePreview();
			preview->addStretchablesFrom(*container, pPoints->firstCorner, pPoints->secondCorner);
            //preview->move(targetPoint-referencePoint);
			preview->stretch(pPoints->firstCorner, pPoints->secondCorner,
							 pPoints->targetPoint-pPoints->referencePoint);
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent end");
}



void RS_ActionModifyStretch::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyStretch::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetFirstCorner:
		pPoints->firstCorner = mouse;
        setStatus(SetSecondCorner);
        break;

    case SetSecondCorner:
		pPoints->secondCorner = mouse;
        deletePreview();
        setStatus(SetReferencePoint);
        break;

    case SetReferencePoint:
		pPoints->referencePoint = mouse;
		graphicView->moveRelativeZero(pPoints->referencePoint);
        setStatus(SetTargetPoint);
        break;

    case SetTargetPoint:
		pPoints->targetPoint = mouse;
		graphicView->moveRelativeZero(pPoints->targetPoint);
        trigger();
        //finish();
        break;

    default:
        break;
    }

}


void RS_ActionModifyStretch::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFirstCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first corner"),
                                            tr("Cancel"));
        break;
    case SetSecondCorner:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second corner"),
                                            tr("Back"));
        break;
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Back"));
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



void RS_ActionModifyStretch::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
