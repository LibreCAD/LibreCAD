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

#ifndef LC_CURVEOFFSET_H
#define LC_CURVEOFFSET_H

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include "lc_offsetoutputbudget.h"
#include "lc_parametriccurveintersection.h"
#include "rs_entity.h"
#include "rs_vector.h"

/**
 * Offsets of RS_Spline and LC_SplinePoints curves.
 *
 * The offset of a curve C at signed distance d is Q(t) = C(t) + d N(t), with the
 * left unit normal N = (-C'y, C'x) / |C'|: a positive d lies to the left of the
 * curve's direction. Q is not a spline of the same kind, so the engine
 * approximates it by cubic Bezier pieces that keep their source parameter
 * interval (a branch), and checks each piece against Q itself.
 *
 * Direct mode returns the whole oriented offset. It
 * requires a nonzero source tangent and 1 - d * kappa away from zero, both
 * proved with interval bounds everywhere except at cusps and stalls. A cusp is
 * an isolated root where 1 - d * kappa changes sign, inside a span or across a
 * join; the offset turns back there, which ends one branch and starts the
 * next. A stall is a stretch over which the offset moves less than the merge
 * tolerance without turning back: 1 - d * kappa touches zero, where the
 * distance equals a radius of curvature, or two cusps lie closer than that.
 * A stall is not fitted; the pieces on either side meet. Any other root fails
 * the request. At a tangent break of the source, a kink, the offset rounds a
 * corner that turns away from it by an arc of radius |d| about the kink; where
 * the corner turns towards it, the offsets of its two sides overlap, and the
 * branch of each ends there (LC_OffsetBranchEnd::Kink). Direct mode does not
 * remove loops or self-intersections, and its error check is sampled
 * evidence, never a proof of the maximum deviation.
 *
 * Trimmed mode, programmatic only, removes from the noded Direct offset the
 * parts provably nearer to the source than the distance, less the tolerance
 * budget, and fails where it cannot decide.
 *
 * RegionBoundary mode, programmatic only, takes the region a closed source
 * encloses under the fill rule and returns the boundary of that region grown
 * or shrunk by the disk of the distance, as closed branches with the result on
 * their left. A direction point outside the region grows it and one inside
 * shrinks it; LC_CurveOffsetSide::Right grows and Left shrinks. The branch
 * pieces are offsets of the region's boundary and, at its corners, arcs of
 * the circles about them (LC_OffsetBranchProvenance::arcCentre).
 */

enum class LC_CurveOffsetStatus {
    Ok,
    /** Options, budget or request not filled by a factory, or a mode not implemented. */
    InvalidRequest,
    InvalidSource,
    UnsupportedNonPlanar,
    InvalidDistance,
    UnsupportedClosedResult,
    AmbiguousSide,
    UndefinedTangent,
    DiscontinuousNormal,
    SingularOffset,
    AmbiguousRegularity,
    AmbiguousTopology,
    FitFailed,
    ToleranceNotMet,
    LimitExceeded
};

enum class LC_CurveOffsetMode {
    Direct,
    Trimmed,
    RegionBoundary
};

enum class LC_CurveFillRule {
    NonZero,
    EvenOdd
};

enum class LC_CurveOffsetSide {
    /** The side the request's direction point lies on. */
    FromDirectionPoint,
    Left,
    Right
};

enum class LC_OffsetValidationLevel {
    None,
    /** Checked at sample points: evidence, not a bound between them. */
    SampledBidirectional,
    IntervalCertified
};

/**
 * Tolerances, all world lengths except parameter. The defaults are NaN, so an
 * options object that was not filled by a factory fails validation.
 */
struct LC_CurveOffsetTolerances {
    /** The geometric tolerance asked for: the sum of the three budgets below. */
    double requestedGeometry{std::numeric_limits<double>::quiet_NaN()};
    double evaluation{std::numeric_limits<double>::quiet_NaN()};
    /** The share a fitted piece may deviate from the exact offset. */
    double fit{std::numeric_limits<double>::quiet_NaN()};
    /** The share spent pinning neighbouring pieces to a shared point. */
    double nodeMerge{std::numeric_limits<double>::quiet_NaN()};
    /** Parameter resolution for bisection and Newton steps; not a geometric claim. */
    double parameter{std::numeric_limits<double>::quiet_NaN()};
    double rootResidual{std::numeric_limits<double>::quiet_NaN()};
    /** A direction point closer to the curve than this has no side. */
    double classification{std::numeric_limits<double>::quiet_NaN()};
};

struct LC_CurveOffsetRequest {
    /** Used only with LC_CurveOffsetSide::FromDirectionPoint. */
    RS_Vector directionPoint{false};
    double distanceMagnitude{std::numeric_limits<double>::quiet_NaN()};
    LC_CurveOffsetSide side{LC_CurveOffsetSide::FromDirectionPoint};
};

struct LC_CurveOffsetOptions {
    LC_CurveOffsetMode mode{LC_CurveOffsetMode::Direct};
    LC_CurveFillRule fillRule{LC_CurveFillRule::NonZero};
    LC_CurveOffsetTolerances tolerance{};
    /** Tangent angle a fitted piece may deviate by, in radians; refinement only. */
    double angleTolerance{std::numeric_limits<double>::quiet_NaN()};
    unsigned maxSubdivisionDepth{0};
    /** Exact offset evaluations per source. */
    std::size_t maxSamples{0};
    std::size_t maxOutputBranches{0};
    /** Box pairs an intersection query may examine. */
    std::size_t maxIntersectionPairs{0};
    std::size_t maxDistanceMapBoxes{0};
    std::size_t maxArrangementEdges{0};
    std::size_t maxArrangementFaces{0};
    /**
     * Topology mode for Direct: find where the offset meets itself and split
     * its pieces there, so every crossing is a shared end of the pieces
     * incident to it. The geometry is the same; only the pieces differ.
     */
    bool nodeIntersections{false};
};

/** Where a piece came from: an interval of one source span, and the distance. */
struct LC_OffsetBranchProvenance {
    std::size_t sourceSpan{0};
    double sourceT0{0.0};
    double sourceT1{0.0};
    double signedDistance{0.0};
    bool forward{true};
    /**
     * Set only for an arc about a corner of the source, a kink or a vertex of a
     * region boundary: the piece follows the circle of radius |signedDistance|
     * about this point, over the angles sourceT0 to sourceT1 (which may run
     * down), instead of the offset of a source interval.
     */
    RS_Vector arcCentre{false};
};

/**
 * One cubic Bezier piece of an offset branch. Its own parameter u in
 * [localU0, localU1] maps linearly onto [sourceT0, sourceT1].
 */
struct LC_OffsetCubicPiece {
    LC_OffsetBranchProvenance provenance;
    double localU0{0.0};
    double localU1{1.0};
    std::array<RS_Vector, 4> bezier;
};

/** How an end of an open branch meets the rest of the offset. */
enum class LC_OffsetBranchEnd {
    /** A free end: the offset of an open source's end, or where trimming cut it. */
    Free,
    /** The offset turns back into the neighbouring branch, which shares the point. */
    Cusp,
    /**
     * The offset of one side of a corner that turns towards it: the branch
     * runs on past its neighbour, which does not share the point, until
     * trimming cuts both where they cross.
     */
    Kink
};

struct LC_OffsetBranch {
    /**
     * In source parameter order, which for a closed source may wrap past its
     * seam; each piece starts where the previous one ends.
     */
    std::vector<LC_OffsetCubicPiece> cubicPieces;
    /** The last piece ends where the first begins. */
    bool closed{false};
    /** The branch is exactly the straight segment from its start to its end. */
    bool straight{false};
    /**
     * The offset runs against its source here, where 1 - d * kappa < 0: every
     * point of it is nearer to the source than the distance.
     */
    bool reversed{false};
    LC_OffsetBranchEnd startEnd{LC_OffsetBranchEnd::Free};
    LC_OffsetBranchEnd endEnd{LC_OffsetBranchEnd::Free};
};

struct LC_CurveOffsetGeometryResult {
    LC_CurveOffsetStatus status{LC_CurveOffsetStatus::InvalidSource};
    std::vector<LC_OffsetBranch> branches;
    LC_OffsetValidationLevel validationLevel{LC_OffsetValidationLevel::None};
    double maxObservedError{std::numeric_limits<double>::quiet_NaN()};
    double maxCertifiedError{std::numeric_limits<double>::quiet_NaN()};
    /** The signed distance the branches were built for. */
    double signedDistance{std::numeric_limits<double>::quiet_NaN()};
    std::size_t exactSamples{0};
    std::size_t sourceIntersections{0};
    std::size_t offsetIntersections{0};
    std::size_t removedIntervals{0};
    /**
     * With nodeIntersections: where the exact offset meets itself, by branch and
     * source parameter; each is a shared end of the pieces incident to it.
     */
    std::vector<LC_ParametricIntersection> intersections;
};

/** Fresh output entities, owned here until the caller releases them. Move-only. */
struct LC_CurveOffsetMaterializationResult {
    LC_CurveOffsetStatus status{LC_CurveOffsetStatus::InvalidSource};
    std::vector<std::unique_ptr<RS_Entity>> entities;
    LC_OffsetOutputUsage usage{};
    LC_OffsetValidationLevel validationLevel{LC_OffsetValidationLevel::None};
    double maxObservedError{std::numeric_limits<double>::quiet_NaN()};
    double maxCertifiedError{std::numeric_limits<double>::quiet_NaN()};
};

/**
 * The side of a source a direction point selects, computed once and reused for
 * every copy distance of a multi-copy offset.
 */
struct LC_OffsetSideResolution {
    LC_CurveOffsetStatus status{LC_CurveOffsetStatus::InvalidSource};
    /** Left or Right when status is Ok. */
    LC_CurveOffsetSide side{LC_CurveOffsetSide::FromDirectionPoint};
    /** The source parameters nearest to the direction point. */
    std::vector<double> occurrences;
    /** How far the direction point is from the source, when status is Ok. */
    double distance{std::numeric_limits<double>::quiet_NaN()};
};

namespace LC_CurveOffset {

/**
 * Relative geometric tolerance used when the caller gives none: a fraction of
 * the larger of the source's extent and the offset distance.
 */
inline constexpr double kDefaultRelativeOffsetTolerance = 1e-6;
inline constexpr double kDefaultOffsetAngleTolerance = 1e-3;
inline constexpr unsigned kDefaultMaxSubdivisionDepth = 20;
inline constexpr std::size_t kDefaultMaxSamples = 65536;
inline constexpr std::size_t kDefaultMaxOutputBranches = 256;

/** Whether the entity is a curve the engine can offset: an RS_Spline or an
 *  LC_SplinePoints, subclasses included. */
bool isSupportedSource(const RS_Entity& source);

/**
 * Direct-mode options with tolerances derived from the source's scale and the
 * distance. @p requestedTolerance, if positive, replaces the default relative
 * tolerance. Left unfilled (and so rejected by the engine) when the source or
 * distance has no finite scale.
 */
LC_CurveOffsetOptions makeDirectOptions(const RS_Entity& source, double distanceMagnitude,
                                        double requestedTolerance = 0.0);

LC_CurveOffsetRequest makeDirectionRequest(const RS_Vector& directionPoint, double distanceMagnitude);
LC_CurveOffsetRequest makeSideRequest(LC_CurveOffsetSide side, double distanceMagnitude);

inline LC_OffsetSourceBudget makeDirectSourceBudget() {
    return makeDefaultOffsetSourceBudget();
}

/**
 * The side of @p source that @p directionPoint lies on, from the source
 * parameters nearest to it. AmbiguousSide if the point is on the curve or
 * equally near occurrences disagree.
 */
LC_OffsetSideResolution resolveSide(const RS_Entity& source, const RS_Vector& directionPoint,
                                    const LC_CurveOffsetOptions& options);

/**
 * The Direct offset of @p source as cubic Bezier branches; no entity is made.
 * Stops with LimitExceeded before exceeding @p budget's cubic pieces.
 */
LC_CurveOffsetGeometryResult buildDirectBranches(const RS_Entity& source, const LC_CurveOffsetRequest& request,
                                                 const LC_CurveOffsetOptions& options,
                                                 const LC_OffsetSourceBudget& budget);

/**
 * Fresh, parentless entities for @p geometry, a successful result of
 * buildDirectBranches() (or of a trimmed build) for the same source and
 * options: one entity per branch. That is an RS_Line for a single straight
 * piece, a degree-1 RS_Spline through the corners of an all-straight branch,
 * and otherwise a clamped cubic RS_Spline whose knots are the integers, each of
 * multiplicity 3, so span i holds piece i exactly. A closed branch is an open
 * spline whose ends meet, starting inside a piece rather than at a corner. The
 * entities copy only the source's pen and layer. Each piece is compared with
 * the exact offset once more at parameters the fitter never used and from both
 * sides: paired at the same source parameter, and from the piece back to the
 * nearest offset point. A piece that fails is split and refitted in a local
 * copy; @p geometry is not changed. Output is counted against @p budget as it is
 * made. On any failure no entity is returned.
 */
LC_CurveOffsetMaterializationResult materializeBranches(const RS_Entity& source,
                                                        const LC_CurveOffsetGeometryResult& geometry,
                                                        const LC_CurveOffsetOptions& options,
                                                        const LC_OffsetSourceBudget& budget);

/**
 * RS_Entity::createOffset() for splines: the Direct offset through @p coord at
 * |@p distance| with default options and limits. The entities are released
 * only when the whole result is valid; any failure gives an empty vector.
 * Callers that must tell a failure from "not handled" use createEntities().
 */
std::vector<RS_Entity*> createLegacyOffset(const RS_Entity& source, const RS_Vector& coord, double distance);

/** buildDirectBranches(), then materializeBranches(): the programmatic entry point. */
LC_CurveOffsetMaterializationResult createEntities(const RS_Entity& source, const LC_CurveOffsetRequest& request,
                                                   const LC_CurveOffsetOptions& options,
                                                   const LC_OffsetSourceBudget& budget);

} // namespace LC_CurveOffset

#endif
