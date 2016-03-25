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

#include "rs_actionmodifydelete.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_debug.h"



RS_ActionModifyDelete::RS_ActionModifyDelete(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Delete Entities",
					container, graphicView) {
	actionType=RS2::ActionModifyDelete;
}


void RS_ActionModifyDelete::init(int status) {
    RS_ActionInterface::init(status);

    trigger();
}



void RS_ActionModifyDelete::trigger() {

    RS_DEBUG->print("RS_ActionModifyDelete::trigger()");

    RS_Modification m(*container, graphicView);
    m.remove();

    finish(false);

	RS_DIALOGFACTORY->updateSelectionWidget(
				container->countSelected(), container->totalSelectedLength());
}



void RS_ActionModifyDelete::updateMouseButtonHints() {
	switch (getStatus()) {
	//case Acknowledge:
	//    RS_DIALOGFACTORY->updateMouseWidget(tr("Acknowledge"),
	//	tr("Cancel"));
	//    break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionModifyDelete::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::DelCursor);
}

// EOF
