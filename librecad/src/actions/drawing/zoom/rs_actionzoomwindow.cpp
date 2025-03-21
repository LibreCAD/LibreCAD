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
#include <QMouseEvent>

#include "rs_actionzoomwindow.h"
#include "rs_debug.h"

#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionZoomWindow::Points {
 RS_Vector ucsV1;
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
RS_ActionZoomWindow::RS_ActionZoomWindow(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView, bool keepAspectRatio)
    :RS_PreviewActionInterface("Zoom Window",
                               container, graphicView), pPoints(std::make_unique<Points>()), keepAspectRatio(keepAspectRatio){
}

RS_ActionZoomWindow::~RS_ActionZoomWindow() = default;

void RS_ActionZoomWindow::init(int status){
    RS_DEBUG->print("RS_ActionZoomWindow::init()");

    RS_PreviewActionInterface::init(status);
    pPoints.reset(new Points{});
//deleteSnapper();
    // snapMode.clear();
    // snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionZoomWindow::doTrigger() {
    RS_DEBUG->print("RS_ActionZoomWindow::trigger()");
    if (pPoints->v1.valid && pPoints->v2.valid){
        if (viewport->toGuiDX(pPoints->v1.distanceTo(pPoints->v2)) > 5){
            RS_Vector point1 = toUCS(pPoints->v1);
            RS_Vector point2 = toUCS(pPoints->v2);
            viewport->zoomWindow(point1, point2, keepAspectRatio);
            init(SetFirstCorner);
        }
    }
}

void RS_ActionZoomWindow::mouseMoveEvent(QMouseEvent *e){
    deletePreview();
    snapFree(e);
    drawSnapper();
    if (getStatus() == SetSecondCorner && pPoints->v1.valid){
        pPoints->v2 = snapFree(e);

        RS_Vector worldCorner1 = pPoints->v1;
        RS_Vector worldCorner3 = pPoints->v2;

        RS_Vector worldCorner2,worldCorner4;
        calcRectCorners(worldCorner1, worldCorner3, worldCorner2, worldCorner4);

        preview->addRectangle(worldCorner1, worldCorner2, worldCorner3, worldCorner4);
    }
    drawPreview();
}

void RS_ActionZoomWindow::mousePressEvent(QMouseEvent *e){
    if (e->button() == Qt::LeftButton){
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

void RS_ActionZoomWindow::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");
    if (status == SetSecondCorner){
        pPoints->v2 = e->graphPoint;
        if (fabs(pPoints->v1.x - pPoints->v2.x) < RS_TOLERANCE
            || fabs(pPoints->v1.y - pPoints->v2.y) < RS_TOLERANCE){//invalid zoom window
            deletePreview();
            initPrevious(status);
        }
        trigger();
    }
}

void RS_ActionZoomWindow::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");
    if (status == SetSecondCorner){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionZoomWindow::updateMouseButtonHints(){
    RS_DEBUG->print("RS_ActionZoomWindow::updateMouseButtonHints()");

    switch (getStatus()) {
        case SetFirstCorner:
            updateMouseWidgetTRCancel(tr("Specify first edge"));
            break;
        case SetSecondCorner:
            updateMouseWidgetTRBack(tr("Specify second edge"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionZoomWindow::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::MagnifierCursor;
}
