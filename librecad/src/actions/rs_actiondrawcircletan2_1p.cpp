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

#include "rs_actiondrawcircletan2_1p.h"

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
RS_ActionDrawCircleTan2_1P::RS_ActionDrawCircleTan2_1P(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw tangent circle 2P",
                               container, graphicView),
      cData(RS_Vector(0.,0.),1.),
      enTypeList()
{
    //    supported types
    enTypeList<<RS2::EntityLine<<RS2::EntityArc<<RS2::EntityCircle;
}



RS_ActionDrawCircleTan2_1P::~RS_ActionDrawCircleTan2_1P() {
}



QAction* RS_ActionDrawCircleTan2_1P::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Common Tangential Circle 1 Point"), NULL);
    action->setIcon(QIcon(":/extui/circletan2_1p.png"));
    return action;
}

void RS_ActionDrawCircleTan2_1P::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status>=0) {
        RS_Snapper::suspend();
    }

    if (status<=SetCircle2) {
        bool updateNeeded(false);
        if(circles.size()>=2 && circles[1]!=NULL) {
            if(circles[1]->isHighlighted()){
                circles[1]->setHighlighted(false);
                updateNeeded=true;
            }
        }
        if(status<= SetCircle1 && circles.size()>=1&&circles[0]!=NULL) {
            if(circles[0]->isHighlighted()){
                circles[0]->setHighlighted(false);
                updateNeeded=true;
            }
        }
        if(updateNeeded) graphicView->redraw(RS2::RedrawDrawing);
        circles.clear();
    }
}


void RS_ActionDrawCircleTan2_1P::finish(bool updateTB){
    if( circles.size() >0) {
        foreach(RS_AtomicEntity* circle, circles)
            circle->setHighlighted(false);
        graphicView->redraw(RS2::RedrawDrawing);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

//void RS_ActionDrawCircleTan2_1P::finish(bool updateTB){
////    for(int i=0;i<circles.size();i++) circles[i]->setHighlighted(false);
////    graphicView->redraw(RS2::RedrawDrawing);
////    circles.clear();
//    RS_PreviewActionInterface::finish(updateTB);
//}


void RS_ActionDrawCircleTan2_1P::trigger() {
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


    foreach(RS_AtomicEntity* circle, circles)
        circle->setHighlighted(false);
    graphicView->redraw(RS2::RedrawDrawing);
    circles.clear();
    //    drawSnapper();

    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::trigger():"
                    " entity added: %d", c->getId());
}


bool RS_ActionDrawCircleTan2_1P::getCenters()
{
    if(circles.size()<2) return false;
    LC_Quadratic lc0(circles[0], point);
    LC_Quadratic lc1(circles[1], point);

    auto&& list=LC_Quadratic::getIntersection(lc0,lc1);
    centers.clean();
    for(int i=0;i<list.size();i++){
        auto&& vp=list.get(i);
        double r0=fabs(
                    vp.distanceTo(circles[0]->getCenter())-
                    circles[0]->getRadius()
                    );
        double r1=fabs(
                    vp.distanceTo(circles[1]->getCenter())-
                    circles[1]->getRadius()
                    );
        if( fabs(r0-r1)>=RS_TOLERANCE) continue;
        centers.push_back(vp);
    }
    return centers.size()>0;
}

void RS_ActionDrawCircleTan2_1P::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::mouseMoveEvent begin");

    switch( getStatus()){
    case SetPoint:
        coord=snapPoint(e);
        point=coord;
        break;
    case SetCenter:
        coord=graphicView->toGraph(e->x(),e->y());
        break;
    default:
        return;
    }
    deletePreview();
    if(preparePreview()){
        RS_Circle* e=new RS_Circle(preview, cData);
        preview->addEntity(e);
        drawPreview();
    }
    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::mouseMoveEvent end");
}

//void RS_ActionDrawCircleTan2_1P::setRadius(const double& r)
//{
//    cData.radius=r;
//    if(getStatus() == SetPoint2){
//        RS_Circle c(NULL,cData);
//        centers=c.createTan2_1P(circle,cData.radius);
//    }
//}

bool RS_ActionDrawCircleTan2_1P::preparePreview(){
    if( getCenters() ==false) return false;
//    for(int i=0;i<centers.size();i++){
//        double ds2=(centers[i]-point).squared();
//        if( (centers[i]-circles[0]).squared()<ds2
//    }
    cData.center=centers.getClosest(coord);
    cData.radius=fabs(cData.center.distanceTo(circles[0]->getCenter())
                      -circles[0]->getRadius());
//    cData.radius=point.distanceTo(cData.center);
    return true;
}

RS_Entity* RS_ActionDrawCircleTan2_1P::catchCircle(QMouseEvent* e) {
    RS_Entity* ret=NULL;
    RS_Entity*  en = catchEntity(e,enTypeList, RS2::ResolveAll);
    if(en == NULL) return ret;
    if(en->isVisible()==false) return ret;
    for(int i=0;i<getStatus();i++) {
        if(en->getId() == circles[i]->getId()) return ret; //do not pull in the same line again
    }
    if(en->getParent() != NULL) {
        if ( en->getParent()->rtti() == RS2::EntityInsert         /**Insert*/
             || en->getParent()->rtti() == RS2::EntitySpline
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

void RS_ActionDrawCircleTan2_1P::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {

        switch (getStatus()) {
        case SetCircle1:
        case SetCircle2:
        {
            circles.resize(getStatus());
            RS_AtomicEntity*  en = static_cast<RS_AtomicEntity*>(catchCircle(e));
            if (en==NULL) return;
//            circle = static_cast<RS_AtomicEntity*>(en);
            en->setHighlighted(true);
            circles<<en;
            graphicView->redraw(RS2::RedrawDrawing);
            setStatus(getStatus()+1);
        }
            break;
        case SetPoint:
        {
            RS_Vector snapped = snapPoint(e);
            RS_CoordinateEvent ce(snapped);
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


void RS_ActionDrawCircleTan2_1P::coordinateEvent(RS_CoordinateEvent* e) {

    RS_Vector mouse = e->getCoordinate();
    switch(getStatus()){

    case SetPoint:
        point=mouse;
        coord=mouse;
        if(getCenters()) {
            if(centers.size()==1) trigger();
            else setStatus(getStatus()+1);
        }
        break;
        default:
        break;
//    case SetCenter:
//        coord=mouse;
//        trigger();
    }

}

//fixme, support command line

/*
void RS_ActionDrawCircleTan2_1P::commandEvent(RS_CommandEvent* e) {
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


//void RS_ActionDrawCircleTan2_1P::showOptions() {
//    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::showOptions");
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::showOptions();

//        RS_DIALOGFACTORY->requestOptions(this, true);
//    }
//    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::showOptions: OK");
//}



//void RS_ActionDrawCircleTan2_1P::hideOptions() {
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::hideOptions();

//        RS_DIALOGFACTORY->requestOptions(this, false);
//    }
//}


QStringList RS_ActionDrawCircleTan2_1P::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleTan2_1P::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetCircle1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify an arc/circle"),
                                                tr("Cancel"));
            break;

        case SetCircle2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the another arc/circle"),
                                                tr("Back"));
            break;

        case SetPoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify a point on the tangent circle"),
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



void RS_ActionDrawCircleTan2_1P::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionDrawCircleTan2_1P::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}

// EOF
