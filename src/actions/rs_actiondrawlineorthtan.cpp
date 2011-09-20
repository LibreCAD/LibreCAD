/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actiondrawlineorthtan.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_selection.h"


RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", container, graphicView) {

    normal = NULL;
    tangent = NULL;
    circle = NULL;
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
    en = catchEntity(e, RS2::ResolveAll);
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
                finish();
        }else{
                init(getStatus()-1);
        }
    } else {
        switch (getStatus()) {
        case SetLine: {
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
            trigger();
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
    //not needed any more with new snap
    return;
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
        }
    }
}



// EOF
