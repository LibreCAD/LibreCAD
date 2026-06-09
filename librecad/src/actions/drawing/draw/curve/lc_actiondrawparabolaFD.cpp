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
#include "lc_actiondrawparabolaFD.h"

#include "lc_parabola.h"
#include "rs_constructionline.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_preview.h"

struct LC_ActionDrawParabolaFD::ActionData {
    RS_Vector focus;
    RS_Vector startPoint;
    RS_Vector endPoint;
    RS_Vector projection;
    RS_Vector axis;
    RS_Vector vertex;
    RS_Line* directrix = nullptr;
    LC_ParabolaData data;
    double h = 0.;
    bool valid = false;
    double getX(const RS_Vector& point) const {
        auto vp = point - vertex;
        return vp.rotate(M_PI/2 - axis.angle()).x;
    }

    std::pair<RS_Vector, RS_Vector> fromX(const double x) const {
        return {RS_Vector{x, x*x/(4.*h)}.rotate(axis.angle() - M_PI/2) + vertex,
                    RS_Vector{2.*h, x}.rotate(axis.angle() - M_PI/2)};
    }
    bool setEnd(const RS_Vector& point) {
        endPoint = point;
        const double x0 = getX(startPoint);
        const auto& [p0, t0] = fromX(x0);
        const double x1 = getX(point);
        const auto& [p1, t1] = fromX(x1);
        data = LC_ParabolaData::FromEndPointsTangents({p0, p1}, {t0, t1});
        return data.m_valid;
    }

    bool setStart(const RS_Vector& point) {
        startPoint = point;
        const double x = getX(startPoint);
        if (std::abs(x) > RS_TOLERANCE) {
            const auto& [p0, t0] = fromX(x);
            const auto& [p1, t1] = fromX(-x);
            data = LC_ParabolaData::FromEndPointsTangents({p0, p1}, {t0, t1});
        }
        return true;
    }

    bool setDirectrix(RS_Line& line) {
        double dist=RS_MAXDOUBLE;
        projection = line.getNearestPointOnEntity(focus, false, &dist);
        vertex = (focus + projection) * 0.5; // // fixme - sand -  hm.. Why 0.5 is used there?  it's a middle between line and focus for sure, yet what for?
        axis = focus - vertex;
        h = axis.magnitude();
        valid = h > RS_TOLERANCE;
        if (valid) {
            directrix = &line;
        }
        return valid;
    }

    bool setDirectrix(const RS_Vector& end) {
        projection = end;
//        vertex = (focus + projection) * 0.5;
        vertex = end;
        axis = focus - vertex;
        h = axis.magnitude();
        valid = h > RS_TOLERANCE;
        return valid;
    }
};

/**
 * Constructor.
 *
 */
LC_ActionDrawParabolaFD::LC_ActionDrawParabolaFD(LC_ActionContext *actionContext)
    :LC_SingleEntityCreationAction("ActionDrawParabolaFocusDiretrix", actionContext,RS2::ActionDrawParabolaFocusDiretrix)
    , m_actionData(std::make_unique<ActionData>()){
}

LC_ActionDrawParabolaFD::~LC_ActionDrawParabolaFD() = default;

void LC_ActionDrawParabolaFD::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (m_actionData->directrix != nullptr) {
        switch(status) {
            case SetFocus:
            case SetDirectrix: {
                m_actionData->directrix = nullptr;
                break;
            }
            default:
                break;
        }
    }
}

RS_Entity* LC_ActionDrawParabolaFD::doTriggerCreateEntity() {
  if(m_actionData->data.isValid()){
        auto* en = new LC_Parabola{m_document, m_actionData->data};
        return en;
    }
    return nullptr;
}

void LC_ActionDrawParabolaFD::doTriggerCompletion([[maybe_unused]]bool success) {
    init(SetFocus);
}

bool LC_ActionDrawParabolaFD::isInVisualSnapStatus(int status) {
    return (status == SetStartPoint) || (status == SetEndPoint) || (status == SetFocus) || (status == SetDirectrix);
}

void LC_ActionDrawParabolaFD::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetFocus:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetDirectrix: {
            mouse = e->graphPoint; // tmp - which snap is better there?
            RS_Entity* entity = catchEntityByEvent(e, RS2::EntityLine);

            if (entity != nullptr) {
                highlightHover(entity);
            }
            else{
                mouse = getSnapAngleAwarePoint(e, m_actionData->focus, mouse, true);
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(m_actionData->focus, mouse);
                }
            }
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->focus);
            }
            break;
        }
        case SetStartPoint: {
            m_actionData->setStart(mouse);
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->focus);
                previewRefPoint(m_actionData->vertex);
                previewRefLine(m_actionData->focus, m_actionData->vertex);

                RS_Vector rotatedAxis = m_actionData->axis;
                rotatedAxis = rotatedAxis.rotate(M_PI/2);
                auto tmpLine = RS_ConstructionLine(nullptr, RS_ConstructionLineData(m_actionData->vertex, m_actionData->vertex  + rotatedAxis));
                RS_Vector projection = tmpLine.getNearestPointOnEntity(mouse, false);

                previewRefPoint(projection);
                previewRefLine(m_actionData->vertex, projection);
            }
            LC_Parabola* parabola = preparePreview();
            if (m_showRefEntitiesOnPreview) {
                if (parabola != nullptr) {
                    RS_Vector startPoint = parabola->getStartpoint();
                    previewRefSelectablePoint(startPoint);
                }
            }
            break;
        }
        case SetEndPoint: {
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->focus);
                previewRefPoint(m_actionData->vertex);
                previewRefLine(m_actionData->focus, m_actionData->vertex);

                RS_Vector rotatedAxis = m_actionData->axis;
                rotatedAxis = rotatedAxis.rotate(M_PI/2);
                auto tmpLine = RS_ConstructionLine(nullptr, RS_ConstructionLineData(m_actionData->vertex, m_actionData->vertex  + rotatedAxis));

                RS_Vector projectionStart = tmpLine.getNearestPointOnEntity(m_actionData->startPoint, false);
                RS_Vector projectionEnd = tmpLine.getNearestPointOnEntity(mouse, false);
                previewRefPoint(projectionStart);
                previewRefSelectablePoint(projectionEnd);
                previewRefLine(projectionStart, projectionEnd);
            }
            m_actionData->setEnd(mouse);
            LC_Parabola *parabola = preparePreview();
            if (m_showRefEntitiesOnPreview) {
                if (parabola != nullptr) {
                    RS_Vector startPoint = parabola->getStartpoint();
                    RS_Vector endPoint = parabola->getEndpoint();
                    previewRefSelectablePoint(endPoint);
                    previewRefPoint(startPoint);
                }
            }
            break;
        }
        default:
            break;
    }
    if (m_actionData->directrix != nullptr){
        highlightSelected(m_actionData->directrix);
    }
}

LC_Parabola* LC_ActionDrawParabolaFD::preparePreview() const {
  if (m_actionData->data.isValid()) {
        auto* pl = new LC_Parabola{m_preview.get(), m_actionData->data};
        previewEntity(pl);
        return pl;
    }
    return nullptr;
}

void LC_ActionDrawParabolaFD::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status){
        case SetDirectrix:{
            RS_Entity* entity = catchEntityByEvent(e, RS2::EntityLine);
            if (entity != nullptr) {
                if (m_actionData->setDirectrix(*static_cast<RS_Line *>(entity))) {
                    setStatus(status+1);
                }
            }
            else{
                RS_Vector mouse = e->graphPoint;  // tmp - should we use free or normal snap there?
                mouse = getSnapAngleAwarePoint(e, m_actionData->focus, mouse);
                if (m_actionData->setDirectrix(mouse)) {
                    setStatus(status+1);
                }
            }
            break;
        }
        default:{
            fireCoordinateEventForSnap(e);
            break;
        }
    }
}

void LC_ActionDrawParabolaFD::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawParabolaFD::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    moveRelativeZero(coord);
    switch (status) {
        case SetFocus:
            m_actionData->focus = coord;
            setStatus(getStatus()+1);
            break;
        case SetStartPoint:
            m_actionData->setStart(coord);
            setStatus(status+1);
            break;
        case SetEndPoint:
            if(m_actionData->setEnd(coord)) {
                trigger();
            }
            break;
        default:
            break;
    }
}

//fixme, support command line

QStringList LC_ActionDrawParabolaFD::getAvailableCommands() {
    return {};
}

void LC_ActionDrawParabolaFD::updateActionPrompt() {
    switch (getStatus()) {
        case SetFocus:
            updatePromptTRCancel(tr("Specify the focus of parabola"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetDirectrix:
            updatePromptTRBack(tr("Select line that is parallel to directrix of parabola or set vertex point"));
            break;

        case SetStartPoint:
            updatePromptTRBack(tr("Specify the start point on parabola"));
            break;

        case SetEndPoint:
            updatePromptTRBack(tr("Specify the end point on parabola"));
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionDrawParabolaFD::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
