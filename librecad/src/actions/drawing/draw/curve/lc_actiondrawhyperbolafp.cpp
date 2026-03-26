/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 Dongxu Li (github.com/dxli)

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

#include <cmath>
#include <memory>

#include "lc_actiondrawhyperbolafp.h"
#include "lc_hyperbola.h"
#include "rs_preview.h"
#include "qg_graphicview.h"

LC_ActionDrawHyperbolaFP::LC_ActionDrawHyperbolaFP(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawHyperbolaFoci2Points", actionContext, RS2::ActionDrawHyperbolaFoci2Points) {
    LC_ActionDrawHyperbolaFP::reset();
}

void LC_ActionDrawHyperbolaFP::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

void LC_ActionDrawHyperbolaFP::reset() {
    focus1 = focus2 = startPoint = endPoint = RS_Vector(false);
    setStatus(SetFocus1);
}

bool LC_ActionDrawHyperbolaFP::isInVisualSnapStatus(int status) {
    return (status == SetFocus1) || (status == SetFocus2) || (status == SetEndPoint) || (status == SetStartPoint);
}

void LC_ActionDrawHyperbolaFP::preparePreview() const {
    if (!focus1.valid || !focus2.valid || !startPoint.valid) {
        return;
    }

    auto hyperbola = std::make_unique<LC_Hyperbola>(nullptr, LC_HyperbolaData{focus1, focus2, startPoint});
    if (!hyperbola->isValid()) {
        return;
    }

    const bool rev = (startPoint.distanceTo(focus1) - startPoint.distanceTo(focus2) < 0.0);

    const double phiStart = hyperbola->getParamFromPoint(startPoint, rev);
    if (std::isnan(phiStart)) {
        return;
    }

    double phi1 = phiStart;
    double phi2 = phiStart;

    if (getStatus() == SetStartPoint) {
        phi1 = -std::abs(phiStart);
        phi2 = std::abs(phiStart);
    }
    else if (getStatus() == SetEndPoint && endPoint.valid) {
        const double phiEnd = hyperbola->getParamFromPoint(endPoint, rev);
        if (!std::isnan(phiEnd)) {
            phi1 = std::min(phiStart, phiEnd);
            phi2 = std::max(phiStart, phiEnd);
        }
    }

    hyperbola->setAngle1(phi1);
    hyperbola->setAngle2(phi2);

    previewEntity(hyperbola.release());
}

// lc_ActionDrawHyperbolaFP.cpp - fixed createHyperbola() using temporary hyperbola

RS_Entity* LC_ActionDrawHyperbolaFP::doTriggerCreateEntity() {
    if (!focus1.valid || !focus2.valid || !startPoint.valid || !endPoint.valid) {
        reset();
        return nullptr;
    }

    if (focus1.distanceTo(focus2) < RS_TOLERANCE) {
        commandMessage(tr("Foci cannot be coincident"));
        reset();
        return nullptr;
    }

    if (startPoint.distanceTo(endPoint) < RS_TOLERANCE) {
        commandMessage(tr("Start and end points cannot be the same"));
        reset();
        return nullptr;
    }

    // Temporary hyperbola to access getParamFromPoint()
    LC_Hyperbola temp(nullptr, LC_HyperbolaData(focus1, focus2, startPoint));
    if (!temp.isValid()) {
        commandMessage(tr("Invalid foci or point"));
        reset();
        return nullptr;
    }

    //bool rev = (startPoint.distanceTo(focus1) - startPoint.distanceTo(focus2) < 0.0);
    const bool rev = false;

    const double phiStart = temp.getParamFromPoint(startPoint, rev);
    const double phiEnd = temp.getParamFromPoint(endPoint, rev);

    if (std::isnan(phiStart) || std::isnan(phiEnd)) {
        commandMessage(tr("Points not on hyperbola"));
        reset();
        return nullptr;
    }

    // Final data with correct angles
    LC_HyperbolaData data(temp.getData().getFocus1(), temp.getData().getFocus2(), startPoint);
    data.angle1 = std::min(phiStart, phiEnd);
    data.angle2 = std::max(phiStart, phiEnd);
    data.reversed = rev;

    const auto hyperbola = new LC_Hyperbola(nullptr, data);
    if (hyperbola->isValid()) {
        moveRelativeZero(hyperbola->getCenter());
        hyperbola->calculateBorders();
        hyperbola->setFlag(RS2::FlagVisible);
    }
    else {
        delete hyperbola;
    }
    return hyperbola;
}

void LC_ActionDrawHyperbolaFP::doTriggerCompletion(const bool success) {
    LC_SingleEntityCreationAction::doTriggerCompletion(success);
    setStatus(SetFocus1);
}

void LC_ActionDrawHyperbolaFP::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    if (!snapped.valid) { // fixme - sand - may it really be invalid? check when..
        return;
    }

    switch (status) {
        case SetFocus1: {
            snapped = getRelZeroAwarePoint(e, snapped);
            break;
        }
        case SetFocus2: {
            snapped = getSnapAngleAwarePoint(e, focus1, snapped, false);
            break;
        }
        default:
            break;
    }

    onCoordinateEvent(status, false, snapped);
}

void LC_ActionDrawHyperbolaFP::onMouseMoveEvent(const int status, const LC_MouseEvent* event) {
    switch (status) {
        case SetFocus1: {
            const RS_Vector mouse = getRelZeroAwarePoint(event, event->snapPoint);
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
            }
            break;
        }
        case SetFocus2: {
            const RS_Vector mouse = getSnapAngleAwarePoint(event, focus1,  event->snapPoint, true);
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(focus1);
                previewRefSelectablePoint(mouse);
            }
            focus2 = mouse;
            break;
        }
        case SetStartPoint: {
            const RS_Vector mouse = event->snapPoint;
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(focus1);
                previewRefPoint(focus2);
                previewRefSelectablePoint(mouse);
            }
            startPoint = mouse;
            endPoint.valid = false;
            break;
        }
        case SetEndPoint: {
            const RS_Vector mouse = event->snapPoint;
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(focus1);
                previewRefPoint(focus2);
                previewRefPoint(startPoint);
                previewRefSelectablePoint(mouse);
            }
            endPoint = mouse;
            break;
        }
        default:
            break;
    }
    preparePreview();
}

void LC_ActionDrawHyperbolaFP::onMouseRightButtonRelease(const int status, const LC_MouseEvent*) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawHyperbolaFP::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    if (!pos.valid) {
        return;
    }
    moveRelativeZero(pos);

    switch (status) {
        case SetFocus1: {
            focus1 = pos;
            addSnappedPointToVisualSnap(pos);
            setStatus(SetFocus2);
            break;
        }
        case SetFocus2: {
            if (focus1.distanceTo(pos) < RS_TOLERANCE) {
                commandMessage(tr("Foci cannot be coincident"));
                return;
            }
            focus2 = pos;
            addSnappedPointToVisualSnap(pos);
            setStatus(SetStartPoint);
            break;
        }
        case SetStartPoint: {
            startPoint = pos;
            addSnappedPointToVisualSnap(pos);
            setStatus(SetEndPoint);
            break;
        }
        case SetEndPoint: {
            if (startPoint.distanceTo(pos) < RS_TOLERANCE) {
                commandMessage(tr("Start and end points cannot be the same"));
                return;
            }
            addSnappedPointToVisualSnap(pos);
            endPoint = pos;
            trigger();
            break;
        }
        default:
            break;
    }

    updateActionPrompt();
    m_graphicView->redraw(RS2::RedrawOverlay);
}

void LC_ActionDrawHyperbolaFP::updateActionPrompt() {
    switch (getStatus()) {
        case SetFocus1:
            updatePromptTRCancel(tr("Specify first focus"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetFocus2:
            updatePromptTRCancel(tr("Specify second focus"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetStartPoint:
            updatePromptTRCancel(tr("Specify start point on branch"));
            break;
        case SetEndPoint:
            updatePromptTRCancel(tr("Specify end point on branch"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawHyperbolaFP::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
