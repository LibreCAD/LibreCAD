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

#include "rs_actionmodifyrotate.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionModifyRotate::RS_ActionModifyRotate(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Rotate Entities",
							   container, graphicView)
	,data(new RS_RotateData())
{
	actionType=RS2::ActionModifyRotate;
}

RS_ActionModifyRotate::~RS_ActionModifyRotate() = default;

void RS_ActionModifyRotate::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionModifyRotate::trigger() {

    RS_DEBUG->print("RS_ActionModifyRotate::trigger()");

    RS_Modification m(*container, graphicView);
	m.rotate(*data);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
}

void RS_ActionModifyRotate::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
    case setCenterPoint:
    case setReferencePoint:
        break;

    case setTargetPoint:
        if( ! mouse.valid ) return;
        deletePreview();
        preview->addSelectionFrom(*container);
		preview->rotate(data->center,RS_Math::correctAngle((mouse - data->center).angle() - data->angle));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent end");
}

void RS_ActionModifyRotate::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyRotate::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();
    if (! pos.valid ) {
        return;
    }
    switch (getStatus()) {
    case setCenterPoint:
		data->center = pos;
		graphicView->moveRelativeZero(data->center);
        setStatus(setReferencePoint);
        break;
    case setReferencePoint:
		pos -= data->center;
        if ( pos.squared()< RS_TOLERANCE2 ) {
			data->angle = 0.;//angle not well defined, go direct to dialog
            setStatus(ShowDialog);
			if (RS_DIALOGFACTORY->requestRotateDialog(*data)) {
                trigger();
                finish(false);
            }
        } else {
			data->angle=pos.angle();
            setStatus(setTargetPoint);
        }
        break;
    case setTargetPoint:
		pos -= data->center;
        if ( pos.squared()< RS_TOLERANCE2 ) {
			data->angle = 0.;//angle not well defined
        } else {
			data->angle = RS_Math::correctAngle(pos.angle() - data->angle);
        }
        setStatus(ShowDialog);
		if (RS_DIALOGFACTORY->requestRotateDialog(*data)) {
            trigger();
            finish(false);
        }
        break;

    default:
        break;
    }
}

void RS_ActionModifyRotate::updateMouseButtonHints() {
    switch (getStatus()) {
    case setCenterPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify rotation center"),
                                            tr("Back"));
        break;

    case setReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Back"));
        break;
    case setTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point to rotate to"),
                                            tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionModifyRotate::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
