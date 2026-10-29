/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/
// Dedicated executable only. Keep global allocation interposition out of Catch
// and application processes.

#include "lc_curveoffset.h"
#include "lc_ellipseoffsetgeometry.h"
#include "rs_ellipse.h"
#include "rs_spline.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <optional>
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

Fixture wholeCircleFixture(const bool reversed) {
    const RS_EllipseData source{RS_Vector{0, 0}, RS_Vector{1, 0}, 1.0, 0.0, 0.0, false};
    Fixture fixture;
    fixture.definition.basis = source;
    fixture.definition.distance = 1.0;
    fixture.definition.relTolerance = .01;
    fixture.definition.cacheReversed = reversed;

    auto& cache = fixture.cache;
    cache.degree = 3;
    for (int knot = 0; knot <= 4; ++knot) {
        const int multiplicity = knot == 0 || knot == 4 ? 4 : 3;
        cache.knotslist.insert(cache.knotslist.end(), multiplicity, double(knot));
    }
    std::array<RS_Vector, 4> controls{{{2, 0}, {2, 9.0 / 8}, {9.0 / 8, 2}, {0, 2}}};
    cache.controlPoints.reserve(13);
    for (int quadrant = 0; quadrant < 4; ++quadrant) {
        if (quadrant != 0) {
            for (auto& point : controls) { point = {-point.y, point.x}; }
        }
        const auto begin = quadrant == 0 ? controls.begin() : controls.begin() + 1;
        cache.controlPoints.insert(cache.controlPoints.end(), begin, controls.end());
    }
    cache.weights.assign(cache.controlPoints.size(), 1.0);
    if (reversed) { std::reverse(cache.controlPoints.begin(), cache.controlPoints.end()); }
    return fixture;
}

std::optional<Fixture> openTrimmedComponentFixture() {
    const RS_EllipseData source{RS_Vector{0, 0}, RS_Vector{100, 0}, .05,
        M_PI / 12, 7 * M_PI / 4, false};
    const RS_Ellipse ellipse{nullptr, source};
    auto options = LC_CurveOffset::makeDirectOptions(ellipse, 4, 1e-4);
    options.mode = LC_CurveOffsetMode::Trimmed;
    auto made = LC_CurveOffset::createEntities(ellipse,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 4), options,
        LC_CurveOffset::makeDirectSourceBudget());
    if (made.status != LC_CurveOffsetStatus::Ok || made.entities.size() != 2) { return {}; }

    const RS_Spline* open = nullptr;
    for (const auto& entity : made.entities) {
        const auto* spline = dynamic_cast<const RS_Spline*>(entity.get());
        if (!spline) { return {}; }
        const auto& data = spline->getData();
        if (data.controlPoints.front() != data.controlPoints.back()) { open = spline; }
    }
    if (!open) { return {}; }

    Fixture fixture;
    fixture.cache = open->getData();
    fixture.definition.basis = source;
    fixture.definition.distance = 4;
    fixture.definition.towardCentre = true;
    fixture.definition.relTolerance = .01;
    return fixture;
}

int sweep(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
          const char* label, std::size_t& totalInjectionPoints, const double worldTolerance = .01,
          const bool expectSeam = false, const std::optional<bool> expectClosed = std::nullopt) {
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
        baseline = geo::checkCache(cache, definition, worldTolerance, baselineBudget);
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
    if (expectSeam) {
        const auto& joins = baseline.analysis->joins();
        if (!baseline.analysis->closed() || std::none_of(joins.begin(), joins.end(), [](const auto& join) {
                return join.kind == geo::CachePathJoinKind::Seam;
            })) { return 9; }
    }
    if (expectClosed && baseline.analysis->closed() != *expectClosed) { return 10; }

    for (std::size_t failure = 1; failure <= completeAllocationCount; ++failure) {
        geo::WorkBudget failedBudget;
        failAt = failure;
        allocationCount = 0;
        injected = false;
        armed = true;
        bool threw = false;
        try {
            auto failed = geo::checkCache(cache, definition, worldTolerance, failedBudget);
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

        const auto recovered = geo::checkCache(cache, definition, worldTolerance, failedBudget);
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

template<typename Operation, typename Complete>
int sweepStage(const char* label, Operation&& operation, Complete&& complete,
               std::size_t& totalInjectionPoints) {
    namespace geo = LC_EllipseOffsetGeometry;
    geo::WorkBudget baselineBudget;
    failAt = 0;
    allocationCount = 0;
    injected = false;
    armed = true;
    auto baseline = operation(baselineBudget);
    armed = false;
    const std::size_t completeAllocationCount = allocationCount;
    if (!complete(baseline) || !sameWork(baseline.work, baselineBudget.consumed()) ||
        completeAllocationCount == 0) {
        std::printf("allocation_stage_baseline_failed=1 fixture=%s status=%u allocations=%zu\n",
                    label, unsigned(baseline.status), completeAllocationCount);
        return 12;
    }

    for (std::size_t failure = 1; failure <= completeAllocationCount; ++failure) {
        geo::WorkBudget failedBudget;
        failAt = failure;
        allocationCount = 0;
        injected = false;
        armed = true;
        bool threw = false;
        try {
            auto failed = operation(failedBudget);
            (void)failed;
        } catch (const std::bad_alloc&) {
            threw = true;
        } catch (...) {
            armed = false;
            return 13;
        }
        armed = false;

        const auto& work = failedBudget.consumed();
        if (!threw || !injected || failedBudget.status() != geo::BudgetStatus::Ok ||
            work.cacheSpans > baseline.work.cacheSpans || work.vertices > baseline.work.vertices ||
            work.distancePairs > baseline.work.distancePairs || work.deepest > baseline.work.deepest ||
            work.orderingComparisons > baseline.work.orderingComparisons) {
            std::printf("allocation_stage_failure=%zu/%zu fixture=%s threw=%d injected=%d status=%u\n",
                        failure, completeAllocationCount, label, threw, injected, unsigned(failedBudget.status()));
            return 14;
        }

        const auto recovered = operation(failedBudget);
        if (!complete(recovered) || !sameWork(recovered.work, failedBudget.consumed())) {
            std::printf("allocation_stage_recovery=%zu/%zu fixture=%s status=%u\n",
                        failure, completeAllocationCount, label, unsigned(recovered.status));
            return 15;
        }
    }

    totalInjectionPoints += completeAllocationCount;
    std::printf("allocation_stage_sweep_recovered=1 fixture=%s injection_points=%zu vertices=%zu\n",
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
    {
        const auto cubic = cubicFixture(false);
        const auto bounds = [&](geo::WorkBudget& budget) {
            return geo::analyseCacheBounds(cubic.cache, cubic.definition.basis, cubic.definition.distance, .01, budget);
        };
        const auto boundsComplete = [](const geo::CacheBoundsResult& result) {
            return result.status == geo::CacheBoundsStatus::Ok && bool(result.analysis);
        };
        if (const int result = sweepStage("standalone-cache-bounds", bounds, boundsComplete, injectionPoints)) {
            return result;
        }
        const auto local = [&](geo::WorkBudget& budget) {
            return geo::analyseCacheTopologyLocally(cubic.cache, cubic.definition, .01, budget);
        };
        const auto localComplete = [](const geo::CacheLocalTopologyResult& result) {
            return result.status == geo::CacheLocalTopologyStatus::Ok && bool(result.analysis) &&
                result.boundsStatus == geo::CacheBoundsStatus::Ok;
        };
        if (const int result = sweepStage("standalone-cache-local-topology", local, localComplete, injectionPoints)) {
            return result;
        }
    }
    for (const bool reversed : {false, true}) {
        auto circle = wholeCircleFixture(reversed);
        if (const int result = sweep(circle.cache, circle.definition,
            reversed ? "whole-circle-seam-reversed" : "whole-circle-seam", injectionPoints, .02, true, true)) {
            return result;
        }
    }
    const auto openTrimmed = openTrimmedComponentFixture();
    if (!openTrimmed) { return 11; }
    if (const int result = sweep(openTrimmed->cache, openTrimmed->definition,
        "two-component-trim-open-path", injectionPoints, .001, false, false)) { return result; }
    std::printf("allocation_failure_sweep_all_fixtures_recovered=1 injection_points=%zu\n", injectionPoints);
    return 0;
}
