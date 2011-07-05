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

#include "rs_actiondrawlineparallel.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_actiondrawlineparallelthrough.h"



RS_ActionDrawLineParallel::RS_ActionDrawLineParallel(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Parallels", container, graphicView) {

    parallel = NULL;
    entity = NULL;
    distance = 1.0;
    number = 1;
    coord = RS_Vector(false);
}

QAction* RS_ActionDrawLineParallel::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {

    QAction* action = NULL;

    if (type==RS2::ActionDrawLineParallel) {
		// tr("Para&llel"),
        action = new QAction(tr("Parallel"), NULL);
		action->setIcon(QIcon(":/extui/linespara.png"));
    } else if (type==RS2::ActionDrawArcParallel) {
        action = new QAction(tr("Concentric"), NULL);
		action->setIcon(QIcon(":/extui/arcspara.png"));
    } else if (type==RS2::ActionDrawCircleParallel) {
        action = new QAction(tr("Concentric"), NULL);
		action->setIcon(QIcon(":/extui/circlespara.png"));
    }
    //action->zetStatusTip(tr("Draw parallels to existing lines, arcs, "circles"));
    return action;
}


void RS_ActionDrawLineParallel::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Creation creation(container, graphicView);
    RS_Entity* e = creation.createParallel(coord,
                                           distance, number,
                                           entity);

    if (e==NULL) {
        RS_DEBUG->print("RS_ActionDrawLineParallel::trigger:"
                        " No parallels added\n");
    }
}



void RS_ActionDrawLineParallel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent begin");

    coord = RS_Vector(graphicView->toGraphX(e->x()),
                      graphicView->toGraphY(e->y()));

    entity = catchEntity(e, RS2::ResolveAll);

    switch (getStatus()) {
    case SetEntity: {
            deletePreview();

            RS_Creation creation(preview, NULL, false);
            creation.createParallel(coord,
                                    distance, number,
                                    entity);

            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent end");
}



void RS_ActionDrawLineParallel::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        trigger();
    }
}



void RS_ActionDrawLineParallel::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetEntity:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify Distance <%1> or select entity or [%2]")
                .arg(distance).arg(RS_COMMANDS->command("through")),
                tr("Cancel"));
            break;

        case SetNumber:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawLineParallel::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true);
    }
    updateMouseButtonHints();
}



void RS_ActionDrawLineParallel::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



void RS_ActionDrawLineParallel::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetEntity: {
            if (checkCommand("through", c)) {
                finish();
                graphicView->setCurrentAction(
                    new RS_ActionDrawLineParallelThrough(*container,
                                                         *graphicView));
            } else if (checkCommand("number", c)) {
                deletePreview();
                setStatus(SetNumber);
            } else {
                bool ok;
                double d = RS_Math::eval(c, &ok);
                if (ok==true && d>1.0e-10) {
                    distance = d;
                } else {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                    }
                }
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->requestOptions(this, true, true);
                }
                updateMouseButtonHints();
                //setStatus(SetEntity);
            }
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok==true) {
                if (n>0 && n<100) {
                    number = n;
                } else {
                    if (RS_DIALOGFACTORY!=NULL) {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                            "Try 1..99"));
                    }
                }
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->requestOptions(this, true, true);
            }
            setStatus(SetEntity);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawLineParallel::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetEntity:
        cmd += command("number");
        cmd += command("through");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLineParallel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawLineParallel::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarLines);
    }
}


// EOF
