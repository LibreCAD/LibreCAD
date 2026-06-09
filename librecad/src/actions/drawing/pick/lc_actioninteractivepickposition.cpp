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

#include "lc_actioninteractivepickposition.h"

#include "lc_actioncontext.h"
#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"

class LC_ActionInteractivePickBase;

LC_ActionInteractivePickPosition::LC_ActionInteractivePickPosition(LC_ActionContext *actionContext, const RS2::ActionType actionType)
    :LC_ActionInteractivePickBase("InteractivePickPosition", actionContext, actionType){
}

LC_ActionInteractivePickPosition::~LC_ActionInteractivePickPosition() = default;

void LC_ActionInteractivePickPosition::init(const int status) {
    LC_ActionInteractivePickBase::init(status);
}

void LC_ActionInteractivePickPosition::doSetInteractiveInputValue(LC_ActionContext::InteractiveInputInfo* interactiveInputInfo) {
    interactiveInputInfo->wcsPoint = m_wcsPosition;
}

bool LC_ActionInteractivePickPosition::isInteractiveDataValid() {
    return m_wcsPosition.valid;
}

void LC_ActionInteractivePickPosition::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint: {
            if (e->isShift) {
                m_coordinatesType = POS_RELATIVE_ZERO;
                fireCoordinateEvent(mouse);
            }
            else {
                if (m_showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                }
                updateInfoCursor(mouse, m_viewport->getRelativeZero());
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickPosition::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &relZero) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        msgStart()
            .string (tr("Pick Coordinates"))
            .vector(tr("Absolute:"), mouse)
            .polar(tr("Polar:"), mouse)
            .relative(tr("Relative:"), mouse)
            .relativePolar(tr("Relative Polar:"), mouse)
            .vector(tr("Relative Zero:"), relZero)
            .toInfoCursorZone2(false);
    }
}

bool LC_ActionInteractivePickPosition::isInVisualSnapStatus(int status) {
    return (status == SetPoint);
}

void LC_ActionInteractivePickPosition::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector snap = e->snapPoint;
    switch (status){
        case SetPoint:{
            m_coordinatesType = POS_ABSOLUTE;
            if (e->isControl && !m_viewport->hasUCS()) {
                m_coordinatesType = POS_RELATIVE;  // fixme - how to process relative coordinates if UCS applied??? It should be ucsDelta...
            }
            else if (e->isShift) {
                m_coordinatesType = POS_RELATIVE_ZERO;
            }
            fireCoordinateEvent(snap);
            // moveRelativeZero(m_actionData->point1);
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickPosition::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    if (status == InitialActionStatus) {
        skipInteractiveInput();
    }
    initPrevious(status);
}

void LC_ActionInteractivePickPosition::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero,
                                                         const RS_Vector& pos) {
    switch (status) {
        case SetPoint: {
            switch (m_coordinatesType) {
                case POS_ABSOLUTE: {
                    m_wcsPosition = pos;
                    break;
                }
                case POS_RELATIVE: {
                    const RS_Vector relativePosition = m_viewport->getRelativeZero();
                    const RS_Vector relative = pos - relativePosition;
                    m_wcsPosition = relative;
                    break;
                }
                case POS_RELATIVE_ZERO: {
                    const RS_Vector relativePosition = m_viewport->getRelativeZero();
                    m_wcsPosition = relativePosition;
                    break;
                }
                default:
                    break;
            }
            deletePreview();
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickPosition::updateActionPrompt(){
    switch (getStatus()) {
        case SetPoint:
            updatePromptTRBack(tr("Pick coordinates from the drawing"),
                MOD_SHIFT_AND_CTRL(tr("Pick relative zero coordinates"), m_viewport->hasUCS() ? "" : tr("Pick relative coordinates")));
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionInteractivePickPosition::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
