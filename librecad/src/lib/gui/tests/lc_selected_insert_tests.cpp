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

// Issue #3013: a selected insert vanished from the drawing the moment its expansion was rebuilt,
// which is what closing a block's editor window does to the window of the drawing it came from.
// The renderer draws a drawing in two passes, the unselected entities and then the selected ones,
// and takes an entity for a pass by its own selected flag, the children of a container included.
// The insert was selected and its rebuilt children were not, so neither pass drew them.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QImage>
#include <QList>
#include <QString>
#include <utility>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_selection.h"
#include "rs_vector.h"

namespace {

constexpr int kSize = 200;

RS_Pen blackPen() {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, RS2::SolidLine};
    pen.setScreenWidth(1.0);
    return pen;
}

/// draws a drawing in the two passes of LC_WidgetViewPortRenderer::drawLayerEntities
class TwoPassRenderer final : public LC_GraphicViewRenderer {
public:
    TwoPassRenderer(LC_GraphicViewport* viewport, QPaintDevice* device)
        : LC_GraphicViewRenderer(viewport, device) {
        m_colorSelectedEntity = RS_Color(Qt::black);
    }

    void draw(RS_Painter& painter, RS_Document& document) {
        m_renderBoundingClipRect = prepareBoundingClipRect();
        setupPainter(&painter);
        painter.setDrawSelectedOnly(false);
        doSetupBeforeContainerDraw();
        justDrawEntity(&painter, &document);

        QList<RS_Entity*> selected;
        if (document.collectSelected(selected)) {
            painter.setDrawSelectedOnly(true);
            doSetupBeforeContainerDraw();
            for (RS_Entity* e : std::as_const(selected)) {
                painter.drawEntity(e);
            }
        }
    }
};

/// a block of a line and a circle, inserted into a drawing, directly and through another block
struct Scene {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    RS_Insert* m_insert = nullptr;
    RS_Insert* m_nested = nullptr;

    explicit Scene(const int columns = 1, const int rows = 1) {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_view.getViewPort()->setSize(kSize, kSize);
        m_view.getViewPort()->setOffsetAndFactor(0, 0, 1.0);

        auto* part = new RS_Block(&m_graphic, RS_BlockData(QStringLiteral("PART"), RS_Vector{0, 0}, false));
        auto* line = new RS_Line(part, RS_LineData(RS_Vector{0, 0}, RS_Vector{30, 0}));
        line->setPen(blackPen());
        part->addEntity(line);
        auto* circle = new RS_Circle(part, RS_CircleData(RS_Vector{15, 15}, 10));
        circle->setPen(blackPen());
        part->addEntity(circle);
        m_graphic.addBlock(part);

        auto* outer = new RS_Block(&m_graphic, RS_BlockData(QStringLiteral("OUTER"), RS_Vector{0, 0}, false));
        outer->addEntity(new RS_Insert(outer, RS_InsertData(QStringLiteral("PART"), RS_Vector{5, 5}, RS_Vector{1, 1},
                                                            0., 1, 1, RS_Vector{0, 0})));
        m_graphic.addBlock(outer);

        m_insert = new RS_Insert(&m_graphic, RS_InsertData(QStringLiteral("PART"), RS_Vector{20, 20}, RS_Vector{1, 1},
                                                           0., columns, rows, RS_Vector{40, 40}));
        m_graphic.addEntity(m_insert);
        m_nested = new RS_Insert(&m_graphic, RS_InsertData(QStringLiteral("OUTER"), RS_Vector{120, 120}, RS_Vector{1, 1},
                                                           0., 1, 1, RS_Vector{0, 0}));
        m_graphic.addEntity(m_nested);
        m_graphic.updateInserts();
    }

    void select(RS_Entity* e) {
        RS_Selection(&m_graphic, m_view.getViewPort()).selectSingle(e);
    }

    /// the pixels the drawing paints, as the drawing widget would
    int ink() {
        QImage image(kSize, kSize, QImage::Format_RGB32);
        image.fill(Qt::white);
        {
            RS_Painter painter(&image);
            TwoPassRenderer renderer(m_view.getViewPort(), &image);
            renderer.setBackground(RS_Color(Qt::white));
            renderer.draw(painter, m_graphic);
            painter.end();
        }
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                painted += image.pixelColor(x, y) != QColor(Qt::white);
            }
        }
        return painted;
    }
};

/// how many children of an insert are selected, and how many it has
std::pair<int, int> selectedChildren(RS_Insert* insert) {
    int selected = 0;
    int all = 0;
    for (const RS_Entity* child : *insert) {
        ++all;
        selected += child->getFlag(RS2::FlagSelected) ? 1 : 0;
    }
    return {selected, all};
}

} // namespace

TEST_CASE("A selected insert is still drawn after its expansion is rebuilt", "[insert][selection][render][3013]") {
    const int columns = GENERATE(1, 3);
    INFO("columns " << columns);
    Scene scene(columns, columns);
    const bool nested = GENERATE(false, true);
    RS_Insert* insert = nested ? scene.m_nested : scene.m_insert;
    INFO((nested ? "an insert of a block that holds an insert" : "an insert of a block of entities"));

    const int unselected = scene.ink();
    REQUIRE(unselected > 0);

    scene.select(insert);
    REQUIRE(insert->isSelected());
    const int selected = scene.ink();
    CHECK(selected > 0);

    // the window of the drawing is activated again when a block editor closes, and other
    // commands do it as well: the insert is the same one, so it must paint the same
    scene.m_graphic.updateInserts();
    CHECK(insert->isSelected());
    CHECK(scene.ink() == selected);
}

TEST_CASE("The expansion of an insert is selected as the insert is", "[insert][selection][3013]") {
    const int columns = GENERATE(1, 3);
    INFO("columns " << columns);
    Scene scene(columns, columns);
    const bool nested = GENERATE(false, true);
    RS_Insert* insert = nested ? scene.m_nested : scene.m_insert;
    INFO((nested ? "an insert of a block that holds an insert" : "an insert of a block of entities"));

    SECTION("not selected") {
        scene.m_graphic.updateInserts();
        const auto [selected, all] = selectedChildren(insert);
        CHECK(all > 0);
        CHECK(selected == 0);
    }

    SECTION("selected") {
        scene.select(insert);
        REQUIRE(insert->isSelected());
        scene.m_graphic.updateInserts();
        const auto [selected, all] = selectedChildren(insert);
        CHECK(all > 0);
        CHECK(selected == all);
    }

    SECTION("selected, then not selected") {
        scene.select(insert);
        REQUIRE(insert->isSelected());
        RS_Selection(&scene.m_graphic, scene.m_view.getViewPort()).selectAll(false);
        REQUIRE_FALSE(insert->isSelected());
        scene.m_graphic.updateInserts();
        const auto [selected, all] = selectedChildren(insert);
        CHECK(all > 0);
        CHECK(selected == 0);
        // and it is drawn, with the unselected entities
        CHECK(scene.ink() > 0);
    }
}
