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
#include "rs_actionzoomwindow.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionZoomWindow::Points {
	RS_Vector v1;
	RS_Vector v2;
};


/**
 * Default constructor.
 *
 * @param keepAspectRatio Keep the aspect ratio. true: the factors
 *          in x and y will stay the same. false Exactly the chosen
 *          area will be fit to the viewport.
 */
RS_ActionZoomWindow::RS_ActionZoomWindow(RS_EntityContainer& container,
        RS_GraphicView& graphicView, bool keepAspectRatio)
        : RS_PreviewActionInterface("Zoom Window",
							container, graphicView)
		, pPoints(new Points{})
		, keepAspectRatio(keepAspectRatio)
{
}

RS_ActionZoomWindow::~RS_ActionZoomWindow() = default;


void RS_ActionZoomWindow::init(int status) {
    RS_DEBUG->print("RS_ActionZoomWindow::init()");

    RS_PreviewActionInterface::init(status);
	pPoints.reset(new Points{});
	//deleteSnapper();
   // snapMode.clear();
   // snapMode.restriction = RS2::RestrictNothing;
}



void RS_ActionZoomWindow::trigger() {
    RS_DEBUG->print("RS_ActionZoomWindow::trigger()");

    RS_PreviewActionInterface::trigger();

	if (pPoints->v1.valid && pPoints->v2.valid) {
        deletePreview();
        //deleteSnapper();
		if (graphicView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2))>5) {
			graphicView->zoomWindow(pPoints->v1, pPoints->v2, keepAspectRatio);
            init();
        }
    }
}



void RS_ActionZoomWindow::mouseMoveEvent(QMouseEvent* e) {
    snapFree(e);
    drawSnapper();
	if (getStatus()==SetSecondCorner && pPoints->v1.valid) {
		pPoints->v2 = snapFree(e);
        deletePreview();
		preview->addRectangle(pPoints->v1, pPoints->v2);
        drawPreview();
    }
}



void RS_ActionZoomWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetFirstCorner:
			pPoints->v1 = snapFree(e);
            drawSnapper();
            setStatus(SetSecondCorner);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionZoomWindow::mousePressEvent(): %f %f",
					pPoints->v1.x, pPoints->v1.y);
}



void RS_ActionZoomWindow::mouseReleaseEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");

    if (e->button()==Qt::RightButton) {
        if (getStatus()==SetSecondCorner) {
            deletePreview();
        }
        init(getStatus()-1);
    } else if (e->button()==Qt::LeftButton) {
        if (getStatus()==SetSecondCorner) {
			pPoints->v2 = snapFree(e);
			if( fabs(pPoints->v1.x-pPoints->v2.x) < RS_TOLERANCE
					|| fabs(pPoints->v1.y-pPoints->v2.y) < RS_TOLERANCE ) {//invalid zoom window
                deletePreview();
                init(getStatus()-1);
            }
            trigger();
        }
    }
}



void RS_ActionZoomWindow::updateMouseButtonHints() {
    RS_DEBUG->print("RS_ActionZoomWindow::updateMouseButtonHints()");

    switch (getStatus()) {
    case SetFirstCorner:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first edge"), tr("Cancel"));
        break;
    case SetSecondCorner:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second edge"), tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionZoomWindow::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::MagnifierCursor);
}


// EOF
