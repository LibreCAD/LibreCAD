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

#include<QAction>
#include "rs_actionmodifyoffset.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"


RS_ActionModifyOffset::RS_ActionModifyOffset(RS_EntityContainer& container,
                                   RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw arcs",
                           container, graphicView) {
entity=NULL;
creation=NULL;
distance=0.;
}



RS_ActionModifyOffset::~RS_ActionModifyOffset() {}

QAction* RS_ActionModifyOffset::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action = new QAction(tr("&Offset"),NULL);
    action->setIcon(QIcon(":/extui/arcspara.png"));
    return action;
}


//void RS_ActionModifyOffset::reset() {
//    //bool rev = data.reversed;

//    if (data.reversed) {
//        data = RS_ArcData(RS_Vector(false),
//                          0.0,
//                          2*M_PI, 0.0,
//                          true);
//    } else {
//        data = RS_ArcData(RS_Vector(false),
//                          0.0,
//                          0.0, 2*M_PI,
//                          false);
//    }
//}



void RS_ActionModifyOffset::init(int status) {
    RS_PreviewActionInterface::init(status);

}



void RS_ActionModifyOffset::trigger() {
    RS_PreviewActionInterface::trigger();

    /*
    container->addEntity(arc);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(arc);
        document->endUndoCycle();
    }

        graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(arc->getCenter());
*/
    setStatus(SelectEntity);

//    RS_DEBUG->print("RS_ActionModifyOffset::trigger(): arc added: %d", arc->getId());
}



void RS_ActionModifyOffset::mouseMoveEvent(QMouseEvent* e) {
//    RS_DEBUG->print("RS_ActionModifyOffset::mouseMoveEvent begin");
if(getStatus()==SetPosition){

}
}



void RS_ActionModifyOffset::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch(getStatus()){
        case SelectEntity:
            setStatus(SetPosition);
            break;
            case SetPosition:
            trigger();
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


/*
void RS_ActionModifyOffset::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
        data.center = mouse;
        graphicView->moveRelativeZero(mouse);
        setStatus(SetRadius);
        break;

    case SetRadius:
        if (data.center.valid) {
            data.radius = data.center.distanceTo(mouse);
        }
        setStatus(SetAngle1);
        break;

    case SetAngle1:
        data.angle1 = data.center.angleTo(mouse);
        setStatus(SetAngle2);
        break;

    case SetAngle2:
        data.angle2 = data.center.angleTo(mouse);
        trigger();
        break;

    case SetIncAngle:
        data.angle2 = data.angle1 + data.center.angleTo(mouse);
        trigger();
        break;

    case SetChordLength: {
            double x = data.center.distanceTo(mouse);
            if (fabs(x/(2*data.radius))<=1.0) {
                data.angle2 = data.angle1 + asin(x/(2*data.radius)) * 2;
                trigger();
            }
        }
        break;

    default:
        break;
    }
}
*/

/*
void RS_ActionModifyOffset::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (RS_COMMANDS->checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    if (RS_COMMANDS->checkCommand("reversed", c)) {
        e->accept();
        setReversed(!isReversed());

        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->requestOptions(this, true, true);
        }
        return;
    }

    switch (getStatus()) {

    case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.radius = r;
                setStatus(SetAngle1);
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
                data.angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle2: {
            if (RS_COMMANDS->checkCommand("angle", c)) {
                setStatus(SetIncAngle);
            } else if (RS_COMMANDS->checkCommand("chord length", c)) {
                setStatus(SetChordLength);
            } else {
                bool ok;
                double a = RS_Math::eval(c, &ok);
                if (ok==true) {
                    data.angle2 = RS_Math::deg2rad(a);
                    trigger();
                } else {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                    }
                }
            }
        }
        break;

    case SetIncAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.angle2 = data.angle1 + RS_Math::deg2rad(a);
                trigger();
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetChordLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok==true) {
                if (fabs(l/(2*data.radius))<=1.0) {
                    data.angle2 = data.angle1 + asin(l/(2*data.radius)) * 2;
                    trigger();
                } else {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DIALOGFACTORY->commandMessage(
                            tr("Not a valid chord length"));
                    }
                }
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



QStringList RS_ActionModifyOffset::getAvailableCommands() {
    QStringList cmd;
    cmd += RS_COMMANDS->command("reversed");
    return cmd;
}
*/

void RS_ActionModifyOffset::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SelectEntity:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select Entity to create offset"), tr("Cancel"));
            break;
        case SetPosition:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify direction of offset"), tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionModifyOffset::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestModifyOffsetOptions(distance, true);
    }
}



void RS_ActionModifyOffset::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestModifyOffsetOptions(distance, false);
    }
}



void RS_ActionModifyOffset::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyOffset::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}


// EOF

