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

#include "rs_actionselect.h"

#include "rs_snapper.h"
#include "rs_actionselectsingle.h"


RS_ActionSelect::RS_ActionSelect(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView,
                                 RS2::ActionType nextAction)
        :RS_ActionInterface("Select Entities", container, graphicView) {

    this->nextAction = nextAction;
}



void RS_ActionSelect::init(int status) {
	RS_ActionInterface::init(status);
	graphicView->setCurrentAction(
		new RS_ActionSelectSingle(*container, *graphicView));
}



void RS_ActionSelect::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionSelect::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBarSelect(this, nextAction);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
        }
    }
}



// EOF
