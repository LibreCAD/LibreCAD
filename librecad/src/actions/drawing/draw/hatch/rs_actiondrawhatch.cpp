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
#include <iostream>

#include <QMouseEvent>

#include "rs_actiondrawhatch.h"
#include "rs_dialogfactory.h"
#include "rs_eventhandler.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_hatch.h"
#include "rs_debug.h"

namespace {
bool hatchAble(RS_Entity* entity) {
    if (entity == nullptr)
        return false;
    switch (entity->rtti()) {
    case RS2::EntityHatch:
    case RS2::EntityPoint:
    case RS2::EntityImage:
    case RS2::EntityMText:
    case RS2::EntityText:
        return false;
    default:
        break;
    }
    if (RS_Information::isDimension(entity->rtti()))
        return false;

    return  entity->getLength() > RS_TOLERANCE;
}
}


// fixme - review hatching and check the possibility to add preview mode!!

RS_ActionDrawHatch::RS_ActionDrawHatch(RS_EntityContainer& container, RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Hatch", container, graphicView)
    , data{std::make_unique<RS_HatchData>()}
{
    actionType = RS2::ActionDrawHatch;
}

RS_ActionDrawHatch::~RS_ActionDrawHatch() = default;

void RS_ActionDrawHatch::setShowArea(bool s){
    m_bShowArea=s;
}

void RS_ActionDrawHatch::init(int status){
    RS_PreviewActionInterface::init(status);

    RS_Hatch tmp(container, *data);
    tmp.setLayerToActive();
    tmp.setPenToActive();
    if (RS_DIALOGFACTORY->requestHatchDialog(&tmp)) {
        *data = tmp.getData();
        trigger();
        graphicView->redraw(RS2::RedrawDrawing);
    }
    finish(false);
}

void RS_ActionDrawHatch::trigger() {

    RS_DEBUG->print("RS_ActionDrawHatch::trigger()");

    // deselect unhatchable entities:
    for(auto e: *container) { // fixme - sand -  iteration over all entities in container
        if (e->isSelected() && !hatchAble(e))
            e->setSelected(false);
    }

    for (auto e=container->firstEntity(RS2::ResolveAll); e != nullptr;
         e=container->nextEntity(RS2::ResolveAll)) {
        if (e->isSelected() && !hatchAble(e))
            e->setSelected(false);
    }

    // look for selected contours:
    bool haveContour = false;
    for (auto e=container->firstEntity(RS2::ResolveAll); e != nullptr;
         e=container->nextEntity(RS2::ResolveAll)) {
        if (e->isSelected()) {
            haveContour = true;
        }
    }

    if (!haveContour){
        LC_ERR << "RS_ActionDrawHatch:: "<<__func__<<"(): line "<<__LINE__<<", no contour selected\n";
        return;
    }

    std::unique_ptr<RS_Hatch> hatch = std::make_unique<RS_Hatch>(container, *data);
    hatch->setLayerToActive();
    hatch->setPenToActive();
    auto *loop = new RS_EntityContainer(hatch.get());
    loop->setPen(RS_Pen(RS2::FlagInvalid));

    // add selected contour:
    for (auto e=container->firstEntity(RS2::ResolveAll); e;
         e=container->nextEntity(RS2::ResolveAll)) {

        if (e->isSelected()){
            e->setSelected(false);
            // entity is part of a complex entity (spline, polyline, ..):
            if (e->getParent() &&
                    // RVT - Don't de-delect the parent EntityPolyline, this is messing up the getFirst and getNext iterators
                    //			    (e->getParent()->rtti()==RS2::EntitySpline ||
                    //				 e->getParent()->rtti()==RS2::EntityPolyline)) {
                    (e->getParent()->rtti()==RS2::EntitySpline)) {
                e->getParent()->setSelected(false);
            }
            RS_Entity *cp = e->clone();
            cp->setPen(RS_Pen(RS2::FlagInvalid));
            cp->reparent(loop);
            loop->addEntity(cp);
        }
    }

    hatch->addEntity(loop);
    if (hatch->validate()){
        container->addEntity(hatch.get());

        addToDocumentUndoable(hatch.get());

        hatch->update();

        graphicView->redraw(RS2::RedrawDrawing);

        bool printArea = true;
        switch( hatch->getUpdateError()) {
        case RS_Hatch::HATCH_OK :
                commandMessage(tr("Hatch created successfully."));
                break;
            case RS_Hatch::HATCH_INVALID_CONTOUR :
                commandMessage(tr("Hatch Error: Invalid contour found!"));
                printArea = false;
                break;
            case RS_Hatch::HATCH_PATTERN_NOT_FOUND :
                commandMessage(tr("Hatch Error: Pattern not found!"));
                break;
            case RS_Hatch::HATCH_TOO_SMALL :
                commandMessage(tr("Hatch Error: Contour or pattern too small!"));
                break;
            case RS_Hatch::HATCH_AREA_TOO_BIG :
                commandMessage(tr("Hatch Error: Contour too big!"));
                break;
            default :
                commandMessage(tr("Hatch Error: Undefined Error!"));
                printArea = false;
                break;
        }
        if (m_bShowArea && printArea){
            commandMessage(tr("Total hatch area = %1").
                arg(hatch->getTotalArea(), 12, 'g', 10));
        }

        hatch.release();

    } else {
        hatch.reset();
        commandMessage(tr("Invalid hatch area. Please check that the entities chosen form one or more closed contours."));
	}
    //}
}

void RS_ActionDrawHatch::mouseMoveEvent(QMouseEvent*) {
    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent begin");

    /*if (getStatus()==SetPos) {
        RS_Vector mouse = snapPoint(e);
        pos = mouse;


        deletePreview();
        if (hatch && !hatch->isVisible()) {
            hatch->setVisible(true);
        }
        offset = RS_Vector(graphicView->toGuiDX(pos.x),
                           -graphicView->toGuiDY(pos.y));
        drawPreview();
}*/

    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent end");
}

void RS_ActionDrawHatch::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    snapPoint(e);
}

void RS_ActionDrawHatch::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    //deletePreview();
    initPrevious(status);
}


void RS_ActionDrawHatch::updateMouseButtonHints() {
	 updateMouseWidget();
}
RS2::CursorType RS_ActionDrawHatch::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
