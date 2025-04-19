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
#include "rs_actionpolylinedel.h"

#include "rs_debug.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionPolylineDel::RS_ActionPolylineDel(LC_ActionContext *actionContext)
    :LC_ActionPolylineDeleteBase("Delete node",actionContext, RS2::ActionPolylineDel){
}

RS_ActionPolylineDel::~RS_ActionPolylineDel() = default;

void RS_ActionPolylineDel::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status <= SetPolyline){
        m_polylineToModify = nullptr;
    }
}

void RS_ActionPolylineDel::drawSnapper() {
    // completely disable snapper for action
}

void RS_ActionPolylineDel::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineDel::trigger()");
    RS_Modification m(*m_container, m_viewport);
    auto createdPolyline = m.deletePolylineNode(*m_polylineToModify, m_vertexToDelete, false);
    if (createdPolyline != nullptr){
        m_polylineToModify = createdPolyline;
        m_vertexToDelete = RS_Vector(false);
        deleteHighlights();
    }
}

void RS_ActionPolylineDel::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetPolyline: {
            auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e, RS2::ResolveNone));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetVertex1:{

            RS_Vector vertex;
            RS_Entity * segment;
            getSelectedPolylineVertex(e, vertex, segment);

            if (vertex.valid){
                highlightHover(segment);
                previewRefSelectablePoint(vertex);
                RS_Modification m(*m_preview, m_viewport);
                m.deletePolylineNode(*m_polylineToModify, vertex, true);
            }
            break;
         }
        default:
            break;
    }
}

void RS_ActionPolylineDel::onMouseLeftButtonRelease(int status, LC_MouseEvent *e){
    switch (status) {
        case SetPolyline: {
            auto en = catchEntityByEvent(e);
            if (en == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (!isPolyline(en)){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
                m_polylineToModify->setSelected(true);
                setStatus(SetVertex1);
                redraw();
            }
            break;
        }
        case SetVertex1: {
            if (m_polylineToModify == nullptr){
                commandMessage(tr("No Entity found."));
            } else {
                RS_Vector vertex;
                RS_Entity * segment;
                getSelectedPolylineVertex(e, vertex, segment);
                if (vertex.valid){
                    if (!m_polylineToModify->isPointOnEntity(vertex)){
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else{
                        m_vertexToDelete = vertex;
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
    invalidateSnapSpot();
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
