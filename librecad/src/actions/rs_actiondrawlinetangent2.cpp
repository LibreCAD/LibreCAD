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
#include "rs_actiondrawlinetangent2.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace{
auto circleType={RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse};
}

RS_ActionDrawLineTangent2::RS_ActionDrawLineTangent2(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw Tangents 2", container, graphicView)
	,circle1(nullptr)
	,circle2(nullptr)
	,valid(false)
{
	actionType=RS2::ActionDrawLineTangent2;
    setStatus(SetCircle1);
}

RS_ActionDrawLineTangent2::~RS_ActionDrawLineTangent2() = default;


void RS_ActionDrawLineTangent2::finish(bool updateTB){
    if(circle1){
        circle1->setHighlighted(false);
		graphicView->drawEntity(circle1);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineTangent2::trigger() {
    RS_PreviewActionInterface::trigger();

	RS_Entity* newEntity = new RS_Line(container, *lineData);

    if (newEntity) {
        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        container->addEntity(newEntity);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(newEntity);
            document->endUndoCycle();
		}
		clearHighlighted();

        setStatus(SetCircle1);
    }
    tangent.reset();
}

void RS_ActionDrawLineTangent2::clearHighlighted()
{
	for(RS_Entity** p: {&circle1, &circle2}){
		if(*p){
			(*p)->setHighlighted(false);
			graphicView->drawEntity(*p);
			*p=nullptr;
		}
	}
}

void RS_ActionDrawLineTangent2::mouseMoveEvent(QMouseEvent* e) {
//    RS_DEBUG->print("RS_ActionDrawLineTangent2::mouseMoveEvent begin");
	e->accept();
    if(getStatus() != SetCircle2) return;
	RS_Entity* en= catchEntity(e, circleType, RS2::ResolveAll);
	if(!en || en==circle1) return;
	if(circle2){
		circle2->setHighlighted(false);
		graphicView->drawEntity(circle2);
	}
	circle2=en;
	circle2->setHighlighted(true);
	graphicView->drawEntity(circle2);
	RS_Creation creation(nullptr, nullptr);
    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));
    tangent.reset(creation.createTangent2(mouse,
                                          circle1,
                                          circle2));
	if(!tangent.get()){
        valid=false;
        return;
    }
    valid=true;
	lineData.reset(new RS_LineData(tangent->getData()));

    deletePreview();
	preview->addEntity(new RS_Line(preview.get(), *lineData));
    drawPreview();
}

void RS_ActionDrawLineTangent2::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        deletePreview();
		init(getStatus()-1);
		if(getStatus()>=0){
			clearHighlighted();
        }
        return;
    }
    switch (getStatus()) {
    case SetCircle1:
    {
        circle1 = catchEntity(e, circleType, RS2::ResolveAll);
		if(!circle1) return;
        circle1->setHighlighted(true);
		graphicView->drawEntity(circle1);
        setStatus(getStatus()+1);
    }
        break;

    case SetCircle2:
        if(valid) trigger();
        break;
    }
}

void RS_ActionDrawLineTangent2::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCircle1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select first circle or ellipse"),
											tr("Cancel"));
		break;
	case SetCircle2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select second circle or ellipse"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionDrawLineTangent2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
