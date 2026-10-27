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
struct Native : LC_OffsetCurveEditResult {
    std::optional<Curve> curve;
    LC_OffsetOutputUsage attemptedOutputUsage{};
    bool attemptedDeepCountComplete = true;
    LC_OffsetOutputUsage outputUsage{};
    std::size_t sampleUnits = 0;
};
/** A failed computation/serialization may have lost its exact work snapshot.
 * Such a reply has no result and must not refund an operation's reservation.
 */
enum class Failure { None, AllocationFailure, InternalFailure };
struct Reply {
    LC_OffsetWorkerRequest::Token token;
    std::size_t operation = 0;
    Failure failure = Failure::InternalFailure;
    std::variant<std::monostate, Native, LC_OffsetCacheTrim::Result> result;
};

/** Matched envelope [2,token,operation,failure,payload-or-null].
 * Native payload [refusal,engineStatus-or-null,checkReason-or-null,checkWork,
 * throughWork,engineRuns,changed,curve-or-null,attemptedOutputUsage,
 * attemptedDeepCountComplete,outputUsage,sampleUnits].
 * CheckThrough uses Native with only refusal and bounded throughWork populated;
 * success has no curve and changed=false, and grants no cache admission authority.
 * Curve [cache,definition,worldTolerance,closed,grips,midpoint].
 * Trim payload [refusal,fragments,conicFragments,roots,removed-or-null,
 * rootCount,fragmentCount,work]. All records retain their native field order.
 * Strict value/status/count validation only, never geometric certification.
 */
std::optional<QCborArray> encode(const Reply&, const LC_OffsetWorkerRequest::Request&);
std::optional<Reply> decode(const QCborArray&, const LC_OffsetWorkerRequest::Request&);
}
#endif
