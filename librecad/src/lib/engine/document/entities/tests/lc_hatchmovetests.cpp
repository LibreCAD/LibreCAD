/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

// Issue #3008. Once a hatch has cloned its contour it must not look at the
// entities the contour was picked from, nor at their layers: both can go while
// the hatch stays.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>

#include "lc_actiontestsupport.h"
#include "lc_settingguard.h"
#include "rs_block.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

using Catch::Matchers::WithinAbs;

const RS_Vector kCorners[] = {{0, 0}, {20, 0}, {20, 10}, {0, 10}};

// A drawing, and in it a 20 x 10 hatch as Draw Hatch builds one: a single loop
// of clones of the picked entities.
struct HatchFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    lc::test::SettingGuard m_patterns{RS_SETTINGS, "Paths", "Patterns"};
    RS_Graphic m_graphic;
    std::unique_ptr<RS_Hatch> m_hatch;

    HatchFixture() {
        m_patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
        m_graphic.initForNewDocument();
    }

    void hatchOver(const QList<RS_Entity*>& picked, const bool solid) {
        m_hatch = std::make_unique<RS_Hatch>(&m_graphic, RS_HatchData(solid, 1.0, 0.0, solid ? "SOLID" : "ANSI31"));
        auto* loop = new RS_EntityContainer(m_hatch.get());
        m_hatch->addEntity(loop);
        for (const RS_Entity* entity : picked) {
            RS_Entity* clone = entity->clone();
            clone->reparent(loop);
            loop->addEntity(clone);
        }
        m_hatch->update();
        REQUIRE(m_hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    }

    // The hatch is whole, lies at the offset, and its cached boundary names
    // nothing in the drawing.
    void checkMovedBy(const RS_Vector& offset) const {
        REQUIRE(m_hatch->getUpdateError() == RS_Hatch::HATCH_OK);
        CHECK_THAT(m_hatch->getTotalArea(), WithinAbs(200.0, 1e-8));
        CHECK_THAT(m_hatch->getMin().x, WithinAbs(offset.x, 1e-8));
        CHECK_THAT(m_hatch->getMin().y, WithinAbs(offset.y, 1e-8));
        CHECK_THAT(m_hatch->getMax().x, WithinAbs(offset.x + 20, 1e-8));
        CHECK_THAT(m_hatch->getMax().y, WithinAbs(offset.y + 10, 1e-8));
        REQUIRE(m_hatch->countAllLoops() == 1);
        for (const RS_Entity* edge : *m_hatch->getBoundaryContainer(0)) {
            CHECK(edge->getParent() == nullptr);
            CHECK(edge->getLayer(false) == nullptr);
        }
    }
};

} // namespace

// Moving an insert rebuilds its children, so the polyline that the cached
// boundary edges were cloned from is gone before the hatch updates.
TEST_CASE("A hatch moves with a block insert in its contour", "[hatch-move]") {
    const bool solid = GENERATE(false, true);
    HatchFixture f;
    auto* block = new RS_Block(&f.m_graphic, RS_BlockData("frame", {0, 0}, false));
    auto* outline = new RS_Polyline(block);
    for (const RS_Vector& corner : kCorners) {
        outline->addVertex(corner);
    }
    outline->setClosed(true, 0.0);
    block->addEntity(outline);
    REQUIRE(f.m_graphic.addBlock(block));
    RS_Insert insert{&f.m_graphic, RS_InsertData("frame", {0, 0}, {1, 1}, 0.0, 1, 1, {0, 0})};
    insert.update();
    f.hatchOver({&insert}, solid);

    f.m_hatch->move({7, -3});

    f.checkMovedBy({7, -3});
}

// The clones in a contour keep the layer of the entities picked for it. Deleting
// that layer does not reach inside a hatch, and freezing it hides the clones.
TEST_CASE("A hatch moves whatever becomes of the layer of its contour", "[hatch-move]") {
    const bool solid = GENERATE(false, true);
    const bool deleted = GENERATE(false, true);
    CAPTURE(solid, deleted);
    HatchFixture f;
    auto* layer = new RS_Layer("contour");
    f.m_graphic.addLayer(layer);
    RS_EntityContainer picked;
    for (int i = 0; i < 4; ++i) {
        auto* edge = new RS_Line(&picked, kCorners[i], kCorners[(i + 1) % 4]);
        edge->setLayer(layer);
        picked.addEntity(edge);
    }
    f.hatchOver(picked.getEntityList(), solid);
    if (deleted) {
        f.m_graphic.removeLayer(layer);
    }
    else {
        layer->freeze(true);
    }

    f.m_hatch->move({7, -3});
    f.m_hatch->calculateBorders(); // as a drawing does for its extent

    f.checkMovedBy({7, -3});
}
