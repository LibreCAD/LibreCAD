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
#include "rs_actiondrawlinefree.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionDrawLineFree::RS_ActionDrawLineFree(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw freehand lines",
					container, graphicView)
		,vertex(new RS_Vector{}){
	preview->setOwner(false);
	actionType=RS2::ActionDrawLineFree;
}

RS_ActionDrawLineFree::~RS_ActionDrawLineFree() = default;

void RS_ActionDrawLineFree::doTrigger() {
    if (polyline.get() != nullptr){
        polyline->endPolyline();
        RS_VectorSolutions sol = polyline->getRefPoints();
        if (sol.getNumber() > 2){
            RS_Entity *ent = polyline->clone();
            undoCycleAdd(ent);
            RS_DEBUG->print("RS_ActionDrawLineFree::trigger(): polyline added: %lu", ent->getId());
        }
        polyline.reset();
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
    if (status==Dragging && polyline.get())     {
        const QPointF mousePosition = e->uiPosition;
        if (QLineF(mousePosition,oldMousePosition).length() < 1) {
            //do not add the same mouse position
            return;
        }
        auto ent = static_cast<RS_Polyline*>(polyline->addVertex(v));

        if (ent->count()){
            preview->addCloneOf(polyline.get(), viewport);
        }

        *vertex = v;
        oldMousePosition = mousePosition;
    }
}

void RS_ActionDrawLineFree::onMouseLeftButtonPress([[maybe_unused]]int status, LC_MouseEvent *e) {
    switch(getStatus()){
        case SetStartpoint:
            setStatus(Dragging);
            // fall-through
        case Dragging:
            *vertex = e->snapPoint;
            polyline.reset(new RS_Polyline(container, RS_PolylineData(*vertex, *vertex, false)));
            setPenAndLayerToActive(polyline.get());
            break;
        default:
            break;
    }
}

void RS_ActionDrawLineFree::onMouseLeftButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if(status==Dragging){
        *vertex = {};
        trigger();
    }
}

void RS_ActionDrawLineFree::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if (polyline.get() != nullptr) {
        polyline.reset();
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
