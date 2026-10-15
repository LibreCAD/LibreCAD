// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors

#include <catch2/catch_test_macros.hpp>
#include <QImage>
#include <QTemporaryDir>
#include "lc_actiontestsupport.h"
#include "lc_crosshair.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_settings.h"
#include "lc_polygonselection.h"
#include "rs_graphic.h"
#include "rs_circle.h"
#include "rs_arc.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_image.h"

namespace {
QImage cursorImage(int shape, int lines, int size, int percent, int dpr, RS_Vector pos = {100, 100}) {
    lc::test::application();
    LC_GraphicViewport viewport;
    viewport.setSize(200, 200);
    QImage image(200 * dpr, 200 * dpr, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::black);
    RS_Painter painter(&image);
    painter.setViewPort(&viewport);
    RS_Pen pen(RS_Color(Qt::white), RS2::Width00, RS2::SolidLine);
    pen.setScreenWidth(1);
    LC_Crosshair cursor(RS_Vector(false), shape, lines, pen, 0, 0);
    cursor.setShapesPen(pen);
    cursor.setUiPosition(pos);
    cursor.setPickboxSize(size);
    cursor.setLengthPercent(percent);
    cursor.draw(&painter);
    painter.end();
    return image;
}

TEST_CASE("Freehand selection follows concave boundaries and exact curves", "[selection][lasso]") {
    lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    LC_GraphicViewport viewport;
    viewport.setDocument(&graphic);
    auto* enclosed = new RS_Line(&graphic, {1, 1}, {2, 2});
    auto* bridge = new RS_Line(&graphic, {1, 8}, {8, 1});
    auto* outside = new RS_Line(&graphic, {6, 7}, {9, 8});
    auto* tangent = new RS_Circle(&graphic, RS_CircleData({1, 1}, 1));
    auto* surrounding = new RS_Circle(&graphic, RS_CircleData({5, 5}, 20));
    auto* arc = new RS_Arc(&graphic, RS_ArcData({1, 1}, .5, 0, M_PI, false));
    RS_EllipseData ellipseData;
    ellipseData.center = {1.5, 1.5};
    ellipseData.majorP = {.7, 0};
    ellipseData.ratio = .5;
    auto* ellipse = new RS_Ellipse(&graphic, ellipseData);
    for (auto* entity : QList<RS_Entity*>{enclosed, bridge, outside, tangent, surrounding, arc, ellipse}) {
        entity->setLayer(graphic.getActiveLayer());
        graphic.addEntity(entity);
    }
    graphic.calculateBorders();
    const QPolygonF boundary{{0, 0}, {10, 0}, {10, 3}, {3, 3}, {3, 10}, {0, 10}};
    graphic.setModified(false);
    const bool modified = graphic.isModified();
    LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityUnknown, false, true);
    CHECK(enclosed->isSelected());
    CHECK_FALSE(bridge->isSelected());
    CHECK_FALSE(outside->isSelected());
    CHECK(tangent->isSelected());
    CHECK(arc->isSelected());
    CHECK(ellipse->isSelected());
    CHECK_FALSE(surrounding->isSelected()); // lasso inside a circle is not a boundary hit
    CHECK(graphic.isModified() == modified);
    LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityUnknown, true, true);
    CHECK(bridge->isSelected());
    CHECK_FALSE(outside->isSelected());
    LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityUnknown, true, false);
    CHECK_FALSE(enclosed->isSelected());
    CHECK_FALSE(bridge->isSelected());
    graphic.getActiveLayer()->lock(true);
    LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityUnknown, true, true);
    CHECK_FALSE(enclosed->isSelected());
    graphic.getActiveLayer()->lock(false);
    LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityLine, false, true);
    CHECK(enclosed->isSelected());
    CHECK_FALSE(tangent->isSelected());
}

TEST_CASE("Image lasso selection distinguishes filled crossing from full containment", "[selection][lasso][image]") {
    lc::test::application();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto filename = directory.filePath("lasso.png");
    QImage pixels(10, 10, QImage::Format_RGB32);
    pixels.fill(Qt::white);
    REQUIRE(pixels.save(filename));

    for (double rotation : {0.0, M_PI / 4}) {
        INFO("Image rotation " << rotation);
        RS_Graphic graphic;
        graphic.initForNewDocument();
        LC_GraphicViewport viewport;
        viewport.setDocument(&graphic);
        const RS_Vector origin{20, 30};
        const auto u = RS_Vector::polar(1, rotation);
        const auto v = RS_Vector::polar(1, rotation + M_PI / 2);
        auto* image = new RS_Image(&graphic,
            RS_ImageData(0, origin, u, v, {10, 10}, filename, 50, 50, 0));
        image->setLayer(graphic.getActiveLayer());
        graphic.addEntity(image);
        graphic.calculateBorders();
        graphic.setModified(false);

        auto select = [&](const QPolygonF& local, bool crossing) {
            QPolygonF boundary;
            for (const auto& point : local) {
                const auto world = origin + u * point.x() + v * point.y();
                boundary.append(QPointF(world.x, world.y));
            }
            image->setSelectionFlag(false);
            LC_PolygonSelection::select(&graphic, &viewport, boundary, RS2::EntityUnknown, crossing, true);
            CHECK_FALSE(graphic.isModified());
            return image->isSelected();
        };
        const QPolygonF interior{{2, 2}, {4, 2}, {4, 4}, {2, 4}};
        CHECK_FALSE(select(interior, false)); // Small window inside a larger image.
        CHECK(select(interior, true));       // Crossing includes the filled interior.
        const QPolygonF enclosing{{-1, -1}, {11, -1}, {11, 11}, {-1, 11}};
        CHECK(select(enclosing, false));
        CHECK(select(enclosing, true));
        const QPolygonF strip{{-1, 4}, {11, 4}, {11, 6}, {-1, 6}};
        CHECK_FALSE(select(strip, false));
        CHECK(select(strip, true));          // Neither shape has a corner in the other.
        const QPolygonF outside{{12, 12}, {14, 12}, {14, 14}, {12, 14}};
        CHECK_FALSE(select(outside, false));
        CHECK_FALSE(select(outside, true));
        const QPolygonF concave{{-1, -1}, {11, -1}, {11, 11}, {6, 11},
                                {6, 4}, {4, 4}, {4, 11}, {-1, 11}};
        CHECK_FALSE(select(concave, false)); // All corners fit, but an edge crosses the notch.
        CHECK(select(concave, true));
    }
}

bool ink(const QImage& image, int x, int y) {
    const int dpr = int(image.devicePixelRatio());
    for (int dx = 0; dx < dpr; ++dx)
        for (int dy = 0; dy < dpr; ++dy)
            if (image.pixelColor(x * dpr + dx, y * dpr + dy).lightness() > 0) return true;
    return false;
}
}

TEST_CASE("Drawing cursor modes render distinct logical geometry on Retina", "[cursor][render]") {
    for (int dpr : {1, 2}) {
        INFO("DPR " << dpr);
        const auto idle = cursorImage(LC_Crosshair::Square, LC_Crosshair::Crosshair, 10, 50, dpr);
        CHECK(ink(idle, 105, 100));
        CHECK(ink(idle, 125, 100));
        CHECK_FALSE(ink(idle, 100, 100));
        CHECK_FALSE(ink(idle, 175, 100));
        const auto point = cursorImage(LC_Crosshair::NoShape, LC_Crosshair::Crosshair, 10, 50, dpr);
        CHECK(ink(point, 100, 100));
        CHECK_FALSE(ink(point, 105, 105));
        const auto selection = cursorImage(LC_Crosshair::Square, LC_Crosshair::NoLines, 10, 50, dpr);
        CHECK(ink(selection, 105, 100));
        CHECK_FALSE(ink(selection, 125, 100));
        CHECK_FALSE(ink(selection, 100, 100));
    }
}

TEST_CASE("Short cursor arms never run backwards into a pickbox", "[cursor][render]") {
    for (int dpr : {1, 2}) {
        const auto shortArms = cursorImage(LC_Crosshair::Square, LC_Crosshair::Crosshair, 80, 1, dpr);
        CHECK(ink(shortArms, 140, 100));
        CHECK_FALSE(ink(shortArms, 125, 100));
        CHECK_FALSE(ink(shortArms, 100, 100));
        const auto edge = cursorImage(LC_Crosshair::Square, LC_Crosshair::Crosshair, 10, 100, dpr, {2, 100});
        CHECK_FALSE(ink(edge, 1, 100));
        CHECK(ink(edge, 20, 100));
        const auto bounded = cursorImage(LC_Crosshair::Square, LC_Crosshair::NoLines, -50, 200, dpr);
        CHECK(ink(bounded, 102, 100));
        CHECK_FALSE(ink(bounded, 110, 100));
    }
}

TEST_CASE("Selection grips keep their logical size at every zoom and device ratio", "[selection][render]") {
    lc::test::application();
    struct Renderer : LC_GraphicViewRenderer {
        using LC_GraphicViewRenderer::LC_GraphicViewRenderer;
        using LC_GraphicViewRenderer::drawEntityReferencePoints;
        using LC_GraphicViewRenderer::setPenForEntity;
        using LC_GraphicViewRenderer::setPenForDraftEntity;
    };
    const bool oldStyle = LC_GET_ONE_BOOL("Appearance", "CADSelectionStyle", true);
    const int oldSize = LC_GET_ONE_INT("Appearance", "EntityHandleSize", 8);
    LC_SET_ONE("Appearance", "CADSelectionStyle", true);
    LC_SET_ONE("Appearance", "EntityHandleSize", 8);
    for (int dpr : {1, 2}) {
        for (double zoom : {1.0, 4.0}) {
            INFO("DPR " << dpr << " zoom " << zoom);
            LC_GraphicViewport viewport;
            viewport.setSize(200, 200);
            viewport.justSetOffsetAndFactor(20, 20, zoom);
            QImage image(200 * dpr, 200 * dpr, QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(dpr);
            image.fill(Qt::black);
            RS_Painter painter(&image);
            painter.setViewPort(&viewport);
            Renderer renderer(&viewport, &image);
            renderer.loadSettings();
            RS_Line line(nullptr, {20, 20}, {30, 20});
            const RS_Pen original(RS_Color(Qt::yellow), RS2::Width22, RS2::DashLine);
            line.setPen(original);
            line.setSelectionFlag(true);
            renderer.setPenForEntity(&painter, &line, false);
            CHECK(painter.getPen().getLineType() == RS2::SolidLine);
            CHECK(painter.getPen().getScreenWidth() == 1.5);
            renderer.setPenForDraftEntity(&painter, &line, false);
            CHECK(painter.getPen().getLineType() == RS2::SolidLine);
            CHECK(painter.getPen().getScreenWidth() == 1.5);
            renderer.drawEntityReferencePoints(&painter, &line);
            const auto position = painter.toGui(line.getStartpoint());
            painter.end();
            const int x = qRound(position.x), y = qRound(position.y);
            CHECK(ink(image, x, y));
            CHECK(ink(image, x - 3, y));
            CHECK_FALSE(ink(image, x - 5, y));
            CHECK(line.getPen(false).getColor() == original.getColor());
            CHECK(line.getPen(false).getWidth() == original.getWidth());
            CHECK(line.getPen(false).getLineType() == original.getLineType());
        }
    }
    LC_SET_ONE("Appearance", "CADSelectionStyle", false);
    LC_GraphicViewport viewport;
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    RS_Painter painter(&image);
    painter.setViewPort(&viewport);
    Renderer renderer(&viewport, &image);
    renderer.loadSettings();
    RS_Line line(nullptr, {0, 0}, {10, 0});
    line.setSelectionFlag(true);
    renderer.setPenForEntity(&painter, &line, false);
    CHECK(painter.getPen().getLineType() == RS2::DashLineTiny);
    CHECK(painter.getPen().getScreenWidth() == 0.0);
    painter.end();
    LC_SET_ONE("Appearance", "CADSelectionStyle", oldStyle);
    LC_SET_ONE("Appearance", "EntityHandleSize", oldSize);
}
