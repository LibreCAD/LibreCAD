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

#include<QAction>
#include <QMouseEvent>
#include "rs_actiondrawlinetangent1.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace{
auto circleType={RS2::EntityArc, RS2::EntityCircle,
				 RS2::EntityEllipse, RS2::EntitySplinePoints
				};
}

RS_ActionDrawLineTangent1::RS_ActionDrawLineTangent1(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw Tangents 1", container, graphicView)
	,tangent(nullptr)
	,point(new RS_Vector{})
	,circle(nullptr)
{
	actionType=RS2::ActionDrawLineTangent1;
}

RS_ActionDrawLineTangent1::~RS_ActionDrawLineTangent1() = default;

void RS_ActionDrawLineTangent1::trigger() {
	RS_PreviewActionInterface::trigger();

	if (tangent) {
		RS_Entity* newEntity = nullptr;

		newEntity = new RS_Line(container,
								tangent->getData());

		if (newEntity) {
			if(circle){
				circle->setHighlighted(false);
				graphicView->drawEntity(circle);
			}

			newEntity->setLayerToActive();
			newEntity->setPenToActive();
			container->addEntity(newEntity);

			// upd. undo list:
			if (document) {
				document->startUndoCycle();
				document->addUndoable(newEntity);
				document->endUndoCycle();
			}

			graphicView->redraw(RS2::RedrawDrawing);

			setStatus(SetPoint);
		}
		tangent.reset();
	} else {
		RS_DEBUG->print("RS_ActionDrawLineTangent1::trigger:"
						" Entity is nullptr\n");
	}
}



void RS_ActionDrawLineTangent1::mouseMoveEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_ActionDrawLineTangent1::mouseMoveEvent begin");

	RS_Vector mouse(graphicView->toGraphX(e->x()),
					graphicView->toGraphY(e->y()));

	switch (getStatus()) {
	case SetPoint:
		*point = snapPoint(e);
		break;

	case SetCircle: {
		RS_Entity* en = catchEntity(e, circleType, RS2::ResolveAll);
		if (en && (en->isArc() ||
				   en->rtti()==RS2::EntitySplinePoints)) {
			if(circle){
				circle->setHighlighted(false);
				graphicView->drawEntity(en);
			}
			circle = en;
			circle->setHighlighted(true);
			graphicView->drawEntity(en);


			RS_Creation creation(nullptr, nullptr);
			tangent.reset(
						creation.createTangent1(mouse,
												*point,
												circle)
						);

			if (tangent) {
				deletePreview();
				preview->addEntity(tangent->clone());
				drawPreview();
			}
		}
	}
		break;

	default:
		break;
	}

	RS_DEBUG->print("RS_ActionDrawLineTangent1::mouseMoveEvent end");
}

void RS_ActionDrawLineTangent1::mouseReleaseEvent(QMouseEvent* e) {

	if (e->button()==Qt::RightButton) {
		deletePreview();
		if(circle){
			circle->setHighlighted(false);
			graphicView->drawEntity(circle);
		}
		init(getStatus()-1);
	} else {
		switch (getStatus()) {
		case SetPoint: {
			RS_CoordinateEvent ce(snapPoint(e));
			coordinateEvent(&ce);
		}
			break;

		case SetCircle:
			if(tangent){
				trigger();
			}
			break;
		}
	}
}

void RS_ActionDrawLineTangent1::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;
	switch (getStatus()) {
	case SetPoint:
		*point = e->getCoordinate();
		graphicView->moveRelativeZero(*point);
		setStatus(SetCircle);
		break;

	default:
		break;
	}
}

void RS_ActionDrawLineTangent1::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetPoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify point"),
											tr("Cancel"));
		break;
	case SetCircle:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionDrawLineTangent1::updateMouseCursor()
{
    switch (getStatus())
    {
        case SetPoint:
            graphicView->setMouseCursor(RS2::CadCursor);
            break;
        case SetCircle:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
    }
}

// EOF
