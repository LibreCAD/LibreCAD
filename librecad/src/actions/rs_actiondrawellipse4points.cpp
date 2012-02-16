/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

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

#include "rs_actiondrawellipse4points.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipse4Points::RS_ActionDrawEllipse4Points(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw ellipse from 4 points",
                           container, graphicView),
          cData(RS_Vector(0.,0.),1.),
          eData(RS_Vector(0.,0.),RS_Vector(1.,0),1.,0.,0.,false)
{
          points.clean();
}



RS_ActionDrawEllipse4Points::~RS_ActionDrawEllipse4Points() {
    points.clean();
}


QAction* RS_ActionDrawEllipse4Points::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Ellipse &4 Point"), NULL);
    action->setIcon(QIcon(":/extui/ellipse4points.png"));
    return action;
}

void RS_ActionDrawEllipse4Points::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(getStatus() == SetPoint1) points.clean();
}



void RS_ActionDrawEllipse4Points::trigger() {
    RS_PreviewActionInterface::trigger();
    RS_Entity* en;
    if(getStatus()==SetPoint4&&evalid){
        en=new RS_Ellipse(container, eData);
    }else{
        en=new RS_Circle(container, cData);
    }

    // update undo list:
    deletePreview();
    container->addEntity(en);
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(en);
        document->endUndoCycle();
    }
    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(rz);
    drawSnapper();
    setStatus(SetPoint1);
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::trigger():" " entity added: %d", ellipse->getId());

    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::trigger():" " entity added: %d", ellipse->getId());
}



void RS_ActionDrawEllipse4Points::mouseMoveEvent(QMouseEvent* e) {
//    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    points.set(getStatus(),mouse);
    if(preparePreview()) {
        switch(getStatus()) {


        case SetPoint2:
        case SetPoint3:
        {
            RS_Circle* circle=new RS_Circle(preview, cData);
            deletePreview();
            preview->addEntity(circle);
            drawPreview();
        }
            break;
        case SetPoint4:
        {
            deletePreview();
            RS_Ellipse* e=new RS_Ellipse(preview, eData);
            preview->addEntity(e);
            drawPreview();
        }
        default:
            break;
        }

    }
//    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
}


bool RS_ActionDrawEllipse4Points::preparePreview(){
    valid=false;
    switch(getStatus()) {
    case SetPoint2:
    case SetPoint3:
    {
        RS_Circle c(preview,cData);
        valid= c.createFrom3P(points);
        if(valid){
            cData=c.getData();
        }

    }
        break;
    case SetPoint4:
    {
        int j=SetPoint4;
        evalid=false;
        if( (points.get(j) - points.get(j-1)).squared() <RS_TOLERANCE*RS_TOLERANCE){
            RS_Circle c(preview,cData);
            valid= c.createFrom3P(points);
            if(valid){
                cData=c.getData();
            }
        }else{
            RS_Ellipse e(preview,eData);
            valid= e.createFrom4P(points);
            if(valid){
                evalid=valid;
                eData=e.getData();
            }
        }
    }
        break;
    default:
        break;
    }
    return valid;
}

void RS_ActionDrawEllipse4Points::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }

    // Return to last status:
    else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionDrawEllipse4Points::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();
    points.alloc(getStatus()+1);
    points.set(getStatus(),mouse);

    switch (getStatus()) {
    case SetPoint1:
        graphicView->moveRelativeZero(mouse);
        setStatus(SetPoint2);
        break;
    case SetPoint2:
    case SetPoint3:
    case SetPoint4:

        if( preparePreview()) {
            graphicView->moveRelativeZero(mouse);
            if(getStatus() == SetPoint4 ||
                    (points.get(getStatus()) - points.get(getStatus()-1)).squared() <RS_TOLERANCE*RS_TOLERANCE) {
                //also draw the entity, if clicked on the same point twice
                trigger();
            }else{
                setStatus(getStatus()+1);
            }
        }

    default:
        break;
    }
}

//fixme, support command line

/*
void RS_ActionDrawEllipse4Point::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok==true) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    default:
        break;
    }
}
*/


QStringList RS_ActionDrawEllipse4Points::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawEllipse4Points::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetPoint1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first point on ellipse"),
                                                tr("Cancel"));
            break;

        case SetPoint2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second point on ellipse"),
                                                tr("Back"));
            break;

        case SetPoint3:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the third point on ellipse"),
                                                tr("Back"));
            break;

        case SetPoint4:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the fourth point on ellipse"),
                                                tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawEllipse4Points::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionDrawEllipse4Points::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}

// EOF
