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

#include "rs_actiondrawlinetangent2.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"



RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Tangents 2", container, graphicView) {

    tangent = NULL;
    circle1 = NULL;
    circle2 = NULL;
}

QAction* RS_ActionDrawLineTangent2::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Tan&gent (C,C)"),
    QAction* action = new QAction(tr("Tangent (C,C)"), NULL);
	action->setIcon(QIcon(":/extui/linestan2.png"));
    //action->zetStatusTip(tr("Draw tangent (circle, circle)"));

    return action;
}


void RS_ActionDrawLineTangent2::trigger() {
    RS_PreviewActionInterface::trigger();

    if (tangent!=NULL) {
        RS_Entity* newEntity = NULL;

        newEntity = new RS_Line(container,
                                tangent->getData());

        if (newEntity!=NULL) {
            newEntity->setLayerToActive();
            newEntity->setPenToActive();
            container->addEntity(newEntity);

            // upd. undo list:
            if (document!=NULL) {
                document->startUndoCycle();
                document->addUndoable(newEntity);
                document->endUndoCycle();
            }
			graphicView->redraw(RS2::RedrawDrawing);
            setStatus(SetCircle1);
        }
        delete tangent;
        tangent = NULL;
    } else {
        RS_DEBUG->print("RS_ActionDrawLineTangent2::trigger:"
                        " Entity is NULL\n");
    }
}



void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
    case SetCircle1: {
            RS_Entity* en = catchEntity(e, RS2::ResolveAll);
            if (en!=NULL && (en->rtti()==RS2::EntityCircle ||
                             en->rtti()==RS2::EntityArc)) {
                circle1 = en;
            }
        }
        break;

    case SetCircle2: {
            RS_Entity* en = catchEntity(e, RS2::ResolveAll);
            if (en!=NULL && (en->rtti()==RS2::EntityCircle ||
                             en->rtti()==RS2::EntityArc)) {
                circle2 = en;

                RS_Creation creation(NULL, NULL);
                RS_Line* t = creation.createTangent2(mouse,
                                                     circle1,
                                                     circle2);

                if (t!=NULL) {
                    if (tangent!=NULL) {
                        delete tangent;
                    }
                    tangent = (RS_Line*)t->clone();

                    deletePreview();
                    preview->addEntity(t);
                    drawPreview();
                }
            }
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent end");
}



void RS_ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    } else {
        switch (getStatus()) {
        case SetCircle1:
            setStatus(SetCircle2);
            break;

        case SetCircle2:
            trigger();
            break;
        }
    }

}



void RS_ActionDrawLineTangent2::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCircle1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select first circle or arc"),
                                                tr("Cancel"));
            break;
        case SetCircle2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select second circle or arc"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawLineTangent2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLineTangent2::updateToolBar() {
    //not needed any more with new snap
    return;
    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}



// EOF
