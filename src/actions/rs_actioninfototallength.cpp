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

#include "rs_actioninfototallength.h"

#include "rs_actionselectsingle.h"
#include "rs_modification.h"
#include "rs_snapper.h"
#include "rs_point.h"



RS_ActionInfoTotalLength::RS_ActionInfoTotalLength(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Info Total Length",
                    container, graphicView) {}


QAction* RS_ActionInfoTotalLength::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Total length of selected entities")
    QAction* action = new QAction(tr("&Total length of selected entities"), NULL);
    //action->zetStatusTip(tr("Measures the total length of all selected entities"));
	action->setIcon(QIcon(":/extui/infototallength.png"));
    return action;
}

void RS_ActionInfoTotalLength::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}


void RS_ActionInfoTotalLength::trigger() {

    RS_DEBUG->print("RS_ActionInfoTotalLength::trigger()");

    double len = 0.0;
    for (RS_Entity* e = container->firstEntity(RS2::ResolveNone);
            e != NULL;
            e = container->nextEntity(RS2::ResolveNone)) {

        if (e->isVisible() && e->isSelected()) {
            double l = e->getLength();
            if (l<0.0) {
                len = -1.0;
                break;
            } else {
                len += l;
            }
        }
    }

    if (len>0.0) {
        RS_DIALOGFACTORY->commandMessage(
            tr("Total Length of selected entities: %1").arg(len));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("At least one of the selected "
                                            "entities cannot be measured."));
    }

    finish();
}



void RS_ActionInfoTotalLength::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarInfo);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
        }
    }
}


// EOF
