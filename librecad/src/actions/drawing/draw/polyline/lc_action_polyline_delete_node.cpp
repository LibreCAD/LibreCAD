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
#include "lc_action_polyline_delete_node.h"

#include "lc_actioncontext.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_polyline.h"

LC_ActionPolylineDeleteNode::LC_ActionPolylineDeleteNode(LC_ActionContext *actionContext)
    :LC_ActionPolylineDeleteBase("ActionPolylineDel",actionContext, RS2::ActionPolylineDel){
}

LC_ActionPolylineDeleteNode::~LC_ActionPolylineDeleteNode() = default;

void LC_ActionPolylineDeleteNode::doInitialInit() {
    m_polylineToModify = nullptr;
}

void LC_ActionPolylineDeleteNode::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

void LC_ActionPolylineDeleteNode::drawSnapper() {
    // completely disable snapper for action
}

bool LC_ActionPolylineDeleteNode::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const auto createdPolyline = RS_Modification::deletePolylineNode(m_polylineToModify, m_vertexToDelete, ctx);
    if (createdPolyline != nullptr) {
        createdPolyline->setLayer(m_polylineToModify->getLayer());
        createdPolyline->setPen(m_polylineToModify->getPen(false));
        ctx.dontSetActiveLayerAndPen();
    }
    m_polylineToModify = createdPolyline;
    m_vertexToDelete   = RS_Vector(false);
    deleteHighlights();
    return true;
}

void LC_ActionPolylineDeleteNode::doTriggerCompletion([[maybe_unused]]bool success) {
    if (m_polylineToModify != nullptr) {
        select(m_polylineToModify);
    }
    else {
        setStatus(SetPolyline);
    }
}

void LC_ActionPolylineDeleteNode::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetPolyline: {
            const auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e, RS2::ResolveNone));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetVertex1:{
            RS_Vector vertex;
            RS_Entity * segment = nullptr;
            getSelectedPolylineVertex(e, vertex, segment);

            if (vertex.valid){
                highlightHover(segment);
                previewRefSelectablePoint(vertex);
                LC_DocumentModificationBatch ctx;
                const auto modifiedPolyline = RS_Modification::deletePolylineNode(m_polylineToModify, vertex, ctx);
                if (modifiedPolyline != nullptr) {
                    previewEntity(modifiedPolyline);
                }
            }
            break;
         }
        default:
            break;
    }
}

void LC_ActionPolylineDeleteNode::setPolylineToModify(RS_Entity* en) {
    if (en == nullptr){
        commandMessage(tr("No Entity found."));
    } else if (!isPolyline(en)){
        commandMessage(tr("Entity must be a polyline."));
    } else {
        m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
        select(m_polylineToModify);
        setStatus(SetVertex1);
        redraw();
    }
}

void LC_ActionPolylineDeleteNode::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e){
    switch (status) {
        case SetPolyline: {
            const auto en = catchEntityByEvent(e);
            setPolylineToModify(en);
            break;
        }
        case SetVertex1: {
            if (m_polylineToModify == nullptr){
                commandMessage(tr("No Entity found."));
            } else {
                RS_Vector vertex;
                RS_Entity * segment = nullptr;
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

void LC_ActionPolylineDeleteNode::updateActionPrompt(){
    switch (getStatus()) {
        case SetPolyline:
            updatePromptTRCancel(tr("Specify polyline to delete node"));
            break;
        case SetVertex1:
            updatePromptTRBack(tr("Specify deleting node's point"));
            break;
        default:
            updatePrompt();
            break;
    }
}
