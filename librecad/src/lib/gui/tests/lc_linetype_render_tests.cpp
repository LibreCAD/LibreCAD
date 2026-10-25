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
// $DIMSCALE, and follows device millimetres for a selected entity. The painter
// draws a named line type from the drawing's table when the renderer resolves it.

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

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
#include "lc_linetype.h"
#include "lc_printpreviewviewrenderer.h"
#include "lc_printviewportrenderer.h"
#include "lc_splinepoints.h"
#include "rs_color.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_layer.h"
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

enum class Rendering { Drawing, Draft, Preview, Print };

struct RenderHarness {
    std::unique_ptr<LC_GraphicViewportRenderer> renderer;
    std::function<void(RS_Entity*)> setPen;

    RenderHarness(Rendering mode, LC_GraphicViewport& viewport, RS_Painter& painter, QPaintDevice& device,
                  RS_Graphic* graphic = nullptr, bool whiteBackground = false) {
        const auto setup = [&](auto& r) {
            if (whiteBackground) r->setBackground(RS_Color(Qt::white));
            if (graphic) r->updateGraphicRelatedSettings(graphic);
            r->setupPainter(&painter);
        };
        if (mode == Rendering::Drawing || mode == Rendering::Draft) {
            auto view = std::make_unique<LineTypeViewRenderer>(&viewport, &device);
            setup(view);
            setPen = [&painter, view = view.get(), mode](RS_Entity* e) {
                if (mode == Rendering::Draft) view->setPenForDraftEntity(&painter, e, false);
                else view->setPenForEntity(&painter, e, false);
            };
            renderer = std::move(view);
        } else if (mode == Rendering::Preview) {
            auto preview = std::make_unique<LineTypePreviewRenderer>(&viewport, &device);
            setup(preview);
            setPen = [&painter, preview = preview.get()](RS_Entity* e) {
                preview->setPenForPrintingEntity(&painter, e);
            };
            renderer = std::move(preview);
        } else {
            auto print = std::make_unique<LineTypePrintRenderer>(&viewport, &painter);
            setup(print);
            setPen = [&painter, print = print.get()](RS_Entity* e) {
                print->setPenForPrintingEntity(&painter, e);
            };
            renderer = std::move(print);
        }
    }
};

RS_Pen namedPen(const QString& name, const QColor& color = Qt::black) {
    RS_Pen pen = lineTypePen(RS2::SolidLine);
    pen.setColor(RS_Color(color));
    pen.setLineTypeName(name);
    return pen;
}

// A line type of the drawing's own making, with this group 49 list.
void addLineType(RS_Graphic& graphic, const QString& name,
                 const std::vector<double>& pattern) {
    auto* entry = new LC_LineType(name);
    entry->pattern = pattern;
    entry->origin = LC_LineType::Origin::Imported;
    graphic.addLineType(entry);
}

void checkDashPattern(const QVector<qreal>& actual, const QVector<qreal>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (int i = 0; i < expected.size(); ++i) {
        CHECK(actual.at(i) == Catch::Approx(expected.at(i)));
    }
}

// The painter's scale from drawing units to QPen dash units, read back from an
// unnamed DashLine pen so no case hard-codes the viewport's factor.
double painterDashScale(RS_Painter& painter, LineTypeViewRenderer& renderer) {
    RS_Line reference{RS_Vector{0., 0.}, RS_Vector{1., 0.}};
    reference.setPen(lineTypePen(RS2::DashLine));
    renderer.setPenForEntity(&painter, &reference, false);
    const QPen installed = painter.pen();
    REQUIRE_FALSE(installed.dashPattern().isEmpty());
    return installed.dashPattern().front()
        / std::abs(RS_LineTypePattern::getPattern(RS2::DashLine)->pattern.at(0));
}

struct InstalledPen {
    QVector<qreal> dashPattern;
    Qt::PenStyle style = Qt::SolidLine;
    bool hasDashes = false;
    double scale = 0.;
};

// One view pass over a line whose pen is @p pen, with @p graphic as the drawing.
InstalledPen installInView(RS_Graphic& graphic, const RS_Pen& pen) {
    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    InstalledPen result;
    result.scale = painterDashScale(painter, renderer);

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(pen);
    renderer.setPenForEntity(&painter, &line, false);
    result.dashPattern = painter.pen().dashPattern();
    result.style = painter.pen().style();
    result.hasDashes = painter.penHasDashPattern();
    return result;
}

// The static screen pattern of an enum, scaled as the painter scales it.
QVector<qreal> screenPattern(const RS2::LineType lineType, const double k) {
    QVector<qreal> pattern;
    for (const qreal dash : RS_LineTypePattern::getPattern(lineType)->pattern) {
        pattern.push_back(std::abs(dash) * k);
    }
    pattern.resize(pattern.size() - pattern.size() % 2);
    return pattern;
}

} // namespace

TEST_CASE("NoPen is not a patterned pen", "[gui][pen][linetype]") {
    (void)lc::test::application();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(lineTypePen(RS2::NoPen));

    Rendering mode = Rendering::Drawing;
    SECTION("drawing") { mode = Rendering::Drawing; }
    SECTION("draft") { mode = Rendering::Draft; }
    SECTION("print preview") { mode = Rendering::Preview; }
    SECTION("print and image export") { mode = Rendering::Print; }
    RenderHarness renderer(mode, viewport, painter, image);
    renderer.setPen(&line);

    REQUIRE_FALSE(painter.penHasDashPattern());

    // What RS_Line::draw() does before drawing the line.
    painter.updateDashOffset(&line);

    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.currentDashOffset() == Catch::Approx(0.0));
}

TEST_CASE("An unknown pattern falls back to a solid pen", "[gui][pen][linetype]") {
    (void)lc::test::application();
    QImage image(64, 64, QImage::Format_RGB32);
    RS_Painter painter(&image);
    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    painter.setPen(lineTypePen(RS2::DashLine));
    REQUIRE(painter.penHasDashPattern());
    painter.setPen(lineTypePen(RS2::LineTypeUnchanged));
    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.pen().style() == Qt::SolidLine);
    painter.updateDashOffset(&line);
    CHECK(painter.currentDashOffset() == Catch::Approx(0.0));
    painter.setPen(lineTypePen(RS2::DashLine));
    CHECK(painter.penHasDashPattern());
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

    Rendering mode = Rendering::Drawing;
    SECTION("drawing") { mode = Rendering::Drawing; }
    SECTION("print preview") { mode = Rendering::Preview; }
    SECTION("print and image export") { mode = Rendering::Print; }
    RenderHarness renderer(mode, viewport, painter, image);

    renderer.setPen(&first);
    CHECK(painter.penHasDashPattern());
    painter.updateDashOffset(&first);
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    renderer.setPen(&middle);
    CHECK_FALSE(painter.penHasDashPattern());
    CHECK(painter.pen().style() == Qt::SolidLine);
    painter.updateDashOffset(&middle);
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    renderer.setPen(&last);
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

    Rendering mode = Rendering::Drawing;
    SECTION("drawing") { mode = Rendering::Drawing; }
    SECTION("print preview") { mode = Rendering::Preview; }
    SECTION("print and image export") { mode = Rendering::Print; }
    RenderHarness renderer(mode, viewport, painter, image, &graphic);

    renderer.setPen(&line);
    painter.updateDashOffset(&line);
    CHECK(painter.currentDashOffset() == Catch::Approx(10.0));

    // Install the running phase; the pen carries it into a QPen whose offset
    // and pattern the painter scale by the same factor k.
    renderer.setPen(&line);
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
    CHECK(painter.currentDashOffset(RS_Painter::DashSpace::Screen) == Catch::Approx(expected));
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
        RenderHarness renderer(rendering, viewport, painter, image, &graphic, true);
        RS_Line first{RS_Vector{5., 20.}, RS_Vector{split ? 42. : 125., 20.}};
        RS_Line second{RS_Vector{42., 20.}, RS_Vector{125., 20.}};
        first.setPen(lineTypePen(RS2::DashLine));
        second.setPen(lineTypePen(RS2::DashLine));
        renderer.setPen(&first);
        first.draw(&painter);
        if (split) {
            renderer.setPen(&second);
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

TEST_CASE("A selected entity keeps the selection dashes whatever its name",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    // A selected reference line reads back the factor a Screen-space pattern
    // is scaled by, the way painterDashScale() does for model space.
    RS_Line reference{RS_Vector{0., 0.}, RS_Vector{1., 0.}};
    reference.setPen(lineTypePen(RS2::DashLineTiny));
    reference.setFlag(RS2::FlagSelected);
    renderer.setPenForEntity(&painter, &reference, false);
    const QPen referencePen = painter.pen();
    REQUIRE_FALSE(referencePen.dashPattern().isEmpty());
    const double k = referencePen.dashPattern().front()
        / std::abs(RS_LineTypePattern::getPattern(RS2::DashLineTiny)->pattern.at(0));
    // Drawing the reference advances the phase, which steps past the renderer's
    // own pen cache.
    painter.updateDashOffset(&reference);

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(namedPen("DASHEDM"));
    line.setFlag(RS2::FlagSelected);
    renderer.setPenForEntity(&painter, &line, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    checkDashPattern(painter.pen().dashPattern(),
                     screenPattern(RS2::DashLineTiny, k));
    // Selection sets the line type by enum, which drops the name.
    CHECK(painter.getPen().getLineTypeId() == 0);
}

TEST_CASE("Group 49 dash lists are canonicalised", "[gui][pen][linetype][dash]") {
    CHECK(RS_Painter::canonicalDashPattern({20., -20., 10.}) == QVector<qreal>{30., 20.});
    CHECK(RS_Painter::canonicalDashPattern({-5., 10., -3.}) == QVector<qreal>{10., 8.});
    CHECK(RS_Painter::canonicalDashPattern({12.7, -6.35, 0., -6.35, 0., -6.35})
          == QVector<qreal>{12.7, 6.35, 0., 6.35, 0., 6.35});
    CHECK(RS_Painter::canonicalDashPattern({0., -6.35}) == QVector<qreal>{0., 6.35});
    CHECK(RS_Painter::canonicalDashPattern({-8., -8.}).isEmpty());
    CHECK(RS_Painter::canonicalDashPattern({0., 0.}).isEmpty());
    CHECK(RS_Painter::canonicalDashPattern({5.}).isEmpty());
    CHECK(RS_Painter::canonicalDashPattern(
              {std::numeric_limits<double>::quiet_NaN(), -1.}).isEmpty());
}

TEST_CASE("A named line draws its table dashes", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});

    const InstalledPen installed = installInView(graphic, namedPen("DASHEDM"));
    CHECK(installed.style == Qt::CustomDashLine);
    CHECK(installed.hasDashes);
    checkDashPattern(installed.dashPattern,
                     {10. * installed.scale, 5. * installed.scale});
}

TEST_CASE("Two names with one enum keep separate dash lists", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});
    addLineType(graphic, "POINTLINE", {20., -5., 0., -5.});

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);
    const double k = painterDashScale(painter, renderer);

    RS_Line first{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    first.setPen(namedPen("DASHEDM"));
    REQUIRE(first.getPen().getLineType() == RS2::SolidLine);
    renderer.setPenForEntity(&painter, &first, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    const QVector<qreal> firstPattern = painter.pen().dashPattern();
    checkDashPattern(firstPattern, {10. * k, 5. * k});

    RS_Line second{RS_Vector{0., 10.}, RS_Vector{10., 10.}};
    second.setPen(namedPen("POINTLINE"));
    REQUIRE(second.getPen().getLineType() == RS2::SolidLine);
    renderer.setPenForEntity(&painter, &second, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    checkDashPattern(painter.pen().dashPattern(), {20. * k, 5. * k, 0., 5. * k});
    checkDashPattern(firstPattern, {10. * k, 5. * k});
}

TEST_CASE("A name the table does not hold draws solid", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    const InstalledPen installed = installInView(graphic, namedPen("NOSUCHTYPE"));
    CHECK(installed.style == Qt::SolidLine);
    CHECK_FALSE(installed.hasDashes);
}

TEST_CASE("A record without dashes draws solid", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "NAMEONLY", {});

    const InstalledPen installed = installInView(graphic, namedPen("NAMEONLY"));
    CHECK(installed.style == Qt::SolidLine);
    CHECK_FALSE(installed.hasDashes);
}

TEST_CASE("A vendor name on a layer draws the layer's pattern", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});

    auto* layer = new RS_Layer("Vendor");
    layer->setPen(namedPen("DASHEDM"));
    graphic.addLayer(layer);

    auto* line = new RS_Line(&graphic, RS_Vector{0., 0.}, RS_Vector{10., 0.});
    line->setPen(RS_Pen{});
    line->setLayer(layer);
    graphic.addEntity(line);
    REQUIRE(line->getPenResolved().getLineTypeName() == "DASHEDM");

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);
    const double k = painterDashScale(painter, renderer);

    renderer.setPenForEntity(&painter, line, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.penHasDashPattern());
    checkDashPattern(painter.pen().dashPattern(), {10. * k, 5. * k});
}

TEST_CASE("The global linetype scale scales a named pattern", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});

    SECTION("$LTSCALE scales it") {
        graphic.addVariable("$LTSCALE", 2.0, DXF_FORMAT_GC_LTScale);
        const InstalledPen installed = installInView(graphic, namedPen("DASHEDM"));
        checkDashPattern(installed.dashPattern,
                         {20. * installed.scale, 10. * installed.scale});
    }
    SECTION("$DIMSCALE does not") {
        graphic.addVariable("$DIMSCALE", 2.0, DXF_FORMAT_GC_DimScale);
        const InstalledPen installed = installInView(graphic, namedPen("DASHEDM"));
        checkDashPattern(installed.dashPattern,
                         {10. * installed.scale, 5. * installed.scale});
    }
}

TEST_CASE("An unnamed built-in keeps its screen pattern", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    // DashDotLine, so neither the reference pen painterDashScale() installed
    // nor its painter memo can serve the pattern.
    RS_Pen dashDot = lineTypePen(RS2::DashDotLine);
    dashDot.setColor(RS_Color(Qt::red));
    const InstalledPen installed = installInView(graphic, dashDot);
    REQUIRE(installed.style == Qt::CustomDashLine);
    checkDashPattern(installed.dashPattern, screenPattern(RS2::DashDotLine, installed.scale));
}

TEST_CASE("A built-in name the file redefines keeps the screen pattern for now",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "HIDDEN", {64., -32.});

    const InstalledPen installed = installInView(graphic, namedPen("HIDDEN"));
    REQUIRE(installed.style == Qt::CustomDashLine);
    checkDashPattern(installed.dashPattern, screenPattern(RS2::HiddenLine, installed.scale));
}

TEST_CASE("A known alias keeps the screen pattern", "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "ACAD_ISO01W100", {5., -5.});

    const InstalledPen installed = installInView(graphic, namedPen("ACAD_ISO01W100"));
    CHECK(installed.style == Qt::SolidLine);
    CHECK_FALSE(installed.hasDashes);
}

TEST_CASE("A table entry added after a pass is seen on the next pass",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);

    RS_Line before{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    before.setPen(namedPen("DASHEDM"));
    renderer.setPenForEntity(&painter, &before, false);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK_FALSE(painter.penHasDashPattern());

    addLineType(graphic, "DASHEDM", {10., -5.});

    // Other colours only step past the renderer's pen cache, which this case
    // is not about: the resolver cache still holds the first pass's table.
    RS_Line stale{RS_Vector{0., 10.}, RS_Vector{10., 10.}};
    stale.setPen(namedPen("DASHEDM", Qt::red));
    renderer.setPenForEntity(&painter, &stale, false);
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK_FALSE(painter.penHasDashPattern());

    renderer.setupPainter(&painter);

    RS_Line after{RS_Vector{0., 20.}, RS_Vector{10., 20.}};
    after.setPen(namedPen("DASHEDM", Qt::blue));
    renderer.setPenForEntity(&painter, &after, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    CHECK(painter.penHasDashPattern());
}

TEST_CASE("A pattern changed between passes is drawn on the next pass",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "DASHEDM", {10., -5.});

    LC_GraphicViewport viewport;
    viewport.setSize(640, 480);
    viewport.setDocument(&graphic);
    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    LineTypeViewRenderer renderer(&viewport, &image);
    renderer.setupPainter(&painter);
    const double k = painterDashScale(painter, renderer);

    RS_Line line{RS_Vector{0., 0.}, RS_Vector{10., 0.}};
    line.setPen(namedPen("DASHEDM"));
    renderer.setPenForEntity(&painter, &line, false);
    REQUIRE(painter.pen().style() == Qt::CustomDashLine);
    checkDashPattern(painter.pen().dashPattern(), {10. * k, 5. * k});
    // Drawing the line advances the phase, which steps past the renderer's own
    // pen cache on the next pass.
    painter.updateDashOffset(&line);

    LC_LineType* entry = graphic.findLineType("DASHEDM");
    REQUIRE(entry != nullptr);
    entry->pattern = {20., -5.};

    renderer.setupPainter(&painter);
    renderer.setPenForEntity(&painter, &line, false);
    CHECK(painter.pen().style() == Qt::CustomDashLine);
    checkDashPattern(painter.pen().dashPattern(), {20. * k, 5. * k});
}

TEST_CASE("A painter without a renderer draws a named line solid",
          "[gui][pen][linetype]") {
    (void)lc::test::application();

    QImage image(640, 480, QImage::Format_RGB32);
    RS_Painter painter(&image);
    painter.setPen(namedPen("DASHEDM"));
    CHECK(painter.pen().style() == Qt::SolidLine);
    CHECK_FALSE(painter.penHasDashPattern());
}
