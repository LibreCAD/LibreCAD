/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVE_H
#define LC_OFFSETCURVE_H

#include <cstddef>
#include <memory>
#include <variant>

#include "lc_curveoffset.h"
#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

struct LC_OffsetRunLimits {
    LC_OffsetSourceBudget outputBudget = makeDefaultOffsetSourceBudget();
    /** Zero keeps the engine default; nonzero values may only lower it. */
    std::size_t maxSamples = 0;
    std::size_t maxIntersectionPairs = 0;
    LC_OffsetCheckLimits checkLimits{};
};

enum class LC_OffsetCurveRefusal {
    None, InvalidInput, Undefined, Inconsistent, TooSmall, NothingLeft,
    SeveralPieces, NotCacheRepresentable, CheckFailed, Engine, ThroughNotOnCache
};

struct LC_OffsetCurveEditResult {
    LC_OffsetCurveRefusal refusal = LC_OffsetCurveRefusal::InvalidInput;
    /** Empty means that stage did not run, not a fabricated component result. */
    std::optional<LC_CurveOffsetStatus> engineStatus;
    std::optional<LC_OffsetCheckResult::Reason> checkReason;
    /** C-only work; through-point membership is accounted separately. */
    LC_EllipseOffsetGeometry::CheckWork checkWork{};
    /** Additional membership work, including a refused traversal's actual prefix. */
    LC_EllipseOffsetGeometry::CheckWork throughWork{};
    std::size_t engineRuns = 0;
    bool changed = false;
    explicit operator bool() const noexcept { return refusal == LC_OffsetCurveRefusal::None; }
};

struct LC_OffsetCurveCreateResult;
struct LC_OffsetCurveEditPreparation;
struct LC_OffsetThroughProposal {
    double distance;
    bool towardCentre;
    double parameter;
};

/** Bounded, value-owned extension items; file adapters still validate target encoding. */
struct LC_OffsetCurveExtension {
    int code = 0;
    std::variant<QString, RS_Vector, double, std::int16_t, std::int32_t> value;
};

/** An editable ellipse offset whose ordinary SPLINE cache was geometrically certified. */
class LC_OffsetCurve final : public RS_Spline {
public:
    using OwnedTail = std::vector<LC_OffsetCurveExtension>;
    static constexpr std::size_t MaximumOwnedTailItems = 64;
    static constexpr std::size_t MaximumOwnedTailLogicalBytes = 16u * 1024u;
    static constexpr std::size_t MaximumOwnedStringUnits = 4096;
    static constexpr unsigned MaximumOwnedTailDepth = 8;

    /** Caller limits only lower caps. outcome is C's actual result; refusal is native admission. */
    static std::unique_ptr<LC_OffsetCurve> adopt(const RS_Entity &cache,
        const LC_EllipseOffsetDefinition &definition, double worldTolerance,
        LC_OffsetCheckResult *outcome = nullptr, const LC_OffsetCheckLimits &limits = {},
        LC_OffsetCurveRefusal *refusal = nullptr);

    /** Creates a new identity; relativeTolerance=0 selects the ellipse engine default. */
    static LC_OffsetCurveCreateResult create(const RS_Ellipse &source, double distance, bool towardCentre,
        const LC_OffsetRunLimits &limits = {}, double relativeTolerance = 0, bool cacheReversed = false);
    LC_OffsetCurveEditResult setDefinition(const LC_EllipseOffsetDefinition &definition,
        const LC_OffsetRunLimits &limits = {});
    /** No producer/C run. Owns a cheap proof-preserving replacement or describes
     * a generation job; the Generate case is NOT a successful edit result.
     */
    LC_OffsetCurveEditPreparation prepareDefinition(const LC_EllipseOffsetDefinition &definition,
        const LC_OffsetRunLimits &limits = {}) const;
    /** Analytic proposal only; CheckThrough and Generate still require worker computation. */
    LC_OffsetCurveEditPreparation prepareThrough(const RS_Vector &point,
        const LC_OffsetRunLimits &limits = {}) const;
    LC_OffsetCurveEditResult setDistance(double distance, const LC_OffsetRunLimits &limits = {});
    LC_OffsetCurveEditResult setTowardCentre(bool towardCentre, const LC_OffsetRunLimits &limits = {});
    LC_OffsetCurveEditResult setThrough(const RS_Vector &point, const LC_OffsetRunLimits &limits = {});
    LC_OffsetCurveEditResult setBasis(const RS_EllipseData &basis, const LC_OffsetRunLimits &limits = {});
    /** Bounded analytic proposal only; the final retained cache is checked separately on commit. */
    std::optional<LC_OffsetThroughProposal> proposeThrough(const RS_Vector &point) const;

    LC_OffsetCurve(const LC_OffsetCurve &) = default;
    RS2::EntityType rtti() const override { return RS2::EntityOffsetCurve; }
    RS_Entity *clone() const override;
    const LC_EllipseOffsetDefinition &definition() const noexcept { return m_definition; }
    bool isDefined() const noexcept { return m_defined; }
    bool isConsistent() const noexcept;
    bool hasEditableDefinition() const noexcept { return isDefined() && isConsistent(); }
    bool sourceIsWhole() const noexcept;
    bool cacheFormsClosedChain() const noexcept { return hasEditableDefinition() && m_closed; }
    double checkedWorldTolerance() const noexcept { return m_worldTolerance; }
    std::unique_ptr<RS_Spline> toPlainSpline() const;
    std::unique_ptr<RS_Ellipse> originalEllipse() const;

    const std::vector<RS_Vector> &cachedDefinitionGrips() const noexcept { return m_grips; }
    /** A unique cached grip match, using local roundoff and explicit coordinate ULPs. */
    std::optional<std::size_t> definitionGripIndex(const RS_Vector &ref) const;
    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getMiddlePoint() const override;
    void moveRef(const RS_Vector &ref, const RS_Vector &offset) override;
    unsigned unknownOwnedFlags() const noexcept { return m_unknownFlags; }
    const OwnedTail &unknownOwnedTail() const noexcept { return m_unknownTail; }
    /** Target-independent storage admission, not a target-encoding guarantee.
     * Logical charges are 3+2*UTF16-units for strings, 25 for vectors, 9 for
     * doubles, 3 for int16 and 5 for int32; bounded container overhead is separate.
     * Strings must be valid UTF16 without NUL/CR/LF and are copied into fresh
     * storage, including strings backed by fromRawData or excess caller capacity.
     * Brace items count toward the item/depth caps. No partial tail is retained.
     * Validation refusal and propagating allocation exceptions preserve prior
     * flags/tail. File adapters separately enforce encoded-byte and codec limits.
     */
    bool setOwnedExtensions(unsigned flags, const OwnedTail &tail);

    void update() override;
    void move(const RS_Vector &offset) override;
    void rotate(const RS_Vector &center, double angle) override;
    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
    void scale(const RS_Vector &center, const RS_Vector &factor) override;
    RS_Entity &shear(double k) override;
    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;
    void revertDirection() override;

protected:
    RS_Vector doGetNearestRef(const RS_Vector &coord, double *dist) const override;
    RS_Vector doGetNearestSelectedRef(const RS_Vector &coord, double *dist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector &coord, double *dist, RS_Entity **entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector &coord, double *dist, RS_Entity **entity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector &coord, double *dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector &coord, double *dist) const override;

private:
    friend class LC_OffsetWorkerKernel;
    friend class LC_OffsetWorkerCompletion;
    friend class LC_OffsetWorkerEdit;
    struct ValidatedCache {};
    LC_OffsetCurve(const RS_Entity &source, const RS_SplineData &cache,
        const LC_EllipseOffsetDefinition &definition, double tolerance,
        const LC_EllipseOffsetGeometry::CacheTopologyAnalysis &topology, ValidatedCache);
    struct ReceivedCache {};
    LC_OffsetCurve(const RS_Entity &source, const RS_SplineData &cache,
        const LC_EllipseOffsetDefinition &definition, double tolerance, bool closed,
        std::vector<RS_Vector> grips, const RS_Vector &midpoint, ReceivedCache);
    // Similarities transport the existing proof only if every stored coordinate
    // is the exact affine image. Rounded similarities refuse unchanged, not restamp.
    bool transform(const std::array<double, 6> &matrix, double similarityScale, bool reflected);
    void publish(RS_Spline &prepared, const LC_EllipseOffsetDefinition &definition,
        std::vector<RS_Vector> &grips, double tolerance, bool defined, bool closed, const RS_Vector &midpoint);
    static LC_OffsetCurveCreateResult runEngine(const RS_Entity &attributes,
        const LC_EllipseOffsetDefinition &definition, const LC_OffsetRunLimits &limits);
    static bool checkThrough(const LC_OffsetCurve &curve, const RS_Vector &point,
        const LC_OffsetCheckLimits &limits, const LC_EllipseOffsetGeometry::CheckWork &checked,
        LC_EllipseOffsetGeometry::CheckWork &throughWork);
    static bool checkThroughCache(const RS_SplineData &cache, double tolerance, const RS_Vector &point,
        const LC_OffsetCheckLimits &limits, LC_EllipseOffsetGeometry::CheckWork &throughWork);
    LC_OffsetCurveEditResult eligibility() const noexcept;
    LC_OffsetCurveEditResult edit(const LC_EllipseOffsetDefinition &definition,
        const LC_OffsetRunLimits &limits, const RS_Vector *through = nullptr);
    LC_OffsetCurveEditResult commitPrepared(LC_OffsetCurveCreateResult &prepared);

    LC_EllipseOffsetDefinition m_definition;
    double m_worldTolerance = 0;
    std::uint64_t m_stamp = 0;
    bool m_defined = true;
    bool m_closed = false;
    std::vector<RS_Vector> m_grips;
    RS_Vector m_cachedMiddlePoint{false};
    unsigned m_unknownFlags = 0;
    OwnedTail m_unknownTail;
};

struct LC_OffsetCurveCreateResult : LC_OffsetCurveEditResult {
    std::unique_ptr<LC_OffsetCurve> entity;
    LC_OffsetOutputUsage attemptedOutputUsage{};
    bool attemptedDeepCountComplete = true;
    LC_OffsetOutputUsage outputUsage{};
    std::size_t sampleUnits = 0;
    explicit operator bool() const noexcept {
        return LC_OffsetCurveEditResult::operator bool() && bool(entity);
    }
};

struct LC_OffsetCurveEditPreparation {
    enum class Kind { Refused, Unchanged, Prepared, Generate, CheckThrough };
    Kind kind = Kind::Refused;
    LC_OffsetCurveEditResult result;
    std::unique_ptr<LC_OffsetCurve> local;
    LC_EllipseOffsetDefinition definition;
};
#endif
