/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include<vector>
#include<QAction>
#include <QMouseEvent>
#include "rs_actiondrawcircletan2.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_point.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace {
auto enTypeList={RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct RS_ActionDrawCircleTan2::Points {
	RS_CircleData cData;
	RS_Vector coord;
	double radius{0.};
	bool valid{false};
	RS_VectorSolutions centers;
	std::vector<RS_AtomicEntity*> circles;
};

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan2::RS_ActionDrawCircleTan2(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw circle inscribed",
							   container, graphicView)
	, pPoints(new Points{})
{
	actionType=RS2::ActionDrawCircleTan2;
}

RS_ActionDrawCircleTan2::~RS_ActionDrawCircleTan2() = default;

void RS_ActionDrawCircleTan2::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status>=0) {
        RS_Snapper::suspend();
    }

    if (status==SetCircle1) {
        pPoints->circles.clear();
    }
}


void RS_ActionDrawCircleTan2::finish(bool updateTB){
    if(pPoints->circles.size()>0){
		for(auto p: pPoints->circles){
			if(p) p->setHighlighted(false);
        }
        graphicView->redraw(RS2::RedrawDrawing);
        pPoints->circles.clear();
    }
    RS_PreviewActionInterface::finish(updateTB);
}


void RS_ActionDrawCircleTan2::trigger() {

    RS_PreviewActionInterface::trigger();


	RS_Circle* circle=new RS_Circle(container, pPoints->cData);

    container->addEntity(circle);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(circle);
        document->endUndoCycle();
    }

	for(auto p: pPoints->circles)
		p->setHighlighted(false);
    graphicView->redraw(RS2::RedrawDrawing);
    //    drawSnapper();

    pPoints->circles.clear();
    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan2::trigger():"
                    " entity added: %d", circle->getId());
}



void RS_ActionDrawCircleTan2::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircleTan2::mouseMoveEvent begin");

    switch(getStatus() ){
    case SetCenter: {
        //        RS_Entity*  en = catchEntity(e, enTypeList, RS2::ResolveAll);
		pPoints->coord= graphicView->toGraph(e->x(), e->y());
        //        circles[getStatus()]=static_cast<RS_Line*>(en);
        if(preparePreview()) {
            deletePreview();
			RS_Circle* e=new RS_Circle(preview.get(), pPoints->cData);
            preview->addEntity(e);
			for(size_t i=0; i< pPoints->centers.size(); ++i){
				preview->addEntity(new RS_Point(preview.get(), RS_PointData(pPoints->centers.at(i))));
			}
            drawPreview();
        }
    }
        break;
    default:
        break;
    }
    RS_DEBUG->print("RS_ActionDrawCircleTan2::mouseMoveEvent end");
}

void RS_ActionDrawCircleTan2::setRadius(const double& r)
{
	pPoints->cData.radius=r;
    if(getStatus() == SetCenter){
		pPoints->centers=RS_Circle::createTan2(pPoints->circles,
			pPoints->cData.radius);
    }
}

bool RS_ActionDrawCircleTan2::getCenters(){
    if(getStatus() != SetCircle2) return false;
	pPoints->centers=RS_Circle::createTan2(pPoints->circles,
				pPoints->cData.radius);
	pPoints->valid= (pPoints->centers.size()>0);
	return pPoints->valid;
}

bool RS_ActionDrawCircleTan2::preparePreview(){
	if (pPoints->valid) {
		pPoints->cData.center=pPoints->centers.getClosest(pPoints->coord);
    }
	return pPoints->valid;
}

RS_Entity* RS_ActionDrawCircleTan2::catchCircle(QMouseEvent* e) {
    RS_Entity*  en = catchEntity(e,enTypeList, RS2::ResolveAll);
	if (!en) return nullptr;
	if (!en->isVisible()) return nullptr;
	for (int i=0;i<getStatus();i++) {
		if(en->getId() == pPoints->circles[i]->getId()) return nullptr; //do not pull in the same line again
    }
	if(en->getParent()) {
        if ( en->getParent()->ignoredOnModification()){
			return nullptr;
        }
    }
    return en;
}

void RS_ActionDrawCircleTan2::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {

        switch (getStatus()) {
        case SetCircle1:
        case SetCircle2: {
            RS_Entity*  en = catchCircle(e);
			if (!en) return;
            pPoints->circles.resize(getStatus());
            pPoints->circles.push_back(static_cast<RS_AtomicEntity*>(en));
            if(getStatus()==SetCircle1 || getCenters()){
                pPoints->circles.at(pPoints->circles.size()-1)->setHighlighted(true);
                graphicView->redraw(RS2::RedrawDrawing);
                setStatus(getStatus()+1);
            }
        }
            break;
        case SetCenter:
			pPoints->coord= graphicView->toGraph(e->x(), e->y());
            if( preparePreview()) trigger();
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        // Return to last status:
        if(getStatus()>0){
            pPoints->circles[getStatus()-1]->setHighlighted(false);
            pPoints->circles.pop_back();
            graphicView->redraw(RS2::RedrawDrawing);
            deletePreview();
        }
        init(getStatus()-1);
    }
}


//void RS_ActionDrawCircleTan2::coordinateEvent(RS_CoordinateEvent* e) {

//}

//fixme, support command line

/*
void RS_ActionDrawCircleTan2::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
			RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
											 + getAvailableCommands().join(", "));
		return;
	}

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
			if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
			} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
			if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
			if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    default:
        break;
    }
}
*/


void RS_ActionDrawCircleTan2::showOptions() {
	RS_DEBUG->print("RS_ActionDrawCircleTan2::showOptions");
	RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
	RS_DEBUG->print("RS_ActionDrawCircleTan2::showOptions: OK");
}



void RS_ActionDrawCircleTan2::hideOptions() {
	RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}


QStringList RS_ActionDrawCircleTan2::getAvailableCommands() {
	return {};
}



void RS_ActionDrawCircleTan2::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCircle1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first line/arc/circle"),
											tr("Cancel"));
		break;

	case SetCircle2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second line/arc/circle"),
											tr("Back"));
		break;

	case SetCenter:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select the center of the tangent circle"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawCircleTan2::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

double RS_ActionDrawCircleTan2::getRadius() const{
	return pPoints->cData.radius;
}

// EOF
