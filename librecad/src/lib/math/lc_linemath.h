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
#ifndef LC_LINEMATH_H
#define LC_LINEMATH_H

class RS_Line;
class RS_Vector;
class RS_VectorSolutions;

struct RS_LineData;

namespace LC_LineMath {

    enum PointToLinePosition {LEFT,  RIGHT,  BEYOND,  BEHIND, BETWEEN, ORIGIN, DESTINATION};

    bool isMeaningful(double value);
    bool isNotMeaningful(double value);
    double getMeaningful(double candidate, double replacementValue = 0.0);
    double getMeaningfulPositive(double candidate, double replacementValue = 0.0);
    bool isMeaningfulAngle(double value);
    bool isSameAngle(double angle1, double angle2);
    double getMeaningfulAngle(double candidate, double replacementValue = 0.0);
    bool isNonZeroLineLength(const RS_Vector &startPoint, const RS_Vector &endPoint);
    RS_Vector getEndOfLineSegment(const RS_Vector &startPoint, double angleValueDegree, double distance);
    RS_Vector relativePoint(const RS_Vector &startPoint, double distance, double angleValueRad);
    RS_Vector calculateAngleSegment(const RS_Vector &startPoint, const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd,
                                    double angleValueDegree, bool angleRelative, double distance);

    double defineActualSegmentAngle(double angle, bool angleIsRelative, const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd);
    RS_Vector getNearestPointOnInfiniteLine(const RS_Vector &coord, const RS_Vector &lineStartPoint, const RS_Vector &lineEndPoint);
    RS_Vector getNearestPointOnLine(const RS_Line* line, const RS_Vector& coord, bool infiniteLine);
    RS_Vector calculateEndpointForAngleDirection(double wcsAngleRad, const RS_Vector &startPoint, const RS_Vector &toSnapPoint);
    RS_Vector calculateEndpointForAngleDirection(double angleValueDegree, bool angleIsRelative, const RS_Vector &fromPoint, const RS_Vector &toSnapPoint,
                                                 const RS_Vector &previousLineStart, const RS_Vector &previousLineEnd);
    int getPointPosition(const RS_Vector &startPos, const RS_Vector &endPos, const RS_Vector &point);
    RS_Vector findPointOnCircle(double radius, double arcAngle, const RS_Vector& centerCircle);
    bool areLinesOnSameRay(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End);
    RS_Vector getIntersectionLineLine(const RS_Vector& s1, const RS_Vector& e1, const RS_Vector& s2, const RS_Vector& e2);
    RS_Vector getIntersectionLineLineFast(const RS_Vector& s1, const RS_Vector& e1, const RS_Vector& s2, const RS_Vector& e2);
    RS_Vector getIntersectionInfiniteLineLineFast(const RS_Vector& infs1, const RS_Vector& infe1, const RS_Vector& s2, const RS_Vector& e2, double offsetX, double offsetY);
    bool hasIntersectionLineRect(const RS_Vector& s1, const RS_Vector& e1, const RS_Vector& s2, const RS_Vector& e2);
    RS_LineData createParallel(const RS_Vector& start, const RS_Vector& end, double distance);
    bool isMeaningfulDistance(const RS_Vector &v1, const RS_Vector &v2);
    bool isNotMeaningfulDistance(const RS_Vector &v1, const RS_Vector &v2);
    bool hasLineIntersection(RS_Vector p0, RS_Vector direction, RS_Vector p2, RS_Vector p3);
    /**
     * @brief convexHull - find the convex hull of a point cloud
     * @param points - input points
     * @return - the convex hull
     */
    RS_VectorSolutions convexHull(const RS_VectorSolutions& points);
    double angleFor3Points(const RS_Vector& edgePoint1, const RS_Vector& intersection, const RS_Vector& edgePoint2);
}
#endif // LC_LINEMATH_H
