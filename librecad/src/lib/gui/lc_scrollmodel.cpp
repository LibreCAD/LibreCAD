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

    double lo;
    double hi;
    if (hasContent) {
        const double margin = kMarginFraction * length;
        lo = std::floor(std::min(axis.contentMin - margin, viewStart));
        hi = std::ceil(std::max(axis.contentMax + margin, viewStart + length));
    } else {
        // An empty drawing (or invalid content) has nothing to scroll to:
        // region = view, so the bar shows a full-length thumb.
        lo = std::floor(viewStart);
        hi = std::ceil(viewStart + length);
    }
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
    return state;
}

}
