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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Tests for the entity pen cache of LC_GraphicViewRenderer and
// LC_PrintViewportRenderer. A cache hit leaves the painter's pen untouched, so a
// pen installed on the painter directly shows whether the cache answered.

#include <functional>

#include <catch2/catch_test_macros.hpp>

#include <QImage>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_overlayentity.h"
#include "lc_overlaysmanager.h"
#include "lc_printviewportrenderer.h"
#include "lc_ref_snap_mark.h"
#include "rs_color.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_vector.h"

namespace {

RS_Pen testPen(const RS2::LineType lineType) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(1.0);
    return pen;
}

class TestViewRenderer final : public LC_GraphicViewRenderer {
public:
    TestViewRenderer(LC_GraphicViewport* viewport, QPaintDevice* device)
        : LC_GraphicViewRenderer(viewport, device) {
    }

    using LC_GraphicViewRenderer::drawOverlayEntitiesInOverlay;
    using LC_GraphicViewRenderer::setPenForDraftEntity;
    using LC_GraphicViewRenderer::setPenForEntity;
    using LC_GraphicViewRenderer::setPenForOverlayEntity;
    void beginPass() { doSetupBeforeContainerDraw(); }
};

class TestPrintRenderer final : public LC_PrintViewportRenderer {
public:
    using LC_PrintViewportRenderer::LC_PrintViewportRenderer;
    using LC_PrintViewportRenderer::setPenForPrintingEntity;
};

struct GreenPen final : LC_OverlayDrawable {
    void draw(RS_Painter* painter) override { painter->setPen(RS_Color(Qt::green)); }
};

// Installs a red pen of the given line type on the painter behind the renderer.
void installMarker(RS_Painter& painter, const RS2::LineType lineType) {
    RS_Pen marker = testPen(lineType);
    marker.setColor(RS_Color(Qt::red));
    painter.setPen(marker);
}

} // namespace

TEST_CASE("a run of selected entities keeps advancing the selection dash phase",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    // A selected entity is painted with RS2::DashLineTiny whatever line type it
    // resolved to, so the second of two same-pen selected lines must be given the
    // running offset rather than be answered from the cache.
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);

    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
    TestViewRenderer renderer{&viewport, &image};
    renderer.beginPass();

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{100., 0.}};
    first.setPen(testPen(RS2::SolidLine));
    first.setFlag(RS2::FlagSelected);
    RS_Line second{RS_Vector{0., 10.}, RS_Vector{100., 10.}};
    second.setPen(testPen(RS2::SolidLine));
    second.setFlag(RS2::FlagSelected);

    renderer.setPenForEntity(&painter, &first, false);
    REQUIRE(painter.pen().style() == Qt::CustomDashLine);
    const double firstPhase = painter.pen().dashOffset();

    // What RS_Line::draw() does for every line it draws.
    painter.updateDashOffset(&first);
    REQUIRE(painter.currentDashOffset() != 0.0);

    renderer.setPenForEntity(&painter, &second, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.pen().dashOffset() != firstPhase);
}

TEST_CASE("the pen cache ignores the dash offset of solid pens", "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Line line{RS_Vector{0., 0.}, RS_Vector{100., 0.}};
    const RS_Line zeroLength{RS_Vector{0., 0.}, RS_Vector{0., 0.}};

    // Solid pens: a repeated request after the running offset moved is a hit.
    // Dashed pens: a hit only while the offset stays where the pen was installed.
    const auto check = [&](RS_Painter& painter, const std::function<void()>& prepare) {
        line.setPen(testPen(RS2::SolidLine));
        prepare();
        installMarker(painter, RS2::DashLine);
        painter.updateDashOffset(&line);
        REQUIRE(painter.currentDashOffset() != 0.0);
        prepare();
        CHECK(painter.pen().color() == QColor(Qt::red));

        line.setPen(testPen(RS2::DashLine));
        prepare();
        const QPen dashed = painter.pen();
        installMarker(painter, RS2::DashLine);
        painter.updateDashOffset(&zeroLength);
        prepare();
        CHECK(painter.pen().color() == QColor(Qt::red));
        painter.updateDashOffset(&line);
        prepare();
        CHECK(painter.pen().color() == dashed.color());
        CHECK(painter.pen().dashOffset() != dashed.dashOffset());
    };

    SECTION("widget renderer") {
        RS_Painter painter{&image};
        TestViewRenderer renderer{&viewport, &image};
        renderer.beginPass();
        check(painter, [&] { renderer.setPenForEntity(&painter, &line, false); });
    }
    SECTION("draft mode") {
        RS_Painter painter{&image};
        TestViewRenderer renderer{&viewport, &image};
        renderer.beginPass();
        check(painter, [&] { renderer.setPenForDraftEntity(&painter, &line, false); });
    }
    SECTION("print renderer") {
        RS_Painter painter{&image};
        TestPrintRenderer renderer{&viewport, &painter};
        check(painter, [&] { renderer.setPenForPrintingEntity(&painter, &line); });
    }
}

TEST_CASE("draft and scaled pens do not answer for each other", "[gui][pen]") {
    (void)lc::test::application();

    // While panning, texts are drawn as draft at screen width 0 and other entities
    // at their lineweight in the same pass.
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
    TestViewRenderer renderer{&viewport, &image};
    renderer.beginPass();

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{100., 0.}};
    line.setPen(testPen(RS2::SolidLine));

    renderer.setPenForDraftEntity(&painter, &line, false);
    installMarker(painter, RS2::SolidLine);
    renderer.setPenForEntity(&painter, &line, false);
    CHECK(painter.pen().color() != QColor(Qt::red));

    installMarker(painter, RS2::SolidLine);
    renderer.setPenForDraftEntity(&painter, &line, false);
    CHECK(painter.pen().color() != QColor(Qt::red));
}

TEST_CASE("pens set outside the entity pen cache invalidate it", "[gui][pen]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image{64, 64, QImage::Format_RGB32};
    RS_Painter painter{&image};
    TestViewRenderer renderer{&viewport, &image};
    renderer.beginPass();

    RS_Pen bluePen = testPen(RS2::SolidLine);
    bluePen.setColor(RS_Color(Qt::blue));
    RS_Line first{RS_Vector{0., 0.}, RS_Vector{100., 0.}};
    first.setPen(bluePen);
    RS_Line second{RS_Vector{0., 10.}, RS_Vector{100., 10.}};
    second.setPen(bluePen);

    renderer.setPenForOverlayEntity(&painter, &first);
    const QPen entityPen = painter.pen();

    SECTION("a snap mark") {
        LC_RefSnapMark mark{nullptr, RS_Vector{0., 0.}, 5, LC_RefSnapMark::NORMAL};
        renderer.setPenForOverlayEntity(&painter, &mark);
    }
    SECTION("an overlay drawable") {
        LC_OverlaysManager* overlays = viewport.getOverlaysManager();
        overlays->addOverlay(new GreenPen, RS2::OverlayGraphics::Snapper);
        renderer.drawOverlayEntitiesInOverlay(overlays, &painter, RS2::OverlayGraphics::Snapper);
    }
    REQUIRE(painter.pen() != entityPen);

    renderer.setPenForOverlayEntity(&painter, &second);
    CHECK(painter.pen() == entityPen);
}
