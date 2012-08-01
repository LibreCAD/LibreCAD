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
        :RS_PreviewActionInterface("Draw Tangents 2", container, graphicView),
          valid(false)
{

    circle1 = NULL;
    circle2 = NULL;

    circleType.clear();
    circleType<<RS2::EntityArc<<RS2::EntityCircle<<RS2::EntityEllipse;
    setStatus(SetCircle1);
}

QAction* RS_ActionDrawLineTangent2::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        // tr("Tan&gent (C,C)"),
    QAction* action = new QAction(tr("Tangent (C,C)"), NULL);
        action->setIcon(QIcon(":/extui/linestan2.png"));
    //action->zetStatusTip(tr("Draw tangent (circle, circle)"));

    return action;
}

//void RS_ActionDrawLineTangent2::init(int status) {
//    RS_PreviewActionInterface::init(status);
//    circle1->setHighlighted(false);
//    graphicView->redraw(RS2::RedrawDrawing);
//}

void RS_ActionDrawLineTangent2::finish(bool updateTB){
    if(circle1!=NULL){
        circle1->setHighlighted(false);
        graphicView->redraw(RS2::RedrawDrawing);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Entity* newEntity = NULL;

    newEntity = new RS_Line(container, lineData);

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
        if(circle1!=NULL){
            circle1->setHighlighted(false);
            graphicView->redraw(RS2::RedrawDrawing);
            circle1=NULL;
        }

        setStatus(SetCircle1);
    }
    tangent.reset();
}



void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e) {
//    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");
    if(getStatus() != SetCircle2) return;
    circle2= catchEntity(e, circleType, RS2::ResolveAll);
    if(circle2==NULL) return;
    if(circle2->rtti()!=RS2::EntityCircle &&
            circle2->rtti()!=RS2::EntityEllipse &&
            circle2->rtti()!=RS2::EntityArc
            ) {
        circle2=NULL;
        return;
    }
    RS_Creation creation(NULL, NULL);
    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));
    tangent.reset(creation.createTangent2(mouse,
                                          circle1,
                                          circle2));
    if(tangent.get()==NULL) {
        valid=false;
        return;
    }
    valid=true;
    lineData=tangent->getData();

    deletePreview();
    preview->addEntity(new RS_Line(preview, lineData));
    drawPreview();
}



void RS_ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        deletePreview();
        if(getStatus()>=0){
            init(getStatus()-1);
            if(circle1!=NULL){
                circle1->setHighlighted(false);
                graphicView->redraw(RS2::RedrawDrawing);
                deletePreview();
            }
        }
        return;
    }
    switch (getStatus()) {
    case SetCircle1:
    {
        circle1 = catchEntity(e, circleType, RS2::ResolveAll);
        if(circle1==NULL) return;
        if(circle1->rtti()!=RS2::EntityCircle &&
                circle1->rtti()!=RS2::EntityEllipse &&
                circle1->rtti()!=RS2::EntityArc
                ) {
            circle1=NULL;
            return;
        }
        circle1->setHighlighted(true);
        setStatus(getStatus()+1);
        graphicView->redraw(RS2::RedrawDrawing);
    }
        break;

    case SetCircle2:
        if(valid) trigger();
        break;
    }

}



void RS_ActionDrawLineTangent2::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCircle1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select first circle or ellipse"),
                                                tr("Cancel"));
            break;
        case SetCircle2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select second circle or ellipse"),
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



//void RS_ActionDrawLineTangent2::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}



// EOF
