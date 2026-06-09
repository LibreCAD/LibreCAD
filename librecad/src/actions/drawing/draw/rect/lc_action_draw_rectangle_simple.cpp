/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include "lc_action_draw_rectangle_simple.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "rs_document.h"
#include "rs_polyline.h"
#include "rs_preview.h"

struct RS_ActionDrawLineRectangle::ActionData {
    /**
     * 1st corner.
     */
    RS_Vector corner1;
    /**
     * 2nd corner.
     */
    RS_Vector corner2;
};

RS_ActionDrawLineRectangle::RS_ActionDrawLineRectangle(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawLineRectangle", actionContext, RS2::ActionDrawLineRectangle),
      m_actionData(std::make_unique<ActionData>()) {
}

RS_ActionDrawLineRectangle::~RS_ActionDrawLineRectangle() = default;

RS_Entity* RS_ActionDrawLineRectangle::doTriggerCreateEntity() {
    auto* polyline = new RS_Polyline(m_document);

    // create and add rectangle:
    const RS_Vector worldCorner1 = m_actionData->corner1;
    const RS_Vector worldCorner3 = m_actionData->corner2;

    RS_Vector worldCorner2, worldCorner4;
    calcRectCorners(worldCorner1, worldCorner3, worldCorner2, worldCorner4);

    polyline->addVertex(worldCorner1);
    polyline->addVertex(worldCorner2);
    polyline->addVertex(worldCorner3);
    polyline->addVertex(worldCorner4);
    polyline->setClosed(true);
    polyline->endPolyline();

    moveRelativeZero(worldCorner3);

    return polyline;
}

bool RS_ActionDrawLineRectangle::isInVisualSnapStatus(int status) {
    return (status == SetCorner1) || (status == SetCorner2);
}

void RS_ActionDrawLineRectangle::doTriggerCompletion([[maybe_unused]] bool success) {
}

void RS_ActionDrawLineRectangle::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetCorner1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetCorner2: {
            if (m_actionData->corner1.valid) {
                m_actionData->corner2 = mouse;

                const RS_Vector worldCorner1 = m_actionData->corner1;
                const RS_Vector worldCorner3 = m_actionData->corner2;

                RS_Vector worldCorner2, worldCorner4;
                calcRectCorners(worldCorner1, worldCorner3, worldCorner2, worldCorner4);

                m_preview->addRectangle(worldCorner1, worldCorner2, worldCorner3, worldCorner4);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->corner1);
                    previewRefPoint(m_actionData->corner2);
                    previewRefPoint((m_actionData->corner1 + m_actionData->corner2) * 0.5); // center of rect
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("To be created:"), tr("Rectangle")).linear(tr("Width:"), abs(m_actionData->corner1.x - m_actionData->corner2.x)).
                                                               linear(tr("Height:"), abs(m_actionData->corner1.y - m_actionData->corner2.y))
                                                              .vector(tr("Center:"), (m_actionData->corner1 + m_actionData->corner2) * 0.5).
                                                               toInfoCursorZone2(false);
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRectangle::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawLineRectangle::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineRectangle::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetCorner1: {
            m_actionData->corner1 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetCorner2);
            break;
        }
        case SetCorner2: {
            m_actionData->corner2 = coord;
            addSnappedPointToVisualSnap(coord);
            trigger();
            setStatus(SetCorner1);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRectangle::updateActionPrompt() {
    switch (getStatus()) {
        case SetCorner1:
            updatePromptTRCancel(tr("Specify first corner"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCorner2:
            updatePromptTRBack(tr("Specify second corner"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineRectangle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
