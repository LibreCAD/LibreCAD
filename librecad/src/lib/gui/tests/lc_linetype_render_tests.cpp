/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
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
#include <catch2/generators/catch_generators.hpp>

#include <QImage>
#include <QPainterPath>
#include <QPen>

#include "dxf_format.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_printpreviewviewrenderer.h"
#include "lc_printviewportrenderer.h"
#include "lc_splinepoints.h"
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

class AnisotropicViewport final : public LC_GraphicViewport {
public:
    using LC_GraphicViewport::setFactorY;
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
    void setSelectedColor(const RS_Color& color) { m_colorSelectedEntity = color; }
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
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    setPenFor(&middle);
    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.pen().style() == Qt::SolidLine);
    painter.updateDashOffset(&middle);
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    setPenFor(&last);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&last);
    CHECK(painter.currentDashOffset() == Catch::Approx(15.0));
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
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    // Install the running phase; the pen carries it into a QPen whose offset
    // and pattern the painter scale by the same factor k.
    setPenFor();
    REQUIRE(painter.penHasDashPattern());
    const QPen installed = painter.pen();
    REQUIRE_FALSE(installed.dashPattern().isEmpty());
    const double staticFirst =
        std::abs(RS_LineTypePattern::getPattern(RS2::DashLine)->pattern.at(0));
    CHECK(installed.dashOffset() / installed.dashPattern().front()
          == Catch::Approx(10.0 / staticFirst));
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));
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
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    renderer.setPenForEntity(&painter, &second, false);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&second);
    CHECK(painter.currentDashOffset() == Catch::Approx(15.0));
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
        10.0 * std::abs(painter.toGuiDX(1.0)) / painter.getDpmmCached();
    CHECK(painter.currentDashOffset() == Catch::Approx(expected));
}

TEST_CASE("Dash phase remains consistent when selection changes its units", "[gui][pen][linetype]") {
    (void)lc::test::application();
    const bool draft = GENERATE(false, true);
    const int dpr = GENERATE(1, 2);
    const double zoom = GENERATE(0.5, 2.0, 5.0);
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setOffsetAndFactor(0, 0, zoom);
    QImage image(640, 480, QImage::Format_RGB32);
    image.setDevicePixelRatio(dpr);
    LineTypePainter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    RS_Line second{RS_Vector{10., 0.}, RS_Vector{17., 0.}};
    RS_Line third{RS_Vector{17., 0.}, RS_Vector{22., 0.}};
    for (RS_Line* line : {&first, &second, &third}) line->setPen(lineTypePen(RS2::DashLine));
    SECTION("selected, unselected, selected") {
        first.setFlag(RS2::FlagSelected);
        third.setFlag(RS2::FlagSelected);
    }
    SECTION("unselected, selected, unselected") {
        second.setFlag(RS2::FlagSelected);
    }
    const double screenUnitsPerModelUnit = std::abs(painter.toGuiDX(1.0)) / painter.getDpmmCached();
    const auto checkPhase = [&](RS_Line& line, double precedingLength) {
        if (draft) renderer.setPenForDraftEntity(&painter, &line, false);
        else renderer.setPenForEntity(&painter, &line, false);
        const QPen installed = painter.pen();
        REQUIRE_FALSE(installed.dashPattern().isEmpty());
        const auto* pattern = RS_LineTypePattern::getPattern(painter.getPen().getLineType());
        REQUIRE(pattern != nullptr);
        const double expected = precedingLength
            * (line.getFlag(RS2::FlagSelected) ? screenUnitsPerModelUnit : 1.0);
        CHECK(installed.dashOffset() / installed.dashPattern().front()
              == Catch::Approx(expected / std::abs(pattern->pattern.front())));
        painter.updateDashOffset(&line);
    };
    checkPhase(first, 0.0);
    checkPhase(second, 10.0);
    checkPhase(third, 17.0);
}

TEST_CASE("A split dashed line renders like a continuous line", "[gui][pen][linetype]") {
    (void)lc::test::application();
    const double dimensionScale = GENERATE(1.0, 2.0);
    const double zoom = GENERATE(1.0, 2.0);
    enum class Rendering { Drawing, Draft, Preview, Print };
    Rendering rendering = Rendering::Drawing;
    SECTION("drawing") { rendering = Rendering::Drawing; }
    SECTION("draft") { rendering = Rendering::Draft; }
    SECTION("print preview") { rendering = Rendering::Preview; }
    SECTION("print and image export") { rendering = Rendering::Print; }
    const auto render = [rendering, dimensionScale, zoom](bool split) {
        QImage image(300, 80, QImage::Format_RGB32);
        image.fill(Qt::white);
        RS_Graphic graphic;
        graphic.initForNewDocument();
        graphic.addVariable("$DIMSCALE", dimensionScale, DXF_FORMAT_GC_DimScale);
        LC_GraphicViewport viewport;
        viewport.setSize(300, 80);
        viewport.setDocument(&graphic);
        viewport.setOffsetAndFactor(0, 0, zoom);
        RS_Painter painter(&image);
        std::unique_ptr<LC_GraphicViewportRenderer> renderer;
        std::function<void(RS_Line*)> setPenFor;
        if (rendering == Rendering::Drawing || rendering == Rendering::Draft) {
            auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &image);
            view->setBackground(RS_Color(Qt::white));
            view->updateGraphicRelatedSettings(&graphic);
            view->setupPainter(&painter);
            setPenFor = [&painter, view = view.get(), rendering](RS_Line* line) {
                if (rendering == Rendering::Draft) view->setPenForDraftEntity(&painter, line, false);
                else view->setPenForEntity(&painter, line, false);
            };
            renderer = std::move(view);
        } else if (rendering == Rendering::Preview) {
            auto preview = std::make_unique<LineTypePreviewRenderer>(&viewport, &image);
            preview->setBackground(RS_Color(Qt::white));
            preview->updateGraphicRelatedSettings(&graphic);
            preview->setupPainter(&painter);
            setPenFor = [&painter, preview = preview.get()](RS_Line* line) {
                preview->setPenForPrintingEntity(&painter, line);
            };
            renderer = std::move(preview);
        } else {
            auto print = std::make_unique<LineTypePrintRenderer>(&viewport, &painter);
            print->setBackground(RS_Color(Qt::white));
            print->updateGraphicRelatedSettings(&graphic);
            print->setupPainter(&painter);
            setPenFor = [&painter, print = print.get()](RS_Line* line) {
                print->setPenForPrintingEntity(&painter, line);
            };
            renderer = std::move(print);
        }
        RS_Line first{RS_Vector{5., 20.}, RS_Vector{split ? 42. : 125., 20.}};
        RS_Line second{RS_Vector{42., 20.}, RS_Vector{125., 20.}};
        first.setPen(lineTypePen(RS2::DashLine));
        second.setPen(lineTypePen(RS2::DashLine));
        setPenFor(&first);
        first.draw(&painter);
        if (split) {
            setPenFor(&second);
            second.draw(&painter);
        }
        painter.end();
        return image;
    };
    const QImage whole = render(false);
    const QImage split = render(true);
    int darkPixels = 0;
    const int y = 80 - static_cast<int>(20 * zoom);
    const int start = static_cast<int>(47 * zoom);
    const int end = static_cast<int>(120 * zoom);
    for (int x = start; x < end; ++x) {
        darkPixels += whole.pixelColor(x, y) != QColor(Qt::white);
        CHECK(split.pixelColor(x, y) == whole.pixelColor(x, y));
    }
    CHECK(darkPixels > 0);
    CHECK(darkPixels < end - start);
}

TEST_CASE("A vertical dashed run stays continuous with unequal zoom factors", "[gui][pen][linetype]") {
    (void)lc::test::application();
    const bool selected = GENERATE(false, true);
    const bool draft = GENERATE(false, true);
    const auto render = [selected, draft](bool split) {
        QImage image(80, 200, QImage::Format_RGB32);
        image.fill(Qt::white);
        AnisotropicViewport viewport;
        viewport.setSize(80, 200);
        viewport.setOffsetAndFactor(0, 0, 1.0);
        viewport.setFactorY(2.0);
        RS_Painter painter(&image);
        LineTypeViewRenderer renderer(&viewport, &image);
        renderer.setBackground(RS_Color(Qt::white));
        renderer.setSelectedColor(RS_Color(Qt::black));
        renderer.setupPainter(&painter);
        RS_Line first{RS_Vector{20., 5.}, RS_Vector{20., split ? 42. : 75.}};
        RS_Line second{RS_Vector{20., 42.}, RS_Vector{20., 75.}};
        for (RS_Line* line : {&first, &second}) {
            line->setPen(lineTypePen(RS2::DashLine));
            if (selected) line->setFlag(RS2::FlagSelected);
        }
        const auto draw = [&](RS_Line& line) {
            if (draft) renderer.setPenForDraftEntity(&painter, &line, false);
            else renderer.setPenForEntity(&painter, &line, false);
            line.draw(&painter);
        };
        draw(first);
        if (split) draw(second);
        painter.end();
        return image;
    };
    const QImage whole = render(false);
    const QImage split = render(true);
    int ink = 0;
    for (int y = 55; y < 112; ++y) {
        ink += whole.pixelColor(20, y) != QColor(Qt::white);
        CHECK(split.pixelColor(20, y) == whole.pixelColor(20, y));
    }
    CHECK(ink > 0);
    CHECK(ink < 57);
}

TEST_CASE("Spline dash phase uses its rendered length with unequal zoom factors", "[gui][pen][linetype]") {
    (void)lc::test::application();
    AnisotropicViewport viewport;
    viewport.setSize(200, 200);
    viewport.setOffsetAndFactor(0, 0, 1.0);
    viewport.setFactorY(2.0);
    QImage image(200, 200, QImage::Format_RGB32);
    LineTypePainter painter(&image);
    painter.setViewPort(&viewport);
    painter.setPen(lineTypePen(RS2::DashLine));
    LC_SplinePointsData data;
    data.useControlPoints = true;
    data.controlPoints = {{0., 0.}, {10., 20.}, {25., 5.}};
    LC_SplinePoints spline(nullptr, data);
    QPainterPath path(painter.toGuiPointF(data.controlPoints[0]));
    path.quadTo(painter.toGuiPointF(data.controlPoints[1]), painter.toGuiPointF(data.controlPoints[2]));
    painter.updateDashOffset(&spline);
    CHECK(painter.currentDashOffset(RS_Painter::DashSpace::Model) == Catch::Approx(path.length()));
    CHECK(painter.currentDashOffset(RS_Painter::DashSpace::Screen)
          == Catch::Approx(path.length() / painter.getDpmmCached()));
}

TEST_CASE("Diagonal dash phase uses its rendered length with unequal zoom factors", "[gui][pen][linetype]") {
    (void)lc::test::application();
    AnisotropicViewport viewport;
    viewport.setSize(200, 200);
    viewport.setOffsetAndFactor(0, 0, 1.0);
    viewport.setFactorY(2.0);
    QImage image(200, 200, QImage::Format_RGB32);
    LineTypePainter painter(&image);
    painter.setViewPort(&viewport);
    painter.setPen(lineTypePen(RS2::DashLine));
    RS_Line line{RS_Vector{0., 0.}, RS_Vector{3., 4.}};
    painter.updateDashOffset(&line);
    const double renderedLength = std::hypot(3.0, 8.0);
    CHECK(painter.currentDashOffset(RS_Painter::DashSpace::Model) == Catch::Approx(renderedLength));
    CHECK(painter.currentDashOffset(RS_Painter::DashSpace::Screen)
          == Catch::Approx(renderedLength / painter.getDpmmCached()));
}

TEST_CASE("Mixed selection uses rendered distance under unequal zoom", "[gui][pen][linetype]") {
    (void)lc::test::application();
    const bool draft = GENERATE(false, true);
    const bool firstSelected = GENERATE(false, true);
    AnisotropicViewport viewport;
    viewport.setSize(200, 200);
    viewport.setOffsetAndFactor(0, 0, 1.0);
    viewport.setFactorY(2.0);
    QImage image(200, 200, QImage::Format_RGB32);
    LineTypePainter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);
    RS_Line first{RS_Vector{0., 0.}, RS_Vector{0., 10.}};
    RS_Line second{RS_Vector{0., 10.}, RS_Vector{7., 10.}};
    for (RS_Line* line : {&first, &second}) line->setPen(lineTypePen(RS2::DashLine));
    (firstSelected ? first : second).setFlag(RS2::FlagSelected);
    const auto setPen = [&](RS_Line& line) {
        if (draft) renderer.setPenForDraftEntity(&painter, &line, false);
        else renderer.setPenForEntity(&painter, &line, false);
    };
    setPen(first);
    painter.updateDashOffset(&first);
    setPen(second);
    const QPen installed = painter.pen();
    const auto* pattern = RS_LineTypePattern::getPattern(painter.getPen().getLineType());
    REQUIRE(pattern != nullptr);
    REQUIRE_FALSE(installed.dashPattern().isEmpty());
    const double scale = second.getFlag(RS2::FlagSelected) ? painter.getDpmmCached() : 1.0;
    CHECK(installed.dashOffset() / installed.dashPattern().front()
          == Catch::Approx(20.0 / scale / std::abs(pattern->pattern.front())));
}
