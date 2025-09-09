/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_actionsingleentityselectbase.h"

#include "lc_actioncontext.h"
#include "lc_dimordinate.h"
#include "rs_document.h"

LC_ActionSingleEntitySelectBase::LC_ActionSingleEntitySelectBase(const char* name, LC_ActionContext* actionContext,
                                                                 RS2::ActionType actionType) :
    RS_PreviewActionInterface(name, actionContext, actionType)
    , m_entity{nullptr} {
}

LC_ActionSingleEntitySelectBase::~LC_ActionSingleEntitySelectBase() = default;

void LC_ActionSingleEntitySelectBase::updateMouseButtonHints() {
     updateMouseWidgetTRCancel(doGetMouseButtonHint());
}


void LC_ActionSingleEntitySelectBase::doInitialInit() {
    auto contextEntity = m_actionContext->getContextMenuActionContextEntity();
    if (contextEntity == nullptr) { // proceed selection if we have no context menu entity
        std::vector<RS_Entity*> selectedEntities;
        m_document->collectSelected(selectedEntities, false, {RS2::EntityDimOrdinate});
        if (selectedEntities.size() == 1) {
            RS_Entity* selectedEntity = selectedEntities[0];
            if (doCheckMaySelectEntity(selectedEntity)) {
                m_entity = selectedEntity;
                trigger();
            }
        }
    }
}

void LC_ActionSingleEntitySelectBase::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (doCheckMaySelectEntity(contextEntity)) {
        m_entity = contextEntity;
        trigger();
    }
}


void LC_ActionSingleEntitySelectBase::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent* event) {
    RS_Entity* e = catchEntity(event->snapPoint, RS2::ResolveNone);
    if (e != nullptr) {
        if (doCheckMaySelectEntity(e)) {
            highlightHover(e);
        }
    }
}

void LC_ActionSingleEntitySelectBase::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent* event) {
    RS_Entity* e = catchEntity(event->snapPoint, RS2::ResolveNone);
    if (e != nullptr) {
        if (e->rtti() == RS2::EntityDimOrdinate) {
            m_entity = dynamic_cast<LC_DimOrdinate*>(e);
            if (m_entity != nullptr) {
                trigger();
            }
        }
    }
}

void LC_ActionSingleEntitySelectBase::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent* e) {
    initPrevious(status);
}
