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

#include "lc_action_polyline_arcs_to_lines.h"

#include "lc_actioncontext.h"
#include "lc_containertraverser.h"
#include "rs_document.h"
#include "rs_pen.h"
#include "rs_polyline.h"

LC_ActionPolylineArcsToLines::LC_ActionPolylineArcsToLines(LC_ActionContext *actionContext)
    : LC_UndoableDocumentModificationAction("ActionPolylineArcsToLines", actionContext, RS2::ActionPolylineArcsToLines),
      m_polyline{nullptr} {
}

LC_ActionPolylineArcsToLines::~LC_ActionPolylineArcsToLines() = default;

void LC_ActionPolylineArcsToLines::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

void LC_ActionPolylineArcsToLines::init(const int status) {
    m_polyline = nullptr;
    RS_PreviewActionInterface::init(status);
}

bool LC_ActionPolylineArcsToLines::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (hasArcsSegments(m_polyline)) {
        auto* createdPolyline =  createPolyline(m_polyline);
        createdPolyline->setLayer(m_polyline->getLayer());
        createdPolyline->setPen(m_polyline->getPen(false));
        ctx.replace(m_polyline, createdPolyline);
        return true;
    }
    return false;
}

void LC_ActionPolylineArcsToLines::doTriggerCompletion([[maybe_unused]]bool success) {
    m_polyline = nullptr;
}

void LC_ActionPolylineArcsToLines::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    const auto entity = catchAndDescribe(e, RS2::EntityPolyline);
    if (entity != nullptr){
        const auto* selectedPolyline = dynamic_cast<RS_Polyline*>(entity);
        if (hasArcsSegments(selectedPolyline)) {
            const RS_Polyline *previewPolyline = createPolyline(selectedPolyline);
            previewEntity(previewPolyline);
            highlightHover(entity);
        }
    }
}

void LC_ActionPolylineArcsToLines::setPolylineToModify(RS_Entity* entity) {
    if (isPolyline(entity)) {
        m_polyline = dynamic_cast<RS_Polyline *>(entity);
        trigger();
    }
}

void LC_ActionPolylineArcsToLines::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    const auto entity = catchEntityByEvent(e, RS2::EntityPolyline);
    if (entity != nullptr){
        setPolylineToModify(entity);
    }
}

void LC_ActionPolylineArcsToLines::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    init(-1);
}

RS_Polyline *LC_ActionPolylineArcsToLines::createPolyline(const RS_Polyline *original) const {
    auto* clone = new RS_Polyline(m_document);

    clone->addVertex(original->getStartpoint());
    for(const RS_Entity* entity: lc::LC_ContainerTraverser{*original, RS2::ResolveAll}.entities()) {
        clone->addVertex(entity->getEndpoint());
    }

    clone->setClosed(original->isClosed());
    return clone;
}

bool LC_ActionPolylineArcsToLines::hasArcsSegments(const RS_Polyline *p) {
    lc::LC_ContainerTraverser traverser{*p, RS2::ResolveAll};
    for(const RS_Entity* entity = traverser.first(); entity != nullptr; entity = traverser.next()) {
        const RS2::EntityType rtti = entity->rtti();
        if (rtti == RS2::EntityArc){
            return true;
        }
    }
    return false;
}

void LC_ActionPolylineArcsToLines::updateActionPrompt() {
    updatePromptTRCancel(tr("Specify polyline with arc segments"));
}

RS2::CursorType LC_ActionPolylineArcsToLines::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
