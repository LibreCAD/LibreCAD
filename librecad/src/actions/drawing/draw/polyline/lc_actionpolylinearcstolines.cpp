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

#include "lc_actionpolylinearcstolines.h"
#include "rs_polyline.h"
#include "rs_document.h"
#include "rs_graphicview.h"

LC_ActionPolylineArcsToLines::LC_ActionPolylineArcsToLines(RS_EntityContainer &container, RS_GraphicView &graphicView)
:RS_PreviewActionInterface("PolylineArcsToLines", container, graphicView) {
    actionType = RS2::ActionPolylineArcsToLines;
}

LC_ActionPolylineArcsToLines::~LC_ActionPolylineArcsToLines() {
}

void LC_ActionPolylineArcsToLines::doTrigger() {
    // todo - move to RS_Modification?
    auto* createdPolyline =  createPolyline(polyline);

    createdPolyline->setLayer(polyline->getLayer());
    createdPolyline->setPen(polyline->getPen(false));

    container->addEntity(createdPolyline);
    undoCycleReplace(polyline, createdPolyline);

    polyline = nullptr;
}

void LC_ActionPolylineArcsToLines::mouseMoveEvent(QMouseEvent *e) {
    deleteHighlights();
    deletePreview();
    snapPoint(e);
    auto entity = catchEntityOnPreview(e, RS2::EntityPolyline);
    if (entity != nullptr){
        auto* selectedPolyline = dynamic_cast<RS_Polyline*>(entity);
        if (hasArcsSegments(selectedPolyline)) {
            RS_Polyline *previewPolyline = createPolyline(selectedPolyline);
            previewEntity(previewPolyline);
            highlightHover(entity);
        }
    }
    drawHighlights();
    drawPreview();
}

void LC_ActionPolylineArcsToLines::onMouseLeftButtonRelease([[maybe_unused]] int status, QMouseEvent *e) {
    auto entity = catchEntity(e, RS2::EntityPolyline);
    if (entity != nullptr){
        polyline = dynamic_cast<RS_Polyline *>(entity);
        trigger();
    }
}

void LC_ActionPolylineArcsToLines::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status < 0){
       polyline = nullptr;
    }
}

void LC_ActionPolylineArcsToLines::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent *e) {
    init(-1);
}

RS_Polyline *LC_ActionPolylineArcsToLines::createPolyline(RS_Polyline *original) {
    auto* clone = new RS_Polyline(container);

    clone->addVertex(original->getStartpoint());

    for (RS_Entity *entity = original->firstEntity(RS2::ResolveAll); entity; entity = original->nextEntity(RS2::ResolveAll)) {
        clone->addVertex(entity->getEndpoint());
    }

    clone->setClosed(original->isClosed());
    return clone;
}

bool LC_ActionPolylineArcsToLines::hasArcsSegments(RS_Polyline *p) {
    for (RS_Entity *entity = p->firstEntity(RS2::ResolveAll); entity; entity = p->nextEntity(RS2::ResolveAll)) {
        int rtti = entity->rtti();
        if (rtti == RS2::EntityArc){
            return true;
        }
    }
    return false;
}

void LC_ActionPolylineArcsToLines::updateMouseButtonHints() {
    updateMouseWidgetTRCancel(tr("Specify polyline with arc segments"));
}

RS2::CursorType LC_ActionPolylineArcsToLines::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
