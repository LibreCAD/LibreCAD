/****************************************************************************
**
Various utility computation methods

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
#include <cmath>

#include "lc_linemath.h"
#include "rs.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_vector.h"

/**
 * Calculates point located on specified distance and angle from given starting point
 * @param startPoint start point
 * @param angleValueDegree angle in degrees
 * @param distance distance for vector
 * @return resulting point
 */
RS_Vector LC_LineMath::getEndOfLineSegment(const RS_Vector &startPoint, double angleValueDegree, double distance){
    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector line = RS_Vector::polar(distance, angle);
    RS_Vector result  = startPoint + line;
    return result;
}


/**
 * Return points that is located by specified distance and angle from given start point
 * @param startPoint start point
 * @param angleValueRad angle to target end point from start point (in radians)
 * @param distance  distance between start and end point
 * @return resulting point
 */
RS_Vector LC_LineMath::relativePoint(const RS_Vector &startPoint, double distance, double angleValueRad){
    RS_Vector offsetVector = RS_Vector::polar(distance, angleValueRad);
    RS_Vector result  = startPoint + offsetVector;
    return result;
}

RS_Vector LC_LineMath::calculateAngleSegment(const RS_Vector &startPoint, const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd, double angleValueDegree, bool angleRelative, double distance){

    double angle = RS_Math::deg2rad(angleValueDegree);
    double realAngle = defineActualSegmentAngle(angle, angleRelative, previousLineStart, previousLineEnd);
    RS_Vector line = RS_Vector::polar(distance, realAngle);
    RS_Vector result  = startPoint + line;
    return result;
}

/**
 * Defines actual angle to provide line segment. If angle value is relative, adds that angle to own angle of line segment
 * @param angle specified angle
 * @param angleIsRelative if true, angle value is related
 * @param previousLineStart previous line segment start
 * @param previousLineEnd previous line segment end
 * @return
 */
double LC_LineMath::defineActualSegmentAngle(double angle, bool angleIsRelative, const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd){
    double result = angle;
    if (angleIsRelative){
        RS_Vector line = previousLineEnd - previousLineStart;
        double previousSegmentAngle = line.angle();
        result  = angle + previousSegmentAngle;

    }
    return result;
}
/**
 * Calculates projection of given point to infinite line (ray), that extends lines provided by coordinates
 * @param coord point coordinates
 * @param lineStartPoint line start
 * @param lineEndPoint  line end
 * @return projectsion coordinates
 */
RS_Vector LC_LineMath::getNearestPointOnInfiniteLine(const RS_Vector &coord, const RS_Vector &lineStartPoint, const RS_Vector &lineEndPoint){
    RS_Vector ae = lineEndPoint - lineStartPoint;
    RS_Vector ea = lineStartPoint-lineEndPoint;
    RS_Vector ap = coord-lineStartPoint;

    if (ae.magnitude()<RS_TOLERANCE|| ea.magnitude()<RS_TOLERANCE) {
        return RS_Vector(false);
    }

    // Orthogonal projection from both sides:
    RS_Vector ba = ae * RS_Vector::dotP(ae, ap)
                   / (ae.magnitude()*ae.magnitude());

    return lineStartPoint+ba;
}

/**
 * Calculates actual endpoint of line segment, that starts from given point by provided angle and is controlled by provided
 * snap point. Actually, the endpoint of segment is projection of snap point to infinite line that has specified angle and starts
 * from given starting point.
 * @param angleValueDegree angle value in degrees
 * @param startPoint start point of line segment
 * @param toSnapPoint snap point
 * @return end point for the segment
 */
RS_Vector LC_LineMath::calculateEndpointForAngleDirection(double angleValueDegree, const RS_Vector &startPoint, const RS_Vector &toSnapPoint){
    RS_Vector possibleEndPoint;
    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector infiniteTickStartPoint = startPoint;


    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, angle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = getNearestPointOnInfiniteLine(toSnapPoint, infiniteTickStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

RS_Vector LC_LineMath::calculateEndpointForAngleDirection(double angleValueDegree, bool angleIsRelative, const RS_Vector &fromPoint, const RS_Vector &toSnapPoint,
                                                          const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd){
    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector infiniteTickStartPoint = fromPoint;
    double realAngle = defineActualSegmentAngle(angle, angleIsRelative, previousLineStart, previousLineEnd);

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, realAngle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = getNearestPointOnInfiniteLine(toSnapPoint, infiniteTickStartPoint, infiniteTickEndPoint);

    return pointOnInfiniteTick;
}

/**
 * Returns nearest projection of point to given line
 * @param line  line
 * @param coord point
 * @param infiniteLine if true, given line is considered to be infinite
 * @return point
 */
RS_Vector LC_LineMath::getNearestPointOnLine(const RS_Line* line, const RS_Vector& coord, bool infiniteLine){
    // For infinite lines, find the nearest point is not limited by start/end points
    bool onEntity = ! infiniteLine;
    return line != nullptr ? line->getNearestPointOnEntity(coord, onEntity, nullptr) : RS_Vector{false};
}

/**
 * Determines point that lies on circle of provided center and radius and with specific angle to point
 * @param radius circle radius
 * @param arcAngle  circle angle
 * @param centerCircle center of circle
 * @return point coordinates
 */
RS_Vector LC_LineMath::findPointOnCircle(double radius, double arcAngle, const RS_Vector& centerCircle){
    RS_Vector radiusVector = RS_Vector::polar(radius, arcAngle);
    RS_Vector pointPos = centerCircle + radiusVector;
    return pointPos;
}

/**
 * Determines how given point relates to provided line
 * @param startPos start point of line
 * @param endPos end point of line
 * @param point  point to check
 * @return point position
 */
int LC_LineMath::getPointPosition(const RS_Vector &startPos, const RS_Vector &endPos, const RS_Vector &point)
{
    RS_Vector a = endPos - startPos; // 1
    RS_Vector b = point - startPos; // 2
    double sa = a. x * b.y - b.x * a.y; // 3
    if (sa > 0.0)
        return LEFT;
    if (sa < 0.0)
        return RIGHT;
    if ((a.x * b.x < 0.0) || (a.y * b.y < 0.0))
        return BEHIND;
    if (a.magnitude() < b.magnitude())
        return BEYOND;
    if (startPos == point)
        return ORIGIN;
    if (endPos == point)
        return DESTINATION;
    return BETWEEN;
}
/**
 * Checks whether two lines defined by their edge points lay on the same ray vector
 * @param line1Start
 * @param line1End
 * @param line2Start
 * @param line2End
 * @return
 */
bool LC_LineMath::areLinesOnSameRay(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End){
    // TODO: create a generic algorithm to find whether points are collinear
    // Find the rank of the matrix formed by point homogeneous coordinates:
    // coincidence: rank = 1
    // collinear:   rank = 2
    // coplanar:    rank = 3
    double angle1 = line1Start.angleTo(line1End);
    double angle2 = line1Start.angleTo(line2End);
    double angle3 = line1Start.angleTo(line2Start);

    // if all points are on the same ray, the angles from first point to remaining 3 points will be the same

    angle1 = RS_Math::correctAngle0ToPi(angle1);
    angle2 = RS_Math::correctAngle0ToPi(angle2);
    angle3 = RS_Math::correctAngle0ToPi(angle3);

    bool sameLine = false;
    if (std::abs(angle1 - angle2) < RS_TOLERANCE_ANGLE && std::abs(angle1 - angle3) < RS_TOLERANCE_ANGLE){
        sameLine = true;
    }
    return sameLine;
}

/**
 * Check whether distance between points is meaningful
 * @param startPoint
 * @param endPoint
 * @return
 */
bool LC_LineMath::isNonZeroLineLength(const RS_Vector &startPoint, const RS_Vector &endPoint){
    return (endPoint - startPoint).squared() > RS_TOLERANCE;
}

/**
 * Returns meaningful value. If provided candidate is not meaningful, returns replacement value
 * @param candidate
 * @param replacementValue
 * @return
 */
double LC_LineMath::getMeaningful(double candidate, double replacementValue){
    return (std::abs(candidate) < RS_TOLERANCE) ? replacementValue : candidate;
}


double LC_LineMath::getMeaningfulPositive(double candidate, double replacementValue){
    return (candidate < RS_TOLERANCE) ? replacementValue : candidate;
}

/**
 * Checks and returns meaningful angle - if provided value is not meaningful, returns replacement value
 * @param candidate
 * @param replacementValue
 * @return
 */
double LC_LineMath::getMeaningfulAngle(double candidate, double replacementValue){
    return (std::abs(candidate) < RS_TOLERANCE_ANGLE) ? replacementValue : candidate;
}

/**
 * Returns true if value is meaningful
 * @param value
 * @return
 */
bool LC_LineMath::isMeaningful(double value){
    return std::abs(value) > RS_TOLERANCE;
}

/**
 * Return true if angle value is not meaningful
 * @param value
 * @return
 */
bool LC_LineMath::isNotMeaningful(double value){
    return std::abs(value) < RS_TOLERANCE;
}

/**
 * Returns true if angle value is meaningful
 * @param value
 * @return
 */
bool LC_LineMath::isMeaningfulAngle(double value){
    return std::abs(value) > RS_TOLERANCE_ANGLE;
}

/**
 * Return true if distance between two points is meaningful and so these are
 * different points
 * @param v1 first point
 * @param v2 second point
 * @return true if points are different
 */
bool LC_LineMath::isMeaningfulDistance(const RS_Vector &v1, const RS_Vector &v2){
    double distance = v1.distanceTo(v2);
    bool result = distance > RS_TOLERANCE;
    return result;
}
/**
 * Return true if distance between two points is not meaningful (close to zero
 * and less than RS_TOLERANCE) - so basically these to points may be considered as
 * the same point
 * @param v1 first point
 * @param v2 second point
 * @return true if distance not meaningful, false otherwise
 */
bool LC_LineMath::isNotMeaningfulDistance(const RS_Vector &v1, const RS_Vector &v2){
    double distance = v1.distanceTo(v2);
    bool result = distance < RS_TOLERANCE;
    return result;
}

/**
 * Creates line that is parallel to given original line (defined by start and and point), that is located
 * on given distance
 * @param start start of original line
 * @param end end of original line
 * @param distance distance for parallel line
 * @return line data that describes parallel line
 */
RS_LineData LC_LineMath::createParallel(const RS_Vector &start, const RS_Vector &end, double distance){
    RS_Line line{nullptr, {start, end}};
    double ang = line.getDirection1() + M_PI_2;
    // calculate 1st parallel:
    line.move(RS_Vector::polar(distance, ang));
    return line.getData();
}

/**
 * Determines intersection point for two lines defined by provided start and end vectors
 * @param s1  start of line 1
 * @param e1  end of line 1
 * @param s2  start of line 2
 * @param e2  end of line 2
 * @return intersection point, invalid vector if no intersection point found
 */
RS_Vector LC_LineMath::getIntersectionLineLine(const RS_Vector& s1, const RS_Vector& e1, const RS_Vector& s2, const RS_Vector& e2) {
    RS_Line line1{nullptr, {s1, e1}};
    RS_Line line2{nullptr, {s2, e2}};
    RS_VectorSolutions sol = RS_Information::getIntersectionLineLine(&line1, &line2);
    return sol.empty() ? RS_Vector{false} : sol.at(0);
}
