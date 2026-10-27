/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerkernel.h"

#include "rs_line.h"
#include <cmath>
#include <cstring>
#include <type_traits>

namespace req = LC_OffsetWorkerRequest;
namespace result = LC_OffsetWorkerResult;
namespace geo = LC_EllipseOffsetGeometry;
namespace trim = LC_OffsetCacheTrim;
namespace {
bool same(double a, double b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }
bool same(const RS_Vector& a, const RS_Vector& b) {
    return same(a.x, b.x) && same(a.y, b.y) && same(a.z, b.z) && a.valid == b.valid;
}
template<class T> bool same(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false;
    return true;
}
bool same(const RS_SplineData& a, const RS_SplineData& b) {
    return a.degree == b.degree && a.type == b.type && a.savedOpenType == b.savedOpenType &&
        a.m_closedFlag == b.m_closedFlag && a.m_periodicFlag == b.m_periodicFlag &&
        same(a.controlPoints, b.controlPoints) && same(a.knotslist, b.knotslist) &&
        same(a.weights, b.weights) && same(a.fitPoints, b.fitPoints) && same(a.savedOpenKnots, b.savedOpenKnots);
}
result::Curve value(const LC_OffsetCurve& curve) {
    return {curve.getData(), curve.definition(), curve.checkedWorldTolerance(),
        curve.cacheFormsClosedChain(), curve.cachedDefinitionGrips(), curve.getMiddlePoint()};
}
struct Snapshot {
    LC_OffsetWorkerKernel::Trace& trace;
    const LC_OffsetCurveEditResult& observed;
    ~Snapshot() { trace.checkWork = observed.checkWork; trace.throughWork = observed.throughWork; }
};

class RawEllipse final : public RS_Ellipse {
public:
    explicit RawEllipse(const RS_EllipseData& data) { m_data = data; }
};
class RawCircle final : public RS_Circle {
public:
    explicit RawCircle(const RS_CircleData& data) { m_data = data; }
};

std::unique_ptr<RS_Entity> cutter(const req::Cutter& input) {
    return std::visit([](const auto& data) -> std::unique_ptr<RS_Entity> {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, req::Segment>)
            return std::make_unique<RS_Line>(nullptr, data.start, data.end);
        else if constexpr (std::is_same_v<T, RS_ArcData>) {
            auto shell = std::make_unique<RS_Arc>();
            shell->getData() = data;
            return shell;
        }
        else if constexpr (std::is_same_v<T, RS_CircleData>)
            return std::make_unique<RawCircle>(data);
        else if constexpr (std::is_same_v<T, RS_EllipseData>)
            return std::make_unique<RawEllipse>(data);
        else {
            // This private adapter is never drawn. Assign the original value
            // AFTER constructing an empty ordinary shell, so update() cannot
            // repair untrusted knots/weights before trim's own admission.
            auto shell = std::make_unique<RS_Spline>(nullptr, RS_SplineData{});
            shell->getData() = data;
            return shell;
        }
    }, input);
}
}

result::Native LC_OffsetWorkerKernel::generate(const req::Generate& input, Trace& trace) {
    const RS_Line attributes{nullptr, {0, 0}, {1, 0}};
    result::Native output;
    const Snapshot observed{trace, output};
    // runEngine normalizes the raw basis before any ellipse constructor.
    trace.nativePrepareEntered = true;
    auto made = LC_OffsetCurve::runEngine(attributes, input.definition, input.limits);
    static_cast<LC_OffsetCurveEditResult&>(output) = made;
    output.attemptedOutputUsage = made.attemptedOutputUsage;
    output.attemptedDeepCountComplete = made.attemptedDeepCountComplete;
    output.outputUsage = made.outputUsage;
    output.sampleUnits = made.sampleUnits;
    trace.checkEntered = made.checkReason.has_value();
    if (made && input.throughPoint) {
        trace.throughEntered = true;
        if (!LC_OffsetCurve::checkThrough(*made.entity, *input.throughPoint, input.limits.checkLimits,
                                         output.checkWork, output.throughWork)) {
            output.refusal = LC_OffsetCurveRefusal::ThroughNotOnCache;
            output.changed = false;
            return output;
        }
    }
    if (made) output.curve = value(*made.entity);
    return output;
}

result::Native LC_OffsetWorkerKernel::validate(const req::ValidateCache& input, Trace& trace) {
    result::Native output;
    const Snapshot observed{trace, output};
    auto definition = input.definition;
    if (!geo::normaliseBasis(definition.basis)) return output;
    geo::WorkBudget budget{input.limits};
    trace.checkEntered = true;
    LC_OffsetCheckResult checked;
    try { checked = geo::checkCache(input.cache, definition, input.worldTolerance, budget); }
    catch (...) { output.checkWork = budget.consumed(); throw; }
    output.checkWork = checked.work;
    output.checkReason = checked.reason;
    output.refusal = LC_OffsetCurveRefusal::CheckFailed;
    if (!checked) return output;

    // Only a successful C result reaches the private native constructor. The
    // proof stays in this process; no pointer/issuer is serialized to the GUI.
    output.refusal = LC_OffsetCurveRefusal::NotCacheRepresentable;
    const auto display = RS_Spline::displayEntityUpperBound(input.cache);
    if (!display || *display > makeDefaultOffsetSourceBudget().maxDeepEntities) return output;
    const RS_Line attributes{nullptr, {0, 0}, {1, 0}};
    std::unique_ptr<LC_OffsetCurve> native{new LC_OffsetCurve(attributes, input.cache, definition,
        input.worldTolerance, *checked.analysis, LC_OffsetCurve::ValidatedCache{})};
    if (!native->validate() || !same(input.cache, native->getData()) ||
        native->cachedDefinitionGrips().empty() || !geo::isFinite2D(native->getMiddlePoint()) ||
        !native->hasEditableDefinition()) return output;
    output.curve = value(*native);
    output.refusal = LC_OffsetCurveRefusal::None;
    output.changed = true;
    return output;
}

result::Native LC_OffsetWorkerKernel::checkThrough(const req::CheckThrough& input, Trace& trace) {
    result::Native output;
    const Snapshot observed{trace, output};
    auto definition = input.definition;
    if (!std::isfinite(input.worldTolerance) || input.worldTolerance <= 0 ||
        !geo::normaliseBasis(definition.basis)) return output;
    trace.throughEntered = true;
    output.refusal = LC_OffsetCurve::checkThroughCache(input.cache, input.worldTolerance, input.point,
        input.limits, output.throughWork) ? LC_OffsetCurveRefusal::None : LC_OffsetCurveRefusal::ThroughNotOnCache;
    return output;
}

result::Reply LC_OffsetWorkerKernel::run(const req::Request& input, Trace* observed) noexcept {
    Trace local;
    Trace& trace = observed ? *observed : local;
    trace = {};
    result::Reply reply;
    reply.token = input.token;
    reply.operation = input.operation.index();
    try {
        if (!req::admitted(input)) return reply;
        switch (input.operation.index()) {
        case 0: reply.result = generate(std::get<req::Generate>(input.operation), trace); break;
        case 1: reply.result = validate(std::get<req::ValidateCache>(input.operation), trace); break;
        case 2: {
            const auto& op = std::get<req::TrimCache>(input.operation);
            auto operand = cutter(op.cutter);
            trace.trimEntered = true;
            reply.result = trim::prepare(op.cache, *operand, op.pick, op.limits, {}, &trace.trimWork);
            break;
        }
        case 3: {
            const auto& op = std::get<req::TrimConic>(input.operation);
            auto operand = cutter(op.conic);
            trace.trimEntered = true;
            reply.result = trim::prepareConic(op.cache, *operand, op.conicPick, op.cachePick,
                                             op.limits, {}, &trace.trimWork);
            break;
        }
        case 4: reply.result = checkThrough(std::get<req::CheckThrough>(input.operation), trace); break;
        default: return reply;
        }
        trace.computationCompleted = true;
        reply.failure = result::Failure::None;
    } catch (const std::bad_alloc&) {
        reply.result = std::monostate{};
        reply.failure = result::Failure::AllocationFailure;
    } catch (...) {
        reply.result = std::monostate{};
        reply.failure = result::Failure::InternalFailure;
    }
    return reply;
}
