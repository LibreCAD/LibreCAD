/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerclient.h"

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
