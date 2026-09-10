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
******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <QApplication>

#include "rs_graphic.h"
#include "rs_constructionline.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_settings.h"
#include "rs_text.h"

namespace {

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
    (void) settingsReady;
    return app;
}

RS_TextData textData(const RS_Vector& insertionPoint) {
    return {insertionPoint, insertionPoint, 1.0, 1.0,
            RS_TextData::VABaseline, RS_TextData::HALeft, RS_TextData::None,
            QStringLiteral("T"), QStringLiteral("standard"), 0.0, RS2::NoUpdate};
}

RS_MTextData mtextData(const RS_Vector& insertionPoint) {
    return {insertionPoint, 1.0, 0.0,
            RS_MTextData::VATop, RS_MTextData::HALeft,
            RS_MTextData::LeftToRight, RS_MTextData::AtLeast, 1.0,
            QStringLiteral("M"), QStringLiteral("standard"), 0.0, RS2::NoUpdate};
}

} // namespace

TEST_CASE("snap candidate index returns only local entities in drawing order",
          "[snap][candidates]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* first = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    auto* second = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    graphic.addEntity(first);
    graphic.addEntity(second);
    graphic.addEntity(new RS_Line(nullptr, RS_Vector{100.0, 0.0}, RS_Vector{110.0, 0.0}));

    const QList<RS_Entity*> candidates = graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0);

    REQUIRE(candidates.size() == 2);
    CHECK(candidates.at(0) == first);
    CHECK(candidates.at(1) == second);
}

TEST_CASE("snap candidate index includes Text and MText insertion anchors",
          "[snap][text][mtext]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* text = new RS_Text(nullptr, textData({10.0, 20.0}));
    auto* mtext = new RS_MText(nullptr, mtextData({30.0, 40.0}));
    graphic.addEntity(text);
    graphic.addEntity(mtext);

    CHECK(graphic.getSnapCandidates(RS_Vector{10.0, 20.0}, 0.0).contains(text));
    CHECK(graphic.getSnapCandidates(RS_Vector{30.0, 40.0}, 0.0).contains(mtext));
}

TEST_CASE("snap candidate index is invalidated when document entities change",
          "[snap][candidates]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    REQUIRE(graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0).size() == 1);

    graphic.clear();
    CHECK(graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0).empty());

    auto* replacement = new RS_Line(nullptr, RS_Vector{20.0, 0.0}, RS_Vector{30.0, 0.0});
    graphic.addEntity(replacement);
    CHECK(graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0).empty());
    CHECK(graphic.getSnapCandidates(RS_Vector{25.0, 0.0}, 1.0).size() == 1);

    replacement->move(RS_Vector{20.0, 0.0});
    graphic.invalidateSnapIndex();
    CHECK(graphic.getSnapCandidates(RS_Vector{25.0, 0.0}, 1.0).empty());
    CHECK(graphic.getSnapCandidates(RS_Vector{45.0, 0.0}, 1.0).size() == 1);
}

TEST_CASE("snap candidate index retains unbounded construction lines", "[snap][candidates]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* constructionLine = new RS_ConstructionLine(nullptr, {{0.0, 0.0}, {1.0, 0.0}});
    graphic.addEntity(constructionLine);

    const QList<RS_Entity*> candidates = graphic.getSnapCandidates({1000.0, 0.0}, 1.0);

    CHECK(candidates.contains(constructionLine));
}

TEST_CASE("snap candidate index retains construction-layer lines", "[snap][candidates]") {
    (void)application();
    RS_Layer constructionLayer(QStringLiteral("construction"));
    RS_Graphic graphic;
    graphic.initForNewDocument();
    REQUIRE(constructionLayer.setConstruction(true));
    auto* line = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{1.0, 0.0});
    line->setLayer(&constructionLayer);
    graphic.addEntity(line);

    const QList<RS_Entity*> candidates = graphic.getSnapCandidates({1000.0, 0.0}, 1.0);

    CHECK(candidates.contains(line));
}

TEST_CASE("snap candidate index follows document draw order changes", "[snap][candidates]") {
    (void)application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* first = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    auto* second = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    graphic.addEntity(first);
    graphic.addEntity(second);
    REQUIRE(graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0) == QList<RS_Entity*>{first, second});

    QList<RS_Entity*> moved{first};
    graphic.moveEntity(graphic.count() + 1, moved);

    CHECK(graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0) == QList<RS_Entity*>{second, first});
}

TEST_CASE("ResolveAllButTexts never descends into text geometry",
          "[snap][text][mtext]") {
    RS_EntityContainer container(nullptr);
    auto* text = new RS_Text(&container, textData({0.0, 0.0}));
    text->addEntity(new RS_Line(text, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}));
    text->forcedCalculateBorders();
    container.addEntity(text);

    RS_Entity* nearest = nullptr;
    const double distance = container.getDistanceToPoint(RS_Vector{5.0, 0.0}, &nearest,
                                                         RS2::ResolveAllButTexts);

    CHECK(distance == RS_MAXDOUBLE);
    CHECK(nearest == nullptr);
}

TEST_CASE("snap candidate index scales with nearby Text and MText bounds",
          "[snap][text][mtext][.performance]") {
    (void)application();
    RS_Graphic graphic;
    auto* line = new RS_Line(nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0});
    graphic.addEntity(line);
    for (int i = 1; i <= 1000; ++i) {
        graphic.addEntity(new RS_Text(nullptr, textData({double(i) * 100.0, 0.0})));
        graphic.addEntity(new RS_MText(nullptr, mtextData({double(i) * 100.0, 10.0})));
    }

    const QList<RS_Entity*> candidates = graphic.getSnapCandidates(RS_Vector{5.0, 0.0}, 1.0);

    REQUIRE(candidates.size() == 1);
    CHECK(candidates.front() == line);
}
