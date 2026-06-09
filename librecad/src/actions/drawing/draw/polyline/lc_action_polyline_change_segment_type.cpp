/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_action_polyline_change_segment_type.h"

#include "lc_actioncontext.h"
#include "lc_containertraverser.h"
#include "lc_creation_arc.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_pen.h"
#include "rs_polyline.h"

LC_ActionPolylineChangeSegmentType::LC_ActionPolylineChangeSegmentType(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionPolylineChangeSegmentType",actionContext, RS2::ActionPolylineChangeSegmentType) {
}

LC_ActionPolylineChangeSegmentType::~LC_ActionPolylineChangeSegmentType() = default;

void LC_ActionPolylineChangeSegmentType::doInitialInit() {
    m_polyline = nullptr;
    m_arcPoint = RS_Vector();
    m_polylineSegment = nullptr;
}

void LC_ActionPolylineChangeSegmentType::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& pos) {
    setPolylineToModify(contextEntity);
}

bool LC_ActionPolylineChangeSegmentType::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    auto* clone = createModifiedPolyline();
    if (clone != nullptr) {
        clone->setLayer(m_polyline->getLayer());
        clone->setPen(m_polyline->getPen(false));
        ctx.dontSetActiveLayerAndPen();
        ctx.replace(m_polyline, clone);
        m_polyline  = clone;
        return true;
    }
    commandMessage(tr("Invalid arc point to create arc, select another one"));
    return false;
}

void LC_ActionPolylineChangeSegmentType::doTriggerCompletion([[maybe_unused]]bool success) {
    if (true) {
        m_polylineSegment = nullptr;
        setStatus(SetSegment);
    }
}

void LC_ActionPolylineChangeSegmentType::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetEntity: {
            const auto entity = catchAndDescribe(e, RS2::EntityPolyline);
            if (entity != nullptr){
                highlightHover(entity);
            }
            break;
        }
        case SetSegment:{
            const auto entity = catchAndDescribe(e, RS2::ResolveAllButTextImage);
            bool segmentFound = false;
            if (isAtomic(entity)){
                if (m_polyline == entity->getParent()){
                    const int rtti = entity->rtti();
                    switch (rtti){
                        case RS2::EntityArc:{
                            segmentFound = true;
                            highlightHover(entity);
                            if (m_showRefEntitiesOnPreview) {
                                previewLine(entity->getStartpoint(), entity->getEndpoint());
                            }
                            break;
                        }
                        case RS2::EntityLine:{
                            segmentFound = true;
                            highlightHover(entity);
                            const RS_Vector midPoint = (entity->getStartpoint() + entity->getEndpoint()) * 0.5;
                            previewRefSelectablePoint(midPoint);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            if (!segmentFound){
                highlightHover(m_polyline);
            }
            break;
        }
        case SetArcPoint:{
            RS_ArcData arcData;
            const bool success = LC_CreationArc::createFrom3P(m_polylineSegment->getStartpoint(), mouse,m_polylineSegment->getEndpoint(),arcData);
            previewRefLine(m_polylineSegment->getStartpoint(), m_polylineSegment->getEndpoint());
            if (success){
                previewRefSelectablePoint(mouse);
                previewRefPoint(arcData.center);
                previewRefPoint(m_polylineSegment->getStartpoint());
                previewRefPoint(m_polylineSegment->getEndpoint());
                previewArc(arcData);
            }
            break;
        }
        default:
            break;
    }
}

RS_Polyline* LC_ActionPolylineChangeSegmentType::createModifiedPolyline() const {
    auto* result = new RS_Polyline(m_document);

    for(RS_Entity* entity: lc::LC_ContainerTraverser{*m_polyline, RS2::ResolveAll}.entities()) {
        if (m_polylineSegment == entity){
            const int status = getStatus();
            switch (status){
                case SetSegment: { // arc to line
                    result->addVertex(entity->getStartpoint());
                    break;
                }
                case SetArcPoint: { // line to arc
                    RS_ArcData arcData;
                    const bool success = LC_CreationArc::createFrom3P(m_polylineSegment->getStartpoint(), m_arcPoint,m_polylineSegment->getEndpoint(), arcData);
                    if (success){
                        const double bulge = arcData.getBulge();
                        result->addVertex(entity->getStartpoint(), bulge);
                    }
                    else{ // can't create arc
                        delete result;
                        return nullptr;
                    }
                    break;
                }
                default:
                    // fixme - polyline - ELLIPTIC SEGMENT!
                    break;
            }
        }
        else{
            if (entity->rtti() == RS2::EntityArc){
                const auto arc = static_cast<RS_Arc *>(entity);
                result->addVertex(entity->getStartpoint(), arc->getBulge());
            }
            else {
                result->addVertex(entity->getStartpoint());
            }
        }
    }

    result->addVertex(m_polyline->getEndpoint());
    result->setClosed(m_polyline->isClosed());
    return result;
}

void LC_ActionPolylineChangeSegmentType::setPolylineToModify(RS_Entity* entity) {
    if (isPolyline(entity)){
        m_polyline = static_cast<RS_Polyline *>(entity);
        setStatus(SetSegment);
    }
}

void LC_ActionPolylineChangeSegmentType::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status){
        case SetEntity: {
            const auto entity = catchEntityByEvent(e, RS2::EntityPolyline);
            setPolylineToModify(entity);
            break;
        }
        case SetSegment:{
            const auto entity = catchEntityByEvent(e, RS2::ResolveAllButTextImage);
            if (isAtomic(entity)){
                if (m_polyline == entity->getParent()){
                    const int rtti = entity->rtti();
                    switch (rtti){
                        case RS2::EntityArc:{
                            m_polylineSegment = entity;
                            trigger();
                            break;
                        }
                        case RS2::EntityLine:{
                            m_polylineSegment = entity;
                            setStatus(SetArcPoint);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            break;
        }
        case SetArcPoint:{
            m_arcPoint = e->snapPoint;
            trigger();
            break;
        }
        default:{
            updatePrompt();
            break;
        }
    }
}

void LC_ActionPolylineChangeSegmentType::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    if (status == SetArcPoint){
        m_arcPoint = pos;
        trigger();
    }
}

void LC_ActionPolylineChangeSegmentType::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

RS2::CursorType LC_ActionPolylineChangeSegmentType::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionPolylineChangeSegmentType::updateActionPrompt() {
    switch (getStatus()){
        case SetEntity: {
            updatePromptTRCancel(tr("Select polyline to modify"));
            break;
        }
        case SetSegment:{
            updatePromptTRCancel(tr("Select segment of polyline to modify"));
            break;
        }
        case SetArcPoint:{
            updatePromptTRCancel(tr("Specify position on arc to create"));
            break;
        }
        default:{
            updatePrompt();
            break;
        }
    }
}
