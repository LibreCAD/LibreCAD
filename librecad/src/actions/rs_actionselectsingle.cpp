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

#include "rs_actionselectsingle.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_selection.h"
#include "rs_debug.h"



RS_ActionSelectSingle::RS_ActionSelectSingle(RS_EntityContainer& container,
											 RS_GraphicView& graphicView,
											 RS_ActionInterface* action_select,
											 std::initializer_list<RS2::EntityType> const& entityTypeList)
    :RS_ActionInterface("Select Entities", container, graphicView)
    ,entityTypeList(entityTypeList)
    ,en(nullptr)
    ,actionSelect(action_select)
{
    actionType = RS2::ActionSelectSingle;
}


void RS_ActionSelectSingle::trigger() {
	bool typeMatch{true};
	if (en && entityTypeList.size())
		typeMatch = std::find(entityTypeList.begin(), entityTypeList.end(), en->rtti())
				!= entityTypeList.end();
	if (en && typeMatch) {
        RS_Selection s(*container, graphicView);
        s.selectSingle(en);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    } else {
        RS_DEBUG->print("RS_ActionSelectSingle::trigger: Entity is NULL\n");
    }
}


void RS_ActionSelectSingle::keyPressEvent(QKeyEvent* e)
{
    if (e->key()==Qt::Key_Escape)
    {
        finish(false);
        actionSelect->keyPressEvent(e);
    }

    if (container->countSelected() > 0 && e->key()==Qt::Key_Enter)
    {
        finish(false);
        actionSelect->keyPressEvent(e);
    }
}


void RS_ActionSelectSingle::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button()==Qt::RightButton)
    {
        finish();
        if (actionSelect->rtti() == RS2::ActionSelect)
            actionSelect->finish();
        else
            actionSelect->mouseReleaseEvent(e);
    }
    else
    {
        if (entityTypeList.size()) {
            en = catchEntity(e, entityTypeList);
        }else{
            en = catchEntity(e);
        }
        trigger();
    }
}



void RS_ActionSelectSingle::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
