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
******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <QKeyEvent>

#include "lc_action_modify_move.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

class MoveAction final : public LC_ActionModifyMove {
public:
    explicit MoveAction(LC_ActionContext* context) : LC_ActionModifyMove(context) {}
    using LC_ActionModifyMove::SetReferencePoint;
    using LC_ActionModifyMove::SetTargetPoint;

    void click(const RS_Vector& point, const bool control = false) {
        LC_MouseEvent event = lc::test::eventAt(point.x, point.y);
        event.isControl = control;
        onMouseLeftButtonRelease(getStatus(), &event);
    }
    // the right mouse button, which RS_EventHandler::back() also sends
    void rightClick() {
        const LC_MouseEvent event = lc::test::eventAt(0., 0.);
        onMouseRightButtonRelease(getStatus(), &event);
    }
    void escape() {
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        keyPressEvent(&event);
    }
};

// keeps what the action last asked for on the command line
class PromptContext final : public LC_ActionContext {
public:
    QString prompt;
    QString rightButton;

    void updateActionPrompt(const QString& left, const QString& right,
                            [[maybe_unused]] const LC_ModifiersInfo& modifiers) override {
        prompt = left;
        rightButton = right;
    }
};

// lc::test::ActionFixture, with the prompt kept
struct MoveFixtureBase {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    PromptContext m_context;
    std::unique_ptr<MoveAction> m_action;

    MoveFixtureBase() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<MoveAction>(&m_context);
    }

    // selects the source, starts a plain move of it and picks the reference point
    void start(RS_Entity* source, const RS_Vector& reference) {
        m_graphic.addEntity(source);
        m_graphic.select(QList<RS_Entity*>{source});
        m_action->init(0);
        m_action->setKeepOriginals(false);
        m_action->setUseMultipleCopies(false);
        m_action->setCopiesNumber(1);
        m_action->setUseCurrentLayer(false);
        m_action->setUseCurrentAttributes(false);
        m_action->click(reference);
    }
};

struct MoveFixture : MoveFixtureBase {
    RS_Line* source;

    MoveFixture() {
        source = new RS_Line(&m_graphic, RS_LineData(RS_Vector{2., 3.}, RS_Vector{5., 7.}));
        start(source, {2., 3.});
    }
};

// the drawing of issue #3171: a circle, moved from its centre
struct CircleFixture : MoveFixtureBase {
    RS_Circle* circle;

    CircleFixture() {
        circle = new RS_Circle(&m_graphic, RS_CircleData(RS_Vector{0., 0.}, 2.));
        start(circle, circle->getCenter());
    }
};

// sets the option for one test; the settings exist once the application does
struct KeepModifiedSelected {
    const bool m_qtReady{lc::test::application() != nullptr};

    explicit KeepModifiedSelected(const bool keep) { LC_SET_ONE("Modify", "KeepModifiedSelected", keep); }
    ~KeepModifiedSelected() { LC_SET_ONE("Modify", "KeepModifiedSelected", true); }
};

} // namespace

TEST_CASE("Move/Copy preserves selection when started from an entity context menu", "[modify][move][3083]") {
    for (const bool selected : {false, true}) {
        CAPTURE(selected);
        lc::test::ActionFixture<MoveAction> f;
        auto* source = new RS_Line(&f.m_graphic, RS_LineData(RS_Vector{2., 3.}, RS_Vector{5., 7.}));
        f.m_graphic.addEntity(source);
        if (selected) f.m_graphic.select(QList<RS_Entity*>{source});
        f.m_context.saveContextMenuActionContext(source, source->getStartpoint(), false);
        f.m_action->init(0);

        CHECK(source->isSelected());
        CHECK(f.m_graphic.hasSelection());
        CHECK_FALSE(f.m_action->isFinished());
        f.m_action->click(source->getStartpoint());
        CHECK(f.m_action->getStatus() == MoveAction::SetTargetPoint);
        CHECK(source->isSelected());
    }
}

TEST_CASE("Move/Copy keeps Ctrl-copy active for repeated target clicks", "[modify][move][3080]") {
    MoveFixture f;
    MoveAction& action = *f.m_action;
    REQUIRE(action.getStatus() == MoveAction::SetTargetPoint);
    unsigned copies = 0;
    for (const RS_Vector target : {RS_Vector{12., 8.}, RS_Vector{22., 3.}, RS_Vector{-8., 13.}}) {
        action.click(target, true);
        REQUIRE_FALSE(action.isFinished());
        CHECK(action.getStatus() == MoveAction::SetTargetPoint);
        ++copies;
        REQUIRE(f.m_graphic.count() == copies + 1);
        const RS_Entity* copy = f.m_graphic.lastEntity();
        CHECK(copy->getStartpoint().distanceTo(target) < 1e-9);
        CHECK(copy->getEndpoint().distanceTo(target + RS_Vector{3., 4.}) < 1e-9);
        CHECK_FALSE(f.source->isDeleted());
        CHECK(f.source->getStartpoint().distanceTo(RS_Vector{2., 3.}) < 1e-9);
        CHECK_FALSE(action.isKeepOriginals());
        CHECK(f.m_view.getViewPort()->getRelativeZero().distanceTo(RS_Vector{2., 3.}) < 1e-9);
    }

    const RS_Entity* lastCopy = f.m_graphic.lastEntity();
    REQUIRE(f.m_graphic.undo());
    CHECK(lastCopy->isDeleted());
    CHECK_FALSE(f.source->isDeleted());
    REQUIRE(f.m_graphic.redo());
    CHECK_FALSE(lastCopy->isDeleted());
    CHECK(lastCopy->getStartpoint().distanceTo(RS_Vector{-8., 13.}) < 1e-9);
}

TEST_CASE("Move/Copy returns to moving when a target click no longer holds Ctrl", "[modify][move][3080]") {
    MoveFixture f;
    MoveAction& action = *f.m_action;
    action.click({12., 3.}, true);
    REQUIRE_FALSE(action.isFinished());
    REQUIRE(f.m_graphic.count() == 2);
    const RS_Entity* copy = f.m_graphic.lastEntity();

    action.click({22., 3.});
    CHECK(action.isFinished());
    REQUIRE(f.m_graphic.count() == 3);
    CHECK(f.source->isDeleted());
    CHECK_FALSE(copy->isDeleted());
    CHECK(copy->getStartpoint().distanceTo(RS_Vector{12., 3.}) < 1e-9);
    CHECK(f.m_graphic.lastEntity()->getStartpoint().distanceTo(RS_Vector{22., 3.}) < 1e-9);
}

TEST_CASE("Move/Copy without Ctrl still finishes after one target click", "[modify][move][3080]") {
    MoveFixture f;
    bool keepOriginals = false;
    SECTION("move") {}
    SECTION("copy through the tool options") { keepOriginals = true; }
    f.m_action->setKeepOriginals(keepOriginals);
    f.m_action->click({12., 3.});

    CHECK(f.m_action->isFinished());
    REQUIRE(f.m_graphic.count() == 2);
    CHECK(f.source->isDeleted() == !keepOriginals);
    CHECK(f.m_graphic.lastEntity()->getStartpoint().distanceTo(RS_Vector{12., 3.}) < 1e-9);
}

TEST_CASE("Move/Copy finishes on the right button after Ctrl copies", "[modify][move][3171]") {
    for (const bool keepSelected : {true, false}) {
        CAPTURE(keepSelected);
        KeepModifiedSelected setting(keepSelected);
        CircleFixture f;
        MoveAction& action = *f.m_action;
        const std::vector<RS_Vector> targets{{10., 0.}, {20., 0.}, {0., 10.}};
        for (const RS_Vector& target : targets) {
            action.click(target, true);
        }
        REQUIRE_FALSE(action.isFinished());
        REQUIRE(f.m_graphic.count() == 4);

        action.rightClick();
        CHECK(action.isFinished());
        CHECK(f.m_context.prompt != "Specify reference point");
        // what the reporter did next: were the action still running, these
        // two picks moved the original circle
        if (!action.isFinished()) {
            action.click({0., 0.});
            action.click({-30., 0.});
        }
        CHECK(f.m_graphic.count() == 4);
        CHECK_FALSE(f.circle->isDeleted());
        CHECK(f.circle->getCenter().distanceTo(RS_Vector{0., 0.}) < 1e-9);
        // the copies are what the action made; the original is left as it was
        CHECK(f.circle->isSelected() == !keepSelected);
        for (std::size_t i = 0; i < targets.size(); ++i) {
            CAPTURE(i);
            const auto* copy = static_cast<RS_Circle*>(f.m_graphic.entityAt(static_cast<int>(i) + 1));
            CHECK_FALSE(copy->isDeleted());
            CHECK(copy->getCenter().distanceTo(targets[i]) < 1e-9);
            CHECK(copy->isSelected() == keepSelected);
        }
    }
}

TEST_CASE("Move/Copy offers the right button to finish once a Ctrl copy is made", "[modify][move][3171]") {
    MoveFixture f;
    CHECK(f.m_context.prompt == "Specify target point");
    CHECK(f.m_context.rightButton == "Back");
    f.m_action->click({12., 8.}, true);
    REQUIRE_FALSE(f.m_action->isFinished());
    CHECK(f.m_context.prompt == "Specify target point");
    CHECK(f.m_context.rightButton == "Finish");
}

TEST_CASE("Move/Copy right button still goes back to the reference point before any copy", "[modify][move][3171]") {
    MoveFixture f;
    MoveAction& action = *f.m_action;
    action.rightClick();
    CHECK_FALSE(action.isFinished());
    CHECK(action.getStatus() == MoveAction::SetReferencePoint);
    CHECK(f.m_context.prompt == "Specify reference point");
    CHECK(f.source->isSelected());

    action.click({5., 7.});
    action.click({15., 7.});
    CHECK(action.isFinished());
    REQUIRE(f.m_graphic.count() == 2);
    CHECK(f.source->isDeleted());
    CHECK(f.m_graphic.lastEntity()->getStartpoint().distanceTo(RS_Vector{12., 3.}) < 1e-9);
}

TEST_CASE("Move/Copy Escape after Ctrl copies keeps the copies", "[modify][move][3171]") {
    MoveFixture f;
    f.m_action->click({12., 8.}, true);
    REQUIRE(f.m_graphic.count() == 2);
    const RS_Entity* copy = f.m_graphic.lastEntity();

    f.m_action->escape();
    CHECK(f.m_action->isFinished());
    CHECK(f.m_graphic.count() == 2);
    CHECK_FALSE(copy->isDeleted());
    CHECK(copy->isSelected());
    CHECK_FALSE(f.source->isDeleted());
    CHECK(f.source->getStartpoint().distanceTo(RS_Vector{2., 3.}) < 1e-9);
}
