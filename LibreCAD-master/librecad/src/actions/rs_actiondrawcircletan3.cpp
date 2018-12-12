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

#include "rs_actiondrawcircletan3.h"
#include <QAction>
#include <QMouseEvent>

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "lc_quadratic.h"
#include "rs_information.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace {
auto enTypeList={RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine, RS2::EntityPoint};
}

struct RS_ActionDrawCircleTan3::Points {
		std::vector<RS_AtomicEntity*> circles;
		std::shared_ptr<RS_CircleData> cData{std::make_shared<RS_CircleData>()};
		RS_Vector coord;
		bool valid{false};
		//keep a list of centers found
		std::vector<std::shared_ptr<RS_CircleData> > candidates;
		RS_VectorSolutions centers;
};

RS_ActionDrawCircleTan3::~RS_ActionDrawCircleTan3() = default;


/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan3::RS_ActionDrawCircleTan3(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw circle inscribed",
							   container, graphicView)
	, pPoints(new Points{})
{
	actionType=RS2::ActionDrawCircleTan3;
}

void RS_ActionDrawCircleTan3::init(int status) {
	RS_PreviewActionInterface::init(status);
	if(status>=0) {
		RS_Snapper::suspend();
	}

	if (status==SetCircle1) {
		pPoints->circles.clear();
	}
}


void RS_ActionDrawCircleTan3::finish(bool updateTB){
	if(pPoints->circles.size()>0){
		for(RS_AtomicEntity* const pc: pPoints->circles)
			if(pc) pc->setHighlighted(false);

		graphicView->redraw(RS2::RedrawDrawing);
		pPoints->circles.clear();
	}
	RS_PreviewActionInterface::finish(updateTB);
}


void RS_ActionDrawCircleTan3::trigger() {

	RS_PreviewActionInterface::trigger();


	RS_Circle* circle=new RS_Circle(container, *pPoints->cData);

	container->addEntity(circle);

	// upd. undo list:
	if (document) {
		document->startUndoCycle();
		document->addUndoable(circle);
		document->endUndoCycle();
	}

	for(RS_AtomicEntity* const pc: pPoints->circles)
		if(pc) pc->setHighlighted(false);
	graphicView->redraw(RS2::RedrawDrawing);
	//    drawSnapper();

	pPoints->circles.clear();
	setStatus(SetCircle1);

	RS_DEBUG->print("RS_ActionDrawCircleTan3::trigger():"
					" entity added: %d", circle->getId());
}



void RS_ActionDrawCircleTan3::mouseMoveEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_ActionDrawCircleTan3::mouseMoveEvent begin");

	switch(getStatus() ){
	case SetCenter: {
		//        RS_Entity*  en = catchEntity(e, enTypeList, RS2::ResolveAll);
		pPoints->coord= graphicView->toGraph(e->x(), e->y());
		//        circles[getStatus()]=static_cast<RS_Line*>(en);
		deletePreview();
		if(preparePreview()) {
			RS_Circle* e=new RS_Circle(preview.get(), *pPoints->cData);
			preview->addEntity(e);
			for(auto& c: pPoints->candidates){
                preview->addEntity(new RS_Point(nullptr, RS_PointData(c->center)));
			}
			drawPreview();
		}
	}
		break;
	default:
		break;
	}
	RS_DEBUG->print("RS_ActionDrawCircleTan3::mouseMoveEvent end");
}


bool RS_ActionDrawCircleTan3::getData(){
	if(getStatus() != SetCircle3) return false;
	//find the nearest circle
	size_t i=0;
	size_t const countLines=std::count_if(pPoints->circles.begin(), pPoints->circles.end(), [](RS_AtomicEntity* e)->bool
	{
			return e->rtti()==RS2::EntityLine;
});

	for(;i<pPoints->circles.size();++i)
		if(pPoints->circles[i]->rtti() == RS2::EntityLine) break;
	pPoints->candidates.clear();
	size_t i1=(i+1)%3;
	size_t i2=(i+2)%3;
	if(i<pPoints->circles.size() && pPoints->circles[i]->rtti() == RS2::EntityLine){
		//one or more lines

		LC_Quadratic lc0(pPoints->circles[i],pPoints->circles[i1],false);
		LC_Quadratic lc1;
		RS_VectorSolutions sol;
		//detect degenerate case two circles with the same radius
		switch(countLines){
		default:
		case 0:
			//this should not happen
			assert(false);
		case 1:
			//1 line, two circles
		{
			for(unsigned k=0; k<4; ++k){
				//loop through all mirroring cases
				lc1=LC_Quadratic(pPoints->circles[i],pPoints->circles[i1], k & 1u);
				LC_Quadratic lc2=LC_Quadratic(pPoints->circles[i],pPoints->circles[i2], k & 2u);
				sol.push_back(LC_Quadratic::getIntersection(lc1,lc2));
			}

		}
			break;
		case 2:
			//2 lines, one circle
		{
			if(pPoints->circles[i2]->rtti()==RS2::EntityLine){
				std::swap(i1, i2);
			}
			//i2 is circle

			for(unsigned k=0; k<4; ++k){
				//loop through all mirroring cases
				lc1=LC_Quadratic(pPoints->circles[i2],pPoints->circles[i], k & 1u);
				LC_Quadratic lc2=LC_Quadratic(pPoints->circles[i2],pPoints->circles[i1], k & 2u);
				sol.push_back(LC_Quadratic::getIntersection(lc1,lc2));
			}
		}
			break;
		case 3:
			//3 lines
		{
			lc0=pPoints->circles[i]->getQuadratic();
			lc1=pPoints->circles[i1]->getQuadratic();
			auto lc2=pPoints->circles[i2]->getQuadratic();
			//attempt to have intersections (lc0, lc1), (lc0, lc2)
			auto sol1=LC_Quadratic::getIntersection(lc0,lc1);
			if(sol1.size()<1) {
				std::swap(lc0, lc2);
				std::swap(i, i2);
			}
			sol1=LC_Quadratic::getIntersection(lc0,lc2);
			if(sol1.size()<1) {
				std::swap(lc0, lc1);
				std::swap(i, i1);
			}

			RS_Line* line0=static_cast<RS_Line*>(pPoints->circles[i]);
			RS_Line* line1=static_cast<RS_Line*>(pPoints->circles[i1]);
			RS_Line* line2=static_cast<RS_Line*>(pPoints->circles[i2]);
			lc0=line0->getQuadratic();
			lc1=line1->getQuadratic();
			lc2=line2->getQuadratic();
			//intersection 0, 1
			sol1=LC_Quadratic::getIntersection(lc0,lc1);
			if(!sol1.size()) {
				return false;
			}
			RS_Vector const v1=sol1.at(0);
			double angle1=0.5*(line0->getAngle1()+line1->getAngle1());

			//intersection 0, 2
			sol1=LC_Quadratic::getIntersection(lc0,lc2);
			double angle2;
			if(sol1.size()<1) {
				return false;
			}
			angle2=0.5*(line0->getAngle1()+line2->getAngle1());
			RS_Vector const& v2=sol1.at(0);
			//two bisector lines per intersection
			for(unsigned j=0; j<2; ++j){
				RS_Line l1{v1, v1+RS_Vector{angle1}};
				for(unsigned j1=0; j1<2; ++j1){
					RS_Line l2{v2, v2+RS_Vector{angle2}};
					sol.push_back(RS_Information::getIntersectionLineLine(&l1, &l2));
					angle2 += M_PI_2;
				}
				angle1 += M_PI_2;
			}
		}
		}

		double d;

		//line passes circle center, need a second parabola as the image of the line
		for(int j=1;j<=2;j++){
			if(pPoints->circles[(i+j)%3]->rtti() == RS2::EntityCircle){
				pPoints->circles[i]->getNearestPointOnEntity(pPoints->circles[(i+j)%3]->getCenter(),
						false,&d);
				if(d<RS_TOLERANCE) {
					LC_Quadratic lc2(pPoints->circles[i],pPoints->circles[(i+j)%3], true);
					sol.push_back(LC_Quadratic::getIntersection(lc2,lc1));
				}
			}
		}

		//clean up duplicate and invalid
		RS_VectorSolutions sol1;
		for(const RS_Vector& vp: sol){
			if(vp.magnitude()>RS_MAXDOUBLE) continue;
			if(sol1.size() && sol1.getClosestDistance(vp)<RS_TOLERANCE) continue;
			sol1.push_back(vp);
		}

		for(auto const& v: sol1){
			pPoints->circles[i]->getNearestPointOnEntity(v,false,&d);
			auto data = std::make_shared<RS_CircleData>(v,d);
			if(pPoints->circles[(i+1)%3]->isTangent(*data)==false) continue;
			if(pPoints->circles[(i+2)%3]->isTangent(*data)==false) continue;
			pPoints->candidates.push_back(data);
		}

	} else {
		RS_Circle c{nullptr, *pPoints->cData};
		auto solutions=c.createTan3(pPoints->circles);
		pPoints->candidates.clear();
		for(const RS_Circle& s: solutions){
			pPoints->candidates.push_back(std::make_shared<RS_CircleData>(s.getData()));
		}
	}
	pPoints->valid = ( pPoints->candidates.size() >0);
	return pPoints->valid;
}

bool RS_ActionDrawCircleTan3::preparePreview(){
	if(getStatus() != SetCenter || pPoints->valid==false) {
		pPoints->valid=false;
		return false;
	}
	//find the nearest circle
	size_t index=pPoints->candidates.size();
	double dist=RS_MAXDOUBLE*RS_MAXDOUBLE;
	for(size_t i=0;i<pPoints->candidates.size();++i){

		preview->addEntity(new RS_Point(preview.get(), RS_PointData(pPoints->candidates.at(i)->center)));
		double d;
		RS_Circle(nullptr, *pPoints->candidates.at(i)).getNearestPointOnEntity(pPoints->coord,false,&d);
		double dCenter=pPoints->coord.distanceTo(pPoints->candidates.at(i)->center);
		d=std::min(d,dCenter);
		if(d<dist){
			dist=d;
			index=i;
		}
	}
	if( index<pPoints->candidates.size()){
		pPoints->cData=pPoints->candidates.at(index);
		pPoints->valid=true;
	}else{
		pPoints->valid=false;
	}
	return pPoints->valid;
}

RS_Entity* RS_ActionDrawCircleTan3::catchCircle(QMouseEvent* e) {
    RS_Entity* ret=nullptr;
	RS_Entity*  en = catchEntity(e,enTypeList, RS2::ResolveAll);
	if (!en) return ret;
	if (!en->isVisible()) return ret;
	for(int i=0;i<getStatus();++i) {
		if(en->getId() == pPoints->circles[i]->getId()) return ret; //do not pull in the same line again
	}
	if(en->getParent()) {
		if ( en->getParent()->ignoredOnModification()){
            return nullptr;
		}
	}
	return en;
}

void RS_ActionDrawCircleTan3::mouseReleaseEvent(QMouseEvent* e) {
	// Proceed to next status
	if (e->button()==Qt::LeftButton) {

		switch (getStatus()) {
		case SetCircle1:
		case SetCircle2:
		case SetCircle3: {
			RS_Entity*  en = catchCircle(e);
			if (!en) return;
			pPoints->circles.resize(getStatus());
			for(const RS_AtomicEntity* const pc: pPoints->circles)
				if(pc == en) continue;
			pPoints->circles.push_back(static_cast<RS_AtomicEntity*>(en));
			if(getStatus()<=SetCircle2 || (getStatus()==SetCircle3 && getData())){
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


//void RS_ActionDrawCircleTan3::coordinateEvent(RS_CoordinateEvent* e) {

//}

//fixme, support command line

/*
void RS_ActionDrawCircleTan3::commandEvent(RS_CommandEvent* e) {
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

QStringList RS_ActionDrawCircleTan3::getAvailableCommands() {
	QStringList cmd;
	return cmd;
}

void RS_ActionDrawCircleTan3::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCircle1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first line/arc/circle"),
											tr("Cancel"));
		break;

	case SetCircle2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second line/arc/circle"),
											tr("Back"));
		break;
	case SetCircle3:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the third line/arc/circle"),
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

void RS_ActionDrawCircleTan3::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
