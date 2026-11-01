/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEREADUTILS_H
#define LC_OFFSETCURVEREADUTILS_H

#include <cmath>
#include <optional>
#include "lc_offsetcurve.h"
#include "rs_line.h"

namespace LC_OffsetCurveRead {
/** Read only a bounded, owned finite line display; a definition is not required. */
inline std::optional<double> cachedLength(const LC_OffsetCurve& curve) {
    const auto cap = LC_OffsetCheckLimits{}.maxVertices;
    if (curve.count() == 0 || curve.count() > cap) return std::nullopt;
    for (const auto* child : curve) {
        const auto* line = dynamic_cast<const RS_Line*>(child);
        if (!line || child->rtti() != RS2::EntityLine || child->getParent() != &curve) return std::nullopt;
        for (const auto& point : {line->getStartpoint(), line->getEndpoint()})
            if (!point.valid || !std::isfinite(point.x) || !std::isfinite(point.y) || point.z != 0)
                return std::nullopt;
    }
    const double length = curve.getLength();
    if (!std::isfinite(length) || length < 0) return std::nullopt;
    return length;
}
}
#endif
