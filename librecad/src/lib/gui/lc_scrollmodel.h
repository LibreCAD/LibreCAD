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
    /** default of the Appearance/ScrollBarContentBand setting: the bars show the band */
    constexpr bool kContentBandDefault = true;

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

        /**
         * The drawing extents (Axis::contentMin/contentMax), in scroll-space pixels, for
         * the drawing-extents band painted on the bar (QG_ScrollBar). Kept in pixels, not
         * ticks, so they stay valid when the origin is re-anchored (a rebase); convert
         * with tickFor() against the state the bar actually shows. Only meaningful when
         * hasContent is true.
         */
        bool hasContent = false;
        double contentMin = 0.0;
        double contentMax = 0.0;

        /** the view start that corresponds to the scrollbar value \p tick */
        double viewStartFor(const int tick) const {
            return origin + tick * pixelsPerTick;
        }

        /** the (fractional, never rounded) tick of the scroll-space pixel \p px */
        double tickFor(const double px) const {
            return (px - origin) / pixelsPerTick;
        }
    };

    State compute(const Axis& axis);

    /**
     * The thumb geometry a style reports along the bar, in the bar's own pixels: the
     * slider rect at value 0 and at value maximum (QStyle::subControlRect). Every style
     * checked (Fusion, Windows, macOS, style sheets) moves the thumb linearly between them.
     */
    struct ThumbGeometry {
        double start0 = 0.0; // thumb start at value 0
        double travel = 0.0; // thumb start at value maximum minus start0
        double length = 0.0; // thumb length (the style may clamp it to its minimum)
        int maximum = 0;     // bar maximum (minimum is always 0)
        int pageStep = 1;    // bar page step: the view length in ticks
    };

    /** a band along the bar: pixels [start, end) */
    struct Band {
        bool visible = false;
        int start = 0;
        int end = 0;
    };

    /** a band thinner than this many pixels (a point, a line along the other axis) is widened */
    constexpr double kMinBandPixels = 3.0;

    /**
     * Where the content ticks [\p a, \p b] lie along the bar: [p(a), p(b - L) + T], with
     * p(t) the thumb start at value t (linearly extrapolated) and T the thumb length.
     * Wide content is rounded outward and narrow content inward, against the style's
     * integer thumb. Both invariants below need only p to be monotone (T cancels), so
     * they hold while the style clamps the thumb to its minimum length:
     *  1. the view lies inside the content => the thumb lies inside the band. This is
     *     ONE-DIRECTIONAL: the converse holds only to within one thumb pixel's worth of
     *     ticks, which can be many views when a pixel covers many ticks (up to ~46 pages
     *     at maximum = 2^24);
     *  2. for narrow content, the view covers the content => the thumb covers the band
     *     (unless the band was grown to its minimum length); the converse, again, only to
     *     within one thumb pixel.
     * The band always stays inside the groove, as the scroll region is content +/- L/2.
     */
    Band bandPixels(const ThumbGeometry& g, bool hasContent, double a, double b);

    /**
     * Whether the band for content ticks [\p a, \p b] tells the user anything at bar
     * value \p value and page step \p pageStep: only once the view [value, value +
     * pageStep] and the content are disjoint, i.e. the view has left the drawing along
     * this bar. While any of the drawing is in view the user is not lost, and the band
     * would mislead: with the view inside the content it is just "drawing +/- half a
     * view", the same for every drawing and pan position, tinting most of the track
     * when zoomed in; with the view covering the content it lies under the thumb; and
     * with the view straddling an edge it covers about (1 - 1/zoom) of the track (99%
     * at 100x), flashing on and off as the view crosses the edge by one pixel.
     */
    bool bandIsInformative(double a, double b, int value, int pageStep);
}

#endif
