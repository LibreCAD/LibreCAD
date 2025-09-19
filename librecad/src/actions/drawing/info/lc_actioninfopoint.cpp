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

#include "lc_actioninfopoint.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_graphicviewport.h"
#include "rs.h"

LC_ActionInfoPoint::LC_ActionInfoPoint(LC_ActionContext* actionContext)
    :RS_PreviewActionInterface("Info Point", actionContext, RS2::ActionInfoPoint), m_position{false}{
}

void LC_ActionInfoPoint::doTrigger() {
    if (m_position.valid){
       RS_Vector relZero = m_viewport->getRelativeZero();
       RS_Vector relative = m_position - relZero;

       commandMessage("--- ");
       bool hasUcs = m_viewport->hasUCS();
       QString message = tr("Absolute: (%1)").arg(formatVector(m_position)).append("\n")
        .append(tr("Relative: (%1)").arg(formatRelative(relative))).append("\n")
        .append(tr("Polar: (%1)").arg(formatPolar(relative))).append("\n")
        .append(tr("Polar Relative: (%1)").arg(formatRelativePolar(relative))).append("\n");
        if (hasUcs) {
            message.append(tr("Absolute WCS: (%1)").arg(formatVectorWCS(m_position))).append("\n");
        }
       commandMessage(message);
    }
}

void LC_ActionInfoPoint::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint: {
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
            }
            updateInfoCursor(mouse, m_viewport->getRelativeZero());
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfoPoint::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &relZero) {
    if (m_infoCursorOverlayPrefs->enabled) {
        msgStart()
            .string (tr("Point Coordinates"))
            .vector(tr("Absolute:"), mouse)
            .polar(tr("Polar:"), mouse)
            .relative(tr("Relative:"), mouse)
            .relativePolar(tr("Relative Polar:"), mouse)
            .vector(tr("Relative Zero:"), relZero)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInfoPoint::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetPoint:{
            // todo - if coordinates of the point should be added to the drawing, mode will be useful
            m_coordinatesType = POS_ABSOLUTE;
            if (e->isControl) {
                m_coordinatesType = POS_RELATIVE;
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

void LC_ActionInfoPoint::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionInfoPoint::onCoordinateEvent(int status, [[maybe_unused]] bool isZero,
                                                         const RS_Vector& mouse) {
    switch (status) {
        case SetPoint: {
            m_position = mouse;
            deletePreview();
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfoPoint::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint:
            updateMouseWidgetTRBack(tr("Select position for coordinates"));
                // MOD_SHIFT_AND_CTRL(tr("Pick relative zero coordinates"), tr("Pick relative coordinates")));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType LC_ActionInfoPoint::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
