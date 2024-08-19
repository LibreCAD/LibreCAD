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

#include "rs_actionpolylineadd.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_polyline.h"

RS_ActionPolylineAdd::RS_ActionPolylineAdd(RS_EntityContainer& container,
                                           RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Add node",
                               container, graphicView)
    , addCoord(std::make_unique<RS_Vector>()){
    setActionType(RS2::ActionPolylineAdd);
}

RS_ActionPolylineAdd::~RS_ActionPolylineAdd() = default;

void RS_ActionPolylineAdd::init(int status) {
    RS_PreviewActionInterface::init(status);
    polylineToModify = nullptr;
    addSegment = nullptr;
    *addCoord = {};
}

void RS_ActionPolylineAdd::trigger() {
    RS_PreviewActionInterface::trigger();
    RS_DEBUG->print("RS_ActionPolylineAdd::trigger()");

    if (polylineToModify && addSegment->isAtomic() && addCoord->valid &&
        addSegment->isPointOnEntity(*addCoord)) {
        graphicView->drawEntity(polylineToModify);

        RS_Modification m(*container, graphicView);
        RS_Polyline *createdPolyline = m.addPolylineNode(
            *polylineToModify,
            (RS_AtomicEntity &) *addSegment,
            *addCoord);
        if (createdPolyline != nullptr){
            polylineToModify = createdPolyline;
        }
        *addCoord = {};

        updateSelectionWidget();
    }
    graphicView->redraw(RS2::RedrawDrawing);
}

void RS_ActionPolylineAdd::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionPolylineAdd::mouseMoveEvent begin");
    snapPoint(e);
    deleteHighlights();
    int status = getStatus();
    switch (status) {
        case ChooseSegment: {
            auto polyline = dynamic_cast<RS_Polyline *>(catchEntity(e, RS2::EntityPolyline));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetAddCoord: {
            bool oldSnapOnEntity = snapMode.snapOnEntity;
            snapMode.snapOnEntity = true;
            RS_Vector snap = snapPoint(e);
            snapMode.snapOnEntity = oldSnapOnEntity;
            deletePreview();
            auto polyline = dynamic_cast<RS_Polyline *>(catchEntity(e, RS2::EntityPolyline));
            if (polyline == polylineToModify){
                RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(coordinate);
                RS_Entity * segment = catchEntity(coordinate, RS2::ResolveAll);
                highlightHover(segment);
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionPolylineAdd::mouseMoveEvent end");
}

void RS_ActionPolylineAdd::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case ChooseSegment: {
            auto en = catchEntity(e);
            if (!en){
                commandMessage(tr("No Entity found."));
            } else if (!isPolyline(en)){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                polylineToModify = dynamic_cast<RS_Polyline *>(en);
                polylineToModify->setSelected(true);
                graphicView->drawEntity(polylineToModify);
                setStatus(SetAddCoord);
            }
            break;
        }
        case SetAddCoord: {
            bool oldSnapOnEntity = snapMode.snapOnEntity;
            snapMode.snapOnEntity = true;
            RS_Vector snap = snapPoint(e);
            snapMode.snapOnEntity = oldSnapOnEntity;

            const RS_Vector newCoord = polylineToModify->getNearestPointOnEntity(snap, true);
            *addCoord = newCoord;
            if (!polylineToModify){
                commandMessage(tr("No Entity found."));
            } else if (!addCoord->valid){
                commandMessage(tr("Adding point is invalid."));
            } else {
                addSegment = nullptr;
                addSegment = catchEntity(newCoord, RS2::ResolveAll);
                if (!addSegment){
                    commandMessage(tr("Adding point is not on entity."));
                    break;
                }
                deleteSnapper();
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineAdd::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent *e) {
    deleteSnapper();
    finish(true);
}

void RS_ActionPolylineAdd::finish(bool updateTB){
    if (polylineToModify){
        polylineToModify->setSelected(false);
        graphicView->drawEntity(polylineToModify);
        graphicView->redraw(RS2::RedrawDrawing);
        polylineToModify = nullptr;
        addSegment = nullptr;
        *addCoord = {};
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionPolylineAdd::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseSegment:
            updateMouseWidgetTRCancel(tr("Specify polyline to add nodes"));
            break;
        case SetAddCoord:
            updateMouseWidgetTRBack(tr("Specify adding node's point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionPolylineAdd::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
