/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_division.h"

#include <QVector>
#include <cfloat>

#include "rs_entitycontainer.h"
#include "lc_containertraverser.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_vector.h"

class RS_EntityContainer;

LC_Division::LC_Division(RS_EntityContainer *entityContainer):
    m_container{entityContainer} {
}

/**
 * determines segment of arc selected by the user
 * @param arc arc
 * @param snap snap point
 * @return segment information
 */
LC_Division::ArcSegmentData *LC_Division::findArcSegmentBetweenIntersections(RS_Arc *arc, RS_Vector &snap, bool allowEntireArcAsSegment){
    ArcSegmentData *result = nullptr;
    // detect all intersections
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(arc);
    if (allIntersections.empty()) {
        if (allowEntireArcAsSegment){ // allowing deletion of complete arcs
            result = new ArcSegmentData();
            result->segmentDisposition = SEGMENT_INSIDE;
            result->snapSegmentStartAngle = arc->getAngle1();
            result->snapSegmentEndAngle = arc->getAngle2();
        }
    }
    else{
        // determine selected segment edges
        result = findArcSegmentEdges(arc, snap, allIntersections, allowEntireArcAsSegment);
    }
    return result;
}

/**
 * determines segment of circle selected by the user
 * @param circle circle
 * @param snap snap point
 * @return segment information
 */
LC_Division::CircleSegmentData *LC_Division::findCircleSegmentBetweenIntersections(RS_Circle *circle, RS_Vector &snap, bool allowEntireCircleAsSegment){
    CircleSegmentData *result = nullptr;
    // detect all intersections
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(circle);
    if (allIntersections.empty()) {
        if (allowEntireCircleAsSegment) {
            result = new CircleSegmentData();
            result->snapSegmentStartAngle = 0;
            result->snapSegmentEndAngle = M_PI * 2;
        }
    }
    else{
        // determine selected segment edges
        result = findCircleSegmentEdges(circle, snap, allIntersections);
    }
    return result;
}


/**
 * determines segment of line selected by the user
 * @param line line
 * @param snap snap point
 * @return segment information
 */
LC_Division::LineSegmentData *LC_Division::findLineSegmentBetweenIntersections(RS_Line *line, RS_Vector &snap, bool allowEntireLine){
    LineSegmentData *result = nullptr;
    // find all intersection points for line
    QVector<RS_Vector> allIntersections = collectAllIntersectionsWithEntity(line);
    if (allIntersections.empty()) {
        if (allowEntireLine){ // allow to delete entire line if SHIFT is pressed
            result = new LineSegmentData();
            result->segmentDisposition = SEGMENT_INSIDE;
            result->snapSegmentStart = line->getStartpoint();
            result->snapSegmentEnd = line->getEndpoint();
            result->snap = snap;
        }
    }
    else{
        // determine segments of line that was selected by the user
        result = findLineSegmentEdges(line, snap, allIntersections, allowEntireLine);
    }
    return result;
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
LC_Division::LineSegmentData *LC_Division::findLineSegmentEdges(RS_Line *line, RS_Vector &snap, QVector<RS_Vector> intersections,
    bool allowEntireLineAsSegment){
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
    else{ // there are intersections only on edges. We may return the entire line as segment if SHIFT is pressed and the user would like to delete the entire entity
        if (allowEntireLineAsSegment){
            result = new LineSegmentData();
            result->segmentDisposition = SEGMENT_INSIDE;
            result->snapSegmentStart = line->getStartpoint();
            result->snapSegmentEnd = line->getEndpoint();
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
LC_Division::ArcSegmentData *LC_Division::findArcSegmentEdges(RS_Arc *arc, RS_Vector &snap, const QVector<RS_Vector>& intersections, bool allowEntireArcAsSegment){

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
    else { // there are intersections only on edges. We may return the entire line as segment if SHIFT is pressed and the user would like to delete the entire entity
        if (allowEntireArcAsSegment){
            result = new ArcSegmentData();
            result->segmentDisposition = SEGMENT_INSIDE;
            result->snapSegmentStartAngle = arcStartAngle;
            result->snapSegmentEndAngle = arcEndAngle;
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
LC_Division::CircleSegmentData* LC_Division::findCircleSegmentEdges(RS_Circle *circle, RS_Vector &snap,
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

//            previewRefPoint(v);

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



/**
 * Collects all intersection of given entity with other entities.
 * @param entity entity to check for intersections
 * @return vector of intersection points
 */
QVector<RS_Vector> LC_Division::collectAllIntersectionsWithEntity(RS_Entity *entity){
    QVector<RS_Vector> result;
    RS_VectorSolutions sol;
    // iterate over all entities
    for (auto* e: *m_container) {
        // consider only visible entities
        if (e != nullptr && e->isVisible()){
            // select containers / groups:
            if (e->isContainer()){
                // additional handling for containers
                auto *ec = static_cast<RS_EntityContainer*>(e);

                for(RS_Entity* e2: lc::LC_ContainerTraverser{*ec, RS2::ResolveAll}.entities()) {
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
void LC_Division::addPointsFromSolutionToList(RS_VectorSolutions &sol, QVector<RS_Vector> &result) const{
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
