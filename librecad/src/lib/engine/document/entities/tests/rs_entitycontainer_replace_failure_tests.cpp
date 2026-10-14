/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
// Dedicated executable only. Never link allocator overrides into Catch/app.
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "lc_selectedset.h"
#include "rs_settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>
#include <new>

static thread_local bool armed=false, committed=false;
static thread_local std::size_t failAt=0, allocations=0, commitAllocations=0;
void *operator new(std::size_t size) {
    if (armed) {
        ++allocations;
        if (committed) ++commitAllocations;
        if (allocations==failAt) throw std::bad_alloc();
    }
    if (auto *memory=std::malloc(size?size:1)) return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p,std::size_t) noexcept { std::free(p); }
void operator delete[](void *p,std::size_t) noexcept { std::free(p); }
struct AtomicContainer : RS_EntityContainer {
    using RS_EntityContainer::RS_EntityContainer;
    using RS_EntityContainer::replaceOwnedEntities;
};
struct RetiredLine : RS_Line {
    using RS_Line::RS_Line;
    ~RetiredLine() override { if (armed) committed=true; }
};
int main(int argc,char **argv) {
    QCoreApplication application(argc,argv);
    QTemporaryDir settings;
    if (!settings.isValid()) return 40;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
    QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,settings.path());
    RS_Settings::init("LibreCAD","LibreCAD-tests");
    std::size_t provenFailures=0;
    for (std::size_t injection=1;injection<128;++injection) {
        RS_Graphic graphic;AtomicContainer target(&graphic);
        auto *old=new RetiredLine(&target,RS_LineData({1,2},{3,4}));target.addEntity(old);graphic.select(QList<RS_Entity*>{old});
        std::vector<std::unique_ptr<RS_Entity>> input;
        input.push_back(std::make_unique<RS_Line>(nullptr,RS_LineData({100,200},{110,220})));
        auto *incoming=input.front().get();
        failAt=injection;allocations=commitAllocations=0;committed=false;armed=true;
        bool threw=false;
        try {
            auto notification=target.replaceOwnedEntities(std::move(input));
            armed=false;notification.notify();
        } catch (const std::bad_alloc &) { armed=false;threw=true; }
        if (threw) {
            ++provenFailures;
            if (committed||target.first()!=old||target.getMin()!=RS_Vector(1,2)||target.getMax()!=RS_Vector(3,4)||
                !old->getFlag(RS2::FlagSelected)||graphic.getSelection()->isEmpty()||
                input.front().get()!=incoming||incoming->getParent()!=nullptr) return 10;
        } else {
            std::printf("precommit_std_new_failures=%zu successful_prepare_allocations=%zu postcommit_std_new=%zu\n",
                        provenFailures,allocations,commitAllocations);
            // This injects C++ new, not Qt's private malloc. Qt list reserve/
            // detach sites are additionally ordered before commit by inspection.
            return provenFailures>0&&committed&&commitAllocations==0&&target.first()==incoming&&
                graphic.getSelection()->isEmpty()?0:20;
        }
    }
    return 30;
}
