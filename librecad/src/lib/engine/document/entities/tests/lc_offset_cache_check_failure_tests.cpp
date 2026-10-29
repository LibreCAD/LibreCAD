/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/
// Dedicated executable only. Keep global allocation interposition out of Catch
// and application processes.

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
thread_local bool armed = false;
thread_local bool injected = false;
thread_local std::size_t failAt = 0;
thread_local std::size_t allocationCount = 0;

bool sameWork(const LC_EllipseOffsetGeometry::CheckWork& a,
              const LC_EllipseOffsetGeometry::CheckWork& b) {
    return a.cacheSpans == b.cacheSpans && a.vertices == b.vertices &&
        a.distancePairs == b.distancePairs && a.deepest == b.deepest &&
        a.orderingComparisons == b.orderingComparisons;
}
}

void* operator new(const std::size_t size) {
    if (armed && ++allocationCount == failAt) {
        injected = true;
        throw std::bad_alloc{};
    }
    if (void* memory = std::malloc(size ? size : 1)) { return memory; }
    throw std::bad_alloc{};
}
void* operator new[](const std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    namespace geo = LC_EllipseOffsetGeometry;
    const RS_EllipseData source{RS_Vector{0, 0}, RS_Vector{1, 0}, 1.0, .25, .2501, false};
    const auto frame = geo::ReferenceFrame::create(source, 1.0);
    if (!frame) { return 2; }

    LC_CurveJet first, last;
    if (!frame->offsetJet(0, first) || !frame->offsetJet(frame->parameterEnd(), last)) { return 3; }
    const auto world = [&](const RS_Vector& point) {
        return RS_Vector{source.center.x + frame->scale() * point.x,
                         source.center.y + frame->scale() * point.y};
    };
    RS_SplineData cache{1, false};
    cache.controlPoints = {world(first.point), world(last.point)};
    cache.weights = {1.0, 1.0};
    cache.knotslist = {0.0, 0.0, 1.0, 1.0};

    LC_EllipseOffsetDefinition definition;
    definition.basis = source;
    definition.distance = 1.0;
    definition.relTolerance = .01;

    geo::WorkBudget baselineBudget;
    allocationCount = 0;
    armed = true;
    LC_OffsetCheckResult baseline;
    try {
        baseline = geo::checkCache(cache, definition, .01, baselineBudget);
    } catch (...) {
        armed = false;
        return 4;
    }
    armed = false;
    const std::size_t completeAllocationCount = allocationCount;
    if (!baseline.analysis || baseline.reason != LC_OffsetCheckResult::Reason::None ||
        !sameWork(baseline.work, baselineBudget.consumed()) || completeAllocationCount == 0) {
        return 5;
    }

    geo::WorkBudget failedBudget;
    failAt = completeAllocationCount;
    allocationCount = 0;
    injected = false;
    armed = true;
    bool threw = false;
    try {
        auto failed = geo::checkCache(cache, definition, .01, failedBudget);
        (void)failed;
    } catch (const std::bad_alloc&) {
        threw = true;
    } catch (...) {
        armed = false;
        return 6;
    }
    armed = false;

    if (!threw || !injected || failedBudget.status() != geo::BudgetStatus::Ok ||
        !sameWork(failedBudget.consumed(), baseline.work)) {
        std::printf("allocation_failure=%d injected=%d status=%u vertices=%zu expected=%zu\n",
                    threw, injected, unsigned(failedBudget.status()), failedBudget.consumed().vertices,
                    baseline.work.vertices);
        return 7;
    }

    std::printf("final_allocation_failure_preserved_full_budget=1 allocations=%zu vertices=%zu\n",
                completeAllocationCount, failedBudget.consumed().vertices);
    return 0;
}
