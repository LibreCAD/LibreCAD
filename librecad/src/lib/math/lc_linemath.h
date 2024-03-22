#ifndef LC_LINEMATH_H
#define LC_LINEMATH_H

#include "rs_vector.h"
#include "rs_line.h"

namespace LC_LineMath {

    enum PointToLinePosition {LEFT,  RIGHT,  BEYOND,  BEHIND, BETWEEN, ORIGIN, DESTINATION};

    bool isMeaningful(double value);

    bool isNotMeaningful(double value);

    double getMeaningful(double candidate, double replacementValue = 0.0);
    double getMeaningfulPositive(double candidate, double replacementValue = 0.0);
    bool isMeaningfulAngle(double value);
    double getMeaningfulAngle(double candidate, double replacementValue = 0.0);

    bool isNonZeroLineLength(const RS_Vector &startPoint, const RS_Vector &endPoint);

    RS_Vector calculateAngleSegment(RS_Vector &startPoint, double angleValueDegree, double distance);

    RS_Vector calculateAngleSegment(RS_Vector &startPoint, RS_Vector &previousLineStart, RS_Vector &previousLineEnd,
                                    double angleValueDegree, bool angleRelative, double distance);

    double defineActualSegmentAngle(double angle, bool angleIsRelative, RS_Vector &previousLineStart, RS_Vector &previousLineEnd);

    RS_Vector getNearestPointOnInfiniteLine(const RS_Vector &coord, const RS_Vector &lineStartPoint, const RS_Vector &lineEndPoint);

    RS_Vector getNearestPointOnLine(RS_Line* baseLIne, const RS_Vector& coord, bool infiniteLine);

    RS_Vector calculateEndpointForAngleDirection(double angleValueDegree,RS_Vector &fromPoint, const RS_Vector &toSnapPoint);

    RS_Vector calculateEndpointForAngleDirection(double angleValueDegree, bool angleIsRelative,RS_Vector &fromPoint, const RS_Vector &toSnapPoint,
                                                 RS_Vector &previousLineStart, RS_Vector &previousLineEnd);

    int getPointPosition(RS_Vector &startPos, RS_Vector &endPos, RS_Vector &point);

    RS_Vector findPointOnCircle(double radius, double arcAngle, RS_Vector centerCircle);

    bool areLinesOnSameRay(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End);

    RS_Vector getIntersectionLineLine(RS_Vector& s1, RS_Vector& e1, RS_Vector& s2, RS_Vector& e2);
}
#endif // LC_LINEMATH_H
