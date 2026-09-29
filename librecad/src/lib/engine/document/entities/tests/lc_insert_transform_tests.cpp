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

// Issue #2957: the INSERT transform math tells rounding from a real shear. A nested block's
// expansion is refused as a whole on a shear, and an edit (move, rotate, scale, mirror, the
// paste's) is refused when the insert's new angle and scale factors would not reproduce it,
// so a tolerance of one ulp made both happen, at random, to about 6% of rotated inserts.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "lc_actiontestsupport.h"
#include "lc_insert_transform.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"

namespace {

constexpr double kDegree = M_PI / 180.0;

RS_InsertData insertData(const RS_Vector& at, const RS_Vector& scale, const double angle, const double extrusionZ = 1.0) {
    RS_InsertData data(QStringLiteral("B"), at, scale, angle, 1, 1, RS_Vector{0, 0});
    data.extrusion = RS_Vector{0.0, 0.0, extrusionZ};
    return data;
}

LC_InsertTransform frameOf(const RS_InsertData& data) {
    LC_InsertTransform frame;
    REQUIRE(LC_InsertTransform::fromInsert(data, lcInsertTransformOrigin(), 0, 0, frame) == LC_InsertTransformStatus::Ok);
    return frame;
}

/// a small deterministic generator, so a failing case can be replayed
struct Lcg {
    std::uint64_t m_state = 0x2957;
    double next() {
        m_state = m_state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(m_state >> 11) / static_cast<double>(1ULL << 53);
    }
};

/// both frames map the block's points to the same places, to a relative \p tolerance
void checkSameFrame(const LC_InsertTransform& expected, const LC_InsertTransform& actual, const double tolerance = 1e-9) {
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{1, 0}, RS_Vector{0, 1}, RS_Vector{7, -3}}) {
        const RS_Vector e = expected.mapLeafPoint(p);
        const RS_Vector a = actual.mapLeafPoint(p);
        const double scale = std::max({1.0, std::abs(e.x), std::abs(e.y)});
        CHECK(std::abs(a.x - e.x) <= tolerance * scale);
        CHECK(std::abs(a.y - e.y) <= tolerance * scale);
    }
}

}

TEST_CASE("Composed rotations are never mistaken for a shear", "[insert-transform][2957]") {
    struct Scales {
        RS_Vector m_outer;
        RS_Vector m_inner;
    };
    // the last two are what a move used to leave in the outer scale: one ulp off 1 in one factor
    const double justBelowOne = std::nextafter(1.0, 0.0);
    const double justAboveOne = std::nextafter(1.0, 2.0);
    const Scales scales[] = {
        {{1, 1}, {1, 1}}, {{2, 2}, {0.5, 0.5}}, {{1.5, 1.5}, {3, 3}}, {{1, -1}, {1, 1}}, {{2, -2}, {1, -1}},
        {{justBelowOne, 1}, {1, 1}}, {{1, justAboveOne}, {1, 1}},
    };
    for (const Scales& s : scales) {
        int rejected = 0;
        int wrongAngle = 0;
        for (int outer = 0; outer < 360; ++outer) {
            for (int inner = 0; inner < 360; ++inner) {
                LC_InsertTransform composed;
                REQUIRE(LC_InsertTransform::compose(frameOf(insertData({3, 4}, s.m_outer, outer * kDegree)),
                                                    frameOf(insertData({-2, 5}, s.m_inner, inner * kDegree)), composed));
                LC_InsertTransformParts parts;
                if (composed.decompose(parts) != LC_InsertTransformDecompositionStatus::Ok) {
                    ++rejected;
                    continue;
                }
                // the angles add, but the inner block turns the other way under a mirrored outer one
                const double flip = s.m_outer.y < 0.0 ? -1.0 : 1.0;
                const double expected = outer * kDegree + flip * inner * kDegree;
                if (std::abs(std::remainder(parts.angle - expected, 2.0 * M_PI)) > 1e-9
                    && std::abs(std::remainder(parts.angle - expected - M_PI, 2.0 * M_PI)) > 1e-9) {
                    ++wrongAngle;
                }
            }
        }
        INFO("outer scale " << s.m_outer.x << "," << s.m_outer.y << " inner scale " << s.m_inner.x << "," << s.m_inner.y);
        CHECK(rejected == 0);
        CHECK(wrongAngle == 0);
    }
}

TEST_CASE("A real shear is still refused", "[insert-transform][2957]") {
    LC_InsertTransformParts parts;

    // oblique columns
    LC_InsertTransform oblique;
    oblique.a = 0.0;
    oblique.b = 2.0;
    oblique.c = 3.0;
    oblique.d = 1.0;
    CHECK(oblique.decompose(parts) == LC_InsertTransformDecompositionStatus::Shear);

    // a non-uniform outer scale over an inner block that is not turned a quarter (that is a
    // rotation by 90 degrees with the scale factors swapped, not a shear)
    for (const double innerDegrees : {1.0, 30.0, 45.0, 89.0, 137.0}) {
        LC_InsertTransform composed;
        REQUIRE(LC_InsertTransform::compose(frameOf(insertData({0, 0}, {2, 1}, 0.0)),
                                            frameOf(insertData({0, 0}, {1, 1}, innerDegrees * kDegree)), composed));
        INFO("inner " << innerDegrees << " degrees");
        CHECK(composed.decompose(parts) == LC_InsertTransformDecompositionStatus::Shear);
    }

    // a scale ratio a millionth from uniform, under a 45 degree block
    LC_InsertTransform nearly;
    REQUIRE(LC_InsertTransform::compose(frameOf(insertData({0, 0}, {1, 1 + 1e-6}, 0.0)),
                                        frameOf(insertData({0, 0}, {1, 1}, 45.0 * kDegree)), nearly));
    CHECK(nearly.decompose(parts) == LC_InsertTransformDecompositionStatus::Shear);

    // but not one that is rounding, in the scale itself
    LC_InsertTransform rounding;
    REQUIRE(LC_InsertTransform::compose(frameOf(insertData({0, 0}, {1, 1 + 1e-13}, 0.0)),
                                        frameOf(insertData({0, 0}, {1, 1}, 45.0 * kDegree)), rounding));
    CHECK(rounding.decompose(parts) == LC_InsertTransformDecompositionStatus::Ok);

    // the tolerance is a nanometre on a metre: half of it is accepted, twice it is not
    for (const auto& [ratio, expected] : {std::pair{1.0 + 5e-10, LC_InsertTransformDecompositionStatus::Ok},
                                          std::pair{1.0 + 2e-9, LC_InsertTransformDecompositionStatus::Shear}}) {
        LC_InsertTransform edge;
        REQUIRE(LC_InsertTransform::compose(frameOf(insertData({0, 0}, {1, ratio}, 0.0)),
                                            frameOf(insertData({0, 0}, {1, 1}, 45.0 * kDegree)), edge));
        INFO("scale ratio " << ratio);
        CHECK(edge.decompose(parts) == expected);
    }

    // and a degenerate one is still degenerate
    LC_InsertTransform flat;
    flat.a = 0.0;
    flat.b = 0.0;
    CHECK(flat.decompose(parts) == LC_InsertTransformDecompositionStatus::Degenerate);
}

TEST_CASE("A non-uniform scale over a quarter turn is a rotation, not a shear", "[insert-transform][2957]") {
    // outer diag(2, 1) over a block turned 90, 180 or 270 degrees keeps perpendicular columns; the
    // one-ulp tolerance was about 1e-31 there, and such a nested block expanded to nothing
    const RS_Vector outerScales[] = {{2, 1}, {1, 2}, {3, 0.5}, {-2, 1}};
    for (const RS_Vector& scale : outerScales) {
        for (const int innerDegrees : {90, 180, 270}) {
            for (const int outerDegrees : {0, 30, 90, 200}) {
                LC_InsertTransform composed;
                REQUIRE(LC_InsertTransform::compose(frameOf(insertData({0, 0}, scale, outerDegrees * kDegree)),
                                                    frameOf(insertData({0, 0}, {1, 1}, innerDegrees * kDegree)), composed));
                INFO("outer scale " << scale.x << "," << scale.y << " at " << outerDegrees << ", inner " << innerDegrees);
                LC_InsertTransformParts parts;
                REQUIRE(composed.decompose(parts) == LC_InsertTransformDecompositionStatus::Ok);
                // the parts rebuild the matrix
                const double c = std::cos(parts.angle);
                const double s = std::sin(parts.angle);
                const double scaleMagnitude = std::max(parts.scaleX, std::abs(parts.scaleY));
                CHECK(std::abs(composed.a - parts.scaleX * c) <= 1e-12 * scaleMagnitude);
                CHECK(std::abs(composed.b - parts.scaleX * s) <= 1e-12 * scaleMagnitude);
                CHECK(std::abs(composed.c + parts.scaleY * s) <= 1e-12 * scaleMagnitude);
                CHECK(std::abs(composed.d - parts.scaleY * c) <= 1e-12 * scaleMagnitude);
            }
        }
    }
}

TEST_CASE("Moving an insert leaves its angle, scale and spacing as they were", "[insert-transform][2957]") {
    const RS_Vector scales[] = {{1, 1}, {2, 3}, {-1, 1}, {1.5, -0.5}};
    for (const double extrusionZ : {1.0, -1.0}) {
        for (const RS_Vector& scale : scales) {
            for (int degrees = 0; degrees < 360; ++degrees) {
                RS_InsertData source = insertData({5, 5}, scale, degrees * kDegree, extrusionZ);
                source.cols = 3;
                source.rows = 2;
                source.spacing = RS_Vector{15, 8};
                Lcg random;
                RS_InsertData data = source;
                RS_Vector total{0, 0};
                bool ok = true;
                for (int i = 0; i < 100; ++i) {
                    const RS_Vector offset{(random.next() - 0.5) * 400.0, (random.next() - 0.5) * 400.0};
                    LC_InsertTransform edit;
                    REQUIRE(LC_InsertTransform::translation(offset, edit));
                    RS_InsertData moved;
                    ok = lcApplyInsertSourceEdit(data, edit, moved) == LC_InsertSourceEditStatus::Ok && ok;
                    data = moved;
                    total += offset;
                }
                INFO("extrusion " << extrusionZ << " scale " << scale.x << "," << scale.y << " angle " << degrees);
                REQUIRE(ok);
                // bit for bit: a move does not rebuild them
                CHECK(data.angle == source.angle);
                CHECK(data.scaleFactor.x == source.scaleFactor.x);
                CHECK(data.scaleFactor.y == source.scaleFactor.y);
                CHECK(data.spacing.x == source.spacing.x);
                CHECK(data.spacing.y == source.spacing.y);
                CHECK(data.cols == source.cols);
                CHECK(data.rows == source.rows);
                // and the frame is the source's, moved by the sum of the offsets
                LC_InsertTransform shift;
                REQUIRE(LC_InsertTransform::translation(total, shift));
                LC_InsertTransform expected;
                REQUIRE(LC_InsertTransform::compose(shift, frameOf(source), expected));
                checkSameFrame(expected, frameOf(data), 1e-10);
            }
        }
    }
}

TEST_CASE("A move puts an angle stored outside a full turn back into it, and leaves one inside", "[insert-transform][2957]") {
    // a DXF file keeps the angle as it was written; every other edit normalises it
    LC_InsertTransform edit;
    REQUIRE(LC_InsertTransform::translation(RS_Vector{3, 4}, edit));
    const double angles[] = {-90.0, 450.0, 720.0, -30.0, 360.0, 1e5};
    for (const double degrees : angles) {
        const RS_InsertData source = insertData({1, 2}, {1, 1}, degrees * kDegree);
        RS_InsertData result;
        REQUIRE(lcApplyInsertSourceEdit(source, edit, result) == LC_InsertSourceEditStatus::Ok);
        INFO("stored angle " << degrees << " degrees");
        CHECK(result.angle >= 0.0);
        CHECK(result.angle < 2.0 * M_PI);
        CHECK(std::abs(std::remainder(result.angle - source.angle, 2.0 * M_PI)) < 1e-9);
    }
    for (int degrees = 0; degrees < 360; ++degrees) {
        const RS_InsertData source = insertData({1, 2}, {1, 1}, degrees * kDegree);
        RS_InsertData result;
        REQUIRE(lcApplyInsertSourceEdit(source, edit, result) == LC_InsertSourceEditStatus::Ok);
        CHECK(result.angle == source.angle);
    }
}

TEST_CASE("An edit with an identity linear part changes nothing but the position", "[insert-transform][2957]") {
    // the paste applies scale 1 and rotation 0 about the origin before it moves the entity
    RS_InsertData source = insertData({5, 5}, {1.5, -0.5}, 204.0 * kDegree);
    source.cols = 2;
    source.spacing = RS_Vector{9, 4};
    LC_InsertTransform scaleOne;
    REQUIRE(LC_InsertTransform::scale(RS_Vector{0, 0}, RS_Vector{1, 1}, scaleOne));
    LC_InsertTransform rotateZero;
    REQUIRE(LC_InsertTransform::rotation(RS_Vector{0, 0}, 0.0, rotateZero));
    for (const LC_InsertTransform* edit : {&scaleOne, &rotateZero}) {
        RS_InsertData result;
        REQUIRE(lcApplyInsertSourceEdit(source, *edit, result) == LC_InsertSourceEditStatus::Ok);
        CHECK(result.insertionPoint.x == source.insertionPoint.x);
        CHECK(result.insertionPoint.y == source.insertionPoint.y);
        CHECK(result.angle == source.angle);
        CHECK(result.scaleFactor.x == source.scaleFactor.x);
        CHECK(result.scaleFactor.y == source.scaleFactor.y);
        CHECK(result.spacing.x == source.spacing.x);
        CHECK(result.spacing.y == source.spacing.y);
    }
}

TEST_CASE("An insert can be edited whatever its angle", "[insert-transform][2957]") {
    // move, rotate, mirror and scale of a single insert, at every whole degree: each is
    // representable, so none may be refused (a bare epsilon refused about 6% of them)
    for (const double extrusionZ : {1.0, -1.0}) {
        for (int degrees = 0; degrees < 360; ++degrees) {
            const RS_InsertData source = insertData({5, 5}, {1, 1}, degrees * kDegree, extrusionZ);
            const LC_InsertTransform sourceFrame = frameOf(source);
            struct Edit {
                const char* m_name;
                bool m_built;
                LC_InsertTransform m_edit;
            };
            Edit edits[5];
            edits[0] = {"move", false, {}};
            edits[0].m_built = LC_InsertTransform::translation(RS_Vector{13, -7}, edits[0].m_edit);
            edits[1] = {"rotate", false, {}};
            edits[1].m_built = LC_InsertTransform::rotation(RS_Vector{2, 3}, 30.0 * kDegree, edits[1].m_edit);
            edits[2] = {"mirror", false, {}};
            edits[2].m_built = LC_InsertTransform::reflection(RS_Vector{0, 0}, RS_Vector{std::cos(30.0 * kDegree), std::sin(30.0 * kDegree)}, edits[2].m_edit);
            edits[3] = {"scale", false, {}};
            edits[3].m_built = LC_InsertTransform::scale(RS_Vector{1, 1}, RS_Vector{2.5, 2.5}, edits[3].m_edit);
            edits[4] = {"scale 25.4", false, {}};
            edits[4].m_built = LC_InsertTransform::scale(RS_Vector{0, 0}, RS_Vector{25.4, 25.4}, edits[4].m_edit);
            for (const Edit& e : edits) {
                INFO(e.m_name << " at " << degrees << " degrees, extrusion " << extrusionZ);
                REQUIRE(e.m_built);
                RS_InsertData result;
                REQUIRE(lcApplyInsertSourceEdit(source, e.m_edit, result) == LC_InsertSourceEditStatus::Ok);
                LC_InsertTransform expected;
                REQUIRE(LC_InsertTransform::compose(e.m_edit, sourceFrame, expected));
                checkSameFrame(expected, frameOf(result), 1e-10);
            }
        }
    }
}

TEST_CASE("Hundreds of edits of one insert stay editable and exact", "[insert-transform][2957]") {
    for (int degrees = 0; degrees < 360; degrees += 7) {
        Lcg random;
        RS_InsertData data = insertData({5, 5}, {1, 1}, degrees * kDegree);
        LC_InsertTransform expected = frameOf(data);
        for (int i = 0; i < 300; ++i) {
            LC_InsertTransform edit;
            switch (static_cast<int>(random.next() * 4.0)) {
                case 0:
                    REQUIRE(LC_InsertTransform::translation(RS_Vector{(random.next() - 0.5) * 100.0, (random.next() - 0.5) * 100.0}, edit));
                    break;
                case 1:
                    REQUIRE(LC_InsertTransform::rotation(RS_Vector{random.next() * 10.0, random.next() * 10.0}, random.next() * 2.0 * M_PI, edit));
                    break;
                case 2: {
                    const double axis = random.next() * M_PI;
                    REQUIRE(LC_InsertTransform::reflection(RS_Vector{1, 2}, RS_Vector{1 + std::cos(axis), 2 + std::sin(axis)}, edit));
                    break;
                }
                default: {
                    const double factor = random.next() < 0.5 ? 1.25 : 0.8;
                    REQUIRE(LC_InsertTransform::scale(RS_Vector{random.next(), random.next()}, RS_Vector{factor, factor}, edit));
                    break;
                }
            }
            RS_InsertData next;
            INFO("start angle " << degrees << ", edit " << i);
            REQUIRE(lcApplyInsertSourceEdit(data, edit, next) == LC_InsertSourceEditStatus::Ok);
            data = next;
            LC_InsertTransform composed;
            REQUIRE(LC_InsertTransform::compose(edit, expected, composed));
            expected = composed;
        }
        INFO("start angle " << degrees);
        // the stored fields still describe the frame all the edits added up to, to well within
        // what a drawing can tell apart: 300 edits, each with a few ulps of rounding
        checkSameFrame(expected, frameOf(data), 1e-8);
    }
}

TEST_CASE("A source edit that would shear the insert is still refused", "[insert-transform][2957]") {
    // diag(2,3) after a 45 degree rotation has non-orthogonal columns
    const RS_InsertData source = insertData({10, 20}, {2, 3}, 45.0 * kDegree);
    LC_InsertTransform edit;
    REQUIRE(LC_InsertTransform::scale(RS_Vector{0, 0}, RS_Vector{2, 3}, edit));
    RS_InsertData result;
    CHECK(lcApplyInsertSourceEdit(source, edit, result) == LC_InsertSourceEditStatus::Unrepresentable);
}

TEST_CASE("Moving an insert of a nested block keeps its expansion across a grid of angles", "[insert-transform][insert][2957]") {
    // the case that showed the problem: an outer insert at 70 degrees over an inner one at 40
    // expanded to nothing after a move, because the move rebuilt the outer scale as
    // 0.99999999999999989 and the composed columns then looked sheared by 2e-16
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    int emptied = 0;
    int moved = 0;
    // every 3 degrees of the outer angle over every 7 of the inner one, and the pair of the
    // report, which is not on that grid
    std::vector<int> innerAngles{40};
    for (int inner = 0; inner < 360; inner += 7) {
        innerAngles.push_back(inner);
    }
    std::vector<int> outerAngles{70};
    for (int outer = 0; outer < 360; outer += 3) {
        outerAngles.push_back(outer);
    }
    for (const int inner : innerAngles) {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        auto* leaf = new RS_Block(&graphic, RS_BlockData(QStringLiteral("L"), RS_Vector{0, 0}, false));
        leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector{0, 0}, RS_Vector{10, 0})));
        leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector{0, 0}, RS_Vector{0, 4})));
        graphic.addBlock(leaf);
        auto* outerBlock = new RS_Block(&graphic, RS_BlockData(QStringLiteral("Outer"), RS_Vector{0, 0}, false));
        outerBlock->addEntity(new RS_Insert(outerBlock, RS_InsertData(QStringLiteral("L"), RS_Vector{4, 2}, RS_Vector{1, 1},
                                                                      inner * kDegree, 1, 1, RS_Vector{0, 0})));
        graphic.addBlock(outerBlock);
        for (const int outer : outerAngles) {
            RS_Insert insert(&graphic, RS_InsertData(QStringLiteral("Outer"), RS_Vector{5, 5}, RS_Vector{1, 1}, outer * kDegree, 1, 1, RS_Vector{0, 0}));
            const std::size_t before = insert.count();
            insert.move(RS_Vector{13, -7});
            ++moved;
            if (before == 0 || insert.count() != before || !insert.lastSourceEditSucceeded()) {
                ++emptied;
            }
        }
    }
    CHECK(moved > 0);
    CHECK(emptied == 0);
}

namespace {

/// whether any entity below \p e is an ellipse
bool holdsEllipse(const RS_Entity* e) {
    if (e->rtti() == RS2::EntityEllipse) {
        return true;
    }
    if (e->isContainer()) {
        for (const RS_Entity* child : *static_cast<const RS_EntityContainer*>(e)) {
            if (child != nullptr && holdsEllipse(child)) {
                return true;
            }
        }
    }
    return false;
}

RS_Block* addRoundBlock(RS_Graphic& graphic, const char* name) {
    auto* block = new RS_Block(&graphic, RS_BlockData(QString::fromLatin1(name), RS_Vector{0, 0}, false));
    block->addEntity(new RS_Circle(block, RS_CircleData(RS_Vector{3, 2}, 1.5)));
    block->addEntity(new RS_Arc(block, RS_ArcData(RS_Vector{-2, 1}, 4.0, 0.3, 2.2, false)));
    graphic.addBlock(block);
    return block;
}

}

TEST_CASE("A uniformly scaled insert keeps its circles and arcs at every angle", "[insert-transform][insert][2957]") {
    // the scale factors are rebuilt from the matrix and come out a few ulps apart at 3 to 5% of
    // angles; a bare epsilon made every circle and arc of such an insert an ellipse
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addRoundBlock(graphic, "R");
    for (const double factor : {1.0, 2.0, 25.4, 1000.0, 1e-3}) {
        int ellipses = 0;
        for (int degrees = 0; degrees < 360; ++degrees) {
            RS_Insert insert(&graphic, RS_InsertData(QStringLiteral("R"), RS_Vector{5, 5}, RS_Vector{factor, factor},
                                                     degrees * kDegree, 1, 1, RS_Vector{0, 0}));
            REQUIRE(insert.count() == 2);
            if (holdsEllipse(&insert)) {
                ++ellipses;
            }
        }
        INFO("scale " << factor);
        CHECK(ellipses == 0);
    }
}

TEST_CASE("Circles and arcs of nested uniformly scaled inserts stay circles and arcs", "[insert-transform][insert][2957]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    int ellipses = 0;
    int cases = 0;
    for (int inner = 0; inner < 360; inner += 11) {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        addRoundBlock(graphic, "R");
        auto* outerBlock = new RS_Block(&graphic, RS_BlockData(QStringLiteral("Outer"), RS_Vector{0, 0}, false));
        outerBlock->addEntity(new RS_Insert(outerBlock, RS_InsertData(QStringLiteral("R"), RS_Vector{4, 2}, RS_Vector{0.5, 0.5},
                                                                      inner * kDegree, 1, 1, RS_Vector{0, 0})));
        graphic.addBlock(outerBlock);
        for (int outer = 0; outer < 360; outer += 13) {
            RS_Insert insert(&graphic, RS_InsertData(QStringLiteral("Outer"), RS_Vector{5, 5}, RS_Vector{2.5, 2.5},
                                                     outer * kDegree, 1, 1, RS_Vector{0, 0}));
            ++cases;
            if (insert.count() == 0 || holdsEllipse(&insert)) {
                ++ellipses;
            }
        }
    }
    CHECK(cases > 500);
    CHECK(ellipses == 0);
}

TEST_CASE("A non-uniformly scaled insert still turns its circles and arcs into ellipses", "[insert-transform][insert][2957]") {
    const bool qtReady = lc::test::application() != nullptr;
    REQUIRE(qtReady);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addRoundBlock(graphic, "R");
    for (const RS_Vector& scale : {RS_Vector{2, 1}, RS_Vector{1, 1 + 1e-6}, RS_Vector{3, -1.5}}) {
        for (const int degrees : {0, 30, 45, 200}) {
            RS_Insert insert(&graphic, RS_InsertData(QStringLiteral("R"), RS_Vector{5, 5}, scale, degrees * kDegree, 1, 1, RS_Vector{0, 0}));
            INFO("scale " << scale.x << "," << scale.y << " at " << degrees << " degrees");
            CHECK(holdsEllipse(&insert));
        }
    }
}
