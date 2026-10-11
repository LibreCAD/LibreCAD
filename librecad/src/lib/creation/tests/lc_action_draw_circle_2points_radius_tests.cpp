/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

// Circle, 2 Points and Radius draws the circle of the radius in the tool options through two points. Points
// closer than the diameter have two such circles, one on each side of the chord, and the mouse chooses the one
// with the nearer center. Points a diameter apart have one, centered between them: it is drawn without a choice,
// as soon as the mouse moves. Since the construction moved to LC_CreationCircle (#2540) the action took a center
// only where there were two, so the one circle was not drawn, or was drawn around a center left from before.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include <QMouseEvent>
#include <QStringList>

#include "lc_action_draw_circle_2points_radius.h"
#include "lc_actiontestsupport.h"
#include "lc_creation_circle.h"
#include "lc_graphicviewport.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"

namespace {

std::vector<const RS_Circle*> circlesOf(const RS_EntityContainer& container) {
    std::vector<const RS_Circle*> circles;
    for (const RS_Entity* entity : container) {
        if (entity->rtti() == RS2::EntityCircle) {
            circles.push_back(static_cast<const RS_Circle*>(entity));
        }
    }
    return circles;
}

void checkCircle(const RS_Circle* circle, const RS_Vector& center, const double radius) {
    CHECK(circle->getCenter().x == Catch::Approx(center.x).margin(1e-9));
    CHECK(circle->getCenter().y == Catch::Approx(center.y).margin(1e-9));
    CHECK(circle->getRadius() == Catch::Approx(radius));
}

class MessageContext final : public LC_ActionContext {
public:
    QStringList m_messages;

    void commandMessage(const QString& message) override { m_messages << message; }
};

struct TwoPointsRadius final : LC_ActionDrawCircle2PointsRadius {
    explicit TwoPointsRadius(LC_ActionContext* context) : LC_ActionDrawCircle2PointsRadius(context) {}
    using LC_ActionDrawCircle2PointsRadius::SelectCenter;
    using LC_ActionDrawCircle2PointsRadius::SetPoint1;
    using LC_ActionDrawCircle2PointsRadius::SetPoint2;

    // as RS_PreviewActionInterface::mouseMoveEvent() does it: the preview of a mouse move starts empty
    void move(const double x, const double y) {
        const LC_MouseEvent event = lc::test::eventAt(x, y);
        deletePreviewAndHighlights();
        onMouseMoveEvent(getStatus(), &event);
        drawPreviewAndHighlights();
    }
    void click(const double x, const double y) {
        const LC_MouseEvent event = lc::test::eventAt(x, y);
        onMouseLeftButtonRelease(getStatus(), &event);
    }
    // a coordinate from the command line
    void type(const double x, const double y) {
        RS_CoordinateEvent event(RS_Vector{x, y});
        coordinateEvent(&event);
    }
    std::vector<const RS_Circle*> previewed() const { return circlesOf(*m_preview); }

    // the mouse events of the view, at the pixel of a point of the drawing
    void mouseTo(const RS_Vector& point) {
        const QPointF pixel(m_viewport->toGuiX(point.x), m_viewport->toGuiY(point.y));
        QMouseEvent event(QEvent::MouseMove, pixel, pixel, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        mouseMoveEvent(&event);
    }
    void releaseAt(const RS_Vector& point) {
        const QPointF pixel(m_viewport->toGuiX(point.x), m_viewport->toGuiY(point.y));
        QMouseEvent event(QEvent::MouseButtonRelease, pixel, pixel, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&event);
    }
};

// A new drawing and the action on it, with radius 5. The members are in the order of lc::test::ActionFixture.
struct Radius5 {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    MessageContext m_context;
    std::unique_ptr<TwoPointsRadius> m_action;

    Radius5() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<TwoPointsRadius>(&m_context);
        m_action->init(0);
        m_action->setRadius(5.);
    }
    std::vector<const RS_Circle*> drawn() const { return circlesOf(m_graphic); }
};

// (0, 0) and (6, 0) are on the circles of radius 5 around (3, 4) and (3, -4): the mouse is on the side of one.
void checkChosenCenter(const double side) {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(0., 0.);
    action.click(6., 0.);
    REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);

    action.move(3., 10. * side);
    REQUIRE(action.previewed().size() == 1);
    checkCircle(action.previewed().front(), {3., 4. * side}, 5.);
    CHECK(f.drawn().empty());

    action.click(3., 10. * side);
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {3., 4. * side}, 5.);
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint1);
    CHECK(f.m_context.m_messages.isEmpty());
}

} // namespace

TEST_CASE("LC_CreationCircle::create2PRadius gives the centers of the circles of a radius through two points",
          "[circle-2points-radius]") {
    RS_CircleData circle;
    RS_Vector otherCenter;
    SECTION("two, for points closer than the diameter") {
        REQUIRE(LC_CreationCircle::create2PRadius({0., 0.}, {6., 0.}, 5., otherCenter, circle));
        CHECK(circle.center.distanceTo({3., 4.}) < 1e-9);
        CHECK(otherCenter.distanceTo({3., -4.}) < 1e-9);
        CHECK(circle.radius == 5.);
    }
    SECTION("one, for points a diameter apart: no other center to choose from") {
        CHECK_FALSE(LC_CreationCircle::create2PRadius({0., 0.}, {10., 0.}, 5., otherCenter, circle));
        REQUIRE(circle.center.valid);
        CHECK(circle.center.distanceTo({5., 0.}) < 1e-9);
        CHECK(circle.radius == 5.);
    }
    SECTION("none, for points further apart") {
        circle.center = RS_Vector{1., 2.};
        CHECK_FALSE(LC_CreationCircle::create2PRadius({0., 0.}, {12., 0.}, 5., otherCenter, circle));
        CHECK_FALSE(circle.center.valid);
    }
}

TEST_CASE("LC_CreationCircle::create2PRadius has the middle of a diameter for the center, in any direction of it",
          "[circle-2points-radius]") {
    // With half the distance of the points for the radius the chord is a diameter, as it is measured. The sum of the
    // squares of its components may still round to more than the square of the diameter, and the root of that
    // difference is no number: it made a center of NaN, with another to choose from.
    const RS_Vector origins[] = {{0., 0.}, {12.3, 45.6}, {-1234.5, 987.25}};
    for (const RS_Vector& first : origins) {
        for (const double length : {2., 10., 25.4, 2000.}) {
            for (int tenth = 0; tenth < 3600; ++tenth) {
                const RS_Vector second = first + RS_Vector::polar(length, RS_Math::deg2rad(tenth / 10.));
                const double radius = 0.5 * second.distanceTo(first);
                RS_CircleData circle;
                RS_Vector otherCenter;
                CAPTURE(first.x, first.y, length, tenth);
                REQUIRE_FALSE(LC_CreationCircle::create2PRadius(first, second, radius, otherCenter, circle));
                REQUIRE(circle.center.valid);
                REQUIRE(circle.center.distanceTo((first + second) * 0.5) < 1e-9);
                REQUIRE(circle.radius == radius);
            }
        }
    }
}

TEST_CASE("Circle 2 Points Radius: the mouse chooses between the two circles through the points",
          "[circle-2points-radius]") {
    SECTION("above the chord") { checkChosenCenter(1.); }
    SECTION("below the chord") { checkChosenCenter(-1.); }
}

TEST_CASE("Circle 2 Points Radius: the one circle through points a diameter apart is drawn without choosing a center",
          "[circle-2points-radius]") {
    // (0, 0) and (10, 0): of the circles with radius 5, only the one around (5, 0) passes through both
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;

    SECTION("when the mouse moves") {
        action.click(0., 0.);
        action.click(10., 0.);
        REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
        REQUIRE(f.drawn().empty());
        action.move(5., 3.);
    }
    SECTION("at a click, where the mouse has not moved") {
        action.click(0., 0.);
        action.click(10., 0.);
        action.click(5., 3.);
    }
    SECTION("with coordinates from the command line") {
        action.type(0., 0.);
        action.type(10., 0.);
        REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
        action.type(5., 3.);
    }
    SECTION("after the mouse has come to the second point") {
        action.click(0., 0.);
        // by (9, 0), which is on the circles around (4.5, 2.18) and (4.5, -2.18) with (0, 0)
        action.move(9., 0.);
        action.move(10., 0.);
        action.click(10., 0.);
        action.move(5., 3.);
    }

    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {5., 0.}, 5.);
    // the action is ready for the next circle, and has asked nothing
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint1);
    CHECK(f.m_context.m_messages.isEmpty());
}

TEST_CASE("Circle 2 Points Radius: a circle does not take the center of the circle before it",
          "[circle-2points-radius]") {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(0., 0.);
    action.click(6., 0.);
    action.click(3., 10.);
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {3., 4.}, 5.);

    // the same action goes on with two points a diameter apart
    action.click(0., 0.);
    action.click(10., 0.);
    action.move(5., 3.);
    REQUIRE(f.drawn().size() == 2);
    checkCircle(f.drawn().back(), {5., 0.}, 5.);
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint1);
}

TEST_CASE("Circle 2 Points Radius: a mouse as far from one center as from the other chooses neither",
          "[circle-2points-radius]") {
    // (-3, 0) and (3, 0) are on the circles around (0, 4) and (0, -4): the middle of the chord is 4 from both centers
    const RS_Vector onChord{0., 0.};
    RS_CircleData circle;
    RS_Vector otherCenter;
    REQUIRE(LC_CreationCircle::create2PRadius({-3., 0.}, {3., 0.}, 5., otherCenter, circle));
    REQUIRE(onChord.squaredTo(circle.center) == onChord.squaredTo(otherCenter));

    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(-3., 0.);
    action.click(3., 0.);
    REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
    action.move(0., 3.);
    REQUIRE(action.previewed().size() == 1);
    checkCircle(action.previewed().front(), {0., 4.}, 5.);

    // on its way across the chord the mouse shows no circle, and draws none
    action.move(onChord.x, onChord.y);
    CHECK(action.previewed().empty());
    CHECK(f.drawn().empty());
    CHECK(action.getStatus() == TwoPointsRadius::SelectCenter);

    // a click there is answered with the request to choose
    action.click(onChord.x, onChord.y);
    CHECK(f.drawn().empty());
    CHECK(action.getStatus() == TwoPointsRadius::SelectCenter);
    CHECK(f.m_context.m_messages.size() == 1);

    // off the chord the mouse chooses again
    action.move(0., -1.);
    REQUIRE(action.previewed().size() == 1);
    checkCircle(action.previewed().front(), {0., -4.}, 5.);
    action.click(0., -1.);
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {0., -4.}, 5.);
}

TEST_CASE("Circle 2 Points Radius: the second point is previewed with a circle wherever it is closer than a diameter",
          "[circle-2points-radius]") {
    // As the second point the mouse is on the chord, as far from one center as from the other up to the rounding,
    // and at many places exactly: it has no center to choose yet, and the preview is of one of the two circles.
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(0., 0.);
    for (int x = -9; x <= 9; ++x) {
        for (int y = -9; y <= 9; ++y) {
            if ((x == 0 && y == 0) || (x * x) + (y * y) >= 100) {
                continue;
            }
            action.move(x, y);
            CAPTURE(x, y);
            REQUIRE(action.previewed().size() == 1);
            const RS_Vector center = action.previewed().front()->getCenter();
            REQUIRE(center.distanceTo(RS_Vector{0., 0.}) == Catch::Approx(5.));
            REQUIRE(center.distanceTo(RS_Vector(x, y)) == Catch::Approx(5.));
        }
    }
    CHECK(f.drawn().empty());
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint2);
}

TEST_CASE("Circle 2 Points Radius: a second point further than a diameter from the first is not taken",
          "[circle-2points-radius]") {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(0., 0.);
    REQUIRE(action.getStatus() == TwoPointsRadius::SetPoint2);

    // (12, 0) is 12 from (0, 0), and the diameter is 10
    action.move(12., 0.);
    CHECK(action.previewed().empty());
    action.click(12., 0.);
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint2);
    CHECK(f.m_context.m_messages.size() == 1);
    action.move(6., 20.);
    CHECK(action.previewed().empty());
    CHECK(f.drawn().empty());

    // the first point is kept for a second point within reach
    action.click(6., 0.);
    REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
    action.click(3., 10.);
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {3., 4.}, 5.);
}

TEST_CASE("Circle 2 Points Radius: no circle is drawn while the radius is too small for the two points",
          "[circle-2points-radius]") {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    action.click(0., 0.);
    action.click(6., 0.);
    action.move(3., 10.);
    REQUIRE(action.previewed().size() == 1);

    // the radius in the tool options changes while the center is chosen: a diameter of 4 does not reach from
    // (0, 0) to (6, 0)
    action.setRadius(2.);
    action.move(3., 10.);
    CHECK(action.previewed().empty());
    CHECK(f.drawn().empty());
    action.click(3., 10.);
    CHECK(f.drawn().empty());
    CHECK(action.getStatus() == TwoPointsRadius::SelectCenter);

    // with a radius that reaches, the mouse chooses again
    action.setRadius(5.);
    action.click(3., 10.);
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {3., 4.}, 5.);
}

TEST_CASE("Circle 2 Points Radius: the circle through points a diameter apart is drawn in any direction of the chord",
          "[circle-2points-radius]") {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    const RS_Vector first{12.3, 45.6};
    unsigned circles = 0;
    // the second point is given at the length of the diameter and an angle: @10<angle
    const auto secondPoint = [&first](const int tenth) {
        return first + RS_Vector::polar(10., RS_Math::deg2rad(tenth / 10.));
    };
    const auto checkDrawnAtMouseMove = [&](const RS_Vector& second, const double radius) {
        action.move(first.x + 3., first.y + 20.);
        ++circles;
        REQUIRE(f.m_graphic.count() == circles);
        const auto* circle = dynamic_cast<const RS_Circle*>(f.m_graphic.lastEntity());
        REQUIRE(circle != nullptr);
        REQUIRE(circle->getCenter().distanceTo((first + second) * 0.5) < 1e-9);
        REQUIRE(circle->getRadius() == radius);
        REQUIRE(action.getStatus() == TwoPointsRadius::SetPoint1);
    };

    SECTION("with half the distance of the points for the radius") {
        // as the radius of the tool options is picked in the drawing: every chord is a diameter, as it is measured
        for (int tenth = 0; tenth < 3600; ++tenth) {
            const RS_Vector second = secondPoint(tenth);
            const double radius = 0.5 * second.distanceTo(first);
            action.setRadius(radius);
            action.type(first.x, first.y);
            action.type(second.x, second.y);
            CAPTURE(tenth);
            REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
            checkDrawnAtMouseMove(second, radius);
        }
    }
    SECTION("with the radius of 5") {
        for (int tenth = 0; tenth < 3600; ++tenth) {
            const RS_Vector second = secondPoint(tenth);
            action.type(first.x, first.y);
            action.type(second.x, second.y);
            if (action.getStatus() != TwoPointsRadius::SelectCenter) {
                // the coordinates of the second point round to more than a diameter from the first: it is not taken
                action.init(0);
                continue;
            }
            CAPTURE(tenth);
            checkDrawnAtMouseMove(second, 5.);
        }
        // of the directions, those whose coordinates do not round to more than a diameter
        CHECK(circles > 360);
    }
}

TEST_CASE("Circle 2 Points Radius: the mouse events of the view draw the one circle through points a diameter apart",
          "[circle-2points-radius]") {
    Radius5 f;
    TwoPointsRadius& action = *f.m_action;
    // a pixel is a drawing unit: (100, 100) and (110, 100) come back from their pixels as they are, a diameter apart
    f.m_view.getViewPort()->setSize(f.m_view.getWidth(), f.m_view.getHeight());
    action.mouseTo({100., 100.});
    action.releaseAt({100., 100.});
    action.mouseTo({109., 100.});
    action.mouseTo({110., 100.});
    action.releaseAt({110., 100.});
    REQUIRE(action.getStatus() == TwoPointsRadius::SelectCenter);
    REQUIRE(f.drawn().empty());

    action.mouseTo({105., 103.});
    REQUIRE(f.drawn().size() == 1);
    checkCircle(f.drawn().front(), {105., 100.}, 5.);
    CHECK(action.previewed().empty());
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint1);

    // the click that follows starts the next circle
    action.releaseAt({105., 103.});
    CHECK(f.drawn().size() == 1);
    CHECK(action.getStatus() == TwoPointsRadius::SetPoint2);
}
