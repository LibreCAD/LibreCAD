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

#include "rs_actionselectlayer.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_selection.h"



RS_ActionSelectLayer::RS_ActionSelectLayer(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Select Layers", container, graphicView) {

    en = NULL;
}


QAction* RS_ActionSelectLayer::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action = new QAction(tr("(De-)Select Layer"), NULL);
        action->setIcon(QIcon(":/extui/selectlayer.png"));
    //action->zetStatusTip(tr("(De-)Selects layers"));
    return action;
}


void RS_ActionSelectLayer::trigger() {
    if (en!=NULL) {
        RS_Selection s(*container, graphicView);
        s.selectLayer(en);
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
    } else {
        RS_DEBUG->print("RS_ActionSelectLayer::trigger: Entity is NULL\n");
    }
}



void RS_ActionSelectLayer::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}



void RS_ActionSelectLayer::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
