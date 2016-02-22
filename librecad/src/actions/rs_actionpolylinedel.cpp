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

#include "rs_actionpolylinedel.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionPolylineDel::RS_ActionPolylineDel(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Delete node",
						   container, graphicView)
		, delPoint(new RS_Vector{})
{
	actionType=RS2::ActionPolylineDel;
}

RS_ActionPolylineDel::~RS_ActionPolylineDel() = default;

void RS_ActionPolylineDel::init(int status) {
    RS_ActionInterface::init(status);
	delEntity = nullptr;
	*delPoint = {};
}



void RS_ActionPolylineDel::trigger() {

    RS_DEBUG->print("RS_ActionPolylineDel::trigger()");

		if (delEntity && delPoint->valid &&
			delEntity->isPointOnEntity(*delPoint)) {

                delEntity->setHighlighted(false);
                graphicView->drawEntity(delEntity);

                RS_Modification m(*container, graphicView);
				delEntity = m.deletePolylineNode((RS_Polyline&)*delEntity, *delPoint );

				*delPoint = RS_Vector(false);

            RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    }
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineDel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionPolylineDel::mouseMoveEvent begin");


    switch (getStatus()) {
    case ChooseEntity:
                break;

    case SetDelPoint:
                snapPoint(e);
                break;

    default:
                break;
    }

    RS_DEBUG->print("RS_ActionPolylineDel::mouseMoveEvent end");
}



void RS_ActionPolylineDel::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
                switch (getStatus()) {
                case ChooseEntity:
                    delEntity = catchEntity(e);
					if (delEntity==nullptr) {
                        RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                    } else if (delEntity->rtti()!=RS2::EntityPolyline) {

                        RS_DIALOGFACTORY->commandMessage(
                            tr("Entity must be a polyline."));
                    } else {
							snapPoint(e);
                                delEntity->setHighlighted(true);
                                graphicView->drawEntity(delEntity);
                                setStatus(SetDelPoint);
////////////////////////////////////////2006/06/15
                                graphicView->redraw();
////////////////////////////////////////
                    }
                    break;

                case SetDelPoint:
					*delPoint = snapPoint(e);
					if (delEntity==nullptr) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
					} else if (!delPoint->valid) {
                                RS_DIALOGFACTORY->commandMessage(tr("Deleting point is invalid."));
					} else if (!delEntity->isPointOnEntity(*delPoint)) {
                                RS_DIALOGFACTORY->commandMessage(
                                    tr("Deleting point is not on entity."));
                    } else {
                                deleteSnapper();
                                trigger();
                    }
                    break;

                default:
                    break;
                }
    } else if (e->button()==Qt::RightButton) {
                deleteSnapper();
                if (delEntity) {
                delEntity->setHighlighted(false);
                graphicView->drawEntity(delEntity);
////////////////////////////////////////2006/06/15
                        graphicView->redraw();
////////////////////////////////////////
                }
                init(getStatus()-1);
    }
}

void RS_ActionPolylineDel::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseEntity:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify polyline to delete node"),
                                            tr("Cancel"));
                break;
    case SetDelPoint:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify deleting node's point"),
                                            tr("Back"));
                break;
    default:
                RS_DIALOGFACTORY->updateMouseWidget();
                break;
    }
}



void RS_ActionPolylineDel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
