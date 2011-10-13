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

#include "rs_actiondrawellipsefocipoint.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseFociPoint::RS_ActionDrawEllipseFociPoint(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw ellipse with axis",
                           container, graphicView),
          focus1(false),
          focus2(false),
          point(false)
{//nothing

}



RS_ActionDrawEllipseFociPoint::~RS_ActionDrawEllipseFociPoint() {}


QAction* RS_ActionDrawEllipseFociPoint::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Ellipse &Foci Point"), NULL);
    action->setIcon(QIcon(":/extui/ellipsefocipoint.png"));
    return action;
}

void RS_ActionDrawEllipseFociPoint::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status==SetFocus1) {
        focus1.valid=false;
    }
}



void RS_ActionDrawEllipseFociPoint::trigger() {
    RS_PreviewActionInterface::trigger();


    RS_EllipseData ed(center,
                      major*d,
                      sqrt(d*d-c*c)/d,
                      0., 0.,false);
    RS_Ellipse* ellipse = new RS_Ellipse(container, ed);
    ellipse->setLayerToActive();
    ellipse->setPenToActive();

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

    setStatus(SetFocus1);

    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::trigger():"
                    " entity added: %d", ellipse->getId());
}



void RS_ActionDrawEllipseFociPoint::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {


    case SetPoint:
        point=mouse;
        d=0.5*(focus1.distanceTo(point)+focus2.distanceTo(point));
        if (d > c+ RS_TOLERANCE) {
            deletePreview();
            RS_EllipseData ed(center,
                              major*d,
                              sqrt(d*d-c*c)/d,
                              0., 0.,false);
            preview->addEntity(new RS_Ellipse(preview, ed));
            drawPreview();
        }
        break;



    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::mouseMoveEvent end");
}



void RS_ActionDrawEllipseFociPoint::mouseReleaseEvent(QMouseEvent* e) {
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


void RS_ActionDrawEllipseFociPoint::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetFocus1:
        focus1=mouse;
        setStatus(SetFocus2);
        break;

    case SetFocus2:
        c=0.5*focus1.distanceTo(mouse);
        if(c>RS_TOLERANCE){
        focus2=mouse;
        center=(focus1+focus2)*0.5;
        major=focus1-center;
        major /= c ;
        setStatus(SetPoint);
        }
        break;
    case SetPoint:
        point=mouse;
        d=0.5*(focus1.distanceTo(point)+focus2.distanceTo(point));
        if (d > c+ RS_TOLERANCE) {
            trigger();
        }
        break;


    default:
        break;
    }
}

//fixme, support command line

/*
void RS_ActionDrawEllipseFociPoint::commandEvent(RS_CommandEvent* e) {
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


QStringList RS_ActionDrawEllipseFociPoint::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawEllipseFociPoint::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetFocus1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first focus of ellipse"),
                                                tr("Cancel"));
            break;

        case SetFocus2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second focus of ellipse"),
                                                tr("Back"));
            break;

        case SetPoint:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify a point on ellipse"),
                tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawEllipseFociPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawEllipseFociPoint::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}

// EOF
