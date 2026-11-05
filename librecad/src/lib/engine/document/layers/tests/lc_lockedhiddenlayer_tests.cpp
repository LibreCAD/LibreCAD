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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// The two layer states are independent, and each has one contract (#3229): the entities of a
// locked layer are not modified, and what is on a hidden layer is not visible. These tests keep to
// it where it was not kept: Stretch and Quick Select modified and selected the entities of a locked
// layer, and a drawing opened with every layer locked took what was drawn onto the locked current
// layer. A hidden current layer takes what is drawn, hidden.

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QStringList>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_selection.h"

namespace {

/// keeps the command line messages of RS_DIALOGFACTORY while it lives
class CommandMessages final : public RS_DialogFactoryInterface {
public:
    QStringList m_messages;

    CommandMessages() : m_previous{RS_DialogFactory::instance()->getFactoryObject()} {
        RS_DialogFactory::instance()->setFactoryObject(this);
    }
    ~CommandMessages() override {
        RS_DialogFactory::instance()->setFactoryObject(m_previous);
    }
    CommandMessages(const CommandMessages&) = delete;
    CommandMessages& operator=(const CommandMessages&) = delete;

    void commandMessage(const QString& message) override {
        m_messages << message;
    }

private:
    RS_DialogFactoryInterface* m_previous;
};

// a line on each of "Free", "Hidden" and "Locked", with "Free" current
struct Drawing {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    RS_Layer* m_free = new RS_Layer(QStringLiteral("Free"));
    RS_Layer* m_hidden = new RS_Layer(QStringLiteral("Hidden"));
    RS_Layer* m_locked = new RS_Layer(QStringLiteral("Locked"));
    RS_Line* m_freeLine = nullptr;
    RS_Line* m_hiddenLine = nullptr;
    RS_Line* m_lockedLine = nullptr;

    Drawing() {
        m_graphic.initForNewDocument();
        m_hidden->freeze(true);
        m_locked->lock(true);
        int y = 0;
        for (auto [layer, line] : {std::pair{m_free, &m_freeLine}, std::pair{m_hidden, &m_hiddenLine},
                                   std::pair{m_locked, &m_lockedLine}}) {
            m_graphic.addLayer(layer);
            *line = new RS_Line(&m_graphic, RS_LineData(RS_Vector(0, y), RS_Vector(10, y)));
            (*line)->setLayer(layer);
            m_graphic.addEntity(*line);
            ++y;
        }
        m_graphic.activateLayer(m_free);
        m_view.setDocument(&m_graphic);
    }

    /// adds a line to the drawing as a draw tool does: to the current layer, undoably
    bool draw() {
        return m_graphic.undoableModify(m_view.getViewPort(), [this](LC_DocumentModificationBatch& ctx) {
            ctx += new RS_Line(&m_graphic, RS_LineData(RS_Vector(0, 10), RS_Vector(10, 10)));
            return true;
        });
    }

    int liveLines() const {
        int count = 0;
        for (const RS_Entity* e : m_graphic) {
            if (e != nullptr && !e->isDeleted() && e->rtti() == RS2::EntityLine) {
                ++count;
            }
        }
        return count;
    }
};

} // namespace

TEST_CASE("Stretch does not modify the entities of a locked layer", "[layers][locked][3229]") {
    Drawing d;
    LC_DocumentModificationBatch ctx;
    const QList<RS_Entity*> entities{d.m_freeLine, d.m_hiddenLine, d.m_lockedLine};
    // the whole of each line is inside the window
    RS_Modification::stretch(RS_Vector(-1, -1), RS_Vector(11, 3), RS_Vector(5, 0), entities, true, ctx);

    // a clone of the line of a layer that is not locked, which replaces it; the locked one is as it was
    CHECK(ctx.entitiesToDelete.contains(d.m_freeLine));
    CHECK_FALSE(ctx.entitiesToDelete.contains(d.m_lockedLine));
    CHECK(ctx.entitiesToAdd.size() == 2); // the hidden layer's line is not visible, not locked
    for (const RS_Entity* added : std::as_const(ctx.entitiesToAdd)) {
        CHECK(added->getLayer() != d.m_locked);
    }
    qDeleteAll(ctx.entitiesToAdd);
}

TEST_CASE("Quick select does not select the entities of a locked layer", "[layers][locked][3229]") {
    Drawing d;
    RS_Selection::ConditionalSelectionOptions options;
    options.applyArea = RS_Selection::ConditionalSelectionOptions::Document;
    options.includeIntoSelectionSet = true;
    options.funRttiMatcher = [](RS_Entity* e) { return e->rtti() == RS2::EntityLine; };
    options.funEntityMatcher = [](RS_Entity*) { return true; };

    RS_Selection(&d.m_graphic, d.m_view.getViewPort()).conditionalSelection(options);

    CHECK(d.m_freeLine->isSelected());
    CHECK_FALSE(d.m_lockedLine->isSelected());
    // a hidden layer's entities are not visible, so they are not selected either
    CHECK_FALSE(d.m_hiddenLine->isSelected());
}

TEST_CASE("Quick select of a selection drops the entities of a layer locked after they were selected", "[layers][locked][3229]") {
    Drawing d;
    RS_Selection selection(&d.m_graphic, d.m_view.getViewPort());
    selection.selectSingle(d.m_freeLine);
    REQUIRE(d.m_freeLine->isSelected());

    d.m_free->lock(true);
    RS_Selection::ConditionalSelectionOptions options;
    options.applyArea = RS_Selection::ConditionalSelectionOptions::Selection;
    options.includeIntoSelectionSet = true;
    options.funRttiMatcher = [](RS_Entity* e) { return e->rtti() == RS2::EntityLine; };
    options.funEntityMatcher = [](RS_Entity*) { return true; };
    selection.conditionalSelection(options);

    // it matches, and would stay selected, were its layer not locked
    CHECK_FALSE(d.m_freeLine->isSelected());
}

TEST_CASE("Nothing is added to a locked current layer", "[layers][locked][3229]") {
    Drawing d;
    CommandMessages command;

    SECTION("a drawing opened with every layer locked") {
        // what a file can say: layers locked, the current layer too; no layer is left to change over to
        for (RS_Layer* layer : {d.m_graphic.findLayer(QStringLiteral("0")), d.m_free, d.m_hidden, d.m_locked}) {
            layer->lock(true);
        }
        REQUIRE(d.m_graphic.getActiveLayer() == d.m_free);
        const int before = d.liveLines();
        CHECK_FALSE(d.draw());
        CHECK(d.liveLines() == before);
        REQUIRE(command.m_messages.size() == 1);
        CHECK(command.m_messages.constFirst().contains(QStringLiteral("\"Free\"")));
        CHECK(command.m_messages.constFirst().contains(QStringLiteral("locked")));
    }
    SECTION("the current layer is unlocked") {
        const int before = d.liveLines();
        CHECK(d.draw());
        CHECK(d.liveLines() == before + 1);
        CHECK(command.m_messages.isEmpty());
    }
}

TEST_CASE("What is drawn on a hidden current layer is added, and is not visible", "[layers][locked][3229]") {
    Drawing d;
    CommandMessages command;
    d.m_graphic.activateLayer(d.m_hidden);
    REQUIRE(d.m_graphic.getActiveLayer() == d.m_hidden);
    const int before = d.liveLines();

    CHECK(d.draw());

    CHECK(d.liveLines() == before + 1);
    RS_Entity* drawn = nullptr;
    for (RS_Entity* e : d.m_graphic) {
        if (e->getLayer() == d.m_hidden && e != d.m_hiddenLine) {
            drawn = e;
        }
    }
    REQUIRE(drawn != nullptr);
    CHECK_FALSE(drawn->isVisible());
    CHECK(command.m_messages.isEmpty());
}
