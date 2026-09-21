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

#ifndef LC_PARAMETRICCURVEINTERSECTION_H
#define LC_PARAMETRICCURVEINTERSECTION_H

#include <cstddef>
#include <limits>
#include <vector>

#include "lc_interval.h"
#include "rs_vector.h"

/**
 * Where two parametric branches meet: a pair of parameter occurrences, not only
 * a point, so a triple point keeps all three incident pairs and a self-touch
 * both of its occurrences. (branchA, parameterA) precedes (branchB, parameterB).
 */
struct LC_ParametricIntersection {
    enum class Kind {
        /** The branches cross. */
        Transverse,
        /** The branches touch without crossing. */
        Tangent,
        /** One occurrence is the free end of a branch. */
        Endpoint
    };
    std::size_t branchA{0};
    double parameterA{0.0};
    std::size_t branchB{0};
    double parameterB{0.0};
    RS_Vector point;
    Kind kind{Kind::Transverse};
};

/** A smooth piece of a branch: no derivative jump inside [t0, t1]. */
struct LC_ParametricSegment {
    std::size_t branch{0};
    double t0{0.0};
    double t1{0.0};
};

/**
 * The curves an intersection query reads: branches made of smooth segments,
 * each with its point and derivative, and conservative bounds of both over any
 * parameter box inside a segment.
 */
class LC_ParametricCurves {
public:
    virtual ~LC_ParametricCurves() = default;

    /**
     * Grouped by branch, and in parameter order within a branch: consecutive
     * segments of a branch meet end to start.
     */
    virtual const std::vector<LC_ParametricSegment>& segments() const = 0;

    virtual std::size_t branchCount() const = 0;

    /**
     * The branch whose start meets the end of @p branch, or -1 for a free end:
     * the branch itself when it is closed, the next one across a cusp.
     */
    virtual std::ptrdiff_t next(std::size_t branch) const = 0;

    /** At a segment's ends, the limit from inside it. */
    virtual bool evaluate(std::size_t segment, double t, RS_Vector& point, RS_Vector& derivative) const = 0;

    /** Every point and derivative the segment takes for t in [a, b]. */
    virtual bool bound(std::size_t segment, double a, double b, LC_Interval& x, LC_Interval& y, LC_Interval& dx,
                       LC_Interval& dy) const = 0;
};

struct LC_IntersectionOptions {
    /** Points this close are one node; a contact closer than this to a join is the join. */
    double tolerance{0.0};
    /** Bisections of a leaf pair before an undecided box is classified. */
    unsigned maxDepth{48};
    /** Bisections of a segment into leaves. */
    unsigned maxLeafDepth{40};
    std::size_t maxLeaves{65536};
    /** Box pairs examined in all. */
    std::size_t maxBoxPairs{1u << 20};
    /** Only pairs with a leaf on a branch below this index are examined: the rest meet as they may. */
    std::size_t anchoredBranches{std::numeric_limits<std::size_t>::max()};
};

enum class LC_IntersectionStatus {
    Ok,
    InvalidInput,
    /** A contact that cannot be classified, or branches that retrace each other. */
    AmbiguousTopology,
    LimitExceeded
};

struct LC_IntersectionResult {
    LC_IntersectionStatus status{LC_IntersectionStatus::InvalidInput};
    /** Sorted by (branchA, parameterA, branchB, parameterB). */
    std::vector<LC_ParametricIntersection> intersections;
};

/**
 * Every place where the branches meet, other than where consecutive pieces of a
 * branch, or branches joined at a cusp, share an end. Segments are split into
 * leaves whose tangent directions span less than a right angle, which cannot
 * cross themselves or their neighbours; pairs of leaves with overlapping bounds
 * are resolved by interval exclusion and a Krawczyk uniqueness test, and a root
 * it proves unique is refined by Newton steps. A box that stays undecided is a
 * tangent contact when the branches come within the tolerance and stay on one
 * side of each other, a crossing when they change sides, and ambiguous
 * otherwise, as where they retrace each other. The result does not depend on the
 * order in which pairs are examined.
 */
LC_IntersectionResult findIntersections(const LC_ParametricCurves& curves, const LC_IntersectionOptions& options);

#endif
