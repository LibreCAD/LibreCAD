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

#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawlinefree.h"
#include "rs_polyline.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDrawLineFree::RS_ActionDrawLineFree(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw freehand lines",
					container, graphicView)
		,vertex(new RS_Vector{})
{
	preview->setOwner(false);
	actionType=RS2::ActionDrawLineFree;
}

RS_ActionDrawLineFree::~RS_ActionDrawLineFree() = default;

void RS_ActionDrawLineFree::trigger() {
	deleteSnapper();
	if (polyline.get()) {
		deletePreview();

		polyline->endPolyline();
		RS_VectorSolutions sol=polyline->getRefPoints();
		if(sol.getNumber() > 2 ) {
			RS_Entity* ent=polyline->clone();
			container->addEntity(ent);
			if (document) {
				document->startUndoCycle();
				document->addUndoable(ent);
				document->endUndoCycle();
			}
			graphicView->redraw(RS2::RedrawDrawing);
			RS_DEBUG->print("RS_ActionDrawLineFree::trigger():"
							" polyline added: %d", ent->getId());
		}
		polyline.reset();
	}
	setStatus(SetStartpoint);
}

/*
 * 11 Aug 2011, Dongxu Li
 */

void RS_ActionDrawLineFree::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector v = snapPoint(e);
    drawSnapper();
    if (getStatus()==Dragging && polyline.get()) {
		if( (graphicView->toGui(v) - graphicView->toGui(*vertex)).squared()< 1. ){
            //do not add the same mouse position
            return;
        }
        auto ent = static_cast<RS_Polyline*>(polyline->addVertex(v));
        if (ent->count()){
            preview->addCloneOf(ent);
            drawPreview();
        }

		*vertex = v;

        RS_DEBUG->print("RS_ActionDrawLineFree::%s:"
                        " line added: %d", __func__, ent->getId());
    }
}



void RS_ActionDrawLineFree::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch(getStatus()){
        case SetStartpoint:
            setStatus(Dragging);
            // fall-through
        case Dragging:
			*vertex = snapPoint(e);
			polyline.reset(new RS_Polyline(container,
									   RS_PolylineData(*vertex, *vertex, 0))
						   );
            polyline->setLayerToActive();
            polyline->setPenToActive();
            break;
        default:
            break;

        }
    }
    //else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton && !vertex.valid) {
    //}
}

void RS_ActionDrawLineFree::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        if(getStatus()==Dragging){
		*vertex = {};
        trigger();
        }
    } else if (e->button()==Qt::RightButton) {
		if (polyline.get()) {
			polyline.reset();
        }
        init(getStatus()-1);
    }
}

void RS_ActionDrawLineFree::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
    case Dragging:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Click and drag to draw a line"), tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawLineFree::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
