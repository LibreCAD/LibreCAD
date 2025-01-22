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

#include "lc_actionpolylinechangesegmenttype.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_graphicview.h"

LC_ActionPolylineChangeSegmentType::LC_ActionPolylineChangeSegmentType(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("PolylineChangeSegment",container,graphicView) {
    actionType = RS2::ActionPolylineChangeSegmentType;
}

LC_ActionPolylineChangeSegmentType::~LC_ActionPolylineChangeSegmentType() {

}

void LC_ActionPolylineChangeSegmentType::doTrigger() {
    // todo - move to RS_Modification?
    auto* createdPolyline =  createModifiedPolyline();
    if (createdPolyline != nullptr) {
        createdPolyline->setLayer(polyline->getLayer());
        createdPolyline->setPen(polyline->getPen(false));

        container->addEntity(createdPolyline);

        undoCycleReplace(polyline, createdPolyline);
        polyline = createdPolyline;
        polylineSegment = nullptr;
        setStatus(SetSegment);
    }
    else {
        commandMessage(tr("Invalid arc point to create arc, select another one"));
    }
}

void LC_ActionPolylineChangeSegmentType::mouseMoveEvent(QMouseEvent *e) {
    deletePreview();
    deleteHighlights();

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status){
        case SetEntity: {
            auto entity = catchEntityOnPreview(e, RS2::EntityPolyline);
            if (entity != nullptr){
                highlightHover(entity);
            }
            break;
        }
        case SetSegment:{
            auto entity = catchEntityOnPreview(e, RS2::ResolveAllButTextImage);
            bool segmentFound = false;
            if (entity != nullptr && entity->isAtomic()){
                if (polyline == entity->getParent()){
                    int rtti = entity->rtti();
                    switch (rtti){
                        case RS2::EntityArc:{
                            segmentFound = true;
                            highlightHover(entity);
                            if (showRefEntitiesOnPreview) {
                                previewLine(entity->getStartpoint(), entity->getEndpoint());
                            }
                            break;
                        }
                        case RS2::EntityLine:{
                            segmentFound = true;
                            highlightHover(entity);
                            RS_Vector midPoint = (entity->getStartpoint() + entity->getEndpoint()) * 0.5;
                            previewRefSelectablePoint(midPoint);
                            break;
                        }
                    }
                }
            }
            if (!segmentFound){
                highlightHover(polyline);
            }
            break;
        }
        case SetArcPoint:{
            auto arc = RS_Arc(nullptr, RS_ArcData());
            bool suc = arc.createFrom3P(polylineSegment->getStartpoint(), mouse,polylineSegment->getEndpoint());
            previewRefLine(polylineSegment->getStartpoint(), polylineSegment->getEndpoint());
            if (suc){
                previewRefSelectablePoint(mouse);
                previewRefPoint(arc.getCenter());
                previewRefPoint(polylineSegment->getStartpoint());
                previewRefPoint(polylineSegment->getEndpoint());
                previewArc(arc.getData());
            }
            break;
        }
        default:
            break;
    }

    drawPreview();
    drawHighlights();
}


RS_Polyline* LC_ActionPolylineChangeSegmentType::createModifiedPolyline() {
    auto* result = new RS_Polyline(container);

    for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity; entity = polyline->nextEntity(RS2::ResolveAll)) {
        if (polylineSegment == entity){
            int status = getStatus();
            switch (status){
                case SetSegment: { // arc to line
                    result->addVertex(entity->getStartpoint());
                    break;
                }
                case SetArcPoint: { // line to arc
                    auto arc = RS_Arc(nullptr, RS_ArcData());
                    bool suc = arc.createFrom3P(polylineSegment->getStartpoint(), arcPoint,polylineSegment->getEndpoint());
                    if (suc){
                        double bulge = arc.getBulge();
                        result->addVertex(entity->getStartpoint(), bulge);
                    }
                    else{ // can't create arc
                        delete result;
                        return nullptr;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else{
            if (entity->rtti() == RS2::EntityArc){
                auto arc = static_cast<RS_Arc *>(entity);
                result->addVertex(entity->getStartpoint(), arc->getBulge());
            }
            else {
                result->addVertex(entity->getStartpoint());
            }
        }
    }

    result->addVertex(polyline->getEndpoint());

    result->setClosed(polyline->isClosed());
    return result;
}

void LC_ActionPolylineChangeSegmentType::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetEntity: {
            auto entity = catchEntity(e, RS2::EntityPolyline);
            if (entity != nullptr){
                polyline = static_cast<RS_Polyline *>(entity);
                setStatus(SetSegment);
            }
            break;
        }
        case SetSegment:{
            auto entity = catchEntity(e, RS2::ResolveAllButTextImage);
            if (entity != nullptr && entity->isAtomic()){
                if (polyline == entity->getParent()){
                    int rtti = entity->rtti();
                    switch (rtti){
                        case RS2::EntityArc:{
                            polylineSegment = entity;
                            trigger();
                            break;
                        }
                        case RS2::EntityLine:{
                            polylineSegment = entity;
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
            arcPoint = snapPoint(e);
            trigger();
            break;
        }
        default:{
            updateMouseWidget();
        }
    }
}

void LC_ActionPolylineChangeSegmentType::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    if (status == SetArcPoint){
        arcPoint = pos;
        trigger();
    }
}


void LC_ActionPolylineChangeSegmentType::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    initPrevious(status);
}

RS2::CursorType LC_ActionPolylineChangeSegmentType::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}


void LC_ActionPolylineChangeSegmentType::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity: {
            updateMouseWidgetTRCancel(tr("Select polyline to modify"));
            break;
        }
        case SetSegment:{
            updateMouseWidgetTRCancel(tr("Select segment of polyline to modify"));
            break;
        }
        case SetArcPoint:{
            updateMouseWidgetTRCancel(tr("Specify position on arc to create"));
            break;
        }
        default:{
            updateMouseWidget();
        }
    }
}
