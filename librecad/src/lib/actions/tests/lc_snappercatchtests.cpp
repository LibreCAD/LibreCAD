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

// Entity catching and snapping (issue #2804). A drawing measures only the entities
// whose cached borders can reach the cursor, and a typed catch never opens
// Text/MText glyph geometry. These cases pin down what must not change because of
// that: the resolve level still decides what a catch returns, drawing order and
// type order still settle ties, geometry that reaches past its borders or was
// edited in place is still found, and actions leave alone the geometry another
// entity generates.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "lc_action_modify_line_gap.h"
#include "lc_action_modify_round.h"
#include "lc_action_modify_trim_amount.h"
#include "lc_action_polyline_arcs_to_lines.h"
#include "lc_action_spline_from_polyline.h"
#include "lc_actiontestsupport.h"
#include "lc_containertraverser.h"
#include "lc_hyperbola.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_constructionline.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_spline.h"
#include "rs_text.h"

namespace {

using lc::test::ActionFixture;
using lc::test::eventAt;

RS_TextData textData(const RS_Vector& insertionPoint) {
    return {insertionPoint, insertionPoint, 1.0, 1.0,
            RS_TextData::VABaseline, RS_TextData::HALeft, RS_TextData::None,
            QStringLiteral("T"), QStringLiteral("standard"), 0.0, RS2::NoUpdate};
}

/// Exposes the snapper queries; Round only serves as a concrete snapper.
class SnapperProbe final : public LC_ActionModifyRound {
public:
    explicit SnapperProbe(LC_ActionContext* actionContext) : LC_ActionModifyRound(actionContext) {}

    using RS_Snapper::catchEntity;
    using RS_Snapper::getSnapRange;
    using RS_Snapper::m_keyEntity;
    using RS_Snapper::m_snapMode;

    RS_Vector onEntity(const RS_Vector& coord) {
        RS_Entity* entity = nullptr;
        return RS_Snapper::snapOnEntity(coord, &entity);
    }
};

class RoundProbe final : public LC_ActionModifyRound {
public:
    explicit RoundProbe(LC_ActionContext* actionContext) : LC_ActionModifyRound(actionContext) {}

    using LC_ActionModifyRound::onMouseLeftButtonRelease;
    using LC_ActionModifyRound::SetEntity1;
    using LC_ActionModifyRound::SetEntity2;
};

class ArcsToLinesProbe final : public LC_ActionPolylineArcsToLines {
public:
    explicit ArcsToLinesProbe(LC_ActionContext* actionContext) : LC_ActionPolylineArcsToLines(actionContext) {}

    using LC_ActionPolylineArcsToLines::onMouseLeftButtonRelease;
    using LC_ActionPolylineArcsToLines::onMouseMoveEvent;
    using LC_ActionPolylineArcsToLines::SetEntity;
    using RS_PreviewActionInterface::m_preview;
};

class SplineFromPolylineProbe final : public LC_ActionSplineFromPolyline {
public:
    explicit SplineFromPolylineProbe(LC_ActionContext* actionContext) : LC_ActionSplineFromPolyline(actionContext) {}

    using LC_ActionSplineFromPolyline::onMouseLeftButtonRelease;
    using LC_ActionSplineFromPolyline::SetEntity;
};

class TrimAmountProbe final : public LC_ActionModifyTrimAmount {
public:
    explicit TrimAmountProbe(LC_ActionContext* actionContext) : LC_ActionModifyTrimAmount(actionContext) {}

    using LC_ActionModifyTrimAmount::onMouseLeftButtonRelease;
    using LC_ActionModifyTrimAmount::ChooseTrimEntity;
};

class LineGapProbe final : public LC_ActionModifyLineGap {
public:
    explicit LineGapProbe(LC_ActionContext* actionContext) : LC_ActionModifyLineGap(actionContext) {}

    using LC_AbstractActionWithPreview::checkMayExpandEntity;
};

RS_Line* addLine(RS_Graphic& graphic, const RS_Vector& from, const RS_Vector& to) {
    auto* line = new RS_Line(&graphic, RS_LineData(from, to));
    graphic.addEntity(line);
    return line;
}

RS_Polyline* addPolyline(RS_Graphic& graphic) {
    auto* polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector{0.0, 0.0});
    polyline->addVertex(RS_Vector{100.0, 0.0});
    polyline->addVertex(RS_Vector{100.0, 100.0});
    graphic.addEntity(polyline);
    return polyline;
}

/// A polyline from (0, 0) to (100, 0) along a shallow arc, whose center lies about 500 away.
RS_Polyline* addShallowArcPolyline(RS_Graphic& graphic) {
    auto* polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector{0.0, 0.0}, 0.05);
    polyline->addVertex(RS_Vector{100.0, 0.0});
    graphic.addEntity(polyline);
    return polyline;
}

RS_Block* addBlock(RS_Graphic& graphic, const QString& name) {
    auto* block = new RS_Block(&graphic, RS_BlockData(name, RS_Vector(0.0, 0.0), false));
    graphic.addBlock(block);
    return block;
}

RS_Insert* addInsert(RS_Graphic& graphic, const QString& name, const RS_Vector& insertionPoint) {
    auto* insert = new RS_Insert(&graphic, RS_InsertData(name, insertionPoint, RS_Vector(1.0, 1.0), 0.0, 1, 1,
                                                         RS_Vector(0.0, 0.0)));
    graphic.addEntity(insert);
    insert->update();
    return insert;
}

RS_Hatch* addSolidSquare(RS_Graphic& graphic, const RS_Vector& corner, const double size) {
    auto* hatch = new RS_Hatch(&graphic, RS_HatchData(true, 1.0, 0.0, QStringLiteral("SOLID")));
    auto* loop = new RS_EntityContainer(hatch);
    const RS_Vector b = corner + RS_Vector(size, 0.0);
    const RS_Vector c = corner + RS_Vector(size, size);
    const RS_Vector d = corner + RS_Vector(0.0, size);
    loop->addEntity(new RS_Line(loop, RS_LineData(corner, b)));
    loop->addEntity(new RS_Line(loop, RS_LineData(b, c)));
    loop->addEntity(new RS_Line(loop, RS_LineData(c, d)));
    loop->addEntity(new RS_Line(loop, RS_LineData(d, corner)));
    hatch->addEntity(loop);
    graphic.addEntity(hatch);
    hatch->update();
    return hatch;
}

bool containsDeleted(const RS_EntityContainer& container) {
    return std::any_of(container.begin(), container.end(), [](const RS_Entity* entity) {
        return entity != nullptr && entity->isDeleted();
    });
}

/// The nearest entity as a plain container finds it, measuring every entity of the drawing.
RS_Entity* nearestMeasuringAll(const RS_EntityContainer& drawing, const RS_Vector& pos, double& distance,
                               const RS2::ResolveLevel level) {
    RS_EntityContainer all(nullptr, false);
    for (RS_Entity* entity : drawing) {
        all.push_back(entity);
    }
    distance = 0.0;
    return all.getNearestEntity(pos, &distance, level);
}

} // namespace

// ---------------------------------------------------------------------------
// What a catch returns

TEST_CASE("a typed catch at ResolveNone returns a top-level polyline, not one of its segments",
          "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addPolyline(f.m_graphic);

    CHECK(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::EntityPolyline, RS2::ResolveNone) == polyline);
    CHECK(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::EntityLine, RS2::ResolveNone) == nullptr);
}

TEST_CASE("a typed catch at ResolveAll still reaches polyline segments", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addPolyline(f.m_graphic);

    RS_Entity* segment = f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::EntityLine, RS2::ResolveAll);
    REQUIRE(segment != nullptr);
    CHECK(segment->getParent() == polyline);
    // at this level a polyline request accepts the segment through its parent
    CHECK(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::EntityPolyline, RS2::ResolveAll) == segment);
}

TEST_CASE("a typed catch at ResolveNone returns a block reference, not its contents", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("B"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("B"), RS_Vector(100.0, 0.0));
    REQUIRE(insert->count() == 1);

    CHECK(f.m_action->catchEntity(RS_Vector{105.0, 0.5}, RS2::EntityInsert, RS2::ResolveNone) == insert);
    CHECK(f.m_action->catchEntity(RS_Vector{105.0, 0.5}, RS2::EntityLine, RS2::ResolveNone) == nullptr);
}

TEST_CASE("a typed catch never returns Text or MText glyph geometry", "[snap][catch][text]") {
    ActionFixture<SnapperProbe> f;
    auto* text = new RS_Text(&f.m_graphic, textData({0.0, 0.0}));
    // stands in for glyph geometry
    text->addEntity(new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    text->forcedCalculateBorders();
    f.m_graphic.addEntity(text);
    auto* mtext = new RS_MText(&f.m_graphic, RS_MTextData{});
    mtext->addEntity(new RS_Line(mtext, RS_Vector{0.0, 50.0}, RS_Vector{10.0, 50.0}));
    mtext->forcedCalculateBorders();
    f.m_graphic.addEntity(mtext);

    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 0.5}, RS2::EntityLine, RS2::ResolveAll) == nullptr);
    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 0.5}, RS2::EntityText, RS2::ResolveAll) == text);
    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 50.5}, RS2::EntityLine, RS2::ResolveAll) == nullptr);
    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 50.5}, RS2::EntityMText, RS2::ResolveAll) == mtext);
}

TEST_CASE("an untyped catch at ResolveAllButTextImage passes over text to the entity behind it",
          "[snap][catch][text]") {
    ActionFixture<SnapperProbe> f;
    auto* text = new RS_Text(&f.m_graphic, textData({0.0, 0.0}));
    text->addEntity(new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    text->forcedCalculateBorders();
    f.m_graphic.addEntity(text);
    REQUIRE(f.m_action->catchEntity(RS_Vector{5.0, 0.5}, RS2::ResolveAllButTextImage) == nullptr);
    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 0.5}, RS2::ResolveNone) == text);

    RS_Line* line = addLine(f.m_graphic, {0.0, 3.0}, {10.0, 3.0});

    CHECK(f.m_action->catchEntity(RS_Vector{5.0, 0.5}, RS2::ResolveAllButTextImage) == line);
}

TEST_CASE("ResolveAllButTexts never descends into text geometry", "[snap][text]") {
    RS_EntityContainer container(nullptr);
    auto* text = new RS_Text(&container, textData({0.0, 0.0}));
    text->addEntity(new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    text->forcedCalculateBorders();
    container.addEntity(text);

    RS_Entity* nearest = nullptr;
    const double distance = container.getDistanceToPoint(RS_Vector{5.0, 0.0}, &nearest, RS2::ResolveAllButTexts);

    CHECK(distance == RS_MAXDOUBLE);
    CHECK(nearest == nullptr);
}

TEST_CASE("the traverser keeps text whole at ResolveAllButTextImage", "[container][traverser]") {
    RS_EntityContainer container(nullptr);
    auto* text = new RS_Text(&container, textData({0.0, 0.0}));
    auto* glyph = new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{1.0, 0.0});
    text->addEntity(glyph);
    container.addEntity(text);
    auto* polyline = new RS_Polyline(&container);
    polyline->addVertex(RS_Vector{0.0, 10.0});
    polyline->addVertex(RS_Vector{10.0, 10.0});
    container.addEntity(polyline);

    const std::vector<RS_Entity*> entities =
        lc::LC_ContainerTraverser{container, RS2::ResolveAllButTextImage}.entities();

    CHECK(std::find(entities.begin(), entities.end(), text) != entities.end());
    CHECK(std::find(entities.begin(), entities.end(), glyph) == entities.end());
    CHECK(std::find(entities.begin(), entities.end(), polyline) == entities.end());
}

TEST_CASE("a hidden polyline is not caught through segments that kept their visibility", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addPolyline(f.m_graphic);
    // DXF import can hide a polyline without hiding its segments
    polyline->RS_Entity::setVisible(false);
    REQUIRE(polyline->entityAt(0)->isVisible());

    CHECK(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::EntityLine, RS2::ResolveAll) == nullptr);
    CHECK(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::ResolveAll) == nullptr);
}

// ---------------------------------------------------------------------------
// Ties

TEST_CASE("overlapping hatches are caught in drawing order", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    addSolidSquare(f.m_graphic, RS_Vector(0.0, 0.0), 100.0);
    addSolidSquare(f.m_graphic, RS_Vector(10.0, 10.0), 100.0);
    const RS_Vector inside(50.0, 50.0);

    // the last entity is drawn on top, and ties go to it
    const RS_Entity* topmost = f.m_graphic.entityAt(static_cast<int>(f.m_graphic.count()) - 1);
    double distance = 0.0;
    REQUIRE(nearestMeasuringAll(f.m_graphic, inside, distance, RS2::ResolveNone) == topmost);

    CHECK(f.m_graphic.getNearestEntity(inside, &distance, RS2::ResolveNone) == topmost);
    CHECK(f.m_action->catchEntity(inside, RS2::ResolveNone) == topmost);
}

TEST_CASE("a typed catch settles an exact tie by the order of the requested types", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* circle = new RS_Circle(&f.m_graphic, RS_CircleData(RS_Vector(0.0, 0.0), 10.0));
    f.m_graphic.addEntity(circle);
    auto* arc = new RS_Arc(&f.m_graphic, RS_ArcData(RS_Vector(0.0, 0.0), 20.0, 0.0, M_PI / 2.0, false));
    f.m_graphic.addEntity(arc);
    // both are caught by their shared center, and the arc is drawn last
    const RS_Vector center(0.0, 0.0);

    CHECK(f.m_action->catchEntity(center, EntityTypeList{RS2::EntityArc, RS2::EntityCircle}, RS2::ResolveNone) == circle);
    CHECK(f.m_action->catchEntity(center, EntityTypeList{RS2::EntityCircle, RS2::EntityArc}, RS2::ResolveNone) == arc);
}

TEST_CASE("a drawing finds the nearest entity it would find by measuring every entity", "[snap][catch][container]") {
    ActionFixture<SnapperProbe> f;
    RS_Graphic& graphic = f.m_graphic;
    std::mt19937 random{2804};
    std::uniform_real_distribution<double> coordinate{-500.0, 500.0};
    std::uniform_real_distribution<double> size{1.0, 80.0};
    const auto point = [&] { return RS_Vector{coordinate(random), coordinate(random)}; };

    RS_Block* block = addBlock(graphic, QStringLiteral("K"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(20.0, 5.0))));
    block->addEntity(new RS_Arc(block, RS_ArcData(RS_Vector(0.0, 0.0), 15.0, 0.3, 2.0, false)));
    for (int i = 0; i < 350; ++i) {
        const RS_Vector p = point();
        const double width = size(random);
        const double height = size(random);
        switch (i % 7) {
            case 0:
                addLine(graphic, p, p + RS_Vector{width, height - 40.0});
                break;
            case 1:
                graphic.addEntity(new RS_Arc(&graphic, RS_ArcData(p, width, 0.2 * (i % 5), 1.0 + 0.3 * (i % 4), i % 2 == 0)));
                break;
            case 2:
                graphic.addEntity(new RS_Circle(&graphic, RS_CircleData(p, width)));
                break;
            case 3:
                graphic.addEntity(new RS_Point(&graphic, RS_PointData(p)));
                break;
            case 4: {
                auto* polyline = new RS_Polyline(&graphic);
                polyline->addVertex(p, 0.3);
                polyline->addVertex(p + RS_Vector{width, 0.0});
                polyline->addVertex(p + RS_Vector{width, height});
                graphic.addEntity(polyline);
                break;
            }
            case 5:
                addInsert(graphic, QStringLiteral("K"), p);
                break;
            default: {
                auto* text = new RS_Text(&graphic, textData(p));
                text->addEntity(new RS_Line(text, p, p + RS_Vector{width, 0.0}));
                text->forcedCalculateBorders();
                graphic.addEntity(text);
                break;
            }
        }
    }
    // exact ties: an entity drawn over a copy of itself, and a solid hatch; and a line with no end
    addLine(graphic, RS_Vector{0.0, 0.0}, RS_Vector{100.0, 0.0});
    addLine(graphic, RS_Vector{0.0, 0.0}, RS_Vector{100.0, 0.0});
    addSolidSquare(graphic, RS_Vector{-50.0, -50.0}, 30.0);
    graphic.addEntity(new RS_ConstructionLine(&graphic, RS_ConstructionLineData({0.0, 700.0}, {1.0, 700.0})));

    for (int i = 0; i < 300; ++i) {
        const RS_Vector pos = i < 20 ? RS_Vector{10.0 * i, 0.0} : RS_Vector{1.5 * coordinate(random), 1.5 * coordinate(random)};
        for (const RS2::ResolveLevel level : {RS2::ResolveNone, RS2::ResolveAllButTexts, RS2::ResolveAllButTextImage,
                                              RS2::ResolveAll}) {
            double expectedDistance = 0.0;
            const RS_Entity* expected = nearestMeasuringAll(graphic, pos, expectedDistance, level);
            double distance = 0.0;
            const RS_Entity* found = graphic.getNearestEntity(pos, &distance, level);

            INFO("position " << pos.x << ", " << pos.y << " level " << static_cast<int>(level));
            CHECK(found == expected);
            CHECK(distance == expectedDistance);
        }
    }
}

// ---------------------------------------------------------------------------
// Geometry that reaches past its borders

TEST_CASE("an arc is caught at its center", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* arc = new RS_Arc(&f.m_graphic, RS_ArcData(RS_Vector(0.0, 0.0), 100.0, M_PI / 6.0, M_PI / 3.0, false));
    arc->calculateBorders();
    f.m_graphic.addEntity(arc);
    // the borders of this arc stay far from its center
    REQUIRE(arc->getMin().x > 40.0);

    CHECK(f.m_action->catchEntity(RS_Vector(0.0, 0.0), RS2::ResolveNone) == arc);
    CHECK(f.m_action->catchEntity(RS_Vector(0.0, 0.0), RS2::EntityArc, RS2::ResolveNone) == arc);
}

TEST_CASE("the center of an arc inside a polyline picks nothing", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addShallowArcPolyline(f.m_graphic);
    REQUIRE(polyline->count() == 1);
    RS_Entity* segment = polyline->entityAt(0);
    REQUIRE(segment->rtti() == RS2::EntityArc);
    const RS_Vector center = segment->getCenter();
    REQUIRE(center.distanceTo(RS_Vector{50.0, 0.0}) > 400.0);

    // selection by center point is for entities of the drawing itself, so hover and the
    // context menu (QG_GraphicView::catchContextEntity) agree
    double distance = 0.0;
    f.m_graphic.getNearestEntity(center, &distance, RS2::ResolveNone);
    CHECK(distance > 400.0);
    CHECK(f.m_action->catchEntity(center, RS2::ResolveNone) == nullptr);
    CHECK(f.m_action->catchEntity(center, RS2::EntityArc, RS2::ResolveAll) == nullptr);
    // on the arc itself both are still caught
    CHECK(f.m_action->catchEntity(segment->getMiddlePoint(), RS2::ResolveNone) == polyline);
    CHECK(f.m_action->catchEntity(segment->getMiddlePoint(), RS2::EntityArc, RS2::ResolveAll) == segment);
}

TEST_CASE("construction lines are caught far from their definition points", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* constructionLine = new RS_ConstructionLine(&f.m_graphic, RS_ConstructionLineData({0.0, 0.0}, {1.0, 0.0}));
    f.m_graphic.addEntity(constructionLine);

    CHECK(f.m_action->catchEntity(RS_Vector{1000.0, 0.5}, RS2::ResolveNone) == constructionLine);
}

TEST_CASE("a line is caught along its extension once its layer becomes a construction layer",
          "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Layer* layer = f.m_graphic.getActiveLayer();
    REQUIRE(layer != nullptr);
    REQUIRE_FALSE(layer->isConstruction());
    RS_Line* line = addLine(f.m_graphic, {0.0, 0.0}, {1.0, 0.0});
    line->setLayer(layer);
    REQUIRE(f.m_action->catchEntity(RS_Vector{1000.0, 0.5}, RS2::ResolveNone) == nullptr);

    // the layer tree's edit dialog changes the layer directly, not through RS_Graphic
    REQUIRE(layer->setConstruction(true));
    f.m_graphic.getLayerList()->fireLayerEdited(nullptr);

    CHECK(f.m_action->catchEntity(RS_Vector{1000.0, 0.5}, RS2::ResolveNone) == line);
}

TEST_CASE("polylines and splines drawn on a construction layer are caught along their extensions",
          "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    REQUIRE(f.m_graphic.getActiveLayer()->setConstruction(true));
    // their segments take the active layer
    auto* polyline = new RS_Polyline(&f.m_graphic);
    polyline->addVertex(RS_Vector{0.0, 0.0});
    polyline->addVertex(RS_Vector{100.0, 0.0});
    f.m_graphic.addEntity(polyline);
    auto* spline = new RS_Spline(&f.m_graphic, RS_SplineData(3, false));
    for (const double x : {0.0, 10.0, 20.0, 30.0}) {
        spline->addControlPoint(RS_Vector(x, 500.0));
    }
    spline->update();
    f.m_graphic.addEntity(spline);
    REQUIRE(polyline->entityAt(0)->isConstruction());
    REQUIRE(spline->count() > 0);
    REQUIRE(spline->entityAt(0)->isConstruction());

    for (const RS_Vector& pos : {RS_Vector{1000.0, 0.5}, RS_Vector{1000.0, 500.5}}) {
        double distance = 0.0;
        RS_Entity* expected = nearestMeasuringAll(f.m_graphic, pos, distance, RS2::ResolveNone);
        REQUIRE(distance == Catch::Approx(0.5));
        CHECK(f.m_action->catchEntity(pos, RS2::ResolveNone) == expected);
    }
    CHECK(f.m_action->catchEntity(RS_Vector{1000.0, 0.5}, RS2::ResolveNone) == polyline);
    CHECK(f.m_action->catchEntity(RS_Vector{1000.0, 500.5}, RS2::ResolveNone) == spline);
}

TEST_CASE("a block reference holding a construction line is caught along it", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("X"));
    block->addEntity(new RS_ConstructionLine(block, RS_ConstructionLineData({0.0, 0.0}, {1.0, 0.0})));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("X"), RS_Vector(0.0, 0.0));
    REQUIRE(insert->count() == 1);

    CHECK(f.m_action->catchEntity(RS_Vector{1000.0, 0.5}, RS2::ResolveNone) == insert);
}

TEST_CASE("a rotated hyperbola arc is caught all along the curve", "[snap][catch][hyperbola]") {
    ActionFixture<SnapperProbe> f;
    const RS_Vector majorP = RS_Vector::polar(1000.0, RS_Math::deg2rad(40.0));
    auto* hyperbola = new LC_Hyperbola(&f.m_graphic, LC_HyperbolaData(RS_Vector(0.0, 0.0), majorP, 1.0, -2.0, 2.0, false));
    f.m_graphic.addEntity(hyperbola);

    for (int i = 0; i <= 40; ++i) {
        const double parameter = -2.0 + 4.0 * i / 40.0;
        const RS_Vector onCurve = hyperbola->getPoint(parameter, false);
        REQUIRE(onCurve.valid);
        INFO("parameter " << parameter);
        CHECK(f.m_action->catchEntity(onCurve, RS2::ResolveNone) == hyperbola);
    }
}

// ---------------------------------------------------------------------------
// Geometry edited in place

TEST_CASE("geometry added to a block reference by updateInserts is catchable", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("E"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("E"), RS_Vector(0.0, 0.0));
    REQUIRE(f.m_action->catchEntity(RS_Vector(5.0, 0.5), RS2::ResolveNone) == insert);

    // the block editor changes the definition; activating the drawing window refreshes the inserts
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 500.0), RS_Vector(10.0, 500.0))));
    f.m_graphic.updateInserts();

    CHECK(f.m_action->catchEntity(RS_Vector(5.0, 500.5), RS2::ResolveNone) == insert);
}

TEST_CASE("a polyline grown in place is catchable at its new segment", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addPolyline(f.m_graphic);
    REQUIRE(f.m_action->catchEntity(RS_Vector{50.0, 1.0}, RS2::ResolveNone) == polyline);

    // Draw > Polyline appends vertices to the polyline it has already added to the drawing
    polyline->addVertex(RS_Vector{100.0, 600.0});

    CHECK(f.m_action->catchEntity(RS_Vector{101.0, 400.0}, RS2::ResolveNone) == polyline);
}

TEST_CASE("a circle and a point moved in place are catchable where they went", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* circle = new RS_Circle(&f.m_graphic, RS_CircleData(RS_Vector(0.0, 0.0), 10.0));
    f.m_graphic.addEntity(circle);
    auto* point = new RS_Point(&f.m_graphic, RS_PointData(RS_Vector(0.0, 0.0)));
    f.m_graphic.addEntity(point);

    // plugins edit the entities they created in place (Plugin_Entity::updateData)
    circle->setCenter(RS_Vector(500.0, 0.0));
    circle->setRadius(20.0);
    point->setPos(RS_Vector(0.0, 700.0));

    CHECK(f.m_action->catchEntity(RS_Vector(520.5, 0.0), RS2::ResolveNone) == circle);
    CHECK(f.m_action->catchEntity(RS_Vector(0.5, 700.0), RS2::ResolveNone) == point);
}

TEST_CASE("arcs and elliptic arcs edited in place are catchable along their new shape", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* reversed = new RS_Arc(&f.m_graphic, RS_ArcData(RS_Vector(0.0, 0.0), 100.0, 0.0, M_PI / 2.0, false));
    f.m_graphic.addEntity(reversed);
    auto* replaced = new RS_Arc(&f.m_graphic, RS_ArcData(RS_Vector(0.0, 1000.0), 50.0, 0.0, M_PI / 2.0, false));
    f.m_graphic.addEntity(replaced);
    auto* ellipse = new RS_Ellipse(&f.m_graphic, RS_EllipseData{RS_Vector(0.0, -1000.0), RS_Vector(100.0, 0.0), 0.5, 0.0, M_PI / 2.0, false});
    f.m_graphic.addEntity(ellipse);

    reversed->setReversed(true);
    replaced->setData(RS_ArcData(RS_Vector(400.0, 1000.0), 50.0, M_PI, 1.5 * M_PI, false));
    ellipse->setReversed(true);

    // points on the part of the circle or ellipse each one gained
    CHECK(f.m_action->catchEntity(RS_Vector(-100.0, 0.0), RS2::ResolveNone) == reversed);
    CHECK(f.m_action->catchEntity(RS_Vector(400.0, 1000.0) + RS_Vector::polar(50.0, 1.25 * M_PI), RS2::ResolveNone) == replaced);
    CHECK(f.m_action->catchEntity(RS_Vector(-100.0, -1000.0), RS2::ResolveNone) == ellipse);
    // reversing keeps the start point at the start angle
    CHECK(reversed->getStartpoint().distanceTo(RS_Vector(100.0, 0.0)) == Catch::Approx(0.0).margin(1e-9));
}

TEST_CASE("a moved spline is caught where it is drawn", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    auto* spline = new RS_Spline(&f.m_graphic, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector(0.0, 0.0), RS_Vector(10.0, 20.0), RS_Vector(20.0, -20.0), RS_Vector(30.0, 0.0)}) {
        spline->addControlPoint(p);
    }
    f.m_graphic.addEntity(spline);

    RS_MoveData data;
    data.offset = RS_Vector(1000.0, 0.0);
    data.number = 1;
    f.m_graphic.undoableModify(f.m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) -> bool {
        RS_Modification::move(data, QList<RS_Entity*>{spline}, false, ctx);
        return true;
    });
    RS_Entity* moved = nullptr;
    for (RS_Entity* entity : f.m_graphic) {
        if (entity->rtti() == RS2::EntitySpline && !entity->isDeleted()) {
            moved = entity;
        }
    }
    REQUIRE(moved != nullptr);

    CHECK(f.m_action->catchEntity(RS_Vector(1000.0, 0.5), RS2::ResolveNone) == moved);
    CHECK(f.m_action->catchEntity(RS_Vector(0.0, 0.5), RS2::ResolveNone) == nullptr);
}

TEST_CASE("a block reference is caught on the major vertex of its ellipse", "[snap][catch]") {
    ActionFixture<SnapperProbe> f;
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("V"));
    block->addEntity(new RS_Ellipse(block, RS_EllipseData{RS_Vector(0.0, 0.0), RS_Vector(1000.0, 0.0), 0.5, 0.0, 0.0, false}));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("V"), RS_Vector(0.0, 0.0));
    REQUIRE(insert->count() == 1);

    // the ellipse is a child, so it is not picked by its center: the distance must come from the curve
    CHECK(f.m_action->catchEntity(RS_Vector(1000.0, 0.0), RS2::ResolveNone) == insert);
    CHECK(f.m_action->catchEntity(RS_Vector(-1000.0, 0.0), RS2::ResolveNone) == insert);
}

// ---------------------------------------------------------------------------
// On-entity snapping

TEST_CASE("on-entity snapping ignores text", "[snap][onentity][text]") {
    ActionFixture<SnapperProbe> f;
    f.m_action->m_snapMode.snapFree = true;
    auto* text = new RS_Text(&f.m_graphic, textData({0.0, 0.0}));
    text->addEntity(new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    text->forcedCalculateBorders();
    f.m_graphic.addEntity(text);
    addLine(f.m_graphic, {0.0, 2.0}, {10.0, 2.0});

    const RS_Vector snapped = f.m_action->onEntity(RS_Vector{5.0, 0.2});

    REQUIRE(snapped.valid);
    CHECK(snapped.y == Catch::Approx(2.0));
}

TEST_CASE("on-entity snapping reaches past the snap range only without free snapping", "[snap][onentity]") {
    ActionFixture<SnapperProbe> f;
    RS_Line* line = addLine(f.m_graphic, {0.0, 0.0}, {100.0, 0.0});
    const double range = f.m_action->getSnapRange();
    REQUIRE(range > 0.0);
    const RS_Vector near{50.0, 0.5 * range};
    const RS_Vector far{50.0, 3.0 * range};
    const RS_Vector foot{50.0, 0.0};

    f.m_action->m_snapMode.snapFree = true;
    CHECK(f.m_action->onEntity(near).distanceTo(foot) == Catch::Approx(0.0).margin(1e-9));
    CHECK(f.m_action->m_keyEntity == line);
    CHECK_FALSE(f.m_action->onEntity(far).valid);
    CHECK(f.m_action->m_keyEntity == nullptr);

    f.m_action->m_snapMode.snapFree = false;
    CHECK(f.m_action->onEntity(far).distanceTo(foot) == Catch::Approx(0.0).margin(1e-9));
    CHECK(f.m_action->m_keyEntity == line);
}

TEST_CASE("on-entity snapping keys on the polyline segment it lands on", "[snap][onentity]") {
    ActionFixture<SnapperProbe> f;
    RS_Polyline* polyline = addPolyline(f.m_graphic);

    for (const bool freeSnap : {true, false}) {
        f.m_action->m_snapMode.snapFree = freeSnap;
        const RS_Vector snapped = f.m_action->onEntity(RS_Vector{101.0, 50.0});

        INFO("free snap " << freeSnap);
        REQUIRE(snapped.valid);
        CHECK(snapped.distanceTo(RS_Vector{100.0, 50.0}) == Catch::Approx(0.0).margin(1e-9));
        CHECK(f.m_action->m_keyEntity == polyline->entityAt(1));
    }
}

TEST_CASE("on-entity snapping inside a solid hatch returns", "[snap][onentity][rs_hatch]") {
    // the hatch reports itself as nearest inside its fill; without the "en != this" guard in
    // RS_EntityContainer::getNearestPointOnEntity() this recurses until the stack overflows
    // (issue #2670), and hatch boundaries are no snap targets (issue #652)
    ActionFixture<SnapperProbe> f;
    addSolidSquare(f.m_graphic, RS_Vector(0.0, 0.0), 100.0);

    for (const bool freeSnap : {true, false}) {
        f.m_action->m_snapMode.snapFree = freeSnap;
        INFO("free snap " << freeSnap);
        CHECK_FALSE(f.m_action->onEntity(RS_Vector(50.0, 50.0)).valid);
    }
}

// ---------------------------------------------------------------------------
// Actions and generated geometry

TEST_CASE("only geometry of the drawing itself is editable", "[information][modify]") {
    ActionFixture<SnapperProbe> f;
    RS_Line* line = addLine(f.m_graphic, {0.0, 0.0}, {10.0, 0.0});
    RS_Polyline* polyline = addPolyline(f.m_graphic);
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("P"));
    auto* blockLine = new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
    block->addEntity(blockLine);
    auto* blockPolyline = new RS_Polyline(block);
    blockPolyline->addVertex(RS_Vector{0.0, 5.0});
    blockPolyline->addVertex(RS_Vector{10.0, 5.0});
    block->addEntity(blockPolyline);
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("P"), RS_Vector(200.0, 0.0));
    REQUIRE(insert->count() == 2);
    REQUIRE(insert->entityAt(1)->rtti() == RS2::EntityPolyline);
    RS_Entity* insertLine = insert->entityAt(0);
    RS_Entity* insertSegment = static_cast<RS_EntityContainer*>(insert->entityAt(1))->entityAt(0);
    RS_Hatch* hatch = addSolidSquare(f.m_graphic, RS_Vector(0.0, 300.0), 10.0);
    REQUIRE(hatch->entityAt(0)->isContainer());
    RS_Entity* boundary = static_cast<RS_EntityContainer*>(hatch->entityAt(0))->entityAt(0);

    CHECK(RS_Information::isEditable(line));
    CHECK(RS_Information::isEditable(polyline->entityAt(0)));
    CHECK(RS_Information::isEditable(blockLine));
    CHECK(RS_Information::isEditable(blockPolyline->entityAt(0)));
    CHECK_FALSE(RS_Information::isEditable(insertLine));
    CHECK_FALSE(RS_Information::isEditable(insertSegment));
    CHECK_FALSE(RS_Information::isEditable(boundary));
    CHECK_FALSE(RS_Information::isEditable(nullptr));

    CHECK(RS_Information::isTrimmable(line));
    CHECK(RS_Information::isTrimmable(polyline->entityAt(0)));
    CHECK_FALSE(RS_Information::isTrimmable(insertLine));
    CHECK_FALSE(RS_Information::isTrimmable(insertSegment));
    CHECK_FALSE(RS_Information::isTrimmable(boundary));
}

TEST_CASE("Fillet trims two lines of the drawing, but never a line inside a block reference",
          "[actions][modify][round]") {
    ActionFixture<RoundProbe> f;
    RS_Line* first = addLine(f.m_graphic, {0.0, 0.0}, {100.0, 0.0});
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("R"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 100.0))));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("R"), RS_Vector(100.0, 0.0));
    REQUIRE(insert->count() == 1);
    RS_Entity* child = insert->entityAt(0);
    f.m_action->setRadius(10.0);
    f.m_action->setTrim(true);
    const unsigned before = f.m_graphic.count();

    LC_MouseEvent pickFirst = eventAt(50.0, 0.5);
    f.m_action->onMouseLeftButtonRelease(RoundProbe::SetEntity1, &pickFirst);
    REQUIRE(f.m_action->getStatus() == RoundProbe::SetEntity2);
    LC_MouseEvent pickChild = eventAt(100.5, 50.0);
    f.m_action->onMouseLeftButtonRelease(RoundProbe::SetEntity2, &pickChild);

    CHECK_FALSE(child->isDeleted());
    CHECK_FALSE(first->isDeleted());
    CHECK(f.m_graphic.count() == before);

    // the same corner drawn as a line of the drawing is filleted
    RS_Line* second = addLine(f.m_graphic, {100.0, 0.0}, {100.0, 100.0});
    insert->setVisible(false);
    const unsigned beforeFillet = f.m_graphic.count();
    f.m_action->onMouseLeftButtonRelease(RoundProbe::SetEntity2, &pickChild);

    CHECK(first->isDeleted());
    CHECK(second->isDeleted());
    CHECK(f.m_graphic.count() > beforeFillet);
}

TEST_CASE("dividing actions leave lines inside a block reference whole", "[actions][modify]") {
    ActionFixture<LineGapProbe> f;
    RS_Line* line = addLine(f.m_graphic, {0.0, 0.0}, {10.0, 0.0});
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("D"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("D"), RS_Vector(0.0, 100.0));
    REQUIRE(insert->count() == 1);

    CHECK(f.m_action->checkMayExpandEntity(line, QString()));
    CHECK_FALSE(f.m_action->checkMayExpandEntity(insert->entityAt(0), QString()));
}

TEST_CASE("Arcs to Lines previews and converts a polyline caught on one of its segments",
          "[actions][polyline][snap]") {
    ActionFixture<ArcsToLinesProbe> f;
    RS_Polyline* polyline = addShallowArcPolyline(f.m_graphic);
    const unsigned before = f.m_graphic.count();
    const RS_Vector onArc = polyline->entityAt(0)->getMiddlePoint();
    LC_MouseEvent e = eventAt(onArc.x, onArc.y);

    f.m_action->onMouseMoveEvent(ArcsToLinesProbe::SetEntity, &e);
    CHECK(f.m_action->m_preview->count() > 0);

    f.m_action->onMouseLeftButtonRelease(ArcsToLinesProbe::SetEntity, &e);
    CHECK(polyline->isDeleted());
    CHECK(f.m_graphic.count() == before + 1);
}

TEST_CASE("Spline from Polyline converts a polyline caught on one of its segments", "[actions][polyline][snap]") {
    ActionFixture<SplineFromPolylineProbe> f;
    addPolyline(f.m_graphic);
    const unsigned before = f.m_graphic.count();
    LC_MouseEvent e = eventAt(50.0, 1.0);

    f.m_action->onMouseLeftButtonRelease(SplineFromPolylineProbe::SetEntity, &e);

    CHECK(f.m_graphic.count() == before + 1);
}

TEST_CASE("Lengthen changes a line of the drawing, not polyline segments or block reference contents",
          "[actions][modify][snap]") {
    ActionFixture<TrimAmountProbe> f;
    RS_Line* line = addLine(f.m_graphic, {0.0, 300.0}, {100.0, 300.0});
    RS_Polyline* polyline = addPolyline(f.m_graphic);
    RS_Block* block = addBlock(f.m_graphic, QStringLiteral("L"));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
    RS_Insert* insert = addInsert(f.m_graphic, QStringLiteral("L"), RS_Vector(200.0, 0.0));
    REQUIRE(insert->count() == 1);
    f.m_action->setDistance(1.0);
    const unsigned before = f.m_graphic.count();

    for (const RS_Vector& pick : {RS_Vector{50.0, 0.5}, RS_Vector{205.0, 0.5}}) {
        LC_MouseEvent e = eventAt(pick.x, pick.y);
        f.m_action->onMouseLeftButtonRelease(TrimAmountProbe::ChooseTrimEntity, &e);
    }

    CHECK_FALSE(containsDeleted(*polyline));
    CHECK_FALSE(containsDeleted(*insert));
    CHECK(f.m_graphic.count() == before);

    LC_MouseEvent e = eventAt(90.0, 300.5);
    f.m_action->onMouseLeftButtonRelease(TrimAmountProbe::ChooseTrimEntity, &e);

    CHECK(line->isDeleted());
    CHECK(f.m_graphic.count() == before + 1);
}
