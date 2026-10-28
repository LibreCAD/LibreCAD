/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerkernel.h"

#include "rs_line.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
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
bool ordinarySpline(const RS_SplineData& data) {
    const auto degree = data.degree;
    const auto controls = data.controlPoints.size();
    if ((degree != 1 && degree != 3) || controls < degree + 1 || controls > 16384 ||
        (controls - 1) % degree || data.knotslist.size() != controls + degree + 1 ||
        data.weights.size() != controls || data.type != RS_SplineData::SplineType::ClampedOpen ||
        data.savedOpenType != data.type || !data.fitPoints.empty() || !data.savedOpenKnots.empty()) return false;
    for (const auto& point : data.controlPoints) if (!geo::isFinite2D(point)) return false;
    for (double weight : data.weights) if (weight != 1) return false;
    const auto spans = (controls - 1) / degree;
    if (spans > makeDefaultOffsetSourceBudget().maxCubicPieces) return false;
    std::size_t index = 0;
    double previous = 0;
    for (std::size_t k = 0; k <= spans; ++k) {
        const double knot = data.knotslist[index];
        if (!std::isfinite(knot) || (k && !(previous < knot))) return false;
        const auto multiplicity = k == 0 || k == spans ? degree + 1 : degree;
        for (std::size_t j = 0; j < multiplicity; ++j) if (data.knotslist[index++] != knot) return false;
        previous = knot;
    }
    return true;
}
void ordinary(result::Native& output, const LC_OffsetCurveCreateResult& made,
              const LC_OffsetSourceBudget& limits) {
    if (!made.ordinaryComplete) return;
    output.ordinaryUsage = made.ordinaryUsage;
    output.ordinaryStatus = result::OrdinaryStatus::Complete;
    output.ordinary.reserve(made.ordinary.size());
    const auto hard = makeDefaultOffsetSourceBudget();
    const auto spanCap = std::min(limits.maxCubicPieces, hard.maxCubicPieces);
    std::size_t spans = 0, controls = 0, knots = 0;
    for (const auto& entity : made.ordinary) {
        std::optional<req::Cutter> item;
        std::size_t itemSpans = 0, itemControls = 0, itemKnots = 0;
        if (const auto* line = dynamic_cast<const RS_Line*>(entity.get())) {
            const req::Segment data{line->getStartpoint(), line->getEndpoint()};
            if (geo::isFinite2D(data.start) && geo::isFinite2D(data.end)) {
                item = data; itemSpans = 1; itemControls = 2;
            }
        } else if (const auto* spline = dynamic_cast<const RS_Spline*>(entity.get())) {
            if (ordinarySpline(spline->getData())) {
                const auto& data = spline->getData();
                item = data; itemSpans = (data.controlPoints.size() - 1) / data.degree;
                itemControls = data.controlPoints.size(); itemKnots = data.knotslist.size();
            }
        }
        if (!item || itemSpans > spanCap - spans || itemControls > 16384 - controls ||
            itemKnots > 32768 - knots) {
            // Never publish a supported prefix of a producer's complete output.
            output.ordinary.clear();
            output.ordinaryStatus = result::OrdinaryStatus::Unsupported;
            return;
        }
        spans += itemSpans; controls += itemControls; knots += itemKnots;
        output.ordinary.push_back(std::move(*item));
    }
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
    if (!made) {
        ordinary(output, made, input.limits.outputBudget);
        return output;
    }
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


result::Produced LC_OffsetWorkerKernel::produce(const req::Produce& input, Trace& trace,
    const LC_OffsetPreviewSnapshot* seed) {
    // Empty shells first: no update()/constructor may repair the transported raw arrays.
    class ProduceEllipse final : public RS_Ellipse {
    public:
        explicit ProduceEllipse(const req::Produce& in) {
            m_data = std::get<RS_EllipseData>(in.source);
            m_minV = in.minimum.value_or(RS_Vector{false});
            m_maxV = in.maximum.value_or(RS_Vector{false});
        }
    };
    class ProduceSpline final : public RS_Spline {
    public:
        explicit ProduceSpline(const req::Produce& in) : RS_Spline(nullptr, RS_SplineData{}) {
            getData() = std::get<RS_SplineData>(in.source);
            m_minV = in.minimum.value_or(RS_Vector{false});
            m_maxV = in.maximum.value_or(RS_Vector{false});
        }
    };
    std::unique_ptr<RS_Entity> source;
    if (input.source.index() == 0) source = std::make_unique<ProduceEllipse>(input);
    else source = std::make_unique<ProduceSpline>(input);
    LC_OffsetPreviewCache cache;
    if (seed && !cache.restore(source.get(), *seed)) throw std::logic_error("Invalid ordinary preview cache");
    RS_OffsetData data;
    data.coord = input.coordinate; data.sideFallback = input.fallback.value_or(RS_Vector{false});
    data.distance = input.distance; data.number = input.copies; data.multipleCopies = true;
    data.asOffsetCurve = input.asOffsetCurve; data.keepOriginals = true;
    LC_OffsetSourceOutcome observed;
    observed.copiesRequested = input.copies;
    std::vector<std::unique_ptr<RS_Entity>> roots;
    bool finite = true;
    for (int i = 1; i <= input.copies; ++i) finite &= std::isfinite(double(i) * input.distance);
    if (!finite || !isValidOffsetBudget(input.limits.perSource))
        observed.status = LC_OffsetSourceStatus::OffsetFailed;
    else if (std::abs(input.distance) <= RS_TOLERANCE) {
        observed.status = LC_OffsetSourceStatus::OffsetFailed; observed.detail = LC_OffsetFailureDetail::InvalidDistance;
    } else if (!input.limits.maxDeepEntitiesPerRequest)
        observed.status = LC_OffsetSourceStatus::LimitExceeded;
    else observed.status = RS_Modification::prepareOffsetSource(data, *source, input.preview,
        input.limits, input.limits.maxDeepEntitiesPerRequest, input.context, roots, observed, seed ? &cache : nullptr);
    result::Produced output;
    auto& s = output.source;
    s.status = observed.status; s.engineStatus = observed.engineStatus; s.detail = observed.detail;
    s.stoppedBy = observed.stoppedBy; s.usage = observed.usage; s.copiesMade = observed.copiesMade;
    s.copiesRequested = observed.copiesRequested; s.sourceRemoved = observed.sourceRemoved;
    s.copiesKeptPlain = observed.copiesKeptPlain; s.rebaseEngineRuns = observed.rebaseEngineRuns;
    s.cacheEngineRuns = observed.cacheEngineRuns; s.reusedCacheCopies = observed.reusedCacheCopies;
    s.sampleUnits = observed.sampleUnits; s.intersectionPairAllowance = observed.intersectionPairAllowance;
    s.attemptedOutputUsage = observed.attemptedOutputUsage; s.attemptedDeepCountComplete = observed.attemptedDeepCountComplete;
    s.adoptionAttempts = observed.adoptionAttempts; s.adoptionCheckWork = observed.adoptionCheckWork;
    s.resolvedSide = observed.resolvedSide;
    output.copyOutputCounts = std::move(observed.copyOutputCounts);
    output.keptPlain = std::move(observed.keptPlain); output.rebases = std::move(observed.rebases);
    output.reusedCopies = std::move(observed.reusedCopies);
    if (seed) {
        auto snapshot = cache.snapshot(source.get());
        if (!snapshot) throw std::overflow_error("Ordinary preview cache exceeds transport bounds");
        output.previewCache = std::move(*snapshot);
    }
    trace.checkWork = s.adoptionCheckWork;
    for (const auto& r : output.rebases) {
        trace.checkWork.cacheSpans += r.checkWork.cacheSpans; trace.checkWork.vertices += r.checkWork.vertices;
        trace.checkWork.distancePairs += r.checkWork.distancePairs;
        trace.checkWork.orderingComparisons += r.checkWork.orderingComparisons;
        trace.checkWork.deepest = std::max(trace.checkWork.deepest, r.checkWork.deepest);
    }
    trace.checkEntered = trace.checkWork.cacheSpans || trace.checkWork.vertices ||
        trace.checkWork.distancePairs || trace.checkWork.orderingComparisons || trace.checkWork.deepest;
    trace.nativePrepareEntered = s.rebaseEngineRuns || s.adoptionAttempts;
    if (s.status == LC_OffsetSourceStatus::Succeeded) output.outputStatus = result::OrdinaryStatus::Unsupported;
    // This checks diagnostics/grouping even when no output representation can be transported.
    const auto admitted = [&] {
        return seed ? result::admittedProduced(output, input, *seed) :
            result::admittedProduced(output, input);
    };
    if (!admitted()) throw std::logic_error("Invalid source preparation receipt");
    if (s.status != LC_OffsetSourceStatus::Succeeded) return output;
    if (roots.size() != s.usage.outputEntities) throw std::logic_error("Invalid source output grouping");
    const auto hard = makeDefaultOffsetSourceBudget();
    const auto spanCap = std::min(input.limits.perSource.maxCubicPieces, hard.maxCubicPieces);
    std::size_t spans = 0, controls = 0, knots = 0;
    const auto take = [](std::size_t& total, std::size_t n, std::size_t cap) {
        if (total > cap || n > cap - total) return false;
        total += n; return true;
    };
    output.outputs.reserve(roots.size());
    for (const auto& entity : roots) {
        std::size_t n = 0, c = 0, k = 0;
        const auto* native = dynamic_cast<const LC_OffsetCurve*>(entity.get());
        const auto* spline = dynamic_cast<const RS_Spline*>(entity.get());
        const auto* line = dynamic_cast<const RS_Line*>(entity.get());
        bool supported = false;
        if (spline && ordinarySpline(spline->getData())) {
            const auto& raw = spline->getData();
            n = (raw.controlPoints.size() - 1) / raw.degree; c = raw.controlPoints.size(); k = raw.knotslist.size();
            supported = !native || (native->hasEditableDefinition() &&
                !native->cachedDefinitionGrips().empty() && native->cachedDefinitionGrips().size() <= 256);
        } else if (line) {
            n = 1; c = 2;
            supported = geo::isFinite2D(line->getStartpoint()) && geo::isFinite2D(line->getEndpoint());
        }
        if (!supported || !take(spans, n, spanCap) || !take(controls, c, 16384) || !take(knots, k, 32768)) {
            output.outputs.clear(); return output;
        }
        if (native) output.outputs.emplace_back(value(*native));
        else if (spline) output.outputs.emplace_back(spline->getData());
        else output.outputs.emplace_back(req::Segment{line->getStartpoint(), line->getEndpoint()});
    }
    output.outputStatus = result::OrdinaryStatus::Complete;
    if (!admitted()) {
        output.outputs.clear(); output.outputStatus = result::OrdinaryStatus::Unsupported;
    }
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
        case 5: reply.result = produce(std::get<req::Produce>(input.operation), trace); break;
        case 6: {
            const auto& cached = std::get<req::CachedProduce>(input.operation);
            reply.result = produce(cached.produce, trace, &cached.cache); break;
        }
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
