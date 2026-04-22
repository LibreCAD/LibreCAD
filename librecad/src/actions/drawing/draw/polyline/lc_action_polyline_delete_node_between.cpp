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

#include "lc_action_polyline_delete_node_between.h"

#include "lc_actioncontext.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_polyline.h"

LC_ActionPolylineDeleteNodeBetween::LC_ActionPolylineDeleteNodeBetween(LC_ActionContext* actionContext)
    : LC_ActionPolylineDeleteBase("ActionPolylineDelBetween", actionContext, RS2::ActionPolylineDelBetween) {
}

LC_ActionPolylineDeleteNodeBetween::~LC_ActionPolylineDeleteNodeBetween() = default;

void LC_ActionPolylineDeleteNodeBetween::doInitialInit() {
    m_polylineToModify = nullptr;
}

void LC_ActionPolylineDeleteNodeBetween::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

void LC_ActionPolylineDeleteNodeBetween::drawSnapper() {
    // disable snapper for action
}

bool LC_ActionPolylineDeleteNodeBetween::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Polyline* modifiedPolyline = RS_Modification::deletePolylineNodesBetween(m_polylineToModify, m_vertexToDelete, m_vertexToDelete2,
                                                                                ctx);
    if (modifiedPolyline != nullptr) {
        modifiedPolyline->setLayer(m_polylineToModify->getLayer(false));
        modifiedPolyline->setPen(m_polylineToModify->getPen(false));
        m_polylineToModify = modifiedPolyline;
        ctx.dontSetActiveLayerAndPen();
        return true;
    }
    return false;
}

void LC_ActionPolylineDeleteNodeBetween::doTriggerCompletion(const bool success) {
    if (success) {
        select(m_polylineToModify);
        setStatus(SetVertex1);
    }
    else {
        setStatus(SetVertex2);
    }
}

void LC_ActionPolylineDeleteNodeBetween::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetPolyline: {
            const auto polyline = dynamic_cast<RS_Polyline*>(catchAndDescribe(e));
            if (polyline != nullptr) {
                highlightHover(polyline);
            }
            break;
        }
        case SetVertex1: {
            RS_Vector vertex;
            RS_Entity* segment = nullptr;
            getSelectedPolylineVertex(e, vertex, segment);
            deleteSnapper();
            if (vertex.valid) {
                highlightHover(segment);
                previewRefSelectablePoint(vertex);
            }
            break;
        }
        case SetVertex2: {
            RS_Vector vertex;
            RS_Entity* segment = nullptr;
            getSelectedPolylineVertex(e, vertex, segment);
            deleteSnapper();
            previewRefSelectablePoint(m_vertexToDelete);

            if (vertex.valid) {
                // collect segments between points
                QList<RS_Entity*> entitiesToRemove;
                collectEntitiesToRemove(m_vertexToDelete, vertex, entitiesToRemove);
                if (!entitiesToRemove.isEmpty()) {
                    for (const auto er : std::as_const(entitiesToRemove)) {
                        highlightHover(er);
                    }
                    previewRefSelectablePoint(vertex);
                    LC_DocumentModificationBatch ctx;
                    const auto polyline = RS_Modification::deletePolylineNodesBetween(m_polylineToModify, m_vertexToDelete, vertex, ctx);
                    if (polyline != nullptr) {
                        previewEntity(polyline);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineDeleteNodeBetween::setPolylineToModify(RS_Entity* en) {
    if (en == nullptr) {
        commandMessage(tr("No Entity found."));
    }
    else if (!isPolyline(en)) {
        commandMessage(tr("Entity must be a polyline."));
    }
    else {
        m_polylineToModify = dynamic_cast<RS_Polyline*>(en);
        select(m_polylineToModify);
        setStatus(SetVertex1);
        redraw();
    }
}

void LC_ActionPolylineDeleteNodeBetween::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetPolyline: {
            const auto en = catchEntityByEvent(e);
            setPolylineToModify(en);
            break;
        }
        case SetVertex1: {
            if (m_polylineToModify == nullptr) {
                commandMessage(tr("No Entity found.")); // fixme - really? seems not needed check
            }
            else {
                RS_Vector vertex;
                RS_Entity* segment = nullptr;
                getSelectedPolylineVertex(e, vertex, segment);
                if (vertex.valid) {
                    if (!m_polylineToModify->isPointOnEntity(vertex)) {
                        // fixme - is it really needed?
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else {
                        m_vertexToDelete = vertex;
                        setStatus(SetVertex2);
                    }
                }
                else {
                    commandMessage(tr("Deleting point is invalid."));
                }
            }
            break;
        }
        case SetVertex2: {
            if (m_polylineToModify == nullptr) {
                commandMessage(tr("No polyline found.")); // fixme - really needed?
            }
            else {
                RS_Vector vertex;
                RS_Entity* segment = nullptr;
                getSelectedPolylineVertex(e, vertex, segment);

                if (vertex.valid) {
                    if (!m_polylineToModify->isPointOnEntity(vertex)) {
                        // fixme - is it really needed?
                        commandMessage(tr("Deleting point is not on entity."));
                    }
                    else {
                        QList<RS_Entity*> entitiesToRemove;
                        collectEntitiesToRemove(m_vertexToDelete, vertex, entitiesToRemove);
                        if (!entitiesToRemove.isEmpty()) {
                            m_vertexToDelete2 = vertex;
                            deleteSnapper();
                            trigger();
                        }
                        else {
                            commandMessage(tr("At least two segments of polyline should be between selected points."));
                        }
                    }
                }
                else {
                    commandMessage(tr("Deleting point is invalid."));
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineDeleteNodeBetween::updateActionPrompt() {
    switch (getStatus()) {
        case SetPolyline:
            updatePromptTRCancel(tr("Specify polyline to delete between two nodes"));
            break;
        case SetVertex1:
            updatePromptTRBack(tr("Specify first node"));
            break;
        case SetVertex2:
            updatePromptTRBack(tr("Specify second node"));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionPolylineDeleteNodeBetween::collectEntitiesToRemove(const RS_Vector& first, const RS_Vector& second, QList<RS_Entity*>& list) const {
    if (first.distanceTo(second) > RS_TOLERANCE) {
        bool found = false;

        for (unsigned int i = 0; i < m_polylineToModify->count(); i++) {
            auto* en = m_polylineToModify->entityAt(i);
            auto start = en->getStartpoint();

            if (LC_LineMath::isMeaningfulDistance(start, first) || LC_LineMath::isMeaningfulDistance(start, second)) {
                found = !found;
                if (!found) {
                    continue;
                }
            }
            if (found) {
                list << en;
            }
        }

        if (list.size() == 1) {
            // same entity or single segment, minimum 2 should be found
            list.clear();
        }
    }
}
