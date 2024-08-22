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

#include "rs_actionpolylinedel.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_polyline.h"
#include "lc_actionpolylinedeletebase.h"

RS_ActionPolylineDel::RS_ActionPolylineDel(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :LC_ActionPolylineDeleteBase("Delete node",
						   container, graphicView){
	actionType=RS2::ActionPolylineDel;
}

RS_ActionPolylineDel::~RS_ActionPolylineDel() = default;

void RS_ActionPolylineDel::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status <= SetPolyline){
        polylineToModify = nullptr;
    }
}

void RS_ActionPolylineDel::trigger(){
    RS_DEBUG->print("RS_ActionPolylineDel::trigger()");
    RS_Modification m(*container, graphicView);
    auto createdPolyline = m.deletePolylineNode(*polylineToModify, vertexToDelete, false);
    if (createdPolyline != nullptr){
        polylineToModify = createdPolyline;
        vertexToDelete = RS_Vector(false);
        deleteHighlights();
    }
    updateSelectionWidget();
    graphicView->redraw();
}

void RS_ActionPolylineDel::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionPolylineDel::mouseMoveEvent begin");

    snapPoint(e);
    int status = getStatus();
    deleteHighlights();
    switch (status) {
        case SetPolyline: {
            auto polyline = dynamic_cast<RS_Polyline *>(catchEntity(e));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetVertex1:{
            deletePreview();
            RS_Vector vertex;
            RS_Entity * segment;
            getSelectedPolylineVertex(e, vertex, segment);

            if (vertex.valid){
                highlightHover(segment);
                previewRefSelectablePoint(vertex);
                RS_Modification m(*preview, graphicView);
                m.deletePolylineNode(*polylineToModify, vertex, true);
            }
            drawPreview();
            break;
         }
        default:
            break;
    }
    drawHighlights();

    RS_DEBUG->print("RS_ActionPolylineDel::mouseMoveEvent end");
}

void RS_ActionPolylineDel::onMouseLeftButtonRelease(int status, QMouseEvent *e){
    switch (status) {
        case SetPolyline: {
            auto en = catchEntity(e);
            if (en == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (!isPolyline(en)){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                snapPoint(e);
                polylineToModify = dynamic_cast<RS_Polyline *>(en);
                polylineToModify->setSelected(true);
                graphicView->drawEntity(polylineToModify);
                setStatus(SetVertex1);
                graphicView->redraw();
            }
            break;
        }
        case SetVertex1: {
            if (polylineToModify == nullptr){
                commandMessage(tr("No Entity found."));
            } else {
                RS_Vector vertex;
                RS_Entity * segment;
                getSelectedPolylineVertex(e, vertex, segment);
                if (vertex.valid){
                    if (!polylineToModify->isPointOnEntity(vertex)){
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else{
                        vertexToDelete = vertex;
                        deleteSnapper();
                        trigger();
                    }
                }
                else{
                    commandMessage(tr("Deleting point is invalid."));
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineDel::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPolyline:
            updateMouseWidgetTRCancel(tr("Specify polyline to delete node"));
            break;
        case SetVertex1:
            updateMouseWidgetTRBack(tr("Specify deleting node's point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
