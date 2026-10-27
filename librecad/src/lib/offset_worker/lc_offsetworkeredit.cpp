/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkeredit.h"

LC_OffsetWorkerEdit::Preparation LC_OffsetWorkerEdit::prepare(const LC_OffsetCurve& clone,
    const LC_EllipseOffsetDefinition& definition, const LC_OffsetRunLimits& limits) {
    Preparation result;
    result.m_origin = &clone; result.m_id = clone.getId(); result.m_stamp = clone.m_stamp;
    result.m_original = clone.definition();
    auto plan = clone.prepareDefinition(definition, limits);
    result.m_result = plan.result;
    if (plan.kind == LC_OffsetCurveEditPreparation::Kind::Generate)
        result.m_generate = LC_OffsetWorkerRequest::Generate{plan.definition, limits, {}};
    result.m_local = std::move(plan.local);
    return result;
}

LC_OffsetWorkerEdit::Preparation LC_OffsetWorkerEdit::prepareThrough(const LC_OffsetCurve& clone,
    const RS_Vector& point, const LC_OffsetRunLimits& limits) {
    Preparation result;
    result.m_origin = &clone; result.m_id = clone.getId(); result.m_stamp = clone.m_stamp;
    result.m_original = clone.definition();
    auto plan = clone.prepareThrough(point, limits);
    result.m_result = plan.result;
    if (plan.kind == LC_OffsetCurveEditPreparation::Kind::Generate)
        result.m_generate = LC_OffsetWorkerRequest::Generate{plan.definition, limits, point};
    else if (plan.kind == LC_OffsetCurveEditPreparation::Kind::CheckThrough)
        result.m_membership = LC_OffsetWorkerRequest::CheckThrough{clone.getData(), clone.definition(),
            clone.checkedWorldTolerance(), point, limits.checkLimits};
    return result;
}

LC_OffsetCurveEditResult LC_OffsetWorkerEdit::applyLocal(LC_OffsetCurve& clone, Preparation&& plan) {
    if (plan.m_generate || plan.m_membership) return {};
    if (!plan.m_result) return plan.m_result;
    if (plan.m_origin != &clone || plan.m_id != clone.getId() || !clone.hasEditableDefinition() ||
        plan.m_stamp != clone.m_stamp || LC_OffsetWorkerRequest::Values::encodeDefinition(plan.m_original) !=
        LC_OffsetWorkerRequest::Values::encodeDefinition(clone.definition())) {
        LC_OffsetCurveEditResult stale; stale.refusal = LC_OffsetCurveRefusal::Inconsistent;
        return stale;
    }
    if (!plan.m_local) return plan.m_result.changed ? LC_OffsetCurveEditResult{} : plan.m_result;
    if (!plan.m_local->hasEditableDefinition()) return {};
    LC_OffsetCurveCreateResult prepared;
    static_cast<LC_OffsetCurveEditResult&>(prepared) = plan.m_result;
    prepared.entity = std::move(plan.m_local);
    return clone.commitPrepared(prepared);
}
