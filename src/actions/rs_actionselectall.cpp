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

#include "rs_actionselectall.h"
#include "rs_selection.h"

RS_ActionSelectAll::RS_ActionSelectAll(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView,
                                       bool select)
        :RS_ActionInterface("Select All Entities",
                    container, graphicView) {

    this->select = select;
}

QAction* RS_ActionSelectAll::createGUIAction(RS2::ActionType type, QObject* parent) {
    QAction* action;
    if (type==RS2::ActionSelectAll) {
		// tr("Select All")
		QAction* action = new QAction(tr("Select &All"), parent);
		action->setShortcut(QKeySequence::SelectAll);
		action->setStatusTip(tr("Selects all Entities"));
	} else {
		// tr("Deselect all")
		action = new QAction(tr("Deselect &all"), parent);
		action->setShortcut(QKeySequence(tr("Ctrl+K")));
		action->setStatusTip(tr("Deselects all Entities"));
		
    }
    return action;
}


void RS_ActionSelectAll::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
    finish();
}

void RS_ActionSelectAll::trigger() {
    RS_Selection s(*container, graphicView);
    s.selectAll(select);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}

// EOF
