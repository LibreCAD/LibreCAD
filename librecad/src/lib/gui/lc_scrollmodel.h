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

#ifndef LC_SCROLLMODEL_H
#define LC_SCROLLMODEL_H

/**
 * Pure (widget-free) model of one scrollbar axis of a drawing view.
 *
 * Positions are in "scroll space": view pixels along the bar, growing to the right
 * for the horizontal bar and downwards for the vertical one, with the UCS origin at 0.
 *
 * The scrollable region is (content extents +/- kMarginFraction * view length) united
 * with the current view, so a sync never has to clamp the view: setting the computed
 * value back into a scrollbar never moves the view.
 */
namespace LC_ScrollModel {
    constexpr double kMarginFraction = 0.5;
    constexpr double kLineStepPx = 50.0;
    constexpr double kMaxTicks = 16777216.0; // 2^24

    struct Axis {
        /** false for an empty drawing (or invalid content); the region is then just the view */
        bool hasContent = false;
        /** content interval in scroll space */
        double contentMin = 0.0;
        double contentMax = 0.0;
        /** first visible scroll-space pixel of the view */
        double viewStart = 0.0;
        /** view length in pixels */
        double viewLength = 0.0;
    };

    struct State {
        /** false when the input was degenerate; such a state is never used to move the view */
        bool valid = false;
        int maximum = 0; // minimum is always 0
        int pageStep = 1;
        int singleStep = 1;
        int value = 0;
        /** scroll-space pixel of tick 0 */
        double origin = 0.0;
        /** view pixels per scrollbar tick */
        double pixelsPerTick = 1.0;

        /** the view start that corresponds to the scrollbar value \p tick */
        double viewStartFor(const int tick) const {
            return origin + tick * pixelsPerTick;
        }
    };

    State compute(const Axis& axis);
}

#endif
