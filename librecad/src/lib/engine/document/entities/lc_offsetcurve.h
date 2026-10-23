/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVE_H
#define LC_OFFSETCURVE_H

#include <memory>
#include <variant>

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

/** Bounded, value-owned extension items; file adapters still validate target encoding. */
struct LC_OffsetCurveExtension {
    int code = 0;
    std::variant<QString, RS_Vector, double, std::int16_t, std::int32_t> value;
};

/** An editable ellipse offset whose ordinary SPLINE cache was geometrically certified. */
class LC_OffsetCurve final : public RS_Spline {
public:
    using OwnedTail = std::vector<LC_OffsetCurveExtension>;

    /** Caller limits may lower, but never raise, the native admission/work caps. */
    static std::unique_ptr<LC_OffsetCurve> adopt(const RS_Entity &cache,
        const LC_EllipseOffsetDefinition &definition, double worldTolerance,
        LC_OffsetCheckResult *outcome = nullptr, const LC_OffsetCheckLimits &limits = {});

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
    unsigned unknownOwnedFlags() const noexcept { return m_unknownFlags; }
    const OwnedTail &unknownOwnedTail() const noexcept { return m_unknownTail; }
    bool setOwnedExtensions(unsigned flags, const OwnedTail &tail);

    void update() override;
    void move(const RS_Vector &offset) override;
    void rotate(const RS_Vector &center, double angle) override;
    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
    void scale(const RS_Vector &center, const RS_Vector &factor) override;
    RS_Entity &shear(double k) override;
    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;
    void revertDirection() override;

private:
    struct ValidatedCache {};
    LC_OffsetCurve(const RS_Entity &source, const RS_SplineData &cache,
        const LC_EllipseOffsetDefinition &definition, double tolerance, bool closed, ValidatedCache);
    // Similarities transport the existing proof only if every stored coordinate
    // is the exact affine image. Rounded similarities refuse unchanged, not restamp.
    bool transform(const std::array<double, 6> &matrix, double similarityScale, bool reflected);
    void publish(RS_Spline &prepared, const LC_EllipseOffsetDefinition &definition,
        std::vector<RS_Vector> &grips, double tolerance, bool defined);

    LC_EllipseOffsetDefinition m_definition;
    double m_worldTolerance = 0;
    std::uint64_t m_stamp = 0;
    bool m_defined = true;
    bool m_closed = false;
    std::vector<RS_Vector> m_grips;
    unsigned m_unknownFlags = 0;
    OwnedTail m_unknownTail;
};
#endif
