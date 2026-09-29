/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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

// Issue #2957: a block instance keeps its transform (angle, scale, mirror, array) through
// copy and paste, so that what is pasted differs from what was copied by a translation
// only (or by the paste's own rotation and scale, when the paste is asked for them).

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include "lc_actiontestsupport.h"
#include "lc_copyutils.h"
#include "lc_documentinvariants.h"
#include "rs_block.h"
#include "rs_clipboard.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"

namespace {

struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    LC_ActionContext m_context;

    Drawing() {
        m_graphic.initForNewDocument();
        m_graphic.onLoadingCompleted();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
    }

    /// an "L" (no symmetry, so a wrong angle, scale or mirror shows in its geometry)
    RS_Block* addBlock(const QString& name, const std::function<void(RS_Block&)>& fill) {
        auto* block = new RS_Block(&m_graphic, RS_BlockData(name, RS_Vector{0, 0}, false));
        m_graphic.addBlock(block);
        fill(*block);
        return block;
    }

    RS_Block* addLBlock(const QString& name) {
        return addBlock(name, [](RS_Block& block) {
            block.addEntity(new RS_Line(&block, RS_LineData(RS_Vector{0, 0}, RS_Vector{10, 0})));
            block.addEntity(new RS_Line(&block, RS_LineData(RS_Vector{0, 0}, RS_Vector{0, 4})));
            block.addEntity(new RS_Line(&block, RS_LineData(RS_Vector{10, 0}, RS_Vector{10, 1})));
        });
    }

    RS_Insert* addInsert(const QString& name, const RS_Vector& at, const double angle,
                         const RS_Vector& scale = RS_Vector{1, 1}, const int cols = 1, const int rows = 1,
                         const RS_Vector& spacing = RS_Vector{0, 0}) {
        auto* insert = new RS_Insert(&m_graphic, RS_InsertData(name, at, scale, angle, cols, rows, spacing));
        m_graphic.addEntity(insert);
        insert->update();
        return insert;
    }

    void modify(const std::function<void(LC_DocumentModificationBatch&)>& operation) {
        m_graphic.undoableModify(m_view.getViewPort(), [&](LC_DocumentModificationBatch& ctx) {
            operation(ctx);
            return true;
        });
    }

    QList<RS_Entity*> live(const RS2::EntityType type = RS2::EntityUnknown) const {
        QList<RS_Entity*> entities;
        for (RS_Entity* e : m_graphic) {
            if (e != nullptr && !e->isDeleted() && (type == RS2::EntityUnknown || e->rtti() == type)) {
                entities << e;
            }
        }
        return entities;
    }

    void copy(QList<RS_Entity*> entities, const RS_Vector& ref = RS_Vector{0, 0}) {
        m_graphic.select(entities, true);
        LC_CopyUtils::copy(ref, entities, &m_graphic);
        m_graphic.select(entities, false);
    }

    void paste(const LC_CopyUtils::RS_PasteData& data) {
        modify([&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(data, &m_graphic, ctx);
            ctx.dontSetActiveLayerAndPen();
        });
    }
};

/// Catch::Approx(x).margin(m) still accepts a relative error of 1e-5 (its default epsilon), which
/// is 1e-3 at the coordinates used here: compare absolutely, to \p tolerance relative to \p expected
bool near(const double actual, const double expected, const double tolerance = 1e-9) {
    return std::abs(actual - expected) <= tolerance * std::max(1.0, std::abs(expected));
}

using Segment = std::pair<RS_Vector, RS_Vector>;

/// the end points of every line the insert expands to, in the insert's own order
void collectLines(const RS_Entity* e, std::vector<Segment>& out) {
    if (e->rtti() == RS2::EntityLine) {
        const auto* line = static_cast<const RS_Line*>(e);
        out.emplace_back(line->getStartpoint(), line->getEndpoint());
    }
    else if (e->isContainer()) {
        for (const RS_Entity* child : *static_cast<const RS_EntityContainer*>(e)) {
            if (child != nullptr && !child->isDeleted()) {
                collectLines(child, out);
            }
        }
    }
}

std::vector<Segment> linesOf(const RS_Entity* e) {
    std::vector<Segment> lines;
    collectLines(e, lines);
    return lines;
}

/// \p actual is \p expected moved by \p offset
void checkTranslated(const std::vector<Segment>& expected, const std::vector<Segment>& actual, const RS_Vector& offset) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO("segment " << i);
        const RS_Vector start = expected[i].first + offset;
        const RS_Vector end = expected[i].second + offset;
        CHECK(near(actual[i].first.x, start.x));
        CHECK(near(actual[i].first.y, start.y));
        CHECK(near(actual[i].second.x, end.x));
        CHECK(near(actual[i].second.y, end.y));
    }
}

/// the angles agree modulo a full turn
void checkSameAngle(const double expected, const double actual) {
    CHECK(std::abs(std::remainder(actual - expected, 2.0 * M_PI)) < 1e-9);
}

RS_Insert* onlyNewInsert(Drawing& d, const RS_Insert* original) {
    RS_Insert* pasted = nullptr;
    int count = 0;
    for (RS_Entity* e : d.live(RS2::EntityInsert)) {
        if (e != original) {
            pasted = static_cast<RS_Insert*>(e);
            ++count;
        }
    }
    REQUIRE(count == 1);
    return pasted;
}

}

TEST_CASE("A pasted insert keeps the angle it was copied with", "[copy][paste][insert][2957]") {
    for (const double degrees : {0.0, 30.0, 90.0, 180.0, 270.0, -45.0, 359.0}) {
        INFO("angle " << degrees << " degrees");
        Drawing d;
        d.addLBlock("L");
        const double angle = RS_Math::deg2rad(degrees);
        const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle);
        const std::vector<Segment> before = linesOf(original);

        d.copy(d.live(RS2::EntityInsert));
        d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));

        const RS_Insert* pasted = onlyNewInsert(d, original);
        checkSameAngle(angle, pasted->getAngle());
        CHECK(near(pasted->getScale().x, 1.0));
        CHECK(near(pasted->getScale().y, 1.0));
        CHECK(near(pasted->getInsertionPoint().x, 105.0, 1e-9));
        CHECK(near(pasted->getInsertionPoint().y, 25.0, 1e-9));
        checkTranslated(before, linesOf(pasted), RS_Vector{100, 20});
        // and the original is untouched
        checkSameAngle(angle, original->getAngle());
        checkTranslated(before, linesOf(original), RS_Vector{0, 0});
    }
}

TEST_CASE("A pasted insert is offset by the reference point it was copied about", "[copy][paste][insert][2957]") {
    Drawing d;
    d.addLBlock("L");
    const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, RS_Math::deg2rad(30.0));
    const std::vector<Segment> before = linesOf(original);

    d.copy(d.live(RS2::EntityInsert), RS_Vector{5, 5});
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{200, -40}));

    const RS_Insert* pasted = onlyNewInsert(d, original);
    checkSameAngle(RS_Math::deg2rad(30.0), pasted->getAngle());
    // the reference point (5, 5) lands on the paste point
    checkTranslated(before, linesOf(pasted), RS_Vector{195, -45});
}

TEST_CASE("A pasted insert keeps its scale, mirror and array", "[copy][paste][insert][2957]") {
    struct Case {
        const char* m_name;
        RS_Vector m_scale;
        int m_cols;
        int m_rows;
        RS_Vector m_spacing;
    };
    const Case cases[] = {
        {"uniform scale", {2, 2}, 1, 1, {0, 0}},
        {"different scales", {2, 3}, 1, 1, {0, 0}},
        {"mirrored in x", {-1, 1}, 1, 1, {0, 0}},
        {"mirrored in y", {1, -2}, 1, 1, {0, 0}},
        {"array", {1, 1}, 3, 2, {15, 8}},
        {"scaled array", {1.5, 0.5}, 2, 3, {12, 9}},
    };
    for (const Case& c : cases) {
        for (const double degrees : {0.0, 35.0, 210.0}) {
            INFO(c.m_name << " at " << degrees << " degrees");
            Drawing d;
            d.addLBlock("L");
            const double angle = RS_Math::deg2rad(degrees);
            const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle, c.m_scale, c.m_cols, c.m_rows, c.m_spacing);
            const std::vector<Segment> before = linesOf(original);
            REQUIRE(!before.empty());

            d.copy(d.live(RS2::EntityInsert));
            d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));

            const RS_Insert* pasted = onlyNewInsert(d, original);
            checkSameAngle(angle, pasted->getAngle());
            CHECK(near(pasted->getScale().x, c.m_scale.x));
            CHECK(near(pasted->getScale().y, c.m_scale.y));
            CHECK(pasted->getCols() == c.m_cols);
            CHECK(pasted->getRows() == c.m_rows);
            CHECK(near(pasted->getSpacing().x, c.m_spacing.x));
            CHECK(near(pasted->getSpacing().y, c.m_spacing.y));
            checkTranslated(before, linesOf(pasted), RS_Vector{100, 20});
        }
    }
}

TEST_CASE("Pasting a selection keeps the inserts' angles relative to each other and to the rest", "[copy][paste][insert][2957]") {
    // the report: a whole collection of entities is copied and pasted elsewhere, and every
    // relative position and angle is expected to survive
    Drawing d;
    d.addLBlock("L");
    d.addBlock("T", [](RS_Block& block) {
        block.addEntity(new RS_Line(&block, RS_LineData(RS_Vector{-3, 0}, RS_Vector{3, 0})));
        block.addEntity(new RS_Line(&block, RS_LineData(RS_Vector{0, 0}, RS_Vector{0, 7})));
    });
    d.m_graphic.addEntity(new RS_Line(&d.m_graphic, RS_LineData(RS_Vector{0, 0}, RS_Vector{50, 10})));
    d.addInsert("L", RS_Vector{5, 5}, RS_Math::deg2rad(30.0));
    d.addInsert("T", RS_Vector{20, -8}, RS_Math::deg2rad(120.0), RS_Vector{2, 2});
    d.addInsert("L", RS_Vector{-14, 9}, 0.0);
    d.addInsert("T", RS_Vector{33, 33}, RS_Math::deg2rad(-75.0));

    std::vector<std::vector<Segment>> before;
    std::vector<double> angles;
    for (RS_Entity* e : d.live()) {
        before.push_back(linesOf(e));
        angles.push_back(e->rtti() == RS2::EntityInsert ? static_cast<RS_Insert*>(e)->getAngle() : 0.0);
    }
    const QList<RS_Entity*> sources = d.live();
    d.copy(sources, RS_Vector{7, 3});
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{300, 300}));

    const QList<RS_Entity*> all = d.live();
    REQUIRE(all.size() == 2 * sources.size());
    for (int i = 0; i < sources.size(); ++i) {
        INFO("entity " << i);
        const RS_Entity* copy = all.at(sources.size() + i);
        REQUIRE(copy->rtti() == sources.at(i)->rtti());
        if (copy->rtti() == RS2::EntityInsert) {
            checkSameAngle(angles[static_cast<std::size_t>(i)], static_cast<const RS_Insert*>(copy)->getAngle());
        }
        checkTranslated(before[static_cast<std::size_t>(i)], linesOf(copy), RS_Vector{293, 297});
    }
}

TEST_CASE("Pasting with a rotation adds it to the insert's own angle", "[copy][paste][insert][2957]") {
    for (const double pasteDegrees : {90.0, 25.0, -60.0}) {
        INFO("paste rotated by " << pasteDegrees);
        Drawing d;
        d.addLBlock("L");
        const double angle = RS_Math::deg2rad(30.0);
        const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle, RS_Vector{2, 2});
        const std::vector<Segment> before = linesOf(original);

        d.copy(d.live(RS2::EntityInsert));
        d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}, 1.0, RS_Math::deg2rad(pasteDegrees)));

        const RS_Insert* pasted = onlyNewInsert(d, original);
        checkSameAngle(angle + RS_Math::deg2rad(pasteDegrees), pasted->getAngle());
        // the same as rotating the copied geometry about the origin, then moving it
        std::vector<Segment> rotated = before;
        for (Segment& s : rotated) {
            s.first.rotate(RS_Vector{0, 0}, RS_Math::deg2rad(pasteDegrees));
            s.second.rotate(RS_Vector{0, 0}, RS_Math::deg2rad(pasteDegrees));
        }
        checkTranslated(rotated, linesOf(pasted), RS_Vector{100, 20});
    }
}

TEST_CASE("Pasting with a scale scales the insert's own scale", "[copy][paste][insert][2957]") {
    Drawing d;
    d.addLBlock("L");
    const double angle = RS_Math::deg2rad(30.0);
    const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle);
    const std::vector<Segment> before = linesOf(original);

    d.copy(d.live(RS2::EntityInsert));
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}, 2.5, 0.0));

    const RS_Insert* pasted = onlyNewInsert(d, original);
    checkSameAngle(angle, pasted->getAngle());
    CHECK(near(pasted->getScale().x, 2.5));
    CHECK(near(pasted->getScale().y, 2.5));
    std::vector<Segment> scaled = before;
    for (Segment& s : scaled) {
        s.first.scale(RS_Vector{0, 0}, RS_Vector{2.5, 2.5});
        s.second.scale(RS_Vector{0, 0}, RS_Vector{2.5, 2.5});
    }
    checkTranslated(scaled, linesOf(pasted), RS_Vector{100, 20});
}

TEST_CASE("A block inside a block keeps its angle when the outer insert is pasted", "[copy][paste][insert][2957]") {
    Drawing d;
    d.addLBlock("L");
    d.addBlock("Outer", [](RS_Block& block) {
        auto* inner = new RS_Insert(&block, RS_InsertData("L", RS_Vector{4, 2}, RS_Vector{1, 1}, RS_Math::deg2rad(40.0), 1, 1, RS_Vector{0, 0}));
        block.addEntity(inner);
    });
    const RS_Insert* original = d.addInsert("Outer", RS_Vector{5, 5}, RS_Math::deg2rad(70.0));
    const std::vector<Segment> before = linesOf(original);
    REQUIRE(!before.empty());

    d.copy(d.live(RS2::EntityInsert));
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));

    const RS_Insert* pasted = onlyNewInsert(d, original);
    checkSameAngle(RS_Math::deg2rad(70.0), pasted->getAngle());
    checkTranslated(before, linesOf(pasted), RS_Vector{100, 20});
}

TEST_CASE("An insert pasted into another drawing keeps its angle", "[copy][paste][insert][2957]") {
    Drawing source;
    source.addLBlock("L");
    const RS_Insert* original = source.addInsert("L", RS_Vector{5, 5}, RS_Math::deg2rad(30.0), RS_Vector{1.5, 1.5});
    const std::vector<Segment> before = linesOf(original);
    source.copy(source.live(RS2::EntityInsert));

    Drawing destination;
    destination.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));

    const QList<RS_Entity*> inserts = destination.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 1);
    const auto* pasted = static_cast<const RS_Insert*>(inserts.first());
    checkSameAngle(RS_Math::deg2rad(30.0), pasted->getAngle());
    CHECK(near(pasted->getScale().x, 1.5));
    checkTranslated(before, linesOf(pasted), RS_Vector{100, 20});
}

TEST_CASE("Pasting twice puts two identical inserts", "[copy][paste][insert][2957]") {
    Drawing d;
    d.addLBlock("L");
    const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, RS_Math::deg2rad(30.0));
    const std::vector<Segment> before = linesOf(original);
    d.copy(d.live(RS2::EntityInsert));
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 0}));
    d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{0, 100}));

    const QList<RS_Entity*> inserts = d.live(RS2::EntityInsert);
    REQUIRE(inserts.size() == 3);
    checkTranslated(before, linesOf(inserts.at(1)), RS_Vector{100, 0});
    checkTranslated(before, linesOf(inserts.at(2)), RS_Vector{0, 100});
    checkSameAngle(RS_Math::deg2rad(30.0), static_cast<const RS_Insert*>(inserts.at(1))->getAngle());
    checkSameAngle(RS_Math::deg2rad(30.0), static_cast<const RS_Insert*>(inserts.at(2))->getAngle());
}

TEST_CASE("An insert pastes with its angle and place at every whole degree", "[copy][paste][insert][2957]") {
    // a bare epsilon in the source-edit check refused the paste's move at about 6% of angles
    // (186, 204, 217, ... degrees), which left the copy stacked on the original
    int wrong = 0;
    for (int degrees = 0; degrees < 360; ++degrees) {
        Drawing d;
        d.addLBlock("L");
        const double angle = RS_Math::deg2rad(degrees);
        const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle);
        const std::vector<Segment> before = linesOf(original);
        d.copy(d.live(RS2::EntityInsert), RS_Vector{5, 5});
        d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));

        const RS_Insert* pasted = onlyNewInsert(d, original);
        bool ok = std::abs(std::remainder(pasted->getAngle() - angle, 2.0 * M_PI)) < 1e-9
            && std::abs(pasted->getInsertionPoint().x - 100.0) < 1e-9 && std::abs(pasted->getInsertionPoint().y - 20.0) < 1e-9;
        const std::vector<Segment> after = linesOf(pasted);
        ok = ok && after.size() == before.size();
        for (std::size_t i = 0; ok && i < before.size(); ++i) {
            ok = (after[i].first - (before[i].first + RS_Vector{95, 15})).squared() < 1e-12
                && (after[i].second - (before[i].second + RS_Vector{95, 15})).squared() < 1e-12;
        }
        if (!ok) {
            ++wrong;
            WARN("wrong at " << degrees << " degrees");
        }
    }
    CHECK(wrong == 0);
}

TEST_CASE("Nested inserts paste with their expansion across a grid of pairs of angles", "[copy][paste][insert][2957]") {
    // an outer insert at 70 degrees over an inner one at 40 pasted with no children: the
    // edits before the paste rebuilt the outer scale a ulp off 1, and the composed columns
    // then looked sheared to the expansion
    int wrong = 0;
    int cases = 0;
    for (int outerDegrees = 0; outerDegrees < 360; outerDegrees += 13) {
        for (int innerDegrees = 0; innerDegrees < 360; innerDegrees += 17) {
            Drawing d;
            d.addLBlock("L");
            d.addBlock("Outer", [innerDegrees](RS_Block& block) {
                block.addEntity(new RS_Insert(&block, RS_InsertData("L", RS_Vector{4, 2}, RS_Vector{1, 1}, RS_Math::deg2rad(innerDegrees), 1, 1, RS_Vector{0, 0})));
            });
            const RS_Insert* original = d.addInsert("Outer", RS_Vector{5, 5}, RS_Math::deg2rad(outerDegrees));
            const std::vector<Segment> before = linesOf(original);
            d.copy(d.live(RS2::EntityInsert), RS_Vector{3, 1});
            d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));
            const RS_Insert* pasted = onlyNewInsert(d, original);
            const std::vector<Segment> after = linesOf(pasted);
            bool ok = after.size() == before.size() && !before.empty();
            for (std::size_t i = 0; ok && i < before.size(); ++i) {
                ok = (after[i].first - (before[i].first + RS_Vector{97, 19})).squared() < 1e-12
                    && (after[i].second - (before[i].second + RS_Vector{97, 19})).squared() < 1e-12;
            }
            ++cases;
            if (!ok) {
                ++wrong;
                WARN("wrong at outer " << outerDegrees << ", inner " << innerDegrees);
            }
        }
    }
    CHECK(cases > 500);
    CHECK(wrong == 0);
}

TEST_CASE("Pasting rotated inserts with a scale and a rotation keeps them where the copy puts them", "[copy][paste][insert][2957]") {
    // a scale of 25.4 and a 37 degree rotation: four edits of the clone, about 30% of angles
    // had one refused
    int wrong = 0;
    for (int degrees = 0; degrees < 360; degrees += 3) {
        Drawing d;
        d.addLBlock("L");
        const double angle = RS_Math::deg2rad(degrees);
        const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, angle);
        const std::vector<Segment> before = linesOf(original);
        d.copy(d.live(RS2::EntityInsert), RS_Vector{5, 5});
        d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}, 25.4, RS_Math::deg2rad(37.0)));
        const RS_Insert* pasted = onlyNewInsert(d, original);
        bool ok = std::abs(std::remainder(pasted->getAngle() - angle - RS_Math::deg2rad(37.0), 2.0 * M_PI)) < 1e-9
            && std::abs(pasted->getScale().x - 25.4) < 1e-9;
        const std::vector<Segment> after = linesOf(pasted);
        ok = ok && after.size() == before.size();
        for (std::size_t i = 0; ok && i < before.size(); ++i) {
            // about the reference point, scaled, turned about the origin, then moved to the paste point
            RS_Vector start = (before[i].first - RS_Vector{5, 5}) * 25.4;
            RS_Vector end = (before[i].second - RS_Vector{5, 5}) * 25.4;
            start.rotate(RS_Vector{0, 0}, RS_Math::deg2rad(37.0));
            end.rotate(RS_Vector{0, 0}, RS_Math::deg2rad(37.0));
            ok = (after[i].first - (start + RS_Vector{100, 20})).squared() < 1e-9
                && (after[i].second - (end + RS_Vector{100, 20})).squared() < 1e-9;
        }
        if (!ok) {
            ++wrong;
            WARN("wrong at " << degrees << " degrees");
        }
    }
    CHECK(wrong == 0);
}

TEST_CASE("Pasting into a drawing in other units scales inserts at every angle", "[copy][paste][insert][units][2957]") {
    // inches into millimetres: the paste scales by 25.4 on its own, and turns nothing
    int wrong = 0;
    for (int degrees = 0; degrees < 360; degrees += 3) {
        Drawing source;
        source.m_graphic.setUnit(RS2::Inch);
        source.addLBlock("L");
        const double angle = RS_Math::deg2rad(degrees);
        const RS_Insert* original = source.addInsert("L", RS_Vector{5, 5}, angle, RS_Vector{1, 1}, 3, 2, RS_Vector{15, 8});
        const std::vector<Segment> before = linesOf(original);
        source.copy(source.live(RS2::EntityInsert), RS_Vector{5, 5});

        Drawing destination;
        destination.m_graphic.setUnit(RS2::Millimeter);
        destination.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}));
        const QList<RS_Entity*> inserts = destination.live(RS2::EntityInsert);
        REQUIRE(inserts.size() == 1);
        const auto* pasted = static_cast<const RS_Insert*>(inserts.first());
        bool ok = std::abs(std::remainder(pasted->getAngle() - angle, 2.0 * M_PI)) < 1e-9
            && near(pasted->getScale().x, 25.4) && near(pasted->getScale().y, 25.4)
            && pasted->getCols() == 3 && pasted->getRows() == 2;
        const std::vector<Segment> after = linesOf(pasted);
        ok = ok && after.size() == before.size();
        for (std::size_t i = 0; ok && i < before.size(); ++i) {
            const RS_Vector start = (before[i].first - RS_Vector{5, 5}) * 25.4 + RS_Vector{100, 20};
            const RS_Vector end = (before[i].second - RS_Vector{5, 5}) * 25.4 + RS_Vector{100, 20};
            ok = (after[i].first - start).squared() < 1e-9 && (after[i].second - end).squared() < 1e-9;
        }
        if (!ok) {
            ++wrong;
            WARN("wrong at " << degrees << " degrees");
        }
    }
    CHECK(wrong == 0);
}

TEST_CASE("A mirrored insert pasted with a scale and a rotation keeps its geometry", "[copy][paste][insert][2957]") {
    // the stored fields of a mirrored insert are re-expressed when the paste turns or scales it (a
    // mirror in x is a mirror in y turned half a way), so the geometry is what is checked
    for (const RS_Vector& scale : {RS_Vector{-1, 1}, RS_Vector{1, -2}, RS_Vector{-1.5, 0.5}}) {
        for (int degrees = 0; degrees < 360; degrees += 17) {
            Drawing d;
            d.addLBlock("L");
            const RS_Insert* original = d.addInsert("L", RS_Vector{5, 5}, RS_Math::deg2rad(degrees), scale);
            const std::vector<Segment> before = linesOf(original);
            d.copy(d.live(RS2::EntityInsert), RS_Vector{5, 5});
            d.paste(LC_CopyUtils::RS_PasteData(RS_Vector{100, 20}, 2.0, RS_Math::deg2rad(10.0)));
            const RS_Insert* pasted = onlyNewInsert(d, original);
            std::vector<Segment> expected = before;
            for (Segment& segment : expected) {
                for (RS_Vector* point : {&segment.first, &segment.second}) {
                    *point = (*point - RS_Vector{5, 5}) * 2.0;
                    point->rotate(RS_Vector{0, 0}, RS_Math::deg2rad(10.0));
                    *point += RS_Vector{100, 20};
                }
            }
            INFO("scale " << scale.x << "," << scale.y << " at " << degrees << " degrees");
            checkTranslated(expected, linesOf(pasted), RS_Vector{0, 0});
        }
    }
}
