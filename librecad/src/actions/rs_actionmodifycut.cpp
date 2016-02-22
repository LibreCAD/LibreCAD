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

#include "rs_actionmodifycut.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_debug.h"


RS_ActionModifyCut::RS_ActionModifyCut(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_ActionInterface("Cut Entity",
					container, graphicView)
,cutEntity(nullptr)
,cutCoord(new RS_Vector{})
{
	actionType=RS2::ActionModifyCut;
}

RS_ActionModifyCut::~RS_ActionModifyCut() = default;

void RS_ActionModifyCut::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionModifyCut::trigger() {

    RS_DEBUG->print("RS_ActionModifyCut::trigger()");

	if (cutEntity && cutEntity->isAtomic() && cutCoord->valid &&
			cutEntity->isPointOnEntity(*cutCoord)) {

        cutEntity->setHighlighted(false);
        graphicView->drawEntity(cutEntity);

        RS_Modification m(*container, graphicView);
		m.cut(*cutCoord, (RS_AtomicEntity*)cutEntity);

		cutEntity = nullptr;
		*cutCoord = RS_Vector(false);
        setStatus(ChooseCutEntity);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    }
}



void RS_ActionModifyCut::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyCut::mouseMoveEvent begin");

    switch (getStatus()) {
    case ChooseCutEntity:
        deleteSnapper();
        break;

    case SetCutCoord:
        snapPoint(e);
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyTrim::mouseMoveEvent end");
}


void RS_ActionModifyCut::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case ChooseCutEntity:
            cutEntity = catchEntity(e);
			if (cutEntity==nullptr) {
                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
            } else if(cutEntity->trimmable()){
                cutEntity->setHighlighted(true);
                graphicView->drawEntity(cutEntity);
                setStatus(SetCutCoord);
            }else
                RS_DIALOGFACTORY->commandMessage(
                            tr("Entity must be a line, arc, circle, ellipse or interpolation spline."));
            break;

        case SetCutCoord:
			*cutCoord = snapPoint(e);
			if (cutEntity==nullptr) {
                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
			} else if (!cutCoord->valid) {
                RS_DIALOGFACTORY->commandMessage(tr("Cutting point is invalid."));
			} else if (!cutEntity->isPointOnEntity(*cutCoord)) {
                RS_DIALOGFACTORY->commandMessage(
                    tr("Cutting point is not on entity."));
            } else {
                trigger();
                deleteSnapper();
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        if (cutEntity) {
            cutEntity->setHighlighted(false);
            graphicView->drawEntity(cutEntity);
        }
        init(getStatus()-1);
    }
}



void RS_ActionModifyCut::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseCutEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify entity to cut"),
                                            tr("Cancel"));
        break;
    case SetCutCoord:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify cutting point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionModifyCut::updateMouseCursor()
{
    switch (getStatus()) {
    case ChooseCutEntity:
        graphicView->setMouseCursor(RS2::SelectCursor);
        break;
    case SetCutCoord:
        graphicView->setMouseCursor(RS2::CadCursor);
        break;
    default:
        break;
    }
}

// EOF
