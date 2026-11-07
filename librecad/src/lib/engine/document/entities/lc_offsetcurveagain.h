/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVEAGAIN_H
#define LC_OFFSETCURVEAGAIN_H

#include <optional>

#include "lc_curveoffset.h"
#include "lc_ellipseoffsetgeometry.h"

namespace LC_OffsetCurveAgain {

enum class Refusal {
    None, InvalidDefinition, NonCanonicalBasis, InvalidIncrement, UnresolvedSide,
    InvalidCurrentProfile, ZeroSum, NonFiniteCombination, InexactCombination,
    FirstNotProvedUntrimmed, FinalNotProvedUntrimmed
};
enum class Proof {
    WholeOutwardFirst, WholeInwardErosion, WholeUntrimmedFirst, ArcUntrimmed
};
struct Proposal {
    LC_EllipseOffsetDefinition definition;
    double originalSignedOutward;
    double incrementSignedOutward;
    double combinedSignedOutward;
    Proof proof;
};
struct Result {
    Refusal refusal = Refusal::InvalidDefinition;
    std::optional<Proposal> proposal;
    explicit operator bool() const noexcept { return refusal == Refusal::None && proposal.has_value(); }
};

/** A structural composition proposal, NOT a geometry/adoption/checker result.
 * The caller must first require an editable, consistent LC_OffsetCurve. Its
 * definition must be canonical (normalized axes/angles). This value-only API
 * cannot discover undefined/stale caches and grants no trusted-adoption bypass.
 * Resolve Left/Right on the DISPLAYED cache before calling. The physical outward
 * increment uses basis.reversed XOR cacheReversed exactly once; towardCentre
 * already describes a physical side and needs no further parity adjustment.
 *
 * Retained-set proof for a whole convex ellipse K, with B_r a closed disk:
 * - An outward first profile is K+B_a. Its inward offset by b is the intersection
 *   of support halfplanes h_K+a-b, hence the retained profile at signed a-b;
 *   another outward offset follows disk-addition associativity.
 * - (K erosion B_a) erosion B_b = K erosion (B_a+B_b). Further inward steps
 *   therefore compose EVEN IF the first whole inward profile was trimmed.
 * - Dilation does not invert a trimmed erosion. An outward step after an inward
 *   first profile is admitted only when a < min rho = |major|*ratio^2, so the
 *   first profile still has support function h_K-a without discarded normals.
 *
 * Arcs require both rho+D_first>0 and rho+D_total>0 on their SAME directed
 * sweep. The exact minimum is at a major-axis tip, if included, or an endpoint.
 * If a major-axis minimum is included, these inequalities give a regular convex
 * whole parallel curve. Otherwise the arc lies between consecutive major tips,
 * with normal turn < pi: the rolling-disk inequality for rho>=r gives distance
 * >=r to every point of the arc, including its endpoints. Thus neither pass
 * loses intervals to local loops, another normal, or an endpoint cap. Endpoints
 * remain C(end)+D*N(end), and tangents keep their orientation. Checking only a
 * sampled curvature, or only the new increment against the original rho, would
 * not establish this result. Strict interval margins refuse uncertainty.
 * In normal-angle coordinates the rolling-disk claim follows from
 * C(theta)-(C(theta0)-r*N(theta0)) = r*N(theta) + integral((rho-r)*T):
 * N(theta) has nonnegative dot product with that directed integral when the
 * normal turn is < pi, so the squared norm is >=r^2. For the second inward
 * pass use rho_first=rho+D_first and r=D_first-D_total. Outward passes use
 * supporting tangents of the same convex arc instead.
 *
 * Fixed work: bounded double/exact-dyadic arithmetic, one sweep classification,
 * and at most four existing analytic endpoint-factor queries. No entity copying,
 * materialization, topology/checker work, retries or cache traversal. The signed
 * sum must itself be representable as a double; zero is a separate refusal.
 * A whole final profile may still be empty or fail producer/checker admission.
 * The parent may attempt this ONE candidate once, then its ONE existing plain
 * cache fallback, charging actual attempted work/output independently.
 */
Result propose(const LC_EllipseOffsetDefinition &source,
    LC_CurveOffsetSide resolvedCacheSide, double incrementMagnitude);

} // namespace LC_OffsetCurveAgain
#endif
