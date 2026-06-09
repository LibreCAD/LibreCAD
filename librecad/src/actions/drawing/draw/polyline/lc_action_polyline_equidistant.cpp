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

#include "lc_action_polyline_equidistant.h"

#include "lc_actioncontext.h"
#include "lc_polyline_equidistant_options_filler.h"
#include "lc_polyline_equidistant_options_widget.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_preview.h"

LC_ActionPolylineEquidistant::LC_ActionPolylineEquidistant(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionPolylineEquidistant", actionContext, RS2::ActionPolylineEquidistant), m_distance(1.),
      m_copiesNumber(1) {
}

LC_ActionPolylineEquidistant::~LC_ActionPolylineEquidistant() = default;

void LC_ActionPolylineEquidistant::doSaveOptions() {
    save("Dist", m_distance);
    save("Copies", m_copiesNumber);
}

void LC_ActionPolylineEquidistant::doLoadOptions() {
    m_distance = loadDouble("Dist", 1.0);
    m_copiesNumber = loadInt("Copies", 1);
}

void LC_ActionPolylineEquidistant::init(const int status) {
    m_originalEntity = nullptr;
    m_bRightSide = false;
    RS_PreviewActionInterface::init(status);
}

bool LC_ActionPolylineEquidistant::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_originalEntity != nullptr) {
        QList<RS_Polyline*> polylines;
        makeContour(m_originalEntity, m_bRightSide, polylines);

        for (RS_Polyline* newPolyline : std::as_const(polylines)) {
            ctx += newPolyline;
        }
        return true;
    }
    return false;
}

void LC_ActionPolylineEquidistant::doTriggerCompletion(const bool success) {
    if (success) {
        m_originalEntity = nullptr;
        m_bRightSide = false;
        setStatus(ChooseEntity);
    }
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
RS_Entity* LC_ActionPolylineEquidistant::calculateOffset(RS_Entity* newEntity, RS_Entity* orgEntity, const double distance) const {
    if (isArc(orgEntity) && isArc(newEntity)) {
        auto* arc = static_cast<RS_Arc*>(newEntity);
        const auto originalEntity = static_cast<RS_Arc*>(orgEntity);
        const double r0 = originalEntity->getRadius();
        double r;
        if (originalEntity->isReversed()) {
            r = r0 + distance;
        }
        else {
            r = r0 - distance;
        }
        if (r < 0) {
            return nullptr;
        }
        arc->setData(originalEntity->getData());
        arc->setRadius(r);
        arc->calculateBorders();
        return newEntity;
    }
    if (isLine(orgEntity) && isLine(newEntity)) {
        // fixme - support of polyline
        const auto* line0 = static_cast<RS_Line*>(orgEntity);
        auto* line1 = static_cast<RS_Line*>(newEntity);
        const RS_Vector v0 = line0->getStartpoint();
        const RS_Vector v1(v0.x, v0.y + distance);
        const RS_Vector v2(v0.x + line0->getLength(), v0.y + distance);
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
RS_Vector LC_ActionPolylineEquidistant::calculateIntersection(const RS_Entity* first, const RS_Entity* last) {
    RS_VectorSolutions vsol;
    vsol = RS_Information::getIntersection(first, last, false);

    if (vsol.isEmpty()) {
        //Parallel entities
        return RS_Vector(false);
    }
    const auto solPoint0 = vsol.get(0);
    const auto solPoint1 = vsol.get(1);
    if (vsol.getNumber() > 1 && solPoint0.distanceTo(last->getStartpoint()) > solPoint1.distanceTo(last->getStartpoint())) {
        return solPoint1;
    }
    return solPoint0;
}

void LC_ActionPolylineEquidistant::makeContour(const RS_Polyline* originalPolyline, bool contourOnRightSide,
                                               QList<RS_Polyline*>& createdPolylines) {
    //create a list of entities to offset without length = 0
    QList<RS_Entity*> entities;
    for (auto en : *originalPolyline) {
        if (en->getLength() > 1.0e-12) {
            entities.append(en);
        }
    }
    if (entities.isEmpty()) {
        return;
    }

    double neg = /*bRightSide*/ contourOnRightSide ? -1.0 : 1.0;

    // Create new helper entities
    const RS_Vector origin{0., 0.};
    RS_Line line1{origin, origin}; //current line
    RS_Line lineFirst{origin, origin}; //previous line
    RS_Arc arc1(nullptr, RS_ArcData(origin, 0, 0, 0, false)); //current arc
    RS_Arc arcFirst(nullptr, RS_ArcData(origin, 0, 0, 0, false)); //previous arc

    for (int num = 1; num <= m_copiesNumber || (m_copiesNumber == 0 && num <= 1); num++) {
        auto newPolyline = new RS_Polyline(m_document);

        bool first = true;
        bool closed = originalPolyline->isClosed();
        double bulge = 0.0;
        RS_Entity* prevEntity = entities.last();
        RS_Entity* currEntity = nullptr;
        for (auto en : entities) {
            RS_Vector v{false};
            if (isArc(en)) {
                currEntity = &arc1;
                calculateOffset(currEntity, en, m_distance * num * neg);
                bulge = arc1.getBulge();
            }
            else {
                currEntity = &line1;
                bulge = 0.0;
                calculateOffset(currEntity, en, m_distance * num * neg);
            }
            if (first) {
                if (closed) {
                    if (isArc(prevEntity)) {
                        prevEntity = calculateOffset(&arcFirst, prevEntity, m_distance * num * neg);
                    }
                    else {
                        prevEntity = calculateOffset(&lineFirst, prevEntity, m_distance * num * neg);
                    }
                    v = calculateIntersection(prevEntity, currEntity);
                }
                if (!v.valid) {
                    v = currEntity->getStartpoint();
                    closed = false;
                }
                else if (currEntity->rtti() == RS2::EntityArc) {
                    //update bulge
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
                    arc1.calculateBorders();
                    bulge = arc1.getBulge();
                }
                first = false;
                if (prevEntity == nullptr) {
                    break; //prevent crash if not exist offset for prevEntity
                }
            }
            else {
                v = calculateIntersection(prevEntity, currEntity);
                if (!v.valid) {
                    v = prevEntity->getEndpoint();
                    double dess = currEntity->getStartpoint().distanceTo(prevEntity->getEndpoint());
                    if (dess > 1.0e-12) {
                        newPolyline->addVertex(v, bulge);
                        prevEntity = nullptr;
                        break;
                    }
                }
                double startAngle = prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint());
                if (prevEntity->rtti() == RS2::EntityArc) {
                    arcFirst.setAngle2(arcFirst.getCenter().angleTo(v));
                    arcFirst.calculateBorders();
                    newPolyline->setNextBulge(arcFirst.getBulge());
                }
                //check if the entity are reverted
                if (fabs(remainder(prevEntity->getStartpoint().angleTo(prevEntity->getEndpoint()) - startAngle, 2. * M_PI)) > 0.785) {
                    prevEntity = newPolyline->lastEntity();
                    RS_Vector v0 = calculateIntersection(prevEntity, currEntity);
                    if (prevEntity->rtti() == RS2::EntityArc) {
                        auto arc = static_cast<RS_Arc*>(prevEntity);
                        arc->setAngle2(arcFirst.getCenter().angleTo(v0));
                        arc->calculateBorders();
                        newPolyline->setNextBulge(arc->getBulge());
                    }
                    else {
                        auto line = static_cast<RS_Line*>(prevEntity);
                        line->setEndpoint(v0);
                        newPolyline->setNextBulge(0.0);
                    }
                    newPolyline->setEndpoint(v0);
                }
                if (currEntity->rtti() == RS2::EntityArc) {
                    arc1.setAngle1(arc1.getCenter().angleTo(v));
                    arc1.calculateBorders();
                    bulge = arc1.getBulge();
                }
                else {
                    bulge = 0.0;
                }
            }
            if (prevEntity != nullptr) {
                newPolyline->addVertex(v, bulge, false);
                if (currEntity->rtti() == RS2::EntityArc) {
                    arcFirst.setData(arc1.getData());
                    arcFirst.calculateBorders();
                    prevEntity = &arcFirst;
                }
                else {
                    lineFirst.setStartpoint(line1.getStartpoint());
                    lineFirst.setEndpoint(line1.getEndpoint());
                    prevEntity = &lineFirst;
                }
            }
        }
        //properly terminated, check closed
        if ((prevEntity != nullptr) && (currEntity != nullptr)) {
            if (closed) {
                if (currEntity->rtti() == RS2::EntityArc) {
                    arc1.setAngle2(arc1.getCenter().angleTo(newPolyline->getStartpoint()));
                    arc1.calculateBorders();
                    newPolyline->setNextBulge(arc1.getBulge());
                    bulge = arc1.getBulge();
                }
                newPolyline->setClosed(true, bulge);
            }
            else {
                newPolyline->addVertex(currEntity->getEndpoint(), bulge);
            }
        }

        if (!newPolyline->isEmpty()) {
            createdPolylines << newPolyline;
        }
    }
}

bool LC_ActionPolylineEquidistant::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "spacing") {
        setDistance(distance);
        return true;
    }
    return false;
}

void LC_ActionPolylineEquidistant::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    deleteSnapper();
    if (status == ChooseEntity) {
        const auto en = catchAndDescribe(e, RS2::EntityPolyline);
        if (en != nullptr) {
            highlightHover(en);
            const auto polyline = static_cast<RS_Polyline*>(en);
            const RS_Vector coord = e->graphPoint;
            if (m_showRefEntitiesOnPreview) {
                const RS_Vector nearest = polyline->getNearestPointOnEntity(coord, true);
                previewRefPoint(nearest);
                previewRefLine(nearest, coord);
            }
            const bool pointOnRightSide = isPointOnRightSideOfPolyline(polyline, coord);
            QList<RS_Polyline*> polylines;
            makeContour(polyline, pointOnRightSide, polylines);

            for (RS_Polyline* newPolyline : std::as_const(polylines)) {
                newPolyline->reparent(m_preview.get());
                previewEntity(newPolyline);
            }
        }
    }
}

void LC_ActionPolylineEquidistant::setPolylineToModify(const LC_MouseEvent* e, RS_Entity* en) {
    if (en == nullptr) {
        commandMessage(tr("No Entity found."));
    }
    else if (en->rtti() != RS2::EntityPolyline) {
        commandMessage(tr("Entity must be a polyline."));
    }
    else {
        const auto polyline = dynamic_cast<RS_Polyline*>(en);
        const RS_Vector snapPoint = e->graphPoint;
        const bool pointOnRightSide = isPointOnRightSideOfPolyline(polyline, snapPoint);
        m_bRightSide = pointOnRightSide;
        m_originalEntity = polyline;
        trigger();
    }
}

void LC_ActionPolylineEquidistant::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseEntity: {
            RS_Entity* en = catchEntityByEvent(e);
            setPolylineToModify(e, en);
            break;
        }
        default:
            break;
    }
    invalidateSnapSpot();
}

void LC_ActionPolylineEquidistant::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deleteSnapper();
    if (m_originalEntity != nullptr) {
        redraw();
    }
    initPrevious(status);
}

bool LC_ActionPolylineEquidistant::isPointOnRightSideOfPolyline(const RS_Polyline* polyline, const RS_Vector& snapPoint) const {
    bool pointOnRightSide = false;
    double d = toGraphDX(m_catchEntityGuiRange) * 0.9;
    const auto segment = polyline->getNearestEntity(snapPoint, &d, RS2::ResolveNone);
    if (isLine(segment)) {
        const auto line = static_cast<RS_Line*>(segment);
        const double ang = line->getAngle1();
        const double ang1 = line->getStartpoint().angleTo(snapPoint);
        if (ang > ang1 || ang + M_PI < ang1) {
            pointOnRightSide = true;
        }
    }
    else if (isArc(segment)) {
        const auto arcSegment = static_cast<RS_Arc*>(segment);
        const RS_Vector cen = arcSegment->getCenter();
        if (cen.distanceTo(snapPoint) > arcSegment->getRadius() && arcSegment->getBulge() > 0) {
            pointOnRightSide = true;
        }
    }
    else {
        // fixme - sand - ELLIPTICIC SEGMENT!!!
    }
    return pointOnRightSide;
}

RS2::CursorType LC_ActionPolylineEquidistant::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

void LC_ActionPolylineEquidistant::updateActionPrompt() {
    switch (getStatus()) {
        case ChooseEntity:
            updatePromptTRCancel(tr("Choose the original polyline"));
            break;
        default:
            updatePrompt();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionPolylineEquidistant::createOptionsWidget() {
    return new LC_PolylineEquidistantOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionPolylineEquidistant::createOptionsFiller() {
    return new LC_PolylineEquidistantOptionsFiller();
}
