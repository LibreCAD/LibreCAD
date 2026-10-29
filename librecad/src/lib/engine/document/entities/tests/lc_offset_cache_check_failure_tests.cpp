/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/
// Dedicated executable only. Keep global allocation interposition out of Catch
// and application processes.

#include "lc_ellipseoffsetgeometry.h"
#include "rs_spline.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
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
void* operator new(const std::size_t size, const std::align_val_t alignment) {
    if (armed && ++allocationCount == failAt) {
        injected = true;
        throw std::bad_alloc{};
    }
#if defined(_MSC_VER)
    if (void* memory = _aligned_malloc(size ? size : 1, static_cast<std::size_t>(alignment))) { return memory; }
#else
    void* memory = nullptr;
    if (posix_memalign(&memory, static_cast<std::size_t>(alignment), size ? size : 1) == 0) { return memory; }
#endif
    throw std::bad_alloc{};
}
void* operator new[](const std::size_t size, const std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete(void* memory, std::align_val_t) noexcept {
#if defined(_MSC_VER)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}
void operator delete[](void* memory, std::align_val_t alignment) noexcept {
    ::operator delete(memory, alignment);
}
void operator delete(void* memory, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(memory, alignment);
}
void operator delete[](void* memory, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(memory, alignment);
}

namespace {
struct Fixture {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
};

Fixture cubicFixture(const bool reversed) {
    const RS_EllipseData source{RS_Vector{0, 0}, RS_Vector{1, 0}, 1.0, .25, .2501, false};
    Fixture fixture;
    fixture.definition.basis = source;
    fixture.definition.distance = 1.0;
    fixture.definition.relTolerance = .01;
    fixture.definition.cacheReversed = reversed;

    auto& cache = fixture.cache;
    cache.degree = 3;
    cache.knotslist = {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2};
    const auto point = [](const double angle) { return RS_Vector{2 * std::cos(angle), 2 * std::sin(angle)}; };
    const auto tangent = [](const double angle) { return RS_Vector{-2 * std::sin(angle), 2 * std::cos(angle)}; };
    const std::array<double, 3> angles{{source.angle1, .5 * (source.angle1 + source.angle2), source.angle2}};
    cache.controlPoints.push_back(point(angles[0]));
    for (std::size_t span = 0; span < 2; ++span) {
        const auto first = cache.controlPoints.back();
        const auto last = point(angles[span + 1]);
        const double step = (angles[span + 1] - angles[span]) / 3;
        const std::array<RS_Vector, 4> controls{{first, first + tangent(angles[span]) * step,
            last - tangent(angles[span + 1]) * step, last}};
        cache.controlPoints.insert(cache.controlPoints.end(), controls.begin() + 1, controls.end());
    }
    cache.weights.assign(cache.controlPoints.size(), 1.0);
    if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return fixture;
}

int sweep(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
          const char* label, std::size_t& totalInjectionPoints) {
    namespace geo = LC_EllipseOffsetGeometry;
    const auto& source = definition.basis;
    const auto frame = geo::ReferenceFrame::create(source, 1.0);
    if (!frame) { return 2; }

    geo::WorkBudget baselineBudget;
    failAt = 0;
    allocationCount = 0;
    injected = false;
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

    for (std::size_t failure = 1; failure <= completeAllocationCount; ++failure) {
        geo::WorkBudget failedBudget;
        failAt = failure;
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

        const auto& work = failedBudget.consumed();
        if (!threw || !injected || failedBudget.status() != geo::BudgetStatus::Ok ||
            work.cacheSpans > baseline.work.cacheSpans || work.vertices > baseline.work.vertices ||
            work.distancePairs > baseline.work.distancePairs || work.deepest > baseline.work.deepest ||
            work.orderingComparisons > baseline.work.orderingComparisons) {
            std::printf("allocation_failure=%zu/%zu threw=%d injected=%d status=%u vertices=%zu expected_at_most=%zu\n",
                        failure, completeAllocationCount, threw, injected, unsigned(failedBudget.status()),
                        work.vertices, baseline.work.vertices);
            return 7;
        }

        const auto recovered = geo::checkCache(cache, definition, .01, failedBudget);
        if (!recovered.analysis || recovered.reason != LC_OffsetCheckResult::Reason::None ||
            !sameWork(recovered.work, failedBudget.consumed())) {
            std::printf("allocation_recovery=%zu/%zu reason=%u status=%u vertices=%zu\n",
                        failure, completeAllocationCount, unsigned(recovered.reason),
                        unsigned(failedBudget.status()), failedBudget.consumed().vertices);
            return 8;
        }
    }

    totalInjectionPoints += completeAllocationCount;
    std::printf("allocation_failure_sweep_recovered=1 fixture=%s injection_points=%zu vertices=%zu\n",
                label, completeAllocationCount, baseline.work.vertices);
    return 0;
}
}

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
    RS_SplineData lineCache{1, false};
    lineCache.controlPoints = {world(first.point), world(last.point)};
    lineCache.weights = {1.0, 1.0}; lineCache.knotslist = {0.0, 0.0, 1.0, 1.0};
    LC_EllipseOffsetDefinition lineDefinition;
    lineDefinition.basis = source; lineDefinition.distance = 1.0; lineDefinition.relTolerance = .01;

    std::size_t injectionPoints = 0;
    if (const int result = sweep(lineCache, lineDefinition, "single-line", injectionPoints)) { return result; }
    for (const bool reversed : {false, true}) {
        auto cubic = cubicFixture(reversed);
        if (const int result = sweep(cubic.cache, cubic.definition,
            reversed ? "two-span-cubic-reversed" : "two-span-cubic", injectionPoints)) { return result; }
    }
    std::printf("allocation_failure_sweep_all_fixtures_recovered=1 injection_points=%zu\n", injectionPoints);
    return 0;
}
