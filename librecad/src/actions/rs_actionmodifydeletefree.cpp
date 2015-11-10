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

#include "rs_actionmodifydeletefree.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyDeleteFree::Points {
	RS_Vector v1;
	RS_Vector v2;
};

RS_ActionModifyDeleteFree::RS_ActionModifyDeleteFree(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Delete Entities Freehand",
					container, graphicView)
		, pPoints(new Points{})
{
	init();
}

RS_ActionModifyDeleteFree::~RS_ActionModifyDeleteFree() = default;


void RS_ActionModifyDeleteFree::init(int status) {
    RS_ActionInterface::init(status);
	polyline = nullptr;
	e1 = e2 = nullptr;
	pPoints.reset(new Points{});
    RS_SnapMode *s = getSnapMode();
    s->snapOnEntity = true;
}



void RS_ActionModifyDeleteFree::trigger() {
    if (e1 && e2) {
        RS_EntityContainer* parent = e2->getParent();
        if (parent) {
            if (parent->rtti()==RS2::EntityPolyline) {
                if(parent->getId() == polyline->getId()) {

                    // deletes whole polyline on screen:
                    graphicView->deleteEntity((RS_Entity*)polyline);

                    // splits up the polyline in the container:
                    RS_Polyline* pl1;
                    RS_Polyline* pl2;
                    RS_Modification m(*container);
                    m.splitPolyline(*polyline,
									*e1, pPoints->v1,
									*e2, pPoints->v2,
                                    &pl1, &pl2);

                    if (document) {
                        document->startUndoCycle();
                        document->addUndoable(polyline);
                        document->addUndoable(pl1);
                        document->addUndoable(pl2);
                        document->endUndoCycle();
                    }

                    // draws the new polylines on the screen:
                                        graphicView->redraw(RS2::RedrawDrawing);

                    init();

                    RS_DIALOGFACTORY->updateSelectionWidget(
                        container->countSelected(),container->totalSelectedLength());
                } else {
                                RS_DIALOGFACTORY->commandMessage(tr("Entities not in the same polyline."));
                }
            } else {
                        RS_DIALOGFACTORY->commandMessage(tr("Parent of second entity is not a polyline"));
            }
        } else {
				RS_DIALOGFACTORY->commandMessage(tr("Parent of second entity is nullptr"));
        }
    } else {
		RS_DIALOGFACTORY->commandMessage(tr("One of the chosen entities is nullptr"));
    }
}



void RS_ActionModifyDeleteFree::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {

        switch (getStatus()) {
        case 0: {
				pPoints->v1 = snapPoint(e);
                e1 = getKeyEntity();
                if (e1) {
                    RS_EntityContainer* parent = e1->getParent();
                    if (parent) {
                        if (parent->rtti()==RS2::EntityPolyline) {
                            polyline = (RS_Polyline*)parent;
                            setStatus(1);
                        } else {
                                                RS_DIALOGFACTORY->commandMessage(
                                                                tr("Parent of first entity is not a polyline"));
                        }
                    } else {
                                        RS_DIALOGFACTORY->commandMessage(
														tr("Parent of first entity is nullptr"));
                    }
                } else {
                                RS_DIALOGFACTORY->commandMessage(
												tr("First entity is nullptr"));
                }
            }
            break;

        case 1: {
				pPoints->v2 = snapPoint(e);
                e2 = getKeyEntity();

                if (e2) {
                    trigger();
                } else {
								RS_DIALOGFACTORY->commandMessage(tr("Second entity is nullptr"));
                }
            }
            break;
        }
    }
}



void RS_ActionModifyDeleteFree::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first break point "
                                               "on a polyline"), tr("Cancel"));
        break;
    case 1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second break point "
                                               "on the same polyline"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

// EOF
