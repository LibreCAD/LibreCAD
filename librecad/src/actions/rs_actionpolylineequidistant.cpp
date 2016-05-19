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

#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actionpolylineequidistant.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_polyline.h"
#include "rs_information.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_debug.h"

RS_ActionPolylineEquidistant::RS_ActionPolylineEquidistant(RS_EntityContainer& container,
														   RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Create Equidistant Polylines",
							   container, graphicView)
	, targetPoint(new RS_Vector{})
	,dist(1.)
	,number(1)
{
	actionType=RS2::ActionPolylineEquidistant;
}

RS_ActionPolylineEquidistant::~RS_ActionPolylineEquidistant()=default;


void RS_ActionPolylineEquidistant::init(int status) {
        RS_PreviewActionInterface::init(status);
		originalEntity = nullptr;
		*targetPoint = {};
        bRightSide = false;
}

/**
 * Helper function for makeContour
 * Modify newEntity to parellel of orgEntity at distance dist
 * If dist is positive the offset is in left else in right
 * Bot newEntity and orgEntity are the same type of entity
 * if not return nullptr pointer
 *
 * @retval RS_Entity* of parellel entity
 *
 * @author Rallaz
 */
RS_Entity* RS_ActionPolylineEquidistant::calculateOffset(RS_Entity* newEntity,RS_Entity* orgEntity, double dist) {
    if (orgEntity->rtti()==RS2::EntityArc && newEntity->rtti()==RS2::EntityArc) {
        RS_Arc* arc = (RS_Arc*)newEntity;
        double r0 = ((RS_Arc*)orgEntity)->getRadius();
        double r;
        if ( ((RS_Arc*)orgEntity)->isReversed())
            r = r0 + dist;
        else
            r = r0 - dist;
        if(r < 0)
			return nullptr;
        arc->setData(((RS_Arc*)orgEntity)->getData());
        arc->setRadius(r);
		arc->calculateBorders();
        return newEntity;
    } else if (orgEntity->rtti()==RS2::EntityLine && newEntity->rtti()==RS2::EntityLine) {
        RS_Line* line0 = (RS_Line*)orgEntity;
        RS_Line* line1 = (RS_Line*)newEntity;
        RS_Vector v0 = line0->getStartpoint();
        RS_Vector v1(v0.x,v0.y+dist);
        RS_Vector v2(v0.x+line0->getLength(),v0.y+dist);
        line1->setStartpoint(v1);
        line1->setEndpoint(v2);
        line1->rotate(v0, line0->getAngle1());
        return newEntity;
    }
	return nullptr;
}

/**
 * Helper function for makeContour
 * Calculate the intersection point of first and last entities
 * The first vertex is not added and the last is returned instead of added
 *
 * @retval RS_Vector nearest to startpoint of last and endpoint of first or RS_Vector(false) if not
 *
 * @author Rallaz
 */
RS_Vector RS_ActionPolylineEquidistant::calculateIntersection(RS_Entity* first,RS_Entity* last) {
    RS_VectorSolutions vsol;
    RS_Vector v(false);
    vsol = RS_Information::getIntersection(first, last, false);
    if (vsol.getNumber()==0) {
        //Parallel entities
        return RS_Vector(false);
    } else if (vsol.getNumber()>1 &&
               vsol.get(0).distanceTo(last->getStartpoint()) > vsol.get(1).distanceTo(last->getStartpoint())) {
        return vsol.get(1);
    }
    return vsol.get(0);
}

bool RS_ActionPolylineEquidistant::makeContour() {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_ActionPolylineEquidistant::makeContour: no valid container");
        return false;
    }

    RS_Polyline* originalPolyline = (RS_Polyline*)originalEntity;
//create a list of entities to offset without length = 0
	QList<RS_Entity*> entities;
	for(auto en: *originalPolyline){
        if (en->getLength() > 1.0e-12)
            entities.append(en);
    }
    if (entities.isEmpty()) {
        return false;
    }
	if (document) {
        document->startUndoCycle();
    }
    double neg = 1.0;
    if(bRightSide)
        neg = -1.0;

    // Create new helper entities
	RS_Vector const origin{0.,0.};
	RS_Line line1{origin, origin};//current line
	RS_Line lineFirst{origin, origin};//previous line
	RS_Arc arc1(nullptr, RS_ArcData(origin, 0,0,0,false));//current arc
	RS_Arc arcFirst(nullptr, RS_ArcData(origin, 0,0,0,false));//previous arc

    for (int num=1; num<=number || (number==0 && num<=1); num++) {
        RS_Polyline* newPolyline = new RS_Polyline(container);
        newPolyline->setLayer(((RS_Polyline*)originalEntity)->getLayer());
        newPolyline->setPen(((RS_Polyline*)originalEntity)->getPen());

        bool first = true;
        bool closed = originalPolyline->isClosed();
        double bulge = 0.0;
        RS_Entity* en;
        RS_Entity* prevEntity = entities.last();
		RS_Entity* currEntity=nullptr;
        for (int i = 0; i < entities.size(); ++i) {
            en = entities.at(i);
			RS_Vector v{false};
            if (en->rtti()==RS2::EntityArc) {
                currEntity = &arc1;
                calculateOffset(currEntity, en, dist*num*neg);
                bulge = arc1.getBulge();
            } else {
                currEntity = &line1;
                bulge = 0.0;
                calculateOffset(currEntity, en, dist*num*neg);
            }
            if (first) {
                if (closed){
                    if (prevEntity->rtti()==RS2::EntityArc) {
                        prevEntity = calculateOffset(&arcFirst, prevEntity, dist*num*neg);
                    } else {
                        prevEntity = calculateOffset(&lineFirst, prevEntity, dist*num*neg);
                    }
                    v = calculateIntersection(prevEntity, currEntity);
                }
                if (!v.valid) {
                    v = currEntity->getStartpoint();
                    closed = false;
                } else if (currEntity->rtti()==RS2::EntityArc) {
                    //update bulge
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
					arc1.calculateBorders();
                    bulge = arc1.getBulge();
                }
                first = false;
                if (!prevEntity) break; //prevent crash if not exist offset for prevEntity
            }else{
                v = calculateIntersection(prevEntity, currEntity);
                if (!v.valid) {
                    v= prevEntity->getEndpoint();
                    double dess = currEntity->getStartpoint().distanceTo(prevEntity->getEndpoint());
                    if (dess > 1.0e-12) {
                        newPolyline->addVertex(v, bulge);
						prevEntity = nullptr;
                        break;
                    }
                }
                double startAngle = prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint());
                if (prevEntity->rtti()==RS2::EntityArc) {
                    arcFirst.setAngle2(arcFirst.getCenter().angleTo(v));
					arcFirst.calculateBorders();
                     newPolyline->setNextBulge(arcFirst.getBulge());
                }
                //check if the entity are reverted
                if (fabs(remainder(prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint())- startAngle, 2.*M_PI)) > 0.785){
                    prevEntity = newPolyline->lastEntity();
                    RS_Vector v0 = calculateIntersection(prevEntity, currEntity);
                    if (prevEntity->rtti()==RS2::EntityArc) {
                        ((RS_Arc*)prevEntity)->setAngle2(arcFirst.getCenter().angleTo(v0));
						((RS_Arc*)prevEntity)->calculateBorders();
                        newPolyline->setNextBulge( ((RS_Arc*)prevEntity)->getBulge() );
                    } else {
                        ((RS_Line*)prevEntity)->setEndpoint(v0);
                        newPolyline->setNextBulge( 0.0 );
                    }
                    newPolyline->setEndpoint(v0);
                }
                if (currEntity->rtti()==RS2::EntityArc) {
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
					arc1.calculateBorders();
                    bulge = arc1.getBulge();
                } else
                    bulge = 0.0;
            }
            if (prevEntity) {
                newPolyline->addVertex(v, bulge, false);
                if (currEntity->rtti()==RS2::EntityArc){
                    arcFirst.setData(arc1.getData());
					arcFirst.calculateBorders();
                    prevEntity = &arcFirst;
                } else {
                    lineFirst.setStartpoint(line1.getStartpoint());
                    lineFirst.setEndpoint(line1.getEndpoint());
                    prevEntity = &lineFirst;
                }
            }
        }
        //properly terminated, check closed
		if (prevEntity && currEntity) {
            if (closed){
				if (currEntity->rtti()==RS2::EntityArc) {
                    arc1.setAngle2(arc1.getCenter().angleTo(newPolyline->getStartpoint()));
					arc1.calculateBorders();
                    newPolyline->setNextBulge(arc1.getBulge());
                    bulge = arc1.getBulge();
                }
                newPolyline->setClosed(true, bulge);
            } else {
                newPolyline->addVertex(currEntity->getEndpoint(), bulge);
            }
        }
        if (!newPolyline->isEmpty()) {
            container->addEntity(newPolyline);
			if (document) document->addUndoable(newPolyline);
        }
    }
	if (document) document->endUndoCycle();

	if (graphicView) {
        graphicView->redraw();
    }

    return true;
}

void RS_ActionPolylineEquidistant::trigger() {

        RS_DEBUG->print("RS_ActionPolylineEquidistant::trigger()");

		if (originalEntity && targetPoint->valid ) {

                originalEntity->setHighlighted(false);
                graphicView->drawEntity(originalEntity);

                makeContour();

				originalEntity = nullptr;
				*targetPoint = {};
                bRightSide = false;
                setStatus(ChooseEntity);

                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
        }
////////////////////////////////////////2006/06/15
                graphicView->redraw();
////////////////////////////////////////
}



void RS_ActionPolylineEquidistant::mouseReleaseEvent(QMouseEvent* e) {
        if (e->button()==Qt::LeftButton) {
                switch (getStatus()) {
                case ChooseEntity:
                        originalEntity = catchEntity(e);
						if (!originalEntity) {
                                RS_DIALOGFACTORY->commandMessage(tr("No Entity found."));
                        } else if (originalEntity->rtti()!=RS2::EntityPolyline) {

                                RS_DIALOGFACTORY->commandMessage(
                                        tr("Entity must be a polyline."));
                        } else {
								*targetPoint = snapFree(e);
                                originalEntity->setHighlighted(true);
                                graphicView->drawEntity(originalEntity);
                                double d = graphicView->toGraphDX(snapRange)*0.9;
								RS_Entity* Segment =  ((RS_Polyline*)originalEntity)->getNearestEntity( *targetPoint, &d, RS2::ResolveNone);
                                if (Segment->rtti() == RS2::EntityLine) {
                                double ang = ((RS_Line*)Segment)->getAngle1();
								double ang1 = ((RS_Line*)Segment)->getStartpoint().angleTo(*targetPoint);
                                if( ang > ang1 || ang + M_PI < ang1 )
                                        bRightSide = true;
                                } else {
                                    RS_Vector cen = ((RS_Arc*)Segment)->getCenter();
									if (cen.distanceTo(*targetPoint) > ((RS_Arc*)Segment)->getRadius() && ((RS_Arc*)Segment)->getBulge() > 0 )
                                        bRightSide = true;
                                }
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
				if (originalEntity) {
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
        graphicView->setMouseCursor(RS2::SelectCursor);
}

void RS_ActionPolylineEquidistant::updateMouseButtonHints() {
        switch (getStatus()) {
        case ChooseEntity:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Choose the original polyline"),
                                                                                        tr("Cancel"));
                break;
        default:
				RS_DIALOGFACTORY->updateMouseWidget();
                break;
        }
}

// EOF
