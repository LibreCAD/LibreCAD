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

#include "lc_action_draw_circle_base.h"

#include "rs_ellipse.h"

LC_ActionDrawCircleBase::LC_ActionDrawCircleBase(const QString& name, LC_ActionContext* actionContext, const RS2::ActionType type)
    : LC_SingleEntityCreationAction(name, actionContext, type) {
}

LC_ActionDrawCircleBase::~LC_ActionDrawCircleBase() = default;

void LC_ActionDrawCircleBase::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawCircleBase::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawCircleBase::init(const int status) {
    reset(); // fixme - review implmentation in inherited actions
    m_moveRelPointAtCenterAfterTrigger = true; // todo - read from options?
    RS_PreviewActionInterface::init(status);
}

// fixme - resume method - re-read from options

RS2::CursorType LC_ActionDrawCircleBase::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionDrawCircleBase::reset() {
}

void LC_ActionDrawCircleBase::previewEllipseReferencePoints(const RS_Ellipse* ellipse, const bool drawAxises,
                                                            const bool allPointsNotSelectable, const RS_Vector& mouse) const {
    if (m_showRefEntitiesOnPreview) {
        const RS_Vector center = ellipse->getCenter();
        const RS_Vector majorP = ellipse->getMajorP();
        const RS_Vector& major1 = center - majorP;
        const RS_Vector& major2 = center + majorP;
        const RS_Vector& minor1 = ellipse->getMinorPoint();
        const RS_Vector& minor2 = center - RS_Vector(-majorP.y, majorP.x) * ellipse->getRatio();
        if (allPointsNotSelectable) {
            previewRefPoint(minor1);
            previewRefPoint(minor2);
        }
        else {
            previewRefSelectablePoint(minor1);
            previewRefSelectablePoint(minor2);
        }
        previewRefPoint(center);

        if (drawAxises) {
            if (mouse.valid) {
                RS_Vector minor;
                if (minor1.distanceTo(mouse) < minor2.distanceTo(mouse)) {
                    minor = minor1;
                }
                else {
                    minor = minor2;
                }

                previewRefPoint(major1);
                previewRefPoint(major2);

                previewRefLine(center, minor);
            }
            else {
                previewRefLine(major1, major2);
                previewRefLine(minor1, minor2);
                previewRefSelectablePoint(major1);
                previewRefSelectablePoint(major2);
            }
        }
        else {
            if (allPointsNotSelectable) {
                previewRefPoint(major1);
                previewRefPoint(major2);
            }
            else {
                previewRefSelectablePoint(major1);
                previewRefSelectablePoint(major2);
            }
        }
    }
}
