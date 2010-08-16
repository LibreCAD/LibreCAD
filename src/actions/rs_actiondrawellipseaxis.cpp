/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actiondrawellipseaxis.h"
#include "rs_snapper.h"

/**
 * Constructor.
 *
 * @param isArc true if this action will produce an ellipse arc. 
 *              false if it will produce a full ellipse.
 */
RS_ActionDrawEllipseAxis::RS_ActionDrawEllipseAxis(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    bool isArc)
        :RS_PreviewActionInterface("Draw ellipse with axis",
                           container, graphicView) {

    this->isArc = isArc;
    center = RS_Vector(false);
    major = RS_Vector(false);
    ratio = 0.5;
    angle1 = 0.0;
    angle2 = 2*M_PI;
}



RS_ActionDrawEllipseAxis::~RS_ActionDrawEllipseAxis() {}


QAction* RS_ActionDrawEllipseAxis::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;

    if (type==RS2::ActionDrawEllipseArcAxis) {
/* RVT_PORT        action = new QAction(tr("Ellipse Arc with Axis"),
                             tr("&Ellipse Arc (Axis)"), QKeySequence(), NULL); */
        action = new QAction(tr("Ellipse Arc with Axis"), NULL);
        action->setStatusTip(tr("Draw Ellipse Arcs"));
    } else {
/* RVT_PORT        action = new QAction(tr("Ellipse with Axis"),
                             tr("&Ellipse (Axis)"), QKeySequence(), NULL); */
        action = new QAction(tr("Ellipse with Axis"), NULL);
        action->setStatusTip(tr("Draw Ellipses"));
    }
    return action;
}

void RS_ActionDrawEllipseAxis::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status==SetCenter) {
        center = RS_Vector(false);
    }
    if (status<=SetMajor) {
        major = RS_Vector(false);
    }
    if (status<=SetMinor) {
        ratio = 0.5;
    }
    if (status<=SetAngle1) {
        angle1 = 0.0;
    }
    if (status<=SetAngle2) {
        angle2 = 2*M_PI;
    }
}



void RS_ActionDrawEllipseAxis::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_EllipseData ellipseData(center, major,
                               ratio,
                               angle1, angle2,
                               false);
    RS_Ellipse* ellipse = new RS_Ellipse(container, ellipseData);
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

    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawEllipseAxis::trigger():"
                    " entity added: %d", ellipse->getId());
}



void RS_ActionDrawEllipseAxis::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawEllipseAxis::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetCenter:
        break;

    case SetMajor:
        if (center.valid) {
            deletePreview();
            RS_EllipseData ed(center, mouse-center,
                              0.5,
                              0.0, 2*M_PI,
                              false);
            preview->addEntity(new RS_Ellipse(preview, ed));
            drawPreview();
        }
        break;

    case SetMinor:
        if (center.valid && major.valid) {
            deletePreview();
            RS_Line line(NULL, RS_LineData(center-major, center+major));
            double d = line.getDistanceToPoint(mouse);
            ratio = d/(line.getLength()/2);
            RS_EllipseData ed(center, major,
                              ratio,
                              0.0, 2*M_PI,
                              false);
            preview->addEntity(new RS_Ellipse(preview, ed));
            drawPreview();
        }
        break;

    case SetAngle1:
        if (center.valid && major.valid) {
            deletePreview();
			
            //angle1 = center.angleTo(mouse);

			RS_Vector m = mouse;
			m.rotate(center, -major.angle());
			RS_Vector v = m-center;
			v.scale(RS_Vector(1.0, 1.0/ratio));
			angle1 = v.angle(); // + major.angle();

            preview->addEntity(new RS_Line(preview, RS_LineData(center, mouse)));
			
            RS_EllipseData ed(center, major,
                              ratio,
                              angle1, angle1+1.0,
                              false);
            preview->addEntity(new RS_Ellipse(preview, ed));
            drawPreview();
        }
        break;

    case SetAngle2:
        if (center.valid && major.valid) {
            deletePreview();
            //angle2 = center.angleTo(mouse);
			
			RS_Vector m = mouse;
			m.rotate(center, -major.angle());
			RS_Vector v = m-center;
			v.scale(RS_Vector(1.0, 1.0/ratio));
			angle2 = v.angle(); // + major.angle();

            preview->addEntity(new RS_Line(preview, RS_LineData(center, mouse)));
			
            RS_EllipseData ed(
                center, major,
                ratio,
                angle1, angle2,
                false);
            preview->addEntity(new RS_Ellipse(preview, ed));
            drawPreview();
        }

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawEllipseAxis::mouseMoveEvent end");
}



void RS_ActionDrawEllipseAxis::mouseReleaseEvent(RS_MouseEvent* e) {
    // Proceed to next status
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }

    // Return to last status:
    else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionDrawEllipseAxis::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
        center = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetMajor);
        break;

    case SetMajor:
        major = mouse-center;
        setStatus(SetMinor);
        break;

    case SetMinor: {
            RS_Line line(NULL, RS_LineData(center-major, center+major));
            double d = line.getDistanceToPoint(mouse);
            ratio = d/(line.getLength()/2);
            if (!isArc) {
                trigger();
                setStatus(SetCenter);
            } else {
                setStatus(SetAngle1);
            }
        }
        break;

    case SetAngle1: {
        //angle1 = center.angleTo(mouse);
		RS_Vector m = mouse;
		m.rotate(center, -major.angle());
		RS_Vector v = m-center;
		v.scale(RS_Vector(1.0, 1.0/ratio));
		angle1 = v.angle();
        setStatus(SetAngle2);
		} break;

    case SetAngle2: {
        //angle2 = center.angleTo(mouse);
		RS_Vector m = mouse;
		m.rotate(center, -major.angle());
		RS_Vector v = m-center;
		v.scale(RS_Vector(1.0, 1.0/ratio));
		angle2 = v.angle();
        trigger();
        } break;

    default:
        break;
    }
}



void RS_ActionDrawEllipseAxis::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetMinor: {
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



RS_StringList RS_ActionDrawEllipseAxis::getAvailableCommands() {
    RS_StringList cmd;
    return cmd;
}



void RS_ActionDrawEllipseAxis::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCenter:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify ellipse center"),
                                                tr("Cancel"));
            break;

        case SetMajor:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify endpoint of major axis"),
                                                tr("Back"));
            break;

        case SetMinor:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify endpoint or length of minor axis:"),
                tr("Back"));
            break;

        case SetAngle1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify start angle"),
                                                tr("Back"));
            break;

        case SetAngle2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify end angle"),
                                                tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawEllipseAxis::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawEllipseAxis::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarEllipses);
        }
    }
}

// EOF
