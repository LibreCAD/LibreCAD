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
{
          points.clear();

}



RS_ActionDrawEllipse4Points::~RS_ActionDrawEllipse4Points() {}


QAction* RS_ActionDrawEllipse4Points::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Ellipse &4 Point"), NULL);
    action->setIcon(QIcon(":/extui/ellipsefocipoint.png"));
    return action;
}

void RS_ActionDrawEllipse4Points::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status==SetPoint1) {
        points.clear();
    }
}



void RS_ActionDrawEllipse4Points::trigger() {
    RS_PreviewActionInterface::trigger();


    RS_EllipseData ed(center,
                      major,
                      ratio,
                      0., 0.,false);
    RS_Ellipse* ellipse = new RS_Ellipse(container, ed);

    container->addEntity(ellipse);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(ellipse);
        document->endUndoCycle();
    }

    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(rz);
    drawSnapper();

    setStatus(SetPoint1);

    RS_DEBUG->print("RS_ActionDrawEllipse4Point::trigger():"
                    " entity added: %d", ellipse->getId());
}



void RS_ActionDrawEllipse4Points::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if(getStatus() == SetPoint4){


        points.set(3,mouse);
        if (d > c+ RS_TOLERANCE) {
            deletePreview();
            RS_EllipseData ed(center,
                              major*d,
                              sqrt(d*d-c*c)/d,
                              0., 0.,false);
            RS_Ellipse e=new RS_Ellipse(preview, ed));
            e->createFrom4P(points);
            preview->addEntity(e);
            drawPreview();
        }



    }

    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
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

    switch (getStatus()) {
    case SetPoint1:
        points.clear();
    case SetPoint2:
    case SetPoint3:
        graphicView->moveRelativeZero(mouse);
        points.push_back(mouse);
        break;

    case SetPoint4:
        points.push_back(mouse);
    {
        RS_EllipseData ed(center,
                          major*d,
                          sqrt(d*d-c*c)/d,
                          0., 0.,false);
        RS_Ellipse e=new RS_Ellipse(preview, ed));
        if(e->createFrom4P(points)) {//trigger
            graphicView->moveRelativeZero(mouse);
            container->addEntity(e);

            // upd. undo list:
            if (document!=NULL) {
                document->startUndoCycle();
                document->addUndoable(e);
                document->endUndoCycle();
            }

            RS_Vector rz = graphicView->getRelativeZero();
            graphicView->redraw(RS2::RedrawDrawing);
            graphicView->moveRelativeZero(rz);
            drawSnapper();
           points.clear();
            setStatus(SetPoint1);
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



void RS_ActionDrawEllipse4Points::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}

// EOF
