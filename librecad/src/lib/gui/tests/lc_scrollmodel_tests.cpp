/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Issue #2945: the widget-free scrollbar model and the UCS box it is built on.

#include <algorithm>
#include <cmath>
#include <limits>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QStyle>

#include "lc_coordinates_mapper.h"
#include "lc_scrollmodel.h"
#include "lc_viewmath.h"
#include "rs_math.h"

using Catch::Approx;
using LC_ScrollModel::Axis;
using LC_ScrollModel::State;

namespace {
    double thumbFraction(const State& s) {
        return static_cast<double>(s.pageStep) / (s.maximum + s.pageStep);
    }

    /// a mapper with a UCS rotated so that its x axis points at \p degrees in WCS
    struct RotatedMapper : LC_CoordinatesMapper {
        explicit RotatedMapper(const double degrees) {
            update(RS_Vector(0, 0), -RS_Math::deg2rad(degrees));
            useUCS(true);
        }
    };
}

TEST_CASE("Scroll value never clamps the view", "[navigation][2169]") {
    // content 0..2000 px, view 800 px: inside, straddling, far outside on both sides
    for (const double viewStart : {-400.0, 0.0, 600.0, 1200.0, 1700.0, -900.0, 2300.0, -20000.0, 20000.0}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, viewStart, 800.0});
        INFO("view start " << viewStart);
        REQUIRE(s.valid);
        CHECK(s.pixelsPerTick == 1.0);
        CHECK(s.value >= 0);
        CHECK(s.value <= s.maximum);
        CHECK(s.viewStartFor(s.value) == viewStart);
    }
}

TEST_CASE("The thumb fraction is L/(E+L) and shrinks on zoom in", "[navigation][2945]") {
    double previous = 1.0;
    for (double factor = 1.0; factor <= 64.0; factor *= 2.0) {
        const double extent = 400.0 * factor;
        // view centred on the content
        const State s = LC_ScrollModel::compute({true, 0.0, extent, extent / 2 - 400.0, 800.0});
        INFO("extent " << extent);
        // the region is (content +/- L/2) united with (view +/- L/2): L / (E + L) while the
        // drawing is longer than the view, and half the track while the view covers it
        CHECK(thumbFraction(s) == Approx(800.0 / (std::max(extent, 800.0) + 800.0)).margin(1e-3));
        if (extent > 800.0) {
            CHECK(thumbFraction(s) < previous);
        } else {
            CHECK(thumbFraction(s) <= previous);
        }
        previous = thumbFraction(s);
    }
    // worked example of the plan: 800 px view, 2000 px drawing
    const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
    CHECK(s.maximum == 2000);
    CHECK(s.pageStep == 800);
    CHECK(s.value == 1000);
}

TEST_CASE("The thumb position encodes the view centre", "[navigation][2945]") {
    // exactly while the view lies inside the drawing (centre in [L/2, E - L/2])
    for (const double centre : {400.0, 450.0, 1000.0, 1599.0, 1600.0}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, centre - 400.0, 800.0});
        INFO("centre " << centre);
        CHECK(static_cast<double>(s.value) / s.maximum == Approx(centre / 2000.0).margin(1e-3));
    }
    // beyond that the region grows with the view, keeping half a view of room past it: the
    // thumb still moves with the centre, never reaches an end, and never clamps the view
    double previousPosition = -1.0;
    for (const double centre : {-5000.0, -400.0, 0.0, 250.0, 400.0, 1000.0, 1600.0, 1999.0, 2000.0, 2400.0, 9000.0}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, centre - 400.0, 800.0});
        INFO("centre " << centre);
        CHECK(s.value >= 400);
        CHECK(s.maximum - s.value >= 400);
        CHECK(s.viewStartFor(s.value) == centre - 400.0);
        // moving right, the thumb moves right along the track
        const double position = static_cast<double>(s.value) / s.maximum;
        CHECK(position > previousPosition);
        previousPosition = position;
    }
}

TEST_CASE("The bar always has half a view of room beyond the view", "[navigation][2945-beyond]") {
    // wherever the view is -- inside the drawing, at its edge, or far past it -- the bar can
    // take it at least half a view further in both directions, so a discrete bar step at
    // either end, followed by a resync, always has room again (sand1024's review of #2950)
    for (const double viewStart : {-1.0e6, -5000.0, -800.0, -400.0, 0.0, 600.0, 1200.0, 1600.0, 2000.0, 5000.0,
                                   1.0e6}) {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, viewStart, 800.0});
        INFO("view start " << viewStart);
        REQUIRE(s.valid);
        CHECK(s.value >= 400);
        CHECK(s.maximum - s.value >= 400);
        CHECK(s.viewStartFor(s.value) == viewStart);
        // the drawing and its half-view margins stay inside the region too
        CHECK(s.tickFor(0.0) >= 400.0);
        CHECK(s.tickFor(2000.0) <= s.maximum + 400.0);
    }
}

TEST_CASE("An empty drawing still has half a view of room each way", "[navigation][2945]") {
    // no entity yet, but a command may already be waiting for a point off screen (review of
    // #2950): the region is the view +/- half a view, wherever the view happens to be
    for (const double viewStart : {-400.0, 0.0, 1000.0, -12345.0, 987654.0}) {
        const State s = LC_ScrollModel::compute({false, 123.0, 456.0, viewStart, 800.0});
        INFO("view start " << viewStart);
        REQUIRE(s.valid);
        CHECK(s.maximum == 800);
        CHECK(s.value == 400);
        CHECK(s.pageStep == 800);
        CHECK(s.viewStartFor(s.value) == viewStart);
        CHECK(!s.hasContent);
    }
    // invalid content (unordered or non-finite) is treated the same as no content
    const State unordered = LC_ScrollModel::compute({true, 500.0, -500.0, 0.0, 800.0});
    CHECK(unordered.maximum == 800);
    CHECK(!unordered.hasContent);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const State nonFinite = LC_ScrollModel::compute({true, nan, 456.0, 0.0, 800.0});
    CHECK(nonFinite.maximum == 800);
    CHECK(!nonFinite.hasContent);
}

TEST_CASE("Huge coordinates keep the scroll model int safe", "[navigation][2945]") {
    // extents +/-1e6 at factor 1e9
    const State s = LC_ScrollModel::compute({true, -1e15, 1e15, 0.0, 800.0});
    REQUIRE(s.valid);
    CHECK(s.maximum <= (1 << 24));
    CHECK(s.pageStep >= 1);
    CHECK(s.singleStep >= 1);
    CHECK(s.value >= 0);
    CHECK(s.value <= s.maximum);
    CHECK(std::abs(s.viewStartFor(s.value)) <= s.pixelsPerTick);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, nan}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, 0.0}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, 0.0, -5.0}).valid);
    CHECK(!LC_ScrollModel::compute({true, 0.0, 10.0, inf, 800.0}).valid);
    // non-finite content is treated as no content: region = view +/- half a view
    const State noContent = LC_ScrollModel::compute({true, -inf, nan, 0.0, 800.0});
    CHECK(noContent.valid);
    CHECK(noContent.origin == -400.0);
    CHECK(noContent.maximum == 800);

    CHECK(LC_ViewMath::saturatingRound(nan) == 0);
    CHECK(LC_ViewMath::saturatingRound(inf) == 0);
    CHECK(LC_ViewMath::saturatingRound(-inf) == 0);
    CHECK(LC_ViewMath::saturatingRound(1e300) == static_cast<int>(LC_ViewMath::kMaxViewPixel));
    CHECK(LC_ViewMath::saturatingRound(-2.5) == -3);
    CHECK(LC_ViewMath::saturatingRound(2.5) == 3);
}

TEST_CASE("The vertical scroll axis grows as the view moves down", "[navigation][2945]") {
    // V axis: content [-uMax.y*fy, -uMin.y*fy], view start oy - H
    const double fy = 5.0;
    const int height = 600;
    int previous = -1;
    // views from the drawing's top edge to below it (a view above the top grows the region
    // with it, keeping half a view of room above: its value is always L/2 there)
    for (int oy = 400; oy <= 1100; oy += 100) {
        const State s = LC_ScrollModel::compute({true, -50.0 * fy, 0.0, static_cast<double>(oy - height),
                                                 static_cast<double>(height)});
        CHECK(s.value > previous);
        previous = s.value;
    }
    for (int oy = 100; oy <= 300; oy += 100) {
        const State s = LC_ScrollModel::compute({true, -50.0 * fy, 0.0, static_cast<double>(oy - height),
                                                 static_cast<double>(height)});
        CHECK(s.value == height / 2);
    }
}

TEST_CASE("The UCS box of a WCS box contains all four corners", "[navigation][2945]") {
    const RS_Vector wcsMin(0, 0);
    const RS_Vector wcsMax(100, 10);
    {
        const RotatedMapper mapper(30.0);
        RS_Vector ucsMin, ucsMax;
        mapper.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
        CHECK(ucsMin.y == Approx(-50.0).margin(1e-9));
        CHECK(ucsMax.y == Approx(8.660254).margin(1e-6));
        CHECK(ucsMin.x == Approx(0.0).margin(1e-9));
        CHECK(ucsMax.x == Approx(91.60254).margin(1e-5));

        // guard: the zoom-auto box keeps its two-corner result
        RS_Vector oldMin, oldMax;
        mapper.ucsBoundingBox(wcsMin, wcsMax, oldMin, oldMax);
        CHECK(oldMin.y == Approx(-41.339746).margin(1e-6));
        CHECK(oldMax.y == Approx(0.0).margin(1e-9));
    }
    for (const double degrees : {45.0, -30.0, 120.0, 200.0}) {
        const RotatedMapper mapper(degrees);
        RS_Vector ucsMin, ucsMax;
        mapper.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
        for (const RS_Vector& corner : {RS_Vector(0, 0), RS_Vector(100, 0), RS_Vector(100, 10), RS_Vector(0, 10)}) {
            const RS_Vector u = mapper.toUCS(corner);
            INFO("angle " << degrees << " corner " << corner.x << "," << corner.y);
            CHECK(u.x >= ucsMin.x - 1e-9);
            CHECK(u.x <= ucsMax.x + 1e-9);
            CHECK(u.y >= ucsMin.y - 1e-9);
            CHECK(u.y <= ucsMax.y + 1e-9);
        }
    }
    // without a UCS the box is unchanged
    const LC_CoordinatesMapper identity;
    RS_Vector ucsMin, ucsMax;
    identity.ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
    CHECK(ucsMin == wcsMin);
    CHECK(ucsMax == wcsMax);
}

// --- P18: drawing-extents band (content interval + band pixels + paint rule) --------------

using LC_ScrollModel::Band;
using LC_ScrollModel::ThumbGeometry;

namespace {
    /// a common-style (Fusion/Windows) thumb for a groove of \p groove px starting at \p g0
    struct SimThumb {
        int g0 = 14;
        int groove = 560;
        int minLength = 26;
        int maximum = 0;
        int pageStep = 1;
        int length() const {
            const double len = static_cast<double>(pageStep) * groove / (static_cast<double>(maximum) + pageStep);
            return std::max(minLength, static_cast<int>(len));
        }
        int startAt(const int value) const {
            return g0 + QStyle::sliderPositionFromValue(0, maximum, value, groove - length());
        }
        ThumbGeometry geometry() const {
            return {static_cast<double>(g0), static_cast<double>(startAt(maximum) - startAt(0)),
                    static_cast<double>(length()), maximum, pageStep};
        }
    };

    SimThumb simThumbFor(const State& s, const int minLength = 26) {
        SimThumb t;
        t.minLength = minLength;
        t.maximum = s.maximum;
        t.pageStep = s.pageStep;
        return t;
    }

    double startTick(const State& s) {
        return s.tickFor(s.contentMin);
    }

    double endTick(const State& s) {
        return s.tickFor(s.contentMax);
    }

    Band bandFor(const State& s, const SimThumb& thumb) {
        return LC_ScrollModel::bandPixels(thumb.geometry(), s.hasContent, startTick(s), endTick(s));
    }

    /// the tick interval reproduces the content pixels through the state's own mapping
    void checkTicksReproduceContent(const State& s, const double cMin, const double cMax) {
        REQUIRE(s.hasContent);
        CHECK(s.contentMin == cMin);
        CHECK(s.contentMax == cMax);
        const double tolerance = 1e-9 * std::max({1.0, std::abs(cMin), std::abs(cMax)});
        CHECK(s.origin + startTick(s) * s.pixelsPerTick == Approx(cMin).margin(tolerance));
        CHECK(s.origin + endTick(s) * s.pixelsPerTick == Approx(cMax).margin(tolerance));
    }
}

TEST_CASE("State carries the content interval in pixels; ticks follow a rebased origin", "[navigation][2945-band]") {
    SECTION("wide content, view inside") {
        const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
        checkTicksReproduceContent(s, 0.0, 2000.0);
        CHECK(startTick(s) == 400.0); // half a view of margin before the drawing
        CHECK(endTick(s) == 2400.0);
        CHECK(endTick(s) - startTick(s) > s.pageStep);
        // view inside content <=> value in [a, b - L]
        CHECK(s.value >= startTick(s));
        CHECK(s.value + s.pageStep <= endTick(s));
    }
    SECTION("narrow content, view covers it") {
        const State s = LC_ScrollModel::compute({true, 0.0, 100.0, -350.0, 800.0});
        checkTicksReproduceContent(s, 0.0, 100.0);
        CHECK(endTick(s) - startTick(s) < s.pageStep);
        CHECK(s.value <= startTick(s));
        CHECK(s.value + s.pageStep >= endTick(s));
    }
    SECTION("empty drawing: no interval") {
        const State s = LC_ScrollModel::compute({false, 0.0, 100.0, 0.0, 800.0});
        REQUIRE(s.valid);
        CHECK(!s.hasContent);
        const double nan = std::numeric_limits<double>::quiet_NaN();
        CHECK(!LC_ScrollModel::compute({true, nan, 100.0, 0.0, 800.0}).hasContent);
        CHECK(!LC_ScrollModel::compute({true, 100.0, 0.0, 0.0, 800.0}).hasContent);
    }
    SECTION("the interval always lies inside the scrollable region") {
        for (const double viewStart : {-1e6, -5000.0, -400.0, 0.0, 600.0, 1500.0, 5000.0, 1e6}) {
            for (const double extent : {0.0, 1.0, 100.0, 800.0, 2000.0, 1e5}) {
                const State s = LC_ScrollModel::compute({true, 0.0, extent, viewStart, 800.0});
                INFO("view start " << viewStart << " extent " << extent);
                checkTicksReproduceContent(s, 0.0, extent);
                CHECK(startTick(s) >= s.pageStep / 2.0 - 1.0);
                CHECK(endTick(s) <= s.maximum + s.pageStep / 2.0 + 1.0);
            }
        }
    }
    SECTION("huge coordinates: several pixels per tick, no rounding to ticks") {
        const double cMin = 1.0e9;
        const double cMax = 1.0e9 + 5.0e7 * 400.0;
        const State s = LC_ScrollModel::compute({true, cMin, cMax, cMin + 12345.0, 800.0});
        REQUIRE(s.valid);
        REQUIRE(s.pixelsPerTick > 1.0);
        checkTicksReproduceContent(s, cMin, cMax);
        CHECK(std::isfinite(startTick(s)));
        CHECK(std::isfinite(endTick(s)));
        CHECK(endTick(s) <= s.maximum + s.pageStep);
        CHECK(startTick(s) != std::floor(startTick(s))); // a fraction of a tick is kept
    }
    SECTION("a rebase re-anchors the origin: pixels stay, ticks follow") {
        // what QG_GraphicView::rebaseScrollIfStale() does: a fresh compute, then origin solved
        // for the value the bar still shows; the content stays in pixels, so nothing else moves
        State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
        const double before = startTick(s);
        s.origin -= 123.5;
        checkTicksReproduceContent(s, 0.0, 2000.0);
        CHECK(startTick(s) == Approx(before + 123.5 / s.pixelsPerTick));
        State coarse = LC_ScrollModel::compute({true, 0.0, 5.0e9, 1.0e9, 800.0});
        REQUIRE(coarse.pixelsPerTick > 1.0);
        coarse.origin += 777.25;
        checkTicksReproduceContent(coarse, 0.0, 5.0e9);
    }
}

TEST_CASE("Far-panned views keep the tick interval exact and the band apart from the thumb",
          "[navigation][2945-band]") {
    const double extent = 762.0; // the 100 x 50 drawing at zoom extents
    const double view = 782.0;
    for (const double widths : {2.0, 10.0, 100.0, 1000.0, 1.0e5}) {
        for (const int side : {1, -1}) {
            const double viewStart = side > 0 ? widths * extent : -widths * extent - view;
            const State s = LC_ScrollModel::compute({true, 0.0, extent, viewStart, view});
            INFO("widths " << widths << " side " << side << " pixelsPerTick " << s.pixelsPerTick);
            REQUIRE(s.valid);
            checkTicksReproduceContent(s, 0.0, extent);
            for (const int minLength : {9, 20, 26, 34}) {
                const SimThumb thumb = simThumbFor(s, minLength);
                const Band band = bandFor(s, thumb);
                REQUIRE(band.visible);
                const int thumbStart = thumb.startAt(s.value);
                const int thumbEnd = thumbStart + thumb.length();
                CHECK(band.end - band.start >= LC_ScrollModel::kMinBandPixels);
                CHECK(band.start >= thumb.g0);
                CHECK(band.end <= thumb.g0 + thumb.groove);
                // disjoint, at the opposite end from the thumb; the thumb is half a view
                // short of its end (the room beyond the view), not clamped there
                const int room = LC_ViewMath::saturatingRound(0.5 * view / s.pixelsPerTick);
                if (side > 0) {
                    CHECK(band.end <= thumbStart);
                    CHECK(std::abs(s.maximum - s.value - room) <= 1);
                    CHECK(thumbEnd <= thumb.g0 + thumb.groove);
                } else {
                    CHECK(band.start >= thumbEnd);
                    CHECK(std::abs(s.value - room) <= 1);
                    CHECK(thumbStart >= thumb.g0);
                }
            }
        }
    }
}

// Invariant 1 of LC_ScrollModel::bandPixels() is ONE-DIRECTIONAL: view inside => thumb
// inside is exact, but a thumb inside the band only means the view is inside to within one
// thumb pixel's worth of ticks (many views when a pixel covers many ticks). The converse is
// therefore only checked with 2 px of slack, and only where a pixel is a small part of a view.
TEST_CASE("View inside the drawing => thumb inside the band, swept against Qt's integer thumb",
          "[navigation][2945-band]") {
    long failures = 0;
    long escapes = 0;
    long samples = 0;
    long clamped = 0;
    long coarse = 0;
    for (const double extent : {3.0, 100.0, 700.0, 800.0, 801.0, 2000.0, 1.0e4, 1.0e6, 5.0e8}) {
        for (const int minLength : {9, 20, 26, 34}) {
            for (int i = -40; i <= 40; ++i) {
                const double viewStart = -2.0 * extent - 800.0 + (i + 40) * (5.0 * extent + 1600.0) / 80.0 + 0.37 * i;
                const State s = LC_ScrollModel::compute({true, 0.0, extent, viewStart, 800.0});
                REQUIRE(s.valid);
                const SimThumb thumb = simThumbFor(s, minLength);
                const Band band = bandFor(s, thumb);
                if (!band.visible) {
                    continue;
                }
                ++samples;
                clamped += thumb.length() == minLength ? 1 : 0;
                coarse += s.pixelsPerTick > 1.0 ? 1 : 0;
                const int t0 = thumb.startAt(s.value);
                const int t1 = t0 + thumb.length();
                const double vs = s.viewStartFor(s.value);
                const bool viewInside = vs >= 0.0 && vs + s.pageStep * s.pixelsPerTick <= extent;
                if (viewInside && !(t0 >= band.start && t1 <= band.end)) {
                    ++failures;
                }
                if (band.start < thumb.g0 || band.end > thumb.g0 + thumb.groove) {
                    ++escapes;
                }
                if (t0 >= band.start + 2 && t1 <= band.end - 2 && extent >= 2.0 * s.pageStep) {
                    CHECK(viewInside);
                }
            }
        }
    }
    INFO("samples " << samples << " clamped " << clamped << " coarse " << coarse);
    CHECK(samples > 1000);
    CHECK(clamped > 100); // the sweep covers thumbs clamped to their minimum length ...
    CHECK(coarse > 50);   // ... and more than one pixel per tick
    CHECK(failures == 0);
    CHECK(escapes == 0); // the band never leaves the groove
}

// Invariant 2: for narrow content, view covers => thumb covers is exact (unless the band was
// grown to its minimum length, which is not a projection any more); like invariant 1, the
// converse holds to within one thumb pixel, so it is checked with 1 px of slack.
TEST_CASE("View covers narrow content => thumb covers the band, and back within a pixel",
          "[navigation][2945-band]") {
    long samples = 0;
    long failures = 0;
    long converseSamples = 0;
    for (const double extent : {3.0, 50.0, 100.0, 400.0, 700.0}) {
        for (const int minLength : {9, 20, 26}) {
            for (int i = 0; i <= 200; ++i) {
                const double viewStart = -900.0 + i * (extent + 1000.0) / 200.0;
                const State s = LC_ScrollModel::compute({true, 0.0, extent, viewStart, 800.0});
                const SimThumb thumb = simThumbFor(s, minLength);
                const Band band = bandFor(s, thumb);
                if (!band.visible) {
                    continue;
                }
                const int t0 = thumb.startAt(s.value);
                const int t1 = t0 + thumb.length();
                const double vs = s.viewStartFor(s.value);
                const bool viewCovers = vs <= 0.0 && vs + s.pageStep * s.pixelsPerTick >= extent;
                const bool grown = band.end - band.start <= LC_ScrollModel::kMinBandPixels + 1;
                if (!grown) {
                    ++samples;
                    if (viewCovers && !(t0 <= band.start && t1 >= band.end)) {
                        ++failures;
                        FAIL_CHECK("extent " << extent << " min " << minLength << " view " << vs << " thumb [" << t0
                                   << ", " << t1 << ") band [" << band.start << ", " << band.end << ")");
                    }
                }
                if (t0 <= band.start - 1 && t1 >= band.end + 1) {
                    ++converseSamples;
                    CHECK(viewCovers);
                }
            }
        }
    }
    CHECK(samples > 500);
    CHECK(converseSamples > 300);
    CHECK(failures == 0);
}

TEST_CASE("Placement of the content relative to the view; touching counts as in view", "[navigation][2945-band]") {
    using LC_ScrollModel::Placement;
    using LC_ScrollModel::placement;
    // content [0, 100]
    CHECK(placement(0.0, 100.0, -50.0, 150.0) == Placement::Covered);
    CHECK(placement(0.0, 100.0, 0.0, 100.0) == Placement::Covered);    // exactly the view
    CHECK(placement(0.0, 100.0, 20.0, 80.0) == Placement::Overlaps);   // the view inside it
    CHECK(placement(0.0, 100.0, 50.0, 150.0) == Placement::Overlaps);  // straddles its end
    CHECK(placement(0.0, 100.0, -50.0, 50.0) == Placement::Overlaps);  // straddles its start
    // the view exactly touching either edge: the edge lies on the view's border, in view
    CHECK(placement(0.0, 100.0, 100.0, 200.0) == Placement::Overlaps);
    CHECK(placement(0.0, 100.0, -100.0, 0.0) == Placement::Overlaps);
    // strictly past either edge
    CHECK(placement(0.0, 100.0, 100.5, 200.0) == Placement::Before);
    CHECK(placement(0.0, 100.0, 1.0e6, 1.0e6 + 100.0) == Placement::Before);
    CHECK(placement(0.0, 100.0, -100.0, -0.5) == Placement::After);
    CHECK(placement(0.0, 100.0, -1.0e6 - 100.0, -1.0e6) == Placement::After);
    // zero-length content (a line along the other axis) on the view's border is covered
    CHECK(placement(100.0, 100.0, 100.0, 200.0) == Placement::Covered);
    CHECK(placement(100.0, 100.0, 0.0, 100.0) == Placement::Covered);
    CHECK(placement(100.0, 100.0, 100.5, 200.0) == Placement::Before);
    CHECK(placement(100.0, 100.0, 0.0, 99.5) == Placement::After);
}

TEST_CASE("The band is the linear projection while the thumb is proportional", "[navigation][2945-band]") {
    // no minimum-length clamp: B == [g0 + G*a/(max+L), g0 + G*b/(max+L)]
    const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
    SimThumb thumb = simThumbFor(s, 1);
    const Band band = bandFor(s, thumb);
    REQUIRE(band.visible);
    const double scale = static_cast<double>(thumb.groove) / (s.maximum + s.pageStep);
    CHECK(std::abs(band.start - (thumb.g0 + scale * startTick(s))) <= 1.5);
    CHECK(std::abs(band.end - (thumb.g0 + scale * endTick(s))) <= 1.5);
}

TEST_CASE("Degenerate bands", "[navigation][2945-band]") {
    const State s = LC_ScrollModel::compute({true, 0.0, 2000.0, 600.0, 800.0});
    const SimThumb thumb = simThumbFor(s);
    SECTION("no content, no band") {
        CHECK(!LC_ScrollModel::bandPixels(thumb.geometry(), false, startTick(s), endTick(s)).visible);
        const double nan = std::numeric_limits<double>::quiet_NaN();
        CHECK(!LC_ScrollModel::bandPixels(thumb.geometry(), true, nan, endTick(s)).visible);
        CHECK(!LC_ScrollModel::bandPixels(thumb.geometry(), true, 10.0, 5.0).visible);
    }
    SECTION("a zero-length drawing (a line along the other axis) still shows 3 px") {
        const State line = LC_ScrollModel::compute({true, 500.0, 500.0, 5000.0, 800.0});
        const Band band = bandFor(line, simThumbFor(line));
        REQUIRE(band.visible);
        CHECK(band.end - band.start >= 3);
        CHECK(band.end - band.start <= 4);
    }
    SECTION("a squashed bar, an immovable thumb or an empty slider rect shows nothing") {
        ThumbGeometry g = thumb.geometry();
        g.travel = 0.0;
        CHECK(!LC_ScrollModel::bandPixels(g, true, startTick(s), endTick(s)).visible);
        ThumbGeometry tiny{0.0, 1.0, 3.0, s.maximum, s.pageStep};
        CHECK(!LC_ScrollModel::bandPixels(tiny, true, startTick(s), endTick(s)).visible);
        ThumbGeometry noThumb = thumb.geometry();
        noThumb.length = 0.0;
        CHECK(!LC_ScrollModel::bandPixels(noThumb, true, startTick(s), endTick(s)).visible);
    }
}
