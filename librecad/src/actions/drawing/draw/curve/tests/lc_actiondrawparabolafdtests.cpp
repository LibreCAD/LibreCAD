/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */

#include <array>
#include <cmath>
#include <memory>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "lc_actiondrawparabolaFD.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetsource.h"
#include "lc_parabola.h"

namespace {

class ParabolaFDProbe final : public LC_ActionDrawParabolaFD {
public:
    using LC_ActionDrawParabolaFD::LC_ActionDrawParabolaFD;
    using LC_ActionDrawParabolaFD::SetDirectrix;
    using LC_ActionDrawParabolaFD::SetEndPoint;
    using LC_ActionDrawParabolaFD::SetFocus;
    using LC_ActionDrawParabolaFD::SetStartPoint;
    using LC_ActionDrawParabolaFD::onCoordinateEvent;
    using LC_ActionDrawParabolaFD::onMouseLeftButtonRelease;
};

struct ParabolaFDFixture {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    std::shared_ptr<ParabolaFDProbe> action;

    ParabolaFDFixture() {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        action = std::make_shared<ParabolaFDProbe>(&context);
    }

    ~ParabolaFDFixture() {
        view.beginClose();
    }
};

} // namespace

TEST_CASE("Focus-vertex parabola action creates an exact typed source",
          "[actions][draw][parabola][focus-vertex]") {
    ParabolaFDFixture fixture;
    REQUIRE(fixture.qtReady);
    REQUIRE(fixture.view.setCurrentAction(fixture.action));

    const RS_Vector focus{0.0, 4.0};
    const RS_Vector vertex{0.0, 0.0};
    fixture.action->onCoordinateEvent(ParabolaFDProbe::SetFocus, false, focus);
    REQUIRE(fixture.action->getStatus() == ParabolaFDProbe::SetDirectrix);

    const LC_MouseEvent vertexClick = lc::test::eventAt(vertex.x, vertex.y);
    fixture.action->onMouseLeftButtonRelease(ParabolaFDProbe::SetDirectrix,
                                             &vertexClick);
    REQUIRE(fixture.action->getStatus() == ParabolaFDProbe::SetStartPoint);

    fixture.action->onCoordinateEvent(ParabolaFDProbe::SetStartPoint, false,
                                      {-2.0, 0.25});
    REQUIRE(fixture.action->getStatus() == ParabolaFDProbe::SetEndPoint);
    fixture.action->onCoordinateEvent(ParabolaFDProbe::SetEndPoint, false,
                                      {2.0, 0.25});

    LC_Parabola* created = nullptr;
    for (RS_Entity* entity : fixture.graphic) {
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityParabola) {
            REQUIRE(created == nullptr);
            created = dynamic_cast<LC_Parabola*>(entity);
        }
    }
    REQUIRE(created != nullptr);

    const LC_ParabolaData& data = created->getData();
    REQUIRE(data.isValid());
    CHECK(data.getFocus().distanceTo(focus) < 1e-10);
    CHECK(data.m_vertex.distanceTo(vertex) < 1e-10);

    const RS_LineData directrix = data.getDirectrix();
    CHECK(directrix.startpoint.y == Catch::Approx(-4.0));
    CHECK(directrix.endpoint.y == Catch::Approx(-4.0));
    for (const double x : {-2.0, -1.0, 0.0, 1.0, 2.0}) {
        const RS_Vector point = data.fromX(x);
        const double directrixDistance = std::abs(point.y - directrix.startpoint.y);
        CHECK(point.distanceTo(focus) == Catch::Approx(directrixDistance).margin(1e-10));
    }

    const auto source = LC_OffsetSourceGeometry::capture(*created);
    REQUIRE(source.has_value());
    CHECK(source->kind() == LC_OffsetSourceGeometry::Kind::Parabola);
}
