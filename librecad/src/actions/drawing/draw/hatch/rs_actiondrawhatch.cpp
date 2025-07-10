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

#include "lc_containertraverser.h"

#include "rs_actiondrawhatch.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_pen.h"

namespace {
    bool hatchAble(RS_Entity *entity) {
        if (entity == nullptr) {
            return false;
        }
        switch (entity->rtti()) {
            case RS2::EntityHatch:
            case RS2::EntityPoint:
            case RS2::EntityImage:
            case RS2::EntityMText:
            case RS2::EntityText:
                return false;
            default: {
                if (RS_Information::isDimension(entity->rtti())) {
                    return false;
                }
                return entity->getLength() > RS_TOLERANCE;
            }
        }
    }
}

// fixme - review hatching and check the possibility to add preview mode!!

RS_ActionDrawHatch::RS_ActionDrawHatch(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("Draw Hatch", actionContext, RS2::ActionDrawHatch)
    , m_hatchData{std::make_unique<RS_HatchData>()}{
}

RS_ActionDrawHatch::~RS_ActionDrawHatch() = default;

void RS_ActionDrawHatch::setShowArea(bool s){
    m_bShowArea=s;
}

void RS_ActionDrawHatch::init(int status){
    LC_ActionPreSelectionAwareBase::init(status);
}

bool RS_ActionDrawHatch::isAllowTriggerOnEmptySelection() {
    return false;
}

void RS_ActionDrawHatch::doTrigger([[maybe_unused]]bool keepSelected) {

    RS_DEBUG->print("RS_ActionDrawHatch::trigger()");

    RS_Hatch tmp(m_container, *m_hatchData);
    setPenAndLayerToActive(&tmp);

    if (RS_DIALOGFACTORY->requestHatchDialog(&tmp, m_viewport)) {
        *m_hatchData = tmp.getData();

        // fixme - sand - optimize that mess with cycles!!!
        // deselect unhatchable entities:
        // fixme - sand -  iteration over all entities in container
        for(auto e: *m_container) {
            if (e->isSelected() && !hatchAble(e))
                e->setSelected(false);
        }
        // fixme - sand -  iteration over all entities in container
        std::vector<RS_Entity*> entities = lc::LC_ContainerTraverser{*m_container, RS2::ResolveAll}.entities();
        for (RS_Entity* e: entities) {
            if (e->isSelected() && !hatchAble(e))
                e->setSelected(false);
        }

        // fixme - sand -  iteration over all entities in container
        // look for selected contours:
        bool haveContour = false;
        for (RS_Entity* e: entities) {
            if (e->isSelected()) {
                haveContour = true;
            }
        }

        if (!haveContour){
            LC_ERR << "RS_ActionDrawHatch:: "<<__func__<<"(): line "<<__LINE__<<", no contour selected\n";
            return;
        }

        std::unique_ptr<RS_Hatch> hatch = std::make_unique<RS_Hatch>(m_container, *m_hatchData);
        hatch->setLayerToActive();
        hatch->setPenToActive();
        auto *loop = new RS_EntityContainer(hatch.get());
        loop->setPen(RS_Pen(RS2::FlagInvalid));

        // add selected contour:
        entities = lc::LC_ContainerTraverser{*m_container, RS2::ResolveAll}.entities();
        for (RS_Entity* e: entities) {

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

            undoCycleAdd(hatch.get());

            hatch->update();

            redrawDrawing();

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
    }
    finish(false);
}



void RS_ActionDrawHatch::doSelectEntity(RS_Entity* entityToSelect, [[maybe_unused]] bool selectContour) const {
    // try to minimize selection clicks - and select contour based on selected entity. May be optional, but what for?
    RS_ActionSelectBase::doSelectEntity(entityToSelect, true);
}

bool RS_ActionDrawHatch::isEntityAllowedToSelect(RS_Entity *ent) const {
    return hatchAble(ent);
}

void RS_ActionDrawHatch::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to hatch (Enter to complete)"), MOD_CTRL(tr("Hatch immediately after selection")));
}

RS2::CursorType RS_ActionDrawHatch::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::SelectCursor;
}
