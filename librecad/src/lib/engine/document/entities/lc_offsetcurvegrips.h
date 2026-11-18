/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVEGRIPS_H
#define LC_OFFSETCURVEGRIPS_H

#include <vector>

#include "rs_vector.h"

struct RS_SplineData;
struct LC_EllipseOffsetDefinition;
namespace LC_EllipseOffsetGeometry { class CacheTopologyAnalysis; }

namespace LC_OffsetCurveGrips {

struct Prepared {
    std::vector<RS_Vector> grips;
    RS_Vector midpoint{false};
};

/**
 * Prepare value-owned display/snap hints before native state publication.
 * The caller must already have C-certified this exact normalized unit-weight
 * Bezier cache and definition. This function is not a validator or an adoption
 * token and never invokes the engine, C, entity constructors or mutable caches.
 *
 * Pass that same C result's topology to include retained PairEvent corners
 * (mandatory for complete inward-lens grips). It is borrowed only during this
 * call; its raw cache controls and closure are checked for accidental mismatch.
 * Without topology no trim-corner identity is inferred from fitting tangents.
 *
 * Analytic quarter vertices and the arc midpoint require an exact actual-cache
 * point within tolerance. Hidden arc midpoints fall back to the nearest of a
 * bounded set of cache evaluations. midpoint uses sampled cache arc length,
 * not a certified equal-length solve. Fallback coordinates are rounded exact
 * Bezier evaluations whose rounding distance is itself checked against tolerance.
 * If an ideal quarter lies only within C's admitted 2T band, use a checked
 * closest-cache projection instead of exposing that unproved ideal coordinate.
 * Visibility witnesses are not ordering keys. Lines use exact nearest segment
 * projections; cubics use bounded exact distance/derivative subdivision to
 * enclose all possible closest parameters. Unresolved or overlapping optional
 * correspondences are omitted, with known cache parameters taking priority.
 * Grips follow the stored cache direction; cacheReversed is not applied twice.
 * Deduplication uses four local-scale ULPs, capped at tolerance/16, never an
 * allowance proportional to world-center magnitude.
 *
 * Hard bounds: 4096 spans, 8192 sampling segments, 65536 shared visibility and
 * correspondence work units (never reset per grip),
 * subdivision depth 20, 65536 topology records per list, and 256 result grips.
 * Unsupported input or topology/sampling refusal returns an empty Prepared.
 * Unproved analytic candidates, including shared-budget exhaustion, are
 * omitted. Allocation exceptions propagate;
 * callers must finish preparation before publishing any native state.
 */
Prepared prepare(const RS_SplineData &cache, const LC_EllipseOffsetDefinition &definition,
                 double tolerance, bool closed,
                 const LC_EllipseOffsetGeometry::CacheTopologyAnalysis *topology = nullptr);

/**
 * Recompute only the same bounded sampled-arc-length midpoint from the cache.
 * Useful after exact cache transforms: a previously rounded hint need not be
 * an exact affine image. No definition, topology, engine or C is consulted.
 * Returns invalid on unsupported input or an unrepresentable rounding witness;
 * allocation exceptions propagate. The same C-certified-cache premise applies.
 */
RS_Vector prepareMiddle(const RS_SplineData &cache, double tolerance, bool closed);

} // namespace LC_OffsetCurveGrips
#endif
