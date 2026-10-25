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

// Tests that the renderers know which pen they installed and whether it draws
// a dash pattern, and that the running dash phase advances in the unit of that
// pattern. The phase does not advance for a solid pen, is not scaled by
// $DIMSCALE, and follows device millimetres for a selected entity.

#include <cmath>
#include <functional>
#include <memory>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QImage>
#include <QPen>

#include "dxf_format.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_printpreviewviewrenderer.h"
#include "lc_printviewportrenderer.h"
#include "rs_color.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_linetypepattern.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_vector.h"

namespace {

RS_Pen lineTypePen(const RS2::LineType lineType) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(1.0);
    return pen;
}

class LineTypePainter final : public RS_Painter {
public:
    using RS_Painter::RS_Painter;
    using RS_Painter::getDpmmCached;
};

class LineTypeViewRenderer final : public LC_GraphicViewRenderer {
public:
    LineTypeViewRenderer(LC_GraphicViewport* viewport, QPaintDevice* device)
        : LC_GraphicViewRenderer(viewport, device) {
    }

    using LC_GraphicViewRenderer::setPenForDraftEntity;
    using LC_GraphicViewRenderer::setPenForEntity;
    using LC_GraphicViewRenderer::setupPainter;
    using LC_GraphicViewportRenderer::updateGraphicRelatedSettings;
};

class LineTypePrintRenderer final : public LC_PrintViewportRenderer {
public:
    using LC_PrintViewportRenderer::LC_PrintViewportRenderer;
    using LC_PrintViewportRenderer::setPenForPrintingEntity;
    using LC_PrintViewportRenderer::setupPainter;
    using LC_GraphicViewportRenderer::updateGraphicRelatedSettings;
};

class LineTypePreviewRenderer final : public LC_PrintPreviewViewRenderer {
public:
    using LC_PrintPreviewViewRenderer::LC_PrintPreviewViewRenderer;
    using LC_PrintPreviewViewRenderer::setPenForPrintingEntity;
    using LC_PrintPreviewViewRenderer::setupPainter;
    using LC_GraphicViewportRenderer::updateGraphicRelatedSettings;
};

} // namespace

TEST_CASE("NoPen is not a patterned pen", "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(lineTypePen(RS2::NoPen));

    std::unique_ptr<LC_GraphicViewportRenderer> renderer;
    SECTION("drawing") {
        auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &image);
        view->setupPainter(&painter);
        view->setPenForEntity(&painter, &line, false);
        renderer = std::move(view);
    }
    SECTION("draft") {
        auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &image);
        view->setupPainter(&painter);
        view->setPenForDraftEntity(&painter, &line, false);
        renderer = std::move(view);
    }
    SECTION("print preview") {
        auto preview = std::make_unique<LineTypePreviewRenderer>(&viewport, &image);
        preview->setupPainter(&painter);
        preview->setPenForPrintingEntity(&painter, &line);
        renderer = std::move(preview);
    }
    SECTION("print and image export") {
        auto print = std::make_unique<LineTypePrintRenderer>(&viewport, &painter);
        print->setupPainter(&painter);
        print->setPenForPrintingEntity(&painter, &line);
        renderer = std::move(print);
    }

    REQUIRE_FALSE(painter.penHasDashPattern());

    // What RS_Line::draw() does before drawing the line.
    painter.updateDashOffset(&line);

    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.currentDashOffset() == Catch::Approx(0.0));
}

TEST_CASE("A solid line between dashed ones does not advance the dash phase",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    first.setPen(lineTypePen(RS2::DashLine));
    RS_Line middle{RS_Vector{0., 10.}, RS_Vector{7., 10.}};
    middle.setPen(lineTypePen(RS2::SolidLine));
    RS_Line last{RS_Vector{0., 20.}, RS_Vector{5., 20.}};
    last.setPen(lineTypePen(RS2::DashLine));

    std::unique_ptr<LC_GraphicViewportRenderer> renderer;
    std::function<void(RS_Entity*)> setPenFor;
    SECTION("drawing") {
        auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &image);
        view->setupPainter(&painter);
        setPenFor = [&painter, view = view.get()](RS_Entity* e) {
            view->setPenForEntity(&painter, e, false);
        };
        renderer = std::move(view);
    }
    SECTION("print preview") {
        auto preview = std::make_unique<LineTypePreviewRenderer>(&viewport, &image);
        preview->setupPainter(&painter);
        setPenFor = [&painter, preview = preview.get()](RS_Entity* e) {
            preview->setPenForPrintingEntity(&painter, e);
        };
        renderer = std::move(preview);
    }
    SECTION("print and image export") {
        auto print = std::make_unique<LineTypePrintRenderer>(&viewport, &painter);
        print->setupPainter(&painter);
        setPenFor = [&painter, print = print.get()](RS_Entity* e) {
            print->setPenForPrintingEntity(&painter, e);
        };
        renderer = std::move(print);
    }

    setPenFor(&first);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&first);
    CHECK(painter.currentDashOffset() == Catch::Approx(-10.0));

    setPenFor(&middle);
    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.pen().style() == Qt::SolidLine);
    painter.updateDashOffset(&middle);
    CHECK(painter.currentDashOffset() == Catch::Approx(-10.0));

    setPenFor(&last);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&last);
    CHECK(painter.currentDashOffset() == Catch::Approx(-15.0));
}

TEST_CASE("The dimension scale does not scale the dash phase", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addVariable("$DIMSCALE", 2.0, DXF_FORMAT_GC_DimScale);

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(lineTypePen(RS2::DashLine));

    std::unique_ptr<LC_GraphicViewportRenderer> renderer;
    std::function<void()> setPenFor;
    SECTION("drawing") {
        auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &image);
        view->updateGraphicRelatedSettings(&graphic);
        view->setupPainter(&painter);
        setPenFor = [&painter, &line, view = view.get()] {
            view->setPenForEntity(&painter, &line, false);
        };
        renderer = std::move(view);
    }
    SECTION("print preview") {
        auto preview = std::make_unique<LineTypePreviewRenderer>(&viewport, &image);
        preview->updateGraphicRelatedSettings(&graphic);
        preview->setupPainter(&painter);
        setPenFor = [&painter, &line, preview = preview.get()] {
            preview->setPenForPrintingEntity(&painter, &line);
        };
        renderer = std::move(preview);
    }
    SECTION("print and image export") {
        auto print = std::make_unique<LineTypePrintRenderer>(&viewport, &painter);
        print->updateGraphicRelatedSettings(&graphic);
        print->setupPainter(&painter);
        setPenFor = [&painter, &line, print = print.get()] {
            print->setPenForPrintingEntity(&painter, &line);
        };
        renderer = std::move(print);
    }

    setPenFor();
    painter.updateDashOffset(&line);
    CHECK(painter.currentDashOffset() == Catch::Approx(-10.0));

    // Install the running phase; the pen carries it into a QPen whose offset
    // and pattern the painter scale by the same factor k.
    setPenFor();
    REQUIRE(painter.penHasDashPattern());
    const QPen installed = painter.pen();
    REQUIRE_FALSE(installed.dashPattern().isEmpty());
    const double staticFirst =
        std::abs(RS_LineTypePattern::getPattern(RS2::DashLine)->pattern.at(0));
    CHECK(installed.dashOffset() / installed.dashPattern().front()
          == Catch::Approx(-10.0 / staticFirst));
    CHECK(painter.currentDashOffset() == Catch::Approx(-10.0));
}

TEST_CASE("A dashed run still advances the dash phase", "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    first.setPen(lineTypePen(RS2::DashLine));
    RS_Line second{RS_Vector{10., 0.}, RS_Vector{15., 0.}};
    second.setPen(lineTypePen(RS2::DashLine));

    renderer.setPenForEntity(&painter, &first, false);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&first);
    CHECK(painter.currentDashOffset() == Catch::Approx(-10.0));

    renderer.setPenForEntity(&painter, &second, false);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&second);
    CHECK(painter.currentDashOffset() == Catch::Approx(-15.0));
}

TEST_CASE("A selected entity advances the dash phase in device millimetres",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setOffsetAndFactor(0, 0, 2.0);
    QImage image(640, 480, QImage::Format_RGB32);
    LineTypePainter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(lineTypePen(RS2::SolidLine));
    line.setFlag(RS2::FlagSelected);

    renderer.setPenForEntity(&painter, &line, false);
    REQUIRE(painter.penHasDashPattern());
    CHECK(painter.toGuiDX(1.0) == Catch::Approx(2.0));
    painter.updateDashOffset(&line);
    const double expected =
        -10.0 * std::abs(painter.toGuiDX(1.0)) / painter.getDpmmCached();
    CHECK(painter.currentDashOffset() == Catch::Approx(expected));
}
