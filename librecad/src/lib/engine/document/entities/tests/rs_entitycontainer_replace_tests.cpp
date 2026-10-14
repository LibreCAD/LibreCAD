/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "lc_selectedset.h"
#include "lc_selectedsetlistener.h"
#include "rs_settings.h"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

namespace {
void initializeSettings() {
    if (!RS_Settings::instance()) RS_Settings::init("LibreCAD","LibreCAD-tests");
}
struct AtomicContainer : RS_EntityContainer {
    using RS_EntityContainer::RS_EntityContainer;
    using RS_EntityContainer::replaceOwnedEntities;
};
std::vector<std::unique_ptr<RS_Entity>> replacement() {
    std::vector<std::unique_ptr<RS_Entity>> result;
    result.push_back(std::make_unique<RS_Line>(nullptr,RS_LineData({100,200},{110,220})));
    return result;
}
struct Observer : LC_SelectedSetListener {
    AtomicContainer *target=nullptr;
    LC_SelectedSet *selection=nullptr;
    int calls=0;
    void selectionChanged() override {
        ++calls;
        CHECK(target->count()==1);
        CHECK(target->getMin()==RS_Vector(100,200));
        CHECK(target->getMax()==RS_Vector(110,220));
        CHECK(target->first()->getParent()==target);
        CHECK(selection->isEmpty());
    }
};
}

TEST_CASE("Owned replacement atomically publishes bounds and cleans selected descendants", "[container][native-prerequisite]") {
    initializeSettings();
    RS_Graphic graphic;
    AtomicContainer target(&graphic);
    auto group=std::make_unique<RS_EntityContainer>(&target);
    auto *oldLeaf=new RS_Line(group.get(),RS_LineData({1,2},{3,4}));
    group->addEntity(oldLeaf);group->forcedCalculateBorders();
    target.addEntity(group.release());target.forcedCalculateBorders();
    graphic.select(QList<RS_Entity*>{oldLeaf});REQUIRE_FALSE(graphic.getSelection()->isEmpty());
    // Prime the deep-iteration cached subcontainer with retired geometry.
    REQUIRE(target.firstEntity(RS2::ResolveAll)==oldLeaf);
    Observer observer;observer.target=&target;observer.selection=graphic.getSelection();
    graphic.getSelection()->addListener(&observer);
    auto input=replacement();const auto *incoming=input.front().get();
    auto notification=target.replaceOwnedEntities(std::move(input));
    CHECK(observer.calls==0);CHECK(graphic.getSelection()->isEmpty());
    CHECK(target.count()==1);CHECK(target.first()==incoming);CHECK(target.first()->getParent()==&target);
    CHECK(target.firstEntity(RS2::ResolveAll)==incoming);
    CHECK(target.getMin()==RS_Vector(100,200));CHECK(target.getMax()==RS_Vector(110,220));
    notification.notify();CHECK(observer.calls==1);
    notification.notify();CHECK(observer.calls==1); // Consumed notification.
}

TEST_CASE("Owned replacement preserves silent selection nesting and empty/no-owner contracts", "[container][native-prerequisite]") {
    initializeSettings();
    RS_Graphic graphic;AtomicContainer target(&graphic);
    auto *old=new RS_Line(&target,RS_LineData({1,2},{3,4}));target.addEntity(old);graphic.select(QList<RS_Entity*>{old});
    Observer observer;observer.target=&target;observer.selection=graphic.getSelection();graphic.getSelection()->addListener(&observer);
    graphic.getSelection()->disableListeners();graphic.getSelection()->disableListeners();
    auto input=replacement();auto notification=target.replaceOwnedEntities(std::move(input));notification.notify();
    CHECK(observer.calls==0);graphic.getSelection()->enableListeners();CHECK(observer.calls==0);
    graphic.getSelection()->enableListeners();CHECK(observer.calls==1);
    graphic.getSelection()->removeListener(&observer);
    std::vector<std::unique_ptr<RS_Entity>> empty;
    auto emptyNotification=target.replaceOwnedEntities(std::move(empty));emptyNotification.notify();
    CHECK(target.isEmpty());CHECK_FALSE(target.hasValidBorders());
    AtomicContainer borrowed(nullptr,false);auto refused=replacement();const auto *pointer=refused.front().get();
    CHECK_THROWS_AS(borrowed.replaceOwnedEntities(std::move(refused)),std::invalid_argument);
    CHECK(refused.front().get()==pointer);CHECK(refused.front()->getParent()==nullptr);CHECK(borrowed.isEmpty());
    AtomicContainer parented;refused.front()->setParent(&parented);
    CHECK_THROWS_AS(target.replaceOwnedEntities(std::move(refused)),std::invalid_argument);
    CHECK(refused.front().get()==pointer);CHECK(target.isEmpty());
    refused.front()->setParent(nullptr);
}

TEST_CASE("Owned replacement does not retire selected borrowed descendants", "[container][native-prerequisite][borrowed]") {
    initializeSettings();
    RS_Graphic graphic;
    auto *survivor=new RS_Line(&graphic,RS_LineData({1,2},{3,4}));graphic.addEntity(survivor);
    AtomicContainer target(&graphic);
    auto *borrower=new RS_EntityContainer(&target,false);borrower->addEntity(survivor);target.addEntity(borrower);
    graphic.select(QList<RS_Entity*>{survivor});REQUIRE(survivor->getFlag(RS2::FlagSelected));
    auto input=replacement();auto notification=target.replaceOwnedEntities(std::move(input));notification.notify();
    CHECK(survivor->getFlag(RS2::FlagSelected));REQUIRE_FALSE(graphic.getSelection()->isEmpty());
    CHECK(graphic.getSelection()->first()==survivor);CHECK(survivor->getParent()==&graphic);
    CHECK(graphic.first()==survivor);
}

TEST_CASE("Replacement notification is postcommit even when listeners throw or reenter", "[container][native-prerequisite][notification]") {
    initializeSettings();
    struct ThrowingObserver : LC_SelectedSetListener {
        AtomicContainer *target=nullptr;bool published=false;int calls=0;
        void selectionChanged() override {
            ++calls;CHECK(published);CHECK(target->getMin()==RS_Vector(100,200));
            CHECK(target->first()->getParent()==target);throw std::runtime_error("postcommit notification");
        }
    };
    struct ReentrantObserver : LC_SelectedSetListener {
        AtomicContainer *target=nullptr;int calls=0;
        void selectionChanged() override {
            ++calls;CHECK(target->count()==1);CHECK(target->getMin()==RS_Vector(100,200));
            std::vector<std::unique_ptr<RS_Entity>> empty;
            auto notification=target->replaceOwnedEntities(std::move(empty));notification.notify();
            CHECK(target->isEmpty());CHECK_FALSE(target->hasValidBorders());
        }
    };
    RS_Graphic graphic;AtomicContainer target(&graphic);
    auto selectedOld=[&] {
        auto *old=new RS_Line(&target,RS_LineData({1,2},{3,4}));target.addEntity(old);
        graphic.select(QList<RS_Entity*>{old});
    };
    selectedOld();
    ThrowingObserver throwing;throwing.target=&target;graphic.getSelection()->addListener(&throwing);
    auto input=replacement();auto notification=target.replaceOwnedEntities(std::move(input));
    throwing.published=true;
    CHECK_THROWS_AS(notification.notify(),std::runtime_error);CHECK(throwing.calls==1);
    CHECK(target.getMin()==RS_Vector(100,200));CHECK(graphic.getSelection()->isEmpty());
    notification.notify();CHECK(throwing.calls==1);graphic.getSelection()->removeListener(&throwing);
    target.clear();selectedOld();
    ReentrantObserver reentrant;reentrant.target=&target;graphic.getSelection()->addListener(&reentrant);
    input=replacement();auto next=target.replaceOwnedEntities(std::move(input));next.notify();
    CHECK(reentrant.calls==1);CHECK(target.isEmpty());CHECK(graphic.getSelection()->isEmpty());
}
