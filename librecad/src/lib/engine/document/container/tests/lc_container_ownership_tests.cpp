/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

// Entities that change owner: out of a container, into a document, through
// the plugin API. Each case used to free or add an entity one time too many;
// they matter most under AddressSanitizer.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "doc_plugin_interface.h"
#include "lc_actiondrawdual.h"
#include "lc_actiontestsupport.h"
#include "lc_dimstyle.h"
#include "lc_dimstyleslist.h"
#include "lc_hyperbola.h"
#include "rs_circle.h"
#include "rs_dimlinear.h"
#include "rs_line.h"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/allocator_interface.h>
#define LC_TEST_HAS_ALLOCATOR_STATS 1
#endif
#endif

namespace {

// A drawing in a headless view, with an action context, but no action.
struct DrawingFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;

    DrawingFixture() {
        m_graphic.initForNewDocument();
        m_graphic.onLoadingCompleted();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }

    int occurrences(const RS_Entity* entity) const {
        int n = 0;
        for (const RS_Entity* e : m_graphic) {
            n += e == entity ? 1 : 0;
        }
        return n;
    }
};

class DualProbe final : public LC_ActionDrawDual {
public:
    using LC_ActionDrawDual::LC_ActionDrawDual;
    using LC_ActionPreSelectionAwareBase::m_selectedEntities;
};

} // namespace

TEST_CASE("takeEntities hands a container's children over without deleting them", "[container][ownership]") {
    lc::test::application();
    RS_EntityContainer container(nullptr, true);
    for (int i = 0; i < 3; ++i) {
        container.addEntity(new RS_Line(&container, RS_LineData(RS_Vector{0, double(i)}, RS_Vector{1, double(i)})));
    }

    auto taken = container.takeEntities();

    CHECK(container.count() == 0);
    REQUIRE(taken.size() == 3);
    for (const auto& entity : taken) {
        CHECK(entity->getParent() == nullptr);
        CHECK(entity->getLength() == 1.0);
    }
}

TEST_CASE("Draw Dual of a circle about an outside point adds both hyperbola branches", "[container][ownership][dual]") {
    DrawingFixture f;
    auto* circle = new RS_Circle(&f.m_graphic, RS_CircleData(RS_Vector{0, 0}, 5.0));
    f.m_graphic.addEntity(circle);

    DualProbe action(&f.m_context);
    action.m_selectedEntities = {circle};
    action.onCoordinateEvent(DualProbe::ChooseCenter, false, RS_Vector{20, 0});

    int hyperbolas = 0;
    for (RS_Entity* e : f.m_graphic) {
        if (e->rtti() == RS2::EntityHyperbola && !e->isDeleted()) {
            ++hyperbolas;
            CHECK(e->getParent() == &f.m_graphic);
            CHECK(static_cast<LC_Hyperbola*>(e)->isValid());
        }
    }
    CHECK(hyperbolas == 2);
}

TEST_CASE("A plugin edit adds the edited entity once", "[plugins][ownership]") {
    for (const auto how : {DPI::DELETE_ORIGINAL, DPI::KEEP_ORIGINAL}) {
        auto f = std::make_unique<DrawingFixture>();
        auto* line = new RS_Line(&f->m_graphic, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 1}));
        f->m_graphic.addEntity(line);
        RS_Entity* moved = line->clone();
        moved->move(RS_Vector{5, 0});

        const Doc_plugin_interface plugin(&f->m_context, nullptr);
        CHECK(plugin.addToUndo(line, moved, how));

        CHECK(f->occurrences(moved) == 1);
        CHECK(f->occurrences(line) == 1);
        CHECK(line->isDeleted() == (how == DPI::DELETE_ORIGINAL));
        f.reset(); // closing the drawing frees each entity once
    }
}

TEST_CASE("Updating a dimension with inside-horizontal text leaves nothing behind", "[dimension][ownership]") {
    DrawingFixture f;
    std::unique_ptr<LC_DimStyle> style{f.m_graphic.getDimStyleList()->getFallbackDimStyleFromVars()->getCopy()};
    style->text()->setOrientationInside(LC_DimStyle::Text::TextOrientationPolicy::DRAW_HORIZONTALLY);
    // RS_DimensionData takes ownership of the override.
    const RS_DimensionData data(RS_Vector{0, 10}, RS_Vector{5, 10}, RS_MTextData::VAMiddle, RS_MTextData::HACenter,
                                RS_MTextData::Exact, 1.0, "", "Standard", 0.0, 0.0, true, style.release(), false,
                                false);
    auto* dimension = new RS_DimLinear(&f.m_graphic, data, RS_DimLinearData(RS_Vector{0, 0}, RS_Vector{10, 0}, 0.0, 0.0));
    f.m_graphic.addEntity(dimension);
    dimension->update();
    REQUIRE(dimension->count() > 0);

#ifdef LC_TEST_HAS_ALLOCATOR_STATS
    const std::size_t before = __sanitizer_get_current_allocated_bytes();
#endif
    for (int i = 0; i < 50; ++i) {
        dimension->update();
    }
#ifdef LC_TEST_HAS_ALLOCATOR_STATS
    CHECK(__sanitizer_get_current_allocated_bytes() <= before);
#endif
    CHECK(dimension->count() > 0);
}
