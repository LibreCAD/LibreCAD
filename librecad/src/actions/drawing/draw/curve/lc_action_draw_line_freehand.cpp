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

#include "lc_action_draw_line_freehand.h"

#include <QLineF>

#include "rs_document.h"
#include "rs_polyline.h"
#include "rs_preview.h"

LC_ActionDrawLineFreehand::LC_ActionDrawLineFreehand(LC_ActionContext *actionContext)
    : LC_SingleEntityCreationAction("ActionDrawLineFreehand", actionContext, RS2::ActionDrawLineFreehand),
    m_vertex({0, 0, 0}), m_polyline(nullptr) {
}

LC_ActionDrawLineFreehand::~LC_ActionDrawLineFreehand() = default;


RS_Entity* LC_ActionDrawLineFreehand::doTriggerCreateEntity() {
    if (m_polyline != nullptr){
        m_polyline->endPolyline();
        const RS_VectorSolutions sol = m_polyline->getRefPoints();
        if (sol.getNumber() > 2){
            m_polyline->calculateBorders();
            return m_polyline;
        }
    }
    return nullptr;
}

void LC_ActionDrawLineFreehand::doTriggerCompletion([[maybe_unused]]bool success) {
    if (m_polyline != nullptr) {
        m_polyline = nullptr;
    }
    setStatus(SetStartpoint);
}

/*
 * 11 Aug 2011, Dongxu Li
 */
// todo - relative point snap?
void LC_ActionDrawLineFreehand::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status==Dragging && m_polyline != nullptr)     {
        const QPointF mousePosition = e->uiPosition;
        if (QLineF(mousePosition,m_oldMousePosition).length() < 1) {
            //do not add the same mouse position
            return;
        }

        const RS_Vector v = e->snapPoint;

        m_polyline->addVertex(v);

        if (!m_polyline->isEmpty()) {
            m_preview->addEntity(m_polyline->clone());
        }

        m_vertex = v;
        m_oldMousePosition = mousePosition;
    }
}

void LC_ActionDrawLineFreehand::onMouseLeftButtonPress([[maybe_unused]]int status, const LC_MouseEvent* e) {
    switch(getStatus()){
        case SetStartpoint:
            setStatus(Dragging);
            // fall-through
            [[fallthrough]];
        case Dragging:
            m_vertex = e->snapPoint;
            m_polyline = new RS_Polyline(m_document, RS_PolylineData(m_vertex, m_vertex, false));
            break;
        default:
            break;
    }
}

void LC_ActionDrawLineFreehand::onMouseLeftButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if(status == Dragging){
        m_vertex = {};
        trigger();
    }
}

void LC_ActionDrawLineFreehand::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if (m_polyline != nullptr) {
        delete m_polyline;
        m_polyline = nullptr;
    }
    initPrevious(status);
}

void LC_ActionDrawLineFreehand::updateActionPrompt() {
    switch (getStatus()) {
        case SetStartpoint:
        case Dragging:
            updatePromptTRCancel(tr("Click and drag to draw a line"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawLineFreehand::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
