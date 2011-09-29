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

#include "rs_actionmodifytrimamount.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_modification.h"


RS_ActionModifyTrimAmount::RS_ActionModifyTrimAmount(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Trim Entity by a given amount",
                    container, graphicView) {

    trimEntity = NULL;
    trimCoord = RS_Vector(false);
    distance = 0.0;
    byTotal = false;
}

QAction* RS_ActionModifyTrimAmount::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        // tr("Lengthen")
        QAction* action = new QAction(tr("&Lengthen"), NULL);
        action->setIcon(QIcon(":/extui/modifytrimamount.png"));
        //action->zetStatusTip(tr("Lengthen by a given amount"));
        return action;
}


void RS_ActionModifyTrimAmount::init(int status) {
    RS_ActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
}



void RS_ActionModifyTrimAmount::trigger() {

    RS_DEBUG->print("RS_ActionModifyTrimAmount::trigger()");

    if (trimEntity!=NULL && trimEntity->isAtomic()) {

        RS_Modification m(*container, graphicView);
        double d;
        if(byTotal) {
            //the distance is taken as the new total length
            d = fabs(distance) - trimEntity->getLength();
        } else {
            d = distance;
        }

        m.trimAmount(trimCoord, (RS_AtomicEntity*)trimEntity, d);

        trimEntity = NULL;
        setStatus(ChooseTrimEntity);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    }
}



void RS_ActionModifyTrimAmount::mouseReleaseEvent(QMouseEvent* e) {

    trimCoord = graphicView->toGraph(e->x(), e->y());
    trimEntity = catchEntity(e);

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case ChooseTrimEntity:
            if (trimEntity!=NULL && trimEntity->isAtomic()) {
                trigger();
            } else {
                if (trimEntity==NULL) {
                    RS_DIALOGFACTORY->commandMessage(
                        tr("No entity found. "));
                } else if (trimEntity->rtti()==RS2::EntityInsert) {
                    RS_DIALOGFACTORY->commandMessage(
                        tr("The chosen Entity is in a block. "
                           "Please edit the block."));
                } else {
                    RS_DIALOGFACTORY->commandMessage(
                        tr("The chosen Entity is not an atomic entity "
                           "or cannot be trimmed."));
                }
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionModifyTrimAmount::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case ChooseTrimEntity: {
            bool ok;
            double d = RS_Math::eval(c, &ok);
            if (ok==true) {
                distance = d;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(ChooseTrimEntity);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionModifyTrimAmount::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case ChooseTrimEntity:
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionModifyTrimAmount::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyTrimAmount::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionModifyTrimAmount::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseTrimEntity:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Select entity to trim or enter distance:"),
            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyTrimAmount::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyTrimAmount::updateToolBar() {
    //not needed any more with new snap
    return;
    RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
}


// EOF
