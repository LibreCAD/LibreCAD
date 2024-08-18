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
#include <QMouseEvent>

#include "rs_actiondrawlinerectangle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"

struct RS_ActionDrawLineRectangle::Points {
/**
 * 1st corner.
 */
    RS_Vector corner1;
/**
 * 2nd corner.
 */
    RS_Vector corner2;
};

RS_ActionDrawLineRectangle::RS_ActionDrawLineRectangle(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Draw rectangles",
                               container, graphicView), pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionDrawLineRectangle;
}

RS_ActionDrawLineRectangle::~RS_ActionDrawLineRectangle() = default;

void RS_ActionDrawLineRectangle::trigger(){
    RS_PreviewActionInterface::trigger();

    auto *polyline = new RS_Polyline(container);

// create and add rectangle:
    polyline->addVertex(pPoints->corner1);
    polyline->setLayerToActive();
    polyline->setPenToActive();
    polyline->addVertex({pPoints->corner2.x, pPoints->corner1.y});
    polyline->addVertex(pPoints->corner2);
    polyline->addVertex({pPoints->corner1.x, pPoints->corner2.y});
    polyline->setClosed(true);
    polyline->endPolyline();
    container->addEntity(polyline);

    addToDocumentUndoable(polyline);

    // upd. view
    graphicView->redraw(RS2::RedrawDrawing);
    moveRelativeZero(pPoints->corner2);
}

void RS_ActionDrawLineRectangle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status){
        case SetCorner1:{
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetCorner2:{
            if (pPoints->corner1.valid){
                pPoints->corner2 = mouse;
                deletePreview();
                preview->addRectangle(pPoints->corner1, pPoints->corner2);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->corner1);
                    previewRefPoint(pPoints->corner2);
                }
                drawPreview();
            }
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent end");
}

void RS_ActionDrawLineRectangle::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawLineRectangle::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineRectangle::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCorner1: {
            pPoints->corner1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetCorner2);
            break;
        }
        case SetCorner2: {
            pPoints->corner2 = mouse;
            trigger();
            setStatus(SetCorner1);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRectangle::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCorner1:
            updateMouseWidgetTRCancel(tr("Specify first corner"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCorner2:
            updateMouseWidgetTRBack(tr("Specify second corner"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineRectangle::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
