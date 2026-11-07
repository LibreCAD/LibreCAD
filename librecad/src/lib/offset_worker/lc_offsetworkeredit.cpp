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
    result.m_originalSource = clone.sourceDefinition();
    auto plan = clone.prepareDefinition(definition, limits);
    result.m_result = plan.result;
    if (plan.kind == LC_OffsetCurveEditPreparation::Kind::Generate)
        result.m_generate = LC_OffsetWorkerRequest::Generate{plan.definition, limits, {}};
    result.m_local = std::move(plan.local);
    return result;
}

LC_OffsetWorkerEdit::Preparation LC_OffsetWorkerEdit::prepareSourceDefinition(const LC_OffsetCurve& clone,
    const LC_OffsetSourceDefinition& definition, const LC_OffsetRunLimits& limits) {
    Preparation result;
    result.m_origin = &clone; result.m_id = clone.getId(); result.m_stamp = clone.m_stamp;
    result.m_original = clone.definition(); result.m_originalSource = clone.sourceDefinition();
    if ((!clone.hasEditableDefinition() && !clone.hasGeneralizedSourceDefinition()) ||
        !LC_OffsetWorkerRequest::admitted({{1, 0, 0, 0},
            LC_OffsetWorkerRequest::GenerateSource{definition, limits}})) {
        result.m_result.refusal = clone.isConsistent() ? LC_OffsetCurveRefusal::InvalidInput :
            LC_OffsetCurveRefusal::Inconsistent;
        return result;
    }
    result.m_generateSource = LC_OffsetWorkerRequest::GenerateSource{definition, limits};
    result.m_result = {};
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
    if (plan.m_generate || plan.m_generateSource || plan.m_membership) return {};
    if (!plan.m_result) return plan.m_result;
    const bool sourceMatches = plan.m_originalSource
        ? clone.hasGeneralizedSourceDefinition() && clone.sourceDefinition() &&
            LC_OffsetWorkerRequest::Values::encodeSourceDefinition(*plan.m_originalSource) ==
                LC_OffsetWorkerRequest::Values::encodeSourceDefinition(*clone.sourceDefinition())
        : clone.hasEditableDefinition() && LC_OffsetWorkerRequest::Values::encodeDefinition(plan.m_original) ==
            LC_OffsetWorkerRequest::Values::encodeDefinition(clone.definition());
    if (plan.m_origin != &clone || plan.m_id != clone.getId() || plan.m_stamp != clone.m_stamp || !sourceMatches) {
        LC_OffsetCurveEditResult stale; stale.refusal = LC_OffsetCurveRefusal::Inconsistent;
        return stale;
    }
    if (!plan.m_local) return plan.m_result.changed ? LC_OffsetCurveEditResult{} : plan.m_result;
    if (!plan.m_local->hasEditableDefinition() && !plan.m_local->hasGeneralizedSourceDefinition()) return {};
    LC_OffsetCurveCreateResult prepared;
    static_cast<LC_OffsetCurveEditResult&>(prepared) = plan.m_result;
    prepared.entity = std::move(plan.m_local);
    return clone.commitPrepared(prepared);
}
