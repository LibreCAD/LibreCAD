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

#include "rs_actiondrawcircletan1_2p.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "lc_quadratic.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan1_2P::RS_ActionDrawCircleTan1_2P(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw tangent circle 2P",
                               container, graphicView)
    ,circle(NULL)
    ,cData(RS_Vector(0.,0.),1.)
    ,enTypeList()
{
    //    supported types
    enTypeList<<RS2::EntityLine<<RS2::EntityArc<<RS2::EntityCircle;
}



RS_ActionDrawCircleTan1_2P::~RS_ActionDrawCircleTan1_2P() {
}



QAction* RS_ActionDrawCircleTan1_2P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Circle Tangential 2 P&oints"), NULL);
    action->setIcon(QIcon(":/extui/circletan1_2p.png"));
    return action;
}

void RS_ActionDrawCircleTan1_2P::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status>=0) {
        RS_Snapper::suspend();
    }

    if (status<=SetCircle1) {
        if(circle!=NULL) {
            if(circle->isHighlighted()){
                circle->setHighlighted(false);
                graphicView->redraw(RS2::RedrawDrawing);
            }
        }
        points.clear();
    }
}


void RS_ActionDrawCircleTan1_2P::finish(bool updateTB){
    if( circle != NULL) {
        circle->setHighlighted(false);
        graphicView->redraw(RS2::RedrawDrawing);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan1_2P::trigger() {
    //    std::cout<<__FILE__<<" : "<<__FUNCTION__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"begin"<<std::endl;

    RS_PreviewActionInterface::trigger();


    RS_Circle* c=new RS_Circle(container, cData);

    container->addEntity(c);

    // upd. undo list:
    if (document!=NULL) {
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
        points.clear();
        points<<mouse;
        switch(circle->rtti()){
        case RS2::EntityArc:
        case RS2::EntityCircle:
        {
            RS_Vector&& dvp=mouse - circle->getCenter();
            double&& rvp=dvp.magnitude();
            if(rvp<RS_TOLERANCE2) break;
            cData.radius=(circle->getRadius()+rvp)*0.5;
            cData.center=circle->getCenter()+dvp*(cData.radius/rvp);
            cData.radius=fabs(circle->getRadius()-cData.radius);
        }
            break;
        case RS2::EntityLine:
        {
            RS_Line* line=static_cast<RS_Line*>(circle);
            RS_Vector&& vp=line->getNearestPointOnEntity(points[0],false);
            if(vp.valid){
                cData.center=(vp+points[0])*0.5;
                cData.radius=vp.distanceTo(cData.center);
            }
        }
            break;
        default:
            return;
        }
        deletePreview();
        RS_Circle* e=new RS_Circle(preview, cData);
        preview->addEntity(e);
        drawPreview();
        break;
    }
    case SetPoint2: {
        RS_Vector&& mouse=snapPoint(e);
        points.resize(1);
        points<<mouse;
        deletePreview();
        coord=mouse;
        if(getCenters()==false) return;
        if(preparePreview()) {
            RS_Circle* e=new RS_Circle(preview, cData);
            preview->addEntity(e);
            drawPreview();
        }
        break;
    }
    case SetCenter: {

        //        RS_Entity*  en = catchEntity(e, enTypeList, RS2::ResolveAll);
        coord= graphicView->toGraph(e->x(), e->y());
        //        circles[getStatus()]=static_cast<RS_Line*>(en);
        if(preparePreview()) {
            deletePreview();
            RS_Circle* e=new RS_Circle(preview, cData);
            preview->addEntity(e);
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
//        RS_Circle c(NULL,cData);
//        centers=c.createTan1_2P(circle,cData.radius);
//    }
//}

bool RS_ActionDrawCircleTan1_2P::getCenters(){
    centers.clean();
    if(getStatus() < SetPoint2) return false;

    LC_Quadratic lc0(circle, points[0]);
    LC_Quadratic lc1(circle, points[1]);
    auto&& list=LC_Quadratic::getIntersection(lc0,lc1);
//    DEBUG_HEADER();
//    std::cout<<"intersections : "<<list<<std::endl;

    for(unsigned int i=0;i<list.size();i++){
        auto vp=list.get(i);
        //when taking the path of center of tangent circle passing a given point,
        // the center is never closer to the circle center than the point, for internal and external tangent circles
        double ds0=vp.distanceTo(points[0]);
        double ds1=vp.distanceTo(points[1]);
//        if( fabs(ds0 - ds1)> RS_TOLERANCE) continue;
        if(circle->rtti()==RS2::EntityCircle||circle->rtti()==RS2::EntityArc){
            auto&& ds=vp.distanceTo(circle->getCenter()) - RS_TOLERANCE;
            if( ds0 <= ds || ds1 <= ds ) continue;
        }
        centers.push_back(vp);
    }
//    DEBUG_HEADER();
//    std::cout<<"points: "<<points[0]<<" , "<<points[1]<<std::endl;
//    std::cout<<"centers.size()="<<centers.size()<<std::endl;
//    std::cout<<"centers: "<<centers<<std::endl;
    valid= (centers.size()>0);
    return valid;
}

bool RS_ActionDrawCircleTan1_2P::preparePreview(){
    if(centers.size()==0) getCenters();
    if(centers.size()==0) return false;
    cData.center=centers.getClosest(coord);
    cData.radius=points[0].distanceTo(cData.center);
    return true;
}

RS_Entity* RS_ActionDrawCircleTan1_2P::catchCircle(QMouseEvent* e) {
    RS_Entity* ret=NULL;
    RS_Entity*  en = catchEntity(e,enTypeList, RS2::ResolveAll);
    if(en == NULL) return ret;
    if(en->isVisible()==false) return ret;
    if(en->getParent() != NULL) {
        if ( en->getParent()->rtti() == RS2::EntityInsert         /**Insert*/
             || en->getParent()->rtti() == RS2::EntitySpline
             || en->getParent()->rtti() == RS2::EntityMText        /**< Text 15*/
             || en->getParent()->rtti() == RS2::EntityText         /**< Text 15*/
             || en->getParent()->rtti() == RS2::EntityDimAligned   /**< Aligned Dimension */
             || en->getParent()->rtti() == RS2::EntityDimLinear    /**< Linear Dimension */
             || en->getParent()->rtti() == RS2::EntityDimRadial    /**< Radial Dimension */
             || en->getParent()->rtti() == RS2::EntityDimDiametric /**< Diametric Dimension */
             || en->getParent()->rtti() == RS2::EntityDimAngular   /**< Angular Dimension */
             || en->getParent()->rtti() == RS2::EntityDimLeader    /**< Leader Dimension */
             ){
            return ret;
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
            if (en==NULL) return;
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
            coord=graphicView->toGraph(e->x(),e->y());
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
    coord=mouse;
    switch(getStatus()){

    case SetPoint1:
        points.clear();
        points<<mouse;
        setStatus(getStatus()+1);
        break;

    case SetPoint2:
        points.reserve(1);
        points<<mouse;
        if(getCenters()) {
            if(centers.size()==1) trigger();
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
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok==true) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
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


//void RS_ActionDrawCircleTan1_2P::showOptions() {
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions");
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::showOptions();

//        RS_DIALOGFACTORY->requestOptions(this, true);
//    }
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions: OK");
//}



//void RS_ActionDrawCircleTan1_2P::hideOptions() {
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::hideOptions();

//        RS_DIALOGFACTORY->requestOptions(this, false);
//    }
//}


QStringList RS_ActionDrawCircleTan1_2P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleTan1_2P::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
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
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawCircleTan1_2P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionDrawCircleTan1_2P::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}

// EOF
