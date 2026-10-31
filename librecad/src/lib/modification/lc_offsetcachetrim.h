/*******************************************************************************
 * This file is part of LibreCAD, distributed under GPL version 2 or later.
 * Copyright (C) 2026 LibreCAD.org
 ******************************************************************************/
#ifndef LC_OFFSETCACHETRIM_H
#define LC_OFFSETCACHETRIM_H

#include "lc_parametriccurveintersection.h"
#include "rs_spline.h"
#include "rs_ellipse.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class RS_Entity;

/** Pure, bounded preparation of ordinary open splines from a normalized cache.
 * No native eligibility, document mutation, attributes, refitting or materialization.
 * Parameters follow the STORED controls, not the native definition's traversal flag.
 */
namespace LC_OffsetCacheTrim {

struct Limits {
    std::size_t maxSpansPerCurve = 4096; // raw spans; a conic uses eight bounded patches
    std::size_t maxInputSpans = 8192;
    std::size_t maxInputRecords = 131072;
    std::size_t maxBoxPairs = std::size_t{1} << 20;
    std::size_t maxLeaves = 65536;
    std::size_t maxGeneratedItems = 65536; // core AND kernel temporary records
    std::size_t maxOrderingComparisons = std::size_t{1} << 20;
    // Shared kernel snapshot cap; core discovery is separately item-bounded.
    std::size_t maxRoots = 4096;
    unsigned maxDepth = 48;
    unsigned maxLeafDepth = 40;
    std::size_t maxFragments = 2; // per replaced operand; a paired result may contain four
    std::size_t maxOutputSpans = 4097;
    std::size_t maxOutputControls = 16384;
    std::size_t maxOutputKnots = 32768;
};

struct Pick {
    RS_Vector worldPoint;
    // Optional seeds only. Neither narrows the exhaustive geometry query.
    std::optional<std::size_t> rawSpanHint;
    std::optional<double> localParameterSeed;
};

struct Location {
    std::size_t rawSpan = 0;
    LC_Interval localParameter;
};

enum class EndLabel : unsigned { SourceStart = 1, SourceEnd = 2, CutterRoot = 4 };

struct Endpoint {
    unsigned labels = 0;
    Location source;
    std::optional<std::size_t> rootId;
    LC_Interval imageX, imageY; // world bounds of the actual retained endpoint/root
};

struct Section {
    std::size_t rawSpan = 0;
    double from = 0, to = 1; // representable cuts in the original raw span
    LC_Interval originalDomain;
};

struct Fragment {
    RS_SplineData data; // owned, unit weights, integer breaks, open/clamped
    std::vector<Section> sourceSections;
    Endpoint start, end;
};

struct RootProvenance {
    std::size_t id = 0;
    Location target;
    // Cache segment k uses [k,k+1]; conics use ReferenceFrame sweep parameters.
    LC_IntersectionDomain isolation, proofDomain;
    LC_IntersectionProof proof = LC_IntersectionProof::Uncertified;
    LC_Interval worldX, worldY;
};

struct RemovedInterval {
    Location from, to;
    bool crossesStorageSeam = false;
};

enum class Refusal {
    None, InvalidInput, UnsupportedLayout, UnsupportedCutter, NoUsableRoots,
    AmbiguousPick, AmbiguousRoots, TangencyOrOverlap, EmptyOutput,
    Arithmetic, LimitExceeded, Cancelled, AllocationFailure
};

struct Work {
    std::size_t inputSpans = 0, inputRecords = 0;
    std::size_t rootRecords = 0;
    std::size_t kernelItems = 0, kernelComparisons = 0;
    std::size_t outputSpans = 0, outputControls = 0, outputKnots = 0;
    LC_IntersectionWorkStats intersections;
};

struct Cancellation {
    std::shared_ptr<const std::atomic_bool> requested;
};

struct Result {
    struct ConicFragment {
        // Preserve the ORIGINAL conic axes/ratio; only angular bounds change.
        RS_EllipseData data;
        LC_Interval from, to;
        std::optional<std::size_t> startRoot, endRoot;
        bool crossesStorageSeam = false;
    };
    Refusal refusal = Refusal::InvalidInput;
    std::vector<Fragment> fragments;
    std::vector<ConicFragment> conicFragments;
    // Diagnostics, NOT transferable proof tokens. No session escapes prepare().
    std::vector<RootProvenance> roots;
    std::optional<RemovedInterval> removed;
    std::size_t rootCount = 0, fragmentCount = 0;
    Work work;
};

/** Caller limits can only LOWER the fixed Limits{} ceilings; zero is exhausted.
 * Supported cutters: finite line, arc/circle/ellipse, normalized unit-weight
 * degree-1/3 Bezier-chain spline (including a native entity's displayed cache).
 * Exact endpoint closure determines clamped-cache topology, including a closed
 * producer cache with its DXF closed flag unset. Closed/periodic flags cannot
 * claim an open chain; arbitrary rational/wrapped splines refuse.
 * Noncanonical near-full conic turns and unproved conic endpoints refuse;
 * the native reference utility's full-turn admission band is not an extension.
 * Every refusal has empty fragments, fragmentCount==0 and no removed interval.
 * Cuts stay inside certified isolations; only exact singleton parameters identify
 * source endpoints. A rounded constant retained span refuses the whole result.
 * Cancellation is cooperative at geometry/staging checkpoints, not instant.
 */
Result prepare(const RS_SplineData& target, const RS_Entity& cutter, const Pick&,
               const Limits& = {}, Cancellation = {}, Work* onExit = nullptr);

/** One owned strict contact session, one shared work budget, two ORIGINAL
 * geometries. Always prepare the typed arc/ellipse complement; prepare the
 * polynomial cache complement only when cachePick is supplied (Trim Two).
 * A successful conic-only result has empty fragments and nonempty conicFragments.
 * Conic fragments charge outputSpans/generated items, not polynomial arrays.
 * No diagnostic roots are accepted as proof input, no refit/producer/C call.
 */
Result prepareConic(const RS_SplineData& cache, const RS_Entity& conic,
    const Pick& conicPick, const std::optional<Pick>& cachePick = {},
    const Limits& = {}, Cancellation = {}, Work* onExit = nullptr);

} // namespace LC_OffsetCacheTrim
#endif
