#include <cmath>
#include "lc_linemath.h"
#include "rs_vector.h"
#include "rs_math.h"
#include "rs.h"
#include "rs_line.h"

RS_Vector LC_LineMath::calculateAngleSegment(RS_Vector &startPoint, double angleValueDegree, double distance){

    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector line = RS_Vector::polar(distance, angle);
    RS_Vector result  = startPoint + line;
    return result;
}


RS_Vector LC_LineMath::calculateAngleSegment(RS_Vector &startPoint, RS_Vector &previousLineStart, RS_Vector &previousLineEnd, double angleValueDegree, bool angleRelative, double distance){

    double angle = RS_Math::deg2rad(angleValueDegree);
    double realAngle = defineActualSegmentAngle(angle, angleRelative, previousLineStart, previousLineEnd);
    RS_Vector line = RS_Vector::polar(distance, realAngle);
    RS_Vector result  = startPoint + line;
    return result;
}

double LC_LineMath::defineActualSegmentAngle(double angle, bool angleIsRelative, RS_Vector &previousLineStart, RS_Vector &previousLineEnd){
    double result = angle;
    if (angleIsRelative){
        RS_Vector line = previousLineEnd - previousLineStart;
        double previousSegmentAngle = line.angle();
        result  = angle + previousSegmentAngle;

    }
    return result;
}

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

RS_Vector LC_LineMath::calculateEndpointForAngleDirection(double angleValueDegree,RS_Vector &fromPoint, const RS_Vector &toSnapPoint){
    RS_Vector possibleEndPoint;
    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector infiniteTickStartPoint = fromPoint;


    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, angle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = getNearestPointOnInfiniteLine(toSnapPoint, infiniteTickStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

RS_Vector LC_LineMath::calculateEndpointForAngleDirection(double angleValueDegree, bool angleIsRelative, RS_Vector &fromPoint, const RS_Vector &toSnapPoint,
                                                          RS_Vector &previousLineStart, RS_Vector &previousLineEnd){
    RS_Vector possibleEndPoint;
    double angle = RS_Math::deg2rad(angleValueDegree);
    RS_Vector infiniteTickStartPoint = fromPoint;
    double realAngle = defineActualSegmentAngle(angle, angleIsRelative, previousLineStart, previousLineEnd);

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, realAngle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = getNearestPointOnInfiniteLine(toSnapPoint, infiniteTickStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}


RS_Vector LC_LineMath::getNearestPointOnLine(RS_Line* baseLIne, const RS_Vector& coord, bool infiniteLine){
    if (infiniteLine){

        RS_Vector point1 = baseLIne->getStartpoint();
        RS_Vector point2 = baseLIne->getEndpoint();

        return getNearestPointOnInfiniteLine(coord, point1, point2);

    }
    else{
        return baseLIne->getNearestPointOnEntity(coord, true, nullptr);
    }
}

RS_Vector LC_LineMath::findPointOnCircle(double radius, double arcAngle, RS_Vector centerCircle){
    RS_Vector radiusVector = RS_Vector::polar(radius, arcAngle);
    RS_Vector pointPos = centerCircle + radiusVector;
    return pointPos;
}

int LC_LineMath::getPointPosition(RS_Vector &startPos, RS_Vector &endPos, RS_Vector &point)
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

bool LC_LineMath::areLinesOnSameRay(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End){
    double angle1 = line1Start.angleTo(line1End);
    double angle2 = line1Start.angleTo(line2End);
    double angle3 = line1Start.angleTo(line2Start);

    angle1 = RS_Math::correctAngleU(angle1);
    angle2 = RS_Math::correctAngleU(angle2);
    angle3 = RS_Math::correctAngleU(angle3);

    bool sameLine = false;
    if (std::abs(angle1 - angle2) < RS_TOLERANCE_ANGLE && std::abs(angle1 - angle3) < RS_TOLERANCE_ANGLE){
        sameLine = true;
    }
    return sameLine;
}

bool LC_LineMath::isNonZeroLineLength(const RS_Vector &startPoint, const RS_Vector &endPoint){
    return (endPoint - startPoint).squared() > RS_TOLERANCE;
}

double LC_LineMath::getMeaningful(double candidate, double replacementValue){
    double result = candidate;
    if (std::abs(candidate) < RS_TOLERANCE){
        result = replacementValue;
    }
    return result;
}

double LC_LineMath::getMeaningfulPositive(double candidate, double replacementValue){
    double result = candidate;
    if (candidate < RS_TOLERANCE){
        result = replacementValue;
    }
    return result;
}

double LC_LineMath::getMeaningfulAngle(double candidate, double replacementValue){
    double result = candidate;
    if (std::abs(candidate) < RS_TOLERANCE_ANGLE){
        result = replacementValue;
    }
    return result;
}

bool LC_LineMath::isMeaningful(double value){
    return std::abs(value) > RS_TOLERANCE;
}

bool LC_LineMath::isNotMeaningful(double value){
    return std::abs(value) < RS_TOLERANCE;
}

bool LC_LineMath::isMeaningfulAngle(double value){
    return std::abs(value) > RS_TOLERANCE_ANGLE;
}


RS_Vector LC_LineMath::getIntersectionLineLine(RS_Vector& s1, RS_Vector& e1, RS_Vector& s2, RS_Vector& e2) {

    RS_Vector ret;

    double num = ((e2.x - s2.x) * (s1.y - s2.y) - (e2.y - s2.y) * (s1.x - s2.x));
    double div = ((e2.y - s2.y) * (e1.x - s1.x) - (e2.x - s2.x) * (e1.y - s1.y));

    double angle1 = s1.angleTo(e1);
    double angle2 = s2.angleTo(e2);

    if (fabs(div)>RS_TOLERANCE &&
        fabs(remainder(angle1-angle2, M_PI))>=RS_TOLERANCE*10.) {
        double u = num / div;

        double xs = s1.x + u * (e1.x - s1.x);
        double ys = s1.y + u * (e1.y - s1.y);
        ret = RS_Vector(xs, ys);
    }
    else {
        // lines are parallel
        ret = RS_Vector(false);
    }

    return ret;
}
