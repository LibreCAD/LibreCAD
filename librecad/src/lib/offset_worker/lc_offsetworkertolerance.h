/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERTOLERANCE_H
#define LC_OFFSETWORKERTOLERANCE_H

#include "../engine/document/entities/lc_curveoffset.h"
#include "../engine/document/entities/lc_offsetsource.h"

#include <cmath>
#include <optional>

namespace LC_OffsetWorkerTolerance {

inline std::optional<LC_CurveOffsetOptions> directOptions(
    const RS_Entity& source, double distance, double relativeFactor) {
    if (!std::isfinite(relativeFactor) || relativeFactor < 0.0 || !std::isfinite(distance)) {
        return std::nullopt;
    }

    distance = std::abs(distance);
    relativeFactor = relativeFactor == 0.0
        ? LC_CurveOffset::relativeOffsetTolerance(source)
        : relativeFactor;
    const double worldTolerance = LC_CurveOffset::toleranceFor(source, distance, relativeFactor);
    if (!std::isfinite(worldTolerance) || !(worldTolerance > 0.0)) return std::nullopt;

    LC_CurveOffsetOptions options =
        LC_CurveOffset::makeDirectOptions(source, distance, worldTolerance);
    if (!std::isfinite(options.tolerance.requestedGeometry) ||
        !(options.tolerance.requestedGeometry > 0.0) ||
        options.tolerance.requestedGeometry != worldTolerance) {
        return std::nullopt;
    }
    return options;
}

inline std::optional<LC_CurveOffsetOptions> directOptions(
    const LC_OffsetSourceDefinition& definition, const RS_Entity& source) {
    return directOptions(source, definition.signedDistance, definition.relativeTolerance);
}

} // namespace LC_OffsetWorkerTolerance

#endif
