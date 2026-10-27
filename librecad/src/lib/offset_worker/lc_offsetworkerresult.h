/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERRESULT_H
#define LC_OFFSETWORKERRESULT_H

#include "lc_offsetworkerrequest.h"

/** Result values are NOT admission capabilities. Only the owning client may
 * trust a matched reply from the fixed built helper after successful process
 * completion; arbitrary decoded values cannot construct an editable entity.
 */
namespace LC_OffsetWorkerResult {
struct Curve {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
    double worldTolerance = 0;
    bool closed = false;
    std::vector<RS_Vector> grips;
    RS_Vector midpoint{false};
};
enum class OrdinaryStatus { None, Complete, Unsupported };
struct Native : LC_OffsetCurveEditResult {
    std::optional<Curve> curve;
    LC_OffsetOutputUsage attemptedOutputUsage{};
    bool attemptedDeepCountComplete = true;
    LC_OffsetOutputUsage outputUsage{};
    std::size_t sampleUnits = 0;
    OrdinaryStatus ordinaryStatus = OrdinaryStatus::None;
    std::vector<LC_OffsetWorkerRequest::Cutter> ordinary;
    /** Original producer usage, before seam splitting/collinear compaction;
     * serialized stored spans/controls/knots are independently bounded.
     */
    LC_OffsetOutputUsage ordinaryUsage{};
};
/** Pointer-free numeric source receipt. No document or deletion authority. */
struct SourceDiagnostics {
    LC_OffsetSourceStatus status = LC_OffsetSourceStatus::InvalidSource;
    LC_CurveOffsetStatus engineStatus = LC_CurveOffsetStatus::Ok;
    LC_OffsetFailureDetail detail = LC_OffsetFailureDetail::None;
    LC_OffsetSourceStatus stoppedBy = LC_OffsetSourceStatus::Succeeded;
    LC_OffsetOutputUsage usage{};
    int copiesMade = 0, copiesRequested = 0;
    bool sourceRemoved = false;
    int copiesKeptPlain = 0;
    std::size_t rebaseEngineRuns = 0, cacheEngineRuns = 0, reusedCacheCopies = 0;
    std::size_t sampleUnits = 0, intersectionPairAllowance = 0;
    LC_OffsetOutputUsage attemptedOutputUsage{};
    bool attemptedDeepCountComplete = true;
    std::size_t adoptionAttempts = 0;
    LC_EllipseOffsetGeometry::CheckWork adoptionCheckWork{};
    std::optional<LC_CurveOffsetSide> resolvedSide;
};
using ProducedEntity = std::variant<LC_OffsetWorkerRequest::Segment, RS_SplineData, Curve>;
struct Produced {
    SourceDiagnostics source;
    std::vector<std::size_t> copyOutputCounts;
    std::vector<LC_OffsetPlainOutcome> keptPlain;
    std::vector<LC_OffsetRebaseOutcome> rebases;
    OrdinaryStatus outputStatus = OrdinaryStatus::None;
    std::vector<ProducedEntity> outputs;
};
/** Bounded value admission only, never native materialization authority. */
bool admittedProduced(const Produced&, const LC_OffsetWorkerRequest::Produce&);

/** A failed computation/serialization may have lost its exact work snapshot.
 * Such a reply has no result and must not refund an operation's reservation.
 */
enum class Failure { None, AllocationFailure, InternalFailure };
struct Reply {
    LC_OffsetWorkerRequest::Token token;
    std::size_t operation = 0;
    Failure failure = Failure::InternalFailure;
    std::variant<std::monostate, Native, LC_OffsetCacheTrim::Result, Produced> result;
};

/** Matched envelope [4,token,operation,failure,payload-or-null].
 * Native payload [refusal,engineStatus-or-null,checkReason-or-null,checkWork,
 * throughWork,engineRuns,changed,curve-or-null,attemptedOutputUsage,
 * attemptedDeepCountComplete,outputUsage,sampleUnits,ordinaryStatus,ordinary,ordinaryUsage].
 * Ordinary values use Cutter records, restricted to lines and normalized raw
 * degree-1/3 ClampedOpen splines. Complete/Unsupported are Generate-only native
 * refusals after one successful producer run; Unsupported carries no values.
 * CheckThrough uses Native with only refusal and bounded throughWork populated;
 * success has no curve and changed=false, and grants no cache admission authority.
 * Curve [cache,definition,worldTolerance,closed,grips,midpoint].
 * Trim payload [refusal,fragments,conicFragments,roots,removed-or-null,
 * rootCount,fragmentCount,work]. All records retain their native field order.
 * Produce payload [diagnostics,copyOutputCounts,keptPlain,rebases,outputStatus,outputs].
 * Diagnostics retain SourceDiagnostics member order (19 fields); plain records
 * [copy,reason,checkReason]; rebase records retain LC_OffsetRebaseOutcome order.
 * Each output is [0 Segment / 1 raw Spline / 2 native Curve,record]. Copy counts
 * partition outputs exactly; native records bind to that ordinal and actual side.
 * None is failed-source output only; Unsupported retains all diagnostics/counts
 * but no output prefix. Work lost to a fatal failure never refunds reservations.
 * Strict value/status/count validation only, never geometric certification.
 */
std::optional<QCborArray> encode(const Reply&, const LC_OffsetWorkerRequest::Request&);
std::optional<Reply> decode(const QCborArray&, const LC_OffsetWorkerRequest::Request&);
}
#endif
