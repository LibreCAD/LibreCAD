/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "lc_actiondrawparabola4points.h"
#include "lc_actiontestsupport.h"
#include "lc_parabola.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {

class Parabola4PointProbe final : public LC_ActionDrawParabola4Points {
public:
    using LC_ActionDrawParabola4Points::LC_ActionDrawParabola4Points;
    using LC_ActionDrawParabola4Points::SetAxis;
    using LC_ActionDrawParabola4Points::SetPoint1;
    using LC_ActionDrawParabola4Points::onCoordinateEvent;
    using LC_ActionDrawParabola4Points::onMouseMoveEvent;
};

struct Parabola4PointFixture {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    std::shared_ptr<Parabola4PointProbe> action;

    Parabola4PointFixture() {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        action = std::make_shared<Parabola4PointProbe>(&context);
    }

    ~Parabola4PointFixture() {
        view.beginClose();
    }
};

RS_Vector quadraticBezier(const std::array<RS_Vector, 3>& controls, const double t) {
    const double remaining = 1.0 - t;
    return controls[0] * (remaining * remaining) +
           controls[1] * (2.0 * remaining * t) + controls[2] * (t * t);
}

} // namespace

TEST_CASE("Four-point parabola action resolves dense-end ambiguity by axis pick",
          "[actions][draw][parabola][four-points][conditioning]") {
    Parabola4PointFixture fixture;
    REQUIRE(fixture.qtReady);

    const std::array<RS_Vector, 3> controls{{{0.0, 0.0}, {5.0, 10.0}, {11.0, 2.0}}};
    constexpr double scale = 1e-2;
    constexpr double angle = 0.6435011087932844;
    const RS_Vector translation{1e6, -2e6};
    const auto transform = [&](const RS_Vector& point) {
        RS_Vector mapped = point * scale;
        mapped.rotate(angle);
        return mapped + translation;
    };
    const std::array<RS_Vector, 4> samples{{
        transform(quadraticBezier(controls, 0.0)),
        transform(quadraticBezier(controls, 0.98)),
        transform(quadraticBezier(controls, 0.999)),
        transform(quadraticBezier(controls, 1.0)),
    }};
    const auto candidates = LC_ParabolaData::From4Points(
        {samples.begin(), samples.end()});
    REQUIRE(candidates.size() >= 2);
    const double tolerance =
        256.0 * std::numeric_limits<double>::epsilon() * 2e6;
    const auto matchesSamples = [&](const LC_ParabolaData& candidate) {
        return candidate.isValid() && std::all_of(samples.begin(), samples.end(),
            [&](const RS_Vector& point) {
                return candidate.fromX(candidate.findX(point)).distanceTo(point) <= tolerance;
            });
    };
    CHECK(std::all_of(candidates.begin(), candidates.end(), matchesSamples));

    REQUIRE(fixture.view.setCurrentAction(fixture.action));
    for (const RS_Vector& point : samples) {
        const int status = fixture.action->getStatus();
        fixture.action->onCoordinateEvent(status, false, point);
    }
    REQUIRE(fixture.action->getStatus() == Parabola4PointProbe::SetAxis);

    const RS_Vector axisPick = candidates.front().getAxis().endpoint;
    const LC_MouseEvent move = lc::test::eventAt(axisPick.x, axisPick.y);
    fixture.action->onMouseMoveEvent(Parabola4PointProbe::SetAxis, &move);
    fixture.action->onCoordinateEvent(Parabola4PointProbe::SetAxis, false, axisPick);

    LC_Parabola* created = nullptr;
    for (RS_Entity* entity : fixture.graphic) {
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityParabola) {
            REQUIRE(created == nullptr);
            created = dynamic_cast<LC_Parabola*>(entity);
        }
    }
    REQUIRE(created != nullptr);
    const auto& createdData = created->getData();
    CHECK(std::all_of(samples.begin(), samples.end(), [&](const RS_Vector& point) {
        return createdData.fromX(createdData.findX(point)).distanceTo(point) <= tolerance;
    }));
}
