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

#include <QAction>
#include "rs_actiondrawcircletan3.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "lc_quadratic.h"
#include "rs_information.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan3::RS_ActionDrawCircleTan3(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw circle inscribed",
							   container, graphicView),
	  cData(new RS_CircleData(RS_Vector(0.,0.),1.))
{
	actionType=RS2::ActionDrawCircleTan3;
}

QAction* RS_ActionDrawCircleTan3::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	QAction* action;

	action = new QAction(tr("Tangential &3 Circles"), NULL);
	action->setIcon(QIcon(":/extui/circletan3.png"));
	return action;
}

void RS_ActionDrawCircleTan3::init(int status) {
	RS_PreviewActionInterface::init(status);
	if(status>=0) {
		RS_Snapper::suspend();
	}

	if (status==SetCircle1) {
		circles.clear();
	}
}


void RS_ActionDrawCircleTan3::finish(bool updateTB){
	if(circles.size()>0){
		for(RS_AtomicEntity* const pc: circles)
			if(pc) pc->setHighlighted(false);

		graphicView->redraw(RS2::RedrawDrawing);
		circles.clear();
	}
	RS_PreviewActionInterface::finish(updateTB);
}


void RS_ActionDrawCircleTan3::trigger() {

	RS_PreviewActionInterface::trigger();


	RS_Circle* circle=new RS_Circle(container, *cData);

	container->addEntity(circle);

	// upd. undo list:
	if (document) {
		document->startUndoCycle();
		document->addUndoable(circle);
		document->endUndoCycle();
	}

	for(RS_AtomicEntity* const pc: circles)
		if(pc) pc->setHighlighted(false);
	graphicView->redraw(RS2::RedrawDrawing);
	//    drawSnapper();

	circles.clear();
	setStatus(SetCircle1);

	RS_DEBUG->print("RS_ActionDrawCircleTan3::trigger():"
					" entity added: %d", circle->getId());
}



void RS_ActionDrawCircleTan3::mouseMoveEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_ActionDrawCircleTan3::mouseMoveEvent begin");

	switch(getStatus() ){
	case SetCenter: {
		//        RS_Entity*  en = catchEntity(e, enTypeList, RS2::ResolveAll);
		coord= graphicView->toGraph(e->x(), e->y());
		//        circles[getStatus()]=static_cast<RS_Line*>(en);
		deletePreview();
		if(preparePreview()) {
			RS_Circle* e=new RS_Circle(preview.get(), *cData);
			preview->addEntity(e);
			for(auto& c: candidates){
				preview->addEntity(new RS_Point(NULL, RS_PointData(c->center)));
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
	size_t const countLines=std::count_if(circles.begin(), circles.end(), [](RS_AtomicEntity* e)->bool
	{
			return e->rtti()==RS2::EntityLine;
});

	for(;i<circles.size();++i)
		if(circles[i]->rtti() == RS2::EntityLine) break;
	candidates.clear();
	size_t i1=(i+1)%3;
	size_t i2=(i+2)%3;
	if(i<circles.size() && circles[i]->rtti() == RS2::EntityLine){
		//one or more lines

		LC_Quadratic lc0(circles[i],circles[i1],false);
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
				lc1=LC_Quadratic(circles[i],circles[i1], k & 1u);
				LC_Quadratic lc2=LC_Quadratic(circles[i],circles[i2], k & 2u);
				sol.appendTo(LC_Quadratic::getIntersection(lc1,lc2));
			}

		}
			break;
		case 2:
			//2 lines, one circle
		{
			if(circles[i2]->rtti()==RS2::EntityLine){
				std::swap(i1, i2);
			}
			//i2 is circle

			for(unsigned k=0; k<4; ++k){
				//loop through all mirroring cases
				lc1=LC_Quadratic(circles[i2],circles[i], k & 1u);
				LC_Quadratic lc2=LC_Quadratic(circles[i2],circles[i1], k & 2u);
				sol.appendTo(LC_Quadratic::getIntersection(lc1,lc2));
			}
		}
			break;
		case 3:
			//3 lines
		{
			lc0=circles[i]->getQuadratic();
			lc1=circles[i1]->getQuadratic();
			auto lc2=circles[i2]->getQuadratic();
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

			RS_Line* line0=static_cast<RS_Line*>(circles[i]);
			RS_Line* line1=static_cast<RS_Line*>(circles[i1]);
			RS_Line* line2=static_cast<RS_Line*>(circles[i2]);
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

				RS_Line l1(NULL, RS_LineData(v1, v1+RS_Vector(angle1)));
				for(unsigned j1=0; j1<2; ++j1){
					RS_Line l2(NULL, RS_LineData(v2, v2+RS_Vector(angle2)));
					sol.appendTo(RS_Information::getIntersectionLineLine(&l1, &l2));
					angle2 += M_PI_2;
				}
				angle1 += M_PI_2;
			}
		}
		}

		double d;

		//line passes circle center, need a second parabola as the image of the line
		for(int j=1;j<=2;j++){
			if(circles[(i+j)%3]->rtti() == RS2::EntityCircle){
				circles[i]->getNearestPointOnEntity(circles[(i+j)%3]->getCenter(),
						false,&d);
				if(d<RS_TOLERANCE) {
					LC_Quadratic lc2(circles[i],circles[(i+j)%3], true);
					sol.appendTo(LC_Quadratic::getIntersection(lc2,lc1));
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
			circles[i]->getNearestPointOnEntity(v,false,&d);
			std::shared_ptr<RS_CircleData> data(new RS_CircleData(v,d));
			if(circles[(i+1)%3]->isTangent(*data)==false) continue;
			if(circles[(i+2)%3]->isTangent(*data)==false) continue;
			candidates.push_back(data);
		}

	}else{
		RS_Circle c(NULL,*cData);
		auto&& solutions=c.createTan3(circles);
		candidates.clear();
		for(const RS_Circle& s: solutions){
			candidates.push_back(std::make_shared<RS_CircleData>(s.getData()));
		}
	}
	valid = ( candidates.size() >0);
	return valid;
}

bool RS_ActionDrawCircleTan3::preparePreview(){
	if(getStatus() != SetCenter || valid==false) {
		valid=false;
		return false;
	}
	//find the nearest circle
	size_t index=candidates.size();
	double dist=RS_MAXDOUBLE*RS_MAXDOUBLE;
	for(size_t i=0;i<candidates.size();++i){

		preview->addEntity(new RS_Point(preview.get(), RS_PointData(candidates.at(i)->center)));
		double d;
		RS_Circle(nullptr, *candidates.at(i)).getNearestPointOnEntity(coord,false,&d);
		double dCenter=coord.distanceTo(candidates.at(i)->center);
		d=std::min(d,dCenter);
		if(d<dist){
			dist=d;
			index=i;
		}
	}
	if( index<candidates.size()){
		cData=candidates.at(index);
		valid=true;
	}else{
		valid=false;
	}
	return valid;
}

RS_Entity* RS_ActionDrawCircleTan3::catchCircle(QMouseEvent* e) {
	RS_Entity* ret=NULL;
	RS_Entity*  en = catchEntity(e,enTypeList, RS2::ResolveAll);
	if(en == NULL) return ret;
	if(en->isVisible()==false) return ret;
	for(int i=0;i<getStatus();++i) {
		if(en->getId() == circles[i]->getId()) return ret; //do not pull in the same line again
	}
	if(en->getParent()) {
		if ( en->getParent()->ignoredOnModification()){
			return NULL;
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
			if (en==NULL) return;
			circles.resize(getStatus());
			for(const RS_AtomicEntity* const pc: circles)
				if(pc == en) continue;
			circles.push_back(static_cast<RS_AtomicEntity*>(en));
			if(getStatus()<=SetCircle2 || (getStatus()==SetCircle3 && getData())){
				circles.at(circles.size()-1)->setHighlighted(true);
				graphicView->redraw(RS2::RedrawDrawing);
				setStatus(getStatus()+1);
			}
		}
			break;
		case SetCenter:
			coord= graphicView->toGraph(e->x(), e->y());
			if( preparePreview()) trigger();
			break;

		default:
			break;
		}
	} else if (e->button()==Qt::RightButton) {
		// Return to last status:
		if(getStatus()>0){
			circles[getStatus()-1]->setHighlighted(false);
			circles.pop_back();
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
		if (RS_DIALOGFACTORY) {
			RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
											 + getAvailableCommands().join(", "));
		}
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
			} else {
				if (RS_DIALOGFACTORY) {
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
				}
			}
		}
		break;

	case SetAngle1: {
			bool ok;
			double a = RS_Math::eval(c, &ok);
			if (ok) {
				angle1 = RS_Math::deg2rad(a);
				setStatus(SetAngle2);
			} else {
				if (RS_DIALOGFACTORY) {
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
				}
			}
		}
		break;

	case SetAngle2: {
			bool ok;
			double a = RS_Math::eval(c, &ok);
			if (ok) {
				angle2 = RS_Math::deg2rad(a);
				trigger();
			} else {
				if (RS_DIALOGFACTORY) {
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
				}
			}
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
	if (RS_DIALOGFACTORY) {
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
}

void RS_ActionDrawCircleTan3::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
