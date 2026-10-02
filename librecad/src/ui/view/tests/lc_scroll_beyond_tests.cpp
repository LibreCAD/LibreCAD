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

// Issue #2945 review (sand1024): while a command waits for its next point, the user
// must be able to take the view as far past the drawing's extents as they like with
// the mouse alone, and place the point there. Every test here starts Draw Line, places
// its first point with a real left click, navigates with real mouse input only, and
// then clicks the second point in the view's centre.
//
// Set LC_NAVTEST_REPORT=1 to print how far each method got.

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <QElapsedTimer>

#include "lc_action_draw_line.h"
#include "lc_navtest_support.h"
#include "lc_scrollmodel.h"
#include "qg_scrollbar.h"
#include "rs_actionzoompan.h"
#include "rs_settings.h"

using namespace lc::navtest;

namespace {
    /// Starts the actions the view asks for by type; the middle-button pan is the only one needed here.
    struct NavActionContext : LC_ActionContext {
        void setCurrentAction(const RS2::ActionType actionType, [[maybe_unused]] void* data) override {
            if (actionType == RS2::ActionZoomPan && m_graphicView != nullptr) {
                m_graphicView->setCurrentAction(std::make_shared<RS_ActionZoomPan>(this));
            }
        }
    };

    using NavFixture = BasicViewFixture<NavActionContext>;

    /// the default drawing: the 100 x 50 rectangle at the origin
    constexpr double kDrawingMaxX = 100.0;
    constexpr double kDrawingMaxY = 50.0;
    /// how far every method must take the view (in view lengths past the drawing's edge)
    constexpr double kFar = 10.0;

    enum class Direction { Right, Left, Down, Up };

    const char* name(const Direction direction) {
        switch (direction) {
            case Direction::Right:
                return "right";
            case Direction::Left:
                return "left";
            case Direction::Down:
                return "down";
            default:
                return "up";
        }
    }

    bool isHorizontal(const Direction direction) {
        return direction == Direction::Right || direction == Direction::Left;
    }

    /// +1 when moving in \p direction increases the bar's value
    int barSign(const Direction direction) {
        return direction == Direction::Right || direction == Direction::Down ? 1 : -1;
    }

    /// Settings read by the canvas's mouse handling, fixed for the test and restored after it.
    struct ScopedSetting {
        QString m_group;
        QString m_key;
        bool m_previous;
        ScopedSetting(const QString& group, const QString& key, const bool value)
            : m_group(group), m_key(key), m_previous(LC_GET_ONE_BOOL(group, key, false)) {
            LC_SET_ONE(m_group, m_key, value);
        }
        ~ScopedSetting() {
            LC_SET_ONE(m_group, m_key, m_previous);
        }
        ScopedSetting(const ScopedSetting&) = delete;
        ScopedSetting& operator=(const ScopedSetting&) = delete;
    };

    /**
     * Draw Line with its first point placed by a left click at the view's centre, the
     * view at Zoom Extents on the default 100 x 50 rectangle.
     */
    struct LineScenario {
        ScopedSetting m_noAutoPan{"Appearance", "Autopanning", false};
        NavFixture m_fixture;
        std::shared_ptr<LC_ActionDrawLine> m_line;
        RS_Vector m_point1;
        /// the drawing's extents; in an empty drawing, the first point (the only thing placed yet)
        RS_Vector m_min{0.0, 0.0};
        RS_Vector m_max{kDrawingMaxX, kDrawingMaxY};

        /// \p empty: a new drawing with no entities (the view as the fixture opens it)
        explicit LineScenario(const bool empty = false)
            : m_fixture(QSize(800, 600), true, empty ? std::function<void(RS_Graphic*)>{} : addDefaultDrawing) {
            if (!empty) {
                m_fixture.viewport()->zoomAuto(false, true);
                pump();
            }
            m_line = std::make_shared<LC_ActionDrawLine>(&m_fixture.context);
            m_fixture.view->setCurrentAction(m_line);
            m_point1 = click(centre());
            if (empty) {
                m_min = m_max = m_point1;
            }
        }

        bool isEmpty() const { return m_fixture.graphic->count() == 0; }

        ~LineScenario() {
            // no RS_ActionDefault resume (it would create the QC_ApplicationWindow singleton)
            m_fixture.view->beginClose();
        }

        LineScenario(const LineScenario&) = delete;
        LineScenario& operator=(const LineScenario&) = delete;

        int width() const { return m_fixture.viewport()->getWidth(); }
        int height() const { return m_fixture.viewport()->getHeight(); }
        QPoint centre() const { return {width() / 2, height() / 2}; }

        /// a real left click on the canvas; returns the drawing point under it
        RS_Vector click(const QPoint pos) {
            const RS_Vector world = m_fixture.viewport()->toWorldFromUi(pos.x(), pos.y());
            sendMouse(m_fixture.view, QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton);
            sendMouse(m_fixture.view, QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton);
            sendMouse(m_fixture.view, QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton);
            pump();
            return world;
        }

        /// the command still has its first point and waits for the second
        bool waitingForEndpoint() const {
            return m_fixture.view->getCurrentAction() == m_line.get()
                && m_line->getStatus() != RS_ActionInterface::InitialActionStatus;
        }

        /**
         * How many view lengths the view has left the drawing behind in \p direction: the
         * gap between the drawing's edge and the view's opposite edge, over the view's
         * length. Negative while the drawing is still in view (-0.5: its edge at the centre).
         */
        double beyond(const Direction direction) const {
            const auto* vp = m_fixture.viewport();
            const double left = vp->toUcsX(0);
            const double right = vp->toUcsX(width());
            const double top = vp->toUcsY(0);
            const double bottom = vp->toUcsY(height());
            switch (direction) {
                case Direction::Right:
                    return (left - m_max.x) / (right - left);
                case Direction::Left:
                    return (m_min.x - right) / (right - left);
                case Direction::Down:
                    return (m_min.y - top) / (top - bottom);
                default:
                    return (bottom - m_max.y) / (top - bottom);
            }
        }

        QScrollBar* bar(const Direction direction) const {
            return isHorizontal(direction) ? m_fixture.hBar : m_fixture.vBar;
        }
    };

    struct Outcome {
        /// the farthest the view got, in view lengths past the drawing's edge
        double m_reached = -1.0;
        /// gestures used
        int m_gestures = 0;
        bool m_pointKept = false;
        bool m_lineCreated = false;
        /// the line's start is the first click
        bool m_startIsPoint1 = false;
        /// how far the line's end lies past the drawing's edge, in view lengths at the second click
        double m_endBeyond = -1.0;
        /// with nothing more to do, the event loop left the view and the bars as they were
        bool m_settled = false;
        /// the drawing-extents stripe on the travel axis's bar still marks the drawing
        bool m_bandMarksDrawing = false;
    };

    /**
     * Whether the stripe on \p bar marks the drawing (the 100 x 50 rectangle), located from
     * the view and the bar's value alone (one view pixel per tick at these distances), not
     * from the scroll model's region. An empty drawing has nothing to mark: no stripe.
     */
    bool bandMarksDrawing(const LineScenario& s, const QScrollBar* bar) {
        const auto* scrollBar = qobject_cast<const QG_ScrollBar*>(bar);
        if (scrollBar == nullptr) {
            return false;
        }
        if (s.isEmpty()) {
            return scrollBar->contentBandRect().isNull();
        }
        const bool horizontal = bar->orientation() == Qt::Horizontal;
        const RS_Vector factor = s.m_fixture.viewport()->getFactor();
        const double viewStart = horizontal ? -s.m_fixture.ox() : s.m_fixture.oy() - s.height();
        const double origin = viewStart - bar->value();
        const double contentMin = horizontal ? s.m_min.x * factor.x : -s.m_max.y * factor.y;
        const double contentMax = horizontal ? s.m_max.x * factor.x : -s.m_min.y * factor.y;

        QStyleOptionSlider option;
        SliderAccess::initOption(bar, &option);
        option.sliderPosition = option.sliderValue = 0;
        const QRect r0 = bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, bar);
        option.sliderPosition = option.sliderValue = option.maximum;
        const QRect r1 = bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, bar);
        LC_ScrollModel::ThumbGeometry geometry;
        geometry.start0 = horizontal ? r0.left() : r0.top();
        geometry.travel = (horizontal ? r1.left() : r1.top()) - geometry.start0;
        geometry.length = horizontal ? r0.width() : r0.height();
        geometry.maximum = bar->maximum();
        geometry.pageStep = bar->pageStep();
        const LC_ScrollModel::Band expected =
            LC_ScrollModel::bandPixels(geometry, true, contentMin - origin, contentMax - origin);
        const QRect actual = scrollBar->contentBandRect();
        const int start = horizontal ? actual.left() : actual.top();
        const int end = horizontal ? actual.right() + 1 : actual.bottom() + 1;
        return expected.visible && !actual.isNull() && std::abs(start - expected.start) <= 1
            && std::abs(end - expected.end) <= 1;
    }

    /**
     * Repeats \p gesture (pumping the event loop after each) until the view is kFar view
     * lengths past the drawing or \p maxGestures ran out, then clicks the second point at
     * the view's centre.
     */
    Outcome drive(LineScenario& s, const Direction direction, const int maxGestures,
                  const std::function<void()>& gesture, const char* method) {
        Outcome outcome;
        for (int i = 0; i < maxGestures && outcome.m_reached < kFar; ++i) {
            gesture();
            pump();
            ++outcome.m_gestures;
            outcome.m_reached = std::max(outcome.m_reached, s.beyond(direction));
        }
        outcome.m_pointKept = s.waitingForEndpoint();

        // no runaway: the queued resync never moves the view, and nothing is queued again
        QScrollBar* bar = s.bar(direction);
        const int ox = s.m_fixture.ox();
        const int oy = s.m_fixture.oy();
        const int value = bar->value();
        const int maximum = bar->maximum();
        for (int i = 0; i < 10; ++i) {
            pump();
        }
        outcome.m_settled = s.m_fixture.ox() == ox && s.m_fixture.oy() == oy && bar->value() == value
            && bar->maximum() == maximum;
        outcome.m_bandMarksDrawing = bandMarksDrawing(s, bar);

        const double viewLength = isHorizontal(direction) ? s.m_fixture.viewport()->toUcsX(s.width()) - s.m_fixture.viewport()->toUcsX(0)
                                                          : s.m_fixture.viewport()->toUcsY(0) - s.m_fixture.viewport()->toUcsY(s.height());
        const unsigned countBefore = s.m_fixture.graphic->count();
        const RS_Vector point2 = s.click(s.centre());
        outcome.m_lineCreated = s.m_fixture.graphic->count() == countBefore + 1;
        if (outcome.m_lineCreated) {
            const auto* created = dynamic_cast<RS_Line*>(s.m_fixture.graphic->last());
            if (created != nullptr) {
                const double pixel = 1.0 / s.m_fixture.factor();
                outcome.m_startIsPoint1 = created->getStartpoint().distanceTo(s.m_point1) <= 1e-6 + pixel;
                const RS_Vector end = created->getEndpoint();
                switch (direction) {
                    case Direction::Right:
                        outcome.m_endBeyond = (end.x - s.m_max.x) / viewLength;
                        break;
                    case Direction::Left:
                        outcome.m_endBeyond = (s.m_min.x - end.x) / viewLength;
                        break;
                    case Direction::Down:
                        outcome.m_endBeyond = (s.m_min.y - end.y) / viewLength;
                        break;
                    case Direction::Up:
                        outcome.m_endBeyond = (end.y - s.m_max.y) / viewLength;
                        break;
                }
                outcome.m_lineCreated = created->getEndpoint().distanceTo(point2) <= 1e-6 + pixel;
            }
        }
        if (qEnvironmentVariableIsSet("LC_NAVTEST_REPORT")) {
            std::cout << "[2945-beyond] " << method << " " << name(direction) << ": reached " << outcome.m_reached
                << " view lengths past the drawing in " << outcome.m_gestures << " gestures; point 1 kept "
                << outcome.m_pointKept << "; line created " << outcome.m_lineCreated << " (start is point 1 "
                << outcome.m_startIsPoint1 << ", end " << outcome.m_endBeyond << " view lengths past the drawing)"
                << "; settled " << outcome.m_settled << "; band marks the drawing " << outcome.m_bandMarksDrawing
                << "\n";
        }
        return outcome;
    }

    void checkFar(const Outcome& outcome) {
        CHECK(outcome.m_reached >= kFar);
        CHECK(outcome.m_pointKept);
        CHECK(outcome.m_lineCreated);
        CHECK(outcome.m_startIsPoint1);
        // the end is at the view's centre, half a view past the view's near edge
        CHECK(outcome.m_endBeyond >= kFar);
        CHECK(outcome.m_settled);
        CHECK(outcome.m_bandMarksDrawing);
    }

    QPoint along(const QScrollBar* bar, const QPoint from, const int distance) {
        return bar->orientation() == Qt::Horizontal ? from + QPoint(distance, 0) : from + QPoint(0, distance);
    }

    /// presses the thumb and drags it past the end of the track in the bar's \p sign direction, then releases
    void dragThumbPastEnd(QScrollBar* bar, const int sign) {
        const QRect groove = grooveRect(bar);
        const QPoint press = thumbRect(bar).center();
        const bool horizontal = bar->orientation() == Qt::Horizontal;
        const int grooveEnd = horizontal ? (sign > 0 ? groove.right() : groove.left())
                                         : (sign > 0 ? groove.bottom() : groove.top());
        const int from = horizontal ? press.x() : press.y();
        const int distance = grooveEnd + 40 * sign - from;
        sendMouse(bar, QEvent::MouseButtonPress, press, Qt::LeftButton, Qt::LeftButton);
        constexpr int moves = 4;
        for (int k = 1; k <= moves; ++k) {
            sendMouse(bar, QEvent::MouseMove, along(bar, press, distance * k / moves), Qt::NoButton, Qt::LeftButton);
        }
        sendMouse(bar, QEvent::MouseButtonRelease, along(bar, press, distance), Qt::LeftButton, Qt::NoButton);
    }

    /// a click on the track between the thumb and the track's end; none when the thumb is at that end
    void troughClick(QScrollBar* bar, const int sign) {
        const QRect groove = grooveRect(bar);
        const QRect thumb = thumbRect(bar);
        const bool horizontal = bar->orientation() == Qt::Horizontal;
        int first;
        int last;
        if (horizontal) {
            first = sign > 0 ? thumb.right() + 1 : groove.left();
            last = sign > 0 ? groove.right() : thumb.left() - 1;
        } else {
            first = sign > 0 ? thumb.bottom() + 1 : groove.top();
            last = sign > 0 ? groove.bottom() : thumb.top() - 1;
        }
        if (last < first) {
            return; // no track left to click
        }
        const int position = (first + last) / 2;
        const QPoint point = horizontal ? QPoint(position, thumb.center().y()) : QPoint(thumb.center().x(), position);
        sendMouse(bar, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
        sendMouse(bar, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
    }

    void arrowClick(QScrollBar* bar, const int sign) {
        const QRect arrow = subControlRect(bar, sign > 0 ? QStyle::SC_ScrollBarAddLine : QStyle::SC_ScrollBarSubLine);
        REQUIRE(!arrow.isEmpty());
        sendMouse(bar, QEvent::MouseButtonPress, arrow.center(), Qt::LeftButton, Qt::LeftButton);
        sendMouse(bar, QEvent::MouseButtonRelease, arrow.center(), Qt::LeftButton, Qt::NoButton);
    }

    /// a wheel notch over the bar, towards increasing values when \p sign > 0
    void barWheel(QScrollBar* bar, const int sign) {
        sendWheel(bar, QPoint(0, -120 * sign));
    }

    /// a wheel event at \p pos over the canvas
    void canvasWheel(QWidget* view, const QPoint pos, const int angle, const Qt::KeyboardModifiers modifiers) {
        QWheelEvent event(QPointF(pos), QPointF(view->mapToGlobal(pos)), QPoint(), QPoint(0, angle), Qt::NoButton,
                          modifiers, Qt::NoScrollPhase, false);
        QApplication::sendEvent(view, &event);
    }

    /// a middle-button drag across the canvas, moving the drawing against \p direction
    void middleDrag(LineScenario& s, const Direction direction) {
        const int marginX = 80;
        const int marginY = 80;
        QPoint from;
        QPoint to;
        switch (direction) {
            case Direction::Right:
                from = {s.width() - marginX, s.height() / 2};
                to = {marginX, s.height() / 2};
                break;
            case Direction::Left:
                from = {marginX, s.height() / 2};
                to = {s.width() - marginX, s.height() / 2};
                break;
            case Direction::Down:
                from = {s.width() / 2, s.height() - marginY};
                to = {s.width() / 2, marginY};
                break;
            case Direction::Up:
                from = {s.width() / 2, marginY};
                to = {s.width() / 2, s.height() - marginY};
                break;
        }
        sendMouse(s.m_fixture.view, QEvent::MouseButtonPress, from, Qt::MiddleButton, Qt::MiddleButton);
        constexpr int moves = 5;
        for (int k = 1; k <= moves; ++k) {
            sendMouse(s.m_fixture.view, QEvent::MouseMove, from + (to - from) * k / moves, Qt::NoButton, Qt::MiddleButton);
        }
        sendMouse(s.m_fixture.view, QEvent::MouseButtonRelease, to, Qt::MiddleButton, Qt::NoButton);
    }
}

// --- the scrollbars alone ---------------------------------------------------------------

TEST_CASE("Beyond the drawing by dragging the thumb, releasing and dragging again", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
        LineScenario s;
        REQUIRE(s.waitingForEndpoint());
        QScrollBar* bar = s.bar(direction);
        const Outcome outcome = drive(s, direction, 40, [&] {
            dragThumbPastEnd(bar, barSign(direction));
        }, "thumb drag");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " drags");
        checkFar(outcome);
    }
}

TEST_CASE("Beyond the drawing by clicking the track past the thumb", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
        LineScenario s;
        QScrollBar* bar = s.bar(direction);
        const Outcome outcome = drive(s, direction, 60, [&] {
            troughClick(bar, barSign(direction));
        }, "trough click");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " clicks");
        checkFar(outcome);
    }
}

TEST_CASE("Beyond the drawing by clicking a scrollbar arrow", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
        LineScenario s;
        QScrollBar* bar = s.bar(direction);
        const Outcome outcome = drive(s, direction, 400, [&] {
            arrowClick(bar, barSign(direction));
        }, "arrow click");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " clicks");
        checkFar(outcome);
    }
}

TEST_CASE("Beyond the drawing by turning the wheel over a scrollbar", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
        LineScenario s;
        QScrollBar* bar = s.bar(direction);
        const Outcome outcome = drive(s, direction, 200, [&] {
            barWheel(bar, barSign(direction));
        }, "wheel over the bar");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " notches");
        checkFar(outcome);
    }
}

TEST_CASE("In an empty drawing the scrollbars take the view on from the first point", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    // sand1024's scenario in a new drawing: the first point is all there is yet, so the
    // bars have no extents to go by; they must still move on, gesture after gesture
    const std::pair<const char*, std::function<void(QScrollBar*, int)>> methods[] = {
        {"thumb drag", dragThumbPastEnd},
        {"trough click", troughClick},
        {"arrow click", arrowClick},
        {"wheel over the bar", barWheel},
    };
    for (const auto& method : methods) {
        for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
            LineScenario s(true);
            REQUIRE(s.isEmpty());
            REQUIRE(s.waitingForEndpoint());
            QScrollBar* bar = s.bar(direction);
            const std::function<void(QScrollBar*, int)>& gesture = method.second;
            const Outcome outcome = drive(s, direction, 400, [&] {
                gesture(bar, barSign(direction));
            }, method.first);
            INFO(method.first << " " << name(direction) << ": reached " << outcome.m_reached << " in "
                 << outcome.m_gestures << " gestures");
            checkFar(outcome);
        }
    }
}

// --- the canvas (never bounded by the bars) ---------------------------------------------

TEST_CASE("Beyond the drawing by middle-button drags on the canvas", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Left, Direction::Down, Direction::Up}) {
        LineScenario s;
        const Outcome outcome = drive(s, direction, 40, [&] {
            middleDrag(s, direction);
        }, "middle-button drag");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " drags");
        checkFar(outcome);
    }
}

TEST_CASE("Middle-button double-click zooms extents without cancelling the active action", "[navigation][2982]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    LineScenario s;
    const double extentsFactor = s.m_fixture.viewport()->getFactor().x;
    s.m_fixture.viewport()->zoomIn(4.0, RS_Vector(50.0, 25.0));
    REQUIRE(s.waitingForEndpoint());
    CHECK(s.m_fixture.viewport()->getFactor().x != extentsFactor);

    // A middle click first switches briefly to pan; its release restores Draw Line.
    sendMouse(s.m_fixture.view, QEvent::MouseButtonPress, s.centre(), Qt::MiddleButton, Qt::MiddleButton);
    sendMouse(s.m_fixture.view, QEvent::MouseButtonRelease, s.centre(), Qt::MiddleButton, Qt::NoButton);
    REQUIRE(s.waitingForEndpoint());

    sendMouse(s.m_fixture.view, QEvent::MouseButtonDblClick, s.centre(), Qt::MiddleButton, Qt::MiddleButton);
    sendMouse(s.m_fixture.view, QEvent::MouseButtonRelease, s.centre(), Qt::MiddleButton, Qt::NoButton);
    pump();

    CHECK(std::abs(s.m_fixture.viewport()->getFactor().x - extentsFactor) < 1e-9);
    CHECK(s.waitingForEndpoint());
}

TEST_CASE("Beyond the drawing by zooming out at one side and in at the other", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    LineScenario s;
    ScopedSetting noPanOnZoom("Appearance", "PanOnZoom", false);
    s.m_fixture.view->loadSettings();
    const double factor0 = s.m_fixture.factor();
    // which wheel direction zooms in (the user may have inverted it)
    canvasWheel(s.m_fixture.view, s.centre(), 120, Qt::NoModifier);
    const int zoomIn = s.m_fixture.factor() > factor0 ? 120 : -120;
    canvasWheel(s.m_fixture.view, s.centre(), -zoomIn, Qt::NoModifier);
    const Outcome outcome = drive(s, Direction::Right, 60, [&] {
        const QPoint leftSide(60, s.height() / 2);
        const QPoint rightSide(s.width() - 60, s.height() / 2);
        for (int i = 0; i < 3; ++i) {
            canvasWheel(s.m_fixture.view, leftSide, -zoomIn, Qt::NoModifier);
        }
        for (int i = 0; i < 3; ++i) {
            canvasWheel(s.m_fixture.view, rightSide, zoomIn, Qt::NoModifier);
        }
    }, "wheel zoom out left, in right");
    INFO("reached " << outcome.m_reached << " in " << outcome.m_gestures << " zoom out/in pairs");
    checkFar(outcome);
}

TEST_CASE("Beyond the drawing by Shift+wheel and Ctrl+wheel over the canvas", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    for (const Direction direction : {Direction::Right, Direction::Down}) {
        LineScenario s;
        const Qt::KeyboardModifiers modifier = isHorizontal(direction) ? Qt::ShiftModifier : Qt::ControlModifier;
        // the pan direction of a notch follows the user's invert settings: find it
        const double before = s.beyond(direction);
        canvasWheel(s.m_fixture.view, s.centre(), -120, modifier);
        const int angle = s.beyond(direction) > before ? -120 : 120;
        const Outcome outcome = drive(s, direction, 200, [&] {
            canvasWheel(s.m_fixture.view, s.centre(), angle, modifier);
        }, isHorizontal(direction) ? "Shift+wheel" : "Ctrl+wheel");
        INFO(name(direction) << ": reached " << outcome.m_reached << " in " << outcome.m_gestures << " notches");
        checkFar(outcome);
    }
}

TEST_CASE("Autopan near the view's edge is not bounded by the scrollbars", "[navigation][2945-beyond]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    LineScenario s;
    ScopedSetting autoPan("Appearance", "Autopanning", true);
    const QPoint edge(s.width() - 2, s.height() / 2);
    // autopan pans in real time (a timer, 20 to 100 ms per step, after a 10-step delay):
    // hold the cursor at the edge until the view has left the bars' reach
    sendMouse(s.m_fixture.view, QEvent::MouseMove, edge, Qt::NoButton, Qt::NoButton);
    QElapsedTimer clock;
    clock.start();
    double reached = s.beyond(Direction::Right);
    while (clock.elapsed() < 8000 && reached < 1.0) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        reached = std::max(reached, s.beyond(Direction::Right));
    }
    const qint64 elapsed = clock.elapsed();
    sendMouse(s.m_fixture.view, QEvent::MouseMove, s.centre(), Qt::NoButton, Qt::NoButton);
    pump();
    if (qEnvironmentVariableIsSet("LC_NAVTEST_REPORT")) {
        std::cout << "[2945-beyond] autopan right: reached " << reached << " view lengths past the drawing in "
            << elapsed << " ms\n";
    }
    // the view starts with the drawing's edge at its centre (-0.5): autopan takes it past the drawing
    CHECK(reached >= 1.0);
    CHECK(s.waitingForEndpoint());
    const unsigned countBefore = s.m_fixture.graphic->count();
    s.click(s.centre());
    CHECK(s.m_fixture.graphic->count() == countBefore + 1);
}
