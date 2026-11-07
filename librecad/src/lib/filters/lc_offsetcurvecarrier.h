/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVECARRIER_H
#define LC_OFFSETCURVECARRIER_H

#include <cstddef>
#include <optional>

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

class DRW_Spline;

namespace LC_OffsetCurveCarrier {

enum class Refusal {
    None,
    NotSpline,
    UnsupportedDegree,
    DeclaredCountMismatch,
    ResourceLimit,
    FitData,
    UnsupportedMode,
    InvalidAuxiliaryData,
    InvalidLayout,
    NullControl,
    NonFiniteControl,
    Non2DControl,
    IncompatibleNormal,
    InvalidKnots,
    InvalidWeights,
    RepairDependentClosure
};

struct Result {
    std::optional<RS_SplineData> data;
    Refusal refusal = Refusal::NotSpline;
    // Index within the offending control/knot/weight array; absent for scalar/layout failures.
    std::optional<std::size_t> item;
    explicit operator bool() const noexcept { return refusal == Refusal::None && data.has_value(); }
};

/** Inspect the ORIGINAL carrier, before any projection, rounding or converter repair.
 * Only clamped degree-1/3 polynomial Bezier chains are eligible. Controls and
 * knots are copied unchanged; absent unit weights are materialized. File closure
 * flags are retained on ClampedOpen data, never by synthesizing wrapped controls.
 * The zero normal sentinel and any finite normal parallel to the Z axis are
 * compatible only after every original control passes the exact raw 2D check.
 * DRW has no saved-open representation: the output's saved-open state remains
 * empty/default, and input layouts needing clamping/wrapping/fit repair refuse.
 *
 * maxCacheSpans/maxVertices may lower, never raise, C's default storage caps.
 * Knot storage/scans are also capped at maxVertices+degree+1. Other limit fields
 * are unused: this is a bounded value conversion, not an adaptive C proof.
 * No entity, metadata, borrowed pointer, checker result or editability is created.
 * Refusals contain no partial data. Allocation exceptions propagate unchanged.
 */
Result fromRawSpline(const DRW_Spline &raw, const LC_OffsetCheckLimits &limits = {});

} // namespace LC_OffsetCurveCarrier
#endif
