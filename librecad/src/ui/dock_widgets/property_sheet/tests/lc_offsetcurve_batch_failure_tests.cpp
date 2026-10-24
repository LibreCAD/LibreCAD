/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
// Dedicated executable ONLY. Never link this main/global new into Catch/app.
#include "lc_offsetcurvepropertybatch.h"
#include "lc_graphicviewport.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_line.h"
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
    }
    bool untouched() const {
        if (epoch != 17 || graphic.nesting() != 0 || graphic.getAutoUpdateBorders() != originalBorders ||
            const_cast<Graphic&>(graphic).countUndoCycles() != 0 || pending.size() != 2 ||
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
        auto observed = Batch::tryPrepare(f.pending, f.epoch);
        armed = false;
        const auto prepareAllocations = allocations;
        if (!observed || !f.untouched() || observed->size() != 2 || observed->selectedCount() != 1 ||
            prepareAllocations == 0 || prepareAllocations > 64) return error(10, "preparation census");
        observed.reset();
        for (std::size_t ordinal = 1; ordinal <= prepareAllocations; ++ordinal) {
            arm(ordinal);
            auto failed = Batch::tryPrepare(f.pending, f.epoch);
            armed = false;
            if (!hit || failed || !f.untouched()) return error(11, "preparation must retain both owners");
        }

        // Observe precisely the real std::function constructions AFTER both
        // raw pairs were filled. SBO may eliminate every C++ allocation here.
        Batch pairLists(f.pending, f.epoch);
        arm(); pairLists.prepareCallbacks(); armed = false;
        const auto postPairAllocations = allocations;
        if (postPairAllocations > 64 || !f.untouched()) return error(12, "post-pair census");
        for (std::size_t ordinal = 1; ordinal <= postPairAllocations; ++ordinal) {
            Batch completePairs(f.pending, f.epoch);
            bool threw = false;
            arm(ordinal);
            try { completePairs.prepareCallbacks(); } catch (const std::bad_alloc&) { threw = true; }
            armed = false;
            if (!hit || !threw || !f.untouched()) return error(13, "post-pair callback failure");
        }

        // Actual undo entry is the first C++ allocation after complete list /
        // callback preparation: RS_Undo::startUndoCycle's make_shared. Fail it
        // after BOTH prepared pairs, and verify no empty cycle/epoch/owner loss.
        auto prepared = Batch::tryPrepare(f.pending, f.epoch);
        if (!prepared) return error(14, "retry preparation");
        arm(1);
        const auto before = prepared->execute(f.graphic, &f.viewport);
        armed = false;
        if (!hit || before != Batch::Outcome::AllocationBeforeHandoff ||
            prepared->handedOff() || !f.untouched()) return error(15, "undo-entry allocation failure");

        // Retry through the actual document API, without injection into the
        // explicitly unqualified generic post-handoff allocation phase.
        if (prepared->execute(f.graphic, &f.viewport) != Batch::Outcome::Committed ||
            !prepared->handedOff() || f.epoch != 18 || f.graphic.nesting() != 0 ||
            f.graphic.countUndoCycles() != 1) return error(16, "actual commit retry");
        for (unsigned i = 0; i < 2; ++i) {
            if (f.pending.at(f.originals[i]).candidate || !f.originals[i]->isDeleted() ||
                f.graphic.findEntityIndex(f.candidates[i]) < 0 ||
                f.candidates[i]->isSelected() != (i == 0)) return error(17, "published ownership/selection");
        }
        if (!f.graphic.undo() || f.originals[0]->isDeleted() || f.originals[1]->isDeleted() ||
            f.graphic.undo()) return error(18, "one-step undo");

        // Exercise the exact production handoff callback with operator new
        // armed. It must allocate zero, regardless of std::function SBO policy.
        Fixture swap;
        auto ready = Batch::tryPrepare(swap.pending, swap.epoch);
        if (!ready) return error(19, "handoff preparation");
        LC_DocumentModificationBatch raw;
        arm(1);
        const bool success = ready->modification()(raw);
        armed = false;
        if (!success || hit || allocations != 0 || !ready->handedOff() ||
            raw.entitiesToAdd.size() != 2 || raw.entitiesToDelete.size() != 2 || swap.epoch != 18)
            return error(20, "handoff must be allocation-free");
        // raw is non-owning; this direct callback test, unlike execute(), did
        // not add the transferred entities to a document. Reclaim them here.
        for (auto* entity : raw.entitiesToAdd) delete entity;
        std::printf("prepare_std_new_failures=%zu after_prepared_pairs_std_new=%zu undo_entry_failure=1 handoff_std_new=0\n",
                    prepareAllocations, postPairAllocations);
        // C++ new/new[] only. No Qt/private malloc or aligned-new injection,
        // and no claim of generic postpublication allocation atomicity.
        return 0;
    } catch (...) { return error(30, "unexpected exception"); }
}
