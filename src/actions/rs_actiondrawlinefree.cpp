/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"



RS_ActionDrawLineFree::RS_ActionDrawLineFree(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Draw freehand lines",
                    container, graphicView) {
    vertex = RS_Vector(false);
    polyline = NULL;
}

RS_ActionDrawLineFree::~RS_ActionDrawLineFree() {
    if (polyline!=NULL) {
        delete polyline;
    }
}

QAction* RS_ActionDrawLineFree::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr(""Line: Freehand"),
    QAction* action = new QAction(tr("&Freehand Line"), NULL);
	action->setIcon(QIcon(":/extui/linesfree.png"));
    //action->zetStatusTip(tr("Draw freehand lines"));
    return action;
}

void RS_ActionDrawLineFree::trigger() {
    if (polyline!=NULL) {
            polyline->endPolyline();
            RS_VectorSolutions sol=polyline->getRefPoints();
            if(sol.getNumber() > 2 ) {
                container->addEntity(polyline);
                if (document) {
                        document->startUndoCycle();
                        document->addUndoable(polyline);
                        document->endUndoCycle();
                }
		graphicView->redraw(RS2::RedrawDrawing);
            }
        RS_DEBUG->print("RS_ActionDrawLineFree::trigger():"
                        " polyline added: %d", polyline->getId());

        polyline = NULL;
    }
}

/*
 * 11 Aug 2011, Dongxu Li
 * ToDo, show the line while drawing
 */

void RS_ActionDrawLineFree::mouseMoveEvent(QMouseEvent* e) {
    if (vertex.valid && polyline!=NULL) {
        RS_Vector v = snapPoint(e);
        RS_Entity* ent = polyline->addVertex(v);
        ent->setLayerToActive();
        ent->setPenToActive();

        graphicView->drawEntity(ent);
        drawSnapper();

        vertex = v;

        RS_DEBUG->print("RS_ActionDrawLineFree::mouseMoveEvent():"
                        " line added: %d", ent->getId());
    }
}



void RS_ActionDrawLineFree::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        vertex = snapPoint(e);
        polyline = new RS_Polyline(container,
                                   RS_PolylineData(vertex, vertex, 0));
        polyline->setLayerToActive();
        polyline->setPenToActive();
    }
    //else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton && !vertex.valid) {
    //}
}



void RS_ActionDrawLineFree::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        vertex = RS_Vector(false);
        trigger();
    } else if (e->button()==Qt::RightButton) {
        if (polyline!=NULL) {
            delete polyline;
            polyline = NULL;
        }
        init(getStatus()-1);
    }
}



void RS_ActionDrawLineFree::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Click and drag to draw a line"), tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawLineFree::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLineFree::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}

// EOF
