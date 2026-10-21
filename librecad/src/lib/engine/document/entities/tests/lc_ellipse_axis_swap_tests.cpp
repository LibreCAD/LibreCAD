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

// RS_Ellipse::switchMajorMinor() names the axes of an ellipse the other way round, so that the major
// one is the longer. That describes the same ellipse, so an elliptic arc has to stay the same arc:
// same end points, same length, same middle, same distance snap points. The drawing tools swap after
// the minor axis was picked or typed longer than the major one (Ellipse Arc (Axis)), or the Minor
// Radius option was set larger than the Major Radius (Ellipse Arc (1 Point)); an axis grip dragged
// past the other axis swaps too; the length and Snap Distance of an ellipse that still has its minor
// axis the longer (the Properties dialog can leave one) swap a copy.
//
// Since #2540 the swap moved every arc to another part of its ellipse (issue #2966): the arc that a
// tool created did not end on the directions that were clicked, where the preview had shown it.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <QMouseEvent>

#include "lc_action_draw_ellipse_1point.h"
#include "lc_action_draw_ellipse_axis.h"
#include "lc_actiontestsupport.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_snapper.h"

namespace {

using lc::test::ActionFixture;
using lc::test::eventAt;

constexpr double kTwoPi = 2. * M_PI;

bool samePoint(const RS_Vector& a, const RS_Vector& b, const double tolerance) {
    return a.valid && b.valid && a.distanceTo(b) <= tolerance;
}

/// the same ellipse with its axes named the other way round: what switchMajorMinor() has to give
RS_EllipseData swappedByHand(const RS_EllipseData& d) {
    RS_EllipseData s = d;
    s.majorP = RS_Vector{-d.ratio * d.majorP.y, d.ratio * d.majorP.x};
    s.ratio = 1. / d.ratio;
    // a point at the elliptic angle t of the old axes is at t - pi/2 of the new ones (kept in [0, 2 pi):
    // the tools, the files and getEllipseAngle() give no other angles, and the distance solver of
    // RS_Ellipse does not end for some arcs whose angles are negative)
    if (d.angle1 != 0. || d.angle2 != 0.) {
        const auto wrapped = [](const double t) {
            const double r = std::fmod(t, kTwoPi);
            return r < 0. ? r + kTwoPi : r;
        };
        s.angle1 = wrapped(d.angle1 - M_PI_2);
        s.angle2 = wrapped(d.angle2 - M_PI_2);
    }
    return s;
}

/// where the ray from \p center at the angle \p phi meets the ellipse whose semi-axis \p a lies at
/// the angle \p majorAngle and whose other semi-axis is \p b
RS_Vector pointOnEllipseAlong(const RS_Vector& center, const double majorAngle, const double a, const double b,
                              const double phi) {
    const double c = std::cos(phi - majorAngle);
    const double s = std::sin(phi - majorAngle);
    const double rho = a * b / std::sqrt((b * c) * (b * c) + (a * s) * (a * s));
    return center + RS_Vector{rho * std::cos(phi), rho * std::sin(phi)};
}

/// The end points and the middle of the arc that two clicks, at \p first and \p second, cut out of
/// the ellipse (counterclockwise from the first, or clockwise if \p reversed). The middle is the
/// point half way between the two directions as seen from the center, as RS_Ellipse defines it.
struct ClickedArc {
    RS_Vector start;
    RS_Vector end;
    RS_Vector middle;
};

ClickedArc clickedArc(const RS_Vector& center, const double majorAngle, const double a, const double b,
                      const RS_Vector& first, const RS_Vector& second, const bool reversed) {
    const double phi1 = center.angleTo(first);
    const double phi2 = center.angleTo(second);
    const double sweep = std::fmod((reversed ? phi1 - phi2 : phi2 - phi1) + kTwoPi, kTwoPi);
    const double phiMiddle = reversed ? phi1 - 0.5 * sweep : phi1 + 0.5 * sweep;
    return {pointOnEllipseAlong(center, majorAngle, a, b, phi1), pointOnEllipseAlong(center, majorAngle, a, b, phi2),
            pointOnEllipseAlong(center, majorAngle, a, b, phiMiddle)};
}

const RS_Ellipse* lastEllipse(const RS_Graphic& graphic) {
    const RS_Ellipse* found = nullptr;
    for (const RS_Entity* entity : graphic) {
        if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(entity)) {
            found = ellipse;
        }
    }
    return found;
}

struct AxisArcProbe final : RS_ActionDrawEllipseAxis {
    explicit AxisArcProbe(LC_ActionContext* context) : RS_ActionDrawEllipseAxis(context, true) {}
    using RS_ActionDrawEllipseAxis::onMouseLeftButtonRelease;
    using RS_Snapper::m_snapMode;
    using RS_Snapper::snapPoint;
};

struct OnePointArcProbe final : LC_ActionDrawEllipse1Point {
    explicit OnePointArcProbe(LC_ActionContext* context) : LC_ActionDrawEllipse1Point(context, true) {}
    using LC_ActionDrawEllipse1Point::onMouseLeftButtonRelease;
    using RS_Snapper::m_snapMode;
    using RS_Snapper::snapPoint;
};

template <typename Probe>
void click(Probe& action, const double x, const double y) {
    const LC_MouseEvent event = eventAt(x, y);
    action.onMouseLeftButtonRelease(action.getStatus(), &event);
}

/// what the endpoint and middle snaps of \p action give with the mouse next to \p target
template <typename Probe>
RS_Vector snapNear(lc::test::TestGraphicView& view, Probe& action, const RS_Vector& target) {
    RS_SnapMode& mode = action.m_snapMode;
    mode = RS_SnapMode{};
    mode.snapEndpoint = true;
    mode.snapMiddle = true;
    double x = 0.;
    double y = 0.;
    view.getViewPort()->toUI(target + RS_Vector{0.01, 0.01}, x, y);
    const QMouseEvent event(QEvent::MouseMove, QPointF(x, y), QPointF(x, y), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    return action.snapPoint(&event);
}

/// The arc must end on the two clicked directions, and the snaps must find its ends and middle where
/// the clicks put them. \p a is the semi-axis at \p majorAngle, \p b the other one.
template <typename Probe>
void requireClickedArc(ActionFixture<Probe>& f, const RS_Vector& center, const double majorAngle, const double a,
                       const double b, const RS_Vector& first, const RS_Vector& second, const bool reversed) {
    const RS_Ellipse* arc = lastEllipse(f.m_graphic);
    REQUIRE(arc != nullptr);
    REQUIRE(arc->isEllipticArc());
    REQUIRE(arc->getRatio() <= 1.);
    REQUIRE(arc->isReversed() == reversed);

    const ClickedArc expected = clickedArc(center, majorAngle, a, b, first, second, reversed);
    const double tolerance = 1e-6;
    REQUIRE(samePoint(arc->getStartpoint(), expected.start, tolerance));
    REQUIRE(samePoint(arc->getEndpoint(), expected.end, tolerance));
    REQUIRE(samePoint(arc->getMiddlePoint(), expected.middle, tolerance));

    // the snaps take the mouse to the ends and the middle where they were clicked
    REQUIRE(samePoint(snapNear(f.m_view, *f.m_action, expected.start), expected.start, tolerance));
    REQUIRE(samePoint(snapNear(f.m_view, *f.m_action, expected.end), expected.end, tolerance));
    REQUIRE(samePoint(snapNear(f.m_view, *f.m_action, expected.middle), expected.middle, tolerance));
}

RS_EllipseData randomArc(std::mt19937& rng, const double ratio) {
    std::uniform_real_distribution<double> unit(0., 1.);
    RS_EllipseData d;
    d.center = {unit(rng) * 200. - 100., unit(rng) * 200. - 100.};
    const double rotation = unit(rng) * kTwoPi;
    const double a = 1. + unit(rng) * 40.;
    d.majorP = RS_Vector{a * std::cos(rotation), a * std::sin(rotation)};
    d.ratio = ratio;
    d.angle1 = 0.05 + unit(rng) * (kTwoPi - 0.1);
    d.angle2 = std::fmod(d.angle1 + 0.2 + unit(rng) * (kTwoPi - 0.4), kTwoPi);
    d.reversed = unit(rng) < 0.5;
    return d;
}

} // namespace

TEST_CASE("RS_Ellipse::switchMajorMinor leaves an elliptic arc where it is", "[rs_ellipse][axis-swap]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    std::mt19937 rng(2966);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 600; ++n) {
        // half of them with the minor axis the longer one, the way the drawing tools hand them over
        const RS_EllipseData d = randomArc(rng, n % 2 == 0 ? 1.05 + unit(rng) * 4. : 0.05 + unit(rng) * 0.9);
        CAPTURE(n, d.ratio, d.angle1, d.angle2, d.reversed);
        RS_Ellipse ellipse(&graphic, d);
        REQUIRE(ellipse.isEllipticArc());
        const double scale = std::max(d.majorP.magnitude(), d.majorP.magnitude() * d.ratio);
        const double tolerance = 1e-9 * scale;

        const RS_Vector start = ellipse.getStartpoint();
        const RS_Vector end = ellipse.getEndpoint();
        const RS_Vector middle = ellipse.getMiddlePoint();
        const RS_Vector minCorner = ellipse.getMin();
        const RS_Vector maxCorner = ellipse.getMax();

        // the arc as it is, and as it is with the axes swapped by hand
        const RS_Ellipse byHand(&graphic, swappedByHand(d));
        REQUIRE(samePoint(byHand.getStartpoint(), start, tolerance));
        REQUIRE(samePoint(byHand.getEndpoint(), end, tolerance));
        // the length of an arc that has its minor axis the longer is measured on a swapped copy
        REQUIRE(ellipse.getLength() == Catch::Approx(byHand.getLength()).epsilon(1e-6));

        REQUIRE(ellipse.switchMajorMinor());
        REQUIRE(ellipse.isEllipticArc());
        REQUIRE(ellipse.isReversed() == d.reversed);
        REQUIRE(samePoint(ellipse.getStartpoint(), start, tolerance));
        REQUIRE(samePoint(ellipse.getEndpoint(), end, tolerance));
        REQUIRE(samePoint(ellipse.getMiddlePoint(), middle, 1e-6 * scale));
        REQUIRE(ellipse.getLength() == Catch::Approx(byHand.getLength()).epsilon(1e-6));
        REQUIRE(samePoint(ellipse.getMin(), minCorner, tolerance));
        REQUIRE(samePoint(ellipse.getMax(), maxCorner, tolerance));

        // and back
        REQUIRE(ellipse.switchMajorMinor());
        REQUIRE(samePoint(ellipse.getStartpoint(), start, tolerance));
        REQUIRE(samePoint(ellipse.getEndpoint(), end, tolerance));
    }
}

TEST_CASE("RS_Ellipse::switchMajorMinor keeps a whole ellipse whole", "[rs_ellipse][axis-swap]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_EllipseData d;
    d.center = {3., -4.};
    d.majorP = {10., 5.};
    d.ratio = 2.;
    RS_Ellipse ellipse(&graphic, d);
    const double length = ellipse.getLength();
    REQUIRE_FALSE(ellipse.isEllipticArc());
    REQUIRE(ellipse.switchMajorMinor());
    REQUIRE_FALSE(ellipse.isEllipticArc());
    REQUIRE(ellipse.getRatio() == Catch::Approx(0.5));
    REQUIRE(ellipse.getLength() == Catch::Approx(length).epsilon(1e-9));
}

TEST_CASE("RS_Ellipse::switchMajorMinor keeps the length of a whole-range arc", "[rs_ellipse][axis-swap]") {
    // angle2 = angle1 + 2 pi is a whole ellipse that has a start point
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    for (const double ratio : {0.5, 2.}) {
        for (const double start : {0.3, M_PI_2, 2.}) {
            RS_EllipseData d;
            d.center = {3., -4.};
            d.majorP = {10., 5.};
            d.ratio = ratio;
            d.angle1 = start;
            d.angle2 = start + kTwoPi;
            RS_Ellipse ellipse(&graphic, d);
            const double length = ellipse.getLength();
            CAPTURE(ratio, start);
            REQUIRE(ellipse.switchMajorMinor());
            REQUIRE(ellipse.getLength() == Catch::Approx(length).epsilon(1e-9));
        }
    }
}

TEST_CASE("RS_Ellipse::getNearestDist on an arc with the minor axis the longer matches the arc with its axes swapped",
          "[rs_ellipse][axis-swap]") {
    // Snap Distance measures on a copy whose axes are swapped when the ratio is above 1
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    std::mt19937 rng(2966);
    std::uniform_real_distribution<double> unit(0., 1.);
    for (int n = 0; n < 300; ++n) {
        const RS_EllipseData d = randomArc(rng, 1.05 + unit(rng) * 4.);
        CAPTURE(n, d.ratio, d.angle1, d.angle2, d.reversed);
        const RS_Ellipse ellipse(&graphic, d);
        const RS_Ellipse byHand(&graphic, swappedByHand(d));
        const double scale = d.majorP.magnitude() * d.ratio;
        const double distance = (0.05 + 0.9 * unit(rng)) * byHand.getLength();
        const RS_Vector mouse = ellipse.getCenter() + RS_Vector{(unit(rng) - 0.5) * 3. * scale, (unit(rng) - 0.5) * 3. * scale};

        const RS_Vector expected = byHand.getNearestDist(distance, mouse);
        const RS_Vector found = ellipse.getNearestDist(distance, mouse);
        REQUIRE(expected.valid);
        REQUIRE(samePoint(found, expected, 1e-6 * scale));
    }
}

TEST_CASE("Dragging an axis grip of an elliptic arc past the other axis swaps the axes and keeps the arc",
          "[rs_ellipse][axis-swap]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    for (const bool majorGrip : {false, true}) {
        for (const bool reversed : {false, true}) {
            RS_EllipseData d;
            d.center = {10., -20.};
            d.majorP = {12., 9.};
            d.ratio = 0.5;
            d.angle1 = 0.4;
            d.angle2 = 2.6;
            d.reversed = reversed;
            RS_Ellipse arc(&graphic, d);

            const double majorRadius = d.majorP.magnitude();
            RS_EllipseData expected = d;
            if (majorGrip) {
                // the major axis's end, dragged to 0.3 times its radius: the minor axis is the longer now
                const double newMajorRadius = 0.3 * majorRadius;
                arc.moveRef(arc.getMajorPoint(), d.majorP * ((newMajorRadius - majorRadius) / majorRadius));
                expected.majorP = d.majorP * (newMajorRadius / majorRadius);
                expected.ratio = d.ratio * majorRadius / newMajorRadius;
            } else {
                // the minor axis's end, dragged to 1.6 times the major radius
                const double newMinorRadius = 1.6 * majorRadius;
                const RS_Vector minorDirection = RS_Vector{-d.majorP.y, d.majorP.x} / majorRadius;
                arc.moveRef(arc.getMinorPoint(), minorDirection * (newMinorRadius - arc.getMinorRadius()));
                expected.ratio = newMinorRadius / majorRadius;
            }
            // the arc keeps its angles on the ellipse with the new axes; the swap only renames them
            const RS_Ellipse reference(&graphic, expected);
            const double scale = std::max(reference.getMajorRadius(), reference.getMinorRadius());

            CAPTURE(majorGrip, reversed);
            REQUIRE(arc.getRatio() <= 1.);
            REQUIRE(arc.getMajorRadius() == Catch::Approx(scale).epsilon(1e-9));
            REQUIRE(arc.isReversed() == reversed);
            REQUIRE(samePoint(arc.getStartpoint(), reference.getStartpoint(), 1e-9 * scale));
            REQUIRE(samePoint(arc.getEndpoint(), reference.getEndpoint(), 1e-9 * scale));
        }
    }
}

TEST_CASE("Ellipse Arc (Axis) ends where it was clicked when the minor axis is the longer", "[ellipse-snap][axis-swap]") {
    struct Case {
        RS_Vector center;
        RS_Vector major;
        RS_Vector minor;
        RS_Vector first;
        RS_Vector second;
        bool reversed;
    };
    const Case cases[] = {
        // minor axis longer than the major one: the arc was moved to another part of the ellipse
        {{0., 0.}, {10., 0.}, {0., 20.}, {5., 5.}, {-5., 10.}, false},
        {{0., 0.}, {10., 0.}, {0., 20.}, {5., 5.}, {-5., 10.}, true},
        {{40., -25.}, {10., 10.}, {-30., 30.}, {60., -5.}, {20., -15.}, false},
        {{40., -25.}, {10., 10.}, {-30., 30.}, {60., -5.}, {20., -15.}, true},
        {{-7., 3.}, {0., -6.}, {30., 0.}, {-2., -20.}, {-30., 8.}, false},
        // the minor axis the shorter one: never affected
        {{0., 0.}, {20., 0.}, {0., 10.}, {5., 5.}, {-5., 10.}, false},
        {{0., 0.}, {20., 0.}, {0., 10.}, {5., 5.}, {-5., 10.}, true},
    };
    for (const Case& c : cases) {
        REQUIRE(std::abs(c.major.dotP(c.minor)) < 1e-9);
        ActionFixture<AxisArcProbe> f;
        AxisArcProbe& action = *f.m_action;
        action.init(0);
        action.setReversed(c.reversed);
        click(action, c.center.x, c.center.y);
        click(action, c.center.x + c.major.x, c.center.y + c.major.y);
        click(action, c.center.x + c.minor.x, c.center.y + c.minor.y);
        click(action, c.first.x, c.first.y);
        click(action, c.second.x, c.second.y);

        CAPTURE(c.center.x, c.major.x, c.minor.y, c.reversed);
        requireClickedArc(f, c.center, c.major.angle(), c.major.magnitude(), c.minor.magnitude(), c.first, c.second,
                          c.reversed);
    }
}

TEST_CASE("Ellipse Arc (1 Point) ends where it was clicked when the minor radius is the longer",
          "[ellipse-snap][axis-swap]") {
    for (const double majorDegrees : {0., 30., 125.}) {
        for (const bool reversed : {false, true}) {
            ActionFixture<OnePointArcProbe> f;
            OnePointArcProbe& action = *f.m_action;
            action.init(0);
            action.setMajorRadius(10.);
            action.setMinorRadius(25.);
            action.setHasAngle(majorDegrees != 0.);
            action.setUcsMajorAngleDegrees(majorDegrees);
            action.setReversed(reversed);
            const RS_Vector center{5., 7.};
            const RS_Vector first{25., 30.};
            const RS_Vector second{-15., 40.};
            click(action, center.x, center.y);
            click(action, first.x, first.y);
            click(action, second.x, second.y);

            CAPTURE(majorDegrees, reversed);
            requireClickedArc(f, center, majorDegrees * M_PI / 180., 10., 25., first, second, reversed);
        }
    }
}
