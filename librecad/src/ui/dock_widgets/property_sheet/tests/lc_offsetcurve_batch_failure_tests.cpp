/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
// Dedicated executable ONLY. Never link this main/global new into Catch/app.
#include "lc_offsetcurvepropertybatch.h"
#include "lc_graphicviewport.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>

static thread_local bool armed = false, hit = false;
static thread_local std::size_t allocations = 0, failAt = 0;
void* operator new(std::size_t size) {
    if (armed && ++allocations == failAt) { hit = true; throw std::bad_alloc(); }
    if (auto* memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using Batch = LC_OffsetCurvePropertyBatch;
class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
    using RS_Graphic::countRedoCycles;
    int nesting() const { return m_observedNesting; }
protected:
    void startUndoCycle() override {
        RS_Document::startUndoCycle();
        ++m_observedNesting;
    }
    void endUndoCycle() override {
        --m_observedNesting;
        RS_Document::endUndoCycle();
    }
private:
    int m_observedNesting = 0;
};
struct Entry { std::unique_ptr<LC_OffsetCurve> candidate; };
struct Fixture {
    Graphic graphic;
    LC_GraphicViewport viewport;
    std::map<const RS_Entity*, Entry> pending;
    std::array<LC_OffsetCurve*, 2> originals{}, candidates{};
    std::uint64_t epoch = 17;
    std::uint64_t undoEpoch = 0;
    bool originalBorders = false;
    Fixture() {
        graphic.initForNewDocument();
        originalBorders = graphic.getAutoUpdateBorders();
        for (unsigned i = 0; i < 2; ++i) {
            LC_EllipseOffsetDefinition definition;
            const RS_Vector center{3. * i, 0};
            definition.basis = RS_EllipseData{center, {1, 0}, 1, 0, .1, false};
            definition.distance = .25; definition.relTolerance = .01;
            const RS_Line line{nullptr, center + RS_Vector{1.25, 0},
                center + RS_Vector{1.25 * std::cos(.1), 1.25 * std::sin(.1)}};
            auto native = LC_OffsetCurve::adopt(line, definition, .01);
            if (!native) throw std::runtime_error("actual C adoption failed");
            native->setParent(&graphic); native->setLayer(graphic.getActiveLayer());
            originals[i] = native.release(); graphic.addEntity(originals[i]);
            auto candidate = std::unique_ptr<LC_OffsetCurve>(static_cast<LC_OffsetCurve*>(originals[i]->clone()));
            auto pen = candidate->getPen(false); pen.setWidth(RS2::Width07); candidate->setPen(pen);
            candidates[i] = candidate.get();
            pending.emplace(originals[i], Entry{std::move(candidate)});
        }
        graphic.select(QList<RS_Entity*>{originals[0]});
        undoEpoch = graphic.undoStateEpoch();
    }
    bool untouched(int listeners = 0) const {
        if (epoch != 17 || graphic.undoStateEpoch() != undoEpoch ||
            graphic.nesting() != 0 || graphic.getAutoUpdateBorders() != originalBorders ||
            const_cast<Graphic&>(graphic).countUndoCycles() != 0 || pending.size() != 2 ||
            graphic.getSelection()->listenerCount() != listeners ||
            !originals[0]->isSelected() || originals[1]->isSelected()) return false;
        for (unsigned i = 0; i < 2; ++i) {
            const auto found = pending.find(originals[i]);
            if (found == pending.end() || found->second.candidate.get() != candidates[i] ||
                originals[i]->isDeleted() || originals[i]->getPen(false).getWidth() == RS2::Width07) return false;
        }
        return true;
    }
};
void arm(std::size_t ordinal = 0) { allocations = 0; failAt = ordinal; hit = false; armed = true; }
int error(int code, const char* reason) {
    armed = false;
    std::fprintf(stderr, "native-batch-failure: %s code=%d\n", reason, code);
    return code;
}
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    QTemporaryDir settings;
    if (!settings.isValid()) return 40;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settings.path());
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    RS_DEBUG->setLevel(RS_Debug::D_WARNING);
    try {
        Fixture f;
        arm();
        auto observed = Batch::tryPrepare(f.pending, f.epoch, f.graphic, f.viewport);
        armed = false;
        const auto prepareAllocations = allocations;
        if (!observed || !f.untouched(1) || observed->size() != 2 || observed->selectedCount() != 1 ||
            prepareAllocations == 0 || prepareAllocations > 64) {
            std::fprintf(stderr, "prepared=%d untouched=%d listeners=%d allocations=%zu\n", int(bool(observed)),
                int(f.untouched(1)), f.graphic.getSelection()->listenerCount(), prepareAllocations);
            return error(10, "preparation census");
        }
        observed.reset();
        for (std::size_t ordinal = 1; ordinal <= prepareAllocations; ++ordinal) {
            Fixture fresh;
            arm(ordinal);
            auto failed = Batch::tryPrepare(fresh.pending, fresh.epoch, fresh.graphic, fresh.viewport);
            armed = false;
            if (!hit || failed || !fresh.untouched()) return error(11, "preparation must retain both owners");
        }

        // Undo nodes, history finalization and selection are now staged too.
        // Nothing in the real publication path may allocate after preparation.
        auto prepared = Batch::tryPrepare(f.pending, f.epoch, f.graphic, f.viewport);
        if (!prepared) return error(14, "retry preparation");
        arm(1);
        const auto outcome = prepared->execute();
        armed = false;
        if (hit || allocations || outcome != Batch::Outcome::Committed ||
            !prepared->handedOff() || f.epoch != 18 || f.graphic.nesting() != 0 ||
            f.graphic.countUndoCycles() != 1) return error(16, "allocation-free actual publication");
        for (unsigned i = 0; i < 2; ++i) {
            if (f.pending.at(f.originals[i]).candidate || !f.originals[i]->isDeleted() ||
                f.graphic.findEntityIndex(f.candidates[i]) < 0 ||
                f.candidates[i]->isSelected() != (i == 0)) return error(17, "published ownership/selection");
        }
        if (!f.graphic.undo() || f.originals[0]->isDeleted() || f.originals[1]->isDeleted() ||
            f.graphic.undo()) return error(18, "one-step undo");

        // A refused preparation must preserve the complete redo tail. A later
        // success prunes only the obsolete, deleted replacement entities.
        std::map<const RS_Entity*, Entry> next;
        std::array<LC_OffsetCurve*, 2> nextAddresses{};
        for (unsigned i = 0; i < 2; ++i) {
            auto candidate = std::unique_ptr<LC_OffsetCurve>(static_cast<LC_OffsetCurve*>(f.originals[i]->clone()));
            nextAddresses[i] = candidate.get();
            next.emplace(f.originals[i], Entry{std::move(candidate)});
        }
        const auto beforeRetryEpoch = f.graphic.undoStateEpoch();
        const auto listeners = f.graphic.getSelection()->listenerCount();
        arm();
        auto redoObserved = Batch::tryPrepare(next, f.epoch, f.graphic, f.viewport);
        armed = false;
        const auto redoAllocations = allocations;
        if (!redoObserved || redoAllocations == 0 || redoAllocations > 128) return error(22, "redo preparation census");
        redoObserved.reset();
        for (std::size_t ordinal = 1; ordinal <= redoAllocations; ++ordinal) {
            Fixture fresh;
            auto first = Batch::tryPrepare(fresh.pending, fresh.epoch, fresh.graphic, fresh.viewport);
            if (!first || first->execute() != Batch::Outcome::Committed || !fresh.graphic.undo())
                return error(31, "fresh redo setup");
            std::map<const RS_Entity*, Entry> freshNext;
            for (auto* original : fresh.originals)
                freshNext.emplace(original, Entry{std::unique_ptr<LC_OffsetCurve>(static_cast<LC_OffsetCurve*>(original->clone()))});
            const auto freshEpoch = fresh.graphic.undoStateEpoch();
            const auto freshListeners = fresh.graphic.getSelection()->listenerCount();
            arm(ordinal);
            auto failed = Batch::tryPrepare(freshNext, fresh.epoch, fresh.graphic, fresh.viewport);
            armed = false;
            if (!hit || failed || fresh.graphic.undoStateEpoch() != freshEpoch ||
                fresh.graphic.countUndoCycles() != 0 || fresh.graphic.countRedoCycles() != 1 || fresh.graphic.size() != 4 ||
                fresh.graphic.getSelection()->listenerCount() != freshListeners ||
                !freshNext.at(fresh.originals[0]).candidate || !freshNext.at(fresh.originals[1]).candidate ||
                fresh.originals[0]->isDeleted() || fresh.originals[1]->isDeleted())
                return error(23, "redo tail changed by preparation failure");
        }
        if (f.graphic.undoStateEpoch() != beforeRetryEpoch || f.graphic.getSelection()->listenerCount() != listeners)
            return error(32, "redo observation must not mutate its source");
        auto redoReady = Batch::tryPrepare(next, f.epoch, f.graphic, f.viewport);
        if (!redoReady) return error(24, "redo retry");
        arm(1);
        const auto redoResult = redoReady->execute();
        armed = false;
        if (hit || allocations || redoResult != Batch::Outcome::Committed || f.graphic.size() != 4 ||
            f.graphic.countUndoCycles() != 1 || f.graphic.countRedoCycles() != 0 ||
            f.graphic.findEntityIndex(f.candidates[0]) >= 0 || f.graphic.findEntityIndex(f.candidates[1]) >= 0 ||
            f.graphic.findEntityIndex(nextAddresses[0]) < 0 || f.graphic.findEntityIndex(nextAddresses[1]) < 0)
            return error(25, "atomic redo pruning");

        for (std::size_t ordinal = 1; ordinal <= 8; ++ordinal) {
            Fixture fresh;
            auto ready = Batch::tryPrepare(fresh.pending, fresh.epoch, fresh.graphic, fresh.viewport);
            if (!ready) return error(19, "publication preparation");
            arm(ordinal);
            const auto result = ready->execute();
            armed = false;
            if (hit || allocations || result != Batch::Outcome::Committed || !ready->handedOff() ||
                fresh.graphic.countUndoCycles() != 1 || fresh.graphic.nesting() != 0 ||
                !fresh.originals[0]->isDeleted() || !fresh.originals[1]->isDeleted() ||
                fresh.graphic.findEntityIndex(fresh.candidates[0]) < 0 ||
                fresh.graphic.findEntityIndex(fresh.candidates[1]) < 0 ||
                !fresh.candidates[0]->isSelected() || fresh.candidates[1]->isSelected())
                return error(20, "post-preparation allocation ordinal");
            if (!fresh.graphic.undo() || fresh.originals[0]->isDeleted() || fresh.originals[1]->isDeleted() ||
                !fresh.graphic.redo() || !fresh.originals[0]->isDeleted() || !fresh.originals[1]->isDeleted())
                return error(21, "undo redo after allocation-free publication");
        }

        Fixture stale;
        auto staleReady = Batch::tryPrepare(stale.pending, stale.epoch, stale.graphic, stale.viewport);
        if (!staleReady) return error(26, "stale preparation");
        stale.graphic.select(QList<RS_Entity*>{stale.originals[1]}, true);
        if (staleReady->execute() != Batch::Outcome::Refused || staleReady->handedOff() ||
            stale.graphic.countUndoCycles() || stale.originals[0]->isDeleted() || stale.originals[1]->isDeleted() ||
            !stale.pending.at(stale.originals[0]).candidate || !stale.pending.at(stale.originals[1]).candidate)
            return error(27, "stale publication must retain both owners");

        struct ThrowingObserver final : LC_DocumentModificationListener {
            int modified = 0, undo = 0;
            void graphicModified(const RS_Graphic*, bool) override { ++modified; throw std::bad_alloc(); }
            void undoStateChanged(const RS_Document*, bool, bool) override { ++undo; throw std::bad_alloc(); }
        } observer;
        Fixture notified;
        notified.graphic.setModificationListener(&observer, true);
        auto notificationReady = Batch::tryPrepare(notified.pending, notified.epoch, notified.graphic, notified.viewport);
        if (!notificationReady || notificationReady->execute() != Batch::Outcome::Committed ||
            notified.graphic.countUndoCycles() != 1 || !notified.graphic.isModified() ||
            observer.modified != 1 || observer.undo != 1)
            return error(28, "observer exception must not invalidate committed undo");
        notified.graphic.setModificationListener(&observer, false);
        if (!notified.graphic.undo() || notified.originals[0]->isDeleted() || notified.originals[1]->isDeleted())
            return error(29, "undo after observer exception");

        Fixture relative;
        const auto marked = relative.viewport.getMarkedRelativeZero();
        relative.viewport.setRelativeZero({12, 34});
        auto relativeReady = Batch::tryPrepare(relative.pending, relative.epoch, relative.graphic, relative.viewport);
        if (!relativeReady || relative.viewport.getMarkedRelativeZero() != marked)
            return error(33, "relative zero must not change during preparation");
        arm(1);
        const auto relativeResult = relativeReady->execute();
        armed = false;
        if (hit || allocations || relativeResult != Batch::Outcome::Committed ||
            relative.viewport.getMarkedRelativeZero() != RS_Vector(12, 34) ||
            !relative.graphic.undo() || relative.viewport.getRelativeZero() != marked ||
            !relative.graphic.redo() || relative.viewport.getRelativeZero() != RS_Vector(12, 34))
            return error(34, "relative zero shares the prepared undo cycle");

        Fixture locked;
        auto* lockedLayer = new RS_Layer("locked replacement");
        locked.graphic.addLayer(lockedLayer); lockedLayer->lock(true);
        locked.pending.at(locked.originals[0]).candidate->setLayer(lockedLayer);
        auto lockedReady = Batch::tryPrepare(locked.pending, locked.epoch, locked.graphic, locked.viewport);
        if (!lockedReady || lockedReady->execute() != Batch::Outcome::Committed ||
            locked.candidates[0]->getFlag(RS2::FlagSelected) || locked.graphic.hasSelection() ||
            locked.candidates[0]->getLayer(false) != lockedLayer)
            return error(35, "locked replacement selection follows document policy");

        std::printf("prepare_std_new_failures=%zu redo_prepare_failures=%zu publication_std_new=0 former_failure_ordinals=8 stale=1 throwing_observer=1 relative_zero=1 locked_target=1\n",
            prepareAllocations, redoAllocations);
        // C++ new/new[] only. Qt/private malloc and aligned-new injection remain separate coverage limits.
        return 0;
    } catch (...) { return error(30, "unexpected exception"); }
}
