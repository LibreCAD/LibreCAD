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
#include <QMouseEvent>
#include "rs_actiondrawcircletan1_2p.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_point.h"
#include "lc_quadratic.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace{
//list of entity types supported by current action
auto enTypeList={RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct RS_ActionDrawCircleTan1_2P::Points {
	std::vector<RS_Vector> points;
	RS_CircleData cData;
	RS_Vector coord;
	double radius = 0.;
	bool valid = false;
	//keep a list of centers found
	RS_VectorSolutions centers;
};

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan1_2P::RS_ActionDrawCircleTan1_2P(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw tangent circle 2P",
                               container, graphicView)
	,circle(nullptr)
	,pPoints(new Points{})
{
	actionType = RS2::ActionDrawCircleTan1_2P;
}

RS_ActionDrawCircleTan1_2P::~RS_ActionDrawCircleTan1_2P() = default;

void RS_ActionDrawCircleTan1_2P::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status>=0) {
        RS_Snapper::suspend();
    }

    if (status<=SetCircle1) {
        if(circle) {
            if(circle->isHighlighted()){
                circle->setHighlighted(false);
                graphicView->redraw(RS2::RedrawDrawing);
            }
        }
		pPoints->points.clear();
    }
}


void RS_ActionDrawCircleTan1_2P::finish(bool updateTB){
	if (circle) {
        circle->setHighlighted(false);
        graphicView->redraw(RS2::RedrawDrawing);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan1_2P::trigger() {
    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"begin"<<std::endl;

    RS_PreviewActionInterface::trigger();


	RS_Circle* c=new RS_Circle(container, pPoints->cData);

    container->addEntity(c);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(c);
        document->endUndoCycle();
    }


    circle->setHighlighted(false);
    graphicView->redraw(RS2::RedrawDrawing);
    //    drawSnapper();

    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::trigger():"
                    " entity added: %d", c->getId());
}



void RS_ActionDrawCircleTan1_2P::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::mouseMoveEvent begin");

    switch(getStatus() ){
    case SetPoint1:
    {
        RS_Vector&& mouse=snapPoint(e);
		pPoints->points.clear();
		pPoints->points.push_back(mouse);
        switch(circle->rtti()){
        case RS2::EntityArc:
        case RS2::EntityCircle:
        {
			RS_Vector const& dvp=mouse - circle->getCenter();
			double rvp=dvp.magnitude();
            if(rvp<RS_TOLERANCE2) break;
			pPoints->cData.radius=(circle->getRadius()+rvp)*0.5;
			pPoints->cData.center=circle->getCenter()+dvp*(pPoints->cData.radius/rvp);
			pPoints->cData.radius=fabs(circle->getRadius()-pPoints->cData.radius);
        }
            break;
        case RS2::EntityLine:
        {
            RS_Line* line=static_cast<RS_Line*>(circle);
			RS_Vector&& vp=line->getNearestPointOnEntity(pPoints->points[0],false);
            if(vp.valid){
				pPoints->cData.center=(vp+pPoints->points[0])*0.5;
				pPoints->cData.radius=vp.distanceTo(pPoints->cData.center);
            }
        }
            break;
        default:
            return;
        }
        deletePreview();
		RS_Circle* e=new RS_Circle(preview.get(), pPoints->cData);
        preview->addEntity(e);
        drawPreview();
        break;
    }
    case SetPoint2: {
		RS_Vector const& mouse=snapPoint(e);
		pPoints->points.resize(1);
		pPoints->points.push_back(mouse);
        deletePreview();
		pPoints->coord=mouse;
		if (!getCenters()) return;
		if (preparePreview()) {
			RS_Circle* e=new RS_Circle(preview.get(), pPoints->cData);
            preview->addEntity(e);
            drawPreview();
        }
        break;
    }
    case SetCenter: {

        //        RS_Entity*  en = catchEntity(e, enTypeList, RS2::ResolveAll);
		pPoints->coord= graphicView->toGraph(e->x(), e->y());
        //        circles[getStatus()]=static_cast<RS_Line*>(en);
        if(preparePreview()) {
            deletePreview();
			RS_Circle* e=new RS_Circle(preview.get(), pPoints->cData);
			for(size_t i=0; i<pPoints->centers.size(); ++i)
				preview->addEntity(new RS_Point(preview.get(), RS_PointData(pPoints->centers.at(i))));
            preview->addEntity(e);
//            double r0=cData.radius*0.1;
//            if(centers.size()>1)
//                for(unsigned i=0; i< centers.size(); ++i){
//                    RS_DEBUG->print(RS_Debug::D_ERROR, "center %d: (%g, %g)\n",i,centers.at(i).x,centers.at(i).y);
//                    preview->addEntity(new RS_Circle(preview, RS_CircleData(centers.at(i), r0)));
//                }
            drawPreview();
        }
    }
        break;
    default:
        break;
    }
    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::mouseMoveEvent end");
}

//void RS_ActionDrawCircleTan1_2P::setRadius(const double& r)
//{
//    cData.radius=r;
//    if(getStatus() == SetPoint2){
//        RS_Circle c(nullptr,cData);
//        centers=c.createTan1_2P(circle,cData.radius);
//    }
//}

bool RS_ActionDrawCircleTan1_2P::getCenters(){
	pPoints->centers.clear();
    if(getStatus() < SetPoint2) return false;

	LC_Quadratic lc0(circle, pPoints->points[0]);
//    LC_Quadratic lc1(circle, points[1]);
	LC_Quadratic lc1(pPoints->points[1], pPoints->points[0]);
	auto list=LC_Quadratic::getIntersection(lc0,lc1);
//    DEBUG_HEADER
//    std::cout<<"intersections : "<<list<<std::endl;

	for(const RS_Vector& vp: list){
        //when taking the path of center of tangent circle passing a given point,
        // the center is never closer to the circle center than the point, for internal and external tangent circles
		double ds0=vp.distanceTo(pPoints->points[0]);
//        double ds1=vp.distanceTo(points[1]);
//        if( fabs(ds0 - ds1)> RS_TOLERANCE) continue;
        if(circle->rtti()==RS2::EntityCircle||circle->rtti()==RS2::EntityArc){
            double ds=vp.distanceTo(circle->getCenter());
            //condition for tangential to the given circle
            if( fabs(ds - (ds0 + circle->getRadius())) > RS_TOLERANCE && fabs(ds - fabs(ds0 - circle->getRadius())) > RS_TOLERANCE ) continue;
        }else{
            double ds=0.;
            circle->getNearestPointOnEntity(vp, false,&ds);
            //condition for tangential to the given straight line
            if( fabs(ds - ds0)>RS_TOLERANCE) continue;
        }

        //avoid counting the same center
        bool existing=false;
		for(auto const& vq: pPoints->centers){
			if(vq.squaredTo(vp) < RS_TOLERANCE15 ){
                existing=true;
                break;
            }
        }
        if(existing) continue;
		pPoints->centers.push_back(vp);
    }
//    DEBUG_HEADER
//    std::cout<<"points: "<<points[0]<<" , "<<points[1]<<std::endl;
//    std::cout<<"centers.size()="<<centers.size()<<std::endl;
//    std::cout<<"centers: "<<centers<<std::endl;
	pPoints->valid= (pPoints->centers.size()>0);
	return pPoints->valid;
}

bool RS_ActionDrawCircleTan1_2P::preparePreview(){
	if (!pPoints->centers.size()) getCenters();
	if (!pPoints->centers.size()) return false;
	pPoints->cData.center=pPoints->centers.getClosest(pPoints->coord);
	pPoints->cData.radius=pPoints->points[0].distanceTo(pPoints->cData.center);
    return true;
}

RS_Entity* RS_ActionDrawCircleTan1_2P::catchCircle(QMouseEvent* e) {
	RS_Entity* ret=nullptr;
	RS_Entity* en = catchEntity(e,enTypeList, RS2::ResolveAll);
	if (!en) return ret;
	if (!en->isVisible()) return ret;
	if (en->getParent()) {
        if ( en->getParent()->ignoredOnModification()){
			return nullptr;
        }
    }
    return en;
}

void RS_ActionDrawCircleTan1_2P::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {

        switch (getStatus()) {
        case SetCircle1:
        {
            RS_Entity*  en = catchCircle(e);
			if (!en) return;
            circle = static_cast<RS_AtomicEntity*>(en);
            circle->setHighlighted(true);
            graphicView->redraw(RS2::RedrawDrawing);
            setStatus(getStatus()+1);
        }
            break;
        case SetPoint1:
        case SetPoint2:
        {
            RS_CoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
            break;
        case SetCenter:
			pPoints->coord=graphicView->toGraph(e->x(),e->y());
            if(preparePreview()) trigger();
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        // Return to last status:
        if(getStatus()>0){
            deletePreview();
        }
        init(getStatus()-1);
    }
}


void RS_ActionDrawCircleTan1_2P::coordinateEvent(RS_CoordinateEvent* e) {

    RS_Vector mouse = e->getCoordinate();
	pPoints->coord=mouse;
    switch(getStatus()){

    case SetPoint1:
		pPoints->points.clear();
		pPoints->points.push_back(mouse);
        setStatus(getStatus()+1);
        break;

    case SetPoint2:
//		pPoints->points.reserve(1);
		pPoints->points.push_back(mouse);
        if(getCenters()) {
			if(pPoints->centers.size()==1) trigger();
            else setStatus(getStatus()+1);
        }
        break;
    }

}

//fixme, support command line

/*
void RS_ActionDrawCircleTan1_2P::commandEvent(RS_CommandEvent* e) {
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


//void RS_ActionDrawCircleTan1_2P::showOptions() {
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions");
//        RS_ActionInterface::showOptions();

//        RS_DIALOGFACTORY->requestOptions(this, true);
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions: OK");
//}



//void RS_ActionDrawCircleTan1_2P::hideOptions() {
//        RS_ActionInterface::hideOptions();

//        RS_DIALOGFACTORY->requestOptions(this, false);
//}


QStringList RS_ActionDrawCircleTan1_2P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleTan1_2P::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetCircle1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify a line/arc/circle"),
											tr("Cancel"));
		break;

	case SetPoint1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first point on the tangent circle"),
											tr("Back"));
		break;

	case SetPoint2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second point on the tangent circle"),
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



void RS_ActionDrawCircleTan1_2P::updateMouseCursor()
{
    switch (getStatus())
    {
        case SetCircle1:
        case SetCenter:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
        case SetPoint1:
        case SetPoint2:
            graphicView->setMouseCursor(RS2::CadCursor);
            break;
    }
}


double RS_ActionDrawCircleTan1_2P::getRadius() const{
	return pPoints->cData.radius;
}

// EOF
