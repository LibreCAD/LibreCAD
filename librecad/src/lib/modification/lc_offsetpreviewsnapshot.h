/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETPREVIEWSNAPSHOT_H
#define LC_OFFSETPREVIEWSNAPSHOT_H

#include "lc_curveoffset.h"
#include "rs_line.h"
#include "rs_spline.h"
#include <cstdint>
#include <optional>
#include <variant>

/** Ordinary geometry only, never a native-cache admission capability. A source's
 * entries can be rebound to an owned source in a fresh helper. otherEntries
 * preserves the action-wide clearing threshold without transporting other sources.
 */
struct LC_OffsetPreviewSnapshot {
    struct Entry {
        LC_CurveOffsetSide side = LC_CurveOffsetSide::Left;
        double magnitude = 0;
        LC_OffsetSourceBudget budget{};
        LC_CurveOffsetStatus status = LC_CurveOffsetStatus::InvalidSource;
        LC_OffsetOutputUsage usage{};
        LC_OffsetValidationLevel validation = LC_OffsetValidationLevel::None;
        std::optional<double> observedError;
        std::vector<std::variant<RS_LineData, RS_SplineData>> geometry;
    };
    static constexpr std::size_t maximumEntries = 256, maximumValues = 262144;
    std::uint64_t revision = 0;
    std::size_t otherEntries = 0;
    bool cleared = false;
    std::vector<Entry> entries;
    /** Bounded representation checks, not offset or native geometric proof. */
    bool admitted() const noexcept;
};

#endif
