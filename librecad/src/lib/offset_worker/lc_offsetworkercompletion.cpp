/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerclient.h"
#include "lc_offsetcurvexdata.h"
#include "rs_line.h"

#include <algorithm>

LC_OffsetWorkerCompletion::LC_OffsetWorkerCompletion(LC_OffsetWorkerRequest::Request&& request,
    LC_OffsetWorkerResult::Reply&& reply) : m_request(std::move(request)), m_reply(std::move(reply)) {}

std::unique_ptr<LC_OffsetCurve> LC_OffsetWorkerCompletion::native(const RS_Entity& attributes) const {
    const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&m_reply.result);
    if (!result || !*result || !result->curve) return {};
    const auto& c = *result->curve;
    const auto display = RS_Spline::displayEntityUpperBound(c.cache);
    if (!display || *display > makeDefaultOffsetSourceBudget().maxDeepEntities) return {};
    std::unique_ptr<LC_OffsetCurve> curve{new LC_OffsetCurve(attributes, c.cache, c.definition,
        c.worldTolerance, c.closed, c.grips, c.midpoint, LC_OffsetCurve::ReceivedCache{})};
    // Materialization must retain the admitted original cache, not repair it.
    if (!curve->validate() || !curve->hasEditableDefinition() ||
        RS_Spline::geometryFingerprint(curve->getData()) != RS_Spline::geometryFingerprint(c.cache)) return {};
    if (const auto* previous = dynamic_cast<const LC_OffsetCurve*>(&attributes))
        if (!curve->setOwnedExtensions(previous->unknownOwnedFlags(), previous->unknownOwnedTail())) return {};
    return curve;
}

std::optional<std::vector<std::unique_ptr<RS_Entity>>>
LC_OffsetWorkerCompletion::ordinary(const RS_Entity& attributes) const {
    const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&m_reply.result);
    const auto* generate = std::get_if<LC_OffsetWorkerRequest::Generate>(&m_request.operation);
    if (!generate || generate->throughPoint || !result ||
        result->ordinaryStatus != LC_OffsetWorkerResult::OrdinaryStatus::Complete)
        return std::nullopt;
    if (!attributes.cloneMetadataFits(1024, std::size_t{1} << 20, 4096)) return std::nullopt;
    LC_OffsetCurveXData::Options metadataLimits;
    metadataLimits.maximumItems = 4096; metadataLimits.maximumBytes = std::size_t{1} << 20;
    const auto foreign = LC_OffsetCurveXData::parseAndStrip(attributes.getDrwExtData(), metadataLimits);
    if (!foreign.remaining) return std::nullopt;
    auto cap = makeDefaultOffsetSourceBudget();
    cap.maxCubicPieces = std::min(cap.maxCubicPieces, generate->limits.outputBudget.maxCubicPieces);
    cap.maxOutputEntities = std::min(cap.maxOutputEntities, generate->limits.outputBudget.maxOutputEntities);
    cap.maxDeepEntities = std::min(cap.maxDeepEntities, generate->limits.outputBudget.maxDeepEntities);
    std::vector<std::unique_ptr<RS_Entity>> entities;
    entities.reserve(result->ordinary.size());
    std::size_t deepEntities = 0;
    for (const auto& value : result->ordinary) {
        std::unique_ptr<RS_Entity> entity;
        if (const auto* line = std::get_if<LC_OffsetWorkerRequest::Segment>(&value))
            entity = std::make_unique<RS_Line>(nullptr, RS_LineData{line->start, line->end});
        else if (const auto* data = std::get_if<RS_SplineData>(&value)) {
            const auto display = RS_Spline::displayEntityUpperBound(*data);
            if (!display || *display > cap.maxDeepEntities - deepEntities) return std::nullopt;
            auto spline = std::make_unique<RS_Spline>(nullptr, *data);
            if (!spline->validate() || RS_Spline::geometryFingerprint(spline->getData()) !=
                RS_Spline::geometryFingerprint(*data)) return std::nullopt;
            entity = std::move(spline);
        } else return std::nullopt;
        entity->RS_Entity::operator=(attributes);
        entity->setDrwExtData(*foreign.remaining);
        entity->setLayer(attributes.getLayer(false));
        entity->clearDwgProvenance(RS_Entity::Identity);
        entity->setLineage(RS_Entity::newLineage()); entity->setOwnerLineage(0);
        entity->delFlag(RS2::FlagHatchChild);
        entity->calculateBorders();
        // Parent display expansion is independently bounded, even though the
        // child's ordinary geometry was admitted before serializing it.
        const auto cost = measureOffsetOutput({entity.get()}, cap.maxDeepEntities - deepEntities);
        if (cost.status != LC_OffsetTreeStatus::Ok) return std::nullopt;
        deepEntities += cost.deepEntities;
        entities.push_back(std::move(entity));
    }
    return entities;
}

LC_OffsetCurveEditResult LC_OffsetWorkerCompletion::apply(LC_OffsetCurve& clone) const {
    const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&m_reply.result);
    if (!result) return {};
    auto outcome = static_cast<const LC_OffsetCurveEditResult&>(*result);
    if (!*result) return outcome;
    const auto eligibility = clone.eligibility();
    if (!eligibility) {
        outcome.refusal = eligibility.refusal; outcome.changed = false;
        return outcome; // The child already ran: retain its actual stage work.
    }
    if (const auto* membership = std::get_if<LC_OffsetWorkerRequest::CheckThrough>(&m_request.operation)) {
        // A matched membership receipt can only acknowledge this unchanged raw
        // cache. It cannot mint a native curve or publish a different definition.
        if (clone.checkedWorldTolerance() != membership->worldTolerance ||
            LC_OffsetWorkerRequest::Values::encodeCache(clone.getData()) !=
                LC_OffsetWorkerRequest::Values::encodeCache(membership->cache) ||
            LC_OffsetWorkerRequest::Values::encodeDefinition(clone.definition()) !=
                LC_OffsetWorkerRequest::Values::encodeDefinition(membership->definition))
            outcome.refusal = LC_OffsetCurveRefusal::Inconsistent;
        outcome.changed = false;
        return outcome;
    }
    LC_OffsetCurveCreateResult prepared;
    static_cast<LC_OffsetCurveEditResult&>(prepared) = *result;
    prepared.entity = native(clone);
    if (!prepared.entity) {
        prepared.refusal = LC_OffsetCurveRefusal::NotCacheRepresentable;
        prepared.changed = false;
    }
    return clone.commitPrepared(prepared);
}
