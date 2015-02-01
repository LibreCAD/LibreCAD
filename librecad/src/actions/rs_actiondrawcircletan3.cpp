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
#include <QDebug>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "lc_quadratic.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan3::RS_ActionDrawCircleTan3(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circle inscribed",
                           container, graphicView),
          cData(RS_Vector(0.,0.),1.),
          enTypeList()
{
//    supported types
//    enTypeList<<RS2::EntityLine<<RS2::EntityArc<<RS2::EntityCircle;
    enTypeList<<RS2::EntityLine<<RS2::EntityArc<<RS2::EntityCircle;
}



RS_ActionDrawCircleTan3::~RS_ActionDrawCircleTan3() {
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
        for(int i=0;i<circles.size();i++) {
            if(circles.at(i) != NULL) circles.at(i)->setHighlighted(false);
        }
        graphicView->redraw(RS2::RedrawDrawing);
        circles.clear();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

//void RS_ActionDrawCircleTan3::finish(bool updateTB){
////    for(int i=0;i<circles.size();i++) circles[i]->setHighlighted(false);
////    graphicView->redraw(RS2::RedrawDrawing);
////    circles.clear();
//    RS_PreviewActionInterface::finish(updateTB);
//}


void RS_ActionDrawCircleTan3::trigger() {
//    std::cout<<__FILE__<<" : "<<__FUNCTION__<<" : line "<<__LINE__<<std::endl;
//    std::cout<<"begin"<<std::endl;

    RS_PreviewActionInterface::trigger();


    RS_Circle* circle=new RS_Circle(container, cData);

    container->addEntity(circle);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(circle);
        document->endUndoCycle();
    }

    for(int i=0;i<circles.size();i++) circles[i]->setHighlighted(false);
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
        if(preparePreview()) {
            deletePreview();
            RS_Circle* e=new RS_Circle(preview, cData);
            preview->addEntity(e);
            for(RS_Circle& c: candidates){
                preview->addEntity(new RS_Point(NULL, RS_PointData(c.getCenter())));
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
    int i=0;
    for(;i<circles.size();++i)
        if(circles[i]->rtti() == RS2::EntityLine) break;
    candidates.clear();
        const int i1=(i+1)%3;
        const int i2=(i+2)%3;
        if(i<circles.size() && circles[i]->rtti() == RS2::EntityLine){

            LC_Quadratic lc0(circles[i],circles[i1],false);
             LC_Quadratic lc01(circles[i],circles[i1],true);
            LC_Quadratic lc1;
            RS_VectorSolutions sol;
            //detect degenerate case two circles with the same radius
            if(circles[i1]->rtti()== RS2::EntityCircle &&
                    circles[i2]->rtti()== RS2::EntityCircle
                    ){
                RS_Circle* c1=static_cast<RS_Circle*>(circles[i1]);
                RS_Circle* c2=static_cast<RS_Circle*>(circles[i2]);
                if(fabs(fabs(c1->getRadius())-fabs(c2->getRadius()))<RS_TOLERANCE){
                    //degenerate
                    const RS_Vector p0=(c1->getCenter()+c2->getCenter())*0.5;
                    const RS_Vector p1=p0 + (c1->getCenter() - p0).rotate(0.5*M_PI);
                    lc1=RS_Line(NULL, RS_LineData(p0,p1 )).getQuadratic();
                    sol=LC_Quadratic::getIntersection(lc0,lc1);

                    sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
                    lc1=RS_Line(NULL, RS_LineData(c1->getCenter(),c1->getCenter())).getQuadratic();
                    sol.appendTo(LC_Quadratic::getIntersection(lc0,lc1));
                    sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
                }

            }
            if(sol.size()==0) {
                switch(circles[i2]->rtti()){
                case RS2::EntityCircle:
                    lc1=LC_Quadratic(circles[i],circles[i2], true);
                    sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
                    if(circles[i1]->rtti()== RS2::EntityCircle )
                        sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
                    //there's no break, because the default part would be run for circles as well
                default:
                    lc1=LC_Quadratic(circles[i],circles[i2]);
                    sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
                    if(circles[i1]->rtti()== RS2::EntityCircle )
                        sol.appendTo(LC_Quadratic::getIntersection(lc01,lc1));
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
        for(size_t j=0; j<sol.size(); ++j){
            const RS_Vector&& vp=sol.at(j);
            if(vp.magnitude()>RS_MAXDOUBLE) continue;
            if(sol1.size())
                if(sol1.getClosestDistance(vp)<RS_TOLERANCE) continue;

            sol1.push_back(vp);
        }


        for(size_t j=0;j<sol1.size();j++){
            circles[i]->getNearestPointOnEntity(sol1[j],false,&d);
            RS_CircleData data(sol1[j],d);
            if(circles[(i+1)%3]->isTangent(data)==false) continue;
            if(circles[(i+2)%3]->isTangent(data)==false) continue;
            candidates<<RS_Circle(NULL,data);
        }
    }else{
        RS_Circle c(NULL,cData);
        candidates=c.createTan3(circles);
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
    int index=candidates.size();
    double dist=RS_MAXDOUBLE*RS_MAXDOUBLE;
    for(int i=0;i<candidates.size();i++){
        double d;
        candidates.at(i).getNearestPointOnEntity(coord,false,&d);
        double dCenter=coord.distanceTo(candidates.at(i).getCenter());
        d=std::min(d,dCenter);
        if(d<dist){
            dist=d;
            index=i;
        }
    }
    if( index<candidates.size()){
        cData= candidates.at(index).getData();
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
    for(int i=0;i<getStatus();i++) {
        if(en->getId() == circles[i]->getId()) return ret; //do not pull in the same line again
    }
    if(en->getParent() != NULL) {
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
//            DEBUG_HEADER();
//            qDebug()<<"en="<<en;
            if (en==NULL) return;
            circles.resize(getStatus());
            if(circles.indexOf(static_cast<RS_AtomicEntity*>(en))>=0) return;
//            for(int i=0;i<circles.size();i++){
//                if(
//                        (circles.at(i)->getCenter() - en->getCenter()).squared() < RS_TOLERANCE2
//                        && fabs( circles.at(i)->getRadius() - en->getRadius())<RS_TOLERANCE
//                        ) return;
//            }
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


//void RS_ActionDrawCircleTan3::showOptions() {
//    RS_DEBUG->print("RS_ActionDrawCircleTan3::showOptions");
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::showOptions();

//        RS_DIALOGFACTORY->requestOptions(this, true);
//    }
//    RS_DEBUG->print("RS_ActionDrawCircleTan3::showOptions: OK");
//}



//void RS_ActionDrawCircleTan3::hideOptions() {
//    if(RS_DIALOGFACTORY != NULL){
//        RS_ActionInterface::hideOptions();

//        RS_DIALOGFACTORY->requestOptions(this, false);
//    }
//}


QStringList RS_ActionDrawCircleTan3::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleTan3::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
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
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawCircleTan3::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionDrawCircleTan3::updateToolBar() {
//    if (RS_DIALOGFACTORY!=NULL) {
//        if (isFinished()) {
//            RS_DIALOGFACTORY->resetToolBar();
//        }
//    }
//}

// EOF
