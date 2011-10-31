/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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
**********************************************************************/

#include "rs_actiondrawlineorthtan.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_selection.h"


/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", container, graphicView) {

    normal = NULL;
    tangent = NULL;
    circle = NULL;
    circleList.clear();
    circleList.push_back(RS2::EntityArc);
    circleList.push_back(RS2::EntityCircle);
    circleList.push_back(RS2::EntityEllipse);
    success=false;
}


QAction* RS_ActionDrawLineOrthTan::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action = new QAction(tr("Tangent &Orthogonal"), NULL);
    action->setIcon(QIcon(":/extui/linesorthtan.png"));
    return action;
}

void RS_ActionDrawLineOrthTan::trigger() {
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

            setStatus(SetCircle);
        }
        delete tangent;
        tangent = NULL;
    } else {
        RS_DEBUG->print("RS_ActionDrawLineOrthTan::trigger:"
                        " Entity is NULL\n");
    }
}



void RS_ActionDrawLineOrthTan::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent begin");
    if( getStatus() != SetCircle ) return;

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    RS_Entity* en ;
    en = catchEntity(e, circleList, RS2::ResolveAll);
    if (en!=NULL && (en->rtti()==RS2::EntityCircle ||
                     en->rtti()==RS2::EntityArc ||
                     en->rtti()==RS2::EntityEllipse)) {
        circle = en;

        RS_Creation creation(NULL, NULL);
        RS_Line* t = creation.createLineOrthTan(mouse,
                                                normal,
                                                circle);

        if (t!=NULL) {
            if (tangent!=NULL) {
                delete tangent;
                    tangent=NULL;
                    success=true;
            }
            tangent = (RS_Line*)t->clone();

            deletePreview();
            preview->addEntity(t);
            drawPreview();
        }
    }
    RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent end");
}



void RS_ActionDrawLineOrthTan::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        if( normal != NULL) {
            normal->setHighlighted(false);
            graphicView->drawEntity(normal);
        }
            if (tangent != NULL) {
                    delete tangent;
                    tangent=NULL;
            }
        deletePreview();
        if (getStatus() == SetLine) {
                finish(false);
        }else{
                init(getStatus()-1);
        }
    } else {
        switch (getStatus()) {
        case SetLine: {
            success=false;
            RS_Entity* en=catchEntity(e);
            if(en->rtti() == RS2::EntityLine) {
                    if (en->getLength() < RS_TOLERANCE) {
                            //ignore lines not long enough
                            break;
                    }
                if(normal != NULL) {
                    normal->setHighlighted(false);
                    graphicView->drawEntity(normal);
                }
                normal=static_cast<RS_Line*>(en);
                normal->setHighlighted(true);
                graphicView->drawEntity(normal);
                setStatus(SetCircle);
            }
        }
        break;

        case SetCircle:
            if(success){
            trigger();
            }
            break;
        default:
            break;
        }
    }

}



void RS_ActionDrawLineOrthTan::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetLine:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select a line"),
                                                tr("Cancel"));
            break;
        case SetCircle:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawLineOrthTan::updateMouseCursor() {
        if(isFinished()) {
    graphicView->setMouseCursor(RS2::ArrowCursor);
        }else{
    graphicView->setMouseCursor(RS2::CadCursor);
        }
}



void RS_ActionDrawLineOrthTan::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}



// EOF
