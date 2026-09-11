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

// Ownership of what RS_Modification::round() builds during the fillet hover
// preview (issue #2804).
//
// round() returns raw pointers to a new arc and to clones of both picked
// entities, and registers them in the LC_DocumentModificationBatch it is
// handed. On the hover path that batch is a local whose destructor frees
// nothing, so every entity the preview does not adopt has to be released by
// the action - otherwise each mouse move over a fillet target leaks. The two
// m_preview->removeEntity() calls that used to stand in for this were no-ops:
// the trimmed clones are never added to the preview, and removeEntity() only
// deletes what the container actually holds.

#include <catch2/catch_test_macros.hpp>

#include <QMouseEvent>

#include "lc_action_modify_round.h"
#include "lc_actiontestsupport.h"
#include "rs_preview.h"
#include "rs_line.h"
#include "rs_vector.h"

namespace {

using lc::test::eventAt;

/**
 * A line that keeps count of how many of its instances are alive, clones
 * included. Entities reach these code paths either by RS_Entity::clone(), which
 * this overrides, or by being built directly, and in both cases a dropped
 * entity shows up as a count that never comes back down.
 */
class CountingLine : public RS_Line {
public:
    static int s_live;

    CountingLine(RS_EntityContainer* parent, const RS_LineData& d) : RS_Line(parent, d) { ++s_live; }
    CountingLine(const CountingLine& other) : RS_Line(other) { ++s_live; }

    RS_Entity* clone() const override { return new CountingLine(*this); }

    ~CountingLine() override { --s_live; }
};

int CountingLine::s_live = 0;

/**
 * The action caches the first picked entity and acts on it in the mouse move
 * handler, so the probe exposes that field plus the two framework halves that
 * bracket every mouse move.
 */
class RoundProbe final : public LC_ActionModifyRound {
public:
    explicit RoundProbe(LC_ActionContext* actionContext) : LC_ActionModifyRound(actionContext) {}

    using LC_ActionModifyRound::onMouseMoveEvent;
    using LC_ActionModifyRound::m_entity1;
    using LC_ActionModifyRound::SetEntity2;
    using RS_ActionInterface::setStatus;
    using RS_PreviewActionInterface::deletePreviewAndHighlights;
    using RS_PreviewActionInterface::drawPreviewAndHighlights;
    using RS_PreviewActionInterface::m_preview;
};

struct RoundFixture : lc::test::ActionFixture<RoundProbe> {
    CountingLine* addLine(const RS_Vector& from, const RS_Vector& to) {
        auto* line = new CountingLine(&m_graphic, RS_LineData{from, to});
        m_graphic.addEntity(line);
        return line;
    }

    /// One mouse move, in the order RS_PreviewActionInterface dispatches it.
    void hover(const double x, const double y) {
        m_action->deletePreviewAndHighlights();
        LC_MouseEvent e = eventAt(x, y);
        m_action->onMouseMoveEvent(RoundProbe::SetEntity2, &e);
        m_action->drawPreviewAndHighlights();
    }
};

} // namespace

TEST_CASE("fillet hover preview releases the entities round() hands back",
          "[modify][round][fillet]") {
    RoundFixture f;
    auto* first = f.addLine({0.0, 0.0}, {100.0, 0.0});
    f.addLine({100.0, 0.0}, {100.0, 100.0});

    f.m_action->setRadius(10.0);
    f.m_action->setTrim(true);              // the shipped default
    f.m_action->m_entity1 = first;
    f.m_action->setStatus(RoundProbe::SetEntity2);

    // Hover the second line, where round() produces a preview.
    f.hover(100.0, 50.0);
    const int afterFirstHover = CountingLine::s_live;

    for (int i = 0; i < 50; ++i) {
        f.hover(100.0, 50.0);
    }

    // Each move clones both picked lines. Whatever the preview does not adopt
    // is the action's to release, so the count must not grow with the number of
    // mouse moves.
    INFO("lines still alive after 50 further hover moves");
    CHECK(CountingLine::s_live == afterFirstHover);
}

TEST_CASE("a mouse move on a view without a relative point widget does not crash",
          "[actions][graphicview]") {
    // RS_GraphicView::isInRelativePointInput() is the first statement of every
    // mouse move, and only QG_GraphicView ever creates the holder it reads.
    RoundFixture f;
    f.addLine({0.0, 0.0}, {100.0, 0.0});

    QMouseEvent move(QEvent::MouseMove, QPointF(10.0, 10.0), QPointF(10.0, 10.0),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    f.m_action->mouseMoveEvent(&move);
    SUCCEED("dispatched a mouse move through a view with no relative point widget");
}

TEST_CASE("the preview releases entities it does not adopt past its cap",
          "[preview][overlay]") {
    // addAllFromList() is handed entities the caller has already built and
    // expects the preview to take. It stops adding at m_maxEntities, so
    // everything past the cap has to be released rather than dropped.
    RoundFixture f;
    RS_Preview* preview = f.m_action->m_preview.get();
    const int cap = preview->getMaxAllowedEntities();
    REQUIRE(cap > 0);

    const int baseline = CountingLine::s_live;
    const int count = cap + 50;

    QList<RS_Entity*> handedOver;
    for (int i = 0; i < count; ++i) {
        handedOver.append(new CountingLine(nullptr, RS_LineData{{0.0, double(i)}, {1.0, double(i)}}));
    }
    REQUIRE(CountingLine::s_live == baseline + count);

    preview->addAllFromList(handedOver);
    preview->clear();

    INFO("entities past the preview cap were neither adopted nor released");
    CHECK(CountingLine::s_live == baseline);
}
