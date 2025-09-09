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

#include "rs_actionpolylinedelbetween.h"

#include "lc_actioncontext.h"
#include "rs_debug.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionPolylineDelBetween::RS_ActionPolylineDelBetween(LC_ActionContext *actionContext)
    :LC_ActionPolylineDeleteBase("Delete between two nodes", actionContext, RS2::ActionPolylineDelBetween){
}

RS_ActionPolylineDelBetween::~RS_ActionPolylineDelBetween() = default;

void RS_ActionPolylineDelBetween::doInitialInit() {
    m_polylineToModify = nullptr;
}

void RS_ActionPolylineDelBetween::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

void RS_ActionPolylineDelBetween::drawSnapper() {
    // disable snapper for action
}

void RS_ActionPolylineDelBetween::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineDelBetween::trigger()");

    RS_Modification m(*m_container, m_viewport);
    RS_Polyline *modifiedPolyline = m.deletePolylineNodesBetween(*m_polylineToModify, m_vertexToDelete, m_vertexToDelete2, false);
    if (modifiedPolyline != nullptr){
        m_polylineToModify = modifiedPolyline;
        setStatus(SetVertex1);
    }
    else{
        setStatus(SetVertex2);
    }
}

void RS_ActionPolylineDelBetween::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetPolyline: {
            auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetVertex1: {
            RS_Vector vertex;
            RS_Entity *segment;
            getSelectedPolylineVertex(e, vertex, segment);
            deleteSnapper();
            if (vertex.valid){
                highlightHover(segment);
                previewRefSelectablePoint(vertex);
            }
            break;
        }
        case SetVertex2: {
            RS_Vector vertex;
            RS_Entity *segment;
            getSelectedPolylineVertex(e, vertex, segment);
            deleteSnapper();
            previewRefSelectablePoint(m_vertexToDelete);

            if (vertex.valid){
                // collect segments between points
                QList<RS_Entity *> entitiesToRemove;
                collectEntitiesToRemove(m_vertexToDelete, vertex, entitiesToRemove);
                if (!entitiesToRemove.isEmpty()){
                    for (auto er: entitiesToRemove) {
                        highlightHover(er);
                    }
                    previewRefSelectablePoint(vertex);
                    RS_Modification m(*m_preview, m_viewport);
                    m.deletePolylineNodesBetween(*m_polylineToModify, m_vertexToDelete, vertex  , true);
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineDelBetween::setPolylineToModify(RS_Entity* en) {
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
}

void RS_ActionPolylineDelBetween::onMouseLeftButtonRelease(int status, LC_MouseEvent *e){
    switch (status) {
        case SetPolyline:{
            auto en = catchEntityByEvent(e);
            setPolylineToModify(en);
            break;
        }
        case SetVertex1:{
            if (m_polylineToModify == nullptr){
                commandMessage(tr("No Entity found.")); // fixme - really? seems not needed check
            } else {
                RS_Vector vertex;
                RS_Entity * segment;
                getSelectedPolylineVertex(e, vertex, segment);
                if (vertex.valid){
                    if (!m_polylineToModify->isPointOnEntity(vertex)){ // fixme - is it really needed?
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else{
                        m_vertexToDelete = vertex;
                        setStatus(SetVertex2);
                    }
                }
                else{
                    commandMessage(tr("Deleting point is invalid."));
                }
            }
            break;
        }
        case SetVertex2:{
            if (m_polylineToModify == nullptr){
                commandMessage(tr("No polyline found.")); // fixme - really needed?
            } else{
                RS_Vector vertex;
                RS_Entity * segment;
                getSelectedPolylineVertex(e, vertex, segment);

                if (vertex.valid){
                    if (!m_polylineToModify->isPointOnEntity(vertex)){ // fixme - is it really needed?
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else{
                        QList<RS_Entity*> entitiesToRemove;
                        collectEntitiesToRemove(m_vertexToDelete, vertex, entitiesToRemove);
                        if (!entitiesToRemove.isEmpty()){
                            m_vertexToDelete2 = vertex;
                            deleteSnapper();
                            trigger();
                        }
                        else{
                            commandMessage(tr("At least two segments of polyline should be between selected points."));
                        }
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

void RS_ActionPolylineDelBetween::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPolyline:
            updateMouseWidgetTRCancel(tr("Specify polyline to delete between two nodes"));
            break;
        case SetVertex1:
            updateMouseWidgetTRBack(tr("Specify first node"));
            break;
        case SetVertex2:
            updateMouseWidgetTRBack(tr("Specify second node"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionPolylineDelBetween::collectEntitiesToRemove(RS_Vector first, RS_Vector second, QList<RS_Entity *> &list){
    if (first.distanceTo(second) > RS_TOLERANCE){
        bool found = false;
        for (unsigned int i = 0; i < m_polylineToModify->count(); i++){
            auto* en = m_polylineToModify->entityAt(i);
            auto start = en->getStartpoint();

            if (start == first || start == second){
                found = !found;
                if (!found){
                    continue;
                }
            }
            if (found){
                list << en;
            }
        }

        if (list.size() == 1){ // same entity or single segment, minimum 2 should be found
            list.clear();
        }
    }
}
