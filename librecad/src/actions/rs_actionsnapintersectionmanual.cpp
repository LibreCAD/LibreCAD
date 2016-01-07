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

#include <QAction>
#include <QMouseEvent>
#include "rs_actionsnapintersectionmanual.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

/**
 * @param both Trim both entities.
 */
RS_ActionSnapIntersectionManual::RS_ActionSnapIntersectionManual(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Trim Entity",
							   container, graphicView)
	,entity1(nullptr)
	,entity2(nullptr)
	,coord(new RS_Vector{})
{
}

RS_ActionSnapIntersectionManual::~RS_ActionSnapIntersectionManual()=default;

QAction* RS_ActionSnapIntersectionManual::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	//tr("Intersection Manually")
    QAction* action = new QAction(tr("I&ntersection Manually"), NULL);
    //action->zetStatusTip(tr("Snap to intersection points manually"));
	action->setIcon(QIcon(":/extui/snapintersectionm.png"));
    return action;
}


void RS_ActionSnapIntersectionManual::init(int status) {
    RS_ActionInterface::init(status);
	snapMode.clear();
}



void RS_ActionSnapIntersectionManual::trigger() {

    RS_DEBUG->print("RS_ActionSnapIntersectionManual::trigger()");

    if (entity2 && entity2->isAtomic() &&
            entity1 && entity1->isAtomic()) {

        RS_VectorSolutions sol =
            RS_Information::getIntersection(entity1, entity2, false);

        entity2 = NULL;
        entity1 = NULL;
        if (predecessor) {
			RS_Vector ip = sol.getClosest(*coord);

            if (ip.valid) {
                RS_CoordinateEvent e(ip);
                predecessor->coordinateEvent(&e);
            }
        }
        finish(false);
    }
}



void RS_ActionSnapIntersectionManual::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionSnapIntersectionManual::mouseMoveEvent begin");

    RS_Entity* se = catchEntity(e);
    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());

    switch (getStatus()) {
    case ChooseEntity1:
        entity1 = se;
        break;

    case ChooseEntity2: {
            entity2 = se;
			*coord = mouse;

            RS_VectorSolutions sol =
                RS_Information::getIntersection(entity1, entity2, false);

            //for (int i=0; i<sol.getNumber(); i++) {
            //    ip = sol.get(i);
            //    break;
            //}

			RS_Vector ip = sol.getClosest(*coord);

            if (ip.valid) {
                deletePreview();
                preview->addEntity(
					new RS_Circle(preview.get(),
				{ip, graphicView->toGraphDX(4)}));
                drawPreview();

                RS_DIALOGFACTORY->updateCoordinateWidget(ip,
                        ip - graphicView->getRelativeZero());

            }
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionSnapIntersectionManual::mouseMoveEvent end");
}



void RS_ActionSnapIntersectionManual::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {

        RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
        RS_Entity* se = catchEntity(e);

        switch (getStatus()) {
        case ChooseEntity1:
            entity1 = se;
            if (entity1 && entity1->isAtomic()) {
                setStatus(ChooseEntity2);
            }
            break;

        case ChooseEntity2:
            entity2 = se;
			*coord = mouse;
			if (entity2 && entity2->isAtomic() && coord->valid) {
                trigger();
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionSnapIntersectionManual::updateMouseButtonHints() {
    switch (getStatus()) {
    case ChooseEntity1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first entity"),
                                            tr("Back"));
        break;
    case ChooseEntity2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second entity"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionSnapIntersectionManual::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
