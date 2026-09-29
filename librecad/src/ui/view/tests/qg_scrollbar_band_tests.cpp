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

// Issue #2945 (P18): each drawing-view scrollbar marks where the drawing lies along it,
// with a thin stripe on the groove, painted whenever the drawing has content, with the
// thumb cut out: it shows the parts of the drawing's range outside the view.

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QCheckBox>
#include <QHelpEvent>
#include <QImage>
#include <QSettings>
#include <QStyleFactory>

#include "lc_dimstylepreviewgraphicview.h"
#include "lc_navtest_support.h"
#include "qg_dlg_hatch.h"
#include "qg_dlgoptionsgeneral.h"
#include "qg_scrollbar.h"
#include "rs_color.h"
#include "rs_settings.h"
#include "rs_units.h"

using namespace lc::navtest;

namespace {
    QStyle* fusionStyle() {
        static std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));
        return style.get();
    }

    QG_ScrollBar* band(QScrollBar* bar) {
        return qobject_cast<QG_ScrollBar*>(bar);
    }

    /// the widget tests are written against Fusion (the offscreen default), band on
    void useFusion(ViewFixture& f) {
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            band(bar)->setContentBandEnabled(true);
            if (bar->style()->name().compare("fusion", Qt::CaseInsensitive) != 0) {
                bar->setStyle(fusionStyle());
            }
        }
        pump();
    }

    QPalette darkPalette() {
        QPalette p;
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(35, 35, 35));
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::Light, QColor(75, 75, 75));
        p.setColor(QPalette::Midlight, QColor(62, 62, 62));
        p.setColor(QPalette::Mid, QColor(45, 45, 45));
        p.setColor(QPalette::Dark, QColor(30, 30, 30));
        p.setColor(QPalette::Shadow, QColor(10, 10, 10));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        p.setColor(QPalette::Accent, QColor(42, 130, 218));
#endif
        return p;
    }

    bool horizontal(const QScrollBar* bar) {
        return bar->orientation() == Qt::Horizontal;
    }

    int along(const QScrollBar* bar, const QPoint p) {
        return horizontal(bar) ? p.x() : p.y();
    }

    int across(const QScrollBar* bar, const QPoint p) {
        return horizontal(bar) ? p.y() : p.x();
    }

    /// [start, end) of \p r along \p bar
    std::pair<int, int> span(const QScrollBar* bar, const QRect& r) {
        return horizontal(bar) ? std::make_pair(r.left(), r.right() + 1) : std::make_pair(r.top(), r.bottom() + 1);
    }

    /// [start, end) of \p r across \p bar
    std::pair<int, int> spanAcross(const QScrollBar* bar, const QRect& r) {
        return horizontal(bar) ? std::make_pair(r.top(), r.bottom() + 1) : std::make_pair(r.left(), r.right() + 1);
    }

    /**
     * The band [p(a), p(b - L) + T] derived from the VIEW, independently of the scroll
     * model's state: the 100 x 50 drawing at the origin, in scroll-space pixels, measured
     * against the bar's origin (view start - value; one pixel per tick for this drawing).
     *
     * This re-derives the same formula from the same style queries, so a comparison with
     * it mostly checks rounding. The real guards are the invariant sweeps (here and in
     * lc_scrollmodel_tests.cpp) and the pixel diffs: removing the thumb cutout or the
     * paint rule, or mapping linearly, fails those.
     */
    std::pair<int, int> expectedBand(const ViewFixture& f, const QScrollBar* bar) {
        const bool h = horizontal(bar);
        const RS_Vector factor = f.viewport()->getFactor();
        const double viewStart = h ? -f.ox() : f.oy() - f.viewport()->getHeight();
        const double origin = viewStart - bar->value();
        const double cMin = h ? 0.0 : -50.0 * factor.y;
        const double cMax = h ? 100.0 * factor.x : 0.0;
        const double a = cMin - origin;
        const double b = cMax - origin;

        QStyleOptionSlider option;
        SliderAccess::initOption(bar, &option);
        option.sliderPosition = option.sliderValue = 0;
        const QRect r0 = bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, bar);
        option.sliderPosition = option.sliderValue = option.maximum;
        const QRect r1 = bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, bar);
        const double s0 = h ? r0.left() : r0.top();
        const double travel = (h ? r1.left() : r1.top()) - s0;
        const double length = h ? r0.width() : r0.height();
        const double pageStep = bar->pageStep();
        auto p = [&](const double t) {
            return bar->maximum() > 0 ? s0 + t * travel / bar->maximum() : s0 + t * length / pageStep;
        };
        double lo = p(a);
        double hi = bar->maximum() > 0 ? p(b - pageStep) + length : p(b);
        const bool wide = b - a >= pageStep;
        if (hi - lo < 3.0) {
            const double mid = (lo + hi) / 2;
            lo = mid - 1.5;
            hi = mid + 1.5;
        }
        const double grooveEnd = s0 + travel + length;
        const int start = static_cast<int>(std::clamp(wide ? std::floor(lo) : std::ceil(lo), s0, grooveEnd));
        const int end = static_cast<int>(std::clamp(wide ? std::ceil(hi) : std::floor(hi), s0, grooveEnd));
        return {start, end};
    }

    QImage grabBar(QScrollBar* bar) {
        return bar->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    }

    /// logical pixels where \p a and \p b differ
    std::vector<QPoint> changedPixels(const QImage& a, const QImage& b) {
        std::vector<QPoint> result;
        const double dpr = a.devicePixelRatio();
        for (int y = 0; y < a.height(); ++y) {
            for (int x = 0; x < a.width(); ++x) {
                if (a.pixel(x, y) != b.pixel(x, y)) {
                    result.emplace_back(static_cast<int>(x / dpr), static_cast<int>(y / dpr));
                }
            }
        }
        return result;
    }

    /// pixels of \p a and \p b inside \p r (logical) are identical
    bool identicalIn(const QImage& a, const QImage& b, const QRect& r) {
        const double dpr = a.devicePixelRatio();
        const QRect d(QPoint(static_cast<int>(r.left() * dpr), static_cast<int>(r.top() * dpr)),
                      QSize(static_cast<int>(r.width() * dpr), static_cast<int>(r.height() * dpr)));
        const QRect clipped = d.intersected(a.rect());
        for (int y = clipped.top(); y <= clipped.bottom(); ++y) {
            for (int x = clipped.left(); x <= clipped.right(); ++x) {
                if (a.pixel(x, y) != b.pixel(x, y)) {
                    return false;
                }
            }
        }
        return true;
    }

    /// renders \p bar with the band off and on; leaves the band on
    std::pair<QImage, QImage> grabOffOn(QScrollBar* bar) {
        band(bar)->setContentBandEnabled(false);
        const QImage off = grabBar(bar);
        band(bar)->setContentBandEnabled(true);
        const QImage on = grabBar(bar);
        return {off, on};
    }

    /// the band changes nothing at all: the bar is pixel-identical to the band disabled
    void checkUnpainted(QScrollBar* bar) {
        const auto [off, on] = grabOffOn(bar);
        CHECK(!band(bar)->isContentBandPainted());
        CHECK(off == on);
    }

    /**
     * The painted stripe is [start, end) minus the thumb (+/- 1 px Fusion outline) along the
     * bar, within 1 px, and exactly the stripe's rows (columns for V) across it; the thumb
     * is never painted over.
     */
    void checkPaintedBand(QScrollBar* bar, const std::pair<int, int>& expected) {
        REQUIRE(band(bar)->isContentBandPainted());
        const auto [off, on] = grabOffOn(bar);
        const std::vector<QPoint> changedPoints = changedPixels(off, on);
        std::set<int> changed;
        std::set<int> changedAcross;
        for (const QPoint& p : changedPoints) {
            changed.insert(along(bar, p));
            changedAcross.insert(across(bar, p));
        }
        const auto [t0, t1] = span(bar, thumbRect(bar));
        std::set<int> wanted;
        for (int c = expected.first; c < expected.second; ++c) {
            if (c < t0 - 1 || c >= t1 + 1) {
                wanted.insert(c);
            }
        }
        int stray = 0;
        for (const int c : changed) {
            if (wanted.count(c) == 0 && wanted.count(c - 1) == 0 && wanted.count(c + 1) == 0) {
                ++stray;
            }
        }
        int missing = 0;
        for (const int c : wanted) {
            const bool interior = wanted.count(c - 1) != 0 && wanted.count(c + 1) != 0;
            if (interior && changed.count(c) == 0) {
                ++missing;
            }
        }
        const auto [s0, s1] = spanAcross(bar, band(bar)->contentBandRect());
        INFO("expected band [" << expected.first << ", " << expected.second << ") thumb [" << t0 << ", " << t1
             << ") changed " << changed.size() << " wanted " << wanted.size() << " stripe across [" << s0 << ", " << s1
             << ")");
        CHECK(stray == 0);
        CHECK(missing == 0);
        CHECK(!changedAcross.empty());
        if (!changedAcross.empty()) {
            CHECK(*changedAcross.begin() >= s0);
            CHECK(*changedAcross.rbegin() < s1);
            CHECK(static_cast<int>(changedAcross.size()) == s1 - s0);
        }
        // the thumb itself is never painted over
        CHECK(identicalIn(off, on, thumbRect(bar)));
    }

    /// band pixels along \p bar outside the thumb cut-out (the thumb, one pixel wider each side)
    int bandPixelsOutsideThumb(QScrollBar* bar) {
        const QRect bandRect = band(bar)->contentBandRect();
        if (bandRect.isNull()) {
            return 0;
        }
        const auto [b0, b1] = span(bar, bandRect);
        const auto [t0, t1] = span(bar, thumbRect(bar));
        int count = 0;
        for (int c = b0; c < b1; ++c) {
            count += c < t0 - 1 || c >= t1 + 1 ? 1 : 0;
        }
        return count;
    }

    /**
     * The paint rule: the stripe is painted exactly when some of the band lies outside the
     * thumb cut-out, and it is then the band minus the cut-out (checkPaintedBand()), within
     * 1 px of the band formula; otherwise the bar is pixel-identical to the band disabled.
     * \return whether it is painted
     */
    bool checkBandAsPainted(const ViewFixture& f, QScrollBar* bar) {
        const auto expected = expectedBand(f, bar);
        const auto actual = span(bar, band(bar)->contentBandRect());
        CHECK(std::abs(actual.first - expected.first) <= 1);
        CHECK(std::abs(actual.second - expected.second) <= 1);
        const bool painted = bandPixelsOutsideThumb(bar) > 0;
        CHECK(band(bar)->isContentBandPainted() == painted);
        if (painted) {
            checkPaintedBand(bar, actual);
        } else {
            checkUnpainted(bar);
        }
        return painted;
    }

    bool viewInsideDrawing(const ViewFixture& f, const QScrollBar* bar) {
        const RS_Vector factor = f.viewport()->getFactor();
        if (horizontal(bar)) {
            const double start = -f.ox();
            return start >= 0.0 && start + f.viewport()->getWidth() <= 100.0 * factor.x;
        }
        const double start = f.oy() - f.viewport()->getHeight();
        return start >= -50.0 * factor.y && start + f.viewport()->getHeight() <= 0.0;
    }

    bool viewCoversDrawing(const ViewFixture& f, const QScrollBar* bar) {
        const RS_Vector factor = f.viewport()->getFactor();
        if (horizontal(bar)) {
            const double start = -f.ox();
            return start <= 0.0 && start + f.viewport()->getWidth() >= 100.0 * factor.x;
        }
        const double start = f.oy() - f.viewport()->getHeight();
        return start <= -50.0 * factor.y && start + f.viewport()->getHeight() >= 0.0;
    }

    /// the view and the drawing share no pixel along \p bar: the view has left the drawing
    bool viewDisjointFromDrawing(const ViewFixture& f, const QScrollBar* bar) {
        const RS_Vector factor = f.viewport()->getFactor();
        if (horizontal(bar)) {
            const double start = -f.ox();
            return start + f.viewport()->getWidth() <= 0.0 || start >= 100.0 * factor.x;
        }
        const double start = f.oy() - f.viewport()->getHeight();
        return start + f.viewport()->getHeight() <= -50.0 * factor.y || start >= 0.0;
    }

    /// zooms \p zoom x about the drawing's right edge, then pans the view right until it
    /// starts \p gap pixels past that edge
    void viewPastRightEdge(ViewFixture& f, const double zoom, const int gap) {
        f.viewport()->zoomAuto(false, true);
        f.viewport()->zoomIn(zoom, RS_Vector(100, 25));
        const int start = static_cast<int>(std::ceil(100.0 * f.viewport()->getFactor().x)) + gap;
        f.viewport()->zoomPan(-start - f.ox(), 0);
        pump();
    }

    /// pans the view by \p wx drawing widths right and \p hy drawing heights up, from zoom extents
    void panFromExtents(ViewFixture& f, const double wx, const double hy) {
        f.viewport()->zoomAuto(false, true);
        const RS_Vector factor = f.viewport()->getFactor();
        f.viewport()->zoomPan(static_cast<int>(-wx * 100.0 * factor.x), static_cast<int>(hy * 50.0 * factor.y));
        pump();
    }

    /**
     * Removes an Appearance setting as on a fresh profile. RS_Settings caches every value
     * read or written (a missing key with the reader's default) and does not drop the
     * cache on remove(); an invalid cached value counts as not cached, so the next read
     * goes to the (now missing) key and uses that reader's default.
     */
    void forgetAppearanceSetting(const QString& key) {
        LC_GROUP_GUARD("Appearance");
        RS_SETTINGS->write(key, QVariant());
        RS_SETTINGS->getSettings()->remove("/Appearance/" + key);
    }

    /// restores one Appearance setting to what it was (including "missing") on scope exit
    class AppearanceSettingGuard {
    public:
        explicit AppearanceSettingGuard(QString key) : m_key(std::move(key)) {
            QSettings* s = RS_SETTINGS->getSettings();
            m_existed = s->contains("/Appearance/" + m_key);
            m_saved = s->value("/Appearance/" + m_key);
        }
        ~AppearanceSettingGuard() {
            if (m_existed) {
                LC_GROUP_GUARD("Appearance");
                RS_SETTINGS->write(m_key, m_saved);
            } else {
                forgetAppearanceSetting(m_key);
            }
        }
        AppearanceSettingGuard(const AppearanceSettingGuard&) = delete;
        AppearanceSettingGuard& operator=(const AppearanceSettingGuard&) = delete;

    private:
        QString m_key;
        QVariant m_saved;
        bool m_existed = false;
    };
}

TEST_CASE("Zoomed in: the stripe spans the drawing's range on both sides of the thumb", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const double zoom : {2.0, 4.0, 10.0, 100.0}) {
        f.viewport()->zoomAuto(false, true);
        f.viewport()->zoomIn(zoom, RS_Vector(50, 25));
        pump();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO("zoom " << zoom << " bar " << (horizontal(bar) ? "H" : "V"));
            REQUIRE(viewInsideDrawing(f, bar));
            const QRect bandRect = band(bar)->contentBandRect();
            REQUIRE(!bandRect.isNull());
            const auto [b0, b1] = span(bar, bandRect);
            const auto [t0, t1] = span(bar, thumbRect(bar));
            INFO("band [" << b0 << ", " << b1 << ") thumb [" << t0 << ", " << t1 << ")");
            // the view is centred in the drawing: the band reaches past the thumb on both sides
            CHECK(b0 < t0 - 1);
            CHECK(b1 > t1 + 1);
            REQUIRE(band(bar)->isContentBandPainted());
            checkPaintedBand(bar, expectedBand(f, bar));
            // painted on both sides of the thumb
            const auto [off, on] = grabOffOn(bar);
            bool before = false;
            bool after = false;
            for (const QPoint& p : changedPixels(off, on)) {
                before = before || along(bar, p) < t0 - 1;
                after = after || along(bar, p) >= t1 + 1;
            }
            CHECK(before);
            CHECK(after);
        }
        // panning around inside the drawing keeps it painted
        f.viewport()->zoomPan(f.viewport()->getWidth() / 5, -f.viewport()->getHeight() / 5);
        pump();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO("panned, zoom " << zoom << " bar " << (horizontal(bar) ? "H" : "V"));
            if (viewInsideDrawing(f, bar)) {
                CHECK(checkBandAsPainted(f, bar));
            }
        }
    }
}

TEST_CASE("Zoom Extents (the view covers the drawing): the band lies under the thumb", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const double zoomOut : {1.0, 3.0, 50.0}) {
        f.viewport()->zoomAuto(false, true);
        if (zoomOut > 1.0) {
            f.viewport()->zoomOut(zoomOut, RS_Vector(50, 25));
        }
        pump();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO("zoom out " << zoomOut << " bar " << (horizontal(bar) ? "H" : "V"));
            REQUIRE(viewCoversDrawing(f, bar));
            const auto [b0, b1] = span(bar, band(bar)->contentBandRect());
            const auto [t0, t1] = span(bar, thumbRect(bar));
            INFO("band [" << b0 << ", " << b1 << ") thumb [" << t0 << ", " << t1 << ")");
            CHECK(b1 > b0);
            // the thumb covers the band (within the cut-out's extra pixel), so nothing shows
            CHECK(b0 >= t0 - 1);
            CHECK(b1 <= t1 + 1);
            CHECK(!checkBandAsPainted(f, bar));
        }
    }
}

TEST_CASE("Panned 50 widths away: a stripe at the far end, never thumb-shaped", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const int side : {1, -1}) {
        // side 1: the view goes right and down, so the drawing is at the start of both bars
        panFromExtents(f, 50.0 * side, -50.0 * side);
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO("side " << side << " bar " << (horizontal(bar) ? "H" : "V"));
            REQUIRE(band(bar)->isContentBandPainted());
            const auto [off, on] = grabOffOn(bar);
            const std::vector<QPoint> changed = changedPixels(off, on);
            REQUIRE(!changed.empty());
            std::set<int> alongSet;
            std::set<int> acrossSet;
            for (const QPoint& p : changed) {
                alongSet.insert(along(bar, p));
                acrossSet.insert(across(bar, p));
            }
            const auto [t0, t1] = span(bar, thumbRect(bar));
            const auto [g0, g1] = span(bar, grooveRect(bar));
            const auto [gc0, gc1] = spanAcross(bar, grooveRect(bar));
            const auto [tc0, tc1] = spanAcross(bar, thumbRect(bar));
            // the thumb sits half a view short of the far end (the room beyond the view)
            const int room = side > 0 ? bar->maximum() - bar->value() : bar->value();
            CHECK(std::abs(room - bar->pageStep() / 2) <= 1);
            if (side > 0) {
                CHECK(t1 <= g1);
                CHECK(*alongSet.rbegin() < t0 - 1); // the stripe at the other end
                CHECK(*alongSet.begin() >= g0);
            } else {
                CHECK(t0 >= g0);
                CHECK(*alongSet.begin() >= t1 + 1);
                CHECK(*alongSet.rbegin() < g1);
            }
            // a stripe along the outer edge of the groove, not thumb-thick
            const int stripeThickness = static_cast<int>(acrossSet.size());
            const int grooveThickness = gc1 - gc0;
            const int expectedThickness = std::max(2, static_cast<int>(std::lround(0.25 * grooveThickness)));
            CHECK(stripeThickness == expectedThickness);
            CHECK(stripeThickness != tc1 - tc0);
            CHECK(stripeThickness * 2 < tc1 - tc0);
            CHECK(*acrossSet.rbegin() == gc1 - 1); // bottom (H) / right (V) edge
            CHECK(*acrossSet.begin() == gc1 - expectedThickness);
            CHECK(identicalIn(off, on, thumbRect(bar)));
        }
    }
}

TEST_CASE("Straddling the right edge: the stripe runs from the drawing's start to the thumb",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const double zoom : {4.0, 10.0, 100.0}) {
        f.viewport()->zoomAuto(false, true);
        f.viewport()->zoomIn(zoom, RS_Vector(100, 25)); // the right edge stays in view
        pump();
        INFO("zoom " << zoom);
        QScrollBar* bar = f.hBar;
        REQUIRE(!viewInsideDrawing(f, bar));
        REQUIRE(!viewCoversDrawing(f, bar));
        REQUIRE(!viewDisjointFromDrawing(f, bar));
        const auto expected = expectedBand(f, bar);
        const auto [t0, t1] = span(bar, thumbRect(bar));
        INFO("band [" << expected.first << ", " << expected.second << ") thumb [" << t0 << ", " << t1 << ")");
        // the drawing's range left of the view: the band starts left of the thumb and ends
        // under it, never past it
        CHECK(expected.first < t0 - 1);
        CHECK(expected.second > t0);
        CHECK(expected.second <= t1 + 1);
        REQUIRE(band(bar)->isContentBandPainted());
        checkPaintedBand(bar, expected);
        // the vertical bar: the view is inside the drawing's height, painted both sides
        REQUIRE(viewInsideDrawing(f, f.vBar));
        CHECK(checkBandAsPainted(f, f.vBar));
    }
}

TEST_CASE("View entirely past the right edge: the stripe covers the drawing's range, the thumb cut out",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const double zoom : {4.0, 100.0}) {
        for (const int gap : {0, 2, 40}) {
            viewPastRightEdge(f, zoom, gap);
            INFO("zoom " << zoom << " gap " << gap);
            QScrollBar* bar = f.hBar;
            REQUIRE(viewDisjointFromDrawing(f, bar));
            const auto expected = expectedBand(f, bar);
            const auto [t0, t1] = span(bar, thumbRect(bar));
            const auto [g0, g1] = span(bar, grooveRect(bar));
            INFO("band [" << expected.first << ", " << expected.second << ") thumb [" << t0 << ", " << t1
                 << ") groove [" << g0 << ", " << g1 << ")");
            // the drawing is left of the view: the band starts left of the thumb, well inside
            // the groove at 4x (drawing 4 views, margin half a view)
            CHECK(expected.first < t0 - 1);
            if (zoom < 10.0) {
                CHECK(expected.first > g0);
            }
            if (zoom > 10.0 && gap < 10) {
                // the thumb is clamped to the style's minimum length, longer than the view's
                // share of the bar, so the band reaches under it: only the cutout keeps it off
                CHECK(expected.second > t0 + 1);
            }
            checkPaintedBand(bar, expected);
            // the vertical bar: the view is still inside the drawing's height
            REQUIRE(viewInsideDrawing(f, f.vBar));
            CHECK(checkBandAsPainted(f, f.vBar));
        }
    }
}

TEST_CASE("The painted stripe matches the band formula within 1 px", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    REQUIRE(band(f.hBar)->isContentBandEnabled());

    struct Case {
        const char* name;
        std::function<void()> setUp;
    };
    const int width = f.viewport()->getWidth();
    const int height = f.viewport()->getHeight();
    const std::vector<Case> cases{
        {"zoomed in 4x at a corner", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomIn(4.0, RS_Vector(2, 2)); }},
        {"zoomed out 3x, off-centre", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomOut(3.0, RS_Vector(50, 25)); f.viewport()->zoomPan(width / 2, -height / 2); }},
        {"panned 2 widths", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomPan(-2 * width, 2 * height); }},
        {"panned 20 widths", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomPan(20 * width, -20 * height); }},
        {"panned 200 widths", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomPan(-200 * width, -200 * height); }},
        {"zoomed in 10x, the view just past the lower left corner", [&] { f.viewport()->zoomAuto(false, true); f.viewport()->zoomIn(10.0, RS_Vector(0, 0)); f.viewport()->zoomPan(width, -height); }},
    };
    int painted = 0;
    for (const Case& c : cases) {
        c.setUp();
        pump();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO(c.name << " bar " << (horizontal(bar) ? "H" : "V"));
            painted += checkBandAsPainted(f, bar) ? 1 : 0;
            if (viewDisjointFromDrawing(f, bar)) {
                CHECK(band(bar)->isContentBandPainted());
            }
        }
    }
    CHECK(painted >= 10);
}

TEST_CASE("The thumb lies inside the band whenever the view lies inside the drawing",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    int insideSamples = 0;
    int coverSamples = 0;
    int paintedSamples = 0;
    for (const double zoom : {6.0, 1.0 / 3.0}) {
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            f.viewport()->zoomAuto(false, true);
            if (zoom > 1.0) {
                f.viewport()->zoomIn(zoom, RS_Vector(50, 25));
            } else {
                f.viewport()->zoomOut(1.0 / zoom, RS_Vector(50, 25));
            }
            const bool h = horizontal(bar);
            const int viewLength = h ? f.viewport()->getWidth() : f.viewport()->getHeight();
            const int steps = 240;
            const int step = std::max(1, (6 * viewLength + 7 * (h ? 100 : 50) * static_cast<int>(f.factor())) / steps);
            // start far on one side, sweep across to far on the other
            f.viewport()->zoomPan(h ? steps / 2 * step : 0, h ? 0 : -steps / 2 * step);
            for (int i = 0; i <= steps; ++i) {
                pump();
                const QRect thumb = thumbRect(bar);
                const QRect bandRect = band(bar)->contentBandRect();
                REQUIRE(!bandRect.isNull());
                const auto [t0, t1] = span(bar, thumb);
                const auto [b0, b1] = span(bar, bandRect);
                const bool inside = viewInsideDrawing(f, bar);
                const bool covers = viewCoversDrawing(f, bar);
                INFO("zoom " << zoom << " bar " << (h ? "H" : "V") << " step " << i << " thumb [" << t0 << ", "
                     << t1 << ") band [" << b0 << ", " << b1 << ")");
                if (inside) {
                    ++insideSamples;
                    CHECK((t0 >= b0 && t1 <= b1));
                }
                if (covers && b1 - b0 > 4) {
                    ++coverSamples;
                    CHECK((t0 <= b0 && t1 >= b1));
                }
                // One-directional (see LC_ScrollModel::bandPixels()): a thumb inside the band
                // is a view inside the drawing only to within the style's rounding.
                if (t0 >= b0 + 2 && t1 <= b1 - 2) {
                    CHECK(inside);
                }
                // the paint rule: painted exactly while some of the band lies outside the
                // thumb cut-out; always so once the view has left the drawing, and while
                // zoomed in with the view inside it
                const bool painted = band(bar)->isContentBandPainted();
                CHECK(painted == (bandPixelsOutsideThumb(bar) > 0));
                if (viewDisjointFromDrawing(f, bar) || (inside && zoom > 1.0)) {
                    CHECK(painted);
                }
                paintedSamples += painted ? 1 : 0;
                f.viewport()->zoomPan(h ? -step : 0, h ? 0 : step);
            }
        }
    }
    CHECK(insideSamples > 20);
    CHECK(coverSamples > 20);
    CHECK(paintedSamples > 100);
}

TEST_CASE("Thumb-rect pixels are identical with the band on and off", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    int checked = 0;
    for (const double widths : {0.5, 1.5, 3.0, 30.0}) {
        for (const int side : {1, -1}) {
            panFromExtents(f, widths * side, widths * side);
            for (QScrollBar* bar : {f.hBar, f.vBar}) {
                INFO("widths " << widths << " side " << side << " bar " << (horizontal(bar) ? "H" : "V"));
                const auto [off, on] = grabOffOn(bar);
                const QRect thumb = thumbRect(bar);
                const QRect grown = horizontal(bar) ? thumb.adjusted(-1, 0, 1, 0) : thumb.adjusted(0, -1, 0, 1);
                CHECK(identicalIn(off, on, grown));
                checked += band(bar)->isContentBandPainted() ? 1 : 0;
            }
        }
    }
    CHECK(checked >= 12);
}

TEST_CASE("Band frozen while the thumb is held, resynced with the range on release", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomPan(-10 * f.viewport()->getWidth(), 0); // view right, drawing at the left
    pump();
    const QRect held = band(f.hBar)->contentBandRect();
    REQUIRE(!held.isNull());
    REQUIRE(band(f.hBar)->isContentBandPainted());
    const QImage before = grabBar(f.hBar);
    // plain ints, not a structured binding: lambdas may not capture those before C++20
    const std::pair<int, int> heldSpan = span(f.hBar, held);
    const int b0 = heldSpan.first;
    const int b1 = heldSpan.second;

    bool frozen = true;
    bool paintedFrozen = true;
    // a short drag toward the drawing that stays well off it
    dragThumb(f.hBar, 5, -20, [&](int) {
        frozen = frozen && f.hBar->isSliderDown() && band(f.hBar)->contentBandRect() == held
            && band(f.hBar)->isContentBandPainted();
        // the painted stripe did not move either: its pixels outside the (moving) thumb are unchanged
        const QImage now = grabBar(f.hBar);
        const auto [t0, t1] = span(f.hBar, thumbRect(f.hBar));
        for (int c = b0 + 1; c < b1 - 1; ++c) {
            if (c >= t0 - 2 && c < t1 + 2) {
                continue;
            }
            paintedFrozen = paintedFrozen && identicalIn(before, now, QRect(c, 0, 1, f.hBar->height()));
        }
    });
    CHECK(frozen);
    CHECK(paintedFrozen);
    CHECK(!f.hBar->isSliderDown());
    // released: the region shrank back and the band resynced with it
    const QRect released = band(f.hBar)->contentBandRect();
    CHECK(released != held);
    const auto expected = expectedBand(f, f.hBar);
    const auto actual = span(f.hBar, released);
    CHECK(std::abs(actual.first - expected.first) <= 1);
    CHECK(std::abs(actual.second - expected.second) <= 1);
    checkPaintedBand(f.hBar, expected);
}

TEST_CASE("Bar steps keep the band and the range in one tick space", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    panFromExtents(f, 3.0, -3.0);
    auto click = [](QScrollBar* bar, const QPoint p) {
        sendMouse(bar, QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
        sendMouse(bar, QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::NoButton);
        pump();
    };
    for (QScrollBar* bar : {f.hBar, f.vBar}) {
        for (int i = 0; i < 6; ++i) {
            const QRect t = thumbRect(bar);
            const QRect g = grooveRect(bar);
            if (i % 3 == 2) {
                // a page step in the trough before the thumb (toward the drawing)
                click(bar, horizontal(bar) ? QPoint((g.left() + t.left()) / 2, t.center().y())
                                           : QPoint(t.center().x(), (g.bottom() + t.bottom()) / 2));
            } else {
                click(bar, subControlRect(bar, horizontal(bar) ? QStyle::SC_ScrollBarSubLine
                                                               : QStyle::SC_ScrollBarAddLine).center());
            }
            INFO("bar " << (horizontal(bar) ? "H" : "V") << " step " << i << " value " << bar->value());
            checkBandAsPainted(f, bar);
        }
    }
}

TEST_CASE("Scrollbar input behaves identically with the band on and off", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture on;
    ViewFixture off;
    useFusion(on);
    useFusion(off);
    band(off.hBar)->setContentBandEnabled(false);
    band(off.vBar)->setContentBandEnabled(false);
    REQUIRE(band(on.hBar)->isContentBandEnabled());

    struct Result {
        int ox;
        int oy;
        int h;
        int v;
        bool operator==(const Result& o) const { return ox == o.ox && oy == o.oy && h == o.h && v == o.v; }
    };
    // 4x about the lower left corner, then one view left and down: the view has just left
    // the drawing on both axes, so the band is painted on both bars
    auto run = [](ViewFixture& f, const std::function<void(ViewFixture&)>& input) {
        f.viewport()->zoomAuto(false, true);
        f.viewport()->zoomIn(4.0, RS_Vector(0, 0));
        f.viewport()->zoomPan(f.viewport()->getWidth(), -f.viewport()->getHeight());
        pump();
        input(f);
        pump();
        return Result{f.ox(), f.oy(), f.hBar->value(), f.vBar->value()};
    };
    auto click = [](QScrollBar* bar, const QPoint p) {
        sendMouse(bar, QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
        sendMouse(bar, QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::NoButton);
    };
    const std::vector<std::pair<const char*, std::function<void(ViewFixture&)>>> inputs{
        {"H drag", [](ViewFixture& f) { dragThumb(f.hBar, 5, 13); }},
        {"V drag", [](ViewFixture& f) { dragThumb(f.vBar, 5, -11); }},
        {"H trough after", [&](ViewFixture& f) {
            const QRect t = thumbRect(f.hBar);
            click(f.hBar, QPoint((t.right() + grooveRect(f.hBar).right()) / 2, t.center().y()));
        }},
        {"V trough before", [&](ViewFixture& f) {
            const QRect t = thumbRect(f.vBar);
            click(f.vBar, QPoint(t.center().x(), (t.top() + grooveRect(f.vBar).top()) / 2));
        }},
        {"H add arrow", [&](ViewFixture& f) { click(f.hBar, subControlRect(f.hBar, QStyle::SC_ScrollBarAddLine).center()); }},
        {"V sub arrow", [&](ViewFixture& f) { click(f.vBar, subControlRect(f.vBar, QStyle::SC_ScrollBarSubLine).center()); }},
        {"H wheel", [](ViewFixture& f) { CHECK(sendWheel(f.hBar, QPoint(0, -120))); }},
        {"V wheel", [](ViewFixture& f) { CHECK(sendWheel(f.vBar, QPoint(0, 120))); }},
    };
    for (const auto& [name, input] : inputs) {
        const Result withBand = run(on, input);
        const Result withoutBand = run(off, input);
        INFO(name << " on (" << withBand.ox << "," << withBand.oy << "," << withBand.h << "," << withBand.v
             << ") off (" << withoutBand.ox << "," << withoutBand.oy << "," << withoutBand.h << ","
             << withoutBand.v << ")");
        CHECK(withBand == withoutBand);
    }

    // the style's hit-testing is the same at every pixel of both bars, with the band painted
    run(on, [](ViewFixture&) {});
    run(off, [](ViewFixture&) {});
    for (const int axis : {0, 1}) {
        QScrollBar* a = axis == 0 ? on.hBar : on.vBar;
        QScrollBar* b = axis == 0 ? off.hBar : off.vBar;
        REQUIRE(a->size() == b->size());
        CHECK(band(a)->isContentBandPainted());
        QStyleOptionSlider oa;
        QStyleOptionSlider ob;
        SliderAccess::initOption(a, &oa);
        SliderAccess::initOption(b, &ob);
        int differences = 0;
        for (int y = 0; y < a->height(); ++y) {
            for (int x = 0; x < a->width(); ++x) {
                const auto ha = a->style()->hitTestComplexControl(QStyle::CC_ScrollBar, &oa, QPoint(x, y), a);
                const auto hb = b->style()->hitTestComplexControl(QStyle::CC_ScrollBar, &ob, QPoint(x, y), b);
                differences += ha != hb ? 1 : 0;
            }
        }
        CHECK(differences == 0);
    }
}

TEST_CASE("Far away, the band sits at the opposite end of the thumb", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    for (const int widths : {2, 20, 200, 2000}) {
        for (const int side : {1, -1}) {
            f.viewport()->zoomAuto(false, true);
            // side 1: the view goes right (H) and down (V), so the drawing is at the start of both bars
            f.viewport()->zoomPan(-side * widths * f.viewport()->getWidth(), -side * widths * f.viewport()->getHeight());
            pump();
            for (QScrollBar* bar : {f.hBar, f.vBar}) {
                const auto [t0, t1] = span(bar, thumbRect(bar));
                const auto [b0, b1] = span(bar, band(bar)->contentBandRect());
                const auto [g0, g1] = span(bar, grooveRect(bar));
                INFO("widths " << widths << " side " << side << " bar " << (horizontal(bar) ? "H" : "V")
                     << " thumb [" << t0 << ", " << t1 << ") band [" << b0 << ", " << b1 << ")");
                CHECK(b1 - b0 >= 3);
                CHECK(b0 >= g0);
                CHECK(b1 <= g1);
                // the thumb sits half a view short of the far end (the room beyond the view)
                const int room = side > 0 ? bar->maximum() - bar->value() : bar->value();
                CHECK(std::abs(room - bar->pageStep() / 2) <= 1);
                if (side > 0) {
                    CHECK(b1 <= t0);
                    CHECK(t1 <= g1);
                } else {
                    CHECK(b0 >= t1);
                    CHECK(t0 >= g0);
                }
                checkPaintedBand(bar, expectedBand(f, bar));
            }
        }
    }
}

TEST_CASE("Stripe contrast is at least 3:1 against the groove, Fusion light and dark", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    panFromExtents(f, 20.0, -20.0);
    QColor previous;
    for (const bool dark : {false, true}) {
        const QPalette palette = dark ? darkPalette() : fusionStyle()->standardPalette();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            bar->setPalette(palette); // a PaletteChange drops the cached colour
        }
        pump();
        for (QScrollBar* bar : {f.hBar, f.vBar}) {
            INFO((dark ? "dark" : "light") << " bar " << (horizontal(bar) ? "H" : "V"));
            REQUIRE(band(bar)->isContentBandPainted());
            const QColor expected = QG_ScrollBar::contentBandColorFor(palette);
            CHECK(band(bar)->contentBandColor() == expected);
            CHECK(expected.alpha() == 255);
            CHECK(RS_Color::contrastRatio(expected, palette.color(QPalette::Window)) >= 3.0);

            const QImage image = grabBar(bar);
            const QRect stripe = band(bar)->contentBandRect();
            const QRect groove = grooveRect(bar);
            const double dpr = image.devicePixelRatio();
            // a sample in the middle of the stripe, and one in the groove's middle, same column
            const QPoint stripePoint = stripe.center();
            const QPoint groovePoint = horizontal(bar) ? QPoint(stripePoint.x(), groove.center().y())
                                                       : QPoint(groove.center().x(), stripePoint.y());
            const QColor stripeColor = image.pixelColor(static_cast<int>(stripePoint.x() * dpr),
                                                        static_cast<int>(stripePoint.y() * dpr));
            const QColor grooveColor = image.pixelColor(static_cast<int>(groovePoint.x() * dpr),
                                                        static_cast<int>(groovePoint.y() * dpr));
            INFO("stripe " << stripeColor.name().toStdString() << " groove " << grooveColor.name().toStdString()
                 << " contrast " << RS_Color::contrastRatio(stripeColor, grooveColor));
            CHECK(stripeColor == expected);
            CHECK(RS_Color::contrastRatio(stripeColor, grooveColor) >= 3.0);
        }
        if (previous.isValid()) {
            CHECK(band(f.hBar)->contentBandColor() != previous);
        }
        previous = band(f.hBar)->contentBandColor();
    }
}

TEST_CASE("The stripe colour keeps 3:1 contrast for any accent", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    const std::vector<QColor> accents{QColor(255, 235, 59), QColor(255, 255, 255), QColor(0, 122, 255),
                                      QColor(40, 40, 40), QColor(0, 0, 0), QColor(240, 240, 240),
                                      QColor(255, 200, 0), QColor(90, 90, 90)};
    for (const bool dark : {false, true}) {
        for (const QColor& accent : accents) {
            QPalette palette = dark ? darkPalette() : fusionStyle()->standardPalette();
            palette.setColor(QPalette::Highlight, accent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            palette.setColor(QPalette::Accent, accent);
#endif
            const QColor color = QG_ScrollBar::contentBandColorFor(palette);
            INFO((dark ? "dark " : "light ") << accent.name().toStdString() << " -> " << color.name().toStdString());
            CHECK(color.alpha() == 255);
            CHECK(RS_Color::contrastRatio(color, palette.color(QPalette::Window)) >= 3.0);
        }
    }
    // a good accent is used as it is (light) and lightened (dark)
    QPalette light = fusionStyle()->standardPalette();
    light.setColor(QPalette::Highlight, QColor(0, 90, 200));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    light.setColor(QPalette::Accent, QColor(0, 90, 200));
#endif
    CHECK(QG_ScrollBar::contentBandColorFor(light) == QColor(0, 90, 200));
    CHECK(RS_Color::contrastRatio(Qt::black, Qt::white) == Catch::Approx(21.0));
}

TEST_CASE("The tooltip gives both ranges and where the drawing is", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard guard("ScrollBarContentBand");
    LC_SET_ONE("Appearance", "ScrollBarContentBand", true);
    ViewFixture f;
    f.view->loadSettings();
    useFusion(f);
    // bars without a provider (e.g. previews) keep the plain QWidget tooltip
    CHECK(band(f.hBar)->providedToolTip().isEmpty());
    f.view->setScrollBarToolTips(true);

    const RS2::Unit unit = f.graphic->getUnit();
    const RS2::LinearFormat format = f.graphic->getLinearFormat();
    const int precision = f.graphic->getLinearPrecision();
    auto linear = [&](const double v) {
        return RS_Units::formatLinear(v, unit, format, precision);
    };
    // the view's UCS range, independently of the scroll code: the GUI edges mapped to UCS
    auto viewRange = [&](const bool h) {
        const auto* vp = f.viewport();
        if (h) {
            return std::make_pair(vp->toUCSFromGui(0, 0).x, vp->toUCSFromGui(vp->getWidth(), 0).x);
        }
        return std::make_pair(vp->toUCSFromGui(0, vp->getHeight()).y, vp->toUCSFromGui(0, 0).y);
    };
    const QString dot = QStringLiteral(" · ");
    auto expectedPrefix = [&](const bool h) -> QString {
        const auto [v0, v1] = viewRange(h);
        const QString axis = h ? "X" : "Y";
        return "Drawing " + axis + " " + linear(0.0) + ".." + linear(h ? 100.0 : 50.0) + dot + "View " + axis + " "
            + linear(v0) + ".." + linear(v1) + dot;
    };

    struct Case {
        const char* name;
        double wx; // drawing widths the view is panned right
        double hy; // drawing heights the view is panned up
        QString h; // expected: a direction, or the whole where-text
        QString v;
    };
    const QString covers = "covers"; // at zoom extents, along the other axis
    auto coversText = [](const bool h) {
        return h ? QString("view spans the drawing's X range") : QString("view spans the drawing's Y range");
    };
    auto inViewText = [](const bool h) {
        return h ? QString("drawing's X range is in view") : QString("drawing's Y range is in view");
    };
    const std::vector<Case> cases{
        {"view right", 5.0, 0.0, "to the left", covers},
        {"view left", -5.0, 0.0, "to the right", covers},
        {"view up", 0.0, 7.0, covers, "down"},
        {"view down", 0.0, -7.0, covers, "up"},
        {"view far right", 300.0, 0.0, "to the left", covers},
    };
    int distances = 0;
    for (const Case& c : cases) {
        panFromExtents(f, c.wx, c.hy);
        for (const bool h : {true, false}) {
            QScrollBar* bar = h ? f.hBar : f.vBar;
            const QString text = band(bar)->providedToolTip();
            INFO(c.name << " bar " << (h ? "H" : "V") << ": " << text.toStdString());
            CHECK(text == f.view->scrollBarToolTip(h));
            REQUIRE(text.startsWith(expectedPrefix(h)));
            const QString where = text.mid(expectedPrefix(h).size());
            const QString want = h ? c.h : c.v;
            if (want == covers) {
                CHECK(where == coversText(h));
                continue;
            }
            ++distances;
            const auto [v0, v1] = viewRange(h);
            const double drawingMin = 0.0;
            const double drawingMax = h ? 100.0 : 50.0;
            const double distance = (want == "to the left" || want == "down") ? v0 - drawingMax : drawingMin - v1;
            const QString lengths = h ? "view widths" : "view heights";
            const QString prefix = QString(h ? "drawing's X range is " : "drawing's Y range is ") + linear(distance)
                + " " + want + " (≈";
            CHECK(where.startsWith(prefix));
            CHECK(where.endsWith(" " + lengths + ")"));
            const double count = where.mid(prefix.size()).section(' ', 0, 0).toDouble();
            const double expectedCount = distance / (v1 - v0);
            CHECK(std::abs(count - expectedCount) <= (expectedCount < 10.0 ? 0.05 : 0.5));
            CHECK(distance > 0.0);
        }
    }
    CHECK(distances == 5);
    // just past the drawing's right edge: a distance far below a tenth of the view
    viewPastRightEdge(f, 4.0, 2);
    {
        const auto [v0, v1] = viewRange(true);
        INFO(band(f.hBar)->providedToolTip().toStdString());
        CHECK((v0 - 100.0) / (v1 - v0) < 0.05);
        CHECK(band(f.hBar)->providedToolTip()
              == expectedPrefix(true) + "drawing's X range is " + linear(v0 - 100.0)
                     + " to the left (less than 0.1 view widths)");
    }
    // in view (inside the drawing) and covering it
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(4.0, RS_Vector(50, 25));
    pump();
    CHECK(band(f.hBar)->providedToolTip() == expectedPrefix(true) + inViewText(true));
    CHECK(band(f.vBar)->providedToolTip() == expectedPrefix(false) + inViewText(false));
    f.viewport()->zoomAuto(false, true);
    pump();
    CHECK(band(f.hBar)->providedToolTip() == expectedPrefix(true) + coversText(true));
    CHECK(band(f.vBar)->providedToolTip() == expectedPrefix(false) + coversText(false));

    // the bar answers QEvent::ToolTip itself
    QHelpEvent help(QEvent::ToolTip, f.hBar->rect().center(), f.hBar->mapToGlobal(f.hBar->rect().center()));
    CHECK(QApplication::sendEvent(f.hBar, &help));

    // an empty drawing
    ViewFixture empty(QSize(800, 600), true, nullptr);
    empty.view->loadSettings();
    empty.view->setScrollBarToolTips(true);
    CHECK(band(empty.hBar)->providedToolTip().startsWith("The drawing is empty · View X "));

    // no tooltip at all while the band is switched off, and back when it is on again
    panFromExtents(f, 5.0, 7.0);
    REQUIRE(!band(f.hBar)->providedToolTip().isEmpty());
    LC_SET_ONE("Appearance", "ScrollBarContentBand", false);
    f.view->loadSettings();
    for (const bool h : {true, false}) {
        CHECK(f.view->scrollBarToolTip(h).isEmpty());
        CHECK(band(h ? f.hBar : f.vBar)->providedToolTip().isEmpty());
    }
    QHelpEvent silent(QEvent::ToolTip, f.hBar->rect().center(), f.hBar->mapToGlobal(f.hBar->rect().center()));
    QApplication::sendEvent(f.hBar, &silent);
    CHECK(!silent.isAccepted());
    LC_SET_ONE("Appearance", "ScrollBarContentBand", true);
    f.view->loadSettings();
    CHECK(band(f.hBar)->providedToolTip().startsWith(expectedPrefix(true) + "drawing's X range is "));
    CHECK(band(f.vBar)->providedToolTip().startsWith(expectedPrefix(false) + "drawing's Y range is "));
    // removing the provider
    f.view->setScrollBarToolTips(false);
    CHECK(band(f.hBar)->providedToolTip().isEmpty());
}

TEST_CASE("Tooltip: a view exactly touching the drawing's edge counts as in view, on each side",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard guard("ScrollBarContentBand");
    LC_SET_ONE("Appearance", "ScrollBarContentBand", true);
    ViewFixture f;
    f.view->loadSettings();
    useFusion(f);
    f.view->setScrollBarToolTips(true);
    const RS2::Unit unit = f.graphic->getUnit();
    const RS2::LinearFormat format = f.graphic->getLinearFormat();
    const int precision = f.graphic->getLinearPrecision();
    auto linear = [&](const double v) {
        return RS_Units::formatLinear(v, unit, format, precision);
    };
    // 4 px per unit: the 100 x 50 drawing is 400 x 200 px, and every edge lands on a whole
    // pixel, so a view edge can meet a drawing edge exactly
    const double scale = 4.0;
    const int width = f.viewport()->getWidth();
    const int height = f.viewport()->getHeight();
    const int ox0 = f.ox();
    const int oy0 = f.oy();
    struct Case {
        const char* name;
        bool horizontal;
        int offset;       // offset x (H) or y (V); the other offset stays put
        const char* where; // "in view", or the direction of the drawing's range
        double distance;
    };
    const std::vector<Case> cases{
        // the view starts exactly at the drawing's right edge (x = 100), then a pixel past it
        {"touching the right edge", true, -400, "in view", 0.0},
        {"a pixel past the right edge", true, -401, "to the left", 0.25},
        // the view ends exactly at the drawing's left edge (x = 0), then a pixel before it
        {"touching the left edge", true, width, "in view", 0.0},
        {"a pixel before the left edge", true, width + 1, "to the right", 0.25},
        // the view's bottom exactly at the drawing's top (y = 50), then a pixel above it
        {"touching the top edge", false, -200, "in view", 0.0},
        {"a pixel above the top edge", false, -201, "down", 0.25},
        // the view's top exactly at the drawing's bottom (y = 0), then a pixel below it
        {"touching the bottom edge", false, height, "in view", 0.0},
        {"a pixel below the bottom edge", false, height + 1, "up", 0.25},
    };
    for (const Case& c : cases) {
        f.viewport()->setOffsetAndFactor(c.horizontal ? c.offset : ox0, c.horizontal ? oy0 : c.offset, scale);
        pump();
        const QString text = f.view->scrollBarToolTip(c.horizontal);
        INFO(c.name << ": " << text.toStdString());
        // the view range: exactly on the drawing's edge (or a quarter unit past it)
        const double viewMin = -(c.horizontal ? f.ox() : f.oy()) / scale;
        const double viewMax = ((c.horizontal ? width : height) - (c.horizontal ? f.ox() : f.oy())) / scale;
        const double drawingMax = c.horizontal ? 100.0 : 50.0;
        const QString axis = c.horizontal ? "X" : "Y";
        const QString where = QString(c.where) == "in view"
            ? QString("drawing's " + axis + " range is in view")
            : QString("drawing's " + axis + " range is " + linear(c.distance) + " " + c.where + " (less than 0.1 view "
                      + (c.horizontal ? "widths" : "heights") + ")");
        CHECK(text == "Drawing " + axis + " " + linear(0.0) + ".." + linear(drawingMax) + " · View " + axis + " "
                          + linear(viewMin) + ".." + linear(viewMax) + " · " + where);
        // the same rule as LC_ScrollModel::placement()
        const auto placement = LC_ScrollModel::placement(0.0, drawingMax, viewMin, viewMax);
        CHECK((placement == LC_ScrollModel::Placement::Overlaps) == (QString(c.where) == "in view"));
    }
}

TEST_CASE("Tooltip: the drawing's extents are read when asked, not from the last bar sync",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard guard("ScrollBarContentBand");
    LC_SET_ONE("Appearance", "ScrollBarContentBand", true);
    ViewFixture f(QSize(800, 600), true, nullptr);
    f.view->loadSettings();
    useFusion(f);
    f.view->setScrollBarToolTips(true);
    const RS2::Unit unit = f.graphic->getUnit();
    const RS2::LinearFormat format = f.graphic->getLinearFormat();
    const int precision = f.graphic->getLinearPrecision();
    auto linear = [&](const double v) {
        return RS_Units::formatLinear(v, unit, format, precision);
    };
    CHECK(f.view->scrollBarToolTip(true).startsWith("The drawing is empty · View X "));
    CHECK(f.view->scrollBarToolTip(false).startsWith("The drawing is empty · View Y "));
    // content added with no viewport change, so no bar resync (as on a load or an edit that
    // does not redraw): the tooltip already describes it
    addRectangle(f.graphic);
    CHECK(f.view->scrollBarToolTip(true).startsWith("Drawing X " + linear(0.0) + ".." + linear(100.0) + " · View X "));
    CHECK(f.view->scrollBarToolTip(false).startsWith("Drawing Y " + linear(0.0) + ".." + linear(50.0) + " · View Y "));
    // and an edit that grows the drawing
    addRectangle(f.graphic, RS_Vector(1000, 0), RS_Vector(1100, 500));
    CHECK(f.view->scrollBarToolTip(true).startsWith("Drawing X " + linear(0.0) + ".." + linear(1100.0) + " · "));
    CHECK(f.view->scrollBarToolTip(false).startsWith("Drawing Y " + linear(0.0) + ".." + linear(500.0) + " · "));
}

TEST_CASE("Live toggle via loadSettings does not move the view", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard guard("ScrollBarContentBand");
    ViewFixture f;
    useFusion(f);
    panFromExtents(f, 4.0, -4.0);
    const int ox = f.ox();
    const int oy = f.oy();
    const int h = f.hBar->value();
    const int v = f.vBar->value();
    const int hMax = f.hBar->maximum();
    const QRect bandRect = band(f.hBar)->contentBandRect();
    REQUIRE(band(f.hBar)->isContentBandPainted());
    for (const bool enabled : {false, true, false, true}) {
        LC_SET_ONE("Appearance", "ScrollBarContentBand", enabled);
        f.view->loadSettings();
        pump();
        INFO("enabled " << enabled);
        CHECK(band(f.hBar)->isContentBandEnabled() == enabled);
        CHECK(band(f.vBar)->isContentBandEnabled() == enabled);
        CHECK(band(f.hBar)->isContentBandPainted() == enabled);
        CHECK(f.ox() == ox);
        CHECK(f.oy() == oy);
        CHECK(f.hBar->value() == h);
        CHECK(f.vBar->value() == v);
        CHECK(f.hBar->maximum() == hMax);
        if (enabled) {
            CHECK(band(f.hBar)->contentBandRect() == bandRect);
        } else {
            CHECK(band(f.hBar)->contentBandRect().isNull());
        }
    }
}

TEST_CASE("Hatch and dimension style previews follow the setting, without tooltips", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard guard("ScrollBarContentBand");
    for (const bool enabled : {false, true}) {
        LC_SET_ONE("Appearance", "ScrollBarContentBand", enabled);
        INFO("enabled " << enabled);
        // the dimension style manager's preview
        {
            RS_Graphic original;
            original.initForNewDocument();
            QWidget parent;
            LC_DimStylePreviewGraphicView* preview =
                LC_DimStylePreviewGraphicView::init(&parent, &original, RS2::EntityDimLinear);
            REQUIRE(preview != nullptr);
            const QList<QG_ScrollBar*> bars = preview->findChildren<QG_ScrollBar*>();
            CHECK(bars.size() == 2);
            for (QG_ScrollBar* bar : bars) {
                CHECK(bar->isContentBandEnabled() == enabled);
                CHECK(bar->providedToolTip().isEmpty());
            }
        }
        // the hatch dialog's preview (QG_DlgHatch::init(): addScrollbars(); loadSettings();)
        {
            ViewFixture host(QSize(300, 200), false);
            QG_DlgHatch dialog(nullptr, host.viewport(), nullptr, true);
            auto* preview = dialog.findChild<QG_GraphicView*>("gvPreview");
            REQUIRE(preview != nullptr);
            const QList<QG_ScrollBar*> bars = preview->findChildren<QG_ScrollBar*>();
            CHECK(bars.size() == 2);
            for (QG_ScrollBar* bar : bars) {
                CHECK(bar->isContentBandEnabled() == enabled);
                CHECK(bar->providedToolTip().isEmpty());
            }
        }
    }
}

TEST_CASE("Fresh profile: the options dialog shows Scrollbars checked and the band box enabled",
          "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    AppearanceSettingGuard scrollBarsGuard("ScrollBars");
    AppearanceSettingGuard bandGuard("ScrollBarContentBand");
    // a fresh profile has neither key, and nothing read from them is cached: whatever an
    // earlier test read or wrote, the dialog falls back to its own defaults
    forgetAppearanceSetting("ScrollBars");
    forgetAppearanceSetting("ScrollBarContentBand");
    {
        QG_DlgOptionsGeneral dialog(nullptr);
        auto* scrollbarsBox = dialog.findChild<QCheckBox*>("scrollbars_check_box");
        auto* bandBox = dialog.findChild<QCheckBox*>("cbScrollBarContentBand");
        REQUIRE(scrollbarsBox != nullptr);
        REQUIRE(bandBox != nullptr);
        CHECK(bandBox->text() == "Show drawing extents on scrollbars");
        CHECK(scrollbarsBox->isChecked());
        CHECK(bandBox->isChecked() == QG_ScrollBar::kContentBandDefault);
        CHECK(bandBox->isEnabled());
        scrollbarsBox->setChecked(false);
        CHECK(!bandBox->isEnabled());
        scrollbarsBox->setChecked(true);
        CHECK(bandBox->isEnabled());
    }
    // the forgetting itself: a cached opposite value is gone, the reader's default is back
    LC_SET_ONE("Appearance", "ScrollBars", false);
    forgetAppearanceSetting("ScrollBars");
    CHECK(LC_GET_ONE_BOOL("Appearance", "ScrollBars", true));
    forgetAppearanceSetting("ScrollBars");
}

TEST_CASE("RTL UI: the band stays left to right", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    useFusion(f);
    panFromExtents(f, 6.0, 0.0);
    const QRect ltr = band(f.hBar)->contentBandRect();
    const QImage ltrImage = grabBar(f.hBar);
    REQUIRE(band(f.hBar)->isContentBandPainted());
    f.view->setLayoutDirection(Qt::RightToLeft);
    pump();
    CHECK(f.hBar->layoutDirection() == Qt::LeftToRight);
    CHECK(band(f.hBar)->contentBandRect() == ltr);
    CHECK(grabBar(f.hBar) == ltrImage);
    // the drawing is left of the view: so is the stripe
    CHECK(ltr.right() < thumbRect(f.hBar).left());
    f.view->setLayoutDirection(Qt::LeftToRight);
}

TEST_CASE("An empty drawing or a disabled band paints exactly the plain scrollbar", "[navigation][2945-band]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture empty(QSize(800, 600), true, nullptr);
    useFusion(empty);
    empty.viewport()->zoomAuto(false, true);
    pump();
    for (QScrollBar* bar : {empty.hBar, empty.vBar}) {
        CHECK(band(bar)->contentBandRect().isNull());
        const auto [off, on] = grabOffOn(bar);
        CHECK(off == on);
    }
    // a disabled band is a plain QScrollBar, in a state where the band would be painted
    ViewFixture f;
    useFusion(f);
    panFromExtents(f, 3.0, 0.0);
    QScrollBar plain(Qt::Horizontal, f.view); // same window: same active state and palette
    plain.setStyle(f.hBar->style());
    plain.setLayoutDirection(Qt::LeftToRight);
    plain.resize(f.hBar->size());
    plain.setRange(f.hBar->minimum(), f.hBar->maximum());
    plain.setPageStep(f.hBar->pageStep());
    plain.setSingleStep(f.hBar->singleStep());
    plain.setValue(f.hBar->value());
    plain.setPalette(f.hBar->palette());
    plain.move(0, 0);
    plain.show();
    pump();
    band(f.hBar)->setContentBandEnabled(false);
    CHECK(grabBar(f.hBar) == grabBar(&plain));
    band(f.hBar)->setContentBandEnabled(true);
    CHECK(grabBar(f.hBar) != grabBar(&plain));
}
