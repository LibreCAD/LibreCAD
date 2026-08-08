/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for LC_Division
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <memory>

#include <QApplication>
#include <QList>
#include <QtAlgorithms>

#include <type_traits>

#include "lc_action_modify_break_divide.h"
#include "lc_actioncontext.h"
#include "lc_division.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {

constexpr double HALF_TURN = 3.14159265358979323846;

void addEndpointCutters(RS_EntityContainer& container) {
    container.addEntity(new RS_Line{&container, RS_Vector{0.0, -5.0}, RS_Vector{0.0, 5.0}});
    container.addEntity(new RS_Line{&container, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
}

void addArcEndpointCutters(RS_EntityContainer& container) {
    container.addEntity(new RS_Line{&container, RS_Vector{-10.0, -5.0}, RS_Vector{-10.0, 5.0}});
    container.addEntity(new RS_Line{&container, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
}

} // namespace

TEST_CASE("LC_Division identifies an unbounded selection as the whole entity",
          "[lc_division][break_divide][whole_entity]") {
    RS_EntityContainer emptyContainer;
    LC_Division division{&emptyContainer};

    SECTION("line") {
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
        CHECK(data->snapSegmentStart == line.getStartpoint());
        CHECK(data->snapSegmentEnd == line.getEndpoint());
    }

    SECTION("arc") {
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, false}};
        const std::unique_ptr<LC_Division::ArcSegmentData> data{
            division.findArcSegmentBetweenIntersections(&arc, RS_Vector{0.0, 10.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }

    SECTION("circle") {
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};
        const std::unique_ptr<LC_Division::CircleSegmentData> data{
            division.findCircleSegmentBetweenIntersections(&circle, RS_Vector{10.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }
}

TEST_CASE("endpoint-only intersections do not masquerade as split boundaries",
          "[lc_division][break_divide][whole_entity]") {
    SECTION("line") {
        RS_EntityContainer container;
        addEndpointCutters(container);
        LC_Division division{&container};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }

    SECTION("arc") {
        // The reversed flag swaps the angle bookkeeping ahead of the endpoint
        // checks, so both orientations are distinct paths through
        // findArcSegmentEdges.
        const bool reversed = GENERATE(false, true);
        RS_EntityContainer container;
        addArcEndpointCutters(container);
        LC_Division division{&container};
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, reversed}};
        const RS_Vector snap = reversed ? RS_Vector{0.0, -10.0} : RS_Vector{0.0, 10.0};

        const std::unique_ptr<LC_Division::ArcSegmentData> data{
            division.findArcSegmentBetweenIntersections(&arc, snap, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }
}

TEST_CASE("LC_Division keeps ordinary partial-segment behavior",
          "[lc_division][break_divide]") {
    SECTION("whole entity selection must be explicitly enabled") {
        RS_EntityContainer emptyContainer;
        LC_Division division{&emptyContainer};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, false)};

        CHECK(data == nullptr);
    }

    SECTION("an interior intersection still bounds a partial segment") {
        RS_EntityContainer container;
        container.addEntity(new RS_Line{&container, RS_Vector{5.0, -5.0}, RS_Vector{5.0, 5.0}});
        LC_Division division{&container};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{2.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_TO_START);
    }
}

// The classification tests above prove LC_Division reports SEGMENT_ENTIRE. The
// tests below prove the consumer acts on it: without them, deleting the
// SEGMENT_ENTIRE guards from LC_ActionModifyBreakDivide leaves the suite green
// while #2700 returns.
namespace {

/**
 * Returns a QApplication, reusing the process-wide one if another test built it
 * first. The pointer is deliberately leaked: only one QApplication may exist at
 * a time and only one ~QApplication may run at exit, so every test file uses
 * this reuse-or-create form and none of them destroys the instance.
 */
QApplication* application() {
    static int argc = 1;
    static char name[] = "librecad_tests";
    static char* argv[] = {name, nullptr};
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
    return app;
}

class BreakDivideTestView final : public RS_GraphicView {
public:
    BreakDivideTestView() : RS_GraphicView(nullptr) {}

    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw([[maybe_unused]] RS2::RedrawMethod method = RS2::RedrawAll,
                [[maybe_unused]] bool immediately = false) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor([[maybe_unused]] RS2::CursorType cursor) override {}
    void updateGridStatusWidget([[maybe_unused]] QString status) override {}
};

/**
 * Reaches the protected segment builders, and the protected flag that SHIFT
 * sets: whole-entity selection needs m_alternativeActionMode && m_removeSegments,
 * and there is no public setter for the former.
 */
class BreakDivideProbe final : public LC_ActionModifyBreakDivide {
public:
    explicit BreakDivideProbe(LC_ActionContext* actionContext)
        : LC_ActionModifyBreakDivide(actionContext) {}

    using LC_ActionModifyBreakDivide::createEntitiesForLine;
    using LC_ActionModifyBreakDivide::createEntitiesForCircle;
    using LC_ActionModifyBreakDivide::createEntitiesForArc;
    using LC_ActionModifyBreakDivide::doCheckMayTrigger;

    void setWholeEntityMode(const bool value) { m_alternativeActionMode = value; }

    // The nested TriggerData type is private, but the member is protected; its
    // type is reachable through decltype, so no production visibility change.
    void makeTriggerData(RS_Entity* entity, const RS_Vector& snap) {
        clearTriggerData();
        using TriggerDataT = std::remove_pointer_t<decltype(m_triggerData)>;
        m_triggerData = new TriggerDataT{};
        m_triggerData->entity = entity;
        m_triggerData->snapPoint = snap;
    }
    bool triggerListIsEmpty() const { return m_triggerData->entitiesToCreate.isEmpty(); }
    void clearTriggerData() {
        delete m_triggerData;
        m_triggerData = nullptr;
    }
};

using SegmentCreation = LC_ActionModifyBreakDivide::SegmentCreation;

/**
 * Member order matters: the action is destroyed before the view, because
 * ~RS_PreviewActionInterface reaches into overlay containers the view owns.
 */
struct BreakDivideFixture {
    // Declared first on purpose: members are initialised before the constructor
    // body, and RS_Graphic's own constructor already reads RS_Settings.
    const bool qtReady{application() != nullptr};
    RS_Graphic graphic;
    BreakDivideTestView view;
    LC_ActionContext context;
    std::unique_ptr<BreakDivideProbe> action;

    explicit BreakDivideFixture(const bool removeSelected) {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        action = std::make_unique<BreakDivideProbe>(&context);
        action->setWholeEntityMode(true);  // the SHIFT modifier
        action->setRemoveSegment(true);    // break, not divide
        action->setRemoveSelected(removeSelected);
    }
};

} // namespace

TEST_CASE("whole-entity Break removes the source without replacement geometry",
          "[lc_division][break_divide][whole_entity][action]") {
    BreakDivideFixture fixture{/*removeSelected=*/true};
    QList<RS_Entity*> list;

    SECTION("line") {
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
        CHECK(fixture.action->createEntitiesForLine(&line, RS_Vector{5.0, 0.0}, list, false) == SegmentCreation::RemoveSource);
        CHECK(list.isEmpty());
    }

    SECTION("circle") {
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};
        RS_Vector snap{10.0, 0.0};
        CHECK(fixture.action->createEntitiesForCircle(&circle, snap, list, false) == SegmentCreation::RemoveSource);
        CHECK(list.isEmpty());
    }

    SECTION("arc") {
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, false}};
        RS_Vector snap{0.0, 10.0};
        CHECK(fixture.action->createEntitiesForArc(&arc, snap, list, false) == SegmentCreation::RemoveSource);
        CHECK(list.isEmpty());
    }

    qDeleteAll(list);
}

TEST_CASE("whole-entity Break that keeps the selection is a no-op, not a delete",
          "[lc_division][break_divide][whole_entity][action]") {
    // Without the second conjunct the action would report "remove the source",
    // deleting the entity the user asked to keep.
    BreakDivideFixture fixture{/*removeSelected=*/false};
    QList<RS_Entity*> list;

    SECTION("line") {
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
        CHECK(fixture.action->createEntitiesForLine(&line, RS_Vector{5.0, 0.0}, list, false) == SegmentCreation::KeepEntire);
        CHECK(list.isEmpty());
    }

    SECTION("circle") {
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};
        RS_Vector snap{10.0, 0.0};
        CHECK(fixture.action->createEntitiesForCircle(&circle, snap, list, false) == SegmentCreation::KeepEntire);
        CHECK(list.isEmpty());
    }

    SECTION("arc") {
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, false}};
        RS_Vector snap{0.0, 10.0};
        CHECK(fixture.action->createEntitiesForArc(&arc, snap, list, false) == SegmentCreation::KeepEntire);
        CHECK(list.isEmpty());
    }

    qDeleteAll(list);
}

TEST_CASE("doCheckMayTrigger accepts whole-entity removal, and only that",
          "[lc_division][break_divide][whole_entity][action]") {
    // The acceptance below is the user-visible fix for #2700: an empty
    // replacement list is normally a refusal, whole-entity removal is the one
    // exception. Nothing upstream of createEntitiesForX covered it before.
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

    SECTION("remove-selected: empty replacement list is accepted") {
        BreakDivideFixture fixture{/*removeSelected=*/true};
        fixture.action->makeTriggerData(&line, RS_Vector{5.0, 0.0});
        CHECK(fixture.action->doCheckMayTrigger());
        CHECK(fixture.action->triggerListIsEmpty());
        fixture.action->clearTriggerData();
    }

    SECTION("keep-selected: same click is refused, not treated as a removal") {
        BreakDivideFixture fixture{/*removeSelected=*/false};
        fixture.action->makeTriggerData(&line, RS_Vector{5.0, 0.0});
        CHECK_FALSE(fixture.action->doCheckMayTrigger());
        CHECK(fixture.action->triggerListIsEmpty());
        fixture.action->clearTriggerData();
    }
}

TEST_CASE("Break bounded by real intersections still creates the remaining segments",
          "[lc_division][break_divide][action]") {
    // Catches an over-broad fix: one that keys off the flags instead of off
    // SEGMENT_ENTIRE.
    BreakDivideFixture fixture{/*removeSelected=*/true};
    QList<RS_Entity*> list;

    SECTION("line") {
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{3.0, -5.0}, RS_Vector{3.0, 5.0}});
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{7.0, -5.0}, RS_Vector{7.0, 5.0}});
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
        CHECK(fixture.action->createEntitiesForLine(&line, RS_Vector{5.0, 0.0}, list, false)
              == SegmentCreation::Segments);
        CHECK(list.size() == 2);
        // #2700's signature was the right count of zero-extent remnants.
        for (const RS_Entity* e: list) {
            CHECK(e->getLength() > RS_TOLERANCE);
        }
    }

    SECTION("circle") {
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{0.0, -12.0}, RS_Vector{0.0, 12.0}});
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};
        RS_Vector snap{10.0, 0.0};
        CHECK(fixture.action->createEntitiesForCircle(&circle, snap, list, false)
              == SegmentCreation::Segments);
        CHECK(list.size() == 1);
        // A zero-sweep arc reports a full-turn length, so bound it from both
        // sides: the remainder must be a real partial arc.
        for (const RS_Entity* e: list) {
            CHECK(e->getLength() > RS_TOLERANCE);
            CHECK(e->getLength() < 2.0 * HALF_TURN * 10.0 - RS_TOLERANCE);
        }
    }

    SECTION("arc") {
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{-5.0, -12.0}, RS_Vector{-5.0, 12.0}});
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{5.0, -12.0}, RS_Vector{5.0, 12.0}});
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, false}};
        RS_Vector snap{0.0, 10.0};
        CHECK(fixture.action->createEntitiesForArc(&arc, snap, list, false)
              == SegmentCreation::Segments);
        CHECK(list.size() == 2);
        for (const RS_Entity* e: list) {
            CHECK(e->getLength() > RS_TOLERANCE);
            CHECK(e->getLength() < 2.0 * HALF_TURN * 10.0 - RS_TOLERANCE);
        }
    }

    qDeleteAll(list);
}

TEST_CASE("a single tangency does not divide a circle",
          "[lc_division][break_divide][whole_entity]") {
    // One tangent line touches the circle at exactly one point. A closed curve
    // with fewer than two distinct boundary points has no segments, so SHIFT
    // whole-entity removal must work exactly as it does with no intersections.
    RS_EntityContainer container;
    container.addEntity(new RS_Line{&container, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
    LC_Division division{&container};
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};

    SECTION("whole-entity selection allowed: the circle itself is the segment") {
        const std::unique_ptr<LC_Division::CircleSegmentData> data{
            division.findCircleSegmentBetweenIntersections(&circle, RS_Vector{-10.0, 0.0}, true)};
        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }

    SECTION("divide mode: still nothing to split") {
        const std::unique_ptr<LC_Division::CircleSegmentData> data{
            division.findCircleSegmentBetweenIntersections(&circle, RS_Vector{-10.0, 0.0}, false)};
        CHECK(data == nullptr);
    }

    SECTION("through the action: SHIFT removes the circle cleanly") {
        BreakDivideFixture fixture{/*removeSelected=*/true};
        fixture.graphic.addEntity(new RS_Line{&fixture.graphic, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
        QList<RS_Entity*> list;
        RS_Vector snap{-10.0, 0.0};
        CHECK(fixture.action->createEntitiesForCircle(&circle, snap, list, false)
              == SegmentCreation::RemoveSource);
        CHECK(list.isEmpty());
        qDeleteAll(list);
    }
}
