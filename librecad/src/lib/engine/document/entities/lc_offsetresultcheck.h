/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

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
 ******************************************************************************/

#ifndef LC_OFFSETRESULTCHECK_H
#define LC_OFFSETRESULTCHECK_H

#include <cstddef>
#include <vector>

#include "rs_vector.h"

class RS_Entity;
class RS_Polyline;

/**
 * Exact checks of offsets made of lines and circular arcs.
 *
 * Every distance and intersection here is in closed form between line
 * segments and circular arcs, so a verdict about an offset of a polyline never
 * rests on sampling. Candidate pairs come from a uniform grid over segment
 * boxes, so checking n segments costs O(n) for ordinary shapes.
 *
 * checkLegacyPolylineOffset() is the gate Modify > Offset applies to what
 * RS_Polyline::offset() makes, until polylines are offset by the curve-offset
 * engine: that code joins neighbouring offsets pairwise and never trims a loop,
 * so its result may run backwards, leave gaps, cross itself or come nearer to
 * the source than the distance, and it always reports success.
 */

/** One child of a polyline as it is travelled: a line from a to b, or an arc. */
struct LC_OffsetSegment {
    bool arc = false;
    RS_Vector a{false};
    RS_Vector b{false};
    /** Arcs only: centre, radius, the angle of a, and the signed sweep (anticlockwise > 0). */
    RS_Vector centre{false};
    double radius = 0.0;
    double startAngle = 0.0;
    double sweep = 0.0;

    double length() const;
    /** The point at @p u in [0, 1] of the way along it. */
    RS_Vector pointAt(double u) const;
    /** Its exact bounding box. */
    void box(RS_Vector& lo, RS_Vector& hi) const;
    /** The same segment travelled the other way. */
    LC_OffsetSegment reversed() const;
};

/**
 * The segment of a line or arc child as stored; false for any other entity.
 */
bool makeOffsetSegment(const RS_Entity& child, LC_OffsetSegment& segment);

/**
 * The children of @p polyline oriented head to tail, as RS_Polyline::offset()
 * orients them before it offsets, but read-only: the polyline is not changed.
 * Empty if a child is neither a line nor an arc.
 */
std::vector<LC_OffsetSegment> orientedSegments(const RS_Polyline& polyline);

/** The children of @p polyline in the direction each is stored. Empty as above. */
std::vector<LC_OffsetSegment> storedSegments(const RS_Polyline& polyline);

/** The exact distance from @p p to @p s. */
double pointSegmentDistance(const RS_Vector& p, const LC_OffsetSegment& s);

/** The exact least distance between two segments: 0 where they meet. */
double segmentDistance(const LC_OffsetSegment& s, const LC_OffsetSegment& t);

/**
 * Where two segments meet, each lengthened by @p tolerance at both ends: a
 * point per crossing or touch, and the two ends of each stretch they share
 * (collinear lines, arcs of one circle).
 */
std::vector<RS_Vector> segmentIntersections(const LC_OffsetSegment& s, const LC_OffsetSegment& t,
                                            double tolerance);

bool segmentsIntersect(const LC_OffsetSegment& s, const LC_OffsetSegment& t, double tolerance);

/** The signed area a closed chain of segments encloses (anticlockwise > 0): shoelace plus circular segments. */
double signedArea(const std::vector<LC_OffsetSegment>& chain);

double chainLength(const std::vector<LC_OffsetSegment>& chain);

enum class LC_OffsetCheckVerdict {
    /** The offset may be used as it is. */
    Valid,
    /** Nothing of the source is left at this distance: the offset is its inverted or collapsed ghost. */
    NothingLeft,
    /** The offset is not a trimmed offset: it must not be made. */
    Invalid
};

struct LC_OffsetCheckReport {
    LC_OffsetCheckVerdict verdict = LC_OffsetCheckVerdict::Invalid;
    /**
     * The least distance found from the offset to the source: exact when it
     * is below the distance, and otherwise at least that (infinity when no
     * source child is within the distance of the offset).
     */
    double nearest = 0.0;
    /** A child shorter than the tolerance, or neighbouring children apart. */
    bool broken = false;
    /** A line against its source's direction; an arc that changed sweep, wrapped, or did not move by the distance. */
    bool reversed = false;
    /** Two children of the offset meet other than at their shared end. */
    bool crossing = false;
    /** The offset comes nearer to the source than the distance. */
    bool near = false;
    /** A closed offset whose area is inverted or next to nothing. */
    bool collapsed = false;
    /** Every sampled point of the offset is nearer than the distance. */
    bool entirelyNear = false;
    /** Grid cells and candidate entries examined: the work done. */
    std::size_t cellsVisited = 0;
};

/**
 * The verdict on @p offset, the result of RS_Polyline::offset() on a copy of
 * @p source at @p distance (its magnitude is used). Its children must pair
 * with the source's: the same count, in the same order, each oriented as
 * orientedSegments() orients the source's. The tolerance is 1e-6 of the
 * larger of the source's extent and the distance.
 */
LC_OffsetCheckReport checkLegacyPolylineOffset(const RS_Polyline& source, const RS_Polyline& offset,
                                               double distance);

#endif
