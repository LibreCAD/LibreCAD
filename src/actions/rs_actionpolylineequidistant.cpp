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

#include "rs_actionpolylineequidistant.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_information.h"

RS_ActionPolylineEquidistant::RS_ActionPolylineEquidistant(RS_EntityContainer& container,
                RS_GraphicView& graphicView)
                :RS_PreviewActionInterface("Create Equidistant Polylines",
                                                   container, graphicView) {
        dist = 1.0;
        number = 1;
}


QAction* RS_ActionPolylineEquidistant::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        QAction* action = new QAction(tr("Create &Equidistant Polylines"), NULL);
        action->setShortcut(QKeySequence());
        action->setIcon(QIcon(":/extui/polylineequidstant.png"));
        action->setStatusTip(tr("Create Equidistant Polylines"));
        return action;
}

void RS_ActionPolylineEquidistant::init(int status) {
        RS_ActionInterface::init(status);
        originalEntity = NULL;
        targetPoint = RS_Vector(false);
        bRightSide = false;
}

bool RS_ActionPolylineEquidistant::makeContour() {
        if (container==NULL) {
                RS_DEBUG->print("RS_ActionPolylineEquidistant::makeContour: no valid container",
                                                RS_Debug::D_WARNING);
                return false;
        }

        RS_Vector offset(false);
        QList<RS_Entity*> addList;

        if (document!=NULL) {
                document->startUndoCycle();
        }
        double neg = 1.0;
        if(bRightSide)
                neg = -1.0;
        // Create new entites
        RS_Line line1(NULL, RS_LineData(RS_Vector(true), RS_Vector(true)));
        RS_Line line2(NULL, RS_LineData(RS_Vector(true), RS_Vector(true)));
        for (int num=1;
                        num<=number || (number==0 && num<=1);
                        num++) {
//            std::cout<<"copy: "<<num<<" of "<<number<<std::endl;
                RS_Polyline* newPolyline = new RS_Polyline(container);
                newPolyline->setClosed(((RS_Polyline*)originalEntity)->isClosed());
//		newPolyline->setSelected((RS_Polyline*)originalEntity)->isSelected());
                newPolyline->setLayer(((RS_Polyline*)originalEntity)->getLayer());
                newPolyline->setPen(((RS_Polyline*)originalEntity)->getPen());

                bool first = true;
                RS_Entity* lastEntity = ((RS_Polyline*)originalEntity)->lastEntity();
                for (RS_Entity* en=((RS_Polyline*)originalEntity)->firstEntity(); en!=NULL; en=((RS_Polyline*)originalEntity)->nextEntity()) {
                        double bulge = 0.0;
                        if (en->getLength() < 1.0e-15) continue;
                        if (en->rtti()==RS2::EntityArc) {
                                double r0 = ((RS_Arc*)en)->getRadius();
                                double r = r0 - dist*neg;
                                if(r < 0)
                                        break;
                                ((RS_Arc*)en)->setRadius(r);
                                bulge = ((RS_Arc*)en)->getBulge();
                                ((RS_Arc*)en)->setRadius(r0);
                        } else {
                                bulge = 0.0;
                        }
                        RS_Vector v1 = ((RS_AtomicEntity*)en)->getStartpoint();
                        RS_Vector v2 = ((RS_AtomicEntity*)en)->getEndpoint();
                        offset.set(dist * cos(v1.angleTo(v2)+M_PI*0.5*neg), dist * sin(v1.angleTo(v2)+M_PI*0.5*neg));
                        v1.move(offset*num);
                        v2.move(offset*num);
                        if (first) {
                                line1.setStartpoint(v1);
                                line1.setEndpoint(v2);
                                if(newPolyline->isClosed()){
                                        RS_Vector v01 = ((RS_AtomicEntity*)lastEntity)->getStartpoint();
                                        RS_Vector v02 = ((RS_AtomicEntity*)en)->getStartpoint();
                                        offset.set(dist * cos(v01.angleTo(v02)+M_PI*0.5*neg), dist * sin(v01.angleTo(v02)+M_PI*0.5*neg));
                                        v01.move(offset*num);
                                        v02.move(offset*num);
                                        line2.setStartpoint(v01);
                                        line2.setEndpoint(v02);
                                        RS_VectorSolutions vsol = RS_Information::getIntersection(&line1, &line2, false);
                                        v1 = vsol.get(0);
                                }
                                newPolyline->setStartpoint(v1);
                                newPolyline->setNextBulge(bulge);
                                if (en == lastEntity) {
                                    newPolyline->addVertex(v2, bulge);
                                }
                                first = false;
                        }else{
                            line2.setStartpoint(v1);
                            line2.setEndpoint(v2);
                            RS_VectorSolutions vsol = RS_Information::getIntersection(&line1, &line2, false);
                            RS_Vector v;
                            if (vsol.getNumber()>0) {
                                v= vsol.get(0);
                            }else {
                                //fixme, this is not correct
                                v=(line1.getEndpoint()+v1)*0.5;
                            }

                            newPolyline->addVertex(v, bulge);
                            newPolyline->setEndpoint(v);
                            line1.setStartpoint(v1);
                            line1.setEndpoint(v2);
                            if (en==lastEntity/* && newPolyline->isClosed()==false*/){
                                newPolyline->addVertex(v2, bulge);
                                }
                        }
                }
                double bulge = lastEntity->rtti() == RS2::EntityArc? ((RS_Arc*)lastEntity)->getBulge():0.0;
                newPolyline->setNextBulge(bulge);
                newPolyline->endPolyline();
                container->addEntity(newPolyline);
                document->addUndoable(newPolyline);
        }
        if (document!=NULL) {
                document->endUndoCycle();
        }

        if (graphicView!=NULL) {
                graphicView->redraw();
        }


        return true;
}

void RS_ActionPolylineEquidistant::trigger() {

        RS_DEBUG->print("RS_ActionPolylineEquidistant::trigger()");

        if (originalEntity!=NULL && targetPoint.valid ) {

                originalEntity->setHighlighted(false);
                graphicView->drawEntity(originalEntity);

                makeContour();

                originalEntity = NULL;
                targetPoint = RS_Vector(false);
                bRightSide = false;
                setStatus(ChooseEntity);

                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineEquidistant::mouseMoveEvent(QMouseEvent* e) {
        RS_DEBUG->print("RS_ActionPolylineEquidistant::mouseMoveEvent begin");

        switch (getStatus()) {
        case ChooseEntity:
                snapPoint(e);
                break;
        default:
                break;
        }

        RS_DEBUG->print("RS_ActionPolylineEquidistant::mouseMoveEvent end");
}



void RS_ActionPolylineEquidistant::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
                switch (getStatus()) {
                case ChooseEntity:
                        originalEntity = catchEntity(e);
                        if (originalEntity==NULL) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (originalEntity->rtti()!=RS2::EntityPolyline) {

                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Entity must be a polyline."));
                        } else {
                                targetPoint = snapFree(e);
                                originalEntity->setHighlighted(true);
                                graphicView->drawEntity(originalEntity);
                                double d = graphicView->toGraphDX(snapRange)*0.9;
                                RS_Entity* Segment =  ((RS_Polyline*)originalEntity)->getNearestEntity( targetPoint, &d, RS2::ResolveNone);
                                double ang = ((RS_Line*)Segment)->getAngle1();
                                double ang1 = ((RS_Line*)Segment)->getStartpoint().angleTo(RS_Vector(targetPoint));
                                if( ang > ang1 || ang + M_PI < ang1 )
                                        bRightSide = true;
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
                                trigger();
                        }
                        break;
                default:
                        break;
                }
        } else if (e->button()==Qt::RightButton) {
                deleteSnapper();
                if (originalEntity!=NULL) {
                        originalEntity->setHighlighted(false);
                        graphicView->drawEntity(originalEntity);
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
                }
                init(getStatus()-1);
        }
}

void RS_ActionPolylineEquidistant::showOptions() {
        RS_ActionInterface::showOptions();

        RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionPolylineEquidistant::hideOptions() {
        RS_ActionInterface::hideOptions();

        RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionPolylineEquidistant::updateMouseCursor() {
        graphicView->setMouseCursor(RS2::CadCursor);
}

void RS_ActionPolylineEquidistant::updateMouseButtonHints() {
        switch (getStatus()) {
        case ChooseEntity:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Choose the original polyline"),
                                                                                        tr("Cancel"));
                break;
        default:
                RS_DIALOGFACTORY->updateMouseWidget("", "");
                break;
        }
}

void RS_ActionPolylineEquidistant::updateToolBar() {
    //not needed any more with new snap
    return;
        switch (getStatus()) {
        case ChooseEntity:
                RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
                break;
        default:
                RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarPolylines);
                break;
        }
}
// EOF
