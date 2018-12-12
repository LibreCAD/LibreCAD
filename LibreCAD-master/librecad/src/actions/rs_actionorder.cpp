/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2012 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actionorder.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_debug.h"



RS_ActionOrder::RS_ActionOrder(RS_EntityContainer& container,
        RS_GraphicView& graphicView, RS2::ActionType type)
        :RS_PreviewActionInterface("Sort Entities",
						   container, graphicView)
		,targetEntity(nullptr)
		,orderType(type)
{
	actionType=RS2::ActionOrderBottom;
}

void RS_ActionOrder::init(int status) {
    RS_ActionInterface::init(status);
	targetEntity = nullptr;
    if (orderType == RS2::ActionOrderBottom ||
            orderType == RS2::ActionOrderTop) {
        trigger();
    } else
        snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionOrder::trigger() {
    RS_DEBUG->print("RS_ActionOrder::trigger()");

    QList<RS_Entity *> entList;
	for(auto e: *container){
        if (e->isSelected())
            entList.append(e);
    }

    if (targetEntity) {
		int index = -1;
		targetEntity->setHighlighted(false);
        graphicView->drawEntity(targetEntity);

        switch (orderType) {
        case RS2::ActionOrderLower:
            index = document->findEntity(targetEntity);
            document->moveEntity(index, entList);
            break;
        case RS2::ActionOrderRaise:
            index = document->findEntity(targetEntity)+1;
            document->moveEntity(index, entList);
            break;
        default:
            break;
        }
		targetEntity = nullptr;
    } else {
        switch (orderType) {
        case RS2::ActionOrderBottom:
            document->moveEntity(-1, entList);
            break;
        case RS2::ActionOrderTop:
            document->moveEntity(document->count()+1, entList);
            break;
        default:
            break;
        }
    }
    setStatus(getStatus()-1);
}



void RS_ActionOrder::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionOrder::mouseMoveEvent begin");

    switch (getStatus()) {
    case ChooseEntity:
        snapFree(e);
        break;
    default:
        break;
    }

    RS_DEBUG->print("RS_ActionOrder::mouseMoveEvent end");
}



void RS_ActionOrder::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case ChooseEntity:
            targetEntity = catchEntity(e);
			if (!targetEntity) {
                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
            } else {
                targetEntity->setHighlighted(true);
                graphicView->drawEntity(targetEntity);
                graphicView->redraw();
                trigger();
            }
            break;
        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deleteSnapper();
        if (targetEntity) {
            targetEntity->setHighlighted(false);
            graphicView->drawEntity(targetEntity);
                graphicView->redraw();
        }
        init(getStatus()-1);
    }
    deleteSnapper();
}

void RS_ActionOrder::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Choose entity for order"),
                                            tr("Cancel"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionOrder::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


// EOF
