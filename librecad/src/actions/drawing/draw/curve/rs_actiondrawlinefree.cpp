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

#include <QLineF>

#include "rs_actiondrawlinefree.h"
#include "rs_debug.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionDrawLineFree::RS_ActionDrawLineFree(LC_ActionContext *actionContext)
        :RS_PreviewActionInterface("Draw freehand lines", actionContext,RS2::ActionDrawLineFree)
    ,m_vertex(std::make_unique<RS_Vector>()){
	m_preview->setOwner(false);
}

RS_ActionDrawLineFree::~RS_ActionDrawLineFree() = default;

void RS_ActionDrawLineFree::doTrigger() {
    if (m_polyline != nullptr){
        m_polyline->endPolyline();
        RS_VectorSolutions sol = m_polyline->getRefPoints();
        if (sol.getNumber() > 2){
            RS_Entity *entity = m_polyline->clone();
            entity->reparent(m_container);
            entity->calculateBorders();
            undoCycleAdd(entity);
            LC_LOG<<"RS_ActionDrawLineFree::trigger(): polyline added: "<< entity->getId();
        }
        m_polyline.reset();
    }
    setStatus(SetStartpoint);
}

/*
 * 11 Aug 2011, Dongxu Li
 */
// todo - relative point snap?
// fixme - sand - review drawing line free
void RS_ActionDrawLineFree::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector v = e->snapPoint;
    drawSnapper();
    if (status==Dragging && m_polyline != nullptr)     {
        const QPointF mousePosition = e->uiPosition;
        if (QLineF(mousePosition,m_oldMousePosition).length() < 1) {
            //do not add the same mouse position
            return;
        }
        auto ent = static_cast<RS_Polyline*>(m_polyline->addVertex(v));

        if (ent->count()) {
            deletePreview();
            m_preview->addCloneOf(m_polyline.get(), m_viewport);
            drawPreview();
        }

        *m_vertex = v;
        m_oldMousePosition = mousePosition;
    }
}

void RS_ActionDrawLineFree::onMouseLeftButtonPress([[maybe_unused]]int status, LC_MouseEvent *e) {
    switch(getStatus()){
        case SetStartpoint:
            setStatus(Dragging);
            // fall-through
        case Dragging:
            *m_vertex = e->snapPoint;
            m_polyline = std::make_unique<RS_Polyline>(m_container, RS_PolylineData(*m_vertex, *m_vertex, false));
            setPenAndLayerToActive(m_polyline.get());
            break;
        default:
            break;
    }
}

void RS_ActionDrawLineFree::onMouseLeftButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if(status==Dragging){
        *m_vertex = {};
        trigger();
    }
}

void RS_ActionDrawLineFree::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if (m_polyline.get() != nullptr) {
        m_polyline.reset();
    }
    initPrevious(status);
}

void RS_ActionDrawLineFree::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetStartpoint:
        case Dragging:
            updateMouseWidgetTRCancel(tr("Click and drag to draw a line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineFree::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
