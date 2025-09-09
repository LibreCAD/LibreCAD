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

#ifndef LC_DIVISION_H
#define LC_DIVISION_H
#include <QVector>

#include "rs_vector.h"

class RS_Line;
class RS_Circle;
class RS_Arc;
class RS_EntityContainer;
class RS_Entity;
class RS_Vector;

class LC_Division{
public:
    /**
        * configuration of segment of entity on which snap selection occurred
        */
    enum SegmentDisposition{
        SEGMENT_INSIDE, // segment is between two intersection points
        SEGMENT_TO_START, // snap is between start point of entity and intersection point
        SEGMENT_TO_END // snap is between end point of entity and intersection point
    };

    /**
     * Snap segment info for line
     */
    struct LineSegmentData{
        SegmentDisposition segmentDisposition;
        RS_Vector snap;
        RS_Vector snapSegmentStart;
        RS_Vector snapSegmentEnd;
    };

    /**
     * Snap segment for angle
     */
    struct ArcSegmentData{
        int segmentDisposition;
        double snapSegmentStartAngle;
        double snapSegmentEndAngle;
    };

    /**
     * Snap segment for circle     *
     */
    struct CircleSegmentData{
        double snapSegmentStartAngle;
        double snapSegmentEndAngle;
    };

    LC_Division(RS_EntityContainer *entityContainer);

    ArcSegmentData* findArcSegmentBetweenIntersections(RS_Arc* arc, RS_Vector& snap, bool allowEntireArcAsSegment);
    CircleSegmentData* findCircleSegmentBetweenIntersections(RS_Circle* circle, RS_Vector& snap, bool allowEntireCircleAsSegment);
    LineSegmentData* findLineSegmentBetweenIntersections(RS_Line* line, RS_Vector& snap, bool allowEntireLine);

    LineSegmentData* findLineSegmentEdges(RS_Line* line, RS_Vector& snap, QVector<RS_Vector> intersections, bool allowEntireLinesAsSegment);
    ArcSegmentData* findArcSegmentEdges(RS_Arc* arc, RS_Vector& snap, const QVector<RS_Vector>& intersections, bool allowEntireArcAsSegment);
    CircleSegmentData* findCircleSegmentEdges(RS_Circle* circle, RS_Vector& snap, const QVector<RS_Vector>& intersections);

    QVector<RS_Vector> collectAllIntersectionsWithEntity(RS_Entity *entity);
private:
    RS_EntityContainer *m_container = nullptr;
protected:
    void addPointsFromSolutionToList(RS_VectorSolutions& sol, QVector<RS_Vector>& result) const;
};

#endif // LC_DIVISION_H
