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
#include "rs_actionmodifymirror.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionModifyMirror::Points {
	RS_MirrorData data;
	RS_Vector axisPoint1;
	RS_Vector axisPoint2;
};

RS_ActionModifyMirror::RS_ActionModifyMirror(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Mirror Entities",
						   container, graphicView)
		, pPoints(new Points{})
{
	actionType=RS2::ActionModifyMirror;
}

RS_ActionModifyMirror::~RS_ActionModifyMirror() = default;

void RS_ActionModifyMirror::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionModifyMirror::trigger() {

    RS_DEBUG->print("RS_ActionModifyMirror::trigger()");

    RS_Modification m(*container, graphicView);
	m.mirror(pPoints->data);

	RS_DIALOGFACTORY->updateSelectionWidget(
				container->countSelected(), container->totalSelectedLength());
}

void RS_ActionModifyMirror::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent begin");

    if (getStatus()==SetAxisPoint1 ||
            getStatus()==SetAxisPoint2) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetAxisPoint1:
			pPoints->axisPoint1 = mouse;
            break;

        case SetAxisPoint2:
			if (pPoints->axisPoint1.valid) {
				pPoints->axisPoint2 = mouse;

                deletePreview();
                preview->addSelectionFrom(*container);
				preview->mirror(pPoints->axisPoint1, pPoints->axisPoint2);

				preview->addEntity(new RS_Line{preview.get(),
											   pPoints->axisPoint1, pPoints->axisPoint2});

                drawPreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent end");
}

void RS_ActionModifyMirror::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);

    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyMirror::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

        switch (getStatus()) {
        case SetAxisPoint1:
			pPoints->axisPoint1 = mouse;
            setStatus(SetAxisPoint2);
                graphicView->moveRelativeZero(mouse);
            break;

        case SetAxisPoint2:
			pPoints->axisPoint2 = mouse;
            setStatus(ShowDialog);
                graphicView->moveRelativeZero(mouse);
				if (RS_DIALOGFACTORY->requestMirrorDialog(pPoints->data)) {
					pPoints->data.axisPoint1 = pPoints->axisPoint1;
					pPoints->data.axisPoint2 = pPoints->axisPoint2;
                    deletePreview();
                    trigger();
                    finish(false);
                }
            break;

        default:
            break;
        }
}

void RS_ActionModifyMirror::updateMouseButtonHints() {
	switch (getStatus()) {
	/*case Select:
				RS_DIALOGFACTORY->updateMouseWidget(tr("Pick entities to move"),
											   tr("Cancel"));
				break;*/
	case SetAxisPoint1:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify first point of mirror line"),
					tr("Cancel"));
		break;
	case SetAxisPoint2:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify second point of mirror line"),
					tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionModifyMirror::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
