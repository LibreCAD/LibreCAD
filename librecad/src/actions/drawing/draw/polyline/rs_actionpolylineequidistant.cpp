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

#include "qg_polylineequidistantoptions.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionPolylineEquidistant::RS_ActionPolylineEquidistant(LC_ActionContext *actionContext)
	:RS_PreviewActionInterface("Create Equidistant Polylines", actionContext, RS2::ActionPolylineEquidistant)
	, m_dist(1.)
	,m_number(1){
}

RS_ActionPolylineEquidistant::~RS_ActionPolylineEquidistant()=default;

void RS_ActionPolylineEquidistant::init(int status){
    RS_PreviewActionInterface::init(status);
    m_originalEntity = nullptr;
    m_bRightSide = false;
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
RS_Entity *RS_ActionPolylineEquidistant::calculateOffset(RS_Entity *newEntity, RS_Entity *orgEntity, double distance){
    if (isArc(orgEntity) && isArc(newEntity)){
        auto *arc = static_cast<RS_Arc*>(newEntity);
        auto originalEntity = static_cast<RS_Arc*>(orgEntity);
        double r0 = originalEntity->getRadius();
        double r;
        if (originalEntity->isReversed())
            r = r0 + distance;
        else
            r = r0 - distance;
        if (r < 0)
            return nullptr;
        arc->setData(originalEntity->getData());
        arc->setRadius(r);
        arc->calculateBorders();
        return newEntity;
    } else if (isLine(orgEntity) && isLine(newEntity)){
        auto *line0 = static_cast<RS_Line*>(orgEntity);
        auto *line1 = static_cast<RS_Line*>(newEntity);
        RS_Vector v0 = line0->getStartpoint();
        RS_Vector v1(v0.x, v0.y + distance);
        RS_Vector v2(v0.x + line0->getLength(), v0.y + distance);
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

void RS_ActionPolylineEquidistant::makeContour(RS_Polyline*  originalPolyline, bool contourOnRightSide, QList<RS_Polyline*> &createdPolylines){
    if (!m_container){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_ActionPolylineEquidistant::makeContour: no valid container");
    }

    //create a list of entities to offset without length = 0
    QList<RS_Entity *> entities;
    for (auto en: *originalPolyline) {
        if (en->getLength() > 1.0e-12)
            entities.append(en);
    }
    if (entities.isEmpty()){
        return;
    }

    double neg = /*bRightSide*/ contourOnRightSide ? -1.0 : 1.0;

    // Create new helper entities
    RS_Vector const origin{0., 0.};
    RS_Line line1{origin, origin};//current line
    RS_Line lineFirst{origin, origin};//previous line
    RS_Arc arc1(nullptr, RS_ArcData(origin, 0, 0, 0, false));//current arc
    RS_Arc arcFirst(nullptr, RS_ArcData(origin, 0, 0, 0, false));//previous arc

    for (int num = 1; num <= m_number || (m_number == 0 && num <= 1); num++) {
        auto newPolyline = new RS_Polyline(m_container);

        bool first = true;
        bool closed = originalPolyline->isClosed();
        double bulge = 0.0;
        RS_Entity *prevEntity = entities.last();
        RS_Entity *currEntity = nullptr;
        for (auto en : entities) {
            RS_Vector v{false};
            if (isArc(en)){
                currEntity = &arc1;
                calculateOffset(currEntity, en, m_dist * num * neg);
                bulge = arc1.getBulge();
            } else {
                currEntity = &line1;
                bulge = 0.0;
                calculateOffset(currEntity, en, m_dist * num * neg);
            }
            if (first){
                if (closed){
                    if (isArc(prevEntity)){
                        prevEntity = calculateOffset(&arcFirst, prevEntity, m_dist * num * neg);
                    } else {
                        prevEntity = calculateOffset(&lineFirst, prevEntity, m_dist * num * neg);
                    }
                    v = calculateIntersection(prevEntity, currEntity);
                }
                if (!v.valid){
                    v = currEntity->getStartpoint();
                    closed = false;
                } else if (currEntity->rtti() == RS2::EntityArc){
                    //update bulge
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
                    arc1.calculateBorders();
                    bulge = arc1.getBulge();
                }
                first = false;
                if (!prevEntity) break; //prevent crash if not exist offset for prevEntity
            } else {
                v = calculateIntersection(prevEntity, currEntity);
                if (!v.valid){
                    v = prevEntity->getEndpoint();
                    double dess = currEntity->getStartpoint().distanceTo(prevEntity->getEndpoint());
                    if (dess > 1.0e-12){
                        newPolyline->addVertex(v, bulge);
                        prevEntity = nullptr;
                        break;
                    }
                }
                double startAngle = prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint());
                if (prevEntity->rtti() == RS2::EntityArc){
                    arcFirst.setAngle2(arcFirst.getCenter().angleTo(v));
                    arcFirst.calculateBorders();
                    newPolyline->setNextBulge(arcFirst.getBulge());
                }
                //check if the entity are reverted
                if (fabs(remainder(prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint()) - startAngle, 2. * M_PI)) > 0.785){
                    prevEntity = newPolyline->lastEntity();
                    RS_Vector v0 = calculateIntersection(prevEntity, currEntity);
                    if (prevEntity->rtti() == RS2::EntityArc){
                        auto arc = static_cast<RS_Arc*>(prevEntity);
                        arc->setAngle2(arcFirst.getCenter().angleTo(v0));
                        arc->calculateBorders();
                        newPolyline->setNextBulge(arc->getBulge());
                    } else {
                        auto line = static_cast<RS_Line*>(prevEntity);
                        line->setEndpoint(v0);
                        newPolyline->setNextBulge(0.0);
                    }
                    newPolyline->setEndpoint(v0);
                }
                if (currEntity->rtti() == RS2::EntityArc){
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
                    arc1.calculateBorders();
                    bulge = arc1.getBulge();
                } else
                    bulge = 0.0;
            }
            if (prevEntity != nullptr){
                newPolyline->addVertex(v, bulge, false);
                if (currEntity->rtti() == RS2::EntityArc){
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
        if (prevEntity && currEntity){
            if (closed){
                if (currEntity->rtti() == RS2::EntityArc){
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

        if (!newPolyline->isEmpty()){
            createdPolylines<<newPolyline;
        }
    }
}

void RS_ActionPolylineEquidistant::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineEquidistant::trigger()");
    if (m_originalEntity != nullptr) {
        if (m_document != nullptr){
            undoCycleStart();

            QList<RS_Polyline *> polylines;
            makeContour(m_originalEntity, m_bRightSide, polylines);

            for (RS_Polyline *newPolyline: polylines) {
                newPolyline->setLayerToActive(); // fixme - cache layer to set
                m_container->addEntity(newPolyline);
                undoableAdd(newPolyline);
            }
            undoCycleEnd();
        }

        m_originalEntity = nullptr;
        m_bRightSide = false;
        setStatus(ChooseEntity);
    }
}

void RS_ActionPolylineEquidistant::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    deleteSnapper();
    if (status == ChooseEntity){
        auto en = catchAndDescribe(e, RS2::EntityPolyline);
        if (en != nullptr){
            highlightHover(en);
            auto polyline = dynamic_cast<RS_Polyline *>(en);
            RS_Vector coord = e->graphPoint;
            if (m_showRefEntitiesOnPreview) {
                RS_Vector nearest = polyline->getNearestPointOnEntity(coord, true);
                previewRefPoint(nearest);
                previewRefLine(nearest, coord);
            };
            bool pointOnRightSide = isPointOnRightSideOfPolyline(polyline, coord);
            QList<RS_Polyline *> polylines;
            makeContour(polyline, pointOnRightSide, polylines);

            for (RS_Polyline *newPolyline: polylines) {
               newPolyline->reparent(m_preview.get());
               previewEntity(newPolyline);
            }
        }
    }
}

void RS_ActionPolylineEquidistant::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseEntity:{
            RS_Entity *en = catchEntityByEvent(e);
            if (!en){
                commandMessage(tr("No Entity found."));
            } else if (en->rtti() != RS2::EntityPolyline){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                auto polyline = dynamic_cast<RS_Polyline *>(en);
                RS_Vector snapPoint = e->graphPoint;
                bool pointOnRightSide = isPointOnRightSideOfPolyline(polyline, snapPoint);
                m_bRightSide = pointOnRightSide;
                m_originalEntity = polyline;
                trigger();
            }
            break;
        }
        default:
            break;
    }
    invalidateSnapSpot();
}

void RS_ActionPolylineEquidistant::onMouseRightButtonRelease(int status, [[maybe_unused]]  LC_MouseEvent *e) {
    deleteSnapper();
    if (m_originalEntity){
        redraw();
    }
    initPrevious(status);
}

bool RS_ActionPolylineEquidistant::isPointOnRightSideOfPolyline(const RS_Polyline *polyline, const RS_Vector &snapPoint) const{
    bool pointOnRightSide = false;
    double d = toGraphDX(m_catchEntityGuiRange) * 0.9;
    auto segment = polyline->getNearestEntity(snapPoint, &d, RS2::ResolveNone);
    if (isLine(segment)){
        auto line = dynamic_cast<RS_Line *>(segment);
        double ang = line->getAngle1();
        double ang1 = line->getStartpoint().angleTo(snapPoint);
        if (ang > ang1 || ang + M_PI < ang1)
            pointOnRightSide = true;
    } else {
        RS_Vector cen = ((RS_Arc *) segment)->getCenter();
        if (cen.distanceTo(snapPoint) > ((RS_Arc *) segment)->getRadius() && ((RS_Arc *) segment)->getBulge() > 0)
            pointOnRightSide = true;
    }
    return pointOnRightSide;
}
RS2::CursorType RS_ActionPolylineEquidistant::doGetMouseCursor([[maybe_unused]] int status){
   return RS2::SelectCursor;
}

void RS_ActionPolylineEquidistant::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseEntity:
            updateMouseWidgetTRCancel(tr("Choose the original polyline"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget* RS_ActionPolylineEquidistant::createOptionsWidget(){
    return new QG_PolylineEquidistantOptions();
}
