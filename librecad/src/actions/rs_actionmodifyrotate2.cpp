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

#include "rs_actionmodifyrotate2.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "rs_modification.h"
#include "rs_debug.h"

RS_ActionModifyRotate2::RS_ActionModifyRotate2(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Rotate Entities around two centers",
						   container, graphicView)
		,data(new RS_Rotate2Data())
{
	actionType=RS2::ActionModifyRotate2;
}

RS_ActionModifyRotate2::~RS_ActionModifyRotate2() = default;


void RS_ActionModifyRotate2::init(int status) {
	RS_ActionInterface::init(status);
}

void RS_ActionModifyRotate2::trigger() {

    RS_DEBUG->print("RS_ActionModifyRotate2::trigger()");

    RS_Modification m(*container, graphicView);
	m.rotate2(*data);

    finish(false);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
}

void RS_ActionModifyRotate2::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyRotate2::mouseMoveEvent begin");

    if (getStatus()==SetReferencePoint1 ||
            getStatus()==SetReferencePoint2) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetReferencePoint1:
			data->center1 = mouse;
            break;

        case SetReferencePoint2:
			if (data->center1.valid) {
				data->center2 = mouse;
				//data->offset = data->center2-data->center1;

                /*deletePreview();
                preview->addSelectionFrom(*container);
				preview->rotate(data->center1, data->angle);
				preview->move(data->offset);
                drawPreview();
                */
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyRotate2::mouseMoveEvent end");
}

void RS_ActionModifyRotate2::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyRotate2::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetReferencePoint1:
		data->center1 = pos;
        setStatus(SetReferencePoint2);
        break;

    case SetReferencePoint2:
		data->center2 = pos;
        setStatus(ShowDialog);
		if (RS_DIALOGFACTORY->requestRotate2Dialog(*data)) {
            trigger();
            //finish();
        }
        break;

    default:
        break;
    }
}

void RS_ActionModifyRotate2::commandEvent(RS_CommandEvent* /*e*/) {
}

QStringList RS_ActionModifyRotate2::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}

void RS_ActionModifyRotate2::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify absolute reference point"),
                                            tr("Cancel"));
        break;
    case SetReferencePoint2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify relative reference point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionModifyRotate2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
