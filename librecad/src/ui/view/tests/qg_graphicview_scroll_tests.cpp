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

// Issue #2945: the drawing view's scrollbars show where the view is in the drawing,
// move the view with the thumb, and a wheel over a bar never zooms.

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QWidget>

#include "lc_navtest_support.h"
#include "lc_viewmath.h"
#include "rs_math.h"

using namespace lc::navtest;
using Catch::Approx;

namespace {
    /// view pixels per thumb pixel, i.e. how far the view moves when the thumb moves one pixel
    double pixelsPerThumbPixel(const QScrollBar* bar) {
        return static_cast<double>(bar->maximum() - bar->minimum()) / std::max(1, thumbTravel(bar));
    }

    /// the view start (scroll space) of the axis of \p bar
    int viewStart(const ViewFixture& f, const QScrollBar* bar) {
        return bar->orientation() == Qt::Horizontal ? -f.ox() : f.oy() - f.viewport()->getHeight();
    }
}

TEST_CASE("Both scrollbar thumbs shrink on zoom in and grow on zoom out", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    REQUIRE(f.hBar != nullptr);
    REQUIRE(f.vBar != nullptr);
    f.viewport()->zoomAuto(false, true);
    const RS_Vector centre(50, 25);

    int hPrevious = thumbLength(f.hBar);
    int vPrevious = thumbLength(f.vBar);
    for (int i = 0; i < 3; ++i) {
        f.viewport()->zoomIn(2.0, centre);
        const int h = thumbLength(f.hBar);
        const int v = thumbLength(f.vBar);
        CHECK(h < hPrevious);
        CHECK(v < vPrevious);
        hPrevious = h;
        vPrevious = v;
    }
    // back out to zoom extents (i == 2) the thumbs grow; zoomed out further, the view covers
    // the drawing and the region is the view +/- half a view: both thumbs stay at half the track
    for (int i = 0; i < 5; ++i) {
        f.viewport()->zoomOut(2.0, centre);
        const int h = thumbLength(f.hBar);
        const int v = thumbLength(f.vBar);
        INFO("zoom out step " << i);
        if (i < 3) {
            CHECK(h > hPrevious);
            CHECK(v > vPrevious);
        } else {
            CHECK(h == hPrevious);
            CHECK(v == vPrevious);
            CHECK(std::abs(2 * h - (h + thumbTravel(f.hBar))) <= 2);
            CHECK(std::abs(2 * v - (v + thumbTravel(f.vBar))) <= 2);
        }
        hPrevious = h;
        vPrevious = v;
    }
}

TEST_CASE("The thumb fraction is view / (drawing + view) and its position is the view centre",
          "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(4.0, RS_Vector(50, 25));
    const double fx = f.viewport()->getFactor().x;
    const int width = f.viewport()->getWidth();
    const double drawing = 100.0 * fx;

    // pageStep / (maximum + pageStep) == L / (E + L)
    CHECK(f.hBar->pageStep() == width);
    CHECK(std::abs(f.hBar->maximum() - drawing) <= 2.0);
    // value / maximum == (centre - cMin) / E
    const double centreX = f.viewport()->toUcsX(width / 2) * fx;
    CHECK(std::abs(f.hBar->value() - centreX) <= 1.0);
}

TEST_CASE("The vertical scrollbar range is applied and tracks the view", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(3.0, RS_Vector(50, 25));
    // not Qt's default 0..99 and not stuck
    CHECK(f.vBar->minimum() == 0);
    CHECK(f.vBar->maximum() > 99);
    CHECK(f.vBar->pageStep() == f.viewport()->getHeight());
    const int value0 = f.vBar->value();
    CHECK(value0 > 0);
    CHECK(value0 < f.vBar->maximum());

    // panning the drawing up (the view moves down) increases the value by the same amount
    const int oy0 = f.oy();
    f.viewport()->zoomPan(0, -30);
    REQUIRE(f.oy() == oy0 + 30);
    CHECK(f.vBar->value() == value0 + 30);

    // a pan above the drawing: the region grows to hold the view and half a view above it
    f.viewport()->zoomPan(0, 5 * f.viewport()->getHeight());
    CHECK(std::abs(f.vBar->value() - f.vBar->pageStep() / 2) <= 1);
    CHECK(f.vBar->maximum() > 0);
}

TEST_CASE("A thumb drag moves the view the same way for one or many move events", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Qt::Orientation orientation : {Qt::Horizontal, Qt::Vertical}) {
        for (const int direction : {1, -1}) {
            ViewFixture f;
            auto bar = [&f, orientation] {
                return orientation == Qt::Horizontal ? f.hBar : f.vBar;
            };
            auto reset = [&f] {
                f.viewport()->zoomAuto(false, true);
                f.viewport()->zoomIn(2.0, RS_Vector(50, 25));
            };

            reset();
            const int start = viewStart(f, bar());
            const int value0 = bar()->value();
            const int thumb0 = thumbStart(bar());
            const double scale = pixelsPerThumbPixel(bar());
            // checked while the thumb is held: the release resyncs, and when the view has
            // left the drawing the region then grows with it (the thumb re-lays out)
            int thumbMoved = 0;
            int valueMoved = 0;
            int viewMoved = 0;
            dragThumb(bar(), 1, 40 * direction, [&](int) {
                thumbMoved = thumbStart(bar()) - thumb0;
                valueMoved = bar()->value() - value0;
                viewMoved = viewStart(f, bar()) - start;
            });
            const int oneEvent = viewStart(f, bar()) - start;
            CHECK(viewMoved == oneEvent);
            CHECK(thumbMoved == 40 * direction);
            // the bar value is the view start: one tick is one view pixel
            CHECK(valueMoved == oneEvent);

            reset();
            REQUIRE(viewStart(f, bar()) == start);
            dragThumb(bar(), 20, 2 * direction);
            const int manyEvents = viewStart(f, bar()) - start;

            INFO("orientation " << orientation << " direction " << direction << " one event " << oneEvent
                 << " twenty events " << manyEvents << " view px per thumb px " << scale);
            CHECK(std::abs(oneEvent - manyEvents) <= 1);
            // 40 thumb pixels, within one thumb pixel of quantisation
            CHECK(std::abs(oneEvent - 40 * direction * scale) <= scale + 1);
        }
    }
}

TEST_CASE("The scrollbar range is frozen while the thumb is held and resynced on release", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    const int width = f.viewport()->getWidth();
    const int maximumNear = f.hBar->maximum();

    // pan ten screens to the right: the region grows to hold the view and half a view beyond
    f.viewport()->zoomPan(-10 * width, 0);
    const int maximumFar = f.hBar->maximum();
    REQUIRE(maximumFar > maximumNear);
    REQUIRE(std::abs(maximumFar - f.hBar->value() - f.hBar->pageStep() / 2) <= 1);

    // drag the thumb back to the left end
    bool frozen = true;
    dragThumb(f.hBar, 10, -60, [&](int) {
        frozen = frozen && f.hBar->maximum() == maximumFar && f.hBar->isSliderDown();
    });
    CHECK(frozen);
    CHECK(!f.hBar->isSliderDown());

    // the release resynced the bar: the region shrank back; the sync did not move the view
    const int ox = f.ox();
    CHECK(f.hBar->maximum() < maximumFar);
    const int value = f.hBar->value();
    const int maximum = f.hBar->maximum();
    f.view->adjustOffsetControls();
    CHECK(f.ox() == ox);
    CHECK(f.hBar->value() == value);
    CHECK(f.hBar->maximum() == maximum);
}

TEST_CASE("A trough click pages one view and moves the thumb", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(8.0, RS_Vector(0, 0));
    f.hBar->setValue(0);
    const int width = f.viewport()->getWidth();
    REQUIRE(f.hBar->maximum() > 2 * width);

    const QRect thumb = thumbRect(f.hBar);
    const QRect groove = grooveRect(f.hBar);
    const QPoint click((thumb.right() + groove.right()) / 2, thumb.center().y());
    const int ox0 = f.ox();
    sendMouse(f.hBar, QEvent::MouseButtonPress, click, Qt::LeftButton, Qt::LeftButton);
    sendMouse(f.hBar, QEvent::MouseButtonRelease, click, Qt::LeftButton, Qt::NoButton);
    CHECK(ox0 - f.ox() == width);
    CHECK(thumbStart(f.hBar) > thumb.x());
}

TEST_CASE("A wheel over either scrollbar is accepted at every position and never zooms", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    for (QScrollBar* bar : {f.hBar, f.vBar}) {
        for (const int position : {0, 1, 2}) {
            for (const int delta : {120, -120}) {
                for (const Qt::KeyboardModifiers modifiers : {Qt::KeyboardModifiers(Qt::NoModifier),
                                                              Qt::KeyboardModifiers(Qt::ControlModifier),
                                                              Qt::KeyboardModifiers(Qt::ShiftModifier)}) {
                    bar->setValue(position * bar->maximum() / 2);
                    const double factor = f.factor();
                    INFO("bar " << bar->orientation() << " position " << position << " delta " << delta
                         << " modifiers " << modifiers.toInt());
                    CHECK(sendWheel(bar, QPoint(0, delta), modifiers));
                    CHECK(f.factor() == factor);
                }
            }
        }
    }
}

TEST_CASE("A wheel notch over a scrollbar scrolls three lines", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(4.0, RS_Vector(50, 25));
    const int value = f.vBar->value();
    const int oy = f.oy();
    CHECK(sendWheel(f.vBar, QPoint(0, -120)));
    CHECK(f.vBar->value() == value + 3 * f.vBar->singleStep());
    CHECK(f.oy() == oy + 3 * f.vBar->singleStep());
}

TEST_CASE("Modifier-wheel and trackpad pans over the drawing are not bounded by the scrollbars",
          "[navigation][2945][2288]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    auto wheel = [&f](const QPoint pixel, const QPoint angle, const Qt::KeyboardModifiers modifiers) {
        const QPoint c(f.view->getWidth() / 2, f.view->getHeight() / 2);
        QWheelEvent event(QPointF(c), QPointF(f.view->mapToGlobal(c)), pixel, angle, Qt::NoButton, modifiers,
                          pixel.isNull() ? Qt::NoScrollPhase : Qt::ScrollUpdate, false);
        QApplication::sendEvent(f.view, &event);
    };
    const int notches = 40;

    f.viewport()->zoomAuto(false, true);
    int ox = f.ox();
    for (int i = 0; i < notches; ++i) {
        wheel(QPoint(), QPoint(0, -120), Qt::ShiftModifier);
    }
    CHECK(std::abs(f.ox() - ox) == notches * 120);
    // the bars follow: the region grew to hold the view, with half a view of room beyond it
    const int room = std::min(f.hBar->value(), f.hBar->maximum() - f.hBar->value());
    CHECK(std::abs(room - f.hBar->pageStep() / 2) <= 1);

    f.viewport()->zoomAuto(false, true);
    int oy = f.oy();
    for (int i = 0; i < notches; ++i) {
        wheel(QPoint(), QPoint(0, -120), Qt::ControlModifier);
    }
    CHECK(std::abs(f.oy() - oy) == notches * 120);

    f.viewport()->zoomAuto(false, true);
    f.view->setDeviceName("Trackpad");
    ox = f.ox();
    oy = f.oy();
    const double factor = f.factor();
    for (int i = 0; i < notches; ++i) {
        wheel(QPoint(-40, -40), QPoint(-120, -120), Qt::NoModifier);
    }
    f.view->setDeviceName("Mouse");
    CHECK(std::abs(f.ox() - ox) == notches * 40);
    CHECK(std::abs(f.oy() - oy) == notches * 40);
    CHECK(f.factor() == factor);
}

TEST_CASE("A scrollbar sync never moves the view", "[navigation][2169]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture withBars;
    const QSize viewportSize(withBars.viewport()->getWidth(), withBars.viewport()->getHeight());
    ViewFixture withoutBars(viewportSize, false);
    REQUIRE(withoutBars.viewport()->getWidth() == viewportSize.width());
    REQUIRE(withoutBars.viewport()->getHeight() == viewportSize.height());

    auto same = [&] {
        return withBars.ox() == withoutBars.ox() && withBars.oy() == withoutBars.oy()
            && withBars.factor() == withoutBars.factor();
    };
    for (ViewFixture* f : {&withBars, &withoutBars}) {
        f->viewport()->zoomAuto(false, true);
    }
    REQUIRE(same());
    for (ViewFixture* f : {&withBars, &withoutBars}) {
        f->viewport()->zoomPan(-10 * viewportSize.width(), 3 * viewportSize.height());
    }
    CHECK(same());
    for (ViewFixture* f : {&withBars, &withoutBars}) {
        f->viewport()->zoomOut(5.0, f->viewport()->toWorldFromUi(120, 80));
    }
    CHECK(same());
    for (ViewFixture* f : {&withBars, &withoutBars}) {
        f->viewport()->zoomIn(3.0, f->viewport()->toWorldFromUi(500, 300));
    }
    CHECK(same());
    const int ox = withBars.ox();
    const int oy = withBars.oy();
    withBars.view->adjustOffsetControls();
    CHECK(withBars.ox() == ox);
    CHECK(withBars.oy() == oy);
}

TEST_CASE("Each scrollbar step notifies the viewport listeners once", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(4.0, RS_Vector(50, 25));
    CountingListener counter;
    f.viewport()->addViewportListener(&counter);

    bool once = true;
    dragThumb(f.hBar, 5, 3, [&](int) {
        once = once && counter.changed == 1;
        counter.changed = 0;
    });
    CHECK(once);

    counter.changed = 0;
    sendWheel(f.vBar, QPoint(0, -120));
    CHECK(counter.changed == 1);

    counter.changed = 0;
    f.viewport()->zoomPan(10, 10);
    CHECK(counter.changed == 1);
    f.viewport()->removeViewportListener(&counter);
}

TEST_CASE("A squashed view keeps its scrollbars usable", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    // a child view (as in an MDI window) can be squashed below the scrollbar size
    QWidget window;
    auto* f = new ViewFixture(QSize(800, 600), true, addDefaultDrawing, &window);
    f->viewport()->zoomAuto(false, true);
    const int maximum = f->vBar->maximum();
    f->view->resize(10, 10);
    pump();
    CHECK(f->view->getHeight() <= 0);
    // no sync with a non-positive size: the range is not replaced by a degenerate one
    CHECK(f->vBar->maximum() == maximum);
    f->view->resize(800, 600);
    pump();
    f->viewport()->zoomAuto(false, true);
    CHECK(f->vBar->maximum() > 0);
    CHECK(f->hBar->maximum() > 0);
    CHECK(f->vBar->pageStep() == f->viewport()->getHeight());
    CHECK(f->hBar->pageStep() == f->viewport()->getWidth());
    delete f;
}

TEST_CASE("With a rotated UCS the thumbs reach every drawing corner", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f(QSize(800, 600), true, [](RS_Graphic* g) {
        addRectangle(g, RS_Vector(0, 0), RS_Vector(100, 10));
    });
    f.viewport()->createUCS(RS_Vector(0, 0), RS_Math::deg2rad(30.0));
    REQUIRE(f.viewport()->hasUCS());
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(6.0, RS_Vector(50, 5));
    const int width = f.viewport()->getWidth();
    const int height = f.viewport()->getHeight();

    for (const RS_Vector& corner : {RS_Vector(0, 0), RS_Vector(100, 0), RS_Vector(100, 10), RS_Vector(0, 10)}) {
        const RS_Vector ucs = f.viewport()->toUCS(corner);
        bool xReached = false;
        for (int value = 0; value <= f.hBar->maximum() && !xReached; value += std::max(1, f.hBar->pageStep() / 2)) {
            f.hBar->setValue(value);
            const double gx = f.viewport()->toGuiX(ucs.x);
            xReached = gx >= 0 && gx <= width;
        }
        f.hBar->setValue(f.hBar->maximum());
        xReached = xReached || (f.viewport()->toGuiX(ucs.x) >= 0 && f.viewport()->toGuiX(ucs.x) <= width);
        bool yReached = false;
        for (int value = 0; value <= f.vBar->maximum() && !yReached; value += std::max(1, f.vBar->pageStep() / 2)) {
            f.vBar->setValue(value);
            const double gy = f.viewport()->toGuiY(ucs.y);
            yReached = gy >= 0 && gy <= height;
        }
        f.vBar->setValue(f.vBar->maximum());
        yReached = yReached || (f.viewport()->toGuiY(ucs.y) >= 0 && f.viewport()->toGuiY(ucs.y) <= height);
        INFO("corner " << corner.x << "," << corner.y);
        CHECK(xReached);
        CHECK(yReached);
    }
}

// --- v2 additions (not in the c8d980f13 prototype) --------------------------------------

TEST_CASE("A silent viewport change is rebased before the next trough click pages one view",
          "[navigation][2945]") {
    // Some paths change the viewport without notifying (the DXF VPORT restore through
    // justSetOffsetAndFactor, applyUCSAfterLoad, a keyboard zoom while a thumb is held).
    // The next bar slot call must rebase on the *current* view before mapping, not page
    // from the value it was last synced to.
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f;
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(8.0, RS_Vector(0, 0));
    f.hBar->setValue(0);
    const int width = f.viewport()->getWidth();
    REQUIRE(f.hBar->maximum() > 2 * width);

    const int oxBefore = f.ox();
    const double factor = f.viewport()->getFactor().x;
    f.viewport()->justSetOffsetAndFactor(oxBefore - width / 2, f.oy(), factor);
    REQUIRE(f.ox() == oxBefore - width / 2);
    const int oxSilent = f.ox();

    const QRect thumb = thumbRect(f.hBar);
    const QRect groove = grooveRect(f.hBar);
    const QPoint click((thumb.right() + groove.right()) / 2, thumb.center().y());
    sendMouse(f.hBar, QEvent::MouseButtonPress, click, Qt::LeftButton, Qt::LeftButton);
    sendMouse(f.hBar, QEvent::MouseButtonRelease, click, Qt::LeftButton, Qt::NoButton);
    // exactly one view paged from the current (post-silent-change) view, not the stale one
    CHECK(oxSilent - f.ox() == width);
}

TEST_CASE("A silent FACTOR change while a thumb is NOT held is picked up correctly", "[navigation][2945]") {
    // The test above only ever varies the offset via justSetOffsetAndFactor(), reusing the
    // SAME factor -- that gap is exactly what let this bug through: rebaseScrollIfStale()
    // used to patch only m_hScroll/m_vScroll's origin using the OLD pixelsPerTick, never
    // refreshing the scale itself. pixelsPerTick is clamped to a minimum of 1 view pixel
    // per tick (LC_ScrollModel::kMaxTicks), so an ordinary small drawing never actually
    // exercises a scale change -- this needs survey-scale content (as in "Survey-scale
    // extents..." above) for a factor change to move pixelsPerTick perceptibly.
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);

    ViewFixture f(QSize(800, 600), true, [](RS_Graphic* g) {
        addRectangle(g, RS_Vector(0, 0), RS_Vector(5.0e6, 5.0e6));
    });
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(400.0 / f.factor(), RS_Vector(0, 0));
    REQUIRE(std::abs(f.factor() - 400.0) < 1e-6);
    const int v0 = f.hBar->value();
    const int ox0 = f.ox();
    const int oy0 = f.oy();

    // doubling the factor roughly doubles the scrollable span (content size * factor),
    // and hence roughly doubles pixelsPerTick too, since span is already well past
    // kMaxTicks here (matching "Survey-scale extents..." above: 5e6 * 400 ~= 2e9).
    const double newFactor = f.factor() * 2.0;
    // silent: justSetOffsetAndFactor() does not notify listeners, so m_hScroll/m_scrollKey
    // still reflect the OLD factor (and hence the OLD pixels-per-tick) when the bar next
    // fires valueChanged; the Qt widget's own value is untouched by it too.
    f.viewport()->justSetOffsetAndFactor(ox0, oy0, newFactor);
    REQUIRE(f.viewport()->getFactor().x == newFactor);
    REQUIRE(f.hBar->value() == v0);

    // no view jump: rebasing at the bar's own (unchanged) value must reproduce the
    // current (silently changed) view exactly, not just approximately.
    GraphicViewAccess::triggerSlotHScrolled(f.view, v0);
    CHECK(f.ox() == ox0);

    // a few ticks away from there must move the view by that many ticks' worth of the
    // FRESH pixels-per-tick, not the stale (roughly half, here) one from before the
    // factor change -- patching only the origin cannot get this right for any value
    // other than the exact one the origin was re-centred at.
    constexpr int delta = 5;
    GraphicViewAccess::triggerSlotHScrolled(f.view, v0 + delta);
    const double movedWithFix = std::abs(f.ox() - ox0);

    // Ground truth: force an ordinary, known-correct full resync (adjustOffsetControls()
    // was never the buggy part), then probe by the SAME delta from wherever it now
    // anchors. Both probes are driven through the identical slot machinery, so they must
    // agree once the scale used by the first one is genuinely fresh, not stale. (pageStep
    // / maximum are integers -- pageStep rounds to a handful of ticks at this survey scale,
    // and maximum saturates at kMaxTicks -- so neither is precise enough to check against
    // directly; a second same-size probe measures pixels-per-tick exactly instead.)
    //
    // Both ends of that probe must come from the SAME value-to-pixel mapping: the bar's
    // own value is an integer tick, rounded from the exact sub-tick position, so anchoring
    // on the true (exact) current f.ox() instead of on ox(vFresh) would mix a sub-tick
    // remainder into the comparison -- at this survey scale, up to half of pixelsPerTick
    // (tens to low hundreds of pixels), swamping the 2px tolerance below.
    f.view->adjustOffsetControls();
    const int vFresh = f.hBar->value();
    GraphicViewAccess::triggerSlotHScrolled(f.view, vFresh); // value-derived reference point
    const int oxFreshAnchor = f.ox();
    GraphicViewAccess::triggerSlotHScrolled(f.view, vFresh + delta);
    const double movedFresh = std::abs(f.ox() - oxFreshAnchor);
    REQUIRE(movedFresh > 2.0 * delta); // otherwise this scenario would not exercise the bug

    CHECK(movedWithFix == Approx(movedFresh).margin(2.0));
}

TEST_CASE("Bar input before the first valid sync never moves the view", "[navigation][2945]") {
    // Without scrollbars, adjustOffsetControls() has never run: the snapshot is still the
    // default-constructed, invalid one.
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f(QSize(800, 600), false);
    REQUIRE(!f.view->hasScrollbars());
    const int ox = f.ox();
    const int oy = f.oy();
    const double factor = f.factor();
    GraphicViewAccess::triggerSlotHScrolled(f.view, 12345);
    GraphicViewAccess::triggerSlotVScrolled(f.view, -6789);
    CHECK(f.ox() == ox);
    CHECK(f.oy() == oy);
    CHECK(f.factor() == factor);
}

namespace {
    /// Counts redraw() calls that include RedrawDrawing, to distinguish a full resync
    /// (setOffsetAndFactor) from an overlay-only one (justSetOffsetAndFactor, which never
    /// notifies, so redraw() is never reached from the restore at all).
    struct RedrawCountingView : QG_GraphicView {
        using QG_GraphicView::QG_GraphicView;
        int drawingRedraws = 0;
        void redraw(const RS2::RedrawMethod method = RS2::RedrawAll, const bool immediately = false) override {
            if ((method & RS2::RedrawDrawing) != 0) {
                ++drawingRedraws;
            }
            QG_GraphicView::redraw(method, immediately);
        }
    };
}

TEST_CASE("After the UCS-highlight restore the bars resync and the drawing layer is re-rendered",
          "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    RS_Graphic* graphic = new RS_Graphic();
    graphic->initForNewDocument();
    addDefaultDrawing(graphic);
    graphic->calculateBorders();
    LC_ActionContext context;
    auto* view = new RedrawCountingView(nullptr, graphic, &context);
    view->initView();
    view->loadSettings();
    view->addScrollbars();
    view->move(4000, 4000);
    view->resize(800, 600);
    view->show();
    pump();
    QScrollBar* hBar = nullptr;
    QScrollBar* vBar = nullptr;
    for (auto* bar : view->findChildren<QScrollBar*>()) {
        (bar->orientation() == Qt::Horizontal ? hBar : vBar) = bar;
    }
    REQUIRE(hBar != nullptr);
    REQUIRE(vBar != nullptr);
    auto* viewport = view->getViewPort();

    viewport->zoomAuto(false, true);
    viewport->createUCS(RS_Vector(500, 500), 0.0); // off-screen, so highlighting must pan to it
    LC_UCS* ucs = viewport->getCurrentUCS();
    REQUIRE(ucs != nullptr);

    const int oxBefore = viewport->getOffsetX();
    const int oyBefore = viewport->getOffsetY();
    const double factorBefore = viewport->getFactor().x;

    view->highlightUCSLocation(ucs);
    // the highlight zoomed to include the (off-screen) UCS origin: the view moved
    CHECK((viewport->getOffsetX() != oxBefore || viewport->getFactor().x != factorBefore));

    view->drawingRedraws = 0;
    // mayTick() returns false once blinkNumber exceeds maxBlinkNumber (a settings value,
    // loaded above by loadSettings()): that call restores. Loop generously rather than
    // hard-coding that count.
    bool restored = false;
    for (int i = 0; i < 200 && !restored; ++i) {
        GraphicViewAccess::triggerUcsHighlightStep(view);
        restored = viewport->getOffsetX() == oxBefore && viewport->getOffsetY() == oyBefore;
    }
    REQUIRE(restored);

    CHECK(viewport->getOffsetX() == oxBefore);
    CHECK(viewport->getOffsetY() == oyBefore);
    CHECK(viewport->getFactor().x == factorBefore);
    // setOffsetAndFactor (not justSetOffsetAndFactor) notifies: RedrawAll reached the
    // drawing layer, not only the overlay that ucsHighlightStep's blink always redraws
    CHECK(view->drawingRedraws > 0);
    // the automatic resync already matches a fresh one bit for bit: the bars are not stale
    const int hValue = hBar->value();
    const int vValue = vBar->value();
    const int hMaximum = hBar->maximum();
    const int vMaximum = vBar->maximum();
    view->adjustOffsetControls();
    CHECK(hBar->value() == hValue);
    CHECK(vBar->value() == vValue);
    CHECK(hBar->maximum() == hMaximum);
    CHECK(vBar->maximum() == vMaximum);

    view->hide();
    delete view;
    graphic->setGraphicView(nullptr);
    delete graphic;
}

TEST_CASE("Survey-scale extents (5e6 m at f = 400) drag without clamping", "[navigation][2945]") {
    // v1 and the prototype used kMaxViewPixel = 2^30 (1073741824), which would clamp a
    // content span of 5e6 * 400 = 2e9 px; v2's 2^31 - 2^24 (2130706432) does not.
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    ViewFixture f(QSize(800, 600), true, [](RS_Graphic* g) {
        addRectangle(g, RS_Vector(0, 0), RS_Vector(5.0e6, 5.0e6));
    });
    f.viewport()->zoomAuto(false, true);
    f.viewport()->zoomIn(400.0 / f.factor(), RS_Vector(0, 0));
    REQUIRE(std::abs(f.factor() - 400.0) < 1e-6);
    REQUIRE(f.hBar->maximum() > 0);

    // drag the thumb to the far end of the (survey-scale) drawing
    bool atEnd = false;
    dragThumb(f.hBar, 1, thumbTravel(f.hBar), [&](int) {
        atEnd = f.hBar->value() == f.hBar->maximum();
    });
    CHECK(atEnd);
    // the release resync keeps half a view of room beyond the view
    CHECK(std::abs(f.hBar->maximum() - f.hBar->value() - f.hBar->pageStep() / 2) <= 1);
    CHECK(static_cast<double>(-f.ox()) > 1073741824.0); // > the old (v1) kMaxViewPixel
    CHECK(static_cast<double>(-f.ox()) < LC_ViewMath::kMaxViewPixel);
}

// ------------------------------------------------------------------------------------------

TEST_CASE("Drawing view scrollbars stay left to right in a right-to-left UI", "[navigation][2945]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    QWidget window;
    window.setLayoutDirection(Qt::RightToLeft);
    auto* f = new ViewFixture(QSize(800, 600), true, addDefaultDrawing, &window);
    CHECK(f->view->layoutDirection() == Qt::RightToLeft);
    CHECK(f->hBar->layoutDirection() == Qt::LeftToRight);
    CHECK(f->vBar->layoutDirection() == Qt::LeftToRight);

    f->viewport()->zoomAuto(false, true);
    f->viewport()->zoomIn(2.0, RS_Vector(50, 25));
    const int ox = f->ox();
    dragThumb(f->hBar, 1, 40);
    // dragging right moves the view right
    CHECK(f->ox() < ox);

    // the drawing is painted from x = 0 to getWidth(): the vertical bar must not cover it
    INFO("vertical bar at x " << f->vBar->geometry().x() << ", drawing width " << f->view->getWidth());
    CHECK(f->vBar->geometry().x() >= f->view->getWidth());
    delete f;
}
