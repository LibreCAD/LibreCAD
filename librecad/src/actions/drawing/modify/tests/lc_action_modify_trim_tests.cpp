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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Modify > Trim and Trim Two on entities that are parts of other entities.
//
// The first pick of both tools resolves containers, so a click on a spline caught one of
// the lines RS_Spline::update() draws it with. Trim Two replaced that line with a trimmed
// copy: the spline showed a gap, the copy became a stray line of the drawing, and the undo
// cycle kept the spline's line. The spline frees its lines whenever it is rebuilt, which
// undoing its deletion does, so undoing the trim afterwards wrote to freed memory. Lines of
// block references and segments of polylines were replaced the same way. Plain Trim cut at
// the infinite line through the one segment of a spline clicked instead of at the spline,
// and kept a line of a block reference as its limit, which the reference frees whenever it
// is rebuilt, as all are when a layer is shown or hidden.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "lc_action_modify_trim.h"
#include "lc_actiontestsupport.h"
#include "rs_block.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_spline.h"
#include "rs_vector.h"

namespace {

using lc::test::ActionFixture;
using lc::test::eventAt;

template <bool Both>
class TrimProbe final : public LC_ActionModifyTrim {
public:
    explicit TrimProbe(LC_ActionContext* actionContext) : LC_ActionModifyTrim(actionContext, Both) {}

    using LC_ActionModifyTrim::ChooseLimitEntity;
    using LC_ActionModifyTrim::ChooseTrimEntity;
    using LC_ActionModifyTrim::m_limitEntity;
    using LC_ActionModifyTrim::onMouseMoveEvent;
    using RS_PreviewActionInterface::m_preview;
    using RS_PreviewActionInterface::m_showRefEntitiesOnPreview;

    /// A click, handled in the status the action is in.
    void click(const RS_Vector& point) {
        LC_MouseEvent e = eventAt(point.x, point.y);
        onMouseLeftButtonRelease(getStatus(), &e);
    }

    /// A mouse move, handled in the status the action is in.
    void hover(const RS_Vector& point) {
        LC_MouseEvent e = eventAt(point.x, point.y);
        onMouseMoveEvent(getStatus(), &e);
    }
};

using Trim = TrimProbe<false>;
using TrimTwo = TrimProbe<true>;

RS_Line* addLine(RS_Graphic& graphic, const RS_Vector& from, const RS_Vector& to) {
    auto* line = new RS_Line(&graphic, RS_LineData(from, to));
    graphic.addEntity(line);
    return line;
}

/// An arch from (0, 0) to (100, 0) through (50, 45), the top at parameter 0.5.
RS_Spline* addArch(RS_Graphic& graphic) {
    RS_SplineData data(3, false);
    data.controlPoints = {{0.0, 0.0}, {30.0, 60.0}, {70.0, 60.0}, {100.0, 0.0}};
    data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    data.weights.assign(4, 1.0);
    auto* spline = new RS_Spline(&graphic, data);
    graphic.addEntity(spline);
    spline->update();
    return spline;
}

/// A polyline from (0, 0) through (100, 0) to (100, 100).
RS_Polyline* addCorner(RS_Graphic& graphic) {
    auto* polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector{0.0, 0.0});
    polyline->addVertex(RS_Vector{100.0, 0.0});
    polyline->addVertex(RS_Vector{100.0, 100.0});
    graphic.addEntity(polyline);
    return polyline;
}

/// A block reference whose only entity is the line from (50, -50) to (50, 50).
RS_Insert* addVerticalLineReference(RS_Graphic& graphic) {
    auto* block = new RS_Block(&graphic, RS_BlockData(QStringLiteral("T"), RS_Vector(0.0, 0.0), false));
    graphic.addBlock(block);
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 100.0))));
    auto* insert = new RS_Insert(&graphic, RS_InsertData(QStringLiteral("T"), RS_Vector(50.0, -50.0), RS_Vector(1.0, 1.0),
                                                         0.0, 1, 1, RS_Vector(0.0, 0.0)));
    graphic.addEntity(insert);
    insert->update();
    return insert;
}

bool containsDeleted(const RS_EntityContainer& container) {
    return std::any_of(container.begin(), container.end(), [](const RS_Entity* entity) {
        return entity != nullptr && entity->isDeleted();
    });
}

/// How far the point is from the lines the spline is drawn with.
double distanceToDrawn(const RS_Spline& spline, const RS_Vector& point) {
    double distance = RS_MAXDOUBLE;
    for (const RS_Entity* segment : spline) {
        distance = std::min(distance, segment->getDistanceToPoint(point));
    }
    return distance;
}

RS_Entity* lastEntity(const RS_Graphic& graphic) {
    return graphic.entityAt(static_cast<int>(graphic.count()) - 1);
}

bool canUndo(const RS_Graphic& graphic) {
    bool undoAvailable = false;
    bool redoAvailable = false;
    graphic.collectUndoState(undoAvailable, redoAvailable);
    return undoAvailable;
}

} // namespace

TEST_CASE("Trim Two leaves a spline whole, and its undo history holds none of the spline's lines",
          "[actions][modify][trim]") {
    ActionFixture<TrimTwo> f;
    RS_Spline* spline = addArch(f.m_graphic);
    RS_Line* line = addLine(f.m_graphic, {20.0, -10.0}, {20.0, 80.0});
    const unsigned before = f.m_graphic.count();
    const unsigned segments = spline->count();
    REQUIRE_FALSE(canUndo(f.m_graphic));

    // Trim Two trims both entities it is given, and a spline cannot be trimmed
    f.m_action->click(spline->getPointAt(0.5));
    CHECK(f.m_action->getStatus() == TrimTwo::ChooseLimitEntity);
    f.m_action->click({20.0, 70.0});

    CHECK_FALSE(line->isDeleted());
    CHECK(f.m_graphic.count() == before);
    CHECK(spline->count() == segments);
    CHECK_FALSE(canUndo(f.m_graphic));
    REQUIRE_FALSE(containsDeleted(*spline));

    // undoing the deletion of the spline rebuilds the lines it is drawn with,
    // and an older undo cycle must not hold one of the lines this frees
    f.m_graphic.undoableModify(f.m_view.getViewPort(), [spline](LC_DocumentModificationBatch& ctx) {
        ctx -= spline;
        return true;
    });
    REQUIRE(spline->isDeleted());
    CHECK(f.m_graphic.undo());
    CHECK_FALSE(f.m_graphic.undo());

    CHECK_FALSE(spline->isDeleted());
    CHECK_FALSE(containsDeleted(*spline));
    CHECK(spline->count() == segments);
}

TEST_CASE("Trim Two never trims a part of another entity", "[actions][modify][trim]") {
    // Trim Two adds the trimmed copy to the drawing, which would leave a gap in the
    // entity the part belongs to and an extra line in the drawing
    ActionFixture<TrimTwo> f;
    RS_EntityContainer* owner = nullptr;
    RS_Line* line = nullptr;
    RS_Vector onPart;
    RS_Vector onLine;

    SECTION("a line of a block reference") {
        owner = addVerticalLineReference(f.m_graphic);
        line = addLine(f.m_graphic, {0.0, 0.0}, {100.0, 0.0});
        onPart = {50.0, 25.0};
        onLine = {75.0, 0.0};
    }

    SECTION("a segment of a polyline") {
        owner = addCorner(f.m_graphic);
        line = addLine(f.m_graphic, {50.0, -20.0}, {50.0, 50.0});
        onPart = {25.0, 0.0};
        onLine = {50.0, 30.0};
    }

    REQUIRE(owner->count() > 0);
    const unsigned before = f.m_graphic.count();

    f.m_action->click(onPart);
    CHECK(f.m_action->getStatus() == TrimTwo::ChooseLimitEntity);
    f.m_action->click(onLine);

    CHECK_FALSE(containsDeleted(*owner));
    CHECK_FALSE(line->isDeleted());
    CHECK(f.m_graphic.count() == before);
}

TEST_CASE("Trim cuts at a spline where the line crosses it, wherever the spline was picked",
          "[actions][modify][trim]") {
    ActionFixture<Trim> f;
    RS_Spline* spline = addArch(f.m_graphic);
    RS_Line* line = addLine(f.m_graphic, {20.0, -10.0}, {20.0, 80.0});
    const unsigned before = f.m_graphic.count();
    const unsigned segments = spline->count();

    // picked at its top, about 30 away from where the line crosses it
    f.m_action->click(spline->getPointAt(0.5));
    REQUIRE(f.m_action->getStatus() == Trim::ChooseTrimEntity);

    SECTION("in the preview") {
        f.m_action->m_showRefEntitiesOnPreview = true;
        LC_MouseEvent e = eventAt(20.0, 70.0);
        f.m_action->onMouseMoveEvent(Trim::ChooseTrimEntity, &e);

        // a reference point marks where the line is cut; RS_Preview keeps reference entities apart
        RS_EntityContainer references(nullptr, false);
        f.m_action->m_preview->addReferenceEntitiesToContainer(&references);
        const auto cut = std::find_if(references.begin(), references.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityRefPoint;
        });
        REQUIRE(cut != references.end());
        CHECK(distanceToDrawn(*spline, static_cast<const RS_Point*>(*cut)->getPos()) < 1e-6);
    }

    SECTION("in the drawing") {
        // the part clicked is kept
        f.m_action->click({20.0, 70.0});

        CHECK(line->isDeleted());
        REQUIRE(f.m_graphic.count() == before + 1);
        const RS_Entity* trimmed = lastEntity(f.m_graphic);
        REQUIRE(trimmed->rtti() == RS2::EntityLine);
        CHECK(trimmed->getEndpoint().distanceTo({20.0, 80.0}) < RS_TOLERANCE);
        CHECK(distanceToDrawn(*spline, trimmed->getStartpoint()) < 1e-6);
    }

    CHECK_FALSE(containsDeleted(*spline));
    CHECK(spline->count() == segments);
}

TEST_CASE("Trim cuts at a line inside a block reference, also after the reference was rebuilt",
          "[actions][modify][trim]") {
    ActionFixture<Trim> f;
    RS_Insert* insert = addVerticalLineReference(f.m_graphic);
    REQUIRE(insert->count() == 1);
    RS_Line* line = addLine(f.m_graphic, {0.0, 0.0}, {100.0, 0.0});
    const unsigned before = f.m_graphic.count();

    f.m_action->click({50.0, 25.0});
    REQUIRE(f.m_action->getStatus() == Trim::ChooseTrimEntity);

    // all block references rebuild their entities when a layer is shown or hidden or
    // an edit is redone, so the action keeps none of them for the next click
    REQUIRE(f.m_action->m_limitEntity != insert->entityAt(0));
    f.m_graphic.updateInserts();
    f.m_action->hover({75.0, 0.0});
    f.m_action->click({75.0, 0.0});

    CHECK(line->isDeleted());
    REQUIRE(f.m_graphic.count() == before + 1);
    const RS_Entity* trimmed = lastEntity(f.m_graphic);
    CHECK(trimmed->getStartpoint().distanceTo({50.0, 0.0}) < RS_TOLERANCE);
    CHECK(trimmed->getEndpoint().distanceTo({100.0, 0.0}) < RS_TOLERANCE);
    CHECK_FALSE(containsDeleted(*insert));
}
