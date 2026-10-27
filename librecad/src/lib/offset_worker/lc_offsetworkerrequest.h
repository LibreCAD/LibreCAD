/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERREQUEST_H
#define LC_OFFSETWORKERREQUEST_H

#include "lc_offsetcachetrim.h"
#include "lc_offsetcurve.h"
#include "rs_modification.h"
#include "rs_arc.h"
#include "rs_circle.h"

#include <QCborArray>
#include <optional>
#include <variant>

/** Owned request values only. Syntax admission is NOT a geometric certificate. */
namespace LC_OffsetWorkerRequest {
struct Token {
    qint64 requestId = 0;
    qint64 generation = 0;
    qint64 sourceRevision = 0;
    qint64 undoEpoch = 0;
};
struct Segment { RS_Vector start, end; };
using Cutter = std::variant<Segment, RS_ArcData, RS_CircleData, RS_EllipseData, RS_SplineData>;
struct Generate {
    LC_EllipseOffsetDefinition definition;
    LC_OffsetRunLimits limits;
    std::optional<RS_Vector> throughPoint;
};
struct ValidateCache {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
    double worldTolerance = 0;
    LC_OffsetCheckLimits limits;
};
struct TrimCache {
    RS_SplineData cache;
    Cutter cutter;
    LC_OffsetCacheTrim::Pick pick;
    LC_OffsetCacheTrim::Limits limits;
};
struct TrimConic {
    RS_SplineData cache;
    Cutter conic; // Only Arc, Circle and Ellipse are admitted.
    LC_OffsetCacheTrim::Pick conicPick;
    std::optional<LC_OffsetCacheTrim::Pick> cachePick;
    LC_OffsetCacheTrim::Limits limits;
};
struct CheckThrough {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
    double worldTolerance = 0;
    RS_Vector point;
    LC_OffsetCheckLimits limits;
};
/** Geometry-only source series. Bounds preserve the original drawn-curve side
 * policy without repairing raw input or constructing a synthetic document.
 */
struct Produce {
    std::variant<RS_EllipseData, RS_SplineData> source;
    LC_OffsetSourceContext context;
    std::optional<RS_Vector> minimum, maximum;
    RS_Vector coordinate;
    std::optional<RS_Vector> fallback;
    double distance = 0;
    int copies = 1;
    bool asOffsetCurve = false, preview = false;
    LC_OffsetBatchLimits limits;
};
struct Request {
    Token token;
    std::variant<Generate, ValidateCache, TrimCache, TrimConic, CheckThrough, Produce> operation;
};

/** Shared strict value syntax, NOT C certificates or geometric proof.
 * These use exactly the request layouts and admission caps, including latent
 * fields. No geometry is normalized or repaired; refusals have no partial value.
 */
namespace Values {
std::optional<QCborArray> encodeCache(const RS_SplineData&);
std::optional<RS_SplineData> decodeCache(const QCborArray&);
std::optional<QCborArray> encodeDefinition(const LC_EllipseOffsetDefinition&);
std::optional<LC_EllipseOffsetDefinition> decodeDefinition(const QCborArray&);
std::optional<QCborArray> encodePick(const LC_OffsetCacheTrim::Pick&);
std::optional<LC_OffsetCacheTrim::Pick> decodePick(const QCborArray&);
std::optional<QCborArray> encodeCutter(const Cutter&);
std::optional<Cutter> decodeCutter(const QCborArray&);
}

/** Envelope [4, [requestId,generation,sourceRevision,undoEpoch], op, body].
 * Bodies: [definition,run,throughPoint-or-null], [cache,definition,worldTolerance,check],
 * [cache,cutter,pick,trim], [cache,conic,conicPick,cachePick-or-null,trim].
 * CheckThrough (op 4): [cache,definition,worldTolerance,point,check].
 * Produce (op 5): [source,context,bounds,coordinate,fallback-or-null,distance,
 * copies,asOffsetCurve,preview,batchLimits].
 * Source [0 ellipse / 1 raw spline,record]; context [topLevel,nativeSource,definition-or-null];
 * bounds [minimum-or-null,maximum-or-null]; batchLimits [outputBudget,samples,pairs,requestDeep].
 * Copies are 1..100. Signed distances are finite; zero/overflow are typed kernel refusals.
 * No attributes, document identity, cache import or publication authority is transported.
 * Cutter [ordinal 0..4,record]; vector [x,y,z,valid]; pick [point,hint-or-null,seed-or-null].
 * Definition [basis,distance,towardCentre,cacheReversed,relTolerance].
 * Spline [controls,knots,weights,fit,degree,type,savedOpenType,savedOpenKnots,closed,periodic].
 * Arc/ellipse/circle and budget arrays follow their native data member order,
 * including rendering caches. Run [outputBudget,maxSamples,maxIntersectionPairs,check].
 * Record arrays have exact arity; collection arrays have bounded counts.
 * Doubles are finite CBOR doubles, never integers. Raw floating-point width and
 * framing are checked separately by LC_OffsetWorkerWire before typed decoding.
 * Spline degree and worldTolerance are transported without geometric validation.
 * Refusals return no partial request. Only allocation exceptions may propagate.
 */
std::optional<QCborArray> encode(const Request&);
std::optional<Request> decode(const QCborArray&);
/** Allocation-free admission for already owned values; still not geometry validation. */
bool admitted(const Request&);
}
#endif
