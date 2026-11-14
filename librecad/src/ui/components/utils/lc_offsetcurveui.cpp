/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurveui.h"

#include <QCoreApplication>
#include <QStringList>
#include "rs_modification.h"

namespace {
QString tr(const char* text) {
    return QCoreApplication::translate("LC_OffsetCurveUI", text);
}
QString producerText(LC_CurveOffsetStatus status) {
    // Reuse existing translations where the producer already has this wording.
    const auto offsetTr = [](const char* text) {
        return QCoreApplication::translate("LC_ActionModifyOffset", text);
    };
    switch (status) {
    case LC_CurveOffsetStatus::Ok: return tr("producer completed");
    case LC_CurveOffsetStatus::InvalidRequest: return tr("invalid producer request");
    case LC_CurveOffsetStatus::InvalidSource: return offsetTr("it cannot be offset");
    case LC_CurveOffsetStatus::UnsupportedNonPlanar: return offsetTr("it does not lie in the drawing plane");
    case LC_CurveOffsetStatus::InvalidDistance: return offsetTr("the distance is not valid");
    case LC_CurveOffsetStatus::UnsupportedClosedResult: return tr("unsupported closed result");
    case LC_CurveOffsetStatus::AmbiguousSide: return offsetTr("the point does not show which side to offset to");
    case LC_CurveOffsetStatus::UndefinedTangent:
        return offsetTr("the curve does not move, or has a point where its direction cannot be found");
    case LC_CurveOffsetStatus::DiscontinuousNormal:
        return offsetTr("the curve has a gap or a corner the offset cannot join");
    case LC_CurveOffsetStatus::SingularOffset: return tr("singular offset");
    case LC_CurveOffsetStatus::AmbiguousRegularity: return tr("unresolved offset regularity");
    case LC_CurveOffsetStatus::AmbiguousTopology:
        return offsetTr("the offset touches itself where it cannot be trimmed reliably");
    case LC_CurveOffsetStatus::FitFailed: return tr("offset fitting failed");
    case LC_CurveOffsetStatus::ToleranceNotMet: return tr("offset fitting tolerance was not met");
    case LC_CurveOffsetStatus::LimitExceeded: return tr("bounded producer limit reached");
    }
    return tr("unknown producer refusal");
}
QString checkerText(LC_OffsetCheckResult::Reason reason) {
    using Reason = LC_OffsetCheckResult::Reason;
    switch (reason) {
    case Reason::None: return tr("cache validation completed");
    case Reason::InvalidInput: return tr("invalid cache validation input");
    case Reason::Layout: return tr("unsupported cache layout");
    case Reason::TooSmall: return tr("cache is too small");
    case Reason::ReferenceUnresolved: return tr("analytic reference could not be resolved");
    case Reason::OffCurve: return tr("cache is outside the offset tolerance");
    case Reason::Ambiguous: return tr("ambiguous cache correspondence");
    case Reason::Topology: return tr("cache topology does not match the definition");
    case Reason::LimitExceeded: return tr("bounded cache validation limit reached");
    }
    return tr("unknown cache validation refusal");
}
}

QString LC_OffsetCurveUI::refusalText(const LC_OffsetCurveEditResult& result) {
    QString text;
    switch (result.refusal) {
    case LC_OffsetCurveRefusal::None: return result.changed ? tr("changed") : tr("unchanged");
    case LC_OffsetCurveRefusal::InvalidInput: text = tr("invalid native definition value"); break;
    case LC_OffsetCurveRefusal::Undefined: text = tr("the native definition is undefined"); break;
    case LC_OffsetCurveRefusal::Inconsistent:
        text = tr("cached geometry no longer matches the native definition"); break;
    case LC_OffsetCurveRefusal::TooSmall: text = tr("the result is too small"); break;
    case LC_OffsetCurveRefusal::NothingLeft: text = tr("nothing is left at this distance"); break;
    case LC_OffsetCurveRefusal::SeveralPieces: text = tr("the result has several pieces"); break;
    case LC_OffsetCurveRefusal::NotCacheRepresentable:
        text = tr("the result cannot be represented by one native cache"); break;
    case LC_OffsetCurveRefusal::CheckFailed: text = tr("cache validation refused the edit"); break;
    case LC_OffsetCurveRefusal::Engine: text = tr("the offset producer refused the edit"); break;
    case LC_OffsetCurveRefusal::ThroughNotOnCache:
        text = tr("the through point is not on the retained cache"); break;
    }
    QStringList stages;
    if (result.engineStatus) stages.append(tr("Producer: %1").arg(producerText(*result.engineStatus)));
    if (result.checkReason) stages.append(tr("Checker: %1").arg(checkerText(*result.checkReason)));
    if (!stages.isEmpty()) text += tr(" (%1)").arg(stages.join(tr("; ")));
    return text;
}

LC_OffsetRunLimits LC_OffsetCurveUI::sheetLimits() {
    const auto preview = LC_OffsetBatchLimits::preview();
    LC_OffsetRunLimits limits;
    limits.outputBudget = preview.perSource;
    limits.maxSamples = preview.maxSamples;
    limits.maxIntersectionPairs = preview.maxIntersectionPairs;
    // Keep the checker's full bounded allowance: large, valid ellipse caches need it.
    // Property edits execute in the native worker and retain the worker deadline.
    return limits;
}
