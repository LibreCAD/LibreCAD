/*******************************************************************************
 * This file is part of LibreCAD, distributed under GPL version 2 or later.
 * Copyright (C) 2026 LibreCAD.org
 ******************************************************************************/
// Dedicated executable ONLY, following rs_entitycontainer_replace_failure_tests.
// Never register this global-new replacement or main in a Catch/application TU.
// The separate EXCLUDE_FROM_ALL target is not added to any runner or fast lane.
#include "lc_offsetcachetrim.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "drw_base.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_undo.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_spline.h"
#include "rs_circle.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <sstream>
#include <vector>

namespace {
namespace Trim = LC_OffsetCacheTrim;
constexpr std::size_t traceCap = 65536;
constexpr std::size_t stagingFaultCap = 4096;
struct Event { std::size_t bytes = 0; std::uintptr_t address = 0; };
enum class Injection { Observe, Fail, Cancel };
struct Trace {
    Event* events = nullptr;
    std::size_t count = 0, trigger = 0;
    Injection injection = Injection::Observe;
    std::atomic_bool* cancellation = nullptr;
    bool overflow = false, hit = false;
};
Event baselineEvents[traceCap], trialEvents[traceCap];
thread_local Trace* activeTrace = nullptr;

// The observer uses fixed storage and never allocates or calls geometry.
std::size_t observeAllocation(std::size_t bytes) {
    auto* trace = activeTrace;
    if (!trace) { return 0; }
    const auto ordinal = ++trace->count;
    if (ordinal > traceCap) { trace->overflow = true; }
    else { trace->events[ordinal - 1] = {bytes, 0}; }
    if (ordinal == trace->trigger) {
        trace->hit = true;
        if (trace->injection == Injection::Cancel) { trace->cancellation->store(true, std::memory_order_relaxed); }
        if (trace->injection == Injection::Fail) { throw std::bad_alloc(); }
    }
    return ordinal;
}
}

void* operator new(std::size_t bytes) {
    const auto ordinal = observeAllocation(bytes);
    if (auto* memory = std::malloc(bytes ? bytes : 1)) {
        if (activeTrace && ordinal != 0 && ordinal <= traceCap) {
            activeTrace->events[ordinal - 1].address = reinterpret_cast<std::uintptr_t>(memory);
        }
        return memory;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
bool samePoint(const RS_Vector& a, const RS_Vector& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.valid == b.valid;
}
bool sameData(const RS_SplineData& a, const RS_SplineData& b) {
    if (a.degree != b.degree || a.type != b.type || a.savedOpenType != b.savedOpenType ||
        a.m_closedFlag != b.m_closedFlag || a.m_periodicFlag != b.m_periodicFlag ||
        a.knotslist != b.knotslist || a.weights != b.weights || a.savedOpenKnots != b.savedOpenKnots ||
        a.controlPoints.size() != b.controlPoints.size() || a.fitPoints.size() != b.fitPoints.size()) { return false; }
    for (std::size_t i = 0; i < a.controlPoints.size(); ++i) {
        if (!samePoint(a.controlPoints[i], b.controlPoints[i])) { return false; }
    }
    for (std::size_t i = 0; i < a.fitPoints.size(); ++i) {
        if (!samePoint(a.fitPoints[i], b.fitPoints[i])) { return false; }
    }
    return true;
}
RS_SplineData lineCache(bool reverse) {
    RS_SplineData data{1, false};
    data.controlPoints = {{0, 0}, {10, 0}};
    if (reverse) { std::reverse(data.controlPoints.begin(), data.controlPoints.end()); }
    data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    return data;
}
RS_SplineData cutterCache(bool reverse) {
    RS_SplineData data{1, false};
    data.controlPoints = {{3, -2}, {3, 2}, {7, 2}, {7, -2}};
    if (reverse) { std::reverse(data.controlPoints.begin(), data.controlPoints.end()); }
    data.weights = {1, 1, 1, 1}; data.knotslist = {0, 0, 1, 2, 3, 3};
    return data;
}
bool positive(const Trim::Result& result, bool reverse) {
    if (result.refusal != Trim::Refusal::None || result.fragments.size() != 2 ||
        result.fragmentCount != 2 || result.rootCount != 2 || result.roots.size() != 2 ||
        !result.removed || result.removed->crossesStorageSeam ||
        result.work.outputSpans != 2 || result.work.outputControls != 4 || result.work.outputKnots != 8) { return false; }
    const double coordinates[4] = {reverse ? 10. : 0., reverse ? 7. : 3., reverse ? 3. : 7., reverse ? 0. : 10.};
    for (std::size_t i = 0; i < 2; ++i) {
        const auto& fragment = result.fragments[i];
        if (fragment.data.degree != 1 || fragment.data.controlPoints.size() != 2 ||
            fragment.sourceSections.size() != 1 || fragment.data.weights.size() != 2 ||
            fragment.data.weights[0] != 1 || fragment.data.weights[1] != 1 ||
            fragment.data.type != RS_SplineData::SplineType::ClampedOpen ||
            fragment.data.m_closedFlag || fragment.data.m_periodicFlag) { return false; }
        for (std::size_t j = 0; j < 2; ++j) {
            const auto& p = fragment.data.controlPoints[j];
            if (std::abs(p.x - coordinates[2 * i + j]) > 1e-10 || p.y != 0 || p.z != 0) { return false; }
        }
    }
    return result.fragments[0].start.labels == unsigned(Trim::EndLabel::SourceStart) &&
        result.fragments[0].end.labels == unsigned(Trim::EndLabel::CutterRoot) &&
        result.fragments[1].start.labels == unsigned(Trim::EndLabel::CutterRoot) &&
        result.fragments[1].end.labels == unsigned(Trim::EndLabel::SourceEnd);
}
template <typename T>
std::size_t bufferOrdinal(const std::vector<T>& buffer, const Trace& trace) {
    if (buffer.empty()) { return 0; }
    const auto address = reinterpret_cast<std::uintptr_t>(buffer.data());
    for (std::size_t i = trace.count; i != 0; --i) {
        if (trace.events[i - 1].address == address &&
            trace.events[i - 1].bytes == buffer.capacity() * sizeof(T)) { return i; }
    }
    return 0;
}
bool samePrefix(const Trace& baseline, const Trace& trial, std::size_t through) {
    if (trial.overflow || !trial.hit || trial.count < through || through > baseline.count) { return false; }
    for (std::size_t i = 0; i < through; ++i) {
        if (baseline.events[i].bytes != trial.events[i].bytes) { return false; }
    }
    return true;
}
bool refusedWhole(const Trim::Result& result, Trim::Refusal reason, const Trim::Work& observed) {
    return result.refusal == reason && result.fragments.empty() && result.fragmentCount == 0 &&
        result.roots.empty() && !result.removed && result.rootCount == 2 &&
        result.work.outputSpans == 2 && result.work.outputControls == 4 && result.work.outputKnots == 8 &&
        observed.outputSpans == result.work.outputSpans && observed.outputControls == result.work.outputControls &&
        observed.outputKnots == result.work.outputKnots && observed.kernelItems == result.work.kernelItems &&
        observed.intersections.totalBoxPairs == result.work.intersections.totalBoxPairs;
}
int error(int code, const char* what, std::size_t ordinal = 0) {
    activeTrace = nullptr;
    std::fprintf(stderr, "cache-trim-standalone: %s ordinal=%zu code=%d\n", what, ordinal, code);
    return code;
}

int exercise(bool reverse, bool cutterReverse, std::size_t& faults, std::size_t& cancellations) {
    const auto data = lineCache(reverse); const auto before = data;
    RS_Spline cutter{nullptr, cutterCache(cutterReverse)}; const auto cutterBefore = cutter.getData();
    std::vector<const RS_Entity*> children;
    for (const auto* child : cutter) { children.push_back(child); }
    const auto unchanged = [&]() {
        if (!sameData(data, before) || !sameData(cutter.getData(), cutterBefore)) { return false; }
        std::size_t i = 0;
        for (const auto* child : cutter) {
            if (i >= children.size() || child != children[i++]) { return false; }
        }
        return i == children.size();
    };
    auto flag = std::make_shared<std::atomic_bool>(false);
    // Warm library initialization outside observation; cancellation ownership
    // and inputs are identical for the baseline and every injected call.
    if (!positive(Trim::prepare(data, cutter, {{5, 0}}, {}, {flag}), reverse)) { return error(10, "warmup"); }
    Trace baseline; baseline.events = baselineEvents;
    activeTrace = &baseline;
    auto complete = Trim::prepare(data, cutter, {{5, 0}}, {}, {flag});
    activeTrace = nullptr;
    if (baseline.overflow || !positive(complete, reverse) || !unchanged()) { return error(11, "baseline"); }
    // Returned buffers identify actual successful allocation sites, including
    // the SECOND fragment. No guessed ordinal/size or production hook is used.
    const std::array<std::size_t, 10> sites{{
        bufferOrdinal(complete.fragments, baseline),
        bufferOrdinal(complete.fragments[0].data.controlPoints, baseline),
        bufferOrdinal(complete.fragments[0].data.weights, baseline),
        bufferOrdinal(complete.fragments[0].data.knotslist, baseline),
        bufferOrdinal(complete.fragments[0].sourceSections, baseline),
        bufferOrdinal(complete.fragments[1].data.controlPoints, baseline),
        bufferOrdinal(complete.fragments[1].data.weights, baseline),
        bufferOrdinal(complete.fragments[1].data.knotslist, baseline),
        bufferOrdinal(complete.fragments[1].sourceSections, baseline),
        bufferOrdinal(complete.roots, baseline)}};
    for (std::size_t i = 0; i < sites.size(); ++i) {
        if (!sites[i] || (i && sites[i - 1] >= sites[i])) { return error(12, "buffer allocation mapping", i); }
    }
    const auto firstStaging = sites[0];
    if (baseline.count - firstStaging + 1 > stagingFaultCap) { return error(13, "staging fault census cap"); }
    // Sweep EVERY observed ordinary-new allocation from the staging reserve
    // through final provenance, including arithmetic temporaries between sites.
    for (std::size_t ordinal = firstStaging; ordinal <= baseline.count; ++ordinal) {
        flag->store(false, std::memory_order_relaxed);
        Trace trial; trial.events = trialEvents; trial.trigger = ordinal; trial.injection = Injection::Fail;
        Trim::Work observed;
        activeTrace = &trial;
        auto result = Trim::prepare(data, cutter, {{5, 0}}, {}, {flag}, &observed);
        activeTrace = nullptr;
        if (!samePrefix(baseline, trial, ordinal) ||
            !refusedWhole(result, Trim::Refusal::AllocationFailure, observed) || !unchanged()) {
            return error(14, "staging allocation did not refuse wholly", ordinal);
        }
        ++faults;
    }
    // The second-fragment sites are reached only AFTER the first fragment was
    // fully staged. The final site is after BOTH fragments, before publication.
    for (std::size_t site = 5; site < sites.size(); ++site) {
        flag->store(false, std::memory_order_relaxed);
        Trace trial; trial.events = trialEvents; trial.trigger = sites[site];
        trial.injection = Injection::Cancel; trial.cancellation = flag.get();
        Trim::Work observed;
        activeTrace = &trial;
        auto result = Trim::prepare(data, cutter, {{5, 0}}, {}, {flag}, &observed);
        activeTrace = nullptr;
        if (!flag->load(std::memory_order_relaxed) || !samePrefix(baseline, trial, sites[site]) ||
            !refusedWhole(result, Trim::Refusal::Cancelled, observed) || !unchanged()) {
            return error(15, "mid-staging cancellation did not refuse wholly", sites[site]);
        }
        ++cancellations;
    }
    flag->store(false, std::memory_order_relaxed);
    if (!positive(Trim::prepare(data, cutter, {{5, 0}}, {}, {flag}), reverse) || !unchanged()) {
        return error(16, "retry after fault/cancellation");
    }
    std::printf("stored_reverse=%d cutter_reverse=%d total_std_new=%zu staging_std_new=%zu second_fragment_first=%zu provenance=%zu\n",
                reverse, cutterReverse, baseline.count, baseline.count - firstStaging + 1, sites[5], sites[9]);
    return 0;
}

int exerciseWrapperLastCancellation() {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1., 0., .1, false};
    definition.distance = .25; definition.relTolerance = .01;
    const RS_Line carrier{nullptr, RS_Vector{1.25, 0},
        RS_Vector{1.25 * std::cos(.1), 1.25 * std::sin(.1)}};
    LC_OffsetCheckResult checked;
    auto admitted = LC_OffsetCurve::adopt(carrier, definition, .01, &checked);
    if (!checked || !admitted || !admitted->hasEditableDefinition()) return error(60, "wrapper actual C fixture");
    admitted->setParent(&graphic); admitted->setLayer(graphic.getActiveLayer());
    admitted->setSourceHandle(0x71); admitted->setXDictHandle(0x72); admitted->setReactorHandles({0x73});
    admitted->setUserDefVar("R03_WRAPPER_FOREIGN", "unchanged");
    admitted->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("R03_WRAPPER_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("retained"))});
    if (!admitted->setOwnedExtensions(0x10, {{1000, QStringLiteral("retained wrapper tail")}}))
        return error(61, "wrapper owned fixture metadata");
    auto* source = admitted.release(); graphic.addEntity(source);
    auto* cutter = new RS_Line(&graphic, RS_Vector{0, .0625}, RS_Vector{2, .0625});
    cutter->setLayer(graphic.getActiveLayer()); graphic.addEntity(cutter);
    auto* oldPrefix = new RS_Line(&graphic, RS_Vector{10, 1}, RS_Vector{11, 1});
    oldPrefix->setLayer(graphic.getActiveLayer()); graphic.addEntity(oldPrefix);
    graphic.select(QList<RS_Entity*>{source, cutter});
    graphic.setModified(false);
    const auto before = source->getData();
    const auto* const storage = source->getData().controlPoints.data();
    const auto sourceFlags = source->getFlags(), cutterFlags = cutter->getFlags();
    const auto cutterBefore = cutter->getData();
    const auto oldFlags = oldPrefix->getFlags();
    const auto epoch = graphic.undoStateEpoch();
    auto sourceState = LC_OffsetCurveEditSnapshot::capture(*source);
    if (!sourceState) return error(62, "wrapper source baseline");
    QList<RS_Entity*> selection;
    graphic.collectSelected(selection);
    const auto history = [&] {
        std::ostringstream out; out << static_cast<const RS_Undo&>(graphic); return out.str();
    };
    const auto historyBefore = history();
    const auto unchanged = [&] {
        QList<RS_Entity*> current;
        graphic.collectSelected(current);
        bool undo = true, redo = true; graphic.collectUndoState(undo, redo);
        return graphic.count() == 3 && graphic.findEntityIndex(source) >= 0 &&
            graphic.findEntityIndex(cutter) >= 0 && graphic.findEntityIndex(oldPrefix) >= 0 &&
            sameData(source->getData(), before) && source->getData().controlPoints.data() == storage &&
            sourceState->matchesStoredState(*source) && source->getFlags() == sourceFlags &&
            samePoint(cutter->getData().startpoint, cutterBefore.startpoint) &&
            samePoint(cutter->getData().endpoint, cutterBefore.endpoint) && cutter->getFlags() == cutterFlags &&
            oldPrefix->getFlags() == oldFlags && current == selection && !graphic.isModified() &&
            graphic.undoStateEpoch() == epoch && history() == historyBefore && !undo && !redo;
    };
    auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
    const QList<RS_Entity*> additions{prefix.get()}, deletions{oldPrefix};
    struct OwnedBatch {
        LC_DocumentModificationBatch value;
        explicit OwnedBatch(const QList<RS_Entity*>& add, const QList<RS_Entity*>& remove) {
            value.entitiesToAdd = add; value.entitiesToDelete = remove;
        }
        ~OwnedBatch() {
            // The wrapper only PREPARES these outputs; no document takes ownership.
            // This also reclaims outputs from an old implementation's false success.
            while (value.entitiesToAdd.size() > 1) delete value.entitiesToAdd.takeLast();
        }
    };
    auto flag = std::make_shared<std::atomic_bool>(false);
    LC_StoredCacheTrimOptions options; options.cancellation.requested = flag;
    const auto pick = source->getData().controlPoints.front() * .75 +
        source->getData().controlPoints.back() * .25;
    const auto run = [&](OwnedBatch& batch) {
        return RS_Modification::trimStoredCache(graphic, *source, pick, *cutter, {1, .0625},
            false, batch.value, options);
    };
    const auto positive = [&](const LC_TrimResult& result, const OwnedBatch& batch) {
        if (!result.result || result.cacheRefusal || !result.trimmed1 || result.trimmed2 ||
            result.nativeDemotions != 1 || result.cacheWork.rootRecords == 0 ||
            batch.value.entitiesToAdd.size() != 2 || batch.value.entitiesToDelete.size() != 2 ||
            batch.value.entitiesToAdd.front() != prefix.get() || batch.value.entitiesToAdd.back() != result.trimmed1 ||
            batch.value.entitiesToDelete.front() != oldPrefix || batch.value.entitiesToDelete.back() != source)
            return false;
        const auto* output = dynamic_cast<const RS_Spline*>(result.trimmed1);
        return output && output->rtti() == RS2::EntitySpline && output->getParent() == &graphic &&
            output->getData().degree == 1 && output->getData().controlPoints.size() == 2 &&
            output->getUserDefVar("R03_WRAPPER_FOREIGN") == "unchanged";
    };
    // Warm all fixture/library initialization OUTSIDE observation. The exact
    // same real inputs, token, prefixes and wrapper path are used in every run.
    {
        OwnedBatch warm(additions, deletions);
        const auto result = run(warm);
        const bool prepared = positive(result, warm), preserved = unchanged();
        if (!prepared || !preserved) {
            const auto* output = dynamic_cast<const RS_Spline*>(result.trimmed1);
            std::fprintf(stderr, "wrapper_warm result=%d refusal=%d adds=%lld deletes=%lld native_demotions=%zu roots=%zu degree=%d controls=%zu parent=%d variables=%d preserved=%d\n",
                result.result, result.cacheRefusal ? int(*result.cacheRefusal) : -1,
                static_cast<long long>(warm.value.entitiesToAdd.size()), static_cast<long long>(warm.value.entitiesToDelete.size()),
                result.nativeDemotions, result.cacheWork.rootRecords, output ? output->getData().degree : -1,
                output ? output->getData().controlPoints.size() : 0, output && output->getParent() == &graphic,
                output && output->getUserDefVar("R03_WRAPPER_FOREIGN") == "unchanged", preserved);
            return error(63, "wrapper positive warmup");
        }
    }
    OwnedBatch complete(additions, deletions);
    Trace baseline; baseline.events = baselineEvents;
    activeTrace = &baseline;
    const auto completeResult = run(complete);
    activeTrace = nullptr;
    if (baseline.overflow || baseline.count == 0 || !positive(completeResult, complete) || !unchanged())
        return error(64, "wrapper successful allocation census");
    // Map the LAST allocation of the actual successful wrapper, not a guessed
    // ordinal/size or a kernel-only surrogate. Final native stored validation
    // allocates its membership map and nonempty user-key witness; successful
    // handoff afterward is only swaps/releases/scalar result updates.
    const auto last = baseline.count;
    OwnedBatch cancelled(additions, deletions);
    Trace trial; trial.events = trialEvents; trial.trigger = last;
    trial.injection = Injection::Cancel; trial.cancellation = flag.get();
    activeTrace = &trial;
    const auto result = run(cancelled);
    activeTrace = nullptr;
    if (!flag->load(std::memory_order_relaxed) || !samePrefix(baseline, trial, last) ||
        result.result || !result.cacheRefusal || *result.cacheRefusal != Trim::Refusal::Cancelled ||
        result.trimmed1 || result.trimmed2 || result.nativeDemotions != 0 ||
        cancelled.value.entitiesToAdd != additions || cancelled.value.entitiesToDelete != deletions || !unchanged())
        return error(65, "wrapper last-allocation cancellation escaped publication guard", last);
    flag->store(false, std::memory_order_relaxed);
    {
        OwnedBatch retry(additions, deletions);
        if (!positive(run(retry), retry) || !unchanged()) return error(66, "wrapper positive retry after cancellation");
    }
    // Keep all existing stdout summary lines/counters/regexes unchanged.
    std::fprintf(stderr, "wrapper_last_successful_allocation=%zu cancellation_hit=1 unchanged_inputs_prefixes_history=1 positive_controls=3\n", last);
    return 0;
}

int exerciseConic(bool reverse, std::size_t& faults, std::size_t& cancellations) {
    auto data = lineCache(reverse);
    data.controlPoints = reverse ? std::vector<RS_Vector>{{3, .5}, {-3, .5}} : std::vector<RS_Vector>{{-3, .5}, {3, .5}};
    const auto before = data; RS_Circle circle{nullptr, {{0, 0}, 2}};
    auto flag = std::make_shared<std::atomic_bool>(false);
    const auto run = [&](Trim::Work* observed = nullptr) {
        return Trim::prepareConic(data, circle, {{0, 2}}, Trim::Pick{{0, .5}}, {}, {flag}, observed);
    };
    const auto positive = [](const Trim::Result& result) {
        return result.refusal == Trim::Refusal::None && result.fragments.size() == 2 && result.fragmentCount == 2 &&
            result.conicFragments.size() == 1 && result.rootCount == 2 && result.roots.size() == 2 &&
            result.work.outputSpans == 3 && result.work.outputControls == 4 && result.work.outputKnots == 8;
    };
    if (!positive(run())) return error(50, "conic warmup");
    Trace baseline; baseline.events = baselineEvents; activeTrace = &baseline;
    const auto complete = run(); activeTrace = nullptr;
    if (baseline.overflow || !positive(complete)) return error(51, "conic baseline");
    // These two live returned buffers bracket conic materialization and final
    // provenance AFTER the polynomial complement has already been staged.
    const std::array<std::size_t, 2> sites{{bufferOrdinal(complete.conicFragments, baseline), bufferOrdinal(complete.roots, baseline)}};
    if (!sites[0] || sites[0] >= sites[1]) return error(52, "conic buffer mapping");
    for (const auto site : sites) for (const auto injection : {Injection::Fail, Injection::Cancel}) {
        flag->store(false, std::memory_order_relaxed);
        Trace trial; trial.events = trialEvents; trial.trigger = site; trial.injection = injection; trial.cancellation = flag.get();
        Trim::Work observed; activeTrace = &trial; const auto result = run(&observed); activeTrace = nullptr;
        const auto reason = injection == Injection::Fail ? Trim::Refusal::AllocationFailure : Trim::Refusal::Cancelled;
        if (!samePrefix(baseline, trial, site) || result.refusal != reason || !result.fragments.empty() ||
            !result.conicFragments.empty() || result.fragmentCount != 0 || !result.roots.empty() || result.removed ||
            result.work.outputSpans != 3 || observed.outputSpans != result.work.outputSpans ||
            observed.intersections.totalBoxPairs != result.work.intersections.totalBoxPairs ||
            !sameData(data, before) || !samePoint(circle.getCenter(), RS_Vector{0, 0}) || circle.getRadius() != 2)
            return error(53, "conic staging did not refuse wholly", site);
        if (injection == Injection::Fail) ++faults; else ++cancellations;
    }
    flag->store(false, std::memory_order_relaxed);
    if (!positive(run()) || !sameData(data, before)) return error(54, "conic retry");
    std::printf("conic_reverse=%d typed_buffer=%zu provenance=%zu\n", reverse, sites[0], sites[1]);
    return 0;
}
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    QTemporaryDir settings;
    if (!settings.isValid()) { return 40; }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settings.path());
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    RS_DEBUG->setLevel(RS_Debug::D_WARNING);
    std::size_t faults = 0, cancellations = 0;
    std::size_t conicFaults = 0, conicCancellations = 0;
    try {
        for (const bool reverse : {false, true}) for (const bool cutterReverse : {false, true}) {
            if (const int code = exercise(reverse, cutterReverse, faults, cancellations)) { return code; }
        }
        for (const bool reverse : {false, true}) if (const int code = exerciseConic(reverse, conicFaults, conicCancellations)) return code;
        if (const int code = exerciseWrapperLastCancellation()) return code;
    } catch (...) { return error(30, "unexpected exception"); }
    std::printf("directions=4 staging_allocation_failures=%zu cancellation_after_first_fragment_or_later=%zu\n", faults, cancellations);
    // This replaces ordinary C++ new/new[], not Qt/private malloc or aligned
    // allocation. The mapped std::vector buffers above MUST all be observed.
    std::printf("conic_staging_failures=%zu conic_staging_cancellations=%zu\n", conicFaults, conicCancellations);
    return faults > 0 && cancellations == 20 && conicFaults == 4 && conicCancellations == 4 ? 0 : 20;
}
