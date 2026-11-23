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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QImage>
#include <QList>
#include <QString>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_selectedset.h"
#include "lc_selectedsetlistener.h"
#include "rs_block.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_spline.h"
#include "rs_vector.h"

namespace {

constexpr int kSize = 200;

class TestRenderer final : public LC_GraphicViewRenderer {
public:
    TestRenderer(LC_GraphicViewport* viewport, QImage* image)
        : LC_GraphicViewRenderer(viewport, image) {
        setBackground(RS_Color(Qt::white));
        m_colorSelectedEntity = RS_Color(Qt::black);
    }

    void drawGeometry(RS_Painter& painter) {
        m_renderBoundingClipRect = prepareBoundingClipRect();
        setupPainter(&painter);
        drawLayerEntities(&painter);
    }
};

struct SelectionObserver final : LC_SelectedSetListener {
    void selectionChanged() override { ++changes; }
    int changes = 0;
};

int geometryInk(LC_GraphicViewport* viewport) {
    QImage image(kSize, kSize, QImage::Format_RGB32);
    image.fill(Qt::white);
    {
        RS_Painter painter(&image);
        TestRenderer renderer(viewport, &image);
        renderer.drawGeometry(painter);
    }

    // The INSERT handle is at x=10. All spline geometry starts at x=20 or later.
    int painted = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 16; x < image.width(); ++x) {
            painted += image.pixelColor(x, y) != QColor(Qt::white);
        }
    }
    return painted;
}

} // namespace

TEST_CASE("A selected insert keeps its spline visible when rebuilt", "[insert][selection][render][3013]") {
    const int side = GENERATE(1, 3);
    const bool nested = GENERATE(false, true);
    CAPTURE(side, nested);
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    auto* viewport = view.getViewPort();
    viewport->setSize(kSize, kSize);
    viewport->setOffsetAndFactor(0, 0, 1.0);

    const QString partName = QStringLiteral("PART");
    auto* part = new RS_Block(&graphic, RS_BlockData(partName, RS_Vector{0, 0}, false));
    RS_SplineData splineData(3, false);
    splineData.controlPoints = {{10, 15}, {15, 32}, {30, 35}, {40, 15}};
    splineData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    splineData.weights.assign(splineData.controlPoints.size(), 1.0);
    auto* spline = new RS_Spline(part, splineData);
    spline->setPen(RS_Pen{RS_Color(Qt::black), RS2::Width00, RS2::SolidLine});
    part->addEntity(spline);
    graphic.addBlock(part);

    QString blockName = partName;
    if (nested) {
        blockName = QStringLiteral("OUTER");
        auto* outer = new RS_Block(&graphic, RS_BlockData(blockName, RS_Vector{0, 0}, false));
        outer->addEntity(new RS_Insert(outer, RS_InsertData(partName, RS_Vector{5, 5}, RS_Vector{1, 1},
                                                            0., 1, 1, RS_Vector{0, 0})));
        graphic.addBlock(outer);
    }

    auto* insert = new RS_Insert(&graphic, RS_InsertData(blockName, RS_Vector{10, 10}, RS_Vector{1, 1},
                                                         0., side, side, RS_Vector{50, 50}));
    graphic.addEntity(insert);
    graphic.updateInserts();

    const auto checkExpansion = [&](bool selected) {
        REQUIRE(insert->count() == side * side);
        for (const RS_Entity* entity : *insert) {
            REQUIRE(entity->rtti() == RS2::EntitySpline);
            CHECK(entity->getFlag(RS2::FlagSelected) == selected);
            const auto* expandedSpline = static_cast<const RS_Spline*>(entity);
            REQUIRE(expandedSpline->count() > 0);
            for (const RS_Entity* segment : *expandedSpline) {
                CHECK(segment->getFlag(RS2::FlagSelected) == selected);
            }
        }
    };

    checkExpansion(false);
    const int unselectedInk = geometryInk(viewport);
    REQUIRE(unselectedInk > 0);

    SelectionObserver observer;
    graphic.getSelection()->addListener(&observer);
    graphic.select(QList<RS_Entity*>{insert});
    REQUIRE(insert->isSelected());
    REQUIRE(observer.changes == 1);
    const int selectedInk = geometryInk(viewport);
    REQUIRE(selectedInk > 0);

    // Returning from the block editor calls updateInserts() before redrawing.
    for (int rebuild = 0; rebuild < 2; ++rebuild) {
        CAPTURE(rebuild);
        graphic.updateInserts();
        CHECK(observer.changes == 1);
        CHECK(insert->isSelected());
        QList<RS_Entity*> selected;
        REQUIRE(graphic.collectSelected(selected));
        CHECK(selected == QList<RS_Entity*>{insert});
        checkExpansion(true);
        CHECK(geometryInk(viewport) == selectedInk);
    }

    graphic.unselect(insert);
    graphic.updateInserts();
    CHECK(observer.changes == 2);
    CHECK_FALSE(insert->isSelected());
    QList<RS_Entity*> selected;
    CHECK_FALSE(graphic.collectSelected(selected));
    checkExpansion(false);
    CHECK(geometryInk(viewport) == unselectedInk);
}
