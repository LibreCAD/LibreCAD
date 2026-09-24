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

// What a copy of a container is: a value copy or clone shares nothing with
// the original and owns its own children.

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <set>
#include <type_traits>

#include "lc_actiontestsupport.h"
#include "lc_documentinvariants.h"
#include "rs_block.h"
#include "rs_fontlist.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_spline.h"
#include "rs_system.h"
#include "rs_text.h"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/allocator_interface.h>
#define LC_TEST_HAS_ALLOCATOR_STATS 1
#endif
#endif

static_assert(!std::is_copy_assignable_v<RS_EntityContainer>);
static_assert(!std::is_move_constructible_v<RS_EntityContainer>);
static_assert(!std::is_copy_assignable_v<RS_Polyline>);
static_assert(std::is_copy_constructible_v<RS_Polyline>);

namespace {

struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;

    Drawing() {
        m_graphic.initForNewDocument();
        m_graphic.onLoadingCompleted();
        m_view.setDocument(&m_graphic);
    }
};

// Fonts from the source tree, so that texts have glyphs.
struct SourceFonts {
    SourceFonts() {
        lc::test::application();
        static const bool systemReady = [] {
            RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
            return true;
        }();
        (void)systemReady;
        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Fonts", "");
        LC_SET("Fonts", QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }
    ~SourceFonts() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Fonts", m_previous);
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }
    QString m_previous;
};

void collectDescendants(const RS_Entity* e, std::set<const RS_Entity*>& out) {
    out.insert(e);
    if (e->isContainer()) {
        for (const RS_Entity* child : *static_cast<const RS_EntityContainer*>(e)) {
            if (child != nullptr) {
                collectDescendants(child, out);
            }
        }
    }
}

// Reads every descendant: under ASan this fails if any of them was freed.
double touch(const RS_Entity* e) {
    double sum = e->getMin().x + e->getMax().x;
    if (e->isContainer()) {
        for (const RS_Entity* child : *static_cast<const RS_EntityContainer*>(e)) {
            if (child != nullptr) {
                sum += touch(child);
            }
        }
    }
    return sum;
}

/**
 * A clone of original shares no entity with it, its owned children name
 * their container, and it survives deleting the original.
 */
void checkIndependentClone(std::unique_ptr<RS_Entity> original) {
    INFO("rtti " << static_cast<int>(original->rtti()));
    std::unique_ptr<RS_Entity> clone{original->clone()};
    std::set<const RS_Entity*> originals;
    std::set<const RS_Entity*> clones;
    collectDescendants(original.get(), originals);
    collectDescendants(clone.get(), clones);
    for (const RS_Entity* e : clones) {
        CHECK(originals.count(e) == 0);
    }
    CHECK(clones.size() == originals.size());
    if (clone->isContainer()) {
        CHECK(lc::test::containerProblems(*static_cast<RS_EntityContainer*>(clone.get())).isEmpty());
    }
    const double before = touch(clone.get());
    original.reset();
    CHECK(touch(clone.get()) == before);
}

#ifdef LC_TEST_HAS_ALLOCATOR_STATS
void checkNoBytesLeftPerClone(const RS_Entity& e) {
    INFO("rtti " << static_cast<int>(e.rtti()));
    delete e.clone(); // lazily built caches, fonts
    const std::size_t before = __sanitizer_get_current_allocated_bytes();
    for (int i = 0; i < 20; ++i) {
        delete e.clone();
    }
    CHECK(__sanitizer_get_current_allocated_bytes() <= before);
}
#endif

RS_Polyline* closedTriangle(RS_EntityContainer* parent) {
    auto* p = new RS_Polyline(parent);
    p->addVertex(RS_Vector{0, 0});
    p->addVertex(RS_Vector{10, 0});
    p->addVertex(RS_Vector{10, 10});
    p->setClosed(true);
    p->endPolyline();
    return p;
}

RS_Hatch* squareHatch(RS_EntityContainer* parent) {
    auto* hatch = new RS_Hatch(parent, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector{0, 0}, RS_Vector{10, 0})));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector{10, 0}, RS_Vector{10, 10})));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector{10, 10}, RS_Vector{0, 10})));
    loop->addEntity(new RS_Line(loop, RS_LineData(RS_Vector{0, 10}, RS_Vector{0, 0})));
    hatch->addEntity(loop);
    hatch->update();
    return hatch;
}

// One entity of each container type, built in the drawing but not added to it.
std::vector<std::function<RS_Entity*(RS_Graphic&)>> containerMakers() {
    return {
        [](RS_Graphic& g) -> RS_Entity* {
            auto* c = new RS_EntityContainer(&g, true);
            auto* inner = new RS_EntityContainer(c, true);
            inner->addEntity(new RS_Line(inner, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
            c->addEntity(inner);
            c->addEntity(new RS_Line(c, RS_LineData(RS_Vector{0, 1}, RS_Vector{1, 1})));
            return c;
        },
        [](RS_Graphic& g) -> RS_Entity* { return closedTriangle(&g); },
        [](RS_Graphic& g) -> RS_Entity* {
            auto* s = new RS_Spline(&g, RS_SplineData(3, false));
            for (int i = 0; i < 6; ++i) {
                s->addControlPoint(RS_Vector{double(i), double(i * i % 5)});
            }
            s->update();
            return s;
        },
        [](RS_Graphic& g) -> RS_Entity* {
            auto* i = new RS_Insert(&g, RS_InsertData("B1", RS_Vector{20, 0}, RS_Vector{1, 1}, 0.0, 1, 1, RS_Vector{0, 0}));
            i->update();
            return i;
        },
        [](RS_Graphic& g) -> RS_Entity* { return squareHatch(&g); },
        [](RS_Graphic& g) -> RS_Entity* {
            const RS_TextData data(RS_Vector{0, 0}, RS_Vector{0, 0}, 2.5, 1.0, RS_TextData::VABaseline,
                                   RS_TextData::HALeft, RS_TextData::None, "Copy", "standard", 0.0, RS2::Update);
            return new RS_Text(&g, data);
        },
        [](RS_Graphic& g) -> RS_Entity* {
            const RS_MTextData data(RS_Vector{0, 0}, 2.5, 50.0, RS_MTextData::VATop, RS_MTextData::HALeft,
                                    RS_MTextData::LeftToRight, RS_MTextData::Exact, 1.0, "one\\Ptwo", "standard", 0.0);
            return new RS_MText(&g, data);
        },
    };
}

void addBlockB1(RS_Graphic& g) {
    auto* block = new RS_Block(&g, RS_BlockData("B1", RS_Vector{0, 0}, false));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector{0, 0}, RS_Vector{5, 5})));
    auto* nested = closedTriangle(block);
    block->addEntity(nested);
    g.addBlock(block);
}

} // namespace

TEST_CASE("A clone of any container shares nothing with the original", "[copy][container]") {
    SourceFonts fonts;
    Drawing d;
    addBlockB1(d.m_graphic);
    for (const auto& make : containerMakers()) {
        checkIndependentClone(std::unique_ptr<RS_Entity>{make(d.m_graphic)});
    }
    checkIndependentClone(std::unique_ptr<RS_Entity>{d.m_graphic.findBlock("B1")->clone()});
}

#ifdef LC_TEST_HAS_ALLOCATOR_STATS
TEST_CASE("Cloning a container leaves nothing behind", "[copy][container][leak]") {
    SourceFonts fonts;
    Drawing d;
    addBlockB1(d.m_graphic);
    for (const auto& make : containerMakers()) {
        const std::unique_ptr<RS_Entity> e{make(d.m_graphic)};
        checkNoBytesLeftPerClone(*e);
    }
    checkNoBytesLeftPerClone(*d.m_graphic.findBlock("B1"));
    const std::unique_ptr<RS_Entity> plain{containerMakers().front()(d.m_graphic)};
    delete plain->cloneProxy();
    const std::size_t before = __sanitizer_get_current_allocated_bytes();
    delete plain->cloneProxy();
    CHECK(__sanitizer_get_current_allocated_bytes() <= before);
}
#endif

TEST_CASE("A value copy of a container is a deep copy", "[copy][container]") {
    Drawing d;
    const std::unique_ptr<RS_Polyline> original{closedTriangle(&d.m_graphic)};
    auto copy = std::make_unique<RS_Polyline>(*original);
    REQUIRE(copy->count() == original->count());
    CHECK(copy->entityAt(0) != original->entityAt(0));
    CHECK(copy->entityAt(0)->getParent() == copy.get());
    copy.reset();
    CHECK(original->entityAt(0)->getLength() == 10.0);
}

TEST_CASE("A cloned closed polyline closes itself, not the original", "[copy][polyline]") {
    Drawing d;
    const std::unique_ptr<RS_Polyline> original{closedTriangle(&d.m_graphic)};
    const std::unique_ptr<RS_Polyline> fresh{closedTriangle(nullptr)};
    const std::unique_ptr<RS_Polyline> clone{static_cast<RS_Polyline*>(original->clone())};
    fresh->addVertex(RS_Vector{0, 10});
    clone->addVertex(RS_Vector{0, 10});
    CHECK(clone->count() == fresh->count());
    CHECK(original->count() == 3);
}

TEST_CASE("Reparenting a container keeps its children as its own", "[copy][container]") {
    Drawing d;
    RS_Hatch* hatch = squareHatch(&d.m_graphic);
    RS_EntityContainer other(nullptr, true);
    hatch->reparent(&other);
    CHECK(hatch->getParent() == &other);
    CHECK(lc::test::containerProblems(*hatch).isEmpty());
    delete hatch;
}

TEST_CASE("Adding a plain container to a drawing adds only the container", "[copy][container]") {
    auto d = std::make_unique<Drawing>();
    auto* container = new RS_EntityContainer(&d->m_graphic, true);
    auto* line = new RS_Line(container, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0}));
    container->addEntity(line);
    d->m_graphic.addEntity(container);
    CHECK(d->m_graphic.count() == 1);
    CHECK(line->getParent() == container);
    d.reset(); // each entity is freed once
}
