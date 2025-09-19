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

#include "lc_actiondimordinate.h"

#include "lc_actioncontext.h"
#include "lc_dimordinate.h"
#include "lc_graphicviewport.h"
#include "lc_ucs.h"
#include "rs_document.h"
#include "rs_polyline.h"
#include "rs_preview.h"

class RS_Polyline;

struct LC_ActionDimOrdinate::ActionData {
    ActionData() = default;
    ~ActionData() = default;
    RS_Vector m_wcsFeaturePoint;
    RS_Vector m_leaderEndPoint;
    bool directionX = false;
    bool ctrlPressed = false;
};

LC_ActionDimOrdinate::LC_ActionDimOrdinate(LC_ActionContext* context)
    : RS_ActionDimension("DimOrdinate", context,  RS2::EntityDimRadial, RS2::ActionDimOrdinate)
    ,m_actionData{std::make_unique<ActionData>()}{
}

LC_ActionDimOrdinate::~LC_ActionDimOrdinate()  = default;

void LC_ActionDimOrdinate::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    RS_Vector pos;
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        auto startPoint = entity->getStartpoint();
        auto endPoint = entity->getEndpoint();
        double distToStart = startPoint.distanceTo(clickPos);
        double distToEnd = endPoint.distanceTo(clickPos);
        if (distToEnd < distToStart) {
            pos = endPoint;
        }
        else {
            pos = startPoint;
        }
    }
    else if (isArc(entity) || isCircle(entity) || isEllipse(entity)) {
        pos = entity->getCenter();
    }
    else {
        return;
    }

    m_actionData->m_wcsFeaturePoint = pos;
    moveRelativeZero(pos);
    setStatus(SetLeaderEnd);
}

void LC_ActionDimOrdinate::doTrigger() {
    LC_DimOrdinate* dim  = createDim(m_actionData->m_leaderEndPoint, m_actionData->ctrlPressed, m_document);

    setPenAndLayerToActive(dim);
    undoCycleAdd(dim);

    m_actionData->ctrlPressed = false;
    m_dimensionData->text = "";
    setStatus(SetFeaturePoint);
}

 LC_DimOrdinate*  LC_ActionDimOrdinate::createDim(RS_Vector leaderEndPoint, bool alternateOrdinate, RS_EntityContainer* container) {
    double horAxisDirection = 0;
    RS_Vector originPoint;
    m_viewport->fillCurrentUCSInfo(originPoint, horAxisDirection);

    m_dimensionData->definitionPoint = originPoint;
    m_dimensionData->horizontalAxisDirection = horAxisDirection;
    RS_Vector ucsFeaturePoint = toUCS(m_actionData->m_wcsFeaturePoint);
    RS_Vector ucsLeaderPoint = toUCS(leaderEndPoint);

    RS_Vector deltaInUCS = ucsLeaderPoint-ucsFeaturePoint;

    bool ordinateForX = std::abs(deltaInUCS.x) < std::abs(deltaInUCS.y);

    if (alternateOrdinate) {
        ordinateForX = !ordinateForX;
    }

    LC_DimOrdinateData dimOrdinateData (m_actionData->m_wcsFeaturePoint,leaderEndPoint, ordinateForX);
    auto* dimOrdinate = new LC_DimOrdinate(container,*m_dimensionData, dimOrdinateData);
    dimOrdinate->update();
    return dimOrdinate;
}

void LC_ActionDimOrdinate::onMouseMoveEvent(int status, LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case (SetFeaturePoint): {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case (SetLeaderEnd): {
            mouse = getSnapAngleAwarePoint(e, m_actionData->m_wcsFeaturePoint, mouse, true);

            RS_Vector originPoint;
            LC_DimOrdinate* dimOrdinate = createDim(mouse, e->isControl, m_preview.get());

            previewEntityToCreate(dimOrdinate);

            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
                previewRefPoint(m_actionData->m_wcsFeaturePoint);
                previewRefPoint(originPoint);
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDimOrdinate::doProcessCommand(int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetText: {
            setText(command);
            updateOptions();
            enableCoordinateInput();
            setStatus(m_lastStatus);
            accept = true;
            break;
        }
        default:
            m_lastStatus = (State) getStatus();
            deletePreview();
            if (checkCommand("text", command)) {
                disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            }
            break;
    }
    return accept;
}

void LC_ActionDimOrdinate::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetFeaturePoint: {
            m_actionData->m_wcsFeaturePoint = pos;
            moveRelativeZero(pos);
            setStatus(SetLeaderEnd);
            break;
        }
        case SetLeaderEnd: {
            m_actionData->m_leaderEndPoint = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDimOrdinate::onMouseLeftButtonRelease(int status, LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetFeaturePoint){
        // less restrictive snap
        snap = getRelZeroAwarePoint(e, snap);
    }
    else if (status == SetLeaderEnd) {
        m_actionData->ctrlPressed = e->isControl;
        snap = getSnapAngleAwarePoint(e, m_actionData->m_wcsFeaturePoint, snap);
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDimOrdinate::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent* e) {
    initPrevious(status);
}

QStringList LC_ActionDimOrdinate::doGetAvailableCommands([[maybe_unused]]int status) {
    return {command("text")};
}

void LC_ActionDimOrdinate::updateMouseButtonHints() {
    int status = getStatus();
    switch (status) {
        case SetFeaturePoint: {
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetLeaderEnd: {
            updateMouseWidgetTRCancel(tr("Specify leader end point"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Change Ordinate")));
            break;
        }
        default:
            break;
    }
}
