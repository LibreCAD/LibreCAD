/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_scrollmodel.h"

#include <algorithm>
#include <cmath>

#include "lc_viewmath.h"

namespace LC_ScrollModel {

State compute(const Axis& axis) {
    State state;
    const double length = axis.viewLength;
    const double viewStart = axis.viewStart;
    if (!std::isfinite(length) || length <= 0.0 || !std::isfinite(viewStart)) {
        return state;
    }

    const bool hasContent = axis.hasContent && std::isfinite(axis.contentMin) && std::isfinite(axis.contentMax)
        && axis.contentMin <= axis.contentMax;

    // the view +/- the margin, united with the drawing +/- the same margin when there is
    // one: the bar can always take the view at least one margin further, in both
    // directions, wherever it is, even in an empty drawing (a command's first point may be
    // placed before any entity exists)
    const double margin = kMarginFraction * length;
    double first = viewStart;
    double last = viewStart + length;
    if (hasContent) {
        first = std::min(axis.contentMin, first);
        last = std::max(axis.contentMax, last);
    }
    const double lo = std::floor(first - margin);
    const double hi = std::ceil(last + margin);
    const double span = std::max(0.0, hi - lo - length);
    const double pixelsPerTick = std::max(1.0, span / kMaxTicks);
    if (!std::isfinite(lo) || !std::isfinite(span)) {
        return state;
    }

    state.origin = lo;
    state.pixelsPerTick = pixelsPerTick;
    state.maximum = LC_ViewMath::saturatingRound(span / pixelsPerTick);
    state.pageStep = std::max(1, LC_ViewMath::saturatingRound(length / pixelsPerTick));
    state.singleStep = std::max(1, LC_ViewMath::saturatingRound(kLineStepPx / pixelsPerTick));
    state.value = std::clamp(LC_ViewMath::saturatingRound((viewStart - lo) / pixelsPerTick), 0, state.maximum);
    state.valid = true;
    if (hasContent) {
        state.hasContent = true;
        state.contentMin = axis.contentMin;
        state.contentMax = axis.contentMax;
    }
    return state;
}

namespace {
    // p(t): the thumb start at value t, linearly EXTRAPOLATED outside [0, maximum]
    // (QStyle::sliderPositionFromValue clamps, which would break narrow content, where
    // b - L < 0). With maximum 0 (region == view) the thumb is the whole groove, at
    // length / pageStep pixels per tick.
    double thumbStartAt(const ThumbGeometry& g, const double tick) {
        if (g.maximum <= 0) {
            return g.start0 + tick * (g.length / std::max(1, g.pageStep));
        }
        return g.start0 + tick * (g.travel / g.maximum);
    }
}

Band bandPixels(const ThumbGeometry& g, const bool hasContent, const double a, const double b) {
    Band band;
    if (!hasContent || !std::isfinite(a) || !std::isfinite(b) || a > b || !(g.length > 0.0)
        || !std::isfinite(g.start0) || !std::isfinite(g.travel)) {
        return band;
    }
    if (g.maximum > 0 && g.travel <= 0.0) {
        return band; // the thumb cannot move: nothing to relate the band to
    }
    const double grooveStart = g.start0;
    const double grooveEnd = g.start0 + std::max(0.0, g.travel) + g.length;
    if (grooveEnd - grooveStart < 2.0 * kMinBandPixels) {
        return band; // squashed bar
    }
    double lo = thumbStartAt(g, a);
    double hi;
    bool outward;
    if (g.maximum <= 0) {
        hi = thumbStartAt(g, b);
        outward = true;
    } else {
        const double pageStep = g.pageStep;
        hi = thumbStartAt(g, b - pageStep) + g.length;
        outward = b - a >= pageStep;
    }
    if (hi - lo < kMinBandPixels) {
        const double mid = 0.5 * (lo + hi);
        lo = mid - 0.5 * kMinBandPixels;
        hi = mid + 0.5 * kMinBandPixels;
        outward = true;
    }
    if (outward) {
        lo = std::floor(lo);
        hi = std::ceil(hi);
    } else {
        lo = std::ceil(lo);
        hi = std::floor(hi);
    }
    // a minimum-length band at an end of the groove slides back inside it
    if (hi - lo <= kMinBandPixels + 1.0) {
        if (lo < grooveStart) {
            hi += grooveStart - lo;
            lo = grooveStart;
        }
        if (hi > grooveEnd) {
            lo -= hi - grooveEnd;
            hi = grooveEnd;
        }
    }
    lo = std::clamp(lo, grooveStart, grooveEnd);
    hi = std::clamp(hi, grooveStart, grooveEnd);
    if (hi <= lo) {
        return band;
    }
    band.visible = true;
    band.start = static_cast<int>(lo);
    band.end = static_cast<int>(hi);
    return band;
}

Placement placement(const double contentMin, const double contentMax, const double viewMin, const double viewMax) {
    if (viewMin <= contentMin && contentMax <= viewMax) {
        return Placement::Covered;
    }
    if (contentMax < viewMin) {
        return Placement::Before;
    }
    if (contentMin > viewMax) {
        return Placement::After;
    }
    return Placement::Overlaps;
}

}
