/****************************************************************************
**
* Action that breaks line, arc or circle to segments by points of
* intersection with other entities.

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
#include <cfloat>
#include <QMouseEvent>

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "lc_actionmodifybreakdivide.h"
#include "lc_linemath.h"
#include "lc_modifybreakdivideoptions.h"


namespace {
    //list of entity types supported by current action - line, arc, circle
    const auto enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle/*,RS2::EntityEllipse*/};
}

LC_ActionModifyBreakDivide::LC_ActionModifyBreakDivide(RS_EntityContainer &container, RS_GraphicView &graphicView)
   :LC_AbstractActionWithPreview("Break Out",
                                 container,
                                 graphicView){
    actionType = RS2::ActionModifyBreakDivide;
}

bool LC_ActionModifyBreakDivide::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status == SetLine;
}

/**
 * Creating preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionModifyBreakDivide::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SetLine){
        RS_Entity *en = catchModifiableEntity(e, enTypeList);
        if (en != nullptr){
            int rtti = en->rtti();
            switch (rtti) {
                case RS2::EntityLine: { // process line
                    auto *line = dynamic_cast<RS_Line *>(en);
                    createEntitiesForLine(line, snap, list, true);
                    break;
                }
                case RS2::EntityCircle: { //process circle
                    auto *circle = dynamic_cast<RS_Circle *>(en);
                    createEntitiesForCircle(circle, snap, list, true);
                    break;
                }
                case RS2::EntityArc: { // process arc
                    auto *arc = dynamic_cast<RS_Arc *>(en);
                    createEntitiesForArc(arc, snap, list, true);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void LC_ActionModifyBreakDivide::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
    if (status == SetLine){
        RS_Entity *en = catchModifiableEntity(e, enTypeList);
        if (en != nullptr){
            int rtti = en->rtti();
            switch (rtti) {
                case RS2::EntityLine:
                case RS2::EntityCircle:
                case RS2::EntityArc:
                    // store information about entity and snap point and pass to trigger()
                    triggerData = new TriggerData();
                    triggerData->entity = en;
                    triggerData->snapPoint = snapPoint;
                    trigger();
                    break;
                default:
                    break;
            }
        }
    }
}

bool LC_ActionModifyBreakDivide::doCheckMayTrigger(){
    return triggerData != nullptr;
}

bool LC_ActionModifyBreakDivide::isSetActivePenAndLayerOnTrigger(){
    return false; // action will handle attributes
}

void LC_ActionModifyBreakDivide::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Entity* en = triggerData->entity;
    RS_Vector snap = triggerData->snapPoint;
    if (en != nullptr){
        int rtti = en->rtti();
        // do processing of individual entity types
        switch (rtti) {
            case RS2::EntityLine: {
                auto *line = dynamic_cast<RS_Line *>(en);
                createEntitiesForLine(line, snap, list, false);
                break;
            }
            case RS2::EntityCircle: {
                auto *circle = dynamic_cast<RS_Circle *>(en);
                createEntitiesForCircle(circle, snap, list, false);
                break;
            }
            case RS2::EntityArc: {
                auto *arc = dynamic_cast<RS_Arc *>(en);
                createEntitiesForArc(arc, snap, list, false);
                break;
            }
            default:
                break;
        }
    }
}

void LC_ActionModifyBreakDivide::performTriggerDeletions(){
    // delete original entity as we'll expand it and create segment entities
    deleteEntityUndoable(triggerData->entity);
}

void LC_ActionModifyBreakDivide::doAfterTrigger(){
    delete triggerData;
    triggerData = nullptr;
}

void LC_ActionModifyBreakDivide::doFinish([[maybe_unused]]bool updateTB){
    if (triggerData != nullptr){
        delete triggerData;
        triggerData = nullptr;
    }
}

/**
 * creating segment entities for preview or trigger for line entity
 * @param line selected line
 * @param snap snap point
 * @param list list to which entities should be added
 * @param preview true if entities for preview
 */
void LC_ActionModifyBreakDivide::createEntitiesForLine(RS_Line *line, RS_Vector &snap, QList<RS_Entity*> &list, bool preview){
    // check whether selection entity may be expanded
    if (checkMayExpandEntity(line, "")){
        // determine snap point projection on line
        RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, false);
        RS_Vector start = line->getStartpoint();
        RS_Vector end = line->getEndpoint();


        // create segments only if tick snap point is between of original lines endpoints
        if (nearestPoint != start && nearestPoint != end){
            // calculate segments data
            LineSegmentData *data = calculateLineSegment(line, snap);
            if (data != nullptr){

                if (preview){
                    highlightHover(line);
                }

                // determine which segments should be created
                bool createSnapSegment = !preview;
                bool createNonSnapSegments = !preview;

                if (removeSegments){
                    if (preview){
                        createSnapSegment = removeSelected;
                        createNonSnapSegments = !removeSelected;
                    }
                    else{
                        createSnapSegment = !removeSelected;
                        createNonSnapSegments = removeSelected;
                    }
                }

                // attributes of original entity
                RS_Pen pen = line->getPen();
                RS_Layer* layer = line->getLayer(true);

                // creating snap segment (where snap was performed)
                if (createSnapSegment){
                    createLineEntity(preview, data->snapSegmentStart, data->snapSegmentEnd, pen, layer, list);
                }
                int segmentDisposition = data->segmentDisposition;

                // in preview mode provide visual indication of division/break points
                if (preview){
                    // check that start of segment is not line endpoint
                    if (segmentDisposition != SEGMENT_TO_START){
                        createRefSelectablePoint(data->snapSegmentStart, list);
                    }

                    // check that end of segment is not line endpoint
                    if (segmentDisposition != SEGMENT_TO_END){
                        createRefSelectablePoint(data->snapSegmentEnd, list);
                    }
                }

                // create segments of line that are outsider of segment selected by the user
                if (createNonSnapSegments){
                    if (segmentDisposition != SEGMENT_TO_START){
                        // we don't need this segment if snap is between line start point and intersection point
                        createLineEntity(preview, line->getStartpoint(), data->snapSegmentStart, pen, layer, list);
                    }
                    if (segmentDisposition != SEGMENT_TO_END){
                        // we don't need this segment if snap is between line end point and intersection point
                        createLineEntity(preview, data->snapSegmentEnd, line->getEndpoint(), pen, layer, list);
                    }
                }
            }
            // don't need temporary data we used anymore, so delete it
            delete data;
        }
    }
}
/**
 * Creates individual line for given coordinates. If not for preview, also assigns provided pen and layer attributes
 * @param preview
 * @param start
 * @param end
 * @param pen
 * @param layer
 * @param list
 */
void LC_ActionModifyBreakDivide::createLineEntity(bool preview, const RS_Vector &start, const RS_Vector &end,
                                                  const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const{
    if (preview){
        createRefLine(start, end, list);
        createLine(start, end, list);
    }
    else{
        auto *createdLine = createLine(start, end, list);
        createdLine->setPen(pen);
        createdLine->setLayer(layer);
    }
}

/**
 * Creates arc segment entities for provided circle and snap point
 * @param circle
 * @param snap
 * @param list
 * @param preview
 */
void LC_ActionModifyBreakDivide::createEntitiesForCircle(RS_Circle *circle, RS_Vector &snap, QList<RS_Entity *> &list, bool preview){
    // check that we may expand the circle
    if (checkMayExpandEntity(circle, "")){
        // determine snap point projection on entity
        RS_Vector nearestPoint = circle->getNearestPointOnEntity(snap, true);

        // compute segment data
        CircleSegmentData *data = calculateCircleSegment(circle, nearestPoint);
        if (data != nullptr){

            if (preview){
                highlightHover(circle);
            }

            const RS_Vector &center = circle->getCenter();
            double radius = circle->getRadius();

            // basic arc info
            RS_ArcData arcData;
            arcData.radius = radius;
            arcData.center = center;
            arcData.angle1 = data->snapSegmentStartAngle;
            arcData.angle2 = data->snapSegmentEndAngle;
            arcData.reversed = false;

            // determine which segments should be created of circle
            bool createSnapSegment = !preview;
            bool createNonSnapSegments = !preview;

            if (removeSegments){
                if (preview){
                    createSnapSegment = removeSelected;
                    createNonSnapSegments = !removeSelected;
                }
                else{
                    createSnapSegment = !removeSelected;
                    createNonSnapSegments = removeSelected;
                }
            }

            // attributes of original circle
            RS_Pen pen = circle->getPen();
            RS_Layer* layer = circle->getLayer();

            // create snap arc segment
            if (createSnapSegment){
                createArcEntity(arcData, preview, pen, layer, list);
            }

            // create complimentary non-snap arc segment, if needed
            if (createNonSnapSegments){
                RS_ArcData arcData1 = arcData;
                arcData1.reversed = !arcData.reversed; // that ark wil be in same points, yet reversed
                createArcEntity(arcData1, preview, pen, layer, list);
             }

            // for circle divide mode, add visual indication of divide points if we're creating preview
            if (preview){
                // todo - ignore refpoints visibility for divide?
                 RS_Vector dividePoint1 = center.relative(radius, data->snapSegmentStartAngle);
                 createRefSelectablePoint(dividePoint1, list);

                 RS_Vector dividePoint2 = center.relative(radius, data->snapSegmentEndAngle);
                 createRefSelectablePoint(dividePoint2, list);

            }
        }
        // don't need temporary data, so delete it
        delete data;
    }
}
/**
 * Creates segment entities for provided arc
 * @param arc original arc
 * @param snap snap point (where the user selected arc)
 * @param list list of entities to add
 * @param preview true if we generate entities for preview, false otherwise
 */
void LC_ActionModifyBreakDivide::createEntitiesForArc(RS_Arc *arc, RS_Vector &snap, QList<RS_Entity *> &list, bool preview){
    // check that arc is expandable
    if (checkMayExpandEntity(arc, "")){
        // determine snap point
        RS_Vector nearestPoint = arc->getNearestPointOnEntity(snap, true);        
        RS_Vector start = arc->getStartpoint();
        RS_Vector end = arc->getEndpoint();

        // create segments only if tick snap point is between of original lines endpoints
        if (nearestPoint != start && nearestPoint != end){
            // determine snap segment coordinates
            ArcSegmentData *data = calculateArcSegments(arc, snap);
            if (data != nullptr){
                if (preview){
                    highlightHover(arc);
                }

                // determine which segment entities should be created
                bool createSnapSegment = !preview;
                bool createNonSnapSegments = !preview;

                if (removeSegments){
                    if (preview){
                        createSnapSegment = removeSelected;
                        createNonSnapSegments = !removeSelected;
                    } else {
                        createSnapSegment = !removeSelected;
                        createNonSnapSegments = removeSelected;
                    }
                }

                // current arc attributes
                RS_Pen pen = arc->getPen();
                RS_Layer* layer = arc->getLayer(true);

                // create segment where arc was selected, if necessary
                if (createSnapSegment){
                    RS_ArcData arcData = arc->getData();
                    arcData.angle1 = data->snapSegmentStartAngle;
                    arcData.angle2 = data->snapSegmentEndAngle;
                    createArcEntity(arcData, preview, pen, layer, list);
                }

                // for preview and arc break/divide mode, add points that highlights places where arc will be divided/broken
                int segmentDisposition = data->segmentDisposition;

                // create non-snap segments, if necessary
                if (createNonSnapSegments){
                    if (segmentDisposition != SEGMENT_TO_START){
                        RS_ArcData arcData1 = arc->getData();
                        arcData1.angle1 = arc->getAngle1();
                        arcData1.angle2 = data->snapSegmentStartAngle;
                        createArcEntity(arcData1, preview, pen, layer, list);
                    }

                    if (segmentDisposition != SEGMENT_TO_END){
                        RS_ArcData arcData2 = arc->getData();
                        arcData2.angle1 = data->snapSegmentEndAngle;
                        arcData2.angle2 = arc->getAngle2();
                        createArcEntity(arcData2, preview, pen, layer, list);
                    }
                }

                if (preview){
                    double radius = arc->getRadius();
                    RS_Vector center = arc->getCenter();
                    if (segmentDisposition != SEGMENT_TO_START){
                        RS_Vector segmentStart = center.relative(radius, data->snapSegmentStartAngle);
                        createRefSelectablePoint(segmentStart, list);
                    }

                    if (segmentDisposition != SEGMENT_TO_END){
                        RS_Vector segmentEnd = center.relative(radius, data->snapSegmentEndAngle);
                        createRefSelectablePoint(segmentEnd, list);
                    }
                }
            }
            // don't need temporary data, so delete it
            delete data;
        }
    }
}

/**
 * Utility method that creates arc entity. If is not for preview, also applies provided attributes to  created entity
 * @param arcData data that describes arc
 * @param preview true if entity is created for preview
 * @param pen pen
 * @param layer layer
 * @param list list of entities to add created entity
 */
void LC_ActionModifyBreakDivide::createArcEntity(const RS_ArcData &arcData, bool preview, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const{
    if (preview){
        createRefArc(arcData, list);
        auto arc = new RS_Arc(container, arcData);
        list << arc;
    }
    else{
        auto createdArc = new RS_Arc(container, arcData);
        createdArc->setPen(pen);
        createdArc->setLayer(layer);
        list << createdArc;
    }

}

/**
 * For selection of entities, we'll rely on free snap mode, so point of selection is not affected by intersection points or endpoints.
 * @param e
 * @return
 */
RS_Vector LC_ActionModifyBreakDivide::doGetMouseSnapPoint(QMouseEvent *e){
    snapPoint(e);
    RS_Vector result = toGraph(e);
    return result;
}

/**
 * determines segment of line selected by the user
 * @param line line
 * @param snap snap point
 * @return segment information
 */
LC_ActionModifyBreakDivide::LineSegmentData *LC_ActionModifyBreakDivide::calculateLineSegment(RS_Line *line, RS_Vector &snap){
    LineSegmentData *result = nullptr;
    // find all intersection points for line
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(line);
    if (!allIntersections.empty()){
        // determine segments of line that was selected by the user
        result = findLineSegmentEdges(line, snap, allIntersections);
    }
    return result;
}

/**
 * determines segment of arc selected by the user
 * @param arc arc
 * @param snap snap point
 * @return segment information
 */
LC_ActionModifyBreakDivide::ArcSegmentData *LC_ActionModifyBreakDivide::calculateArcSegments(RS_Arc *arc, RS_Vector &snap){
    ArcSegmentData *result = nullptr;
    // detect all intersections
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(arc);
    if (!allIntersections.empty()){
        // determine selected segment edges
        result = findArcSegmentEdges(arc, snap, allIntersections);
    }
    return result;
}

/**
 * determines segment of circle selected by the user
 * @param circle circle
 * @param snap snap point
 * @return segment information
 */
LC_ActionModifyBreakDivide::CircleSegmentData *LC_ActionModifyBreakDivide::calculateCircleSegment(RS_Circle *circle, RS_Vector &snap){
    CircleSegmentData *result = nullptr;
    // detect all intersections
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(circle);
    if (!allIntersections.empty()){
        // determine selected segment edges
        result = findCircleSegmentEdges(circle, snap, allIntersections);
    }
    return result;
}
/**
 * Collects all intersection of given entity with other entities.
 * @param entity entity to check for intersections
 * @return vector of intersection points
 */
QVector<RS_Vector> LC_ActionModifyBreakDivide::collectAllIntersectionsWithEntity(RS_Entity *entity){
    QVector<RS_Vector> result;
    RS_VectorSolutions sol;
    // iterate over all entities
    for (auto* e: *container) {
        // consider only visible entities
        if (e && e->isVisible()){
            // select containers / groups:
            if (e->isContainer()){
                // additional handling for containers
                auto *ec = (RS_EntityContainer *) e;

                for (RS_Entity *e2 = ec->firstEntity(RS2::ResolveAll); e2;
                     e2 = ec->nextEntity(RS2::ResolveAll)) {
                    sol = RS_Information::getIntersection(entity, e2, true);
                    addPointsFromSolutionToList(sol, result);
                }
            } else {
                // just find intersections between entities
                sol = RS_Information::getIntersection(entity, e, true);
                // and collect them
                addPointsFromSolutionToList(sol, result);
            }
        }
    }
    return result;
}
/**
 * Utility method that collects all valid intersection points for vector solutions to vector
 * @param sol vector solutions with intersections
 * @param result resulting vector
 */
void LC_ActionModifyBreakDivide::addPointsFromSolutionToList(RS_VectorSolutions &sol, QVector<RS_Vector> &result) const{
    if (sol.hasValid()){
        size_t size = sol.size();
        for (size_t i = 0; i < size; i++) {
            const RS_Vector point = sol.at(i);
            if (point.valid){
                result.append(point);
            }
        }
    }
}

/**
 * Method finds edges (start and end point) for the segment of line, selected by the user.
 * Segment should contain snap point and it is limited either by intersection points or
 * intersection point and one of line's endpoints
 * @param line line
 * @param snap point where the user selected the line
 * @param intersections all intersections
 * @return information about line segment
 */
LC_ActionModifyBreakDivide::LineSegmentData *LC_ActionModifyBreakDivide::findLineSegmentEdges(RS_Line *line, RS_Vector &snap, QVector<RS_Vector> intersections){
    double angle = line->getAngle1();
    RS_Vector lineStartPoint = line->getStartpoint();
    RS_Vector lineEndPoint = line->getEndpoint();

    // rotate all intersection over start point of line, so we can check only x coordinates

    int intersectionsCount = intersections.size();
    for (int i = 0; i < intersectionsCount; i++) {
        RS_Vector v = intersections.at(i);
        if (LC_LineMath::isNotMeaningfulDistance(v, lineStartPoint) ||
            LC_LineMath::isNotMeaningfulDistance(v,lineEndPoint)) {
            // it means that intersection is in one of edge points of the line,
            // so we'll skip this intersection for further processing
            intersections.replace(i, RS_Vector(false));
        }
        else {
            // just rotate the intersection point over start of line
            v.rotate(lineStartPoint, -angle);
            intersections.replace(i, v);
        }
    }

    // distances used for finding nearest point to snap - with defaults
    double maxXLeft = -DBL_MAX;
    double minXRight = DBL_MAX;

    // points that are closest to snap point
    RS_Vector nearestLeft(false);
    RS_Vector nearestRight(false);

    // rotate snap point
    RS_Vector rotatedSnap = snap;
    rotatedSnap.rotate(lineStartPoint, -angle);

    RS_Vector rotatedEndPoint = lineEndPoint;
    rotatedEndPoint.rotate(lineStartPoint, -angle);

    double snapX = rotatedSnap.x;

    // flag to check whether there are intersections on the line (not in edges)
    bool hasNonEdgesIntersection = false;

    // iterate over all intersection points
    for (int i = 0; i < intersectionsCount; i++) {
        RS_Vector v = intersections.at(i);
        if (v.valid){
            // this is not edge intersection
            hasNonEdgesIntersection = true;
        }
        else{
            // edge intersection
            continue;
        }
        // position of intersection point on x-axis
        double vX = v.x;
        if (vX <= snapX){ // intersection in on left side from snap
            if (vX >= maxXLeft){
                // and it's closer than previous to snap
                maxXLeft = vX;
                nearestLeft = v;
            }
        } else { // intersection on right of snap
            if (vX < minXRight){
                // and it is closer to snap than previously processed
                minXRight = vX;
                nearestRight = v;
            }
        }
    }

    LineSegmentData* result = nullptr;
    if (hasNonEdgesIntersection){
        // check how line is directed
        bool startOnLeft = rotatedEndPoint.x > lineStartPoint.x;

        result = new LineSegmentData();
        result->segmentDisposition = SEGMENT_INSIDE;

        if (nearestLeft.valid){
            // we found intersection point on the left side of snap, so use it
            // restore coordinate
            nearestLeft.rotate(lineStartPoint, angle);
            // based on direction of line, it will be either start or end point of segment
            if (startOnLeft){
                result->snapSegmentStart = nearestLeft;
            } else {
                result->snapSegmentEnd = nearestLeft;
            }
        } else {
            // no intersection between snap point and edge point of line
            if (startOnLeft){
                // selected segment is from start point to intersection point
                result->segmentDisposition = SEGMENT_TO_START;
                result->snapSegmentStart = lineStartPoint;
            } else {
                // selected segment is from end point to intersection point
                result->segmentDisposition = SEGMENT_TO_END;
                result->snapSegmentEnd = lineEndPoint;
            }
        }

        if (nearestRight.valid){
            // we found that there is intersection point between snap and line edge point on right side of line
            // restore intersection position
            nearestRight.rotate(lineStartPoint, angle);
            // base on direction of line, set segment point
            if (startOnLeft){
                result->snapSegmentEnd = nearestRight;
            } else {
                result->snapSegmentStart = nearestRight;
            }
        } else {
            // no intersection between snap point and edge point found
            if (startOnLeft){
                // segment is from intersection point to end point of line
                result->segmentDisposition = SEGMENT_TO_END;
                result->snapSegmentEnd = lineEndPoint;
            } else {
                // selected segment is from intersection point to start point of line
                result->segmentDisposition = SEGMENT_TO_START;
                result->snapSegmentStart = lineStartPoint;
            }
        }
    }
    return result;
}
/**
 * Determines coordinates of selected segment for arc
 * @param arc arc
 * @param snap point where the user selected arc
 * @param intersections  all intersection points for arc
 * @return segment data
 */
LC_ActionModifyBreakDivide::ArcSegmentData *LC_ActionModifyBreakDivide::findArcSegmentEdges(RS_Arc *arc, RS_Vector &snap, const QVector<RS_Vector>& intersections){

    double arcStartAngle = arc->getAngle1();
    double arcEndAngle = arc->getAngle2();
    double radius = arc->getRadius();
    bool reversed = arc->isReversed();

    if (reversed){
        // for uniform processing of arcs - reverse angles
        arcStartAngle = arcEndAngle;
        arcEndAngle = arc->getAngle1();
    }
    const RS_Vector &center = arc->getCenter();
    double snapAngle = center.angleTo(snap);

    // angles of intersections nearest to snap
    double nearestLeft = -M_PI;
    double nearestRight = 3 * M_PI;

    int intersectionsCount = intersections.size();

    // we'll rotate angle in such way that start angle of arc becomes 0
    double correctedSnapAngle = RS_Math::correctAngle(snapAngle - arcStartAngle);
    double correctedEndAngle = RS_Math::correctAngle(arcEndAngle - arcStartAngle);

    double minLeft = 0;
    double maxRight = 2 * M_PI;

    // coordinates for arc edges
    RS_Vector arcStartPoint = LC_LineMath::findPointOnCircle(radius, arcStartAngle, center);
    RS_Vector arcEndPoint = LC_LineMath::findPointOnCircle(radius, arcEndAngle, center);

    bool hasNonEdgeIntersections = false;

    for (int i = 0; i < intersectionsCount; i++) {
        RS_Vector v = intersections.at(i);
        if (LC_LineMath::isNotMeaningfulDistance(v, arcStartPoint) ||
            LC_LineMath::isNotMeaningfulDistance(v,arcEndPoint)) {
            // this is intersection in edge, skip it
            continue;
        }
        else{
            hasNonEdgeIntersections = true;
        }

        double angleToIntersection = center.angleTo(v);

        // correct angle of
        double vA = RS_Math::correctAngle(angleToIntersection - arcStartAngle);

        if (vA <= correctedSnapAngle){ // intersection in on left from snap
            if (vA >= minLeft){
                nearestLeft = angleToIntersection;
                minLeft = vA;
            }
        } else if (vA < correctedEndAngle){ // intersection on right of snap
            if (vA < maxRight){
                nearestRight = angleToIntersection;
                maxRight = vA;
            }
        }
    }

    ArcSegmentData *result = nullptr;
    if (hasNonEdgeIntersections){
        result = new ArcSegmentData();
        result->segmentDisposition = SEGMENT_INSIDE;


        if (nearestLeft != -M_PI){
            // left intersection from snap found
            result->snapSegmentStartAngle = nearestLeft;
        } else {
            // no intersection till from snap to end point
            if (reversed){
                result->segmentDisposition = SEGMENT_TO_END;
            }
            else {
                result->segmentDisposition = SEGMENT_TO_START;
            }
            result->snapSegmentStartAngle = arcStartAngle;
        }

        if (nearestRight != 3 * M_PI){
            // right intersection is found
           result->snapSegmentEndAngle = nearestRight;

        } else {
            // no intersection till from snap to end point
            if (reversed){
                result->segmentDisposition = SEGMENT_TO_START;
            } else {
                result->segmentDisposition = SEGMENT_TO_END;
             }
            result->snapSegmentEndAngle = arcEndAngle;
        }

        if (reversed){
            std::swap(result->snapSegmentEndAngle, result->snapSegmentStartAngle);
        }

    }
    return result;
}
/**
 * Finds selected segment for circle entity. Is is expected that at least 2 intersection point on circle should be present.
 *
 * @param circle circle
 * @param snap selection point
 * @param intersections intersection points
 * @return segment information
 */
LC_ActionModifyBreakDivide::CircleSegmentData* LC_ActionModifyBreakDivide::findCircleSegmentEdges(RS_Circle *circle, RS_Vector &snap,
                                                                                                  const QVector<RS_Vector> &intersections){

    CircleSegmentData *result = nullptr;
    int intersectionsCount = intersections.size();

    // for circle, we need at least 2 intersection points for properly divide it.
    // potentially, with 1 intersection it is possible to convert the circle into
    // arc that starts and ends in the intersection point, however, from practical point of
    // view, such a features seems to be more than doubtful...
    if (intersectionsCount >= 2){
        const RS_Vector &center = circle->getCenter();
        double snapAngle = center.angleTo(snap);
        double leftAngle = 0;
        double rightAngle = 0;
        double maxRight = M_PI*3;
        double minLeft = -M_PI*3;

        // first pass - assumes that we're in inner segment between intersection points,
        // so there is one intersection is above snap (top half of circle) and one - below snap (bottom half of
        // circle) - considering that we rotate circle in such way that snap point is on 0 angle.
        for (int i = 0; i < intersectionsCount; i++) {
            RS_Vector v = intersections.at(i);
            double angleToIntersection = center.angleTo(v);

            // use corrected angle and thus actually we've rotated intersection point over center of circle
            // to -snap angle (so snap is in zero angle). We used this to divide the entire circle on
            // "left" and "right" arcs, so intersections below snap will be at left, above angle - at right
            // part of circle
            // Also, we do correction of angle difference, to ensure that left angles are less than 0,
            // right angles are more than 0.
            // With such approach, we detect boundaries of snap segment by finding two intersections points
            // from each part of circle with minimal angle to snap point.
            double vA = RS_Math::correctAnglePlusMinusPi(angleToIntersection - snapAngle);

            bool left = vA < 0;

            if (left){
                if (vA > minLeft){
                    minLeft = vA;
                    leftAngle = angleToIntersection;
                }
            } else {
                if (vA < maxRight){
                    maxRight = vA;
                    rightAngle = angleToIntersection;
                }
            }
        }


        if (maxRight == M_PI*3){
            // hmm... no intersection points in top half of rotated circle...
            // so we'll need to check intersection with angle that is closest to M_PI,
            // it should be just intersection with minimal angle from snap
            maxRight =  -M_PI*3;
            for (int i = 0; i < intersectionsCount; i++) {
                RS_Vector v = intersections.at(i);
                double angleToIntersection = center.angleTo(v);
                if (angleToIntersection == leftAngle){
                    continue;
                }
                double vA = RS_Math::correctAnglePlusMinusPi(angleToIntersection - snapAngle);

                if (vA > maxRight){
                    maxRight = vA;
                    rightAngle = angleToIntersection;
                }

            }
        }

        if (minLeft == -M_PI*3){
            // similarly - no intersection points in bottom half of rotated circle...
            // so we'll need to check intersection with angle that is closest to 0,
            // so it should be just intersection with minimal angle from snap
            minLeft =  M_PI*3;
            for (int i = 0; i < intersectionsCount; i++) {
                RS_Vector v = intersections.at(i);
                double angleToIntersection = center.angleTo(v);
                if (angleToIntersection == rightAngle){
                    continue;
                }
                double vA = RS_Math::correctAnglePlusMinusPi(angleToIntersection - snapAngle);

                if (vA < minLeft){
                    minLeft = vA;
                    leftAngle = angleToIntersection;
                }
            }
        }

        result = new CircleSegmentData();

        result->snapSegmentStartAngle = leftAngle;
        result->snapSegmentEndAngle = rightAngle;
    }
    return result;
}

RS2::CursorType LC_ActionModifyBreakDivide::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

void LC_ActionModifyBreakDivide::updateMouseButtonHints(){
    updateMouseWidgetTRCancel(tr("Select line, arc or circle"));
}

LC_ActionOptionsWidget* LC_ActionModifyBreakDivide::createOptionsWidget(){
    return new LC_ModifyBreakDivideOptions();
}
