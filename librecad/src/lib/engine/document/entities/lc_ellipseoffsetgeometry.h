/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ******************************************************************************/

#ifndef LC_ELLIPSEOFFSETGEOMETRY_H
#define LC_ELLIPSEOFFSETGEOMETRY_H

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "lc_curvejet.h"
#include "lc_parametriccurveintersection.h"
#include "rs_ellipse.h"

struct RS_SplineData;

struct LC_EllipseOffsetDefinition {
    RS_EllipseData basis;
    double distance = 0.0;
    bool towardCentre = false;
    bool cacheReversed = false;
    double relTolerance = 0.0;
};

struct LC_OffsetCheckLimits {
    std::size_t maxCacheSpans = 4096;
    std::size_t maxVertices = 65536;
    std::size_t maxDistancePairs = std::size_t{1} << 20;
    unsigned maxDepth = 20;
    std::size_t maxOrderingComparisons = std::size_t{1} << 20;
};

namespace LC_EllipseOffsetGeometry {

struct Sweep {
    double start = 0.0;
    double signedSweep = 0.0;
    bool whole = false;

    bool contains(double parameter, double angularAllowance = 0.0) const;
};

enum class BudgetStatus { Ok, InvalidInput, LimitExceeded };
enum class SubdivisionKind { Candidate, Proof };

struct CheckWork {
    std::size_t cacheSpans = 0;
    std::size_t vertices = 0;
    std::size_t distancePairs = 0;
    unsigned deepest = 0;
    std::size_t orderingComparisons = 0;
};

/** One irreversible allowance shared by all stages of a reference/cache query. */
class WorkBudget {
public:
    explicit WorkBudget(const LC_OffsetCheckLimits& limits = {});
    WorkBudget(const WorkBudget&) = delete;
    WorkBudget& operator=(const WorkBudget&) = delete;

    BudgetStatus status() const;
    const CheckWork& consumed() const;
    std::size_t remainingPieces() const;
    std::size_t remainingVertices() const;
    std::size_t remainingPairs() const;
    std::size_t remainingOrderingComparisons() const;
    bool consumePieces(std::size_t count);
    bool consumeVertices(std::size_t count);
    bool consumePairs(std::size_t count);
    bool consumeOrderingComparisons(std::size_t count);
    /** Atomically add counters and retain the absolute maximum depth, even if delta aliases consumed(). */
    bool adoptWork(const CheckWork& delta);
    /** Reserve both child slots first; a popped parent is not another generated record. */
    bool admitSubdivision(SubdivisionKind kind);
    bool checkDepth(unsigned depth);
    /** Check a combined depth without unsigned addition overflow. */
    bool checkDepth(unsigned cacheDepth, unsigned referenceDepth);
    unsigned depthLimit() const;
    void latchLimitExceeded();
    void latchInvalidInput();

private:
    friend class CriticalBuilder;
    friend class CriticalRefinementSession;
    bool consume(std::size_t count, std::size_t limit, std::size_t& used);

    LC_OffsetCheckLimits m_limits;
    CheckWork m_work;
    BudgetStatus m_status = BudgetStatus::Ok;
    std::shared_ptr<const void> m_identity;
};

struct CacheLayoutCounts {
    std::size_t spans = 0;
    std::size_t controls = 0;
    std::size_t knots = 0;
};

/** Nonallocating checked formulas for normalized degree-1/degree-3 cache layouts. */
std::optional<CacheLayoutCounts> cacheLayoutCounts(std::size_t degree, std::size_t spans) noexcept;
std::optional<CacheLayoutCounts> cacheLayoutCountsFromControls(std::size_t degree,
                                                            std::size_t controls) noexcept;

enum class CacheLayoutStatus { Ok, InvalidInput, InvalidLayout, LimitExceeded };

struct CacheSpan {
    std::size_t degree = 0;
    /** First degree+1 entries are the exact line/Bezier controls, without refitting. */
    std::array<RS_Vector, 4> controls;
    LC_Interval parameter;
};

/** Layout only: Ok does not certify geometry, distance, closure or topology. */
struct CacheLayoutResult {
    CacheLayoutStatus status = CacheLayoutStatus::InvalidInput;
    /** Copied span data; no borrowed source and no partial spans on refusal. */
    std::vector<CacheSpan> spans;
    CheckWork work{};
    std::size_t spanCount() const noexcept { return spans.size(); }
};

/** Inspect already normalized spline data; raw-file repair and RS_Line conversion are not performed. */
CacheLayoutResult inspectCacheLayout(const RS_SplineData& cache, WorkBudget& budget);

/**
 * Exact ellipse/outward-offset formulas in centre-relative, uniformly scaled,
 * world-aligned coordinates. No fitted controls or topology are represented.
 * The basis must already have normalized axes (0 < ratio <= 1) and arc angles
 * in [0, 2*pi); canonical whole and genuine full-turn inputs are accepted.
 */
class ReferenceFrame {
public:
    static std::optional<ReferenceFrame> create(const RS_EllipseData& basis, double signedDistance);
    /** Value-only reparameterization of a whole curve. Open frames and noncanonical starts refuse. */
    std::optional<ReferenceFrame> withWholeStart(double canonicalAngle) const;

    double scale() const;
    double parameterEnd() const;
    const Sweep& sourceSweep() const;
    /** Positive |d(physical angle)/d(stored parameter)|, enclosing the exact input sweep. */
    const LC_Interval& traversalRate() const;
    /** Unwrapped start + signed rate*t, including a whole closing end; cleared on refusal. */
    bool boundUnwrappedAngle(double t0, double t1, LC_Interval& out) const;
    /** Engine distances are positive along the traversal's left normal. */
    double engineSignedDistance() const;

    /** Jets are midpoint estimates of certified zero-width bounds, for numerical seeds only. */
    bool sourceJet(double t, LC_CurveJet& out) const;
    bool offsetJet(double t, LC_CurveJet& out) const;
    /** Continuous exact-sweep bounds, including zero-width queries and periodic endpoint limits. */
    bool boundSourceJet(double t0, double t1, LC_CurveJetBounds& out) const;
    bool boundOffsetJet(double t0, double t1, LC_CurveJetBounds& out) const;
    /** Physical offset factor and its traversal derivative; both cleared on refusal. */
    bool boundOffsetFactor(double t0, double t1, LC_Interval& factor, LC_Interval& derivative) const;
    /** Enclosures of conversions of exact input doubles; cleared on refusal. */
    bool pointToFrame(const RS_Vector& world, LC_Interval& x, LC_Interval& y) const;
    bool pointToWorld(const RS_Vector& local, LC_Interval& x, LC_Interval& y) const;
    /** Convert provider/local bounds; centre addition occurs only here. Cleared on refusal. */
    bool pointToWorld(const LC_Interval& localX, const LC_Interval& localY,
                      LC_Interval& worldX, LC_Interval& worldY) const;

private:
    ReferenceFrame() = default;
    bool trig(double t0, double t1, LC_Interval& cosine, LC_Interval& sine) const;
    bool offsetFactor(const LC_Interval& cosine, const LC_Interval& sine,
                      LC_Interval& factor, LC_Interval& derivative, LC_Interval& speed) const;

    RS_Vector m_centre;
    Sweep m_sweep;
    double m_endAngle = 0.0;
    LC_Interval m_rate;
    LC_Interval m_rateSquared;
    LC_Interval m_rateCubed;
    double m_scale = 1.0;
    int m_scaleExponent = 0;
    double m_signedDistance = 0.0;
    LC_Interval m_majorX;
    LC_Interval m_majorY;
    LC_Interval m_axisSquared;
    LC_Interval m_axis;
    LC_Interval m_ratio;
    LC_Interval m_distance;
};

/** Outward local subcurve/chord data only; no visibility or traversal certificate. */
struct CurveChordBounds {
    LC_Interval startX, startY, endX, endY;
    LC_Interval x, y;
    LC_Interval deviation;
};
struct CacheBezierPoint { LC_Interval x, y; };

/** Fixed-cost proof primitives; callers admit their comparison work before calling. */
bool boundCacheChord(const CacheSpan& span, const ReferenceFrame& frame, double a, double b,
                     CurveChordBounds& out);
bool boundReferenceChord(const ReferenceFrame& frame, double a, double b, CurveChordBounds& out);
/** Bernstein derivative controls on a subdomain, in the original span's parameter convention. */
bool boundCacheDerivativeControls(const CacheSpan& span, const ReferenceFrame& frame, double a, double b,
                                 std::array<CacheBezierPoint, 4>& net, std::size_t& degree);

enum class CriticalRegime {
    PositiveFactor, MinimumStationary, IntermediateThresholds, MaximumStationary, NegativeFactor
};
enum class CriticalKind { SimpleCusp, CriticalStationary };
enum class CriticalProof { MonotonePolynomial, ExactAxisThreshold };
enum class InjectivityProof { RegularProjection, SingleCuspProjection, StationaryProjection };

struct CriticalOccurrence {
    LC_Interval parameter;
    CriticalKind kind;
    CriticalProof proof;
    int quadrant = 0;
    int winding = 0;
    bool atStart = false;
    bool atEnd = false;
};

struct CriticalRegion {
    LC_ParametricSegment segment;
    bool axisBand = false;
    InjectivityProof proof;
};

struct CriticalProvenance;

/** Prepared C1 geometry and whole-region proofs, not an exhaustive event/topology result. */
class CriticalAdapter final : public LC_ParametricCurves {
public:
    const ReferenceFrame& frame() const;
    CriticalRegime regime() const;
    const std::vector<CriticalOccurrence>& criticalOccurrences() const;
    const std::vector<CriticalRegion>& regions() const;
    const std::vector<LC_ParametricSegment>& segments() const override;
    std::size_t branchCount() const override;
    std::ptrdiff_t next(std::size_t branch) const override;
    bool evaluate(std::size_t segment, double t, RS_Vector& point, RS_Vector& derivative) const override;
    bool bound(std::size_t segment, double a, double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override;
    LC_AnalyticLeafCertificate certifyInjective(std::size_t segment, const LC_Interval& parameter) const override;
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override;

private:
    friend class CriticalBuilder;
    friend class ReferenceProviderBuilder;
    CriticalAdapter(const ReferenceFrame& frame, CriticalRegime regime);
    bool contains(std::size_t segment, const LC_Interval& parameter) const;
    struct JoinProof { double from; double to; };
    ReferenceFrame m_frame;
    CriticalRegime m_regime;
    std::vector<LC_ParametricSegment> m_segments;
    std::vector<CriticalRegion> m_regions;
    std::vector<CriticalOccurrence> m_occurrences;
    std::vector<JoinProof> m_joins;
    std::optional<JoinProof> m_seam;
    std::shared_ptr<const CriticalProvenance> m_provenance;
};

struct CriticalAnalysis {
    enum class Reason { None, CollapsedCircle, UnresolvedGeometry };
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    Reason reason = Reason::None;
    std::unique_ptr<const CriticalAdapter> adapter;
    /** Cumulative shared-budget snapshot, including failed and discarded work. */
    CheckWork work;
};

CriticalAnalysis analyseCritical(const RS_EllipseData& basis, double signedDistance, WorkBudget& budget);

struct CriticalCanonicalBounds {
    LC_Interval variable, inverseAngle;
    bool complement = false;
};

struct CriticalOccurrenceSnapshot {
    CriticalOccurrence occurrence{};
    /** Centre-relative, world-aligned provider coordinates, divided by frame.scale(). */
    LC_Interval imageX, imageY;
    std::optional<CriticalCanonicalBounds> canonical;
    unsigned admittedRounds = 0;
};

class CriticalOccurrenceHandle {
public:
    CriticalOccurrenceHandle() = default;
private:
    friend class CriticalRefinementSession;
    std::weak_ptr<const void> m_issuer;
    std::size_t m_index = std::size_t(-1);
};

enum class CriticalWorkSource { CurrentBudget, LastMatchingBudget };

struct CriticalContractionResult {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    std::optional<CriticalOccurrenceSnapshot> snapshot;
    /** Actual per-call charges and absolute entered depth, not differences of maxima. */
    CheckWork entered{};
    CheckWork work{};
    CriticalWorkSource workSource = CriticalWorkSource::LastMatchingBudget;
};

class CriticalRefinementSession {
public:
    ~CriticalRefinementSession();
    CriticalRefinementSession(CriticalRefinementSession&&) noexcept;
    CriticalRefinementSession& operator=(CriticalRefinementSession&&) noexcept;
    CriticalRefinementSession(const CriticalRefinementSession&) = delete;
    CriticalRefinementSession& operator=(const CriticalRefinementSession&) = delete;
    std::size_t occurrenceCount() const;
    CriticalOccurrenceHandle occurrenceHandle(std::size_t index) const;
    std::optional<CriticalOccurrenceSnapshot> snapshot(const CriticalOccurrenceHandle& handle) const;
    CriticalContractionResult contract(const CriticalOccurrenceHandle& handle, WorkBudget& budget);
    CheckWork lastMatchingWork() const;
private:
    friend class CriticalBuilder;
    struct Impl;
    explicit CriticalRefinementSession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

struct CriticalRetainedAnalysis {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    CriticalAnalysis::Reason reason = CriticalAnalysis::Reason::None;
    std::unique_ptr<const CriticalAdapter> adapter;
    std::unique_ptr<CriticalRefinementSession> refinement;
    CheckWork work{};
};

CriticalRetainedAnalysis analyseCriticalRetained(const RS_EllipseData& basis, double signedDistance,
                                               WorkBudget& budget);

enum class ReferenceRole { Own, Opposite, StartCutter, EndCutter };
enum class OppositeDisposition { Present, ExcludedCollapsedCircle };
enum class ReferencePreparationReason { None, CollapsedCircle, UnresolvedGeometry };
struct ReferenceProviderPreparation;

/** Owned reference geometry only: successful preparation does not certify a complete event set. */
class ReferenceProviders final : public LC_ParametricCurves {
public:
    ~ReferenceProviders() override;
    ReferenceProviders(const ReferenceProviders&) = delete;
    ReferenceProviders& operator=(const ReferenceProviders&) = delete;
    const ReferenceFrame& frame() const noexcept;
    std::optional<std::size_t> branchFor(ReferenceRole role) const noexcept;
    OppositeDisposition oppositeDisposition() const noexcept;
    /** Diagnostic own/offset-parameter proof range; copies do not confer proof authority. */
    std::optional<LC_Interval> contactProofInterval(ReferenceRole offset, ReferenceRole cutter) const noexcept;
    const std::vector<LC_ParametricSegment>& segments() const override;
    std::size_t branchCount() const override;
    std::ptrdiff_t next(std::size_t branch) const override;
    bool evaluate(std::size_t segment, double t, RS_Vector& point, RS_Vector& derivative) const override;
    bool bound(std::size_t segment, double a, double b, LC_Interval& x, LC_Interval& y,
               LC_Interval& dx, LC_Interval& dy) const override;
    /** Full local-coordinate jet bounds, including zero-width queries; cleared on refusal. */
    bool boundJet(std::size_t segment, double a, double b, LC_CurveJetBounds& out) const;
    /** Own offset only; preserves its shared factor and factory-proved global sign. */
    bool boundOwnProjectedDerivative(double a, double b, const LC_Interval& axisX,
        const LC_Interval& axisY, LC_Interval& out) const;
    LC_AnalyticOccurrenceIdentityCertificate certifyOccurrenceIdentity(
        const LC_ParametricIntersection&, bool, const LC_ParametricIntersection&, bool) const override;
    LC_AnalyticLeafCertificate certifyInjective(std::size_t segment, const LC_Interval& parameter) const override;
    LC_AnalyticIntersectionCertificate certifyIntersection(const LC_IntersectionDomain& box) const override;

private:
    friend class ReferenceProviderBuilder;
    struct Impl;
    explicit ReferenceProviders(std::unique_ptr<Impl> impl);
    friend class ReferenceEventBuilder;
    std::unique_ptr<const Impl> m_impl;
};

struct ReferenceProviderPreparation {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    ReferencePreparationReason reason = ReferencePreparationReason::None;
    std::unique_ptr<const ReferenceProviders> provider;
    std::unique_ptr<CriticalRefinementSession> ownCritical, oppositeCritical;
    /** Cumulative directly spent budget, never an additional debit. */
    CheckWork work{};
};

ReferenceProviderPreparation prepareReferenceProviders(const RS_EllipseData& basis, double signedDistance,
                                                       WorkBudget& budget);
/** Add complete-box construction-contact proofs, not exhaustive reference events. */
ReferenceProviderPreparation prepareReferenceContactProviders(const RS_EllipseData& basis, double signedDistance,
                                                              WorkBudget& budget);

/** Diagnostic indices only. Copies never confer authority to refine or merge roots. */
struct ReferenceOccurrenceId { std::size_t value = std::size_t(-1); };
struct ReferenceNodeId { std::size_t value = std::size_t(-1); };

struct ReferenceOccurrence {
    ReferenceRole role{};
    LC_Interval parameter;
    ReferenceNodeId node;
};
struct ReferenceNode {
    RS_Vector world;
    LC_Interval localX, localY, worldX, worldY;
};
struct ReferencePairEvent {
    ReferenceOccurrenceId first, second;
    LC_ParametricIntersection::Kind kind{};
    LC_IntersectionProof proof{};
};
struct ReferenceCriticalEvent {
    ReferenceOccurrenceId occurrence;
    CriticalKind kind{};
    CriticalProof proof{};
};
struct ReferenceFreeEnd { ReferenceOccurrenceId occurrence; bool atStart = false; };
struct ReferenceSeam { ReferenceOccurrenceId start, end; };

enum class ReferenceEventReason {
    None, CollapsedCircle, UnresolvedGeometry, UnresolvedIdentity, UnresolvedOrder,
    NodeAccuracy, ProofContradiction
};

/** Complete reference events and ordered occurrences, not retained/visible components. */
class ReferenceEventAnalysis {
public:
    ~ReferenceEventAnalysis();
    ReferenceEventAnalysis(const ReferenceEventAnalysis&) = delete;
    ReferenceEventAnalysis& operator=(const ReferenceEventAnalysis&) = delete;
    const ReferenceProviders& providers() const noexcept;
    /** Actual normalized tolerance passed to the owned strict query; not an identity allowance. */
    double localTolerance() const noexcept;
    const std::vector<ReferenceOccurrence>& occurrences() const noexcept;
    const std::vector<ReferenceOccurrenceId>& ownOccurrences() const noexcept;
    const std::vector<ReferenceNode>& nodes() const noexcept;
    const std::vector<ReferencePairEvent>& pairs() const noexcept;
    const std::vector<ReferenceCriticalEvent>& criticals() const noexcept;
    const std::vector<ReferenceFreeEnd>& freeEnds() const noexcept;
    const std::optional<ReferenceSeam>& seam() const noexcept;
private:
    friend class ReferenceEventBuilder;
    struct Impl;
    explicit ReferenceEventAnalysis(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> m_impl;
};

struct ReferenceEventResult {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    ReferenceEventReason reason = ReferenceEventReason::None;
    std::unique_ptr<const ReferenceEventAnalysis> analysis;
    /** Cumulative shared work, including every refused/entered stage. */
    CheckWork work{};
};

ReferenceEventResult analyseReferenceEvents(const RS_EllipseData& basis, double signedDistance,
                                          double worldTolerance, WorkBudget& budget);

enum class ReferenceVisibility { Visible, Hidden };
enum class ReferenceVisibilityProof { ConvexSupport, CompleteEventClearance };
struct ReferenceFringe {
    /** Full root enclosure. Incident spans select its two symbolic one-sided portions, not two copies of it. */
    ReferenceOccurrenceId occurrence;
    LC_Interval parameter, localX, localY, worldX, worldY;
};
struct ReferenceSpan {
    /** Actual issued roots delimit the span; interior plus their one-sided fringes covers it without gaps. */
    ReferenceOccurrenceId start, end;
    LC_Interval interior;
    ReferenceVisibility visibility{};
    ReferenceVisibilityProof proof{};
    /** Whole symbolic open-span clearance; equality at its event endpoints is permitted. */
    LC_Interval clearance;
};
struct ReferenceEdge {
    std::size_t span = std::size_t(-1);
    ReferenceNodeId start, end;
};
enum class ReferenceEdgeSide { Start, End };
enum class ReferenceTerminalKind { SourceStart, SourceEnd, PairTrim };
struct ReferenceOccurrenceIncidence {
    /** Directed source edges at this exact occurrence, not all edges at its physical node. */
    std::size_t incoming = std::size_t(-1), outgoing = std::size_t(-1);
};
struct ReferenceTerminal {
    ReferenceTerminalKind kind{};
    ReferenceRole role = ReferenceRole::Own;
    ReferenceOccurrenceId occurrence, partner;
    ReferenceNodeId node;
    std::size_t edge = std::size_t(-1);
    ReferenceEdgeSide side{};
    /** Exactly one provenance locator is present, according to kind. */
    std::size_t freeEnd = std::size_t(-1), pairEvent = std::size_t(-1);
};
struct ReferenceIncidence {
    ReferenceNodeId node;
    std::vector<std::size_t> incoming, outgoing;
    bool sourceStart = false, sourceEnd = false, seam = false;
    bool cusp = false, stationary = false, crossing = false;
    bool tangency = false, construction = false;
    /** One-sided retained boundary that is not a free source end. */
    bool trimBoundary = false;
    /** A zero-edge event was proved strictly closer than D to an admitted source point. */
    bool hiddenPoint = false;
};
struct ReferenceComponent {
    /** All edges, in source-parameter order; no crossing-successor pairing is implied. */
    std::vector<std::size_t> edges;
    bool endpointFree = false;
    bool branching = false;
};
enum class ReferenceComponentReason {
    None, EventRefusal, UnresolvedVisibility, UnresolvedFringe, UnresolvedEventPoint, ProofContradiction
};
class ReferenceComponentAnalysis {
public:
    ~ReferenceComponentAnalysis();
    ReferenceComponentAnalysis(const ReferenceComponentAnalysis&) = delete;
    ReferenceComponentAnalysis& operator=(const ReferenceComponentAnalysis&) = delete;
    const ReferenceEventAnalysis& events() const noexcept;
    const std::vector<ReferenceFringe>& fringes() const noexcept;
    const std::vector<ReferenceSpan>& spans() const noexcept;
    const std::vector<ReferenceEdge>& edges() const noexcept;
    const std::vector<ReferenceIncidence>& incidence() const noexcept;
    const std::vector<ReferenceOccurrenceIncidence>& occurrenceIncidence() const noexcept;
    /** Immutable constant-time lookup; a coordinate alone never establishes a terminal. */
    const ReferenceTerminal* terminal(std::size_t edge, ReferenceEdgeSide side) const noexcept;
    const std::vector<ReferenceComponent>& components() const noexcept;
private:
    friend class ReferenceEventBuilder;
    struct Impl;
    explicit ReferenceComponentAnalysis(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> m_impl;
};
struct ReferenceComponentResult {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    ReferenceComponentReason reason = ReferenceComponentReason::None;
    ReferenceEventReason eventReason = ReferenceEventReason::None;
    std::unique_ptr<const ReferenceComponentAnalysis> analysis;
    CheckWork work{};
};
ReferenceComponentResult analyseReferenceComponents(const RS_EllipseData& basis, double signedDistance,
                                                  double worldTolerance, WorkBudget& budget);

enum class CacheWitnessKind { Interior, StartFringe, EndFringe };
struct CacheDyadicDomain {
    std::size_t span = std::size_t(-1), index = 0;
    unsigned depth = 0;
    /** Normalized Bezier parameter, independent of the original affine knot domain. */
    LC_Interval parameter;
};
/** Reflect an exact dyadic cell within its Bezier span; malformed domains are refused atomically. */
bool reflectCacheDyadicDomain(const CacheDyadicDomain& input, CacheDyadicDomain& output);
struct CacheBoundsWitness {
    CacheDyadicDomain cache;
    CacheWitnessKind kind = CacheWitnessKind::Interior;
    std::size_t edge = std::size_t(-1);
    ReferenceOccurrenceId occurrence;
    ReferenceNodeId node;
    LC_Interval referenceParameter;
    unsigned referenceDepth = 0;
    /** Local symmetric setwise error bound; never a parameter/order/identity certificate. */
    LC_Interval geometryError;
    /** For fringe witnesses only, before adding full fringe-to-node coverage error. */
    LC_Interval candidateNodeError;
    /** Interval evaluation of D_local minus the candidate-to-reference/node error. */
    LC_Interval clearanceLowerBound;
};
class CacheBoundsAnalysis {
public:
    ~CacheBoundsAnalysis();
    CacheBoundsAnalysis(const CacheBoundsAnalysis&) = delete;
    CacheBoundsAnalysis& operator=(const CacheBoundsAnalysis&) = delete;
    const CacheLayoutResult& layout() const noexcept;
    const ReferenceComponentAnalysis& reference() const noexcept;
    const std::vector<CacheBoundsWitness>& witnesses() const noexcept;
private:
    friend class CacheBoundsBuilder;
    friend class CacheLocalTopologyBuilder;
    struct Impl;
    explicit CacheBoundsAnalysis(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> m_impl;
};
enum class CacheBoundsStatus { Ok, InvalidInput, InvalidLayout, ReferenceRefusal, OffCurve, Ambiguous, LimitExceeded };
struct CacheBoundsResult {
    CacheBoundsStatus status = CacheBoundsStatus::InvalidInput;
    CacheLayoutStatus layoutStatus = CacheLayoutStatus::InvalidInput;
    LC_IntersectionStatus referenceStatus = LC_IntersectionStatus::InvalidInput;
    ReferenceComponentReason referenceReason = ReferenceComponentReason::None;
    ReferenceEventReason eventReason = ReferenceEventReason::None;
    CheckWork layoutWork{}, referenceWork{}, work{};
    std::unique_ptr<const CacheBoundsAnalysis> analysis;
};
/** Complete bounded proximity relation only; no topology, reverse coverage or public cache-validity claim. */
CacheBoundsResult analyseCacheBounds(const RS_SplineData& cache, const RS_EllipseData& basis,
                                    double signedDistance, double worldTolerance, WorkBudget& budget);

enum class CacheLocalTopologyStatus {
    Ok, InvalidInput, BoundsRefusal, Topology, Ambiguous, LimitExceeded
};
struct CacheLocalTopologyWitness {
    /** Original layout span plus its position after the one whole-cache orientation normalization. */
    CacheDyadicDomain cache;
    std::size_t normalizedSpan = std::size_t(-1);
    CacheWitnessKind kind = CacheWitnessKind::Interior;
    std::size_t edge = std::size_t(-1);
    ReferenceOccurrenceId occurrence;
    ReferenceNodeId node;
    LC_Interval referenceParameter;
    LC_Interval startEndpointError, endEndpointError;
    LC_Interval candidateDerivativeProjection, referenceDerivativeProjection;
    LC_Interval clearanceLowerBound;
    /** For fringe records, which endpoint of this normalized candidate cell is node-bound. */
    bool candidateAtStart = false;
    unsigned referenceDepth = 0;
};
struct CacheLocalTopologyResult;
class CacheLocalTopologyAnalysis {
public:
    ~CacheLocalTopologyAnalysis();
    CacheLocalTopologyAnalysis(const CacheLocalTopologyAnalysis&) = delete;
    CacheLocalTopologyAnalysis& operator=(const CacheLocalTopologyAnalysis&) = delete;
    const CacheBoundsResult& bounds() const noexcept;
    const std::vector<CacheLocalTopologyWitness>& witnesses() const noexcept;
private:
    friend class CacheLocalTopologyBuilder;
    friend CacheLocalTopologyResult analyseCacheTopologyLocally(const RS_SplineData&,
        const LC_EllipseOffsetDefinition&, double, WorkBudget&);
    struct Impl;
    explicit CacheLocalTopologyAnalysis(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> m_impl;
};
struct CacheLocalTopologyResult {
    CacheLocalTopologyResult() = default;
    CacheLocalTopologyResult(const CacheLocalTopologyResult&) = delete;
    CacheLocalTopologyResult& operator=(const CacheLocalTopologyResult&) = delete;
    CacheLocalTopologyResult(CacheLocalTopologyResult&&) noexcept = default;
    CacheLocalTopologyResult& operator=(CacheLocalTopologyResult&&) noexcept = default;
    CacheLocalTopologyStatus status = CacheLocalTopologyStatus::InvalidInput;
    CacheBoundsStatus boundsStatus = CacheBoundsStatus::InvalidInput;
    LC_IntersectionStatus referenceStatus = LC_IntersectionStatus::InvalidInput;
    ReferenceComponentReason referenceReason = ReferenceComponentReason::None;
    ReferenceEventReason eventReason = ReferenceEventReason::None;
    CheckWork work{};
    std::unique_ptr<const CacheLocalTopologyAnalysis> analysis;
};
/** Local forward/injective alternatives only; no global coverage or cache-validity claim. */
CacheLocalTopologyResult analyseCacheTopologyLocally(const RS_SplineData& cache,
                                                    const LC_EllipseOffsetDefinition& definition,
                                                    double worldTolerance, WorkBudget& budget);

enum class CachePathJoinKind { SourceContinuation, PairEvent, Seam };
struct CachePathJoin {
    CachePathJoinKind kind{};
    std::size_t incoming = std::size_t(-1), outgoing = std::size_t(-1);
    ReferenceOccurrenceId from, to;
    std::size_t pairEvent = std::size_t(-1);
};
struct CachePathPiece {
    /** Original span identity is unchanged by whole-cache reversal or refinement. */
    std::size_t span = std::size_t(-1), normalizedSpan = std::size_t(-1), edge = std::size_t(-1);
    LC_Interval candidateParameter, referenceParameter;
    unsigned candidateDepth = 0, referenceDepth = 0;
    LC_Interval geometryError, clearanceLowerBound;
    /** Bounds enclose the actual domain; these endpoints are the SAME owned cut root, not lo/hi. */
    bool startsAtCut = false, endsAtCut = false;
};
struct CachePathCut {
    std::size_t edge = std::size_t(-1), span = std::size_t(-1);
    LC_Interval candidateParameter, referenceParameter;
    std::size_t closingSpan = std::size_t(-1);
    LC_Interval closingCandidateParameter, closingReferenceParameter;
    std::optional<CachePathJoin> transition;
    /** For an interior cut, a unique scalar projection root, not a numerical phase seed. */
    LC_Interval proofParameter, axisX, axisY, derivativeProjection, firstSign, lastSign;
    LC_Interval geometryError, clearanceLowerBound;
    LC_Interval referenceImageDiameter;
    unsigned referenceDepth = 0;
};
struct CachePathEndpoint {
    std::size_t edge = std::size_t(-1), span = std::size_t(-1);
    ReferenceEdgeSide side{};
    ReferenceOccurrenceId occurrence;
    ReferenceNodeId node;
    double candidateParameter = 0;
    LC_Interval geometryError, clearanceLowerBound;
};
/** A complete directed path certificate, never a successful local prefix. */
class CacheTopologyAnalysis {
public:
    ~CacheTopologyAnalysis();
    CacheTopologyAnalysis(const CacheTopologyAnalysis&) = delete;
    CacheTopologyAnalysis& operator=(const CacheTopologyAnalysis&) = delete;
    const CacheLayoutResult& layout() const noexcept;
    const ReferenceComponentAnalysis& reference() const noexcept;
    const std::vector<std::size_t>& edges() const noexcept;
    const std::vector<CachePathJoin>& joins() const noexcept;
    const std::vector<CachePathPiece>& pieces() const noexcept;
    const std::vector<CachePathEndpoint>& endpoints() const noexcept;
    bool closed() const noexcept;
    const std::optional<CachePathCut>& cut() const noexcept;
    const std::optional<ReferenceTerminal>& startTerminal() const noexcept;
    const std::optional<ReferenceTerminal>& endTerminal() const noexcept;
private:
    friend class CacheTopologyBuilder;
    struct Impl;
    explicit CacheTopologyAnalysis(std::unique_ptr<Impl> impl);
    std::unique_ptr<const Impl> m_impl;
};

bool isFinite2D(const RS_Vector& point);
/** Validate raw fields before RS_Ellipse can canonicalize tiny angles. Unchanged on refusal. */
bool normaliseBasis(RS_EllipseData& basis);
/** The source parameter convention used by the offset engine, including its whole-ellipse seam. */
std::optional<Sweep> sweep(const RS_EllipseData& basis);

// These operations require a normalized basis. Invalid/unrepresentable results are invalid vectors or NaN.
RS_Vector ellipsePoint(const RS_EllipseData& basis, double parameter);
RS_Vector ellipseDerivative(const RS_EllipseData& basis, double parameter);
RS_Vector outwardNormal(const RS_EllipseData& basis, double parameter);
RS_Vector exactOffsetPoint(const RS_EllipseData& basis, double parameter, double signedDistance);
double radiusOfCurvature(const RS_EllipseData& basis, double parameter);
double minRadiusOfCurvature(const RS_EllipseData& basis);
bool isInside(const RS_EllipseData& basis, const RS_Vector& point);

/** First-quadrant source parameter of a whole inward lens corner; no value for circles/non-lenses. */
std::optional<double> lensTrimAngle(const RS_EllipseData& basis, double distance);
std::vector<RS_Vector> lensCorners(const RS_EllipseData& basis, double distance);

/** Analytic nearest-point query with a bounded exterior solve, not a conservative cache-validation proof. */
double distanceToOriginal(const RS_EllipseData& basis, const RS_Vector& point, double* parameter = nullptr);

} // namespace LC_EllipseOffsetGeometry

struct LC_OffsetCheckResult {
    enum class Reason {
        None, InvalidInput, Layout, TooSmall, ReferenceUnresolved, OffCurve,
        Ambiguous, Topology, LimitExceeded
    };
    Reason reason = Reason::InvalidInput;
    std::size_t span = std::size_t(-1);
    LC_EllipseOffsetGeometry::CheckWork work{};
    std::unique_ptr<const LC_EllipseOffsetGeometry::CacheTopologyAnalysis> analysis;
    explicit operator bool() const noexcept { return reason == Reason::None && bool(analysis); }
};

namespace LC_EllipseOffsetGeometry {
/** Validate stored geometry without running the offset materializer or repairing its layout. */
LC_OffsetCheckResult checkCache(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
                                double worldTolerance, WorkBudget& budget);
LC_OffsetCheckResult checkCache(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
                                double worldTolerance, const LC_OffsetCheckLimits& limits = {});
} // namespace LC_EllipseOffsetGeometry

#endif
