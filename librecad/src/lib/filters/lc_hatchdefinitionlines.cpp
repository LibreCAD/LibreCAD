/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lc_hatchdefinitionlines.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <utility>

#include "drw_base.h"
#include "rs_hatch.h"
#include "rs_math.h"

namespace LC_HatchDefinitionLines {

namespace {

RS_Vector rotateCCW(RS_Vector v, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return RS_Vector{c * v.x - s * v.y, s * v.x + c * v.y};
}

bool iequals(const std::string& a, const char* b) {
    const std::size_t blen = std::char_traits<char>::length(b);
    if (a.size() != blen)
        return false;
    for (std::size_t i = 0; i < blen; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

bool isApplicationGroup(const std::shared_ptr<DRW_Variant>& tag) {
    if (!tag || tag->code() != 1001 || tag->type() != DRW_Variant::STRING)
        return false;
    return tag->content.s != nullptr && iequals(*tag->content.s, kApplication);
}

bool isAnyApplicationGroup(const std::shared_ptr<DRW_Variant>& tag) {
    return tag && tag->code() == 1001;
}

bool isApproximateTag(const std::shared_ptr<DRW_Variant>& tag) {
    if (!tag || tag->code() != 1000 || tag->type() != DRW_Variant::STRING)
        return false;
    return tag->content.s != nullptr && *tag->content.s == kApproximateTag;
}

/// Removes every kApproximateTag inside a kApplication group and drops an
/// application group left with no items. Returns the count of removed tags.
std::size_t removeApproximateTags(
    std::vector<std::shared_ptr<DRW_Variant>>& extData) {
    std::vector<std::shared_ptr<DRW_Variant>> kept;
    kept.reserve(extData.size());
    bool inApp = false;
    std::size_t removed = 0;
    for (const auto& tag : extData) {
        if (isAnyApplicationGroup(tag)) {
            inApp = isApplicationGroup(tag);
            kept.push_back(tag);
            continue;
        }
        if (inApp && isApproximateTag(tag)) {
            ++removed;
            continue;
        }
        kept.push_back(tag);
    }
    // Drop a 1001 kApplication group that now has no items of its own: a
    // following 1001 (another application) or end-of-list means empty.
    std::vector<std::shared_ptr<DRW_Variant>> trimmed;
    trimmed.reserve(kept.size());
    for (std::size_t i = 0; i < kept.size(); ++i) {
        if (isApplicationGroup(kept[i])) {
            std::size_t j = i + 1;
            bool hasItem = false;
            while (j < kept.size() && !isAnyApplicationGroup(kept[j])) {
                hasItem = true;
                ++j;
            }
            if (!hasItem)
                continue;
        }
        trimmed.push_back(kept[i]);
    }
    extData = std::move(trimmed);
    return removed;
}

} // namespace

std::optional<DRW_Hatch::PatternLine>
realize(const LC_HatchLineFamily& f, const RS_HatchPatternFrame& frame) {
    // The perpendicular component of offset is the spacing between adjacent
    // parallel lines. If it is zero (within a relative floor), the pattern
    // would be infinitely dense; refuse the family so the writer falls back
    // to the placeholder.
    const double offsetLen = std::hypot(f.offset.x, f.offset.y);
    if (!(offsetLen > 0.0) || !std::isfinite(offsetLen))
        return std::nullopt;
    if (std::fabs(f.offset.y) < 1e-9 * offsetLen)
        return std::nullopt;

    const double theta = frame.angle; // radians
    const double a = f.angleDeg * M_PI / 180.0;
    const double t = a + theta;

    DRW_Hatch::PatternLine line;
    // 53 is written in degrees by libdxfrw at writer boundary; we store
    // radians on the DRW record.
    line.angle = RS_Math::correctAngle(t);

    // Base is a point in the pattern-local frame; it rides the hatch rotation
    // only (not the per-family angle), relative to tileMin, and shifts by the
    // lattice origin h.
    const RS_Vector baseLocal{f.base.x - frame.tileMin.x,
                               f.base.y - frame.tileMin.y};
    const RS_Vector baseWorldOff = rotateCCW(baseLocal, theta);
    line.baseX = frame.origin.x + frame.scale * baseWorldOff.x;
    line.baseY = frame.origin.y + frame.scale * baseWorldOff.y;

    // Offset is PAT line-local: dx along the line direction, dy across it.
    // The world offset is R(a + theta) applied to (dx, dy), scaled.
    const RS_Vector world = rotateCCW(f.offset, t);
    line.offsetX = frame.scale * world.x;
    line.offsetY = frame.scale * world.y;

    line.dashList.reserve(f.dashes.size());
    for (double d : f.dashes)
        line.dashList.push_back(frame.scale * d);

    return line;
}

DRW_Hatch::PatternLine placeholderLine(const RS_HatchPatternFrame& frame) {
    double s = frame.scale;
    if (!std::isfinite(s) || s <= 0.0)
        s = 1.0;
    double w = 1.0;
    double h = 1.0;
    if (frame.tileValid && frame.tileSize.valid &&
        std::isfinite(frame.tileSize.x) && std::isfinite(frame.tileSize.y) &&
        frame.tileSize.x > 1e-12 && frame.tileSize.y > 1e-12) {
        w = frame.tileSize.x;
        h = frame.tileSize.y;
    }
    const double perp = std::sqrt(w * h) / 4.0;

    LC_HatchLineFamily family;
    family.angleDeg = 45.0;
    family.base = frame.tileValid ? frame.tileMin : RS_Vector{0.0, 0.0};
    family.offset = RS_Vector{0.0, perp};
    family.dashes.clear();

    RS_HatchPatternFrame effective = frame;
    effective.scale = s;

    auto realized = realize(family, effective);
    // The placeholder is always a non-zero spacing: perp > 0 always.
    return realized.value();
}

ExportLines forExport(const RS_Hatch& hatch) {
    ExportLines result;
    const RS_HatchPatternFrame frame = hatch.patternFrame();
    result.lines.push_back(placeholderLine(frame));
    result.approximate = true;
    return result;
}

void setApproximateTag(std::vector<std::shared_ptr<DRW_Variant>>& extData,
                       bool on) {
    removeApproximateTags(extData);
    if (!on)
        return;
    // Insert the 1000 tag inside the first kApplication group when one is
    // already present; otherwise append a new kApplication group with the
    // tag. The insertion goes at the end of the group's items so the result
    // round-trips exactly through strip-plus-set.
    auto appIt = std::find_if(extData.begin(), extData.end(),
                              isApplicationGroup);
    auto tag = std::make_shared<DRW_Variant>(1000, std::string(kApproximateTag));
    if (appIt == extData.end()) {
        extData.push_back(
            std::make_shared<DRW_Variant>(1001, std::string(kApplication)));
        extData.push_back(std::move(tag));
        return;
    }
    auto insertAt = std::find_if(std::next(appIt), extData.end(),
                                 isAnyApplicationGroup);
    extData.insert(insertAt, std::move(tag));
}

bool stripApproximateTag(std::vector<std::shared_ptr<DRW_Variant>>& extData) {
    return removeApproximateTags(extData) > 0;
}

int boundaryPathFlags(int depth) {
    if (depth <= 0)
        return 1;
    if (depth == 1)
        return 16;
    return 0;
}

} // namespace LC_HatchDefinitionLines
