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

// LC_LayersExporter builds one new RS_Graphic per exported document and hands
// it back in LC_LayerExportData. The export data owns that drawing: dropping it
// frees everything the exporter copied, and nothing of the source drawing,
// whose layers, blocks, UCSs, named views and graphic view the copies came from.

#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "lc_actiontestsupport.h"
#include "lc_layersexporter.h"
#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "lc_view.h"
#include "lc_viewslist.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {

// Counts live instances, clones included, so a test can tell whether the
// export drawings freed the entities copied into them.
class CountedLine final : public RS_Line {
public:
    CountedLine(RS_EntityContainer* parent, const RS_Vector& start, const RS_Vector& end, int& liveCount)
        : RS_Line(parent, start, end), m_liveCount{liveCount} {
        ++m_liveCount;
    }
    CountedLine(const CountedLine& other) : RS_Line(other), m_liveCount{other.m_liveCount} {
        ++m_liveCount;
    }
    ~CountedLine() override { --m_liveCount; }

    RS_Entity* clone() const override { return new CountedLine(*this); }

private:
    int& m_liveCount;
};

// A drawing shown in a headless view, with an entity on each of three layers,
// an insert of a block, a UCS and a named view that uses a UCS.
struct SourceDrawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    int m_liveLines = 0;
    RS_Layer* m_layer0 = nullptr;
    RS_Layer* m_layerA = nullptr;
    RS_Layer* m_layerB = nullptr;
    RS_Block* m_block = nullptr;

    SourceDrawing() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_graphic.setGraphicView(&m_view);

        m_layer0 = m_graphic.findLayer("0");
        m_layerA = new RS_Layer("A");
        m_layerB = new RS_Layer("B");
        m_graphic.addLayer(m_layerA);
        m_graphic.addLayer(m_layerB);

        addLine(m_layer0, 0.0);
        addLine(m_layerA, 10.0);
        addLine(m_layerB, 20.0);

        // Blocks are copied into each export drawing, and RS_BlockList does not
        // free its blocks, so the block holds an uncounted line.
        m_block = new RS_Block(&m_graphic, RS_BlockData("BLK", RS_Vector(0.0, 0.0), false));
        m_block->addEntity(new RS_Line(m_block, RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0)));
        m_graphic.addBlock(m_block);
        auto* insert = new RS_Insert(&m_graphic, RS_InsertData("BLK", RS_Vector(30.0, 0.0), RS_Vector(1.0, 1.0),
                                                               0.0, 1, 1, RS_Vector(0.0, 0.0)));
        insert->setLayer(m_layerA);
        m_graphic.addEntity(insert);
        insert->update();

        auto* ucs = new LC_UCS("SITE");
        ucs->setOrigin(RS_Vector(5.0, 5.0));
        ucs->setXAxis(RS_Vector(0.0, 1.0));
        ucs->setYAxis(RS_Vector(-1.0, 0.0));
        m_graphic.addUCS(ucs);

        auto* view = new LC_View("PLAN");
        view->setCenter(RS_Vector(15.0, 0.0));
        view->setSize(RS_Vector(40.0, 20.0));
        auto* viewUCS = new LC_UCS("SITE");
        viewUCS->setOrigin(RS_Vector(5.0, 5.0));
        viewUCS->setXAxis(RS_Vector(0.0, 1.0));
        viewUCS->setYAxis(RS_Vector(-1.0, 0.0));
        view->setUCS(viewUCS);
        m_graphic.addNamedView(view);
    }

    void addLine(RS_Layer* layer, const double y) {
        auto* line = new CountedLine(&m_graphic, RS_Vector(0.0, y), RS_Vector(5.0, y), m_liveLines);
        line->setLayer(layer);
        m_graphic.addEntity(line);
    }

    LC_LayersExportOptions options(const bool separateDocuments, const bool originalLayers) const {
        LC_LayersExportOptions result;
        result.createSeparateDocumentPerLayer = separateDocuments;
        result.putEntitiesToOriginalLayer = originalLayers;
        result.exportUcSs = true;
        result.exportNamedViews = true;
        result.layers = {m_layer0, m_layerA, m_layerB};
        return result;
    }
};

unsigned countLines(RS_Graphic* graphic) {
    unsigned n = 0;
    for (const RS_Entity* e : *graphic) {
        n += e != nullptr && e->rtti() == RS2::EntityLine ? 1 : 0;
    }
    return n;
}

} // namespace

TEST_CASE("Dropping layer export data frees the exported drawings", "[layers][export][ownership]") {
    const bool separateDocuments = GENERATE(false, true);
    const bool originalLayers = GENERATE(false, true);
    CAPTURE(separateDocuments, originalLayers);

    SourceDrawing source;
    REQUIRE(source.m_liveLines == 3);
    const LC_LayersExportOptions options = source.options(separateDocuments, originalLayers);

    {
        std::vector<LC_LayerExportData> results;
        LC_LayersExporter().exportLayers(&options, &source.m_graphic, results);

        REQUIRE(results.size() == (separateDocuments ? 3u : 1u));
        unsigned exportedLines = 0;
        for (const auto& result : results) {
            REQUIRE(result.graphic != nullptr);
            CHECK(&*result.graphic != &source.m_graphic);
            exportedLines += countLines(&*result.graphic);
            CHECK(result.graphic->getViewList()->count() == 1);
        }
        CHECK(exportedLines == 3);
        CHECK(source.m_liveLines == 6);
    }

    // Every copy is gone once the export data is.
    CHECK(source.m_liveLines == 3);
}

TEST_CASE("Freeing exported drawings leaves the source drawing intact", "[layers][export][ownership]") {
    const bool separateDocuments = GENERATE(false, true);
    CAPTURE(separateDocuments);

    SourceDrawing source;
    const LC_LayersExportOptions options = source.options(separateDocuments, true);
    const unsigned entityCount = source.m_graphic.count();
    const unsigned layerCount = source.m_graphic.getLayerList()->count();
    const unsigned ucsCount = source.m_graphic.getUCSList()->count();

    {
        std::vector<LC_LayerExportData> results;
        LC_LayersExporter().exportLayers(&options, &source.m_graphic, results);
        // The export drawings point at the source graphic view (see the
        // exporter's "dependency" fixme); freeing them must not touch it.
        for (const auto& result : results) {
            CHECK(result.graphic->getGraphicView() == &source.m_view);
        }
    }

    CHECK(source.m_graphic.count() == entityCount);
    CHECK(source.m_graphic.getLayerList()->count() == layerCount);
    CHECK(source.m_graphic.getUCSList()->count() == ucsCount);
    CHECK(source.m_graphic.getUCSList()->find("SITE") != nullptr);
    CHECK(source.m_graphic.getGraphicView() == &source.m_view);

    for (RS_Entity* e : source.m_graphic) {
        const RS_Layer* layer = e->getLayer();
        CHECK((layer == source.m_layer0 || layer == source.m_layerA || layer == source.m_layerB));
        CHECK(e->getParent() == &source.m_graphic);
    }

    REQUIRE(source.m_graphic.findBlock("BLK") == source.m_block);
    CHECK(source.m_block->count() == 1);

    LC_ViewList* views = source.m_graphic.getViewList();
    REQUIRE(views->count() == 1);
    const LC_View* view = views->at(0);
    CHECK(view->getName() == "PLAN");
    REQUIRE(view->getUCS() != nullptr);
    CHECK(view->getUCS()->getName() == "SITE");
    CHECK(view->getUCS()->getOrigin() == RS_Vector(5.0, 5.0));
}
